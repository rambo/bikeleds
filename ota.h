#ifndef OTA_H
#define OTA_H
#include "settings.h"

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

const char* otassid = OTA_WIFI_NAME;
const char* otapassword = OTA_WIFI_PASSWD;
IPAddress ip(10, 46, 75, 10);
IPAddress gateway(10, 46, 75, 1);
IPAddress subnet(255, 255, 255, 0);

AsyncWebServer otawebserver(OTA_WEB_PORT);

void init_ota(void) {
  Serial.println(F("Starting OTA"));
  GLOBAL_system_state = STATE_OTA;
  timerWrite(GLOBAL_wdtimer, 0); //reset timer (feed watchdog)
  timerAlarmDisable(GLOBAL_wdtimer); //disableinterrupt

  // Start softap and web server
  WiFi.softAPConfig(ip, gateway, subnet);
  WiFi.softAP(otassid, otapassword);

  otawebserver.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hi! I am ESP32.");
  });

  AsyncElegantOTA.begin(&otawebserver);
  otawebserver.begin();
  Serial.println(F("HTTP server started"));
}
#endif
