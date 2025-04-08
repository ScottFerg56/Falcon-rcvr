#pragma once

#include <Arduino.h>
#include <ESP32Servo.h>
#include "Metronome.h"
#include "OMObject.h"

class RectennaConnector : public OMConnector
{
public:
    void Init(OMObject *obj) override;
    void Push(OMObject *obj, OMProperty *prop) override;
    void Pull(OMObject *obj, OMProperty *prop) override;
};

extern RectennaConnector RectennaConn;

class Rectenna
{
public:
	enum		RectStates { Stopped, Hold, SweepingCCW, SweepingCW };
	void		Setup();
	void		Run();
    int         Speed = 100;
    void        SetClock();
	bool		GetSweep();
	void		SetSweep(bool onOff);
	int			GetPosition();
	void		SetPosition(int position);
    static Rectenna& GetInstance() { return rectenna; }
    // Delete copy constructor and assignment operator to prevent copying singleton
    Rectenna(const Rectenna&) = delete;
    Rectenna& operator=(const Rectenna&) = delete;
private:
    // Static member variable to hold the single instance
    static Rectenna rectenna;
    // private constructor for singleton
	Rectenna() : Metro(2000) { };
	void		SetState(RectStates state);

	Metronome	Metro;
	RectStates	RectState = Stopped;
	RectStates	LastSweepState = SweepingCW;
	Servo		rectServo;
	int 		rectPos = -1;
};
