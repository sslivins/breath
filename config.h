#ifndef CONFIG_H
#define CONFIG_H

// Device Info
#define DEVICE_NAME   "Breath"

#define ENABLE_SERIAL_DEBUG  // Enable serial debug output

#define RED_LED LED_BUILTIN
#define RESET_WIFI_PIN 4 // Pin to reset WiFi credentials (if needed)
#define DONE_PIN 7

#define FW_VERSION "0.1.18"
#define IMAGE_MANIFEST_URL "https://github.com/sslivins/breath/releases/latest/download/manifest.json"

#endif
