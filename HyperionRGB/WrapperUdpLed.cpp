#include "WrapperUdpLed.h"

WrapperUdpLed::WrapperUdpLed(uint16_t ledCount, uint16_t udpPort, UdpProtocol udpProtocol) {
  _udp = WiFiUDP();
  _ledCount = ledCount;
  _udpPort = udpPort;
  _udpProtocol = udpProtocol;

  _bufferSize = _ledCount * 3; //3 bytes per LED

  _udpBuffer = new byte [_bufferSize + 1];
  _udpBuffer[_bufferSize] = 0;

  _opened = false;
}

void WrapperUdpLed::begin(void) {
  if (!_opened) {
    Log.info("Open port %i for UDP with protocol %i...", _udpPort, _udpProtocol);
    if (_udp.begin(_udpPort)) {
      Log.info("success");
      _opened = true;
    } else {
      Log.error("no success");
    }
  }
}
void WrapperUdpLed::stop(void) {
  if (_opened) {
    Log.info("Close port %i for UDP...", _udpPort);
    _udp.stop();
    _opened = false;
  }
}
void WrapperUdpLed::handle(void) {
  int bytes = _udp.parsePacket();
  if (bytes > 0) {
    Log.debug("UDP-Packet received, length: %i", bytes);
    //See @https://hyperion-project.org/wiki/UDP-Device
    if (_udpProtocol == UDP_RAW) {
      handleProtocolRaw(bytes);
    } else if (_udpProtocol == UDP_FRAGMENT) {
      handleProtocolFragment(bytes);
    } else if (_udpProtocol == UDP_TPM2) {
      handleProtocolTPM2(bytes);
    } else { //Fallback
      handleProtocolRaw(bytes);
    }
  }
}

void WrapperUdpLed::handleProtocolRaw(int bytes) {
  //Raw Format, 3 bytes per LED, Full LED set.
  /// {0: R, 1: G, 2: B}
  if (bytes == _bufferSize) {
    _udp.readBytes(_udpBuffer, _bufferSize);
    Log.verbose("Contents: %s", _udpBuffer);
    for (int i = 0; i < _ledCount; i++) {
      updateLed(i, _udpBuffer[i * 3 + 2], _udpBuffer[i * 3 + 0], _udpBuffer[i * 3 + 1]);
    }
    refreshLeds();
  } else {
    Log.debug("Packet size for protocol 0 invalid: expected=%i, actual=%i", _bufferSize, bytes);
  }
}
void WrapperUdpLed::handleProtocolFragment(int bytes) {
  /// 0: Update ID & 0xF
  /// 1: Fragment ?!
  /// 2: First LED ID, high byte / 3: First LED ID, low byte --> int16_t?
  /// 4: {0: R, 1: G, 2: B}
  if (bytes > 4 && bytes-4 <= _bufferSize) {
    _udp.readBytes(_udpBuffer, 4);
    
    byte updateId = _udpBuffer[0] & 0x0F;
    byte fragment = _udpBuffer[1];
    int ledIdStart = 256 * _udpBuffer[2] + _udpBuffer[3]; //Multiply high byte
    int ledIdEnd = ledIdStart + (bytes - 4) / 3;
    
    Log.verbose("updateId: %X, fragment: %X, ledIdStart: %i, ledIdEnd: %i", updateId, fragment, ledIdStart, ledIdEnd);
    if (ledIdEnd <= _ledCount) {
      _udp.readBytes(_udpBuffer, bytes - 4);
      for (int i=0; i<ledIdEnd-ledIdStart; i++) {
        updateLed(ledIdStart+i, _udpBuffer[i * 3 + 0], _udpBuffer[i * 3 + 1], _udpBuffer[i * 3 + 2]);
      }
      refreshLeds();
    } else {
      Log.error("Too many LEDs: expected=%i, actual=%i", _ledCount, ledIdEnd);
    }
  } else {
    Log.error("Packet size too small or to big for protocol 2, size=%i", bytes);
  }
}
void WrapperUdpLed::handleProtocolTPM2(int bytes) {
  //TPM2.net
  /// 0: 0x9C //magic byte
  /// 1: 0xDA //data frame
  /// 2: Data length (LED count * 3), high byte, 3: Data length (LED count * 3), low byte --> int16_t?
  /// 4: Fragment ID
  /// 5: Fragment ID maximum
  /// 6: {0: R, 1: G, 2: B}
  /// 7: 0x36
  if (bytes > 7 && bytes-7 <= _bufferSize) {
    _udp.readBytes(_udpBuffer, 5);
    Log.verbose("Magic byte: %X, Frame type: %X", _udpBuffer[0], _udpBuffer[1]);
    
    if (_udpBuffer[0] == 0x9C && _udpBuffer[1] == 0xDA) {
      _udp.readBytes(_udpBuffer, 3);
      int dataLength = 256 * _udpBuffer[2] + _udpBuffer[3]; //Multiply high byte
      int ledCount = dataLength / 3;
      byte fragmentId = _udpBuffer[4];
      byte fragmentMax = _udpBuffer[5];
      Log.verbose("Data length: %i, LED count: %i, Fragment ID: %i, Fragment Maximum: %i", dataLength, ledCount, fragmentId, fragmentMax);
      size_t len = _udp.readBytes(_udpBuffer, bytes - 5);
      if (_udpBuffer[len-1] == 0x36) {
        if (fragmentMax == 1) {
          for (int i=0; i<ledCount; i++) {
            updateLed(i, _udpBuffer[i * 3 + 0], _udpBuffer[i * 3 + 1], _udpBuffer[i * 3 + 2]);
            refreshLeds();
          }
        } else {
          if (fragmentId == 0) 
            _tpm2LedId = 0;
          if (_tpm2LedId + ledCount <= _ledCount) {
            for (int i=0; i<ledCount-_tpm2LedId; i++) {
              updateLed(i+_tpm2LedId, _udpBuffer[i * 3 + 0], _udpBuffer[i * 3 + 1], _udpBuffer[i * 3 + 2]);
            }
            _tpm2LedId += ledCount;
            if (fragmentMax == fragmentId)
              refreshLeds();
          } else {
            Log.error("Cannot address more leds than configured for protocol 3");
          }
        }
      } else {
        Log.error("Invalid packet structure, magic byte 0x36 at data end for protocol 3 not found.");
      }   
    } else {
      Log.error("Invalid packet structure, magic bytes for protocol 3 not found.");
    }
  } else {
    Log.error("Packet size too small for protocol 3, size=%i", bytes);
  }
}
void WrapperUdpLed::onUpdateLed(void(* function) (int, byte, byte, byte)) {
  updateLedPointer = function;
}
void WrapperUdpLed::updateLed(int id, byte r, byte g, byte b) {
  if (updateLedPointer) {
    updateLedPointer(id, r, g, b);
  }
}

void WrapperUdpLed::onRefreshLeds(void(* function) (void)) {
  refreshLedsPointer = function;
}
void WrapperUdpLed::refreshLeds(void) {
  if (refreshLedsPointer) {
    refreshLedsPointer();
  }
}
