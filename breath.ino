#include <Arduino.h>
#include <time.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WebSerial.h>
#include <Wire.h>

#include <SensirionI2cScd4x.h>

#include "config.h"
#include "homeassistant.h"
#include "wifi_setup.h"

#ifdef NO_ERROR
#undef NO_ERROR
#endif
#define NO_ERROR 0

AsyncWebServer server(80);

WiFiClient netClient;
SensirionI2cScd4x sensor;

WifiSetup wifiSetup;

static char errorMessage[64];
static int16_t error;
unsigned long last_sensor_reading = millis();

MqttConfig mqttConfig;
HomeAssistant* ha;

void setup() {


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

  //start webserial server
  WebSerial.begin(&server);

  WebSerial.onMessage([&](uint8_t *data, size_t len) {
    WebSerial.println("Received Data...");
    String d = "";
    for(size_t i=0; i < len; i++){
      d += char(data[i]);
    }
    WebSerial.println(d);
  });

  server.begin();

  while (WebSerial.getConnectionCount() == 0) {
    delay(100);
  }
  WebSerial.println("WebSerial client connected!");  

  WebSerial.println(WiFi.localIP());

  //setup CO2 sensor
  Wire.begin();
  sensor.begin(Wire, SCD41_I2C_ADDR_62);    

  uint64_t serialNumber = 0;
  delay(30);
  // Ensure sensor is in clean state
  error = sensor.wakeUp();
  if (error != NO_ERROR) {
      WebSerial.print("Error trying to execute wakeUp(): ");
      errorToString(error, errorMessage, sizeof errorMessage);
      WebSerial.println(errorMessage);
  }
  error = sensor.stopPeriodicMeasurement();
  if (error != NO_ERROR) {
      WebSerial.print("Error trying to execute stopPeriodicMeasurement(): ");
      errorToString(error, errorMessage, sizeof errorMessage);
      WebSerial.println(errorMessage);
  }
  error = sensor.reinit();
  if (error != NO_ERROR) {
      WebSerial.print("Error trying to execute reinit(): ");
      errorToString(error, errorMessage, sizeof errorMessage);
      WebSerial.println(errorMessage);
  }
  // Read out information about the sensor
  error = sensor.getSerialNumber(serialNumber);
  if (error != NO_ERROR) {
      WebSerial.print("Error trying to execute getSerialNumber(): ");
      errorToString(error, errorMessage, sizeof errorMessage);
      WebSerial.println(errorMessage);
      return;
  }
  char buf[21];
  sprintf(buf, "%012llX", serialNumber);
  String sensor_serial = String(buf);

  WebSerial.print("Serial Number: ");
  WebSerial.println(sensor_serial);

  mqttConfig = wifiSetup.getMqttConfig();

  // Log mqttConfig structure
  WebSerial.println("MQTT Config:");
  WebSerial.print("  Server: ");
  WebSerial.println(mqttConfig.server);
  WebSerial.print("  Port: ");
  WebSerial.println(mqttConfig.port);
  WebSerial.print("  User: ");
  WebSerial.println(mqttConfig.user);
  WebSerial.print("  Pass: ");
  WebSerial.println(mqttConfig.pass);

  ha = new HomeAssistant(netClient, mqttConfig.server, mqttConfig.port, mqttConfig.user, mqttConfig.pass, DEVICE_NAME, sensor_serial, WebSerial);
  ha->begin();  
}


void loop() {
  uint16_t co2Concentration = 0;
  float temperature = 0.0;
  float relativeHumidity = 0.0;

  if ((unsigned long)(millis() - last_sensor_reading) > 30000) 
  {
      last_sensor_reading = millis();

      error = sensor.wakeUp();
      if (error != NO_ERROR) {
          WebSerial.print("Error trying to execute wakeUp(): ");
          errorToString(error, errorMessage, sizeof errorMessage);
          WebSerial.println(errorMessage);
          return;
      }

      error = sensor.measureSingleShot();
      if (error != NO_ERROR) {
          WebSerial.print("Error trying to execute measureSingleShot(): ");
          errorToString(error, errorMessage, sizeof errorMessage);
          WebSerial.println(errorMessage);
          return;
      }

      error = sensor.measureAndReadSingleShot(co2Concentration, temperature,
                                              relativeHumidity);
      if (error != NO_ERROR) {
          WebSerial.print("Error trying to execute measureAndReadSingleShot(): ");
          errorToString(error, errorMessage, sizeof errorMessage);
          WebSerial.println(errorMessage);
          return;
      }

      ha->publishState(co2Concentration, temperature, relativeHumidity);      
      
  }
    
  WebSerial.loop();
  ha->loop();
}

