#include <Arduino.h>

// Define the pin connected to the sound sensor's analog output
const int soundSensorPin = A0;

void setup() {
  // Initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
}

void loop() {
  // Read the value from the sound sensor:
  int sensorValue = analogRead(soundSensorPin);
  // Print out the value to the serial monitor
  Serial.println(sensorValue);
  delay(500); // Delay in between reads for stability
}