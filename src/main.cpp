#include "OMObject.h"
#include "Debug.h"
#include "Lights.h"
#include "Rectenna.h"
#include "Sound.h"
#include "Ramp.h"
#include <esp_now.h>
#include <WiFi.h>
#include "SPIFFS.h"
#include "ESPNAgent.h"

class FRoot : public Root
{
public:
	FRoot(bool isDevice) : Root(isDevice, 'R', "Root", nullptr) { }
    void    ReceivedFile(String fileName) override { FileReceived = fileName; }
    void	Run() override
    {
        Root::Run();
        if (!FileReceived.isEmpty())
        {
            // flogv("File transfer complete");
            Sound::GetInstance().Play(FileReceived);
            FileReceived = "";
        }
    }
private:
    String FileReceived;
};

FRoot root(true);   // this is the device to be controlled
ESPNAgent agent(&SPIFFS, &root);

// #include <nvs_flash.h>

// our Mac Address = {0xE8, 0x9F, 0x6D, 0x20, 0x7D, 0x28}

uint8_t peerMacAddress[] = {0xD8, 0x3B, 0xDA, 0x87, 0x52, 0x58};

int flog_printer(const char* s)
{
    int len = Serial.print(s);
    // echo the message to the esp_now peer
    agent.SendCmd(s);
    return len;
}

void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\n", dirname);
  
    File root = fs.open(dirname);
    if(!root){
      Serial.println("Failed to open directory");
      return;
    }
    if(!root.isDirectory()){
      Serial.println("Not a directory");
      return;
    }
  
    File file = root.openNextFile();
    while(file){
      if(file.isDirectory()){
        Serial.print("  DIR : ");
        Serial.println(file.name());
        if(levels){
          listDir(fs, file.name(), levels -1);
        }
      } else {
        Serial.print("  FILE: ");
        Serial.print(file.name());
        Serial.print("  SIZE: ");
        Serial.println(file.size());
      }
      file = root.openNextFile();
    }
  }

#include "OMDef.h"


void setup()
{
    // nvs_flash_erase();      // erase the NVS partition and...
    // nvs_flash_init();       // initialize the NVS partition.
    // while (true);
    Serial.begin(115200);
    delay(2000);
    FLogger::setLogLevel(FLOG_VERBOSE);
    flogv("Falcon rcvr " __DATE__ " " __TIME__);
    // FLogger::setPrinter(flog_printer);

    if (!SPIFFS.begin())
    {
        floge("SPIFFS init error");
    }
    else
    {
        listDir(SPIFFS, "/", 0);
        Serial.printf("Total space: %lu\n", SPIFFS.totalBytes());
        Serial.printf("Used space: %lu\n", SPIFFS.usedBytes());
    }

    agent.Setup(peerMacAddress);

    root.Setup(&agent);
    Rectenna::GetInstance().Setup();
    Ramp::GetInstance().Setup();
    Sound::GetInstance().Setup();
    // I2S audio initialization seems to mess with PIN_NEOPIXEL (pin 0).
    // So we need to setup Sounds first BEFORE Lights setup fixes pin 0!
    // Using that order, both seem to work properly.
    Lights::GetInstance().Setup();
    Debug::GetInstance().Setup();
    root.AddObjects(Objects);
    flogv("Setup done");
}

void loop()
{
    root.Run();
    Rectenna::GetInstance().Run();
    Ramp::GetInstance().Run();
    Sound::GetInstance().Run();
    Lights::GetInstance().Run();
    Debug::GetInstance().Run();
    agent.Run();
}
