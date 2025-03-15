#include <Arduino.h>
#include <DHT.h>

// Pin definitions
const int soundSensorPin = A0;    // Sound sensor on A0
const int ldrPin = A1;            // LDR sensor on A1
const int mq8Pin = A2;            // MQ-8 sensor on A2
const int mq8DigitalPin = 2;      // MQ-8 digital pin on D2
const int DHTPIN = 3;             // DHT11 on digital pin 3
const int buzzerPin = 9;          // Buzzer on digital pin 9
const int ledPin = 10;    // LED on digital pin 10
const int relayPin = 4;           // Relay control pin on D4


#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

// System parameters
const int soundThreshold = 300;
const int lightThreshold = 800;
const int gasThreshold = 100;     // Adjust based on your needs
const float tempThreshold = 28.0;  // Temperature threshold for fan activation (in Celsius)

bool isAlertActive = false;

// Sound alert tone pattern
const int toneFrequencies[] = {2000, 1500, 2000, 1500, 2000};
const int toneDurations[] = {200, 200, 200, 200, 500};
const int numTones = 5;

// Gas alert tone pattern (different from sound alert)
const int gasToneFreqs[] = {1000, 800, 1000, 800, 1000, 800};
const int gasToneDurations[] = {1000, 500, 1000, 500, 1000, 500};
const int numGasTones = 6;

void controlFan(float temperature) {
  if (temperature > tempThreshold) {
    digitalWrite(relayPin, HIGH);  // Turn on fan
  } else {
    digitalWrite(relayPin, LOW);   // Turn off fan
  }
}

void playGasAlertTones() {
  isAlertActive = true;
  
  for (int i = 0; i < numGasTones; i++) {
    tone(buzzerPin, gasToneFreqs[i]);
    delay(gasToneDurations[i]);
    noTone(buzzerPin);
    delay(200);
  }
  
  isAlertActive = false;
}

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
    digitalWrite(ledPin, HIGH);
  } else {
    digitalWrite(ledPin, LOW);
  }
  Serial.print(" || ");

  Serial.print("Light Level: ");
  Serial.print(lightLevel);
  Serial.print(" || ");
}


void readEnvironmentData() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  if (!isnan(humidity) && !isnan(temperature)) {
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.print("°C | Humidity: ");
    Serial.print(humidity);
    Serial.print("% || ");
    
    // Control fan based on temperature
    controlFan(temperature);
  }
  else {
    Serial.println("Failed to read sensor data.");
  }
}


void setup() {
  Serial.begin(9600);
  pinMode(buzzerPin, OUTPUT);
  pinMode(soundSensorPin, INPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(ldrPin, INPUT);
  pinMode(mq8Pin, INPUT);
  pinMode(mq8DigitalPin, INPUT);
}

void loop() {

  
  // Read temperature and humidity
  readEnvironmentData();
  
  // Handle lighting control
  controlLighting();

  // Handle gas detection
  if (!isAlertActive) {
    int gasValue = analogRead(mq8Pin);
    int gasDigital = analogRead(mq8DigitalPin);
    
    Serial.print("Gas Value: ");
    Serial.print(gasValue);
    Serial.print(" | Digital: ");
    Serial.print(gasDigital);
    Serial.print(" || ");
    
    if (gasValue > gasThreshold) {
      playGasAlertTones();
    }
  }
  
  // Handle sound detection
  
  if (!isAlertActive) {
    int soundValue = analogRead(soundSensorPin);
    Serial.print("Sound: ");
    Serial.println(soundValue);
    
    if (soundValue > soundThreshold) {
      playAlertTones();
    }
  }
  
  delay(100);
}
