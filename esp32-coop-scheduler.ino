
#include "global.h"
#include "src/SPIFFSEditor.h"
#include <ESPmDNS.h>
#include <FS.h>
#include <SPIFFS.h>
#include <Update.h>
#include "src/RTClib.h"

#include "configweb.h"
#include "peripheral.h"
#include "rtcldr_mode.h"
#include "rtconly_mode.h"
#include "time.h"

void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info)
{
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

void setTimeZone(long offset, int daylight)
{
    char cst[17] = {0};
    char cdt[17] = "DST";
    char tz[33] = {0};

    if (offset % 3600)
    {
        sprintf(cst, "UTC%ld:%02u:%02u", offset / 3600, abs((offset % 3600) / 60),
                abs(offset % 60));
    }
    else
    {
        sprintf(cst, "UTC%ld", offset / 3600);
    }
    if (daylight != 3600)
    {
        long tz_dst = offset - daylight;
        if (tz_dst % 3600)
        {
            sprintf(cdt, "DST%ld:%02u:%02u", tz_dst / 3600, abs((tz_dst % 3600) / 60),
                    abs(tz_dst % 60));
        }
        else
        {
            sprintf(cdt, "DST%ld", tz_dst / 3600);
        }
    }
    sprintf(tz, "%s%s", cst, cdt);
    setenv("TZ", tz, 1);
    tzset();
}

void setup()
{
    //
    Serial.begin(115200);

    // Get cunique chip ID of ESP32 core
    (void)readChipId();
    debugA("Unique ID of core is %s\r\n", chip_serial);

    // Initialize SPIFFS module
    while (!SPIFFS.begin())
    {
        debugE("Failed to mount SPIFFS ... \r\n");
        SPIFFS.format();
        delay(2500);
    }

    // Loading configuration from SPIFFS
    (void)loadFSConfig();

    // Parse wakeup reason
    ++bootCount;
    esp_sleep_wakeup_cause_t wakeup_reason;
    wakeup_reason = esp_sleep_get_wakeup_cause();

    switch (wakeup_reason)
    {
    case ESP_SLEEP_WAKEUP_EXT0:
        debugA("Wakeup caused by external signal using RTC_IO\r\n");
        break;

    case ESP_SLEEP_WAKEUP_EXT1:
    {
        debugA("Wakeup caused by external signal using RTC_CNTL\r\n");
        int GPIO_reason = esp_sleep_get_ext1_wakeup_status();
        uint8_t pin = log(GPIO_reason) / log(2);
        debugA("GPIO that triggered the wake up: GPIO %d\r\n", pin);
    }
    break;

    case ESP_SLEEP_WAKEUP_TIMER:
        debugA("Wakeup caused by timer\r\n");
        break;

    case ESP_SLEEP_WAKEUP_TOUCHPAD:
        debugA("Wakeup caused by touchpad\r\n");
        break;

    case ESP_SLEEP_WAKEUP_ULP:
        debugA("Wakeup caused by ULP program\r\n");
        break;

    default:
        debugA("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
        break;
    }

    WiFi.onEvent(WiFiGotIP, WiFiEvent_t::SYSTEM_EVENT_STA_GOT_IP);
    (void)initConfigWebService();

    // Initialize and get the time
    // configTime(gmtOffset_sec, daylightOffset_sec, "time.nist.gov",
    //            "0.pool.ntp.org", "pool.ntp.org");
    setTimeZone(-gmtOffset_sec, daylightOffset_sec);

    // Init peripherals
    init_peripherals();
    fallen_sleep_time = millis();
}

void loop()
{
    configWebProc();

    // Peripheral part
    digitalWrite(REDALERT, alert_led);
    switch (motor_direction)
    {
    case STOPPED:
    {
        stop_motor();
    }
    break;

    case FORWARD:
    {
        forward_motor();
    }
    break;

    case BACKWARD:
    {
        backward_motor();
    }
    break;
    }
    realldr = analogRead(LDRADCPIN);
    check_prox();

    //
    control_machine.run();

    switch (config.mode)
    {
    case RTCONLY:
    {
        static uint32_t tickMs = millis();
        if (millis() - tickMs >= 2000)
        {
            tickMs = millis();
            struct tm tmstruct;
            getLocalTime(&tmstruct);
            if (tmstruct.tm_year > 100)
            {
                uint8_t weekday = weekday;
                if (config.same_schedule)
                    weekday = 0;
                // When valid time condition, checking time condition
                uint32_t current_time =
                    tmstruct.tm_hour * 3600 + tmstruct.tm_min * 60 + tmstruct.tm_sec;
                uint32_t wakeup_time = config.segments[weekday].wakehour * 3600 +
                                       config.segments[weekday].wakemin * 60 +
                                       config.segments[weekday].wakesec;
                uint32_t sleep_time = config.segments[weekday].sleephour * 3600 +
                                      config.segments[weekday].sleepmin * 60 +
                                      config.segments[weekday].sleepsec;

                debugA("Current: %d-%02d-%02d %02d:%02d:%02d\r\n",
                       tmstruct.tm_year + 1900, tmstruct.tm_mon + 1, tmstruct.tm_mday,
                       tmstruct.tm_hour, tmstruct.tm_min, tmstruct.tm_sec);

                if (control_machine.isInState(stopped))
                {
                    debugA("stopped ...");
                }
                else if (control_machine.isInState(forward))
                {
                    debugA("forward ...");
                }
                else if (control_machine.isInState(opened))
                {
                    debugA("opened ...");
                }
                else if (control_machine.isInState(backward))
                {
                    debugA("backward ...");
                }
                else if (control_machine.isInState(closed))
                {
                    debugA("closed ...");
                }
                else if (control_machine.isInState(alert))
                {
                    debugA("alert ...");
                }

                debugA("Open: %02d:%02d:%02d, Close: %02d:%02d:%02d\r\n",
                       config.segments[weekday].wakehour,
                       config.segments[weekday].wakemin,
                       config.segments[weekday].wakesec,
                       config.segments[weekday].sleephour,
                       config.segments[weekday].sleepmin,
                       config.segments[weekday].sleepsec);

                if (sleep_time > wakeup_time)
                {
                    if ((current_time >= sleep_time) || (current_time < wakeup_time))
                    {
                        if (control_machine.isInState(opened) ||
                            control_machine.isInState(stopped) ||
                            control_machine.isInState(forward))
                        {
                            debugA("Transition to close from stopped/opened in rtc mode");
                            control_machine.transitionTo(backward);
                        }
                    }
                    else
                    {
                        if (control_machine.isInState(closed) ||
                            control_machine.isInState(stopped) ||
                            control_machine.isInState(backward))
                        {
                            debugA("Transition to open from stopped/closed in rtc mode");
                            control_machine.transitionTo(forward);
                        }
                    }

                    if (control_machine.isInState(opened) ||
                        control_machine.isInState(closed) ||
                        control_machine.isInState(stopped))
                    {
                        if (current_time <= wakeup_time)
                        {
                            if (millis() - fallen_sleep_time >= NoRequestTimeMs)
                                rtcOnlySleep(wakeup_time - current_time);
                        }
                        else if (current_time <= sleep_time)
                        {
                            if (millis() - fallen_sleep_time >= NoRequestTimeMs)
                                rtcOnlySleep(sleep_time - current_time);
                        }
                        else
                        {
                            if (millis() - fallen_sleep_time >= NoRequestTimeMs)
                                rtcOnlySleep(86400 - current_time + wakeup_time);
                        }
                    }
                }
                else
                {
                    if ((current_time > sleep_time) && (current_time < wakeup_time))
                    {
                        if (control_machine.isInState(opened) ||
                            control_machine.isInState(stopped) ||
                            control_machine.isInState(forward))
                        {
                            debugA("Transition to close from stopped/opened in rtc mode");
                            control_machine.transitionTo(backward);
                        }
                    }
                    else
                    {
                        if (control_machine.isInState(closed) ||
                            control_machine.isInState(stopped) ||
                            control_machine.isInState(backward))
                        {
                            debugA("Transition to open from stopped/closed in rtc mode");
                            control_machine.transitionTo(forward);
                        }
                    }

                    if (control_machine.isInState(opened) ||
                        control_machine.isInState(closed) ||
                        control_machine.isInState(stopped))
                    {
                        if (current_time <= sleep_time)
                        {
                            if (millis() - fallen_sleep_time >= NoRequestTimeMs)
                                rtcOnlySleep(sleep_time - current_time);
                        }
                        else if (current_time <= wakeup_time)
                        {
                            if (millis() - fallen_sleep_time >= NoRequestTimeMs)
                                rtcOnlySleep(wakeup_time - current_time);
                        }
                        else
                        {
                            if (millis() - fallen_sleep_time >= NoRequestTimeMs)
                                rtcOnlySleep(86400 - current_time + sleep_time);
                        }
                    }
                }
            }
            else
            {
            }
        }
    }
    break;

    case RTCLDR:
    {
        static uint32_t tickMs = millis();
        if (millis() - tickMs >= 2000)
        {
            tickMs = millis();
            struct tm tmstruct;
            getLocalTime(&tmstruct);
            if (tmstruct.tm_year > 100)
            {
                // When valid time condition, checking time condition
                uint8_t weekday = weekday;
                if (config.same_schedule)
                    weekday = 0;
                uint32_t current_time =
                    tmstruct.tm_hour * 3600 + tmstruct.tm_min * 60 + tmstruct.tm_sec;
                uint32_t wakeup_time = config.segments[weekday].wakehour * 3600 +
                                       config.segments[weekday].wakemin * 60 +
                                       config.segments[weekday].wakesec;
                uint32_t sleep_time = config.segments[weekday].sleephour * 3600 +
                                      config.segments[weekday].sleepmin * 60 +
                                      config.segments[weekday].sleepsec;

                debugA("Current: %d-%02d-%02d %02d:%02d:%02d\r\n",
                       tmstruct.tm_year + 1900, tmstruct.tm_mon + 1, tmstruct.tm_mday,
                       tmstruct.tm_hour, tmstruct.tm_min, tmstruct.tm_sec);
                debugA("Open: %02d:%02d:%02d, Close: %02d:%02d:%02d\r\n",
                       config.segments[weekday].wakehour,
                       config.segments[weekday].wakemin,
                       config.segments[weekday].wakesec,
                       config.segments[weekday].sleephour,
                       config.segments[weekday].sleepmin,
                       config.segments[weekday].sleepsec);

                if (sleep_time > wakeup_time)
                {
                    if ((current_time > wakeup_time) && (current_time > sleep_time) &&
                        (realldr > config.ldrthreshold))
                    {
                        if (control_machine.isInState(opened) ||
                            control_machine.isInState(stopped) ||
                            control_machine.isInState(forward))
                        {
                            control_machine.transitionTo(backward);
                        }
                    }
                    else
                    {
                        if (control_machine.isInState(closed) ||
                            control_machine.isInState(stopped) ||
                            control_machine.isInState(backward))
                        {
                            control_machine.transitionTo(forward);
                        }
                    }
                }
                else
                {
                    if ((current_time < sleep_time || current_time > wakeup_time) &&
                        (realldr > config.ldrthreshold))
                    {
                        if (control_machine.isInState(closed) ||
                            control_machine.isInState(stopped) ||
                            control_machine.isInState(backward))
                        {
                            control_machine.transitionTo(forward);
                        }
                    }
                    else
                    {
                        if (control_machine.isInState(opened) ||
                            control_machine.isInState(stopped) ||
                            control_machine.isInState(forward))
                        {
                            control_machine.transitionTo(backward);
                        }
                    }
                }
            }
            else
            {
            }
        }
    }
    break;

    default:
        break;
    }

    if (control_machine.isInState(stopped))
    {
        if (config.mode != TESTMODE)
        {
            motor_direction = 0;
            alert_led = false;
        }
    }
    else if (control_machine.isInState(forward))
    {
        if (config.mode != TESTMODE)
        {
            motor_direction = 1;
            alert_led = false;
        }
        if (control_machine.timeInCurrentState() > MAXIMUM_MOTORTIME)
        {
            if (config.mode != TESTMODE)
            {
                debugA("Transition to alert from open");
                control_machine.transitionTo(alert);
            }
        }
        else
        {
            if (opened_prox)
            {
                debugA("Transition to opened from open");
                control_machine.transitionTo(opened);
            }
        }
    }
    else if (control_machine.isInState(opened))
    {
        if (config.mode != TESTMODE)
        {
            motor_direction = 0;
            alert_led = false;
        }
    }
    else if (control_machine.isInState(backward))
    {

        if (config.mode != TESTMODE)
        {
            motor_direction = 2;
            alert_led = false;
        }

        if (control_machine.timeInCurrentState() > MAXIMUM_MOTORTIME)
        {
            if (config.mode != TESTMODE)
            {
                debugA("Transition to alert from close ");
                control_machine.transitionTo(alert);
            }
        }
        else
        {
            if (closed_prox)
            {
                debugA("Transition to closed from close");
                control_machine.transitionTo(closed);
            }
        }
    }
    else if (control_machine.isInState(closed))
    {
        if (config.mode != TESTMODE)
        {
            motor_direction = 0;
            alert_led = false;
        }
    }
    else if (control_machine.isInState(alert))
    {
        alert_led = true;
        motor_direction = 0;
    }

    if (motor_direction == 0)
    {
        if (closed_prox)
        {
            if (control_machine.isInState(stopped) ||
                control_machine.isInState(backward))
            {
                debugA("Transition to closed ");
                control_machine.transitionTo(closed);
            }
        }
        else if (opened_prox)
        {
            if (control_machine.isInState(stopped) ||
                control_machine.isInState(forward))
            {
                debugA("Transition to opened ");
                control_machine.transitionTo(opened);
            }
        }
        else if (!control_machine.isInState(alert) &&
                 !control_machine.isInState(stopped))
        {
            debugA("Transition to stopped ");
            control_machine.transitionTo(stopped);
        }
    }
}