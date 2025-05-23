#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <Arduino.h>

#define DEFAULT_MQTT_SERVER "homeassistant.local"
#define DEFAULT_MQTT_PORT   1883
#define WIFI_AP_NAME "Breath-Setup"

struct MqttConfig {
    String server;
    int port;
    String user;
    String pass;
};

class Settings {
public:
    Settings();
    void setConfig(const MqttConfig& config);
    bool getConfig(MqttConfig& config);
};

#endif