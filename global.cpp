/**
 * 
 * FILE       : global.cpp
 * PROJECT    : 
 * AUTHOR     : 
 * DESCRITION : 
 *
 */

#include "global.h"
#include "peripheral.h"
#include <FS.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

uint8_t systemMode = DUMMY;
RTC_DATA_ATTR int bootCount = 0;

// Unique id of ESP32 core
char chip_serial[30];
// const int32_t gmtOffset_sec = 3600 * 8;
const int32_t gmtOffset_sec = 3600 * 11;
const int32_t daylightOffset_sec = 0;
struct Config config;

State stopped(entered_stopped_callback, NULL, NULL);
State forward(NULL);
State opened(NULL);
State backward(NULL);
State closed(NULL);
State alert(NULL);
FSM control_machine(stopped);

uint32_t fallen_sleep_time = 0;
/**
 * 
*/
void readChipId()
{
    // Get unique chip id of ESP32 core
    uint8_t bytes[8] = {0};
    uint64_t id = ESP.getEfuseMac();
    uint8_t *p = (uint8_t *)&id;
    for (uint8_t idx = 0; idx < 8; idx++)
        bytes[idx] = p[idx];
    sprintf(chip_serial, "%02x%02x%02x%02x%02x%02x",
            bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5]);
}

void loadFSConfig()
{
    if (SPIFFS.exists("/config.json"))
    {
        // File exists, reading and loading
        File configFile = SPIFFS.open("/config.json", "r");
        debugA("Loading system configuration ...\r\n");

        if (configFile)
        {
            //
            StaticJsonDocument<2048> doc;
            // Deserialize the JSON document
            DeserializationError error = deserializeJson(doc, configFile);
            if (error)
                debugE("Failed to read configuration file, using default configuration \r\n");

            config.mode = doc["mode"] | (uint8_t)RTCONLY;
            config.ldrthreshold = doc["ldrthreshold"] | (uint16_t)DEFAULT_LDR_THRESHOLD;

            JsonArray segments = doc["weekOfDays"].as<JsonArray>();
            uint8_t index = 0;
            for (JsonObject obj : segments)
            {
                config.segments[index].wakehour = (int8_t)obj["wakehour"];
                config.segments[index].wakemin = (int8_t)obj["wakemin"];
                config.segments[index].wakesec = (int8_t)obj["wakesec"];
                config.segments[index].sleephour = (int8_t)obj["sleephour"];
                config.segments[index].sleepmin = (int8_t)obj["sleepmin"];
                config.segments[index].sleepsec = (int8_t)obj["sleepsec"];
                index++;
            }

            //
            config.ldrthreshold = (int16_t)doc["ldrthreshold"];
            config.same_schedule = doc["same_schedule"] | false;

            // Close the file (Curiously, File's destructor doesn't close the file)
            configFile.close();
            debugA("Mode: %d ...\r\n", config.mode);
        }
        else
        {
            debugE("Fatal error found to read configuration \r\n");
        }
    }
    else
    {
        debugE("No found config file in SPIFFS, will save default config\r\n");
        config.mode = RTCONLY;
        config.ldrthreshold = DEFAULT_LDR_THRESHOLD;
        config.same_schedule = false;

        for (uint8_t idx = 0; idx < sizeof(config.segments) / sizeof(TimeSegment); idx++)
        {
            config.segments[idx].wakehour = 0;
            config.segments[idx].wakemin = 0;
            config.segments[idx].wakesec = 0;
            config.segments[idx].sleephour = 23;
            config.segments[idx].sleepmin = 59;
            config.segments[idx].sleepsec = 59;
        }
        saveFSConfig();
    }
}

void saveFSConfig()
{
    StaticJsonDocument<1024> doc;
    doc["mode"] = config.mode;
    doc["ldrthreshold"] = config.ldrthreshold;
    doc["same_schedule"] = config.same_schedule;
    JsonArray segments = doc.createNestedArray("weekOfDays");
    for (uint8_t idx = 0; idx < sizeof(config.segments) / sizeof(TimeSegment); idx++)
    {
        JsonObject segment = segments.createNestedObject();
        //     {"wakehour": 0, "wakemin": 0, "wakesec": 0, "sleephour": 23, "sleepmin": 59, "sleepsec": 59},
        segment["wakehour"] = config.segments[idx].wakehour;
        segment["wakemin"] = config.segments[idx].wakemin;
        segment["wakesec"] = config.segments[idx].wakesec;
        segment["sleephour"] = config.segments[idx].sleephour;
        segment["sleepmin"] = config.segments[idx].sleepmin;
        segment["sleepsec"] = config.segments[idx].sleepsec;
    }

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile)
    {
        debugE("Failed to open config file for writing \r\n");
    }
    else
    {
        debugA("Successfully saved current configuration into SPIFFS ...\r\n");
    }
    serializeJson(doc, configFile);
    configFile.close();
}

void entered_stopped_callback()
{
    fallen_sleep_time = millis();
}
