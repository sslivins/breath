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
        Serial.printf("Failed to fetch manifest: %d\n", httpCode);
        return;
    }

    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, http.getString());
    http.end();
    if (error) return;

    String latestVersion = doc["version"];
    String binUrl = doc["bin_url"];
    if (isNewerVersion(latestVersion, FW_VERSION)) {
        Serial.printf("New version available: %s (current: %s)\n", latestVersion.c_str(), FW_VERSION);
        Serial.printf("Downloading update from: %s\n", binUrl.c_str());
        performOTA(binUrl);
    }
}

bool OTAUpdater::isNewerVersion(const String& remote, const String& local) {
    return remote != local; // Replace with semantic version compare if needed
}

void OTAUpdater::performOTA(const String& binUrl) {
    Serial.println("Starting OTA update...");
    HTTPClient http;
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.begin(binUrl);
    int httpCode = http.GET();
    if (httpCode != 200) {
        Serial.printf("Failed to download firmware: %d\n", httpCode);
        http.end();
        return;
    }

    int contentLength = http.getSize();
    Serial.printf("Firmware size: %d bytes\n", contentLength);
    bool canBegin = Update.begin(contentLength);
    if (!canBegin) {
        Serial.println("Not enough space to begin OTA");
        http.end();
        return;
    }

    WiFiClient* stream = http.getStreamPtr();
    Serial.println("Writing firmware to flash...");
    size_t written = Update.writeStream(*stream);
    Serial.printf("Written %u/%u bytes\n", (unsigned)written, (unsigned)contentLength);
    if (written == contentLength && Update.end()) {
        if (Update.isFinished()) {
            Serial.println("OTA update successful. Restarting...");
            ESP.restart();
        } else {
            Serial.println("OTA update not finished.");
        }
    } else {
        Serial.printf("OTA update failed. Error #: %d\n", Update.getError());
    }
    http.end();
}