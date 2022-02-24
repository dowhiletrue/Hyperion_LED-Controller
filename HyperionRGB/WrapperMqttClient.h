#ifndef WrapperMqttClient_h
#define WrapperMqttClient_h

#include "BaseHeader.h"
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define TCP_BUFFER 512



class WrapperMqttClient {
  public:
    WrapperMqttClient();
    
    void
      begin(void),
      handle(void),
      publish(const char* topic, const char* payload),
      onEffectChange(void(* function) (Mode, int)),
      onLedColorWipe(void(* function) (byte, byte, byte));

  private:
    WiFiClient _tcpClient;
    PubSubClient _client;

    void
      callback(String topic, byte* message, unsigned int length),
      reconnect(void),
      effectChange(Mode effect, int interval = 0),
      (* effectChangePointer) (Mode, int),
      ledColorWipe(byte r, byte g, byte b),
      (* ledColorWipePointer) (byte, byte, byte);

    boolean
      connect();
};

#endif
