/*
 * ESP8266 framework that provides
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

using namespace Framework;
ESP8266WebServer Framework::server(80);  // declared

void tick() {
  int state = digitalRead(BUILTIN_LED);  // get the current state of GPIO1 pin
  digitalWrite(BUILTIN_LED, !state);     // set pin to the opposite state
}
Ticker ticker;

// called when WiFiManager enters configuration mode
void 
configModeCallback (WiFiManager *myWiFiManager)
{
  Serial.println("Config SSID="); Serial.println(myWiFiManager->getConfigPortalSSID());
  ticker.attach(0.2, tick);
}

static void 
handleRoot(void)
{
  Serial.println("handleRoot");
    int const sec = millis() / 1000;
    int const min = sec / 60;
    int const hr = min / 60;

    server.send( 200, "text/html",
    F("<html>\n"
        "<head>\n"
        "    <meta name='viewport' content='width=device-width, initial-scale=1, minimum-scale=1.0 maximum-scale=1.0' />\n"
        "</head>\n"
        "<body>\n"
        "  <style>\n"
        "    body {width:100%;height:100%;margin:0;overflow:hidden;background-color:#252525;}\n"
        "    #iframe {position:absolute;left:0px;top:0px;}\n"
        "  </style>\n"
        "  <h1>Page loading</h1>\n"
        "  <iframe id='iframe' name='iframe1' frameborder='0' width='100%' height='100%' src='https://coertvonk.com/'></iframe>\n"
        "</body>\n"
        "</html>") );
}

static void 
handleFatal(void)
{
  Serial.println("handleFatal");
  // send response header to the web browser
  WiFiClient client = server.client();
  client.print(F("HTTP/1.1 200 OK\r\n"
           "Content-Type: text/plain\r\n"
           "Connection: close\r\n"
           "\r\n"));
  Fatal::print(client);  // send crash information to the web browser
  Fatal::clear();
}

static void 
handleJson(void)
{
  Serial.println("handleJson");
    for ( uint_least8_t ii = 0; ii < server.args(); ii++ ) {
        Serial.printf( "%s=%s ", server.argName( ii ).c_str(), server.arg( ii ).c_str() );
    }  
    server.send(200, "text/plain", "OK");
    //delay(1); // give the web browser time to receive the data
    //server.stop();  // close the connection:
    Serial.println("[Client disconnected]");
}

static void 
handleNotFound(void)
{
  Serial.println("handleNotFound");
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";

    for ( uint8_t i = 0; i < server.args(); i++ ) {
        message += " " + server.argName( i ) + ": " + server.arg( i ) + "\n";
    }
    server.send( 404, "text/plain", message );
}

uint8_t const
Framework::begin() 
{
  Serial.begin(115200);
  Serial.println("\nBooting");
  pinMode(BUILTIN_LED, OUTPUT);
  ticker.attach(0.6, tick);

  // WiFiManager
  { 
    WiFiManager wifiManager;
    //wifiManager.resetSettings();  // for testing
    wifiManager.setAPCallback(configModeCallback);  // called when previous WiFi fails, when entering AP mode
    // fetches ssid and pass and tries to connect
    // on failure, starts an access point and wait for further instructions (wifi SSID/passwd)
    if (!wifiManager.autoConnect()) {
      Serial.println("timeout, no connection");    
      ESP.reset();  //reset and try again, or maybe put it to deep sleep
      delay(1000);
    }
    Serial.print("IP address: "); Serial.println(WiFi.localIP());  // we're connected
    //ticker.detach();
    //digitalWrite(BUILTIN_LED, HIGH);
    ticker.attach(2, tick);
  }
  
  // Be receiptive to over-the-air (OTA) updates
  {  
    ArduinoOTA.onStart([]() {
      Serial.printf("\nOTA start %s", (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem");
    });
    ArduinoOTA.onEnd([]() {
      Serial.printf("\nOTA end\n");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("OTA progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("OTA error %u\n", error);
    });
    ArduinoOTA.begin();
  }

   // Fatal
  {  
    Fatal::begin(0x0010, 0x0200);
    server.on("/fatal", handleFatal);
    server.onNotFound(handleNotFound);

    WiFi.persistent(false);
  }
  return 0;
}


uint8_t const
Framework::handle() {
  ArduinoOTA.handle();
  server.handleClient();  
  return 0;
}

