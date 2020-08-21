#ifndef LEDUPDATE_H
#define LEDUPDATE_H

#include <Arduino.h>
#include <Task.h>

// Update the LEDs
class LEDUpdater : public TimedTask
{
public:
    LEDUpdater(uint32_t _rate);
    virtual void run(uint32_t runtime);
private:
    uint32_t _rate;    // Interval 
    bool _stop;
};

LEDUpdater::LEDUpdater(uint32_t _rate)
: TimedTask(millis()),
  _rate(_rate)
{
}

void LEDUpdater::run(uint32_t now)
{
    timerWrite(GLOBAL_wdtimer, 0); //reset timer (feed watchdog)
    // Remember to set the next runtime
    incRunTime(_rate);
    /*
    Serial.print(F("now="));
    Serial.print(now, DEC);
    Serial.println(F(": Updating LEDs"));
    */
    show_check_interlock();
}

#endif
