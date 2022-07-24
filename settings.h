#ifndef SETTINGS_H
#define SETTINGS_H

#define SPP_DEVICE_NAME "DucatiLEDs"
#define OTA_WIFI_NAME "DucatiLEDs-OTA"
#define OTA_WIFI_PASSWD "Do Not Brick"
// Device IP is "10.46.75.10", "gw" is .1 and netmask is /24 (255.255.255.0)
#define OTA_WEB_PORT 80
#define DEFAULT_BRIGHTNESS 64
#define LOW_DP 4
#define LOW_NL 31
#define HIGH_DP 2
#define HIGH_NL 58
#define INTERLOCK_PIN 33

#define IDX_CMD 0x1
#define IDX_LEDS_LOW 0x2
#define IDX_LEDS_HIGH 0x3
#define IDX_PATTERNARGS 0x4

#define CONNECTED_MAX_IDLE 60*5*1000 // 5 minutes of idle will sleep even if connected
#define CONNECTED_ACTIVE_TIMEOUT 2*1000
#define DISCONNECTED_MAX_IDLE 10*1000
#define IDLE_SLEEP_TIME 5*1000
//#define PATTERN_MAX_IDLE 60*30*1000 // 30 minutes of idle will sleep even if on pattern
#define PATTERN_MAX_IDLE 60*120*1000 // 120 minutes of idle will sleep even if on pattern
#define OTA_MAX_IDLE 60*10*1000 // 10 minutes of idle will sleep in OTA

#endif
