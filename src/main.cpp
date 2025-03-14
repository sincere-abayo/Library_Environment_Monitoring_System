#include <Arduino.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>


// WiFi credentials
const char* ssid = "bjaynet";
const char* password = "bjay1010..";

// Server details
const char* serverName = "http://192.168.43.50/library_monitoring_system/api/add_sensor_data.php";

// Pin definitions
const int soundSensorPin = D0;    // Sound sensor on A0
const int ldrPin = A0;            // LDR sensor on A1
const int mq8Pin = D5;            // MQ-8 sensor on A2
const int mq8DigitalPin = D6;      // MQ-8 digital pin on D2
const int DHTPIN = D7;             // DHT11 on digital pin 3
const int buzzerPin = D3;          // Buzzer on digital pin 9
const int ledPin = D4;    // LED on digital pin 10
const int relayPin = D8;           // Relay control pin on D4


#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

// System parameters
const int soundThreshold = 300;
const int lightThreshold = 800;
const int gasThreshold = 100;     // Adjust based on your needs
const float tempThreshold = 28.0;  // Temperature threshold for fan activation (in Celsius)

bool isAlertActive = false;
bool fanStatus = false;
bool ledStatus = false;

// Sound alert tone pattern
const int toneFrequencies[] = {2000, 1500, 2000, 1500, 2000};
const int toneDurations[] = {200, 200, 200, 200, 500};
const int numTones = 5;

// Gas alert tone pattern (different from sound alert)
const int gasToneFreqs[] = {1000, 800, 1000, 800, 1000, 800};
const int gasToneDurations[] = {1000, 500, 1000, 500, 1000, 500};
const int numGasTones = 6;

// Function to send data to database
void sendToDatabase(float temperature, float humidity, int lightLevel, int soundLevel, int gasLevel) {
  // Check WiFi connection status
  if(WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;
    
    // Your Domain name with URL path or IP address with path
    http.begin(client, serverName);
    
    // Specify content-type header
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    
    // Prepare your HTTP POST request data
    String httpRequestData = "temperature=" + String(temperature) +
                          "&humidity=" + String(humidity) + 
                          "&light_level=" + String(lightLevel) +
                          "&sound_level=" + String(soundLevel) +
                          "&gas_level=" + String(gasLevel) +
                          "&fan_status=" + String(fanStatus ? 1 : 0) +
                          "&led_status=" + String(ledStatus ? 1 : 0);
    
    // Send HTTP POST request
    int httpResponseCode = http.POST(httpRequestData);
    
    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
    }
    else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    // Free resources
    http.end();
  }
  else {
    Serial.println("WiFi Disconnected");
  }
}
void controlFan(float temperature) {
  if (temperature > tempThreshold) {
    digitalWrite(relayPin, HIGH);
    fanStatus = true;
  } else {
    digitalWrite(relayPin, LOW);
    fanStatus = false;
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
    ledStatus = true;
  } else {
    digitalWrite(ledPin, LOW);
    ledStatus = false;
  }
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
  pinMode(relayPin, OUTPUT);


    // Initialize DHT sensor
  dht.begin();
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
    // Read sensor data
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  int lightLevel = analogRead(ldrPin);
  int soundLevel = digitalRead(soundSensorPin);
  int gasLevel = digitalRead(mq8Pin);


   if (!isnan(temperature)) {
    controlFan(temperature);
  }

  // Handle lighting control
  controlLighting();
  
  // Read temperature and humidity
  // readEnvironmentData();

// Print sensor data to serial
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print("°C | Humidity: ");
  Serial.print(humidity);
  Serial.print("% | Light: ");
  Serial.print(lightLevel);
  Serial.print(" | Sound: ");
  Serial.print(soundLevel);
  Serial.print(" | Gas: ");
  Serial.println(gasLevel);

   // Send data to database every 30 seconds
  static unsigned long lastTime = 0;
  if ((millis() - lastTime) > 30000) {
    sendToDatabase(temperature, humidity, lightLevel, soundLevel, gasLevel);
    lastTime = millis();
  }

  // Handle gas detection
  if (!isAlertActive) {
    int gasValue = digitalRead(mq8Pin);
    int gasDigital = digitalRead(mq8DigitalPin);
    
    Serial.print("Gas Value: ");
    Serial.print(gasValue);
    Serial.print(" | Digital: ");
    Serial.print(gasDigital);
    Serial.print(" || ");
    
    // if (gasValue > gasThreshold) {
    //   playGasAlertTones();
    // }
      if (gasDigital) {
      playGasAlertTones();
    }
  }
  
  // Handle sound detection
  
  if (!isAlertActive) {
    int soundValue = digitalRead(soundSensorPin);
    Serial.print("Sound: ");
    Serial.println(soundValue);
    
    // if (soundValue > soundThreshold) {
    //   playAlertTones();
    // }
     if (soundValue ) {
      playAlertTones();
    }
  }
  
  delay(100);
}
