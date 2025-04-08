#include "Debug.h"
#include "Agent.h"

DebugConnector DebugConn;
Debug Debug::debug;

void DebugConnector::Init(OMObject* obj)
{
    auto debug = &Debug::GetInstance();
    obj->Data = debug;
    debug->DebugObject = obj;
}

void DebugConnector::Push(OMObject* obj, OMProperty* prop)
{
    auto debug = (Debug*)obj->Data;
    auto id = prop->GetID();
    switch (id)
    {
    case 'l':   // LogLevel
        FLogger::setLogLevel((flog_level)((OMPropertyChar*)prop)->Index());
        break;
    }
}

void DebugConnector::Pull(OMObject *obj, OMProperty *prop)
{
    auto debug = (Debug*)obj->Data;
    auto id = prop->GetID();
    switch (id)
    {
    case 'l':   // LogLevel
        ((OMPropertyChar*)prop)->Value = ((OMPropertyChar*)prop)->Valid[FLogger::getLogLevel()];
        break;
    }
}

void Debug::Setup()
{
    flogv("Debug setup");
}

void Debug::Run()
{
	if (Metro)
	{
		if (Serial.available())
		{
            static String cmd;
            while (Serial.available())
            {
                char c = Serial.read();
                // echo back to terminal
                if (c == '\n')
                    continue;
                Serial.write(c);
                if (c == '\r')
                    Serial.write('\n');
                if (c < ' ')
                {
                    // terminate command
                    if (cmd.length() > 0)
                    {
                        if (cmd[0] == '|')
                        {
                            // pipe a command to our peer
                            ((Root*)(DebugObject->MyRoot()))->SendCmd(cmd.substring(1));
                        }
                        else if (cmd[0] == 'x')
                        {
                            // file transfer
                            Agent::GetInstance().StartFileTransfer("/Sad R2D2.mp3");
                        }
                        else
                        {
                            // command for ourself
                            ((Root*)(DebugObject->MyRoot()))->Command(cmd);
                        }
                    }
                    cmd.clear();
                }
                else
                {
                    cmd.concat(c);
                }
            }
		}
	}
}
