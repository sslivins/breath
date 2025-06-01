#include "ota.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Update.h>
#include "config.h"

OTAUpdater::OTAUpdater(const String& manifestUrl) : manifestUrl(manifestUrl) {}

void OTAUpdater::checkAndUpdate() {
    HTTPClient http;
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
        performOTA(binUrl);
    }
}

bool OTAUpdater::isNewerVersion(const String& remote, const String& local) {
    return remote != local; // Replace with semantic version compare if needed
}

void OTAUpdater::performOTA(const String& binUrl) {
    HTTPClient http;
    http.begin(binUrl);
    int httpCode = http.GET();
    if (httpCode != 200) {
        http.end();
        return;
    }

    int contentLength = http.getSize();
    bool canBegin = Update.begin(contentLength);
    if (!canBegin) {
        http.end();
        return;
    }

    WiFiClient* stream = http.getStreamPtr();
    size_t written = Update.writeStream(*stream);
    if (written == contentLength && Update.end()) {
        if (Update.isFinished()) {
            ESP.restart();
        }
    }
    http.end();
}