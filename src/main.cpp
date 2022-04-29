// Camera libraries
#include "esp_camera.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "driver/rtc_io.h"

//MicroSD Libraries {File System}
#include "FS.h" 
#include "SD_MMC.h"

//EEPROM Library
#include "EEPROM.h"
 //setup button
int count = 0;
//use 1 byte of EEPROM space
#define EEPROM_SIZE 1

//counter for file number
unsigned int fileCount = 0;
//Pin Definitions for CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM   32  //power
#define RESET_GPIO_NUM  -1
#define XCLK_GPIO_NUM    0
#define SIOD_GPIO_NUM   26  //sda
#define SIOC_GPIO_NUM   27  //scl
#define Y9_GPIO_NUM     35  //d7
#define Y8_GPIO_NUM     34  //d6
#define Y7_GPIO_NUM     39  //d5
#define Y6_GPIO_NUM     36  //d4
#define Y5_GPIO_NUM     21  //d3
#define Y4_GPIO_NUM     19  //d2
#define Y3_GPIO_NUM     18  //d1
#define Y2_GPIO_NUM     5   //d0
#define VSYNC_GPIO_NUM  25
#define HREF_GPIO_NUM   23
#define PCLK_GPIO_NUM   22


void initMicroSD() {
    //start SD card
    Serial.println("Mounting MicroSD Card");
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
    
    /*----------------- card size -------------------*/
    uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
    Serial.printf("SD_MMC Card Size: %lluMB\n", cardSize);

}

/*-- CREATE FOLDER -- */
void createDir(fs::FS &fs, char * path){
    Serial.printf("Creating Dir: %s\n", path);
    if(fs.mkdir(path)){
        Serial.println("Dir created");
    }
    else{
        Serial.println("mkdir failed");
    }
}
/*-- CREATE FILES --*/
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
/*-- LIST DIRECTORIES --*/
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
char *string2char (String command){
    if (command.length()!=0){
        char p[command.length()+1];
        strcpy(p, command.c_str());
        for (int i=0; i<command.length(); i++){
            p[i];
        }
        return p;
    }
}

void setup() {
    Serial.begin(115200);
    //Init micro_sd
    initMicroSD();

    // CALL: create folder
    char foldername[20] = "/distance-folder";
    //char filename[20] = "/example0001.txt";
    char * fdir = foldername;
    createDir(SD_MMC, fdir);

    // CALL: create file in folder
    String note = "In our case, the ESP32-CAM outputs 3.3V whether it is powered with 5V or 3.3V. Next to the VCC pin, there are two pads. One labeled as 3.3V and other as 5V.";
    char dest[45];
    char filename[20];
    for (count=0; count < 5; count++){
        /*convert string filepath to a char array first*/
        String filepath = "/example" + String(fileCount) + ".txt";
        char* p = new char[filepath.length()+1];
        memcpy(p, filepath.c_str(), filepath.length()+1);
        /*concat path*/
        strcpy(dest, foldername);
        strcpy(dest, p);
        char * fname = dest;
        createFiles(SD_MMC, fname, note);
        fileCount++;
    }
    
    // CALL: READ FILES
    listDir(SD_MMC,"/", 0);
}

void loop(){
    
    }


