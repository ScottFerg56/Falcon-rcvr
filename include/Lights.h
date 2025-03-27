#pragma once

#include "OMObject.h"

class Lights : public OMObject
{
public:
	Lights() { };
	void		Setup();
	void		Run();
    char        GetID() { return 'l'; }
    const char* GetName() { return "Lights"; }
};
