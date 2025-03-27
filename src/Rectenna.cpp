#include "rcvr.h"
#include "Rectenna.h"
#include "FLogger.h"

class APropSpeed : public OMPropertyLong
{
public:
    APropSpeed() {}

    char GetID() { return 'v'; }
    const char* GetName() { return "Speed"; }

    long GetMin() { return 0; }
    long GetMax() { return 100; }

    void Set(long value)
    {
        if (!TestRange(value) || Rect()->Speed == value)
            return;
        Rect()->Speed = value;
        Changed = true;
        Rect()->SetClock();
        flogd("Rectenna speed: %d", value);
    }

    long Get() { return Rect()->Speed; }

private:
    Rectenna* Rect() { return (Rectenna*)Parent; }
};

class APropSweep : public OMPropertyBool
{
public:
    APropSweep() {}

    char GetID() { return 's'; }
    const char* GetName() { return "Sweep"; }

    void Set(bool value)
    {
        if (Rect()->GetSweep() == value)
            return;
        Rect()->SetSweep(value);
        Changed = true;
    }

    bool Get() { return Rect()->GetSweep(); }
private:
    Rectenna* Rect() { return (Rectenna*)Parent; }
};

class APropPosition : public OMPropertyLong
{
public:
    APropPosition() {}

    char GetID() { return 'p'; }
    const char* GetName() { return "Position"; }

    long GetMin() { return 0; }
    long GetMax() { return 100; }

    void Set(long value)
    {
        if (!TestRange(value) || Rect()->GetPosition() == value)
            return;
        Rect()->SetPosition(value);
        Changed = true;
    }

    long Get() { return Rect()->GetPosition(); }

private:
    Rectenna* Rect() { return (Rectenna*)Parent; }
};

void Rectenna::Setup()
{
    AddProperty(new APropSpeed());
    AddProperty(new APropSweep());
    AddProperty(new APropPosition());

	ESP32PWM::allocateTimer(0);
	ESP32PWM::allocateTimer(1);
	ESP32PWM::allocateTimer(2);
	ESP32PWM::allocateTimer(3);
	rectServo.setPeriodHertz(50);    // standard 50 hz servo
	RectState = Stopped;
}

int Rectenna::GetPosition()
{
	// convert position range [0-180] to [0-100]
    return map(rectPos, 0, 180, 0, 100);
	// return (uint32_t)round(((float)rectPos / 180) * 100);
}

void Rectenna::SetPosition(int position)
{
	// make sure we're not competing with Sweep to set position
	// and that the servo is attached and not free-spinning
	SetState(Hold);
	// convert position range from [0-100] to [0-180]
    rectPos = map(position, 0, 100, 0, 180);
	rectServo.write(rectPos);
	flogv("Rectenna position: %d", rectPos);
}

bool Rectenna::GetSweep()
{
	return RectState == SweepingCCW || RectState == SweepingCW;
}

void Rectenna::SetSweep(bool onOff)
{
	if (GetSweep() == onOff)
		return;
	SetState(onOff ? SweepingCW : Stopped); // LastSweepState
}

void Rectenna::SetClock()
{
	// convert Speed range from [0-100] to [5-30]
	Metro.PeriodMS = map(Speed, 0, 100, 100, 20);
	flogv("Rectenna speed: %d  period %d ms", Speed, Metro.PeriodMS);
}

void Rectenna::SetState(RectStates state)
{
	if (GetSweep())
		LastSweepState = RectState;
	RectState = state;
	switch (state)
	{
	case Rectenna::Stopped:
        //Detach();
        Metro.PeriodMS = 60000;
		break;
	case Rectenna::Hold:
        //Attach();
        Metro.PeriodMS = 60000;
        break;
	case Rectenna::SweepingCCW:
	case Rectenna::SweepingCW:
        SetClock();
        //Attach();
		break;
	default:
		break;
	}
}

void Rectenna::Run()
{
    if (!Metro)
        return;

	if (!GetSweep())
    {
        if (rectPos == -1)
        {
            rectServo.attach(Pin_Servo, 550, 2400);
            SetPosition(50);
        }
        return;
    }    

    // flogv("state: %d", RectState);
	switch (RectState)
	{
	case SweepingCW:
		if (++rectPos >= 180)
        {
			RectState = SweepingCCW;
            // flogv("flip to CCW");
        }
		else
			rectServo.write(rectPos);
		break;
	case SweepingCCW:
		if (--rectPos <= 0)
        {
			RectState = SweepingCW;
            // flogv("flip to CW");
        }
		else
			rectServo.write(rectPos);
		break;
	default:
		break;
	}
}
