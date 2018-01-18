/*
 * Example that uses the ESP8266 framework to provide
 *   - Online WiFi SSID/passwd configuration
 *   - Saves fatal exception details to non-volatile memory
 *   - Over-the-air software updates
 *
 * Platform: ESP8266 using Arduino IDE, min 1 MByte of flash for OTA updates
 * Documentation: https://github.com/cvonk/ESP8266_Framework
 * Tested with: Arduino IDE 1.8.5, board package esp8266 2.4.0, Adafruit huzzah (feather) esp8266
 *
 * GNU GENERAL PUBLIC LICENSE Version 3, check the file LICENSE.md for more information
 * (c) Copyright 2018, Coert Vonk
 * All text above must be included in any redistribution
 */

#include "framework.h"

void handleRoot(void)
{
  Serial.println("handleRoot");
    String message = "OK\n\n";
    message += "URI: ";
    message += Framework::server.uri();
    message += "\nMethod: ";
    message += (Framework::server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += Framework::server.args();
    message += "\n";

    for ( uint8_t i = 0; i < Framework::server.args(); i++ ) {
        message += " " + Framework::server.argName( i ) + ": " + Framework::server.arg( i ) + "\n";
    }
    Framework::server.send( 404, "text/plain", message );
}

void handleJson(void)
{
  Serial.println("handleJson");
    for ( uint_least8_t ii = 0; ii < Framework::server.args(); ii++ ) {
        Serial.printf( "%s=%s ", Framework::server.argName( ii ).c_str(), Framework::server.arg( ii ).c_str() );
    }  
    Framework::server.send(200, "text/plain", "OK");
}

static void 
menu(void)
{
  Serial.println(
    "Press a key + <enter>\n"
    "p : print crash information\n"
    "c : clear crash information\n"
    "r : reset module\n"
    "t : restart module\n"
    "s : cause software WDT exception\n"
    "h : cause hardware WDT exception\n"
    "0 : cause divide by 0 exception\n"
    "n : cause null pointer read exception\n"
    "w : cause null pointer write exception\n");
}

void setup() 
{
  Framework::begin();
  Framework::server.on("/", handleRoot);
  Framework::server.on("/json", handleJson);
  Framework::server.begin();
  Serial.println("Ready");

  menu();
}

void loop() {
  Framework::handle();
  
  // Fatal test: read the keyboard

  if (Serial.available()) {
    switch (Serial.read()) {
      case 'p':
        Fatal::print();
        break;
      case 'c':
        Fatal::clear();
        Serial.println("All clear");
        break;
      case 'r':
        Serial.println("Reset");
        ESP.reset();
        // nothing will be saved in EEPROM
        break;
      case 't':
        Serial.println("Restart");
        ESP.restart();
        // nothing will be saved in EEPROM
        break;
      case 'h':
        Serial.printf("Hardware WDT");
        ESP.wdtDisable();
        while (true) {
          ; // block forever
        }
        // nothing will be saved in EEPROM
        break;
      case 's':
        Serial.printf("Software WDT (exc.4)");
        while (true) {
          ; // block forever
        }
        break;
      case '0':
        Serial.println("Divide by zero (exc.0)");
        {
          int zero = 0;
          volatile int result = 1 / 0;
        }
        break;
      case 'e':
        Serial.println("Read from NULL ptr (exc.28)");
        {
          volatile int result = *(int *)NULL;
        }
        break;
      case 'w':
        Serial.println("Write to NULL ptr (exc.29)");
        *(int *)NULL = 0;
        break;
      case '\n':
      case '\r':
        break;
      default:
        menu();
        break;
    }
  }
}

