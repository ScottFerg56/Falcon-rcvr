#pragma once
#include "OMObject.h"
#include "Metronome.h"

class SoundConnector : public OMConnector
{
public:
    void Init(OMObject *obj) override;
    void Push(OMObject *obj, OMProperty *prop) override;
    void Pull(OMObject *obj, OMProperty *prop) override;
};

extern SoundConnector SoundConn;

class Sound
{
public:
	void		    Setup();
	void		    Run();
    void            Play(String fileName);
    void            ReceivedFile(String fileName);
    void            SetSounds();
    OMObject*       SoundObject;
    std::vector<String> Sounds;
    // get singleton instance
    static Sound& GetInstance() { return sound; }
    // Delete copy constructor and assignment operator to prevent copying singleton
    Sound(const Sound&) = delete;
    Sound& operator=(const Sound&) = delete;
private:
    // Static member variable to hold the single instance
    static Sound sound;
    // private constructor for singleton
	Sound() : Metro(2000) { };
	Metronome	Metro;
};
