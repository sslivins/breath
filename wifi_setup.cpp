#include "wifi_setup.h"
#include <WiFiManager.h>

class WifiSetup::Impl {
public:
    WiFiManager wifiManager;
    WiFiManagerParameter custom_mqtt_server;
    WiFiManagerParameter custom_mqtt_port;
    WiFiManagerParameter custom_mqtt_user;
    WiFiManagerParameter custom_mqtt_pass;
    bool usedConfigPortal;

    Impl()
        : custom_mqtt_server("server", "MQTT Server", "", 40),
          custom_mqtt_port("port", "MQTT Port", "", 6),
          custom_mqtt_user("user", "MQTT User", "", 32),
          custom_mqtt_pass("pass", "MQTT Password", "", 32),
          usedConfigPortal(false)
    {}
};

WifiSetup::WifiSetup() : pImpl(new Impl()) {}

void WifiSetup::begin(const char* apName) {
    // Load MQTT config from NVS
    MqttConfig storedConfig;
    Settings settings;
    if (settings.getConfig(storedConfig)) {
        pImpl->custom_mqtt_server.setValue(storedConfig.server.c_str(), 40);
        char portStr[8];
        snprintf(portStr, sizeof(portStr), "%d", storedConfig.port);
        pImpl->custom_mqtt_port.setValue(portStr, 6);
        pImpl->custom_mqtt_user.setValue(storedConfig.user.c_str(), 32);
        pImpl->custom_mqtt_pass.setValue(storedConfig.pass.c_str(), 32);
    } else {
        pImpl->custom_mqtt_server.setValue(DEFAULT_MQTT_SERVER, 40);
        pImpl->custom_mqtt_port.setValue(String(DEFAULT_MQTT_PORT).c_str(), 6);
    }

    pImpl->wifiManager.addParameter(&pImpl->custom_mqtt_server);
    pImpl->wifiManager.addParameter(&pImpl->custom_mqtt_port);
    pImpl->wifiManager.addParameter(&pImpl->custom_mqtt_user);
    pImpl->wifiManager.addParameter(&pImpl->custom_mqtt_pass);

    pImpl->wifiManager.setSaveConfigCallback([this]() {
        pImpl->usedConfigPortal = true;
    });

    pImpl->wifiManager.autoConnect(apName);

    // If portal was used, save new values to NVS
    if (pImpl->usedConfigPortal) {
        MqttConfig config;
        config.server = String(pImpl->custom_mqtt_server.getValue());
        config.port = atoi(pImpl->custom_mqtt_port.getValue());
        config.user = String(pImpl->custom_mqtt_user.getValue());
        config.pass = String(pImpl->custom_mqtt_pass.getValue());
        settings.setConfig(config);
    }
}

void WifiSetup::resetCredentials() {
    pImpl->wifiManager.resetSettings();
}

MqttConfig WifiSetup::getMqttConfig() {
    Settings settings;
    MqttConfig config;
    if (settings.getConfig(config)) {
        return config;
    }
    // If not found, return current portal values (may be defaults)
    config.server = String(pImpl->custom_mqtt_server.getValue());
    config.port = atoi(pImpl->custom_mqtt_port.getValue());
    config.user = String(pImpl->custom_mqtt_user.getValue());
    config.pass = String(pImpl->custom_mqtt_pass.getValue());
    return config;
}