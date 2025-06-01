#ifndef HOMEASSISTANT_H
#define HOMEASSISTANT_H

#include <WiFiClient.h>
#include <PubSubClient.h>

class HomeAssistant {
public:
    HomeAssistant(WiFiClient& netClient,
                 const String& mqttServer, int mqttPort,
                 const String& mqttUser, const String& mqttPass,
                 const String& deviceName, const String& serialNumber,
                 const String& configUrl);

  void begin();
  void loop();
  void publishState(uint16_t co2, float temp, float hum);

private:
  PubSubClient client;
  String mqttServer;
  String mqttUser;
  String mqttPass;
  int mqttPort;
  String deviceName;
  String serial;
  String nodeId;
  String stateTopic;

  void connectMQTT();
  void SendDiscovery();
  void publishDiscovery(const String& entityType,
                        const String& name,
                        const String& stateTopic,
                        const String& commandTopic,
                        const String& deviceClass,
                        const String& unit,
                        const String& valueTemplate);
};

#endif