#include <time.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <ArduinoOTA.h>

#include "ntp.h"
#include "dst.h"
#include "display.h"

WiFiUDP UDP;                     // Create an instance of the WiFiUDP class to send and receive

const byte led = 13;

void startWiFi() { // Try to connect to some given access points. Then wait for a connection
  WiFiManager wifiManager;

  // Uncomment and run it once, if you want to erase all the stored information
  //wifiManager.resetSettings();

  wifiManager.autoConnect();

  Serial.println("WiFi connected");
}

void startUDP() {
  Serial.println("Starting UDP");
  UDP.begin(UDP_PORT);                          // Start listening for UDP messages
  Serial.print("Local port:\t");
  Serial.println(UDP.localPort());
  Serial.println();
}

void startOTA() {
  ArduinoOTA.setHostname("Clock-ESP8266");
  ArduinoOTA.setPassword("esp8266");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  ArduinoOTA.begin();

  Serial.println("OTA ready");
}

void setup() {
  Serial.begin(115200);         // Start the Serial communication to send messages to the computer
  delay(10);

  startWiFi();
  startUDP();
  startOTA();
  display_init();

  pinMode(led, OUTPUT);
  digitalWrite(led, 1);
}

void displayProgress() {
  spin();
}

void dumpTime(struct tm *tm)
{

  Serial.print(tm->tm_hour);
  Serial.print(":");
  Serial.print(tm->tm_min);
  Serial.print(":");
  Serial.println(tm->tm_sec);
}

void updateTime(unsigned long time) {
  struct tm *tm = gmtime((time_t*)&time);
  int y = tm->tm_year + 1900;
  int m = tm->tm_mon + 1;
  int d = tm->tm_mday;
  unsigned long tz = adjustDstEurope(y, m, d);

  unsigned long lt = time + tz;
  tm = gmtime((time_t*)&lt);

  render_time(tm->tm_hour, tm->tm_min, tm->tm_sec);
  display_time(tm->tm_hour);
  dumpTime(tm);
}

unsigned long previousTime = millis();

const unsigned long intervalNTPTimeout = 10000;
const unsigned long intervalNTPUpdate = 60000; // Request NTP time every minute
unsigned long prevNTP = 0;
unsigned long lastNTPResponse = 0;
unsigned long lastNTPSend = 0;

enum NTPState {
  not_started,
  started
};

NTPState ntp_state = not_started;
uint32_t timeUNIX = 0;

void loop() {
  ArduinoOTA.handle();

  unsigned long currentMillis = millis();

  // start time update
  if (ntp_state == not_started && currentMillis - lastNTPResponse > intervalNTPUpdate ) {
    if (sendNTPpacket(UDP)) {
      ntp_state = started;
      lastNTPSend = currentMillis;
    }
  }

  // wait for response
  if (ntp_state == started) {
    timeUNIX = getTime(UDP);
    if (timeUNIX) {
      ntp_state = not_started;
      lastNTPResponse = currentMillis;
    }
    else if (currentMillis - lastNTPSend > intervalNTPTimeout) {
      ntp_state = not_started;
    }
  }

  if (currentMillis % 1000 == 0) {
    if (timeUNIX == 0) {
      displayProgress();
    }
    else {
      unsigned long dt = (lastNTPResponse - currentMillis) / 1000;
      unsigned long tm = timeUNIX + dt;
      updateTime(tm);
    }
  }
}

