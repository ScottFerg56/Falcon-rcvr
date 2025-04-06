#include "OMObject.h"
#include "Debug.h"
#include "Lights.h"
#include "Rectenna.h"
#include "Sounds.h"
#include "Ramp.h"
#include <esp_now.h>
#include <WiFi.h>
#include "SPIFFS.h"
#include "Agent.h"

Root root;

// #include <nvs_flash.h>

// our Mac Address = {0xE8, 0x9F, 0x6D, 0x20, 0x7D, 0x28}

uint8_t peerMacAddress[] = {0xD8, 0x3B, 0xDA, 0x87, 0x52, 0x58};

int flog_printer(const char* s)
{
    int len = Serial.print(s);
    // echo the message to the esp_now peer
    len = strlen(s);
    Agent::GetInstance().SendData((uint8_t*)s, len);
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

void setup()
{
    // nvs_flash_erase();      // erase the NVS partition and...
    // nvs_flash_init();       // initialize the NVS partition.
    // while (true);
    Serial.begin(115200);
    delay(2000);
    FLogger::setLogLevel(FLOG_VERBOSE);
    // FLogger::setPrinter(flog_printer);
    root.AddObject(new Debug());
    root.AddObject(new Sounds());
    // I2S audio initialization seems to mess with PIN_NEOPIXEL (pin 0).
    // So we need to setup Sounds first BEFORE Lights setup fixes pin 0!
    // Using that order, both seem to work properly.
    root.AddObject(new Lights());
    root.AddObject(new Ramp());
    root.AddObject(new Rectenna());

    root.SetSend([](String cmd) { Agent::GetInstance().SendCmd(cmd); });

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

    Agent::GetInstance().Setup(&SPIFFS, peerMacAddress, [](String cmd) { root.Command(cmd); });

    root.Setup();
    flogv("Setup done");
}

void loop()
{
    root.Run();
    if (!Agent::GetInstance().FileReceived.isEmpty())
    {
        // flogv("File transfer complete");
        ((Sounds*)root.GetObject('s'))->Play(Agent::GetInstance().FileReceived);
        Agent::GetInstance().FileReceived = "";
    }

    Agent::GetInstance().Loop();

}
