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
    const char* Path;
    uint8_t Anim;
    FXParams Params;
    FXSegmentBase* Seg;
    uint8_t SegId;
};

#define DEF_SPEED 1000

// Pixels grouped into logical elements, ordered by pixel number.
const uint16_t Pixels_Warning        [] = { 1, 5, 16, 17, 18, 19, 23, 24, 42, /*63, 66, 98, 99, 102, 103,*/ PMAP_END };
const uint16_t Pixels_Landing        [] = { 2, 4, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 21, 22, 25, /*62, 64, 65, 67, 68, 69, 70, 71, 72, 73, 74, 100, 101, 104, 105,*/ PMAP_END };
const uint16_t Pixels_Headlight      [] = { 20, /*61,*/ PMAP_END };
const uint16_t Pixels_Sconce         [] = { 26, 27, 28, 29, 30, 31, 32, 33, 34, /*58, 59, 60,*/ PMAP_END };
const uint16_t Pixels_Floor          [] = { 44, 45, 46, 47, 48, 57, PMAP_END };
const uint16_t Pixels_Engine         [] = { /*75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97,*/ PMAP_END };

LightDef LightDefs[] = 
{//             Path   Anim                 On     Color1    Color2    Speed      Reverse Segment
{ /*Engine*/	"le",  FX_STATIC,  FXParams(false, 0x000040, 0x004000, DEF_SPEED, false), new FXStripSegMapped(Pixels_Engine, &NeoBus)    },
{ /*Landing*/	"ll",  FX_STATIC,  FXParams(false, 0x404040, 0x000000, DEF_SPEED, false), new FXStripSegMapped(Pixels_Landing, &NeoBus)   },
{ /*Warning*/	"lw",  FX_STATIC,  FXParams(false, 0x400000, 0x000000, DEF_SPEED, false), new FXStripSegMapped(Pixels_Warning, &NeoBus)   },
{ /*Headlight*/ "ld",  FX_STATIC,  FXParams(false, 0x404040, 0x000000, DEF_SPEED, false), new FXStripSegMapped(Pixels_Headlight, &NeoBus) },
{ /*Gunwell*/   "lg",  FX_STATIC,  FXParams(false, 0x804040, 0x000000, DEF_SPEED, false), new FXStripSegRange(0, &NeoBus)                 },
{ /*Ramp*/      "lr",  FX_STATIC,  FXParams(false, 0x404040, 0x000000, DEF_SPEED, false), new FXStripSegRange(3, &NeoBus)                 },
{ /*Sconce*/    "lts", FX_STATIC,  FXParams(false, 0x004000, 0x000000, DEF_SPEED, false), new FXStripSegMapped(Pixels_Sconce, &NeoBus)    },
{ /*Floor*/     "ltf", FX_STATIC,  FXParams(false, 0x004040, 0x000000, DEF_SPEED, false), new FXStripSegMapped(Pixels_Floor, &NeoBus)     },
{ /*Bay*/	    "lhy", FX_STATIC,  FXParams(false, 0x404040, 0x000000, DEF_SPEED, false), new FXStripSegRange(55, 56, &NeoBus)            },
{ /*Bed*/       "lhb", FX_STATIC,  FXParams(false, 0xFFFFFF, 0x000000, DEF_SPEED, false), new FXLedSegBase(Pin_LED_Hold_Monitor)          },
{ /*Grates*/    "lhg", FX_STATIC,  FXParams(false, 0x404040, 0x000000, DEF_SPEED, false), new FXStripSegRange(53, 54, &NeoBus)            },
{ /*Monitor*/   "lhm", FX_STATIC,  FXParams(false, 0xFFFFFF, 0x000000, DEF_SPEED, false), new FXLedSegBase(Pin_LED_Hold_Bed)              },
{ /*Red*/	    "lh0", FX_STATIC,  FXParams(false, 0x400000, 0x000000, DEF_SPEED, false), new FXStripSegRange(49, &NeoBus)                },
{ /*Green*/	    "lh1", FX_STATIC,  FXParams(false, 0x004000, 0x000000, DEF_SPEED, false), new FXStripSegRange(51, &NeoBus)                },
{ /*Blue*/	    "lh2", FX_STATIC,  FXParams(false, 0x000040, 0x000000, DEF_SPEED, false), new FXStripSegRange(52, &NeoBus)                },
{ /*Yellow*/    "lh3", FX_STATIC,  FXParams(false, 0x004040, 0x000000, DEF_SPEED, false), new FXStripSegRange(50, &NeoBus)                },
{ /*Monitor*/   "lcm", FX_BLINK,   FXParams(false, 0xFFFFFF, 0x000000, DEF_SPEED, false), new FXLedSegBase(Pin_LED_Cockpit_Monitor)       },
{ /*Red*/	    "lc0", FX_STATIC,  FXParams(false, 0x400000, 0x000000, DEF_SPEED, false), new FXStripSegRange(37, &NeoBus)                },
{ /*Green*/	    "lc1", FX_STATIC,  FXParams(false, 0x004000, 0x000000, DEF_SPEED, false), new FXStripSegRange(35, &NeoBus)                },
{ /*Blue*/	    "lc2", FX_STATIC,  FXParams(false, 0x000040, 0x000000, DEF_SPEED, false), new FXStripSegRange(36, &NeoBus)                },
{ /*Yellow*/    "lc3", FX_STATIC,  FXParams(false, 0x404000, 0x000000, DEF_SPEED, false), new FXStripSegRange(38, &NeoBus)                },
{ /*WallUL*/    "lc4", FX_STATIC,  FXParams(false, 0x404040, 0x000000, DEF_SPEED, false), new FXStripSegRange(41, &NeoBus)                },
{ /*WallUR*/    "lc5", FX_STATIC,  FXParams(false, 0x404040, 0x000000, DEF_SPEED, false), new FXStripSegRange(40, &NeoBus)                },
{ /*WallLL*/    "lc6", FX_STATIC,  FXParams(false, 0x404040, 0x000000, DEF_SPEED, false), new FXStripSegRange(43, &NeoBus)                },
{ /*WallLR*/    "lc7", FX_STATIC,  FXParams(false, 0x404040, 0x000000, DEF_SPEED, false), new FXStripSegRange(39, &NeoBus)                },
{ /*Test Neo*/  "lz",  FX_BLINK,   FXParams(true,  0x000040, 0x004000, DEF_SPEED, false), new FXStripSegRange(0, &NeoOne)                 },
};

void GroupConnector::Push(OMObject *obj, OMProperty *prop)
{
    // only one property
    // all lights in the group turned on or off
    bool value = ((OMPropertyBool *)prop)->Value;
    for (auto o : obj->Objects)
    {
        auto p = (OMPropertyBool *)o->GetProperty('o');
        if (p->Value == value)
            continue;
        // push the value to the light
        p->Set(value);
        // update the controller
        p->Send();
    }
}

void GroupConnector::Pull(OMObject *obj, OMProperty *prop)
{
    // only one property
    // set On if any in the group are On
    for (auto o : obj->Objects)
    {
        auto p = (OMPropertyBool *)o->GetProperty('o');
        if (p->Value)
        {
            ((OMPropertyBool *)prop)->Value = true;
            return;
        }
    }
    ((OMPropertyBool *)prop)->Value = false;
}

void LightConnector::Init(OMObject* obj)
{
    obj->Data = nullptr;
    auto path = obj->GetPath();
    for (int i = 0; i < sizeof(LightDefs) / sizeof(LightDefs[0]); i++)
    {
        if (path == LightDefs[i].Path)
        {
            obj->Data = &LightDefs[i];
            break;
        }
    }
    if (!obj->Data)
        floge("path not found: %s", path);
}

void LightConnector::Push(OMObject* obj, OMProperty* prop)
{
    auto def = (LightDef*)obj->Data;
    auto id = prop->Id;
    switch (id)
    {
    case 'o':   // Light on/off
        {
            def->Params.On = ((OMPropertyBool*)prop)->Value;
            fxServer.GetEffect(def->SegId)->Reset();
            auto group = (OMObject*)obj->Parent;
            if (group->Properties.size() == 1)
            {
                // in a group
                auto pOn = ((OMPropertyBool *)group->GetProperty('o'));
                bool wasOn = pOn->Value;
                // let the group decide if it should still be on
                pOn->Pull();
                // send the value if it changed
                if (pOn->Value != wasOn)
                    pOn->Send();
            }
        }
        break;
    case 'a':   // Animation effect
        {
            // create the new effect by index/id
            auto value = ((OMPropertyLong*)prop)->Value;
            auto newEffect = fxFactory.CreateEffect((uint8_t)value, def->Seg, &def->Params);
            if (!newEffect)
            {
                floge("invalid Light animation value: [%d]", value);
                return;
            }
            def->Anim = value;
            // setting the effect copies the Params pointer and resets the effect
            auto oldEffect = fxServer.SetEffect(def->SegId, newEffect);
            delete oldEffect;
        }
        break;
    case 'c':   // Color1
    case 'd':   // Color2
        {
            uint32_t& v = id == 'c' ? def->Params.Color0 : def->Params.Color1;
            v = ((OMPropertyLong*)prop)->Value;
            fxServer.GetEffect(def->SegId)->Reset();
        }
        break;
    case 's':   // effect speed
        def->Params.Speed = ((OMPropertyLong*)prop)->Value;
        fxServer.GetEffect(def->SegId)->Reset();
        break;
    case 'r':   // effect reverse
        def->Params.Reverse = ((OMPropertyBool*)prop)->Value;
        fxServer.GetEffect(def->SegId)->Reset();
        break;
    }
}

void LightConnector::Pull(OMObject *obj, OMProperty *prop)
{
    auto def = (LightDef*)obj->Data;
    auto id = prop->Id;
    switch (id)
    {
    case 'o':   // Light on/off
        ((OMPropertyBool*)prop)->Value = def->Params.On;
        break;
    case 'a':   // Animation effect
        ((OMPropertyLong*)prop)->Value = def->Anim;
        break;
    case 'c':   // Color1
    case 'd':   // Color2
        ((OMPropertyLong*)prop)->Value = id == 'c' ? def->Params.Color0 : def->Params.Color1;
        break;
    case 's':   // effect speed
        ((OMPropertyLong*)prop)->Value = def->Params.Speed;
        break;
    case 'r':   // effect reverse
        ((OMPropertyBool*)prop)->Value = def->Params.Reverse;
        break;
    }
}

GroupConnector GroupConn;
LightConnector LightConn;

Lights Lights::lights;

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

	for (int i = 0; i < sizeof(LightDefs) / sizeof(LightDefs[0]); i++)
    {
        LightDef* def = &LightDefs[i];
        def->SegId = i;
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
