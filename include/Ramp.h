#pragma once

#include <Arduino.h>
#include "OMObject.h"
#include "Metronome.h"
#include <Adafruit_MotorShield.h>

class RampConnector : public OMConnector
{
public:
    void Init(OMObject *obj) override;
    void Push(OMObject *obj, OMProperty *prop) override;
    void Pull(OMObject *obj, OMProperty *prop) override;
};

extern RampConnector RampConn;

class Ramp
{
public:
	enum		RampStates { Retracted, Retracting, Stopped, Extending, Extended };
	void		Setup();
	void		Run();
    void        SetState(RampStates state);
    RampStates	RampState = Stopped;
    void        SetSpeed(uint8_t speed)
    {
        Speed = speed;
        myMotor->setSpeed(Speed);
    }
    uint8_t     GetSpeed() { return Speed; }
    OMObject*   RampObject;
    // get singleton instance
    static Ramp& GetInstance() { return ramp; }
    // Delete copy constructor and assignment operator to prevent copying singleton
    Ramp(const Ramp&) = delete;
    Ramp& operator=(const Ramp&) = delete;
private:
    // Static member variable to hold the single instance
    static Ramp ramp;
    // private constructor for singleton
	Ramp() : Metro(100) { };
    Adafruit_DCMotor *myMotor;
    uint8_t		Speed = 255;
	Metronome	Metro;
	bool		RampRetracted();
	bool		RampExtended();
};
