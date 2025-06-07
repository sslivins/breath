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

#ifdef ENABLE_SERIAL_DEBUG
  Serial.begin(SERIAL_BAUD_RATE);
#endif

#ifdef ENABLE_SERIAL_DEBUG
  Serial.print("Breath ");
  Serial.println(FW_VERSION);
#endif

  pinMode(RED_LED, OUTPUT);
  pinMode(RESET_WIFI_PIN, INPUT_PULLUP); // Button is active LOW
  pinMode(DONE_PIN, OUTPUT);
  digitalWrite(DONE_PIN, LOW); //set pin to LOW initially

  // Check if boot button is held at startup
  if (digitalRead(RESET_WIFI_PIN) == LOW) {
    unsigned long startTime = millis();
    bool stillHolding = true;

#ifdef ENABLE_SERIAL_DEBUG
    Serial.println("Boot button held, entering WiFi reset mode...");
#endif

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
#ifdef ENABLE_SERIAL_DEBUG
      Serial.println("Button still held, resetting WiFi credentials...");
#endif
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
#ifdef ENABLE_SERIAL_DEBUG
      Serial.print("Error trying to execute wakeUp(): ");
      errorToString(error, errorMessage, sizeof errorMessage);
      Serial.println(errorMessage);
#endif
  }
  error = sensor.stopPeriodicMeasurement();
  if (error != NO_ERROR) {
#ifdef ENABLE_SERIAL_DEBUG
      Serial.print("Error trying to execute stopPeriodicMeasurement(): ");
      errorToString(error, errorMessage, sizeof errorMessage);
      Serial.println(errorMessage);
#endif
  }
  error = sensor.reinit();
  if (error != NO_ERROR) {
#ifdef ENABLE_SERIAL_DEBUG
      Serial.print("Error trying to execute reinit(): ");
      errorToString(error, errorMessage, sizeof errorMessage);
      Serial.println(errorMessage);
#endif
  }
  // Read out information about the sensor
  error = sensor.getSerialNumber(serialNumber);
  if (error != NO_ERROR) {
#ifdef ENABLE_SERIAL_DEBUG
      Serial.print("Error trying to execute getSerialNumber(): ");
      errorToString(error, errorMessage, sizeof errorMessage);
      Serial.println(errorMessage);
#endif
      return;
  }
  char buf[21];
  sprintf(buf, "%012llX", serialNumber);
  String sensor_serial = String(buf);

  mqttConfig = wifiSetup.getMqttConfig();

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
#ifdef ENABLE_SERIAL_DEBUG
          Serial.print("Error trying to execute wakeUp(): ");
          errorToString(error, errorMessage, sizeof errorMessage);
          Serial.println(errorMessage);
#endif
          return;
      }

      error = sensor.measureSingleShot();
      if (error != NO_ERROR) {
#ifdef ENABLE_SERIAL_DEBUG
          Serial.print("Error trying to execute measureSingleShot(): ");
          errorToString(error, errorMessage, sizeof errorMessage);
          Serial.println(errorMessage);
#endif
          return;
      }

      error = sensor.measureAndReadSingleShot(co2Concentration, temperature,
                                              relativeHumidity);
      if (error != NO_ERROR) {
#ifdef ENABLE_SERIAL_DEBUG
          Serial.print("Error trying to execute measureAndReadSingleShot(): ");
          errorToString(error, errorMessage, sizeof errorMessage);
          Serial.println(errorMessage);
#endif
          return;
      }

      //read battery voltage and conver to percentage
      //hardcode to 100% for now
      int batteryPercentage = 100;

      ha->publishState(co2Concentration, temperature, relativeHumidity, batteryPercentage);

      //run loop for a second to ensure message is sent
      for(int i = 0; i < 3; i++) {
          ha->loop();
          delay(100);
      }      

      ha->disconnect();



      digitalWrite(DONE_PIN, HIGH); // Set pin high tell TPS5110 to go to sleep
  }


  ha->loop();
}