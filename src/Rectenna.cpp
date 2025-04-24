#include "rcvr.h"
#include "Rectenna.h"
#include "FLogger.h"

// 167 degrees seems to be the limit for this setup
#define MIN_POS	0
#define MAX_POS	180     // 167

Rectenna Rectenna::rectenna;

void RectennaConnector::Init(OMObject* obj)
{
    auto rect = &Rectenna::GetInstance();
    obj->Data = rect;
    rect->RectennaObject = obj;
}

void RectennaConnector::Push(OMObject* obj, OMProperty* prop)
{
    auto rect = (Rectenna*)obj->Data;
    auto id = prop->Id;
    switch (id)
    {
    case 's':   // sweep
        rect->SetSweep(((OMPropertyBool*)prop)->Value);
        break;
    case 'v':   // speed
        rect->Speed = ((OMPropertyLong*)prop)->Value;
        rect->SetClock();
        flogd("Rectenna speed: %d", rect->Speed);
        break;
    case 'p':   // position
        rect->SetPosition(((OMPropertyLong*)prop)->Value);
        break;
    }
}

void RectennaConnector::Pull(OMObject *obj, OMProperty *prop)
{
    auto rect = (Rectenna*)obj->Data;
    auto id = prop->Id;
    switch (id)
    {
    case 's':   // sweep
        ((OMPropertyBool*)prop)->Value = rect->GetSweep();
        break;
    case 'v':   // speed
        ((OMPropertyLong*)prop)->Value = rect->Speed;
        break;
    case 'p':   // position
        ((OMPropertyLong*)prop)->Value = rect->GetPosition();
        break;
    }
}

RectennaConnector RectennaConn;

void Rectenna::Setup()
{
	ESP32PWM::allocateTimer(0);
	ESP32PWM::allocateTimer(1);
	ESP32PWM::allocateTimer(2);
	ESP32PWM::allocateTimer(3);
    rectServo.attach(Pin_Servo, 550, 2227);
	rectServo.setPeriodHertz(50);    // standard 50 hz servo
	RectState = Stopped;
}

int Rectenna::GetPosition()
{
	// convert position range [degrees] to [0-100]
    return mapr(rectPos, MIN_POS, MAX_POS, 0, 100);
}

void Rectenna::SetPosition(int position)
{
	// make sure we're not competing with Sweep to set position
	// and that the servo is attached and not free-spinning
	SetState(Hold);
	// convert position range from [0-100] to [degrees]
    rectPos = mapr(position, 0, 100, MIN_POS, MAX_POS);
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
    if (!onOff)
        RectennaObject->GetProperty('p')->PullSend();
}

void Rectenna::SetClock()
{
	// convert Speed range from [0-100] to [5-30]
	Metro.PeriodMS = mapr(Speed, 0, 100, 100, 20);
	flogv("Rectenna speed: %d  period %d ms", Speed, Metro.PeriodMS);
}

void Rectenna::SetState(RectStates state)
{
	auto sweep = GetSweep();
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
    if (sweep != GetSweep())
    {
        // if state change affects Sweep, notify the controller
        RectennaObject->GetProperty('s')->PullSend();
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
            SetPosition(50);
            RectennaObject->GetProperty('p')->PullSend();
        }
        return;
    }    

    // flogv("state: %d", RectState);
	switch (RectState)
	{
	case SweepingCW:
		if (++rectPos >= MAX_POS)
        {
			RectState = SweepingCCW;
            // flogv("flip to CCW");
        }
		else
			rectServo.write(rectPos);
		break;
	case SweepingCCW:
		if (--rectPos <= MIN_POS)
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
