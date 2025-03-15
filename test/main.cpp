#include <Arduino.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

// WiFi credentials
const char* ssid = "bjaynet";
const char* password = "bjay1010..";

// LCD setup (0x27 is the default I2C address, adjust if needed)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Server details
const char* serverName = "http://192.168.43.50/library_monitoring_system/api/add_sensor_data.php";
const char* getModeUrl = "http://192.168.43.50/library_monitoring_system/api/get_mode.php";
const char* getControlUrl = "http://192.168.43.50/library_monitoring_system/api/get_control.php";

// Pin definitions
const int soundSensorPin = D0;    // Sound sensor
const int ldrPin = A0;            // LDR sensor
// const int mq8Pin = D5;            // MQ-8 sensor
const int mq8DigitalPin = D6;     // MQ-8 digital pin
const int DHTPIN = D4;            // DHT22
const int buzzerPin = D5;         // Buzzer
const int ledPin = D8;            // LED
const int relayPin = D7;          // Relay for fan

#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// System parameters
const int soundThreshold = 300;
const int lightThreshold = 800;
const int gasThreshold = 100;
const float tempThreshold = 28.0;

bool isAlertActive = false;
bool fanStatus = 0;
bool ledStatus = 0;
bool automationMode = false;  // Default to manual mode
// Add these variables
bool manualAlarmActive = false;
String alarmReason = "";
// Define alert tone patterns
const int soundAlertTones[] = {2000, 1500, 2000, 1500, 2000};
const int soundAlertDurations[] = {200, 200, 200, 200, 500};
const int numSoundAlertTones = 5;

const int gasAlertTones[] = {3000, 2500, 3000, 2500, 3000, 2500, 3000};
const int gasAlertDurations[] = {150, 150, 150, 150, 150, 150, 500};
const int numGasAlertTones = 7;

void playManualAlarm();
void  checkManualAlarm();
// Function to get operation mode from server
bool getOperationMode() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;
    
    http.begin(client, getModeUrl);
    int httpResponseCode = http.GET();
    
    if (httpResponseCode > 0) {
      String payload = http.getString();
      
      // Parse JSON response
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, payload);
      
      bool mode = doc["automation_mode"];
      http.end();
      return mode;
    }
    http.end();
  }
  return false; // Default to manual mode if can't connect
}

// Function to get manual control commands when in manual mode
void getManualControl() {
  if (!automationMode && WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;
    
    http.begin(client, getControlUrl);
    int httpResponseCode = http.GET();
    
    if (httpResponseCode > 0) {
      String payload = http.getString();
      
      // Parse JSON response
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, payload);
      
      // Update device status based on manual control
      fanStatus = doc["fan_status"];
      ledStatus = doc["led_status"];
      
      
      // Apply the manual controls
      digitalWrite(relayPin, fanStatus ? LOW : HIGH);
      digitalWrite(ledPin, ledStatus ? LOW : HIGH);
      
      Serial.print("Manual control - Fan: ");
      Serial.print(fanStatus ? "ON" : "OFF");
      Serial.print(", LED: ");
      Serial.println(ledStatus ? "ON" : "OFF");
    }
    http.end();
  }
}

// Function to control devices in automation mode
void automatedControl(float temperature, int lightLevel) {
  if (automationMode) {
    // Control fan based on temperature
    if (temperature > tempThreshold) {
      digitalWrite(relayPin, LOW);
      fanStatus = true;
      Serial.println("Auto: Fan turned ON (high temperature)");
    } else {
      digitalWrite(relayPin, HIGH);
      fanStatus = false;
      Serial.println("Auto: Fan turned OFF (normal temperature)");
    }
    
    // Control LED based on light level
    if (lightLevel < lightThreshold) {
      digitalWrite(ledPin, LOW);
      ledStatus = true;
      Serial.println("Auto: LED turned ON (low light)");
    } else {
      digitalWrite(ledPin, HIGH);
      ledStatus = false;
      Serial.println("Auto: LED turned OFF (bright environment)");
    }
  }
}

// Function to send data to database
void sendToDatabase(float temperature, float humidity, int lightLevel, int soundLevel, int gasLevel) {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;
    
    http.begin(client, serverName);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    
    String httpRequestData = "temperature=" + String(temperature) +
                          "&humidity=" + String(humidity) + 
                          "&light_level=" + String(lightLevel) +
                          "&sound_level=" + String(soundLevel) +
                          "&gas_level=" + String(gasLevel);
                          // "&fan_status=" + String(fanStatus ? 1 : 0) +
                          // "&led_status=" + String(ledStatus ? 1 : 0);
    
    int httpResponseCode = http.POST(httpRequestData);
    
    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
    } else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("WiFi Disconnected");
  }
}

// Function to update LCD display
void updateLCD(float temperature, float humidity, int lightLevel, bool fanOn, bool ledOn) {
  static unsigned long lastLCDUpdate = 0;
  static int displayState = 0;
  
  // Update LCD every 3 seconds and rotate between different screens
  if (millis() - lastLCDUpdate > 3000) {
    lastLCDUpdate = millis();
    displayState = (displayState + 1) % 3;  // Cycle through 3 different screens
    
    lcd.clear();
    
    switch (displayState) {
      case 0:  // Temperature and humidity
        lcd.setCursor(0, 0);
        lcd.print("Temp: ");
        lcd.print(temperature, 1);
        lcd.print((char)223);  // Degree symbol
        lcd.print("C");
        
        lcd.setCursor(0, 1);
        lcd.print("Humidity: ");
        lcd.print(humidity, 1);
        lcd.print("%");
        break;
        
      case 1:  // Light level and mode
        lcd.setCursor(0, 0);
        lcd.print("Light: ");
        lcd.print(lightLevel);
        
        lcd.setCursor(0, 1);
        lcd.print("Mode: ");
        lcd.print(automationMode ? "AUTO" : "MANUAL");
        break;
        
      case 2:  // Device status
        lcd.setCursor(0, 0);
        lcd.print("Fan: ");
        lcd.print(fanOn ? "ON " : "OFF");
        
        lcd.setCursor(8, 0);
        lcd.print("LED: ");
        lcd.print(ledOn ? "ON " : "OFF");
        
        lcd.setCursor(0, 1);
        if (isAlertActive) {
          lcd.print("ALERT ACTIVE!");
        } else {
          lcd.print("System Normal");
        }
        break;
    }
  }
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


// Function to play alert tones for sound detection
void playSoundAlert() {
  isAlertActive = true;
  
  // Display alert on LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SOUND ALERT!");
  lcd.setCursor(0, 1);
  lcd.print("Noise detected");
  
  // Play alert pattern 3 times for sound detection
  for (int repeat = 0; repeat < 3; repeat++) {
    for (int i = 0; i < numSoundAlertTones; i++) {
      tone(buzzerPin, soundAlertTones[i]);
      delay(soundAlertDurations[i]);
      noTone(buzzerPin);
      delay(50);  // Short pause between tones
    }
    delay(300);  // Pause between pattern repetitions
  }
  
  isAlertActive = false;
}

// Function to play alert tones for gas detection (more urgent pattern)
void playGasAlert() {
  isAlertActive = true;
  
  // Display alert on LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("GAS ALERT!");
  lcd.setCursor(0, 1);
  lcd.print("EVACUATE AREA!");
  
  // Play urgent alert pattern 5 times for gas detection
  for (int repeat = 0; repeat < 5; repeat++) {
    for (int i = 0; i < numGasAlertTones; i++) {
      tone(buzzerPin, gasAlertTones[i]);
      delay(gasAlertDurations[i]);
      noTone(buzzerPin);
      delay(30);  // Shorter pause for more urgent feel
    }
    delay(200);  // Shorter pause between repetitions for urgency
  }
  
  isAlertActive = false;
}


// Function to play manual alarm
void playManualAlarm() {
  isAlertActive = true;
  
  // Play a distinctive pattern for manual alarms
  for (int repeat = 0; repeat < 10; repeat++) {
    // SOS pattern (... --- ...)
    // Short beeps (S)
    for (int i = 0; i < 3; i++) {
      tone(buzzerPin, 2500);
      delay(200);
      noTone(buzzerPin);
      delay(200);
    }
    
    delay(300);
    
    // Long beeps (O)
    for (int i = 0; i < 3; i++) {
      tone(buzzerPin, 2500);
      delay(600);
      noTone(buzzerPin);
      delay(200);
    }
    
    delay(300);
    
    // Short beeps (S)
    for (int i = 0; i < 3; i++) {
      tone(buzzerPin, 2500);
      delay(200);
      noTone(buzzerPin);
      delay(200);
    }
    
    delay(1000);
    
    // Check if alarm was stopped
    checkManualAlarm();
    if (!manualAlarmActive) {
      break;
    }
  }
  
  isAlertActive = false;
}


// Function to check for manual alarm activation
void checkManualAlarm() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;
    
    // Create a URL to get alarm status
    String url = String(serverName);
    url.replace("add_sensor_data.php", "get_alarm_status.php");
    
    http.begin(client, url);
    int httpResponseCode = http.GET();
    
    if (httpResponseCode > 0) {
      String payload = http.getString();
      
      // Parse JSON response
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, payload);
      
      bool alarmStatus = doc["alarm_active"];
      
      // If alarm status changed
      if (alarmStatus != manualAlarmActive) {
        manualAlarmActive = alarmStatus;
        
        if (manualAlarmActive) {
          // Get the reason
          alarmReason = doc["alarm_reason"].as<String>();
          
          // Display on LCD
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("MANUAL ALARM!");
          lcd.setCursor(0, 1);
          lcd.print(alarmReason);
          
          // Trigger alarm
          playManualAlarm();
        } else {
          // Alarm was stopped
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Alarm stopped");
          lcd.setCursor(0, 1);
          lcd.print("System normal");
          delay(2000);
        }
      }
    }
    http.end();
  }
}
void setup() {
  Serial.begin(115200);
  
  // Initialize I2C and LCD
  Wire.begin(D2, D1);  // SDA, SCL pins for ESP8266 (D2=GPIO4, D3=GPIO0)
    lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Library Monitor");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");

  // Initialize pins
  pinMode(buzzerPin, OUTPUT);
  pinMode(soundSensorPin, INPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(ldrPin, INPUT);
  // pinMode(mq8Pin, INPUT);
  pinMode(mq8DigitalPin, INPUT);
  pinMode(relayPin, OUTPUT);
  
  // Initialize with devices off
  digitalWrite(ledPin, HIGH);
  digitalWrite(relayPin, HIGH);
  
  // Initialize DHT sensor
  dht.begin();
  
   // Connect to WiFi
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
 WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  int dots = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    lcd.setCursor(dots % 16, 1);
    lcd.print(".");
    dots++;
  }
  Serial.println();
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi Connected");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
  // delay(2000);
}

void loop() {
  // Check operation mode every 5 seconds
  static unsigned long lastModeCheck = 0;
  if (millis() - lastModeCheck > 50) {
    bool newMode = getOperationMode();
    if (newMode != automationMode) {
      automationMode = newMode;
      Serial.print("Operation mode changed to: ");
      Serial.println(automationMode ? "AUTOMATION" : "MANUAL");
      
      // Update LCD immediately when mode changes
      
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Mode changed to:");
      lcd.setCursor(0, 1);
      lcd.print(automationMode ? "AUTOMATION" : "MANUAL");
      delay(500);  // Show mode change for 2 seconds
    }
    lastModeCheck = millis();
  }
   // Check for manual alarm activation every 5 seconds
  static unsigned long lastAlarmCheck = 0;
  if (millis() - lastAlarmCheck > 50) {
    checkManualAlarm();
    lastAlarmCheck = millis();
  }
  
  // Read sensor data
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  int lightLevel = analogRead(ldrPin);
  int soundLevel = digitalRead(soundSensorPin);
  int gasLevel = digitalRead(mq8DigitalPin);
  
  // Update LCD with current readings
  updateLCD(temperature, humidity, lightLevel, fanStatus, ledStatus);
  
  // Rest of your existing loop code...
  
  // Print sensor data
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
  
  // Control logic based on operation mode
  if (automationMode) {
    // In automation mode, control devices based on sensor readings
    automatedControl(temperature, lightLevel);
  } else {
    // In manual mode, get control commands from server
    getManualControl();
  }
    // Handle lighting control
  controlLighting();
  // Handle alerts (always active regardless of mode)
  if (!isAlertActive) {
    // Sound alert
    if (soundLevel == 0) {
      Serial.println("Sound alert triggered!");
      // Add your sound alert code here
      playSoundAlert();
      // Show alert on LCD
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("SOUND ALERT!");
      lcd.setCursor(0, 1);
      lcd.print("Noise detected");
    }
    
    // Gas alert
    if (gasLevel == 0) {
      Serial.println("Gas alert triggered!");
      // Add your gas alert code here
       playGasAlert();


      // Show alert on LCD
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("GAS ALERT!");
      lcd.setCursor(0, 1);
      lcd.print("Gas detected");
    }
  }
  
  // Send data to database every 1 seconds
  static unsigned long lastDataSend = 0;
  if (millis() - lastDataSend > 1000) {
    sendToDatabase(temperature, humidity, lightLevel, soundLevel, gasLevel);
    lastDataSend = millis();
  }
  
  delay(500);
}
