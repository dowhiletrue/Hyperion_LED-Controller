#ifndef WrapperMqttClient_h
#define WrapperMqttClient_h

#include "BaseHeader.h"
#include <WiFiClient.h>
#include <PubSubClient.h>



class WrapperMqttClient {
  public:
    WrapperMqttClient();
    
    void
      begin(void),
      handle(void),
      publish(const char* topic, const char* payload);

  private:
    WiFiClient _tcpClient;
    PubSubClient _client;

    void
      callback(String topic, byte* message, unsigned int length),
      reconnect(void);
};

#endif
