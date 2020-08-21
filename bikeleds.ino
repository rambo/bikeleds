#include "settings.h"
// Get this library from http://bleaklow.com/files/2010/Task.tar.gz (and fix WProgram.h -> Arduino.h)
// and read http://bleaklow.com/2010/07/20/a_very_simple_arduino_task_manager.html for background and instructions
#include <Task.h>
#include <TaskScheduler.h>

#include <elapsedMillis.h>
#include <Bounce2.h>
#include <FastLED.h>
#include <MsgPacketizer.h>
#include "BluetoothSerial.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_bt_device.h"
#include "esp_spp_api.h"

CRGBArray<LOW_NL> low;
CRGBArray<HIGH_NL> high;
BluetoothSerial SerialBT;



#include "esp_system.h"
const int wdtTimeout = 1000;  //time in ms to trigger the watchdog
hw_timer_t *GLOBAL_wdtimer = NULL;

void IRAM_ATTR resetModule() {
  ets_printf("\n WDT reboot\n");
  esp_restart();
}


typedef enum {
    STATE_BOOT = 0,
    STATE_IDLE,
    STATE_CONNECTED_IDLE,
    STATE_CONNECTED_ACTIVE,
    STATE_PATTERN,
    STATE_MAX
} state_t;


RTC_DATA_ATTR state_t GLOBAL_system_state = STATE_BOOT;
elapsedMillis GLOBAL_last_active_command;
elapsedMillis GLOBAL_idle_timer;
elapsedMillis GLOBAL_pattern_idle_timer;
Bounce GLOBAL_interlock = Bounce();


bool GLOBAL_BT_connected;
void bt_event_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
    Serial.println(F("BT Activity, reset idle timer"));
    GLOBAL_idle_timer = 0;
    if (event == ESP_SPP_SRV_OPEN_EVT)
    {
        Serial.println(F("ESP_SPP_SRV_OPEN_EVT"));
        GLOBAL_BT_connected = true;
    }
    if (event == ESP_SPP_CLOSE_EVT)
    {
        Serial.println(F("ESP_SPP_CLOSE_EVT"));
        GLOBAL_BT_connected = false;
    }
}


#include "patterns.h"
PatternMaker patterntask(16); // ~60fps update
#include "idlesleep.h"
IdleChecker idletask(5); // Check idle timers every 5 ms
#include "iotask.h"
IOHandler iotask;
#include "ledupdate.h"
LEDUpdater ledtask(16); // ~60fps update

inline void show_check_interlock()
{
    if (GLOBAL_interlock.read())
    {
        Serial.println(F("Interlocked, clearing LEDs"));
        if (GLOBAL_system_state == STATE_PATTERN)
        {
            patterntask.stop_pattern();
        }
        FastLED.clear(true);
    }
    FastLED.show();
}


inline void connection_active_command()
{
    if (GLOBAL_system_state < STATE_CONNECTED_ACTIVE)
    {
        GLOBAL_system_state = STATE_CONNECTED_ACTIVE;
    }
    GLOBAL_last_active_command = 0;
    GLOBAL_idle_timer = 0;
    GLOBAL_pattern_idle_timer = 0;
}

void setup()
{
    GLOBAL_wdtimer = timerBegin(0, 80, true);                  //timer 0, div 80
    timerAttachInterrupt(GLOBAL_wdtimer, &resetModule, true);  //attach callback
    timerAlarmWrite(GLOBAL_wdtimer, wdtTimeout * 1000, false); //set time in us
    timerAlarmEnable(GLOBAL_wdtimer);                          //enable interrupt
    timerWrite(GLOBAL_wdtimer, 0); //reset timer (feed watchdog)

    pinMode(INTERLOCK_PIN, INPUT);
    GLOBAL_interlock.attach(INTERLOCK_PIN);
    GLOBAL_interlock.interval(5); // interval in ms

    Serial.begin(115200);
    SerialBT.begin(SPP_DEVICE_NAME);
    SerialBT.register_callback(&bt_event_cb);
    FastLED.addLeds<WS2812B,LOW_DP,GRB>(low,LOW_NL);
    FastLED.addLeds<WS2812B,HIGH_DP,GRB>(high,HIGH_NL);
    FastLED.setBrightness(DEFAULT_BRIGHTNESS);
    
    MsgPacketizer::subscribe(SerialBT, IDX_CMD,
        [&](const MsgPack::arr_size_t& sz, const String& cmd, const float val)
        {
            connection_active_command();
            Serial.println(F("Got command packet"));
            if (sz.size() == 2) // if array size is correct
            {
                Serial.print(F("Got command: "));
                Serial.print(cmd);
                Serial.print(F(" with value: "));
                Serial.print(val, DEC);
                Serial.println();

                if (cmd.equals("pattern"))
                {
                    if (!val)
                    {
                        patterntask.stop_pattern();
                    }
                    else
                    {
                        patterntask.start_pattern((pattern_t)static_cast<int>(val));
                    }
                    // TODO: send ACK reply on BT
                }
                if (cmd.equals("brightness"))
                {
                    FastLED.setBrightness(val);
                    Serial.println(F("Brightness set"));
                    // TODO: send ACK reply on BT
                }
                if (cmd.equals("off"))
                {
                    FastLED.clear(true);
                    FastLED.show();
                    Serial.println(F("LEDs off"));
                    // TODO: send ACK reply on BT
                    idletask.enter_sleep();
                }
            }
        }
    );
    MsgPacketizer::subscribe(SerialBT, IDX_PATTERNARGS,
        [&](const MsgPack::arr_t<int>& inarr)
        {
            connection_active_command();
            Serial.println(F("Got patternargs packet"));
            Serial.print(F("arr size "));
            Serial.println(inarr.size(), DEC);
            patterntask.pattern_args.clear();
            patterntask.pattern_args.assign(inarr.begin(), inarr.end());
            // TODO: send ACK reply on BT
        }
    );
    MsgPacketizer::subscribe(SerialBT, IDX_LEDS_LOW,
        [&](const MsgPack::arr_t<uint8_t>& inarr)
        {
            connection_active_command();
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
            connection_active_command();
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
    timerWrite(GLOBAL_wdtimer, 0); //reset timer (feed watchdog)
    GLOBAL_system_state = STATE_IDLE;
}



void loop() {
  Task *tasks[] = { 
      &iotask,
      &idletask,
      &patterntask,
      &ledtask
  };
  TaskScheduler sched(tasks, NUM_TASKS(tasks));
  Serial.println(F("Starting task scheduler"));
  sched.run();

}
