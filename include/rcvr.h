#pragma once

// PIN_NEOPIXEL = 0 is the onboard neopixel
// NEOPIXEL_I2C_POWER = 2 needs to be HIGH to power the onboard neopixel
#define Pin_NEOSTRIP            32

#define Pin_LED_Cockpit_Monitor LED_BUILTIN // = 13 - the onboard red LED doubling as external LED
#define Pin_LED_Hold_Bed		5
#define Pin_LED_Hold_Monitor	4

#define Pin_RetractedLimitSW	33
#define Pin_ExtendedLimitSW	    27

#define Pin_I2S_DOUT            25
#define Pin_I2S_BCLK            26
#define Pin_I2S_LRC             12

// https://github.com/madhephaestus/ESP32Servo
// All pin numbers are allowed, but only pins 2,4,12-19,21-23,25-27,32-33 are recommended.
#define Pin_Servo               14

#define Pin_Main_Switch         15

extern long mapr(long x, long in_min, long in_max, long out_min, long out_max);
