#ifndef PATTERNS_H
#define PATTERNS_H

#include <Arduino.h>
#include <Task.h>

#define LOW_MIDX (LOW_NL-1)
#define HIGH_MIDX (HIGH_NL-1)

typedef enum {
    PATTERN_NONE = 0,
    PATTERN_SINELON,
    PATTERN_BREATHE,
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
    void init_breathe();
    void run_breathe();
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

void PatternMaker::init_breathe()
{
    if (!pattern_args.empty() && pattern_args.size() < 3)
    {
        Serial.println(F("init_breathe: invalid pattern args"));
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
        // breathe BPM
        pattern_args.push_back(5);
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
        Serial.println(F("run_sinelon: Invalid pattern args (should not hit this unless someone updated them mid-pattern)"));
        pattern_args.clear();
        init_sinelon();
  }
  // a colored dot sweeping back and forth, with fading trails
  high.fadeToBlackBy(pattern_args[4]);
 #if HIGH_NL < 254
  int pos = beatsin8( pattern_args[3], 0, HIGH_MIDX );
 #else
  int pos = beatsin16( pattern_args[3], 0, HIGH_MIDX );
 #endif
  high[pos] += CHSV( pattern_args[0], pattern_args[1], pattern_args[2]);
}

void PatternMaker::run_breathe()
{
  if (pattern_args.size() != 5)
  {
        Serial.println(F("run_breathe: Invalid pattern args (should not hit this unless someone updated them mid-pattern)"));
        pattern_args.clear();
        init_breathe();
  }
  // a colored dot sweeping back and forth, with fading trails
  low.fadeToBlackBy(pattern_args[4]);
 #if LOW_NL < 254
  int width = beatsin8( pattern_args[3], 0, LOW_MIDX/2 );
 #else
  int width = beatsin16( pattern_args[3], 0, LOW_MIDX/2 );
 #endif
  int left = LOW_MIDX/2 - width;
  if (left < 0) { 
    Serial.println(F("run_breathe: left too low"));
    left = 0;
  };
  int right = LOW_MIDX/2 + width;
  if (right > LOW_MIDX) {
    Serial.println(F("run_breathe: right too high"));
    right=LOW_MIDX;
  }
  low(left,right) = CHSV( pattern_args[0], pattern_args[1], pattern_args[2]);
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
        case PATTERN_BREATHE:
          init_breathe();
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
        case PATTERN_BREATHE:
          run_breathe();
          break;
    }
}

#endif
