#ifndef WrapperJsonServer_h
#define WrapperJsonServer_h

#include "BaseHeader.h"
#include <ArduinoJson.h>
#include <WiFiServer.h>
#include <WiFiClient.h>

#define TCP_BUFFER 512

class WrapperJsonServer {
  public:
    WrapperJsonServer();
    WrapperJsonServer(uint16_t ledCount, uint16_t tcpPort);
    
    void
      begin(void),
      handle(void);

    void
      onLedColorWipe(void(* function) (byte, byte, byte)),
      onClearCmd(void(* function) (void)),
      onEffectChange(void(* function) (Mode, int)),
      setActiveMode(Mode activeMode);
  private:
    void
      handleConnection(boolean newClient),
      readData(void);
  
    void 
      ledColorWipe(byte r, byte g, byte b),
      (* ledColorWipePointer) (byte, byte, byte);
    void 
      clearCmd(void),
      (* clearCmdPointer) (void);
    void 
      effectChange(Mode effect, int interval = 0),
      (* effectChangePointer) (Mode, int);
  
    WiFiServer _tcpServer;
    WiFiClient _tcpClient;
    
    uint16_t _ledCount;
    uint16_t _tcpPort;

    byte* _activeLedColor;
    Mode activeMode;
    const char* modeStr[9] = {"Off", "Hyperion UDP", "Lightning", "Static Color", "Rainbow swirl", "Fire2012", "Rainbow swirl v2", "Rainbow full", "None"};
};

#endif
