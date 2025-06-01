#ifndef CONFIG_H
#define CONFIG_H

// Device Info
#define DEVICE_NAME   "Breath"

#define RED_LED LED_BUILTIN
#define BOOT_BUTTON_PIN 9
#define NEOPIXEL_PIN 9   // Feather ESP32-C6 NeoPixel is on GPIO 9
#define NUMPIXELS    1   // There's only one NeoPixel on the board

#define FW_VERSION "0.1.4"
#define IMAGE_MANIFEST_URL "https://github.com/sslivins/breath/releases/latest/download/manifest.json"

#endif
