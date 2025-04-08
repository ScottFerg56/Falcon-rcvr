#include "rcvr.h"
#include "Rectenna.h"
#include "FLogger.h"

Rectenna Rectenna::rectenna;

void RectennaConnector::Init(OMObject* obj)
{
    obj->Data = &Rectenna::GetInstance();
}

void RectennaConnector::Push(OMObject* obj, OMProperty* prop)
{
    auto rect = (Rectenna*)obj->Data;
    auto id = prop->GetID();
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
    auto id = prop->GetID();
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
	rectServo.setPeriodHertz(50);    // standard 50 hz servo
	RectState = Stopped;
}

int Rectenna::GetPosition()
{
	// convert position range [0-180] to [0-100]
    return map(rectPos, 0, 180, 0, 100);
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
