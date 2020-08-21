#ifndef IDLESLEEP_H
#define IDLESLEEP_H

#include <Arduino.h>
#include <Task.h>
#include "esp_system.h"

// Update the LEDs
class IdleChecker : public TimedTask
{
public:
    IdleChecker(uint32_t _rate);
    virtual void run(uint32_t runtime);
    void enter_sleep();
private:
    uint32_t _rate;    // Interval 
    bool _stop;
};

IdleChecker::IdleChecker(uint32_t _rate)
: TimedTask(millis()),
  _rate(_rate)
{
}

void IdleChecker::enter_sleep()
{
    Serial.println(F("Going to sleep"));
    FastLED.clear(true);
    FastLED.show();
    Serial.println(F("LEDs off"));

    esp_sleep_enable_timer_wakeup(IDLE_SLEEP_TIME * 1000); // The sleep time is usec
    Serial.print(F("Waking up in "));
    Serial.print(IDLE_SLEEP_TIME, DEC);
    Serial.println(F("ms"));

    timerAlarmDisable(GLOBAL_wdtimer);  // Disable watchdog
    Serial.println(F("WDT off"));

    Serial.println(F("Bye..."));
    esp_deep_sleep_start();
}


void IdleChecker::run(uint32_t runtime)
{
    if (GLOBAL_system_state >= STATE_PATTERN)
    {
        // If patterns are running, disable all idle checking
        GLOBAL_idle_timer = 0;
        GLOBAL_last_active_command = 0;
        return;
    }
    // If we got new connection, set state and reset the idle timer
    if (SerialBT.connected() && GLOBAL_system_state == STATE_IDLE)
    {
        GLOBAL_system_state = STATE_CONNECTED_IDLE;
        GLOBAL_idle_timer = 0;
    }
    // Go idle if connection is lost (and we're not in a pattern)
    if (GLOBAL_system_state != STATE_IDLE && !SerialBT.connected())
    {
        Serial.println(F("BT Disconnected, goind idle"));
        GLOBAL_idle_timer = 0;
        GLOBAL_system_state = STATE_IDLE;
    }
    // Check connection idle
    if (GLOBAL_system_state == STATE_CONNECTED_ACTIVE)
    {
        if (GLOBAL_last_active_command > CONNECTED_ACTIVE_TIMEOUT)
        {
            Serial.println(F("Marking BT connection idle"));
            GLOBAL_idle_timer = 0;
            GLOBAL_system_state = STATE_CONNECTED_IDLE;
        }
    }
    if (GLOBAL_system_state == STATE_CONNECTED_IDLE)
    {
        if (GLOBAL_idle_timer > CONNECTED_MAX_IDLE)
        {
            enter_sleep();
        }
    }
    if (GLOBAL_system_state == STATE_IDLE)
    {
        if (GLOBAL_idle_timer > DISCONNECTED_MAX_IDLE)
        {
            enter_sleep();
        }
    }

}

#endif
