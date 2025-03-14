#include <Arduino.h>

// Define LED pin based on the platform
#if defined(ESP8266)
  const int ledPin = 2; // GPIO2 for ESP-01
#else
  const int ledPin = 2; // Pin 2 for Arduino Uno
#endif

void setup() {
  pinMode(ledPin, OUTPUT);
}

void loop() {
  digitalWrite(ledPin, HIGH);
  delay(1000);
  digitalWrite(ledPin, LOW);
  delay(1000);
}
