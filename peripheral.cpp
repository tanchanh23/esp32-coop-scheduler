/**
 * 
 * FILE       : peripheral.cpp
 * PROJECT    : 
 * AUTHOR     : 
 * DESCRITION : 
 *
 */

#include "global.h"
#include "peripheral.h"

#include "src/FBD.h"

RTC_DS3231 ds3231;

// Setting PWM properties
const int freq = 1000;
const int pwmChannel = 0;
const int resolution = 8;

void init_peripherals()
{
    if (!ds3231.begin())
    {
        Serial.println("Couldn't find RTC");
        Serial.flush();
    }
    else
    {
        if (ds3231.lostPower())
        {
            Serial.println("RTC lost power");
        }
        else
        {
            Serial.print("DS3231 RTC: ");
            DateTime now = ds3231.now();
            Serial.print(now.year(), DEC);
            Serial.print('/');
            Serial.print(now.month(), DEC);
            Serial.print('/');
            Serial.print(now.day(), DEC);
            Serial.print(" (");
            Serial.print(now.dayOfTheWeek());
            Serial.print(") ");
            Serial.print(now.hour(), DEC);
            Serial.print(':');
            Serial.print(now.minute(), DEC);
            Serial.print(':');
            Serial.print(now.second(), DEC);
            Serial.println();
            time_t rtc = now.unixtime() - gmtOffset_sec;
            timeval tv = {rtc, 0};
            settimeofday(&tv, nullptr);
        }
    }

    pinMode(OPENEDPROX, INPUT);
    pinMode(CLOSEDPROX, INPUT);

    pinMode(L298N_EN, OUTPUT);
    pinMode(L298N_IN2, OUTPUT);
    pinMode(L298N_IN1, OUTPUT);

    // configure LED PWM functionalitites
    ledcSetup(pwmChannel, freq, resolution);
    // attach the channel to the GPIO to be controlled
    ledcAttachPin(L298N_EN, pwmChannel);
    // PWM duty as 50%
    ledcWrite(pwmChannel, 127);

    pinMode(REDALERT, OUTPUT);
    digitalWrite(REDALERT, LOW);

    //
    gpio_hold_dis((gpio_num_t)POWER_12V);
    pinMode(POWER_12V, OUTPUT);
    digitalWrite(POWER_12V, LOW);
}

uint16_t realldr = 0x0;

uint8_t motor_direction = STOPPED;
bool closed_prox = false;
bool opened_prox = false;
bool alert_led = false;

#define PROX_SENSOR_DEBOUNCETIME 2500

TON closed_prox_on_debounce(PROX_SENSOR_DEBOUNCETIME);
Rtrg closed_prox_on_trg;
TON closed_prox_off_debounce(PROX_SENSOR_DEBOUNCETIME);
Rtrg closed_prox_off_trg;

TON opened_prox_on_debounce(PROX_SENSOR_DEBOUNCETIME);
Rtrg opened_prox_on_trg;
TON opened_prox_off_debounce(PROX_SENSOR_DEBOUNCETIME);
Rtrg opened_prox_off_trg;

void check_prox()
{
    closed_prox_on_debounce.IN = digitalRead(CLOSEDPROX) == LOW;
    closed_prox_on_debounce.update();
    closed_prox_on_trg.IN = closed_prox_on_debounce.Q;
    closed_prox_on_trg.update();

    closed_prox_off_debounce.IN = digitalRead(CLOSEDPROX) == HIGH;
    closed_prox_off_debounce.update();
    closed_prox_off_trg.IN = closed_prox_off_debounce.Q;
    closed_prox_off_trg.update();

    if (closed_prox_on_trg.Q)
        closed_prox = HIGH;
    if (closed_prox_off_trg.Q)
        closed_prox = LOW;

    opened_prox_on_debounce.IN = digitalRead(OPENEDPROX) == LOW;
    opened_prox_on_debounce.update();
    opened_prox_on_trg.IN = opened_prox_on_debounce.Q;
    opened_prox_on_trg.update();

    opened_prox_off_debounce.IN = digitalRead(OPENEDPROX) == HIGH;
    opened_prox_off_debounce.update();
    opened_prox_off_trg.IN = opened_prox_off_debounce.Q;
    opened_prox_off_trg.update();

    if (opened_prox_on_trg.Q)
        opened_prox = HIGH;
    if (opened_prox_off_trg.Q)
        opened_prox = LOW;
}

//
void stop_motor()
{
    digitalWrite(L298N_IN1, LOW);
    digitalWrite(L298N_IN2, LOW);
}

//
void forward_motor()
{
    digitalWrite(L298N_IN1, LOW);
    digitalWrite(L298N_IN2, HIGH);
}

void backward_motor()
{
    digitalWrite(L298N_IN1, HIGH);
    digitalWrite(L298N_IN2, LOW);
}
