#include <Arduino.h>
#include "rcvr.h"
#include "Ramp.h"
#include "FLogger.h"

// Create the motor shield object with the default I2C address
Adafruit_MotorShield AFMS = Adafruit_MotorShield();

#define MIN_SPEED	155
#define MAX_SPEED	255

Ramp Ramp::ramp;

void RampConnector::Init(OMObject* obj)
{
    auto ramp = &Ramp::GetInstance();
    obj->Data = ramp;
    ramp->RampObject = obj;
}

void RampConnector::Push(OMObject* obj, OMProperty* prop)
{
    auto ramp = (Ramp*)obj->Data;
    auto id = prop->Id;
    switch (id)
    {
    case 's':   // state
        ramp->SetState((Ramp::RampStates)((OMPropertyChar*)prop)->Index());
        break;
    case 'v':   // speed
        {
            // convert speed range from [0-100]
            auto speed = map(((OMPropertyLong*)prop)->Value, 0, 100, MIN_SPEED, MAX_SPEED);
            ramp->SetSpeed(speed);
            flogd("Ramp speed: %d", ((OMPropertyLong*)prop)->Value);
        }
        break;
    }
}

void RampConnector::Pull(OMObject *obj, OMProperty *prop)
{
    auto ramp = (Ramp*)obj->Data;
    auto id = prop->Id;
    switch (id)
    {
    case 's':   // state
        ((OMPropertyChar*)prop)->Value = ((OMPropertyChar*)prop)->FromIndex(ramp->RampState);
        // flogv("pulled: %c", ((OMPropertyChar*)prop)->Value);
        break;
    case 'v':   // speed
        // convert speed range to [0-100]
        ((OMPropertyLong*)prop)->Value = map(ramp->GetSpeed(), MIN_SPEED, MAX_SPEED, 0, 100);
        break;
    }
}

RampConnector RampConn;

void Ramp::SetState(RampStates state)
{
    if (RampState == state)
        return;

    switch (state)
    {
        case Retracted:
        case Retracting:
            if (RampRetracted())
            {
                flogd("ramp Retracted");
                RampState = Retracted;
            }
            else
            {
                // myMotor->run(BACKWARD);
                // myMotor->setSpeed(Speed);
                flogd("ramp Retracting");
                RampState = Retracting;
            }
            break;
        case Stopped:
            // myMotor->run(RELEASE);
            // myMotor->setSpeed(0);
            if (RampExtended())
            {
                flogd("ramp Extended");
                RampState = Extended;
            }
            else if (RampRetracted())
            {
                flogd("ramp Retracted");
                RampState = Retracted;
            }
            else
            {
                flogd("ramp Stopped");
                RampState = Stopped;
            }
            break;
        case Extending:
        case Extended:
            if (RampExtended())
            {
                flogd("ramp Extended");
                RampState = Extended;
            }
            else
            {
                // myMotor->run(FORWARD);
                // myMotor->setSpeed(Speed);
                flogd("ramp Extending");
                RampState = Extending;
            }
        break;
    }

    // notify of state change
    auto prop = (OMPropertyChar*)(RampObject->GetProperty('s'));
    prop->SetSend(prop->FromIndex(RampState));
}

bool Ramp::RampRetracted() { return !digitalRead(Pin_RetractedLimitSW); }
bool Ramp::RampExtended() { return  !digitalRead(Pin_ExtendedLimitSW); }

void Ramp::Setup()
{
	// set inputs for limit switches
	pinMode(Pin_RetractedLimitSW, INPUT_PULLUP);
	pinMode(Pin_ExtendedLimitSW, INPUT_PULLUP);

	// create with the default frequency 1.6KHz
	// AFMS.begin();
	// turn off motor
    myMotor = AFMS.getMotor(1);
}

void Ramp::Run()
{
    if (!Metro)
        return;

    if (RampState == Extending)
    {
        flogd("ramp Extending");
        if (RampExtended())
            SetState(Stopped);
    }
    else if (RampState == Retracting)
    {
        flogd("ramp Retracting");
        if (RampRetracted())
            SetState(Stopped);
    }   
}
