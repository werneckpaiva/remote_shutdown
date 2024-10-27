#ifndef local_wifi_h
#define local_wifi_h

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

#include "wifi_config.h"
#include "blink.h"

wl_status_t connectToWifiBlinking(){
  wl_status_t current_status = WiFi.status();
  for (byte i=0; i<50; i++){
      if (current_status == WL_CONNECTED) {
        return current_status;
      }
      blinkBuiltinLed(100);
      current_status = WiFi.status();
  }
  return current_status;
}

void connectToWifi(){
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.print("Connecting to WiFi... ");
  while(1){
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    
    if (connectToWifiBlinking() == WL_CONNECTED){
      break;
    }
    Serial.println(" failed.");
    Serial.print("Retrying... ");
  }
  Serial.println(" connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}


#endif