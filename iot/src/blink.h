#ifndef blink_h
#define blink_h

#include <Arduino.h>

const byte LED_BUILTIN=38;

void blinkBuiltinLed(unsigned int timeToWait){
    digitalWrite(LED_BUILTIN, HIGH);
    delay(timeToWait);
    digitalWrite(LED_BUILTIN, LOW);
    delay(timeToWait);
}

#endif