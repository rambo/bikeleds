#ifndef PATTERNS_H
#define PATTERNS_H

#include <Arduino.h>
#include <Task.h>

typedef enum {
    PATTERN_NONE = 0,
    PATTERN_SINELON,
    PATTERN_MAX
} pattern_t;



// Make patterns
class PatternMaker : public TimedTask
{
public:
    PatternMaker(uint32_t _rate);
    virtual void run(uint32_t runtime);
    MsgPack::arr_t<int> pattern_args;
    void start_pattern(pattern_t pnum);
    void stop_pattern();
private:
    pattern_t active_pattern;
    MsgPack::arr_t<int> internal_pattern_args;
    uint32_t _rate;    // Interval 
    bool _stop;
    void init_sinelon();
    void run_sinelon();
};

PatternMaker::PatternMaker(uint32_t _rate)
: TimedTask(millis()),
  _rate(_rate)
{
}

void PatternMaker::init_sinelon()
{
    if (!pattern_args.empty() && pattern_args.size() < 3)
    {
        Serial.println(F("init_sinelon: invalid pattern args"));
        pattern_args.clear();
    }
    if (pattern_args.empty())
    {
        // Dot HSV
        pattern_args.push_back(204);
        pattern_args.push_back(153);
        pattern_args.push_back(255);
    }
    if (pattern_args.size() < 4)
    {
        // Sine BPM
        pattern_args.push_back(60);
    }
    if (pattern_args.size() < 5)
    {
        // Fade speed
        pattern_args.push_back(20);
    }
}

void PatternMaker::run_sinelon()
{
  if (pattern_args.size() != 5)
  {
        Serial.println(F("Invalid pattern args (should not hit this unless someone updated them mid-pattern)"));
        pattern_args.clear();
        init_sinelon();
  }
  // a colored dot sweeping back and forth, with fading trails
  high.fadeToBlackBy(pattern_args[4]);
  int pos = beatsin16( pattern_args[3], 0, HIGH_NL-1 );
  high[pos] += CHSV( pattern_args[0], pattern_args[1], pattern_args[2]);
}

void PatternMaker::start_pattern(pattern_t pnum)
{
    if (pnum <= PATTERN_NONE || pnum >= PATTERN_MAX)
    {
        Serial.println(F("Invalid pattern value"));
        return;
    }
    internal_pattern_args.clear();
    active_pattern = pnum;
    // Insert pattern initializations here
    switch (active_pattern)
    {
        case PATTERN_NONE:
        case PATTERN_MAX:
          return;
        case PATTERN_SINELON:
          init_sinelon();
          break;
    }
    GLOBAL_pattern_idle_timer = 0;
    GLOBAL_system_state = STATE_PATTERN;
}

void PatternMaker::stop_pattern()
{
    active_pattern = PATTERN_NONE;
    // Clear args so that they must be explicitly set for next pattern
    pattern_args.clear();
    // Go idle
    FastLED.clear(true);
    FastLED.show();
    GLOBAL_system_state = STATE_IDLE;
    GLOBAL_idle_timer = 0;
    GLOBAL_last_active_command = 0;
}


void PatternMaker::run(uint32_t now)
{
    if (GLOBAL_pattern_idle_timer > PATTERN_MAX_IDLE)
    {
        Serial.println(F("Idling on pattern for too long, stopping it"));
        stop_pattern();
    }
    timerWrite(GLOBAL_wdtimer, 0); //reset timer (feed watchdog)
    // Remember to set the next runtime
    incRunTime(_rate);
    switch (active_pattern)
    {
        case PATTERN_NONE:
        case PATTERN_MAX:
          return;
        case PATTERN_SINELON:
          run_sinelon();
          break;
    }
}

#endif
