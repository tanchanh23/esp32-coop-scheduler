/**
 * 
 * FILE       : rtcldr_mode.cpp
 * PROJECT    : 
 * AUTHOR     : 
 * DESCRITION : 
 *
 */
#include "rtcldr_mode.h"
#include "global.h"

void rtcLDRProc()
{
    // static uint32_t tickMs = millis();
    // if(millis() - tickMs >= 2000){
    //     tickMs = millis();
    //     struct tm tmstruct;
    //     getLocalTime(&tmstruct);
    //     if(tmstruct.tm_year > 100){
    //         // When valid time condition, checking time condition
    //         uint32_t current_time = tmstruct.tm_hour * 3600 + tmstruct.tm_min * 60 + tmstruct.tm_sec;
    //         uint32_t wakeup_time = config.segments[tmstruct.tm_wday].wakehour * 3600
    //             + config.segments[tmstruct.tm_wday].wakemin * 60 + config.segments[tmstruct.tm_wday].wakesec;
    //         uint32_t sleep_time = config.segments[tmstruct.tm_wday].sleephour * 3600
    //             + config.segments[tmstruct.tm_wday].sleepmin * 60 + config.segments[tmstruct.tm_wday].sleepsec;

    //         debugA("Current: %d-%02d-%02d %02d:%02d:%02d\r\n", tmstruct.tm_year + 1900, tmstruct.tm_mon + 1, tmstruct.tm_mday,
    //             tmstruct.tm_hour, tmstruct.tm_min, tmstruct.tm_sec);
    //         debugA("Wakeup: %02d:%02d:%02d, Sleep: %02d:%02d:%02d\r\n", config.segments[tmstruct.tm_wday].wakehour,
    //             config.segments[tmstruct.tm_wday].wakemin, config.segments[tmstruct.tm_wday].wakesec,
    //             config.segments[tmstruct.tm_wday].sleephour, config.segments[tmstruct.tm_wday].sleepsec,
    //             config.segments[tmstruct.tm_wday].sleepmin);

    //         if(sleep_time > wakeup_time){
    //             if((current_time >= sleep_time) || (current_time < wakeup_time)){
    //                 rtcLDRSleep();
    //             }
    //             else{
    //                 uint16_t ldrAdcValue = analogRead(LDRADCPIN);
    //                 debugA("Current ADC from LDR module: %d, threshold: %d\r\n", ldrAdcValue, config.ldrthreshold);
    //                 if(ldrAdcValue > config.ldrthreshold){
    //                     rtcLDRSleep();
    //                 }
    //             }
    //         }
    //         else{
    //             if((current_time > sleep_time) && (current_time < wakeup_time)){
    //                 rtcLDRSleep();
    //             }
    //             else{
    //                 uint16_t ldrAdcValue = analogRead(LDRADCPIN);
    //                 debugA("Current ADC from LDR module: %d, threshold: %d\r\n", ldrAdcValue, config.ldrthreshold);
    //                 if(ldrAdcValue > config.ldrthreshold){
    //                     rtcLDRSleep();
    //                 }
    //             }
    //         }
    //     }
    //     else{
    //         rtcLDRSleep();
    //     }
    // }
}
