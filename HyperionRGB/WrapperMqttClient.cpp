#include "WrapperMqttClient.h"
WrapperMqttClient::WrapperMqttClient(void) : _tcpClient(), _client(_tcpClient) {
}
void WrapperMqttClient::begin(void) {
  _client.setServer(CONFIG_MQTT_BROKER_HOST, 1883);
  _client.setCallback([this] (char* topic, byte* payload, unsigned int length) { this->callback(topic, payload, length); });
  
}

void WrapperMqttClient::handle(void) {
  if (!_client.connected()) {
    reconnect();
  }
  if(!_client.loop()) {
    _client.connect("ESP8266Client");
  }
}

void WrapperMqttClient::publish(const char* topic, const char* payload) {
  _client.publish(topic, payload);
}

void WrapperMqttClient::callback(String topic, byte* message, unsigned int length) {
  Log.info("Message arrived on topic: %s", topic.c_str());
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    messageTemp += (char)message[i];
  }
  Log.info("Message: %s", messageTemp.c_str());
}

void WrapperMqttClient::reconnect(void) {
  if (_client.connect("ESP8266Client")) {
      Log.info("connected to broker");  
      _client.subscribe("ambilight/control");
    } else {
      Log.error("failed to connect to mqtt broker, rc=%i", _client.state());
    }
}