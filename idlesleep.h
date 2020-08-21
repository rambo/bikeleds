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
    if (GLOBAL_BT_connected)
    {
        Serial.println(F("Disconnecting BT"));
        SerialBT.disconnect();
    }

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


void IdleChecker::run(uint32_t now)
{
    timerWrite(GLOBAL_wdtimer, 0); //reset timer (feed watchdog)
    // Remember to set the next runtime
    incRunTime(_rate);
    /*
    Serial.print(F("now="));
    Serial.print(now, DEC);
    Serial.println(F(": Checking idles"));
    Serial.print(F("state="));
    Serial.println(GLOBAL_system_state, DEC);
    Serial.print(F("last_active_command="));
    Serial.println(GLOBAL_last_active_command, DEC);
    Serial.print(F("idle_timer="));
    Serial.println(GLOBAL_idle_timer, DEC);
    */

    if (GLOBAL_system_state == STATE_BOOT)
    {
        Serial.println(F("In boot state, should be at least idle by now!!!"));
        GLOBAL_system_state = STATE_IDLE;
    }

    if (GLOBAL_system_state >= STATE_PATTERN)
    {
        // If patterns are running, disable all idle checking
        GLOBAL_idle_timer = 0;
        GLOBAL_last_active_command = 0;
        return;
    }
    // If we got new connection, set state and reset the idle timer
    if (GLOBAL_BT_connected && GLOBAL_system_state == STATE_IDLE)
    {
        Serial.println(F("Got BT connection"));
        GLOBAL_system_state = STATE_CONNECTED_IDLE;
        GLOBAL_idle_timer = 0;
    }
    // Go idle if connection is lost (and we're not in a pattern)
    if (GLOBAL_system_state != STATE_IDLE && !GLOBAL_BT_connected)
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
            Serial.println(F("Connection idling for too long, going to bed"));
            enter_sleep();
        }
    }
    if (GLOBAL_system_state == STATE_IDLE)
    {
        if (GLOBAL_idle_timer > DISCONNECTED_MAX_IDLE)
        {
            Serial.println(F("Nothing to do, going to bed"));
            enter_sleep();
        }
    }

}

#endif
