/**
 *
 * FILE       : configweb.cpp
 * PROJECT    :
 * AUTHOR     :
 *
 */

#include "configweb.h"
#include "global.h"
#include "src/SerialDebug/SerialDebug.h"

#include "peripheral.h"
#include "src/SPIFFSEditor.h"
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <FS.h>
#include <SPIFFS.h>
#include <Update.h>

/*****************************************************************************/
AsyncWebServer server(80);
DNSServer dns;
AsyncWiFiManager wifiManager(&server, &dns);
static uint32_t requestedTimeMs = millis();
// const uint32_t NoRequestTimeMs = 60000;
// Flag to use from web update to reboot the ESP
bool shouldReboot = false;
/*****************************************************************************/

void initConfigWebService()
{
    char ap_ssid[64];
    sprintf(ap_ssid, "%s-%s", SOFTAP_SSID_PREFIX, chip_serial);
    wifiManager.startConfigPortalModeless(ap_ssid, SOFTAP_PSK);

    // MDNS.addService("http", "tcp", 80);

    server.addHandler(new SPIFFSEditor(SPIFFS, HTTP_EDIT_USER, HTTP_EDIT_PASS));
    server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", String(ESP.getFreeHeap()));
    });

    server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.htm");
    server.serveStatic("/monitor", SPIFFS, "/index.htm");
    // server.serveStatic("/upload", SPIFFS, "/upload.htm");

    server.on(
        "/firmware", HTTP_POST,
        [](AsyncWebServerRequest *request) {
            shouldReboot = !Update.hasError();
            AsyncWebServerResponse *response = request->beginResponse(
                200, "text/plain",
                shouldReboot
                    ? "Firmware has updated successfully, will be rebooted!"
                    : "FAIL!");
            response->addHeader("Connection", "close");
            request->send(response);
            ESP.restart();
        },
        [](AsyncWebServerRequest *request, String filename, size_t index,
           uint8_t *data, size_t len, bool final) {
            if (!index)
            {
                Serial.printf("Firmware updating started: %s\n", filename.c_str());
                // Update.runAsync(true);
                // if(!Update.begin((ESP.getFreeSketchSpace() - 0x1000) &
                // 0xFFFFF000)){
                if (!Update.begin())
                {
                    Update.printError(Serial);
                }
            }
            if (!Update.hasError())
            {
                if (Update.write(data, len) != len)
                {
                    Update.printError(Serial);
                }
            }
            if (final)
            {
                if (Update.end(true))
                {
                    Serial.printf("Firmware updating success: %uB\n", index + len);
                }
                else
                {
                    Update.printError(Serial);
                }
            }
        });

    server.on(
        "/spiffs", HTTP_POST,
        [](AsyncWebServerRequest *request) {
            shouldReboot = !Update.hasError();
            AsyncWebServerResponse *response = request->beginResponse(
                200, "text/plain",
                shouldReboot ? "Data has uploaded successfully, will be rebooted!"
                             : "FAIL!");
            response->addHeader("Connection", "close");
            request->send(response);
            ESP.restart();
        },
        [](AsyncWebServerRequest *request, String filename, size_t index,
           uint8_t *data, size_t len, bool final) {
            if (!index)
            {
                Serial.printf("Data uploading started: %s\n", filename.c_str());
                if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_SPIFFS))
                {
                    Update.printError(Serial);
                }
            }
            if (!Update.hasError())
            {
                if (Update.write(data, len) != len)
                {
                    Update.printError(Serial);
                }
            }
            if (final)
            {
                if (Update.end(true))
                {
                    Serial.printf("Data uploading success: %uB\n", index + len);
                }
                else
                {
                    Update.printError(Serial);
                }
            }
        });

    server.on("/readconf", HTTP_GET, [](AsyncWebServerRequest *request) {
        StaticJsonDocument<1024> doc;
        doc["mode"] = config.mode;
        doc["ldrthreshold"] = config.ldrthreshold;
        doc["same_schedule"] = config.same_schedule;
        JsonArray segments = doc.createNestedArray("weekOfDays");
        for (uint8_t idx = 0; idx < sizeof(config.segments) / sizeof(TimeSegment);
             idx++)
        {
            JsonObject segment = segments.createNestedObject();
            segment["wakehour"] = config.segments[idx].wakehour;
            segment["wakemin"] = config.segments[idx].wakemin;
            segment["wakesec"] = config.segments[idx].wakesec;
            segment["sleephour"] = config.segments[idx].sleephour;
            segment["sleepmin"] = config.segments[idx].sleepmin;
            segment["sleepsec"] = config.segments[idx].sleepsec;
        }
        char response[1024];
        serializeJson(doc, response);
        fallen_sleep_time = millis();
        request->send(200, "text/plain", response);
    });

    server.on("/realvars", HTTP_GET, [](AsyncWebServerRequest *request) {
        fallen_sleep_time = millis();

        StaticJsonDocument<1024> doc;
        doc["realldr"] = realldr;
        doc["closed_prox"] = closed_prox;
        doc["opened_prox"] = opened_prox;
        doc["alert_led"] = alert_led;
        doc["motor_direction"] = motor_direction;
        char response[1024];
        serializeJson(doc, response);
        request->send(200, "text/plain", response);
    });

    server.on("/motor", HTTP_POST, [](AsyncWebServerRequest *request) {
        fallen_sleep_time = millis();

        int args = request->args();
        for (int i = 0; i < args; i++)
        {
            Serial.printf("ARG[%s]: %s\n", request->argName(i).c_str(),
                          request->arg(i).c_str());
        }
        AsyncWebParameter *motor_dir_para =
            request->getParam("motor_direction", true);
        uint8_t motor_direction_req = motor_dir_para->value().toInt();
        motor_direction = motor_direction_req;
        request->send(200, "text/plain", "");
    });

    server.on("/redled", HTTP_POST, [](AsyncWebServerRequest *request) {
        fallen_sleep_time = millis();

        int args = request->args();
        for (int i = 0; i < args; i++)
        {
            Serial.printf("ARG[%s]: %s\n", request->argName(i).c_str(),
                          request->arg(i).c_str());
        }
        AsyncWebParameter *led_req = request->getParam("led", true);
        alert_led = led_req->value() == "true";
        request->send(200, "text/plain", "");
    });

    server.on("/schedule", HTTP_POST, [](AsyncWebServerRequest *request) {
        fallen_sleep_time = millis();

        int args = request->args();
        for (int i = 0; i < args; i++)
        {
            Serial.printf("ARG[%s]: %s\n", request->argName(i).c_str(),
                          request->arg(i).c_str());
        }
        if (args > 5)
        {
            for (uint8_t idx = 0; idx < sizeof(config.segments) / sizeof(TimeSegment);
                 idx++)
            {
                char wakeupVarName[25];
                sprintf(wakeupVarName, "weekOfDays[%d][wakeup]", idx);
                char sleepVarName[25];
                sprintf(sleepVarName, "weekOfDays[%d][sleep]", idx);
                AsyncWebParameter *wakeup = request->getParam(wakeupVarName, true);
                config.segments[idx].wakehour = wakeup->value().substring(0, 2).toInt();
                config.segments[idx].wakemin = wakeup->value().substring(3, 5).toInt();

                AsyncWebParameter *sleep = request->getParam(sleepVarName, true);
                config.segments[idx].sleephour = sleep->value().substring(0, 2).toInt();
                config.segments[idx].sleepmin = sleep->value().substring(3, 5).toInt();
            }
            config.same_schedule = false;
        }
        else
        {
            for (uint8_t idx = 0; idx < sizeof(config.segments) / sizeof(TimeSegment);
                 idx++)
            {
                char wakeupVarName[25];
                sprintf(wakeupVarName, "weekOfDays[%d][wakeup]", 0);
                char sleepVarName[25];
                sprintf(sleepVarName, "weekOfDays[%d][sleep]", 0);
                AsyncWebParameter *wakeup = request->getParam(wakeupVarName, true);
                config.segments[idx].wakehour = wakeup->value().substring(0, 2).toInt();
                config.segments[idx].wakemin = wakeup->value().substring(3, 5).toInt();
                AsyncWebParameter *sleep = request->getParam(sleepVarName, true);
                config.segments[idx].sleephour = sleep->value().substring(0, 2).toInt();
                config.segments[idx].sleepmin = sleep->value().substring(3, 5).toInt();

                Serial.println(config.segments[idx].sleepmin);
            }
            config.same_schedule = true;
        }
        saveFSConfig();
        request->redirect("/monitor");
    });

    server.on("/ldrthreshold", HTTP_POST, [](AsyncWebServerRequest *request) {
        fallen_sleep_time = millis();

        int args = request->args();
        for (int i = 0; i < args; i++)
        {
            Serial.printf("ARG[%s]: %s\n", request->argName(i).c_str(),
                          request->arg(i).c_str());
        }

        AsyncWebParameter *ldr_threshold = request->getParam("ldrthreshold", true);
        config.ldrthreshold = ldr_threshold->value().toInt();

        saveFSConfig();
        request->redirect("/monitor");
    });

    server.on("/mode", HTTP_POST, [](AsyncWebServerRequest *request) {
        fallen_sleep_time = millis();

        int args = request->args();
        for (int i = 0; i < args; i++)
        {
            Serial.printf("ARG[%s]: %s\n", request->argName(i).c_str(),
                          request->arg(i).c_str());
        }

        AsyncWebParameter *mode = request->getParam("mode", true);
        config.mode = mode->value().toInt();
        saveFSConfig();
        control_machine.transitionTo(stopped);
        request->redirect("/monitor");
    });

    server.on("/systemtime", HTTP_GET, [](AsyncWebServerRequest *request) {
        fallen_sleep_time = millis();

        struct tm tmstruct;
        getLocalTime(&tmstruct);
        char response[64];
        sprintf(response, "%04d-%02d-%02dT%02d:%02d", tmstruct.tm_year + 1900, tmstruct.tm_mon + 1, tmstruct.tm_mday,
                tmstruct.tm_hour, tmstruct.tm_min);
        request->send(200, "text/plain", response);
    });

    server.on("/synctime", HTTP_POST, [](AsyncWebServerRequest *request) {
              fallen_sleep_time = millis();

              debugA("From STA: Setting RTC time ...\r\n");
              int args = request->args();
              for (int i = 0; i < args; i++)
              {
                  Serial.printf("ARG[%s]: %s\n", request->argName(i).c_str(),
                                request->arg(i).c_str());
              }
              AsyncWebParameter *timeArg = request->getParam("time", true);
              uint32_t timeFromEpoch = timeArg->value().toInt();
              time_t rtc = timeFromEpoch;
              timeval tv = {rtc, 0};
              settimeofday(&tv, nullptr);
              struct tm tmstruct;
              getLocalTime(&tmstruct);
              char response[64];
              sprintf(response, "%04d-%02d-%02dT%02d:%02d", tmstruct.tm_year + 1900, tmstruct.tm_mon + 1, tmstruct.tm_mday,
                      tmstruct.tm_hour, tmstruct.tm_min);

              ds3231.adjust(DateTime(tmstruct.tm_year + 1900, tmstruct.tm_mon + 1, tmstruct.tm_mday, tmstruct.tm_hour, tmstruct.tm_min, 0));
              request->send(200, "text/plain", "");
          })
        .setFilter(ON_STA_FILTER);

    server.on("/synctime", HTTP_POST, [](AsyncWebServerRequest *request) {
              fallen_sleep_time = millis();

              debugA("From AP: Setting RTC time ...\r\n");
              int args = request->args();
              for (int i = 0; i < args; i++)
              {
                  Serial.printf("ARG[%s]: %s\n", request->argName(i).c_str(),
                                request->arg(i).c_str());
              }
              AsyncWebParameter *timeArg = request->getParam("time", true);
              uint32_t timeFromEpoch = timeArg->value().toInt();
              //   uint32_t timeFromEpoch = timeArg->value().toInt() - gmtOffset_sec;
              time_t rtc = timeFromEpoch;
              timeval tv = {rtc, 0};
              settimeofday(&tv, nullptr);
              struct tm tmstruct;
              getLocalTime(&tmstruct);
              char response[64];
              sprintf(response, "%04d-%02d-%02dT%02d:%02d", tmstruct.tm_year + 1900, tmstruct.tm_mon + 1, tmstruct.tm_mday,
                      tmstruct.tm_hour, tmstruct.tm_min);

              ds3231.adjust(DateTime(tmstruct.tm_year + 1900, tmstruct.tm_mon + 1, tmstruct.tm_mday, tmstruct.tm_hour, tmstruct.tm_min, 0));
              request->send(200, "text/plain", "");
          })
        .setFilter(ON_AP_FILTER);

    server.on("/resetalert", HTTP_POST, [](AsyncWebServerRequest *request) {
        fallen_sleep_time = millis();

        int args = request->args();
        for (int i = 0; i < args; i++)
        {
            Serial.printf("ARG[%s]: %s\n", request->argName(i).c_str(),
                          request->arg(i).c_str());
        }
        control_machine.transitionTo(stopped);
        request->redirect("/monitor");
    });

    server.onNotFound([](AsyncWebServerRequest *request) {
        fallen_sleep_time = millis();

        Serial.printf("NOT_FOUND: ");
        if (request->method() == HTTP_GET)
            Serial.printf("GET");
        else if (request->method() == HTTP_POST)
            Serial.printf("POST");
        else if (request->method() == HTTP_DELETE)
            Serial.printf("DELETE");
        else if (request->method() == HTTP_PUT)
            Serial.printf("PUT");
        else if (request->method() == HTTP_PATCH)
            Serial.printf("PATCH");
        else if (request->method() == HTTP_HEAD)
            Serial.printf("HEAD");
        else if (request->method() == HTTP_OPTIONS)
            Serial.printf("OPTIONS");
        else
            Serial.printf("UNKNOWN");
        Serial.printf(" http://%s%s\n", request->host().c_str(),
                      request->url().c_str());

        if (request->contentLength())
        {
            Serial.printf("_CONTENT_TYPE: %s\n", request->contentType().c_str());
            Serial.printf("_CONTENT_LENGTH: %u\n", request->contentLength());
        }

        int headers = request->headers();
        int i;
        for (i = 0; i < headers; i++)
        {
            AsyncWebHeader *h = request->getHeader(i);
            Serial.printf("_HEADER[%s]: %s\n", h->name().c_str(), h->value().c_str());
        }

        int params = request->params();
        for (i = 0; i < params; i++)
        {
            AsyncWebParameter *p = request->getParam(i);
            if (p->isFile())
            {
                Serial.printf("_FILE[%s]: %s, size: %u\n", p->name().c_str(),
                              p->value().c_str(), p->size());
            }
            else if (p->isPost())
            {
                Serial.printf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
            }
            else
            {
                Serial.printf("_GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
            }
        }
        request->send(404);
    });

    server.onFileUpload([](AsyncWebServerRequest *request, const String &filename,
                           size_t index, uint8_t *data, size_t len, bool final) {
        if (!index)
            Serial.printf("UploadStart: %s\n", filename.c_str());
        Serial.printf("%s", (const char *)data);
        if (final)
            Serial.printf("UploadEnd: %s (%u)\n", filename.c_str(), index + len);
    });

    server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data,
                            size_t len, size_t index, size_t total) {
        if (!index)
            Serial.printf("BodyStart: %u\n", total);
        Serial.printf("%s", (const char *)data);
        if (index + len == total)
            Serial.printf("BodyEnd: %u\n", total);
    });
    // server.begin();
}

void configWebProc() { wifiManager.loop(); }
