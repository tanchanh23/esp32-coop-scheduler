
/**
 * 
 * FILE       : peripheral.h
 * PROJECT    : 
 * AUTHOR     : 
 * DESCRITION : 
 *
 */

#pragma once
#include <Arduino.h>
#include "src/SerialDebug/SerialDebug.h"
#include "src/RTClib.h"

/******************************************************************/
#define OPENEDPROX 15 // GPIO15/D15
#define CLOSEDPROX 4  // GPIO4/D4
#define LDRADCPIN 34  // Available GPIO32, GPIO33, GPIO34, GPIO35, GPIO36, GPIO39

#define L298N_EN 14  // GPIO14/D14
#define L298N_IN2 26 // GPIO26/D26
#define L298N_IN1 27 // GPIO27/D27
#define REDALERT 5   // GPIO5/D5

#define POWER_12V 12 // GPIO12/D12

//
#define DEFAULT_LDR_THRESHOLD 1400
#define MAXIMUM_MOTORTIME 30000
//
void init_peripherals();
void stop_motor();
void forward_motor();
void backward_motor();

enum MOTOR_DIRECTION
{
  STOPPED,
  FORWARD,
  BACKWARD
};

void check_prox();

extern uint8_t motor_direction;
extern bool closed_prox;
extern bool opened_prox;
extern bool alert_led;
extern uint16_t realldr;

extern RTC_DS3231 ds3231;
