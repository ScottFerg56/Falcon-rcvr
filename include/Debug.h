#pragma once

#include "Metronome.h"
#include "OMObject.h"

class Debug : public OMObject
{
public:
	Debug() : Metro(100) { }
    char        GetID() { return 'd'; }
    const char* GetName() { return "Debug"; }
	void        Setup();
	void        Run();
private:
	Metronome	Metro;
};
