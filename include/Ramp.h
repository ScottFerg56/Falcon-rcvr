#pragma once

#include <Arduino.h>
#include "OMObject.h"
#include "Metronome.h"
#include <Adafruit_MotorShield.h>

class Ramp : public OMObject
{
public:
	Ramp() : Metro(100) { };
    char        GetID() { return 'r'; }
    const char* GetName() { return "Ramp"; }
	enum		RampStates { Retracted, Retracting, Stopped, Extending, Extended };
	void		Setup();
	void		Run();
    void        SetState(RampStates state);
    RampStates	RampState = Stopped;
    uint8_t		Speed = 255;
private:
    Adafruit_DCMotor *myMotor;
	Metronome	Metro;
	bool		RampRetracted();
	bool		RampExtended();
};
