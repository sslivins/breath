#include <ArduinoJson.h>

#include "homeassistant.h"

HomeAssistant::HomeAssistant(WiFiClient& netClient,
                             const String& mqttServer, int mqttPort,
                             const String& mqttUser, const String& mqttPass,
                             const String& deviceName, const String& serialNumber,
                             WebSerialClass& webserial)
  : client(netClient),
    mqttServer(mqttServer), mqttPort(mqttPort),
    mqttUser(mqttUser), mqttPass(mqttPass),
    deviceName(deviceName), serial(serialNumber),
    webserial(webserial),
    nodeId(deviceName + "_" + serial),
    stateTopic(nodeId + "/state") {}

void HomeAssistant::begin() {
  connectMQTT();
  SendDiscovery();
}

void HomeAssistant::loop() {
  client.loop();
}

void HomeAssistant::connectMQTT() {
  client.setServer(mqttServer.c_str(), mqttPort);
  client.setBufferSize(1024);  // Make room for JSON messages

  // Set callback once before connecting
  client.setCallback([this](char* topic, byte* payload, unsigned int length) {
    String message;
    for (unsigned int i = 0; i < length; i++) {
      message += (char)payload[i];
    }

    webserial.print("MQTT command received on ");
    webserial.print(topic);
    webserial.print(": ");
    webserial.println(message);

    String ledCommandTopic = nodeId + "/led/set";

    // Handle LED switch
    if (String(topic) == ledCommandTopic) {
      bool ledState = (message == "ON");
      digitalWrite(LED_BUILTIN, ledState ? HIGH : LOW);

      String stateTopic = nodeId + "/led/state";
      client.publish(stateTopic.c_str(), ledState ? "ON" : "OFF", true);
      return;
    }

  });

  int retries = 5;
  while (!client.connected() && retries-- > 0) {
    webserial.println("Connecting to MQTT...");
    if (client.connect(nodeId.c_str(), mqttUser.c_str(), mqttPass.c_str())) {
      webserial.println("✅ MQTT connected.");

      String controlTopic;
      // Subscribe to LED control topic
      controlTopic = nodeId + "/led/set";
      client.subscribe(controlTopic.c_str());

      return;

    } else {
      webserial.print("❌ MQTT connect failed. rc=");
      webserial.println(client.state());
      delay(1000);
    }
  }

  if (!client.connected()) {
    webserial.println("❌ Failed to connect to MQTT after retries.");
  }
}


void HomeAssistant::SendDiscovery() {
  if (!client.connected()) {
    webserial.println("❌ MQTT not connected — reconnecting...");
    connectMQTT();

    if (!client.connected()) {  // ← this had a double negation before
      webserial.println("❌ MQTT failed to reconnect");
      return;
    }

    webserial.println("✅ MQTT successfully reconnected");
  }

  publishDiscovery(
    String("sensor"),
    String("co2"),
    String(nodeId + "/state"),
    String(""),
    String("carbon_dioxide"),
    String("ppm"),
    String("{{ value_json.co2 }}")
  );

  publishDiscovery(
    String("sensor"),
    String("temperature"),
    String(nodeId + "/state"),
    String(""),
    String("temperature"),
    String("°C"),
    String("{{ value_json.temperature }}")
  );

  publishDiscovery(
    String("sensor"),
    String("humidity"),
    String(nodeId + "/state"),
    String(""),
    String("humidity"),
    String("%"),
    String("{{ value_json.humidity }}")
  );

  publishDiscovery(
    String("sensor"),
    String("last_updated"),
    String(nodeId + "/state"),
    String(""),
    String("timestamp"),
    String(""),
    String("{{ now() }}")
  );

  publishDiscovery(
    String("switch"),
    String("led"),
    String(nodeId + "/led/state"),
    String(nodeId + "/led/set"),
    String(""),
    String(""),
    String("")
  );

}

void HomeAssistant::publishDiscovery(const String& entityType,  // e.g. "sensor", "switch"
                                     const String& name,
                                     const String& stateTopic,
                                     const String& commandTopic,
                                     const String& deviceClass,
                                     const String& unit,
                                     const String& valueTemplate) {
    String uid = nodeId + "_" + name;
    String topic = "homeassistant/" + entityType + "/" + uid + "/config";

    String payload = "{\"name\":\"" + name + "\","
                    "\"state_topic\":\"" + stateTopic + "\","
                    "\"unique_id\":\"" + uid + "\","
                    "\"device\":{\"identifiers\":[\"" + nodeId + "\"],"
                    "\"name\":\"" + deviceName + " " + serial + "\","
                    "\"model\":\"" + deviceName + "\","
                    "\"manufacturer\":\"Stefan Labs\"}";

  if (entityType == "light" && name == "neopixel") {
    payload += ",\"schema\":\"json\",\"brightness\":true,\"rgb\":true";
    payload += ",\"supported_color_modes\":[\"rgb\"],\"color_mode\":true";
  }

  if (commandTopic.length() > 0) {
    payload += ",\"command_topic\":\"" + commandTopic + "\"";
  }

  if (deviceClass.length() > 0) {
    payload += ",\"device_class\":\"" + deviceClass + "\"";
  }

  if (unit.length() > 0) {
    payload += ",\"unit_of_measurement\":\"" + unit + "\"";
  }

  if (valueTemplate.length() > 0) {
    payload += ",\"value_template\":\"" + valueTemplate + "\"";
  }

  payload += "}";

  webserial.println("Publishing discovery:");
  webserial.print("  Topic: ");
  webserial.println(topic);
  webserial.print("  Payload: ");
  webserial.println(payload);

  bool result = client.publish(topic.c_str(), payload.c_str(), true);
  webserial.print("  MQTT publish result: ");
  webserial.println(result ? "✅ success" : "❌ failed");
}


String getUTCTimestamp() {
  time_t now = time(nullptr);
  struct tm* utc = gmtime(&now);
  char ts[30];  // needs a bit more space for +00:00
  strftime(ts, sizeof(ts), "%Y-%m-%dT%H:%M:%S+00:00", utc);
  return String(ts);
}


void HomeAssistant::publishState(uint16_t co2, float temp, float hum) {
  if (!client.connected()) {
    webserial.println("❌ MQTT not connected — reconnecting...");
    connectMQTT();

    if (!client.connected()) {  // ← this had a double negation before
      webserial.println("❌ MQTT failed to reconnect");
      return;
    }

    webserial.println("✅ MQTT successfully reconnected");
  }

  String payload = "{\"co2\":" + String(co2) +
                  ",\"temperature\":" + String(temp, 2) +
                  ",\"humidity\":" + String(hum, 2) +
                  ",\"last_updated\":null}";

  webserial.print("Publishing state to ");
  webserial.print(stateTopic);
  webserial.print(": ");
  webserial.println(payload);

  bool result = client.publish(stateTopic.c_str(), payload.c_str(), true);

  webserial.print("  MQTT publish result: ");
  webserial.println(result ? "✅ success" : "❌ failed");
}


