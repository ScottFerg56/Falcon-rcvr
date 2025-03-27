#include "rcvr.h"
#include <Arduino.h>
#include "FLogger.h"
#include "Sounds.h"
#include "SPIFFS.h"
#include <Audio.h>

Audio audio;

class SPropPlay : public OMPropertyLong
{
public:
    SPropPlay() {}

    char GetID() { return 'p'; }
    const char* GetName() { return "Play"; }

    long GetMin() { return 0; }
    long GetMax() { return 100; }

    void Set(long value)
    {
        // Open music file
        bool fret = audio.connecttoFS(SPIFFS,"/Cheerful R2D2.mp3");
        if (!fret)
        {
            floge("audio connect error");
            return;
        }
        auto s = audio.getFileSize();
        flogd("audio size %d", s);
    }

    long Get() { return 0; }
};

void Sounds::Setup()
{
    AddProperty(new SPropPlay());
    if (!SPIFFS.begin())
    {
        floge("SPIFFS init error");
        return;
    }

    // File file = SPIFFS.open("/test.txt");
    // if (file.size() == 0)
    // {
    //     floge("Failed to open file for reading");
    //     return;
    // }
    
    // Serial.println("File Content:");
    // while (file.available())
    // {
    //     Serial.write(file.read());
    // }
    // file.close();

    // Setup I2S 
    bool fret = audio.setPinout(Pin_I2S_BCLK, Pin_I2S_LRC, Pin_I2S_DOUT);
    if (!fret)
    {
        floge("set pinout error");
        return;
    }

    // Set Volume
    audio.setVolume(5);
}

void Sounds::Run()
{
    audio.loop();
    if (!Metro)
        return;
}
