#pragma once

#include <Arduino.h>
#include <ESP32Servo.h>
#include "Metronome.h"
#include "OMObject.h"

class Rectenna : public OMObject
{
public:
	Rectenna() : Metro(2000) { };
    char        GetID() { return 'a'; }
    const char* GetName() { return "Rectenna"; }
	enum		RectStates { Stopped, Hold, SweepingCCW, SweepingCW };
	void		Setup();
	void		Run();
    int         Speed = 100;
    void        SetClock();
	bool		GetSweep();
	void		SetSweep(bool onOff);
	int			GetPosition();
	void		SetPosition(int position);
private:
	void		SetState(RectStates state);

	Metronome	Metro;
	RectStates	RectState = Stopped;
	RectStates	LastSweepState = SweepingCW;
	Servo		rectServo;
	int 		rectPos = -1;
};
