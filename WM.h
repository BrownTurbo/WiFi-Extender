#ifndef WM_H
#define WM_H

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <LittleFS.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include "config.h"
#include "utils.h"

#include <Arduino.h>

class WM {
  public:

//  JsonObject obj = Config.as<JsonObject>();
  void listDir(const char * dirname);
  void begin_server();
  void create_server();
  String get_credentials(int a);

};

String encType(int id);
int dBmtoPercentage(int dBm);

#endif
