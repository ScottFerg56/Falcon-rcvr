#include "rcvr.h"
#include <Arduino.h>
#include "FLogger.h"
#include "Sound.h"
#include "SPIFFS.h"
#include <Audio.h>

Sound Sound::sound;

Audio audio;

void SoundConnector::Init(OMObject* obj)
{
    auto sound = &Sound::GetInstance();
    obj->Data = sound;
    sound->SoundObject = obj;
}

void SoundConnector::Push(OMObject* obj, OMProperty* prop)
{
    auto sound = (Sound*)obj->Data;
    auto id = prop->GetID();
    switch (id)
    {
    case 'p':   // play
        {
            // UNDONE:
            sound->Play("Cheerful R2D2.mp3");
        }
        break;
    case 'l':   // sound file list
        {
            floge("sound list should not be pushed to device");
        }
        break;
    case 'v':   // volume
        {

            int volume = ((OMPropertyLong*)prop)->Value;
            audio.setVolume(volume);
        }
        break;
    }
}

void SoundConnector::Pull(OMObject *obj, OMProperty *prop)
{
    auto sound = (Sound*)obj->Data;
    auto id = prop->GetID();
    switch (id)
    {
    case 'p':   // play
        // UNDONE:
        ((OMPropertyLong*)prop)->Value = 0;
        break;
    case 'l':   // sound file list
        {
            String s;
            File root = SPIFFS.open("/", 0);
            if (!root)
            {
                floge("Failed to open root directory");
                return;
            }
            File file = root.openNextFile();
            while (file)
            {
                if (!file.isDirectory())
                {
                    String name(file.name());
                    name.toLowerCase();
                    if (name.endsWith(".mp3") || name.endsWith(".MP3"))
                    {
                        name.remove(name.length() - 4);
                        // good enough for about 20 sound files in an esp_now packet
                        if (name.length() + s.length() > 240)
                        {
                            floge("sound list too long");
                            break;
                        }
                        if (s.length() > 0)
                            s.concat(',');
                        s.concat(name);
                    }
                }
                file = root.openNextFile();
            }
            ((OMPropertyString*)prop)->Value = s;
        }
        break;
    case 'v':   // volume
        {
            ((OMPropertyLong*)prop)->Value = audio.getVolume();
        }
        break;
    }
}

SoundConnector SoundConn;

void Sound::Play(String fileName)
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

void Sound::Setup()
{
    // Setup I2S 
    if (!audio.setPinout(Pin_I2S_BCLK, Pin_I2S_LRC, Pin_I2S_DOUT))
    {
        floge("Audio set pinout error");
        return;
    }

    // Set Volume
    audio.setVolume(5);
}

void Sound::Run()
{
    audio.loop();
    if (!Metro)
        return;
}
