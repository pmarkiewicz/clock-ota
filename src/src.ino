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
    display_msg("OTA");
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
    display_msg("END");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
      display_msg("Auth");
    }
    else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
      display_msg("E be");
    }
    else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
      display_msg("E co");
    }
    else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
      display_msg("E re");
    }
    else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
      display_msg("E en");
    }
  });

  ArduinoOTA.begin();

  Serial.println("OTA ready");
}

void setup() {
  Serial.begin(115200);         // Start the Serial communication to send messages to the computer
  delay(10);
  display_init();

  startWiFi();
  startUDP();
  startOTA();

  //pinMode(led, OUTPUT);
  //digitalWrite(led, 1);
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

const unsigned long intervalNTPTimeout = 30000;
const unsigned long intervalNTPUpdate = 60000 * 60 * 12; // 12 hrs 
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
  uint32_t dt = currentMillis - lastNTPResponse;
  if (ntp_state == not_started && (timeUNIX == 0 || dt > intervalNTPUpdate) ) {
    Serial.println("Sending NTP");
    if (sendNTPpacket(UDP)) {
      Serial.println("NTP sent");
      ntp_state = started;
      lastNTPSend = currentMillis;
    }
  }

  // wait for response
  if (ntp_state == started) {
    //Serial.println("NTP await");
    uint32_t ntpTime = getTime(UDP);
    if (ntpTime) {
      Serial.print("NTP recived: ");
      Serial.println(timeUNIX);
      ntp_state = not_started;
      lastNTPResponse = currentMillis;
      timeUNIX = ntpTime;
      Serial.println("received");
      updateTime(timeUNIX);
    }
    else if (currentMillis - lastNTPSend > intervalNTPTimeout) {
      Serial.println("NTP not recived");
      ntp_state = not_started;
    }
  }

  if (timeUNIX == 0 && currentMillis % 1000 == 0) {
    displayProgress();
  }
  if (timeUNIX != 0 && currentMillis % 10000 == 0) {
    uint32_t dt = (currentMillis - lastNTPResponse) / 1000;
    uint32_t tm = timeUNIX + dt;
    updateTime(tm);
  }
}

