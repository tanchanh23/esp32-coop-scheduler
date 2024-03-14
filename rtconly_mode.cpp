/**

   FILE       : rtconly_mode.cpp
   PROJECT    :
   AUTHOR     :
   DESCRITION :

*/
#include "rtconly_mode.h"
#include "global.h"
#include "peripheral.h"

void rtcOnlyProc()
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
  //                 rtcOnlySleep();
  //             }
  //         }
  //         else{
  //             if((current_time > sleep_time) && (current_time < wakeup_time)){
  //                 rtcOnlySleep();
  //             }
  //         }
  //     }
  //     else{
  //         rtcOnlySleep();
  //     }
  // }
}

void rtcOnlySleep(uint32_t seconds)
{

  digitalWrite(POWER_12V, HIGH);
  delay(100);
  gpio_hold_en((gpio_num_t)POWER_12V);
  gpio_deep_sleep_hold_en();
  delay(100);
  debugA("Will sleep mode \r\n");

  esp_sleep_enable_timer_wakeup(seconds * uS_TO_S_FACTOR);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_33, 1); //1 = High, 0 = Low
  esp_deep_sleep_start();
}
