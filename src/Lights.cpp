/*
	Lights
*/
#include "rcvr.h"
#include <Arduino.h>
#include "FLogger.h"

#define FXLOGE(format, ...) floge(format, ##__VA_ARGS__)
#define FXLOGW(format, ...) flogw(format, ##__VA_ARGS__)
#define FXLOGD(format, ...) flogd(format, ##__VA_ARGS__)
#define FXLOGV(format, ...) flogv(format, ##__VA_ARGS__)

#include "Lights.h"
#include "StdFX.h"
#include "NeoBusFX.h"
#include "LedFX.h"

// define the type of neopixel bus we're using
typedef  NeoPixelBus<NeoGrbFeature, NeoEsp32Rmt0Ws2812xMethod> NEOMETHOD;    // Neo800KbpsMethod NeoWs2812xMethod
// create the raw NeoPixelBus strip
NEOMETHOD Strip(58, Pin_NEOSTRIP);
// create a strip class from the raw strip conformant to the FX requirements
FXNeoBus<NEOMETHOD> NeoBus(Strip);

// another short bus for the onboard neopixel
// Neo800KbpsMethod also works
// I2S audio initialization seems to mess with Pin_NEOPIXEL (pin 0).
// So we need to setup Sounds first BEFORE Lights setup fixes pin 0!
// Using that order, both seem to work properly.
typedef  NeoPixelBus<NeoGrbFeature, NeoEsp32Rmt1Ws2812xMethod> NEOMETHOD1;
NEOMETHOD1 StripOne(1, (uint8_t)PIN_NEOPIXEL);
FXNeoBus<NEOMETHOD1> NeoOne(StripOne);

// use an FXFactory to map indices to effects
FXFactory fxFactory;
// the FXServer controls multiple strip subsegments at once
FXServer fxServer;

struct LightDef
{
    char Id;
    const char* Name;
    uint8_t Anim;
    FXParams Params;
    FXSegmentBase* Seg;
};

#define DEF_SPEED 1000

//
// Pixels grouped into logical elements, ordered by pixel number.
//
const uint16_t Pixels_Warning        [] = { 1, 5, 16, 17, 18, 19, 23, 24, 42, /*63, 66, 98, 99, 102, 103,*/ PMAP_END };
const uint16_t Pixels_Landing        [] = { 2, 4, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 21, 22, 25, /*62, 64, 65, 67, 68, 69, 70, 71, 72, 73, 74, 100, 101, 104, 105,*/ PMAP_END };
const uint16_t Pixels_Headlight      [] = { 20, /*61,*/ PMAP_END };
const uint16_t Pixels_Sconce         [] = { 26, 27, 28, 29, 30, 31, 32, 33, 34, /*58, 59, 60,*/ PMAP_END };
const uint16_t Pixels_Floor          [] = { 44, 45, 46, 47, 48, 57, PMAP_END };
const uint16_t Pixels_Engine         [] = { /*75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97,*/ PMAP_END };

LightDef LightDefs[] = 
{// Id Name               Anim                 On     Color1    Color2    Speed      Reverse Segment
{ 'e', "Engine",		  FX_STATIC,  FXParams(false, 0x000040, 0x004000, DEF_SPEED, false), new FXStripSegMapped(Pixels_Engine, &NeoBus)    },
{ 'l', "Landing",		  FX_STATIC,  FXParams(false, 0x404040, 0x000000, DEF_SPEED, false), new FXStripSegMapped(Pixels_Landing, &NeoBus)   },
{ 'w', "Warning",		  FX_STATIC,  FXParams(false, 0x400000, 0x000000, DEF_SPEED, false), new FXStripSegMapped(Pixels_Warning, &NeoBus)   },
{ 'd', "Headlight",	      FX_STATIC,  FXParams(false, 0x404040, 0x000000, DEF_SPEED, false), new FXStripSegMapped(Pixels_Headlight, &NeoBus) },
{ 'g', "Gunwell",         FX_STATIC,  FXParams(false, 0x804040, 0x000000, DEF_SPEED, false), new FXStripSegRange(0, &NeoBus)                 },
{ 'r', "Ramp",            FX_STATIC,  FXParams(false, 0x404040, 0x000000, DEF_SPEED, false), new FXStripSegRange(3, &NeoBus)                 },
{ 't', "Tubes",           FX_COUNT,   FXParams(false, 0, 0, 0, false), nullptr },
  { 's', "Sconce",        FX_STATIC,  FXParams(false, 0x004000, 0x000000, DEF_SPEED, false), new FXStripSegMapped(Pixels_Sconce, &NeoBus)    },
  { 'f', "Floor",         FX_STATIC,  FXParams(false, 0x004040, 0x000000, DEF_SPEED, false), new FXStripSegMapped(Pixels_Floor, &NeoBus)     },
  {  0,  "",              FX_COUNT,   FXParams(false, 0, 0, 0, false), nullptr },
{ 'h', "Hold",            FX_COUNT,   FXParams(false, 0, 0, 0, false), nullptr },
  { 'y', "Bay",	          FX_STATIC,  FXParams(false, 0x404040, 0x000000, DEF_SPEED, false), new FXStripSegRange(55, 56, &NeoBus)            },
  { 'b', "Bed",           FX_STATIC,  FXParams(false, 0xFFFFFF, 0x000000, DEF_SPEED, false), new FXLedSegBase(Pin_LED_Hold_Monitor)          },
  { 'g', "Grates",        FX_STATIC,  FXParams(false, 0x404040, 0x000000, DEF_SPEED, false), new FXStripSegRange(53, 54, &NeoBus)            },
  { 'm', "Monitor",       FX_STATIC,  FXParams(false, 0xFFFFFF, 0x000000, DEF_SPEED, false), new FXLedSegBase(Pin_LED_Hold_Bed)              },
  { '0', "Red",	          FX_STATIC,  FXParams(false, 0x400000, 0x000000, DEF_SPEED, false), new FXStripSegRange(49, &NeoBus)                },
  { '1', "Green",	      FX_STATIC,  FXParams(false, 0x004000, 0x000000, DEF_SPEED, false), new FXStripSegRange(51, &NeoBus)                },
  { '2', "Blue",	      FX_STATIC,  FXParams(false, 0x000040, 0x000000, DEF_SPEED, false), new FXStripSegRange(52, &NeoBus)                },
  { '3', "Yellow",        FX_STATIC,  FXParams(false, 0x004040, 0x000000, DEF_SPEED, false), new FXStripSegRange(50, &NeoBus)                },
  {  0,  "",              FX_COUNT,   FXParams(false, 0, 0, 0, false), nullptr },
{ 'c', "Cockpit",         FX_COUNT,   FXParams(false, 0, 0, 0, false), nullptr },
  { 'm', "Monitor",       FX_BLINK,   FXParams(false, 0xFFFFFF, 0x000000, DEF_SPEED, false), new FXLedSegBase(Pin_LED_Cockpit_Monitor)       },
  { '0', "Red",	          FX_STATIC,  FXParams(false, 0x400000, 0x000000, DEF_SPEED, false), new FXStripSegRange(37, &NeoBus)                },
  { '1', "Green",	      FX_STATIC,  FXParams(false, 0x004000, 0x000000, DEF_SPEED, false), new FXStripSegRange(35, &NeoBus)                },
  { '2', "Blue",	      FX_STATIC,  FXParams(false, 0x000040, 0x000000, DEF_SPEED, false), new FXStripSegRange(36, &NeoBus)                },
  { '3', "Yellow",        FX_STATIC,  FXParams(false, 0x404000, 0x000000, DEF_SPEED, false), new FXStripSegRange(38, &NeoBus)                },
  { '4', "Wall UL",       FX_STATIC,  FXParams(false, 0x404040, 0x000000, DEF_SPEED, false), new FXStripSegRange(41, &NeoBus)                },
  { '5', "Wall UR",       FX_STATIC,  FXParams(false, 0x404040, 0x000000, DEF_SPEED, false), new FXStripSegRange(40, &NeoBus)                },
  { '6', "Wall LL",       FX_STATIC,  FXParams(false, 0x404040, 0x000000, DEF_SPEED, false), new FXStripSegRange(43, &NeoBus)                },
  { '7', "Wall LR",       FX_STATIC,  FXParams(false, 0x404040, 0x000000, DEF_SPEED, false), new FXStripSegRange(39, &NeoBus)                },
  {  0,  "",              FX_COUNT,   FXParams(false, 0, 0, 0, false), nullptr },
{ 'z', "Test Neo",        FX_BLINK,   FXParams(true,  0x000040, 0x004000, DEF_SPEED, false), new FXStripSegRange(0, &NeoOne)                 },
};

class Light : public OMObject
{
    class LPropAnim : public OMPropertyLong
    {
    public:
        LPropAnim() {}
    
        char GetID() { return 'a'; }
        const char* GetName() { return "Anim"; }
    
        long GetMin() { return 0; }
        long GetMax() { return FX_COUNT - 1; }
    
        void Set(long value)
        {
            if (!TestRange(value) || Def()->Anim == value)
                return;
            Changed = true;
            // create the new effect by index/id
            auto newEffect = fxFactory.CreateEffect((uint8_t)value, Def()->Seg, &Def()->Params);
            if (!newEffect)
            {
                floge("invalid Light animation value: [%d]", value);
                return;
            }
            Def()->Anim = value;
            // setting the effect copies the Params pointer and resets the effect
            auto oldEffect = fxServer.SetEffect(((Light*)Parent)->SegId, newEffect);
            delete oldEffect;
        }
    
        long Get() { return Def()->Anim; }
    private:
        LightDef* Def() { return ((Light*)Parent)->Def; }
    };
    
    class LPropOn : public OMPropertyBool
    {
    public:
        LPropOn() {}
    
        char GetID() { return 'o'; }
        const char* GetName() { return "On"; }
    
        void Set(bool value)
        {
            if (Def()->Anim != FX_COUNT)
            {
                // normal Light on/off
                if (Def()->Params.On == value)
                    return;
                Changed = true;
                Def()->Params.On = value;
                auto effect = fxServer.GetEffect(((Light*)Parent)->SegId);
                effect->Reset();
            }
            else
            {
                // group on/off
                Def()->Params.On = value;
                // all lights in the group turned on or off
                for (auto o : ((Light*)Parent)->Objects)
                {
                    auto p = (LPropOn*)o->GetProperty('o');
                    p->Set(value);
                }
            }
        }
    
        bool Get() { return Def()->Params.On; }
    private:
        LightDef* Def() { return ((Light*)Parent)->Def; }
    };
    
    class LPropColor : public OMPropertyLong
    {
    public:
        LPropColor(u_int8_t inx) : Inx(inx) {}
        char GetID() { return Inx == 0 ? 'c' : 'd'; }
        const char* GetName() { return Inx == 0 ? "Color0" : "Color1"; }
    
        long GetMin() { return 0; }
        long GetMax() { return 0xFFFFFF; }
        uint8_t GetBase() { return HEX; }
    
        void Set(long value)
        {
            uint32_t& v = Inx == 0 ? Def()->Params.Color0 : Def()->Params.Color1;
            if (!TestRange(value) || v == value)
                return;
            Changed = true;
            v = value;
            // reset the effect after parameter change
            fxServer.GetEffect(((Light*)Parent)->SegId)->Reset();
        }
    
        long Get() { return Inx == 0 ? Def()->Params.Color0 : Def()->Params.Color1; }
    private:
        LightDef* Def() { return ((Light*)Parent)->Def; }
        u_int8_t Inx;
    };
    
    class LPropSpeed : public OMPropertyLong
    {
    public:
        LPropSpeed() {}
    
        char GetID() { return 's'; }
        const char* GetName() { return "Speed"; }
    
        long GetMin() { return 0; }
        long GetMax() { return 60000; }
    
        void Set(long value)
        {
            if (!TestRange(value) || Def()->Params.Speed == value)
                return;
            Changed = true;
            Def()->Params.Speed = value;
            // reset the effect after parameter change
            fxServer.GetEffect(((Light*)Parent)->SegId)->Reset();
    }
    
        long Get() { return Def()->Params.Speed; }
    private:
        LightDef* Def() { return ((Light*)Parent)->Def; }
    };
    
    class LPropReverse : public OMPropertyBool
    {
    public:
        LPropReverse() {}
    
        char GetID() { return 'r'; }
        const char* GetName() { return "Reverse"; }
    
        void Set(bool value)
        {
            if (Def()->Params.Reverse == value)
                return;
            Changed = true;
            Def()->Params.Reverse = value;
            fxServer.GetEffect(((Light*)Parent)->SegId)->Reset();
        }
    
        bool Get() { return Def()->Params.Reverse; }
    private:
        LightDef* Def() { return ((Light*)Parent)->Def; }
    };
    
public:
    Light(LightDef* def, uint8_t segId) : Def(def), SegId(segId)
    {
        if (def->Anim != FX_COUNT)
        {
            // normal Light node with properties
            AddProperty(new LPropAnim());
            AddProperty(new LPropOn());
            AddProperty(new LPropColor(0));
            AddProperty(new LPropColor(1));
            AddProperty(new LPropSpeed());
            AddProperty(new LPropReverse());
        }
        else
        {
            // group Light node with only 'On' property to effect all children
            AddProperty(new LPropOn());
        }
    }
    char GetID() { return Def->Id; }
    const char* GetName() { return Def->Name; }
    void Setup(){}
    void Run(){}
    uint8_t SegId;
private:
    LightDef* Def;
};

void Lights::Setup()
{
    // NEOPIXEL_I2C_POWER needs to be HIGH to power the onboard neopixel
    // but this is apparently already the case when we get here
    // pinMode(NEOPIXEL_I2C_POWER, OUTPUT);
    // digitalWrite(NEOPIXEL_I2C_POWER, HIGH);
    NeoBus.Begin();
    NeoOne.Begin();
    // register all std effects, mapping them to std indices
    fxFactory.RegisterEffects(stdEffectsList, sizeof(stdEffectsList) / sizeof(stdEffectsList[0]));

    auto par = (OMObject*)this;
	for (int i = 0; i < sizeof(LightDefs) / sizeof(LightDefs[0]); i++)
    {
        LightDef* def = &LightDefs[i];
        if (def->Id == 0)
        {
            // leaving group node, parent returns to Lights
            par = this;
            continue;
        }
        // need a unique id for segment registration; the index will suffice
        auto child = new Light(def, i);
        par->AddObject(child);
        if (def->Anim == FX_COUNT)
        {
            // entering group node (not an actual light) which becomes parent to following nodes
            par = child;
        }
        else
        {
            // add the segment to the server for this Light
            // and create the initial effect from the std index/id
            auto effect = fxFactory.CreateEffect(def->Anim, def->Seg, &def->Params);
            if (!effect)
            {
                floge("missing effect %d", def->Anim);
                continue;
            }
            // the FXServer uses the segment Id, NOT the effect Id
            fxServer.AddSegment(i, def->Seg, effect);
        }
    }

    flogv("FX start");
    // start the effects going
    fxServer.Start();
}

void Lights::Run()
{
    // need to run the effects each loop cycle
    fxServer.Run();
}

#if 0   // toJson
void Lights::toJson(JsonDocument& doc)
{
    JsonArray array = doc["Lights"].to<JsonArray>();
  
    for (int index = 0; index < Lights_COUNT; index++)
    {
        auto def = &LightDefs[index];
        char name[2] = {0,0};
        name[0] = def->Id;
        JsonObject obj = array.add<JsonObject>();
        obj["Name"] = name;
        obj["Anim"] = def->Anim;
        obj["Color0"] = def->Params.Color0;
        obj["Color1"] = def->Params.Color1;
        obj["Speed"] = def->Params.Speed;
        obj["Speed"] = def->Params.Reverse;
        obj["On"] = def->Params.On;
    }
}
#endif
