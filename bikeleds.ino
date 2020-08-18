#include "settings.h"
#include <elapsedMillis.h>
#include <FastLED.h>
#include <MsgPacketizer.h>
#include "BluetoothSerial.h"


CRGBArray<LOW_NL> low;
CRGBArray<HIGH_NL> high;
BluetoothSerial SerialBT;

#include "esp_system.h"
const int wdtTimeout = 1000;  //time in ms to trigger the watchdog
hw_timer_t *timer = NULL;

void IRAM_ATTR resetModule() {
  ets_printf("reboot\n");
  esp_restart();
}

inline void show_check_interlock()
{
    // TODO: Check the interlock...
    FastLED.show();
}

void setup()
{
    timer = timerBegin(0, 80, true);                  //timer 0, div 80
    timerAttachInterrupt(timer, &resetModule, true);  //attach callback
    timerAlarmWrite(timer, wdtTimeout * 1000, false); //set time in us
    timerAlarmEnable(timer);                          //enable interrupt
    timerWrite(timer, 0); //reset timer (feed watchdog)

    Serial.begin(115200);
    SerialBT.begin(SPP_DEVICE_NAME);
    FastLED.addLeds<WS2812B,LOW_DP,GRB>(low,LOW_NL);
    FastLED.addLeds<WS2812B,HIGH_DP,GRB>(high,HIGH_NL);
    FastLED.setBrightness(DEFAULT_BRIGHTNESS);
    
    MsgPacketizer::subscribe(SerialBT, IDX_CMD,
        [&](const MsgPack::arr_size_t& sz, const String& cmd, const float val)
        {
            Serial.println(F("Got command packet"));
            if (sz.size() == 2) // if array size is correct
            {
                Serial.print(F("Got command: "));
                Serial.print(cmd);
                Serial.print(F(" with value: "));
                Serial.print(val, DEC);
                Serial.println();

                if (cmd.equals("brightness"))
                {
                    FastLED.setBrightness(val);
                    Serial.println(F("Brightness set"));
                    // TODO: send ACK reply on BT
                }
                show_check_interlock();
                
            }
        }
    );
    MsgPacketizer::subscribe(SerialBT, IDX_LEDS_LOW,
        [&](const MsgPack::arr_t<uint8_t>& inarr)
        {
            Serial.println(F("Got low leds packet"));
            Serial.print(F("arr size "));
            Serial.println(inarr.size(), DEC);
            if (inarr.size() != LOW_NL*3)
            {
                // TODO: Send NACK reply on BT
                Serial.println(F("Input array is incorrect size"));
                return;
            }
            // Copy the vector to the RGB array
            memcpy8(low, inarr.data(), inarr.size());
            show_check_interlock();
            // TODO: send ACK reply on BT
        }
    );
    MsgPacketizer::subscribe(SerialBT, IDX_LEDS_HIGH,
        [&](const MsgPack::arr_t<uint8_t>& inarr)
        {
            Serial.println(F("Got high leds packet"));
            Serial.print(F("arr size "));
            Serial.println(inarr.size(), DEC);
            if (inarr.size() != HIGH_NL*3)
            {
                // TODO: Send NACK reply on BT
                Serial.println(F("Input array is incorrect size"));
                return;
            }
            // Copy the vector to the RGB array
            memcpy8(high, inarr.data(), inarr.size());
            show_check_interlock();
            // TODO: send ACK reply on BT
        }
    );
    
    
    Serial.print("My BT name is: ");
    Serial.println(SPP_DEVICE_NAME);
    Serial.println(F("Booted"));
    timerWrite(timer, 0); //reset timer (feed watchdog)
}



void loop() {
  timerWrite(timer, 0); //reset timer (feed watchdog)
  MsgPacketizer::parse();

}
