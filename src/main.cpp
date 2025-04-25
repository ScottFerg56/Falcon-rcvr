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
#include "rcvr.h"

long mapr(long x, long in_min, long in_max, long out_min, long out_max)
{
    const long run = in_max - in_min;
    const long rise = out_max - out_min;
    const long delta = x - in_min;
    long n = delta * rise;
    return ((long)std::round(static_cast<double>(n) / run)) + out_min;
}

class RootConnector : public OMConnector
{
public:
    void Init(OMObject *obj) override {}
    void Push(OMObject *obj, OMProperty *prop) override
    {
        auto id = prop->Id;
        switch (id)
        {
        case 'x':   // restart
            esp_restart();
            break;
        }
    }
    void Pull(OMObject *obj, OMProperty *prop) override
    {
        auto id = prop->Id;
        switch (id)
        {
        case 'f':   // free space
            ((OMPropertyLong*)prop)->Value = SPIFFS.totalBytes() - SPIFFS.usedBytes();
            break;
        }
    }
};

RootConnector RootConn;

#include "OMDef.h"

class FRoot : public Root
{
public:
	FRoot(bool isDevice) : Root(isDevice, 'R', "Root", &RootConn), Metro(1000) { }
    void    ReceivedFile(String fileName) override { FileReceived = fileName; }
    void    Setup(Agent* pagent) override
    {
        // pinMode(Pin_Main_Switch, INPUT_PULLUP);
        AddProperties(RootProps);
        Root::Setup(pagent);
    }

    void	Run() override
    {
        Root::Run();
        if (!FileReceived.isEmpty())
        {
            Sound::GetInstance().ReceivedFile(FileReceived);
            FileReceived = "";
            GetProperty('f')->PullSend();
        }
        if (Metro)
        {
            // flogv("main switch read");
            pinMode(Pin_Main_Switch, INPUT_PULLUP);
            bool on = !digitalRead(Pin_Main_Switch);
            if (on != MainSwitchOn)
            {
                MainSwitchOn = on;
                if (on)
                {
                    flogv("main switch on");
                    GetObject('l')->TraverseObjects([](OMObject* o) {
                        auto p = o->GetProperty('o');
                        if (p)
                        {
                            ((OMPropertyBool*)p)->Set(true);
                            p->Send();
                        }
                    });
                }
                else
                {
                    flogv("main switch off");
                    Command("<R");
                    Command("?R");
                }
            }
        }
    }

    void    ConnectionChanged(bool connected)
    {
        Root::ConnectionChanged(connected);
        Sound::GetInstance().Play(connected ? "/Cheerful R2D2.mp3" : "/Sad R2D2.mp3");
    }
private:
	Metronome	Metro;
    bool MainSwitchOn = false;
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

    Rectenna::GetInstance().Setup();
    Ramp::GetInstance().Setup();
    Sound::GetInstance().Setup();
    // I2S audio initialization seems to mess with PIN_NEOPIXEL (pin 0).
    // So we need to setup Sounds first BEFORE Lights setup fixes pin 0!
    // Using that order, both seem to work properly.
    Lights::GetInstance().Setup();
    Debug::GetInstance().Setup();
    
    root.AddObjects(Objects);
    root.Setup(&agent);
    Sound::GetInstance().Play("/Startup.mp3");
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
