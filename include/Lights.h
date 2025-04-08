#pragma once

#include "OMObject.h"

class GroupConnector : public OMConnector
{
public:
    void Init(OMObject *obj) override {}
    void Push(OMObject *obj, OMProperty *prop) override;
    void Pull(OMObject *obj, OMProperty *prop) override;
};

class LightConnector : public OMConnector
{
public:
    void Init(OMObject* obj) override;
    void Push(OMObject* obj, OMProperty* prop) override;
    void Pull(OMObject *obj, OMProperty *prop) override;
};

extern GroupConnector GroupConn;
extern LightConnector LightConn;

class Lights
{
public:
	void		    Setup();
	void		    Run();
    OMObject*       LightsObject;
    // get singleton instance
    static Lights& GetInstance() { return lights; }
    // Delete copy constructor and assignment operator to prevent copying singleton
    Lights(const Lights&) = delete;
    Lights& operator=(const Lights&) = delete;
private:
    // Static member variable to hold the single instance
    static Lights lights;
    // private constructor for singleton
	Lights() { };
};
