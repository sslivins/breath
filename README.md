# Breath: ESP32-C6 Air Quality Monitor

This project is an air quality monitor for the Adafruit Feather ESP32-C6 board. It measures CO₂, temperature, and humidity using a Sensirion SCD4x sensor, connects to WiFi using a captive portal (WiFiManager), and publishes data to Home Assistant via MQTT.

---

## Features

- **WiFi Setup via Captive Portal:**  
  Uses WiFiManager to allow easy WiFi configuration from any device.
- **MQTT Integration:**  
  Publishes sensor data to Home Assistant or any MQTT broker.  
  - **CO₂, Temperature, Humidity Sensing:**  
    Uses Sensirion SCD4x sensor for accurate air quality measurements.
  - **LED and GPIO 4 Feedback:**  
    Uses the onboard LED for status and GPIO 4 as the WiFi credential reset button.  

---

## Hardware

- **Board:** Adafruit Feather ESP32-C6
- **Sensor:** Sensirion SCD4x (CO₂, temperature, humidity)
- **Onboard NeoPixel:** Used for status indication (optional)
- **GPIO 4:** tie to ground and then power board and wait 10 seconds until led flashes to reset WiFi credentials

---

## Getting Started

### 1. **Wiring**

- Connect the SCD4x sensor to the I2C pins of the Feather ESP32-C6.

### 2. **Flashing the Firmware**

- Use Arduino IDE or PlatformIO.
- Select the correct board: *Adafruit Feather ESP32-C6* (or compatible).
- Install required libraries:
  - WiFiManager
  - Sensirion I2C SCD4x
  - PubSubClient

### 3. **First Boot & WiFi Setup**

- On first boot (or after WiFi reset), the device creates a WiFi AP named `Breath-Setup`.
- Connect to this AP and follow the captive portal to enter your WiFi and MQTT settings.

### 4. **Resetting WiFi Credentials**

- Hold the **WiFi reset button** (GPIO 4) while pressing reset or powering up.
- The onboard LED will blink rapidly, credentials will be erased, and the device will restart into AP mode.

---

## Configuration

Edit `config.h` for device-specific settings:

```cpp
// Device Info
#define DEVICE_NAME   "Breath"

// Enable serial debug output (uncomment to enable)
// #define ENABLE_SERIAL_DEBUG

// Hardware Pins
#define RED_LED         LED_BUILTIN
#define BOOT_BUTTON_PIN 9          // Boot/reset button GPIO
#define NEOPIXEL_PIN    9          // Feather ESP32-C6 NeoPixel is on GPIO 9
#define NUMPIXELS       1          // Only one NeoPixel on the board

#define RESET_WIFI_PIN  4          // Pin to reset WiFi credentials
#define DONE_PIN        7          // Pin to signal done/sleep
```

- **DEVICE_NAME:** Name of your device (used in MQTT topics and Home Assistant).
- **ENABLE_SERIAL_DEBUG:** Uncomment to enable serial debug output.
- **RED_LED:** GPIO for the onboard red LED.
- **BOOT_BUTTON_PIN:** GPIO for the boot/reset button.
- **NEOPIXEL_PIN / NUMPIXELS:** NeoPixel configuration.
- **RESET_WIFI_PIN:** GPIO for the WiFi reset button.
- **DONE_PIN:** GPIO used to signal completion or sleep (e.g., to a power controller).

Adjust these values as needed for your hardware setup.

---

## Home Assistant Integration

- The device publishes MQTT discovery messages for easy integration.
- Add the MQTT integration in Home Assistant and the sensors will appear automatically.

---

## License

MIT License

---

## Credits

- [Adafruit Feather ESP32-C6](https://www.adafruit.com/product/5679)
- [WiFiManager](https://github.com/tzapu/WiFiManager)
- [Sensirion I2C SCD4x](https://github.com/Sensirion/arduino-i2c-scd4x)
- [PubSubClient](https://github.com/knolleary/pubsubclient)