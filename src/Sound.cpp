#include "rcvr.h"
#include <Arduino.h>
#include "FLogger.h"
#include "Sound.h"
#include "SPIFFS.h"
#include <Audio.h>

Sound Sound::sound;

Audio audio;

std::vector<String> SoundFiles()
{
    std::vector<String> files;
    File root = SPIFFS.open("/", 0);
    if (!root)
    {
        floge("Failed to open root directory");
        return files;
    }
    File file = root.openNextFile();
    while (file)
    {
        if (!file.isDirectory())
        {
            String name(file.name());
            if (name.endsWith(".mp3"))
            {
                name.remove(name.length() - 4);
                files.push_back(name);
            }
        }
        file = root.openNextFile();
    }
    std::sort(files.begin(), files.end());
    return files;
}

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
            sound->Play(sound->Sounds[((int)((OMPropertyLong*)prop)->Value) - 1] + ".mp3");
            ((OMPropertyLong*)prop)->Value = 0;
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
        ((OMPropertyLong*)prop)->Value = 0;
        break;
    case 'l':   // sound file list
        {
            String s;
            for (auto item : sound->Sounds)
            {
                if (item.length() + s.length() > 240)
                {
                    floge("sound list too long");
                    break;
                }
                if (s.length() > 0)
                    s.concat(',');
                s.concat(item);
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
    flogv("playing %s", fileName.c_str());
    bool fret = audio.connecttoFS(SPIFFS, ("/" + fileName).c_str());
    if (!fret)
    {
        floge("audio play error");
        return;
    }
    auto s = audio.getFileSize();
    flogd("audio size %d", s);
}

void Sound::Setup()
{
    Sounds = SoundFiles();

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
