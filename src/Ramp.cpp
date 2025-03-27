#include <Arduino.h>
#include "rcvr.h"
#include "Ramp.h"
#include "FLogger.h"

// Create the motor shield object with the default I2C address
Adafruit_MotorShield AFMS = Adafruit_MotorShield();

#define MIN_SPEED	155
#define MAX_SPEED	255

class RPropState : public OMProperty
{
public:
    RPropState() {}

    char                GetID() { return 's'; }
    const char*         GetName() { return "State"; }
    virtual OMT         GetType() { return OMT_CHAR; }
    const char* levelIDs = "RrSeE";

    virtual void ToString(String& s)
    {
        s.concat(levelIDs[Get()]);
    }

    virtual bool FromString(String& s)
    {
        char c = toupper(s[0]);
        auto p = strchr(levelIDs, c);
        if (!p)
        {
            floge("invalid State value: [%c]", c);
            return false;
        }
        int l = p - levelIDs;
        if (l > Ramp::Extended)
        {
            floge("invalid State value: [%c]", c);
            return false;
        }
        flogd("set state  %c  %d", c, l);
        Set(l);
        flogd("state value  %d", Get());
        s.remove(0, 1);
        return true;
    }

    void Set(long value)
    {
        Rmp()->SetState((Ramp::RampStates)value);
    }

    long Get() { return Rmp()->RampState; }
private:
    Ramp* Rmp() { return (Ramp*)Parent; }
};

class RPropSpeed : public OMPropertyLong
{
public:
    RPropSpeed() {}

    char GetID() { return 'v'; }
    const char* GetName() { return "Speed"; }

    long GetMin() { return 0; }
    long GetMax() { return 100; }

    void Set(long value)
    {
        if (!TestRange(value))
            return;
        // convert speed range from [0-100]
        value = map(value, 0, 100, MIN_SPEED, MAX_SPEED);
        if (Rmp()->Speed == value)
            return;
        Changed = true;
        // myMotor->setSpeed(Speed);
        flogd("Ramp speed: %d", value);
    }

    long Get()
    {
        // convert speed range to [0-100]
        uint8_t speed = map(Rmp()->Speed, MIN_SPEED, MAX_SPEED, 0, 100);
        return speed;
    }

private:
    Ramp* Rmp() { return (Ramp*)Parent; }
};

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
    GetProperty('s')->Changed = true;
}

bool Ramp::RampRetracted() { return !digitalRead(Pin_RetractedLimitSW); }
bool Ramp::RampExtended() { return  !digitalRead(Pin_ExtendedLimitSW); }

void Ramp::Setup()
{
    AddProperty(new RPropState());
    AddProperty(new RPropSpeed());
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
