#pragma once
#include "OMObject.h"
#include "Metronome.h"

class Sounds : public OMObject
{
public:
	Sounds() : Metro(2000) { };
    char            GetID() { return 's'; }
    const char*     GetName() { return "Sounds"; }
	void		    Setup();
	void		    Run();
    void            Play(String fileName);
private:
	Metronome	Metro;
};
