#include <Arduino.h>
// #include <time.h>
#include <WiFi.h>
#include <Wire.h>

#include <SensirionI2cScd4x.h>

#include "config.h"
#include "homeassistant.h"
#include "wifi_setup.h"
#include "ota.h"

#ifdef NO_ERROR
#undef NO_ERROR
#endif
#define NO_ERROR 0

WiFiClient netClient;
SensirionI2cScd4x sensor;

WifiSetup wifiSetup;

unsigned long last_sensor_reading = millis();

MqttConfig mqttConfig;
HomeAssistant* ha;

void setup() {
  static char errorMessage[64];
  static int16_t error;  

  //USB-CDC on boot must be enabled for serial port output to work
  Serial.begin(115200);

  Serial.print("Breath ");
  Serial.println(FW_VERSION);

  pinMode(RED_LED, OUTPUT);
  pinMode(RESET_WIFI_PIN, INPUT_PULLUP); // Button is active LOW
  pinMode(DONE_PIN, OUTPUT);
  digitalWrite(DONE_PIN, LOW); //set pin to LOW initially

  // Check if boot button is held at startup
  if (digitalRead(RESET_WIFI_PIN) == LOW) {
    unsigned long startTime = millis();
    bool stillHolding = true;

    Serial.println("Boot button held, entering WiFi reset mode...");

    // Blink LED slowly for 10 seconds, checking button state
    while (millis() - startTime < 10000) {
      digitalWrite(RED_LED, HIGH);
      delay(200);
      digitalWrite(RED_LED, LOW);
      delay(300);

      if (digitalRead(RESET_WIFI_PIN) != LOW) {
        stillHolding = false;
        break;
      }
    }

    if (stillHolding) {
      Serial.println("Button still held, resetting WiFi credentials...");
      // Blink LED rapidly for 2 seconds before reset
      for (int i = 0; i < 10; i++) {
        digitalWrite(RED_LED, HIGH);
        delay(50);
        digitalWrite(RED_LED, LOW);
        delay(50);
      }
      // Erase WiFi credentials and restart
      wifiSetup.resetCredentials();
      digitalWrite(RED_LED, LOW); // LED off after reset
      delay(500);
      ESP.restart();
    }
  }

  wifiSetup.begin("Breath-Setup");

  // Wait until connected
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(RED_LED, HIGH);   // turn the RED_LED on (HIGH is the voltage level)
    delay(200);                // wait for a half second
    digitalWrite(RED_LED, LOW);    // turn the RED_LED off by making the voltage LOW
    delay(300);
  }

  // // Set timezone environment string for Pacific Time (automatically handles DST)
  // setenv("TZ", "PST8PDT,M3.2.0/2,M11.1.0/2", 1);
  // tzset();  

  // // Set up SNTP (Simple Network Time Protocol)
  // configTime(0, 0, "pool.ntp.org");  // UTC offset, DST offset, NTP server

  // // Wait for time to be set (optional, can retry a few times)
  // struct tm timeinfo;
  // while (!getLocalTime(&timeinfo)) {
  //   digitalWrite(RED_LED, HIGH);   // turn the RED_LED on (HIGH is the voltage level)
  //   delay(50);                // wait for a half second
  //   digitalWrite(RED_LED, LOW);    // turn the RED_LED off by making the voltage LOW
  //   delay(100);
  // }  

  // Serial.println("WiFi connected!");
  // Serial.print("IP Address: ");
  // Serial.println(WiFi.localIP());  

  OTAUpdater ota(IMAGE_MANIFEST_URL);
  ota.checkAndUpdate();

  //setup CO2 sensor
  Wire.begin();
  sensor.begin(Wire, SCD41_I2C_ADDR_62);    

  uint64_t serialNumber = 0;
  delay(30);
  // Ensure sensor is in clean state
  error = sensor.wakeUp();
  if (error != NO_ERROR) {
      Serial.print("Error trying to execute wakeUp(): ");
      errorToString(error, errorMessage, sizeof errorMessage);
      Serial.println(errorMessage);
  }
  error = sensor.stopPeriodicMeasurement();
  if (error != NO_ERROR) {
      Serial.print("Error trying to execute stopPeriodicMeasurement(): ");
      errorToString(error, errorMessage, sizeof errorMessage);
      Serial.println(errorMessage);
  }
  error = sensor.reinit();
  if (error != NO_ERROR) {
      Serial.print("Error trying to execute reinit(): ");
      errorToString(error, errorMessage, sizeof errorMessage);
      Serial.println(errorMessage);
  }
  // Read out information about the sensor
  error = sensor.getSerialNumber(serialNumber);
  if (error != NO_ERROR) {
      Serial.print("Error trying to execute getSerialNumber(): ");
      errorToString(error, errorMessage, sizeof errorMessage);
      Serial.println(errorMessage);
      return;
  }
  char buf[21];
  sprintf(buf, "%012llX", serialNumber);
  String sensor_serial = String(buf);

  // Serial.print("Serial Number: ");
  // Serial.println(sensor_serial);

  mqttConfig = wifiSetup.getMqttConfig();

  // Log mqttConfig structure
  // Serial.println("MQTT Config:");
  // Serial.print("  Server: ");
  // Serial.println(mqttConfig.server);
  // Serial.print("  Port: ");
  // Serial.println(mqttConfig.port);
  // Serial.print("  User: ");
  // Serial.println(mqttConfig.user);
  // Serial.print("  Pass: ");
  // Serial.println(mqttConfig.pass);

  String configUrl = "http://" + WiFi.localIP().toString();
  ha = new HomeAssistant(netClient, mqttConfig.server, mqttConfig.port, mqttConfig.user, mqttConfig.pass, DEVICE_NAME, sensor_serial, configUrl);
  ha->begin();  
}

void loop() {
  static char errorMessage[64];
  static int16_t error;

  uint16_t co2Concentration = 0;
  float temperature = 0.0;
  float relativeHumidity = 0.0;
  // Check if WiFi is still connecte

  if ((unsigned long)(millis() - last_sensor_reading) > 30000) 
  {
      last_sensor_reading = millis();

      error = sensor.wakeUp();
      if (error != NO_ERROR) {
          Serial.print("Error trying to execute wakeUp(): ");
          errorToString(error, errorMessage, sizeof errorMessage);
          Serial.println(errorMessage);
          return;
      }

      error = sensor.measureSingleShot();
      if (error != NO_ERROR) {
          Serial.print("Error trying to execute measureSingleShot(): ");
          errorToString(error, errorMessage, sizeof errorMessage);
          Serial.println(errorMessage);
          return;
      }

      error = sensor.measureAndReadSingleShot(co2Concentration, temperature,
                                              relativeHumidity);
      if (error != NO_ERROR) {
          Serial.print("Error trying to execute measureAndReadSingleShot(): ");
          errorToString(error, errorMessage, sizeof errorMessage);
          Serial.println(errorMessage);
          return;
      }

      ha->publishState(co2Concentration, temperature, relativeHumidity);

      //set DONE_PIN high to indicate sensor reading is done and trigger TPS5110 to go to sleep
      digitalWrite(DONE_PIN, HIGH);
      //wait for TPS5110 to go to slee
      delay(100);
      //set DONE_PIN low to indicate sensor reading is done and trigger TPS5110 to go to sleep
      digitalWrite(DONE_PIN, LOW);
      
  }
    
  ha->loop();
}