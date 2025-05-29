#pragma once
#include <Arduino.h>

class OTAUpdater {
public:
    OTAUpdater(const String& manifestUrl);
    void checkAndUpdate();

private:
    String manifestUrl;
    bool isNewerVersion(const String& remote, const String& local);
    void performOTA(const String& binUrl);
};