#include "WrapperMqttClient.h"
WrapperMqttClient::WrapperMqttClient(void) : _tcpClient(), _client(_tcpClient) {
}
void WrapperMqttClient::begin(void) {
  _client.setServer(CONFIG_MQTT_BROKER_HOST, 1883);
  _client.setCallback([this] (char* topic, byte * payload, unsigned int length) {
    this->callback(topic, payload, length);
  });

}

void WrapperMqttClient::handle(void) {
  if (!_client.connected()) {
    reconnect();
  }
  if (!_client.loop()) {
    this->connect();
  }
}

void WrapperMqttClient::publish(const char* topic, const char* payload) {
  _client.publish(topic, payload, true);
}

void WrapperMqttClient::callback(String topic, byte* message, unsigned int length) {
  Log.info("Message arrived on topic: %s", topic.c_str());
  String messageTemp;

  for (int i = 0; i < length; i++) {
    messageTemp += (char)message[i];
  }
  Log.info("Message: %s", messageTemp.c_str());
  if (topic.equals("ambilight/control/mode")) {
    StaticJsonDocument<TCP_BUFFER> doc;
    deserializeJson(doc, messageTemp.c_str());
    if (!doc.isNull()) {
      JsonObject root = doc.as<JsonObject>();
      String command = root["command"];
      if (command.equals("color")) {
        ledColorWipe(root["color"][0], root["color"][1], root["color"][2]);
      } else if (command.equals("effect")) {
        String effect = root["effect"]["name"];
        for ( const Mode m : allModes ) {
          if (effect.equals(modeStr[m])) {
            effectChange(m);
            break;
          }
        }
      }
    }
  }
}

void WrapperMqttClient::reconnect(void) {
  if (this->connect()) {
    Log.info("connected to broker");
    _client.subscribe("ambilight/control/mode");
  } else {
    Log.error("failed to connect to mqtt broker, rc=%i", _client.state());
  }
}

boolean WrapperMqttClient::connect() {
  boolean successfullyConnected = _client.connect("ambilight", "ambilight/state", 1, true, "DISCONNECTED");
  if (successfullyConnected) {
    _client.publish("ambilight/state", "CONNECTED", true);
  }
}

void WrapperMqttClient::onEffectChange(void(* function) (Mode, int)) {
  effectChangePointer = function;
}
void WrapperMqttClient::effectChange(Mode effect, int interval/* = 0*/) {
  if (effectChangePointer) {
    effectChangePointer(effect, interval);
  }
}


void WrapperMqttClient::onLedColorWipe(void(* function) (byte, byte, byte)) {
  ledColorWipePointer = function;
}
void WrapperMqttClient::ledColorWipe(byte r, byte g, byte b) {
  if (ledColorWipePointer) {
    ledColorWipePointer(r, g, b);
  }
}
