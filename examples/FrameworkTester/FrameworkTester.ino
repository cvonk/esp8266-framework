/*
 * Example that uses the ESP8266 framework to provide
 *   - Online WiFi SSID/passwd configuration
 *   - Saves fatal exception details to non-volatile memory
 *   - Over-the-air software updates
 * The example forces the ESP to crash so that the stack trace can be examined
 *   http://espipnumber/crash to force a crash
 *   http://espipnumber/fatal to dump and clear the stack trace from earlier crashes
 *
 * Platform: ESP8266 using Arduino IDE, min 1 MByte of flash for OTA updates
 * Documentation: https://github.com/cvonk/ESP8266_Framework
 * Tested with: Arduino IDE 1.8.5, board package esp8266 2.4.0, Adafruit huzzah (feather) esp8266
 *
 * ## First Boot (Initial setup)
 * The ESP8266 board will boot into the WiFiManager's AP mode.
 * i.e. It will create a WiFi Access Point with a SSID like: "ESP123456" etc.
 * Connect to that SSID. Then point your browser to http://192.168.4.1/ and
 * configure the ESP8266 to connect to your desired WiFi network.
 * It will remember the new WiFi connection details on next boot.
 * More information can be found here:
 *   https://github.com/tzapu/WiFiManager#how-it-works
 *
 * GNU GENERAL PUBLIC LICENSE Version 3, check the file LICENSE.md for more information
 * (c) Copyright 2018, Coert Vonk
 * All text above must be included in any redistribution
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <WiFiManager.h>  // https://github.com/tzapu/WiFiManager
#include <Fatal.h>        // https://github.com/cvonk/ESP8266_Fatal
#include <Ticker.h>
#include "framework.h"

void tripReset(void) {
	ESP.reset();  // nothing will be saved in EEPROM
}
void tripRestart(void) {
	ESP.restart();  // nothing will be saved in EEPROM
}
void tripHardwareWdt(void) {
	ESP.wdtDisable();
	while (true) {
		; // block forever
	} // nothing will be saved in EEPROM
}
void tripSoftwareWdt(void) {
	while (true) {
		; // block forever
	}
}
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdiv-by-zero"
#pragma GCC diagnostic ignored "-Wunused-variable"
void tripDivBy0(void) {
	int zero = 0;
	int volatile result = 1 / 0;
}
void tripReadNull(void) {
	int volatile result = *(int *)NULL;
}
#pragma GCC diagnostic pop

void tripWriteNull(void) {
	*(int *)NULL = 0;
}

typedef void fnc_t(void);

typedef struct {
	char const * const name;
	char const * const description;
	fnc_t * const      fnc;
} crashcause_t;

crashcause_t crashcauses[] = {
	{ "reset",       "reset module",                    tripReset },       // no Fatal log
	{ "restart",     "restart module",                  tripRestart },     // no Fatal log
	{ "hardwarewdt", "hardware WDT exception",          tripHardwareWdt }, // no Fatal log
	{ "softwarewdt", "software WDT exception (exc.4)",  tripSoftwareWdt },
	{ "divby0",      "divide by zero (exc.0)",          tripDivBy0 },
	{ "readnull",    "read from null pointer (exc.28)", tripReadNull },
	{ "writenull",   "write to null pointer (exc.29)",  tripWriteNull },
	{ NULL,          NULL,                              NULL }
};

void handleRoot(void)
{
	String message = "OK\n\n";
	message += "URI: "; message += Framework::server.uri();
	message += "\nMethod: "; message += (Framework::server.method() == HTTP_GET) ? "GET" : "POST";
	message += "\nArguments: "; message += Framework::server.args();
	message += "\n";
	for ( uint8_t i = 0; i < Framework::server.args(); i++ ) {
		message += " " + Framework::server.argName( i ) + ": " + Framework::server.arg( i ) + "\n";
	}
	message += "\n\nUse /crash to cause a crash";
	Framework::server.send( 404, "text/plain", message );
}

struct {
	crashcause_t const * cause;
	unsigned long        time;  // [msec]
} scheduled = { NULL, 0 };

void handleCrash(void)
{
	if (Framework::server.args() >= 1 && Framework::server.argName(0) == "req") {
		for (crashcause_t const *p = crashcauses; p->name; p++) {
			if (strcmp(p->name, Framework::server.arg(0).c_str()) == 0) {
				scheduled.cause = p;
				scheduled.time = millis() + 5000;  // time to send the replay
				String message("Performing \"");
				message = message + p->description + "\"<br /><a href=\"/fatal\">Fatal history</a> will be available after the ESP reboots.";

				Framework::server.send(200, "text/html", message);
				return;
			}
		}
	}
	String message("Crash to perform (and log):\n<ul>");
	for (crashcause_t const *p=crashcauses; p->name; p++) {
		message = message + "<li><a href=\"/crash?req=" + p->name + "\">" + p->description + "</a></li>\n";
	}
	message += "</ul><br />"
		       "Inspect and clear the <a href=\"/fatal\">Fatal log</a>.<br />"
		       "Remember the ESP reboots from the same source it started from. "
		       "If you uploaded code through the UART, it will try to boot from the UART.";
	Framework::server.send(200, "text/html", message);
}

void setup() {
  Framework::begin();
  Framework::server.on("/", handleRoot);
  Framework::server.on("/crash", handleCrash);
  Framework::server.begin();
  Serial.println("Ready");
}

void loop() {
  Framework::handle();

  if ((scheduled.cause != NULL) && (millis() > scheduled.time)) {
	  (scheduled.cause->fnc)();
  }
}
