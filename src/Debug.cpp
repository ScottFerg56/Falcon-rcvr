#include "Debug.h"
#include "Agent.h"

class DPropLogLevel : public OMProperty
{
public:
    DPropLogLevel() {}

    char                GetID() { return 'l'; }
    const char*         GetName() { return "LogLevel"; }
    virtual OMT         GetType() { return OMT_CHAR; }
    const char* levelIDs = "NFEWIDV";

    virtual void ToString(String& s)
    {
        s.concat(levelIDs[Get()]);
    }

    virtual bool FromString(String& s)
    {
        char c = toupper(s[0]);
        auto p = strchr(levelIDs, c);
        if (!p)
        {
            floge("invalid LogLevel value: [%c]", c);
            return false;
        }
        int l = p - levelIDs;
        if (l > FLOG_VERBOSE)
        {
            floge("invalid LogLevel value: [%c]", c);
            return false;
        }
        Set(l);
        s.remove(0, 1);
        return true;
    }

    void Set(long value)
    {
        if (FLogger::getLogLevel() == value)
            return;
        FLogger::setLogLevel((flog_level)value);
        Changed = true;
    }

    long Get() { return FLogger::getLogLevel(); }
};

void Debug::Setup()
{
    AddProperty(new DPropLogLevel());
    flogv("Falcon rcvr " __DATE__ " " __TIME__);
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
                            ((Root*)Parent)->SendPacket(cmd.substring(1));
                        }
                        else if (cmd[0] == 'x')
                        {
                            // file transfer
                            Agent::GetInstance().StartFileTransfer("/Sad R2D2.mp3");
                        }
                        else
                        {
                            // command for ourself
                            ((Root*)Parent)->Command(cmd);
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
