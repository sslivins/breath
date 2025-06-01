#include <Arduino.h>
#include <time.h>
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

  Serial.println("Breath 0.1.0");

  pinMode(RED_LED, OUTPUT);
  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP); // Boot button is usually active LOW

  //add a delay to allow the user to see the boot button state
  digitalWrite(RED_LED, HIGH);
  delay(10000);
  digitalWrite(RED_LED, LOW);

  // Check if boot button is held at startup
  if (digitalRead(BOOT_BUTTON_PIN) == LOW) {
    // Indicate reset mode (blink LED fast)
    for (int i = 0; i < 10; i++) {
      digitalWrite(RED_LED, HIGH);
      delay(250);
      digitalWrite(RED_LED, LOW);
      delay(250);
    }
    // Erase WiFi credentials and restart
    wifiSetup.resetCredentials();
    delay(500);
    ESP.restart();
  }  

  digitalWrite(RED_LED, HIGH);
  wifiSetup.begin("Breath-Setup");
  digitalWrite(RED_LED, LOW);

  // Wait until connected
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(RED_LED, HIGH);   // turn the RED_LED on (HIGH is the voltage level)
    delay(200);                // wait for a half second
    digitalWrite(RED_LED, LOW);    // turn the RED_LED off by making the voltage LOW
    delay(300);
  }

  // Set timezone environment string for Pacific Time (automatically handles DST)
  setenv("TZ", "PST8PDT,M3.2.0/2,M11.1.0/2", 1);
  tzset();  

  // Set up SNTP (Simple Network Time Protocol)
  configTime(0, 0, "pool.ntp.org");  // UTC offset, DST offset, NTP server

  // Wait for time to be set (optional, can retry a few times)
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)) {
    digitalWrite(RED_LED, HIGH);   // turn the RED_LED on (HIGH is the voltage level)
    delay(50);                // wait for a half second
    digitalWrite(RED_LED, LOW);    // turn the RED_LED off by making the voltage LOW
    delay(100);
  }  

  Serial.println("WiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());  

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

  Serial.print("Serial Number: ");
  Serial.println(sensor_serial);

  mqttConfig = wifiSetup.getMqttConfig();

  // Log mqttConfig structure
  Serial.println("MQTT Config:");
  Serial.print("  Server: ");
  Serial.println(mqttConfig.server);
  Serial.print("  Port: ");
  Serial.println(mqttConfig.port);
  Serial.print("  User: ");
  Serial.println(mqttConfig.user);
  Serial.print("  Pass: ");
  Serial.println(mqttConfig.pass);

  ha = new HomeAssistant(netClient, mqttConfig.server, mqttConfig.port, mqttConfig.user, mqttConfig.pass, DEVICE_NAME, sensor_serial);
  ha->begin();  
}


void loop() {
  static char errorMessage[64];
  static int16_t error;  

  uint16_t co2Concentration = 0;
  float temperature = 0.0;
  float relativeHumidity = 0.0;

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
      
  }
    
  ha->loop();
}