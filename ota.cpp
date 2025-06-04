#include "ota.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Update.h>
#include "config.h"

OTAUpdater::OTAUpdater(const String& manifestUrl) : manifestUrl(manifestUrl) {}

void OTAUpdater::checkAndUpdate() {
    HTTPClient http;
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.begin(manifestUrl);
    int httpCode = http.GET();
    if (httpCode != 200) {
        http.end();
#ifdef ENABLE_SERIAL_DEBUG
        Serial.printf("Failed to fetch manifest: %d\n", httpCode);
#endif
        return;
    }

    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, http.getString());
    http.end();
    if (error) return;

    String latestVersion = doc["version"];
    String binUrl = doc["bin_url"];
    if (isNewerVersion(latestVersion, FW_VERSION)) {
#ifdef ENABLE_SERIAL_DEBUG
        Serial.printf("New version available: %s (current: %s)\n", latestVersion.c_str(), FW_VERSION);
        Serial.printf("Downloading update from: %s\n", binUrl.c_str());
#endif
        performOTA(binUrl);
    }
}

bool OTAUpdater::isNewerVersion(const String& remote, const String& local) {
    return remote != local; // Replace with semantic version compare if needed
}

void OTAUpdater::performOTA(const String& binUrl) {
#ifdef ENABLE_SERIAL_DEBUG
    Serial.println("Starting OTA update...");
#endif
    HTTPClient http;
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.begin(binUrl);
    int httpCode = http.GET();
    if (httpCode != 200) {
#ifdef ENABLE_SERIAL_DEBUG
        Serial.printf("Failed to download firmware: %d\n", httpCode);
#endif
        http.end();
        return;
    }

    int contentLength = http.getSize();
#ifdef ENABLE_SERIAL_DEBUG
    Serial.printf("Firmware size: %d bytes\n", contentLength);
#endif
    bool canBegin = Update.begin(contentLength);
    if (!canBegin) {
#ifdef ENABLE_SERIAL_DEBUG
        Serial.println("Not enough space to begin OTA");
#endif
        http.end();
        return;
    }

    WiFiClient* stream = http.getStreamPtr();
#ifdef ENABLE_SERIAL_DEBUG
    Serial.println("Writing firmware to flash...");
#endif

    Update.onProgress([](size_t written, size_t total) {
        // Example: Blink LED on each progress callback
        static unsigned long lastBlink = 0;
        static bool ledState = LOW;
        if (millis() - lastBlink > 100) { // Blink every 100ms
            ledState = !ledState;
            digitalWrite(RED_LED, ledState);
            lastBlink = millis();
        }
    });

    size_t written = Update.writeStream(*stream);

    // Turn off LED after update
    digitalWrite(RED_LED, LOW);
#ifdef ENABLE_SERIAL_DEBUG
    Serial.printf("Written %u/%u bytes\n", (unsigned)written, (unsigned)contentLength);
#endif
    if (written == contentLength && Update.end()) {
        if (Update.isFinished()) {
#ifdef ENABLE_SERIAL_DEBUG
            Serial.println("OTA update successful. Restarting...");
#endif
            ESP.restart();
        } 
#ifdef ENABLE_SERIAL_DEBUG
        else {

            Serial.println("OTA update not finished.");
        }
#endif        
    } 
#ifdef ENABLE_SERIAL_DEBUG    
    else {

        Serial.printf("OTA update failed. Error #: %d\n", Update.getError());
    }
#endif    
    http.end();
}