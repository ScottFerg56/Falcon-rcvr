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
    auto id = prop->Id;
    switch (id)
    {
    case 'p':   // play
        {
            // convert 1-based index to 0-based array
            sound->Play("/" + sound->Sounds[((int)((OMPropertyLong*)prop)->Value) - 1] + ".mp3");
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
    case 'x':   // delete
        {
            // convert 1-based index to 0-based array
            int inx = ((OMPropertyLong*)prop)->Value;
            SPIFFS.remove("/" + sound->Sounds[inx - 1] + ".mp3");
            ((OMPropertyLong*)prop)->Value = 0;
            // rebuild and send the sound list
            auto prop = sound->SoundObject->GetProperty('l');
            prop->Pull();
            prop->Send();
        }
        break;
    }
}

void SoundConnector::Pull(OMObject *obj, OMProperty *prop)
{
    auto sound = (Sound*)obj->Data;
    auto id = prop->Id;
    switch (id)
    {
    case 'p':   // play
        ((OMPropertyLong*)prop)->Value = 0;
        break;
    case 'l':   // sound file list
        {
            // build comma-delimited list of sound files
            sound->SetSounds();
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
    bool fret = audio.connecttoFS(SPIFFS, fileName.c_str());
    if (!fret)
    {
        floge("audio play error");
        return;
    }
    auto s = audio.getFileSize();
    flogd("audio size %d", s);
}

void Sound::ReceivedFile(String fileName)
{
    flogv("received file %s", fileName.c_str());
    // rebuild and send the sound list
    auto prop = SoundObject->GetProperty('l');
    prop->Pull();
    prop->Send();
    Play(fileName);
}

void Sound::SetSounds()
{
    Sounds.clear();
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
            if (name.endsWith(".mp3"))
            {
                name.remove(name.length() - 4);
                Sounds.push_back(name);
            }
        }
        file = root.openNextFile();
    }
    std::sort(Sounds.begin(), Sounds.end());
    ((OMPropertyLong*)SoundObject->GetProperty('p'))->Max = Sounds.size();
    ((OMPropertyLong*)SoundObject->GetProperty('x'))->Max = Sounds.size();
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
