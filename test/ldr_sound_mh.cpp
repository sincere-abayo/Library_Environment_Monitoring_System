#include <Arduino.h>

// Pin definitions
const int soundSensorPin = A0;    // Sound sensor on A0
const int ldrPin = A1;            // LDR sensor on A1
const int buzzerPin = 9;          // Buzzer on digital pin 9 (PWM for tone)
const int ledPin = 10;            // LED on digital pin 10


// System parameters
const int soundThreshold = 300;
const int lightThreshold = 800;  // Adjust this value based on your environment
bool isAlertActive = false;

// Alert tone frequencies
const int toneFrequencies[] = {2000, 1500, 2000, 1500, 2000};
const int toneDurations[] = {200, 200, 200, 200, 500};
const int numTones = 5;

void playAlertTones() {
  isAlertActive = true;
  
  for (int i = 0; i < numTones; i++) {
    tone(buzzerPin, toneFrequencies[i]);
    delay(toneDurations[i]);
    noTone(buzzerPin);
    delay(100);                                                                                                                                                
  }
  
  isAlertActive = false;
}

void controlLighting() {
  int lightLevel = analogRead(ldrPin);
  if (lightLevel < lightThreshold) {
    digitalWrite(ledPin, HIGH);  // Turn on LED when it's dark
  } else {
    digitalWrite(ledPin, LOW);   // Turn off LED when it's bright
  }
   Serial.print(lightLevel);
    Serial.println();
}

void setup() {
  Serial.begin(9600);
  pinMode(buzzerPin, OUTPUT);
  pinMode(soundSensorPin, INPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(ldrPin, INPUT);
}

void loop() {
  // Handle lighting control
  controlLighting();
  
  // Handle sound detection and alerts
  if (!isAlertActive) {
    int sensorValue = analogRead(soundSensorPin);
    Serial.print(sensorValue);
    Serial.print(" || ");
   
    
    if (sensorValue > soundThreshold) {
      playAlertTones();
    }
  }
  delay(100);
}
