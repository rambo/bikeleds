#include <elapsedMillis.h>
#include <FastLED.h>

#define LOW_DP 4
#define LOW_NL 31
#define HIGH_DP 2
#define HIGH_NL 58


CRGBArray<LOW_NL> low;
CRGBArray<HIGH_NL> high;

#include "esp_system.h"
const int wdtTimeout = 1000;  //time in ms to trigger the watchdog
hw_timer_t *timer = NULL;

void IRAM_ATTR resetModule() {
  ets_printf("reboot\n");
  esp_restart();
}


void setup() {
  Serial.begin(115200);
  FastLED.addLeds<WS2812B,LOW_DP,GRB>(low,LOW_NL);
  FastLED.addLeds<WS2812B,HIGH_DP,GRB>(high,HIGH_NL);
  FastLED.setBrightness(64);


  timer = timerBegin(0, 80, true);                  //timer 0, div 80
  timerAttachInterrupt(timer, &resetModule, true);  //attach callback
  timerAlarmWrite(timer, wdtTimeout * 1000, false); //set time in us
  timerAlarmEnable(timer);                          //enable interrupt

  Serial.println(F("Booted"));
}


int8_t high_dir = 1;
int8_t low_dir = 1;
int8_t high_ledno = 0;
int8_t low_ledno = 0;

void fadeall() { 
  for(int i = 0; i < LOW_NL; i++) { low[i].nscale8(250); }
  for(int i = 0; i < HIGH_NL; i++) { high[i].nscale8(250); }
}



void loop() {
  timerWrite(timer, 0); //reset timer (feed watchdog)
  FastLED.show();
  fadeall();
  high[high_ledno] = CHSV(177, 64, 255);
  low[low_ledno] = CHSV(177, 64, 255);
  low_ledno = low_ledno+low_dir;
  if (low_ledno < 0) {
    low_dir = 1;
    Serial.println(F("low_dir changed to 1"));
  }
  if (low_ledno >= (LOW_NL -1 )) {
    low_dir = -1;
    Serial.println(F("low_dir changed to -1"));
  }
  high_ledno = high_ledno+high_dir;
  if (high_ledno < 0) {
    high_dir = 1;
    Serial.println(F("high_dir changed to 1"));
  }
  if (high_ledno >= (HIGH_NL -1 )) {
    high_dir = -1;
    Serial.println(F("high_dir changed to -1"));
  }
  delay(10);

}
