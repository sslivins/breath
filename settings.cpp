#include "settings.h"
#include <Preferences.h>

// Implement the constructor and methods here, e.g.:
Settings::Settings() {}

void Settings::setConfig(const MqttConfig& config) {
    Preferences prefs;
    prefs.begin("mqtt", false);
    prefs.putString("server", config.server);
    prefs.putInt("port", config.port);
    prefs.putString("user", config.user);
    prefs.putString("pass", config.pass);
    prefs.end();
}

bool Settings::getConfig(MqttConfig& config) {
    Preferences prefs;
    prefs.begin("mqtt", true);
    config.server = prefs.getString("server", "");
    config.port = prefs.getInt("port", -1);
    config.user = prefs.getString("user", "");
    config.pass = prefs.getString("pass", "");
    prefs.end();
    return config.server.length() > 0 && config.port != -1;
}