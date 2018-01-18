#pragma once
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <WiFiManager.h>  // https://github.com/tzapu/WiFiManager
#include <Fatal.h>        // https://github.com/cvonk/ESP8266_Fatal
#include <Ticker.h>

namespace Framework
{
  extern ESP8266WebServer server;   // declared, not defined
  uint8_t const begin(void);
  uint8_t const handle(void);  
};

