
/**
 * 
 * FILE       : global.h
 * PROJECT    : 
 * AUTHOR     : 
 * DESCRITION : 
 *
 */

#pragma once
#include <Arduino.h>
#include "src/SerialDebug/SerialDebug.h"
#include "src/ESPAsyncWiFiManager.h"
#include "src/ESPAsyncWebServer.h"

#include "src/FSM.h"

extern uint8_t systemMode;
extern RTC_DATA_ATTR int bootCount;
#define uS_TO_S_FACTOR 1000000 /* Conversion factor for micro seconds to seconds */
#define NoRequestTimeMs 60000
//
enum SYSTEMMODE
{
    DUMMY,
    TESTMODE,
    RTCONLY,
    RTCLDR,
};

typedef struct
{
    int8_t wakehour;
    int8_t wakemin;
    int8_t wakesec;

    int8_t sleephour;
    int8_t sleepmin;
    int8_t sleepsec;
} TimeSegment;

struct Config
{
    uint8_t mode;

    //
    TimeSegment segments[7];
    uint16_t ldrthreshold;
    bool same_schedule;
};

extern uint32_t fallen_sleep_time;
extern Config config;

// Unique id of ESP32 core
extern char chip_serial[30];
extern const int32_t gmtOffset_sec;
extern const int32_t daylightOffset_sec;

void entered_stopped_callback();

extern State stopped;
extern State forward;
extern State opened;
extern State backward;
extern State closed;
extern State alert;
extern FSM control_machine;

void readChipId();
void loadFSConfig();
void saveFSConfig();
