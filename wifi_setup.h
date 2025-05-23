#ifndef WIFI_SETUP_H
#define WIFI_SETUP_H

#include "settings.h"
#include "config.h"

class WifiSetup {
public:
    WifiSetup();
    void begin(const char* apName = WIFI_AP_NAME);
    MqttConfig getMqttConfig();
    void resetCredentials();

private:
    class Impl;
    Impl* pImpl;
};

#endif