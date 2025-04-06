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
        Snds()->Play("Cheerful R2D2.mp3");
    }

    long Get() { return 0; }
private:
    Sounds* Snds() { return (Sounds*)Parent; }

};

void Sounds::Play(String fileName)
{
    // Open music file
    bool fret = audio.connecttoFS(SPIFFS, ("/" + fileName).c_str());
    if (!fret)
    {
        floge("audio connect error");
        return;
    }
    auto s = audio.getFileSize();
    flogd("audio size %d", s);
}

void Sounds::Setup()
{
    AddProperty(new SPropPlay());

    // Setup I2S 
    if (!audio.setPinout(Pin_I2S_BCLK, Pin_I2S_LRC, Pin_I2S_DOUT))
    {
        floge("Audio set pinout error");
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
