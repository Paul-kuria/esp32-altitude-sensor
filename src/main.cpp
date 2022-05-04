#include "RTClib.h"
#include "Wire.h"
#include "SPI.h"
#include <Arduino.h>
#include "driver/gpio.h"
//MicroSD Libraries {File System}
#include "FS.h" 
#include "SD_MMC.h"

/*-- INITIALIZE DS3231 RTC --*/
RTC_DS3231 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};


/*-- INITIALIZE DS3231 RTC --*/
unsigned int fileCount = 0;

/***----------------------------- MICRO-SD MODULE --------------------------***/
void initMicroSD(){  
  //rtc_gpio_hold_en(GPIO_NUM_4);    //make sure flash is held LOW in sleep
  Serial.println("Mounting MicroSD card");

  // if (!SD_MMC.begin()){
  //   Serial.println("MicroSD card mount failed");
  //   return;
  // }
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

/***-- CREATE FOLDER--***/
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
/***-- APPEND FILE--***/
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

/***-- LIST DIRECTORIES--***/
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


void setup () {
  Serial.begin(115200);
  bool begin(const char * mountpoint="/sdcard", bool mode1bit=true); // cf. SD_MMC.h
  SD_MMC.begin("/sdcard", true);
  Wire.begin(16, 13, 100000);
  SD_MMC.begin("/sdcard", true);
  // pinMode(4, OUTPUT);
  // digitalWrite(4, LOW);                                                                                                                                                                                        );
  delay(3000); // wait for console opening
  Serial.println("\nTroubleshoot sd card pinout");
    

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
  DateTime now = rtc.now();

  String date = String(now.day()) + ":" + String(now.month()) + ":" + String(now.year());
  String time = String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second());
  String dataMessage = date + "," + time + "," + "\r\n";

  Serial.println("saving data: ");
  Serial.println("----------------------------------------------");
  Serial.println(dataMessage);

  appendFile(SD_MMC, "/example2_data.txt", dataMessage.c_str());
  delay(1000);
}