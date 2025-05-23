# Breath: ESP32-C6 Air Quality Monitor

This project is an air quality monitor for the Adafruit Feather ESP32-C6 board. It measures CO₂, temperature, and humidity using a Sensirion SCD4x sensor, connects to WiFi using a captive portal (WiFiManager), and publishes data to Home Assistant via MQTT. It also features a WebSerial interface for debugging and live data.

---

## Features

- **WiFi Setup via Captive Portal:**  
  Uses WiFiManager to allow easy WiFi configuration from any device.
- **MQTT Integration:**  
  Publishes sensor data to Home Assistant or any MQTT broker.
- **WebSerial Debugging:**  
  View logs and interact with the device over the network using WebSerial.
- **CO₂, Temperature, Humidity Sensing:**  
  Uses Sensirion SCD4x sensor for accurate air quality measurements.
- **LED and Button Feedback:**  
  Uses the onboard LED and boot button for status and WiFi credential reset.

---

## Hardware

- **Board:** Adafruit Feather ESP32-C6
- **Sensor:** Sensirion SCD4x (CO₂, temperature, humidity)
- **Onboard NeoPixel:** Used for status indication (optional)
- **Boot Button:** Hold at boot to reset WiFi credentials

---

## Getting Started

### 1. **Wiring**

- Connect the SCD4x sensor to the I2C pins of the Feather ESP32-C6.

### 2. **Flashing the Firmware**

- Use Arduino IDE or PlatformIO.
- Select the correct board: *Adafruit Feather ESP32-C6* (or compatible).
- Install required libraries:
  - WiFiManager
  - ESPAsyncWebServer
  - WebSerial
  - Sensirion I2C SCD4x
  - PubSubClient

### 3. **First Boot & WiFi Setup**

- On first boot (or after WiFi reset), the device creates a WiFi AP named `Breath-Setup`.
- Connect to this AP and follow the captive portal to enter your WiFi and MQTT settings.

### 4. **Resetting WiFi Credentials**

- Hold the **boot button** (GPIO 9) while pressing reset or powering up.
- The onboard LED will blink rapidly, credentials will be erased, and the device will restart into AP mode.

---

## Configuration

Edit `config.h` for device-specific settings:

```cpp
#define DEVICE_NAME   "Breath"
#define RED_LED LED_BUILTIN
#define BOOT_BUTTON_PIN 9
#define NEOPIXEL_PIN 9
#define NUMPIXELS    1
```

---

## Home Assistant Integration

- The device publishes MQTT discovery messages for easy integration.
- Add the MQTT integration in Home Assistant and the sensors will appear automatically.

---

## WebSerial

- After connecting to WiFi, open the WebSerial page (see serial output for the URL).
- Use this for debugging and live data.

---

## License

MIT License

---

## Credits

- [Adafruit Feather ESP32-C6](https://www.adafruit.com/product/5679)
- [WiFiManager](https://github.com/tzapu/WiFiManager)
- [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer)
- [WebSerial](https://github.com/ayushsharma82/WebSerial)
- [Sensirion I2C SCD4x](https://github.com/Sensirion/arduino-i2c-scd4x)