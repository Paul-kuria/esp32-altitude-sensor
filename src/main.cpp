#include "RTClib.h"
#include "Wire.h"
#include "SPI.h"
#include <Arduino.h>
#include "driver/gpio.h"
//TFlidar Libraries
#include "SoftwareSerial.h"
#include "TFMini.h"
//MicroSD Libraries {File System}
#include "FS.h" 
#include "SD_MMC.h"

/*-- INITIALIZE DS3231 RTC --*/
RTC_DS3231 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

/*-- INITIALIZE TF-MINI LIDAR --*/
TFMini tfmini;

#define RXD2 14
#define TXD2 15
int dist; /*----actual distance measurements of LiDAR---*/
int strength; /*----signal strength of LiDAR----------------*/
float temprature;
unsigned char check;        /*----save check value------------------------*/
int i;
unsigned char uart[9];  /*----save data measured by LiDAR-------------*/
const int HEADER=0x59; /*----frame header of data package------------*/
int rec_debug_state = 0x01;//receive state for frame

/*-- INITIALIZE DS3231 RTC --*/
unsigned int fileCount = 0;

/*----------------------------- MICRO-SD MODULE --------------------------*/
void initMicroSD(){  
  
  //rtc_gpio_hold_en(GPIO_NUM_4);    //make sure flash is held LOW in sleep
  Serial.println("Mounting MicroSD card");
  bool begin(const char * mountpoint="/sdcard", bool mode1bit=false); // cf. SD_MMC.h
  SD_MMC.begin("/sdcard", true);
  if (!SD_MMC.begin()){
    Serial.println("MicroSD card mount failed");
    return;
  }
  uint8_t cardType = SD_MMC.cardType();
  if (cardType == CARD_NONE){
    Serial.println("No MicroSD card found");
    return;  
  }
  /*----------------- card type -------------------*/
  Serial.print("SD_MMC card type is: ");
    if(cardType == CARD_MMC){
        Serial.println("MMC");
    }
    else if (cardType == CARD_SD){
        Serial.println("SD");
    }
    else if (cardType == CARD_SDHC){
        Serial.println("SDHC");
    }
    else {
        Serial.println("Unknown");
    }
    uint64_t cardSize = SD_MMC.cardSize()/ (1024*1024);
    Serial.printf("SD_MMC Card Size: %lluMB\n", cardSize);
}
/*-- CREATE FOLDER--*/
void createDir(fs::FS &fs, char * path){
    Serial.printf("Creating Dir: %s\n", path);
    if(fs.mkdir(path)){
        Serial.println("Dir created");
    }
    else{
        Serial.println("mkdir failed");
    }
}
/*-- CREATE FILE--*/
void createFiles(fs::FS &fs, const char * path, String message){
    Serial.printf("Create file here: %s\n", path);
    File file = fs.open(path, FILE_WRITE);
    if (!file){
        Serial.println("Failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("File written successfully");
    }
    else{
        Serial.println("Write to file failed!");
    }
    file.close();

}
/*-- APPEND FILE--*/
void appendFile(fs::FS &fs, const char *path, const char *message){
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if(!file){
    Serial.println("Failed to open file for appending");
    return;
  }
  if(file.print(message)){
    Serial.println("Message appended");
  }
  else {
    Serial.println("Append failed");
  }
  file.close();
}

/*-- LIST DIRECTORIES--*/
void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("List directories: %s\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("Folder failed to open!");
        return;
    }
    if(!root.isDirectory()){
        Serial.println("Folder is not a directory");
        return;
    }
    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR: "); 
            Serial.println(file.name());
            if (levels){
                listDir(fs, file.name(), levels -1);
            }
        }
        else{
            Serial.print("File NAME: ");
            Serial.print(file.name());
            Serial.print("  File SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

/*----------------------------- LIDAR/RTC --------------------------*/
void Get_Lidar_data(){
  DateTime now = rtc.now();
  if (Serial2.available()) //check if serial port has data input
      {
      if(rec_debug_state == 0x01)
          {  //the first byte
            uart[0]=Serial2.read();
            if(uart[0] == 0x59)
                {
                  check = uart[0];
                  rec_debug_state = 0x02;
                }
          }
  else if(rec_debug_state == 0x02)
      {//the second byte
        uart[1]=Serial2.read();
        if(uart[1] == 0x59)
            {
              check += uart[1];
              rec_debug_state = 0x03;
            }
        else{
              rec_debug_state = 0x01;
            }
        }

  else if(rec_debug_state == 0x03)
          {
            uart[2]=Serial2.read();
            check += uart[2];
            rec_debug_state = 0x04;
          }
  else if(rec_debug_state == 0x04)
          {
            uart[3]=Serial2.read();
            check += uart[3];
            rec_debug_state = 0x05;
          }
  else if(rec_debug_state == 0x05)
          {
            uart[4]=Serial2.read();
            check += uart[4];
            rec_debug_state = 0x06;
          }
  else if(rec_debug_state == 0x06)
          {
            uart[5]=Serial2.read();
            check += uart[5];
            rec_debug_state = 0x07;
          }
  else if(rec_debug_state == 0x07)
          {
            uart[6]=Serial2.read();
            check += uart[6];
            rec_debug_state = 0x08;
          }
  else if(rec_debug_state == 0x08)
          {
            uart[7]=Serial2.read();
            check += uart[7];
            rec_debug_state = 0x09;
          }
  else if(rec_debug_state == 0x09)
          {
            uart[8]=Serial2.read();
            if(uart[8] == check)
              {
                dist = uart[2] + uart[3]*256;//the distance
                strength = uart[4] + uart[5]*256;//the strength
                temprature = uart[6] + uart[7] *256;//calculate chip temprature
                temprature = temprature/8 - 256;                              
                while(Serial2.available()){Serial2.read();} // This part is added becuase some previous packets are there in the buffer so to clear serial buffer and get fresh data.
                
                String date = String(now.day()) + ":" + String(now.month()) + ":" + String(now.year());
                String time = String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second());
                String tf = String(dist) + "," + String(strength) + "," + String(temprature);
                String dataMessage = date + "," + time + "," + tf + "\r\n";

                Serial.print("saving data: ");
                Serial.println("----------------------------------------------");
                Serial.println(dataMessage);

                appendFile(SD_MMC, "/example1_data.txt", dataMessage.c_str());
                delay(1000);
              }
            rec_debug_state = 0x01;
        }
    }
}
void setup () {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1 ,RXD2, TXD2);
  Wire.begin(16, 4, 400000);

  pinMode(4, OUTPUT);              //GPIO for LED flash
  digitalWrite(4, LOW);            //turn OFF flash LED
  
  delay(3000); // wait for console opening
  Serial.println("\nTFmini-S UART LiDAR Program");
  

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, lets set the time!");
    // following line sets the RTC to the date &amp; time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  // CALL:
  initMicroSD();

    // CALL: create folder
  char foldername[20] = "/distance-folder";
  //char filename[20] = "/example0001.txt";
  char * fdir = foldername;
  createDir(SD_MMC, fdir);

  // CALL: create file in folder
  String note = "Date, Time(UTC+3:00), Distance, Strength, TF-temperature \r\n";
  createFiles(SD_MMC, "/example1_data.txt", note);
  
  // CALL: READ FILES
  listDir(SD_MMC,"/", 0);
}


void loop () {
 
    Get_Lidar_data();

}