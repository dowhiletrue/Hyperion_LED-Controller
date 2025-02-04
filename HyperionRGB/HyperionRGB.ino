
#include <Thread.h>
#include <ThreadController.h>
#include "BaseHeader.h"

#include "EnhancedThread.h"

#include "LoggerInit.h"

#include "WrapperWiFi.h"
#include "WrapperOTA.h"
#include "WrapperLedControl.h"
#include "WrapperUdpLed.h"
#include "WrapperJsonServer.h"
#include "WrapperMqttClient.h"

#include "WrapperWebconfig.h"
#include <DNSServer.h>

#define LED LED_BUILTIN // LED in NodeMCU at pin GPIO16 (D0) or LED_BUILTIN @Lolin32.
int ledState = LOW;

LoggerInit loggerInit;

WrapperWiFi wifi;
WrapperOTA ota;

WrapperLedControl ledStrip;

WrapperUdpLed udpLed;
WrapperJsonServer jsonServer;
WrapperMqttClient mqttClient;

#ifdef CONFIG_ENABLE_WEBCONFIG
  WrapperWebconfig webServer;
#endif

Mode activeMode = MODE_NONE;
boolean autoswitch;

boolean motionDetected;

ThreadController threadController = ThreadController();
Thread statusThread = Thread();
EnhancedThread animationThread = EnhancedThread();
EnhancedThread resetThread = EnhancedThread();
EnhancedThread motionThread = EnhancedThread();
EnhancedThread blendThread = EnhancedThread();
EnhancedThread apThread = EnhancedThread();

DNSServer dnsServer;

void statusInfo(void) {
  if (ledState == LOW) {
    ledState = HIGH;
  } else {
    ledState = LOW;
    Log.debug("HEAP=%i", ESP.getFreeHeap());
  }
  digitalWrite(LED, ledState);
}


void animationStep() {
  switch (activeMode) {
    case OFF:
      ledStrip.fadeToBlackStep();
      break;
    case LIGHTNING:
      if (motionDetected) {
        ledStrip.blendStep();
      } else {
        ledStrip.fadeToBlackStep();
      }
      break;
    case STATIC_COLOR:
      ledStrip.blendStep();
      break;
    case RAINBOW:
      ledStrip.rainbowStep();
      break;
    case FIRE2012:
      ledStrip.fire2012Step();
      break;
    case RAINBOW_V2:
      ledStrip.rainbowV2Step();
      break;
    case RAINBOW_FULL:
      ledStrip.rainbowFullStep();
      break;
    case HYPERION_UDP:
      ledStrip.blendStep();
      break;
  }
}
void initDark(int interval = 10) {
  animationThread.setInterval(interval);
  ledStrip.initBlend(30);
}
void initStaticColor(int interval = 10) {
  ledStrip.initSolid(static_cast<CRGB>(Config::getConfig()->led.color));
  animationThread.setInterval(interval);
  ledStrip.initBlend(10);
}

void changeMode(Mode newMode, int interval = 0) {
  if (newMode != activeMode) {
    Log.info("Mode changed to %i", newMode);
    activeMode = newMode;
    if (!autoswitch)
      udpLed.stop();
    
    switch (activeMode) {
      case OFF:
      case LIGHTNING:
        initDark();
        break;
      case STATIC_COLOR:
        initStaticColor();
        break;
      case RAINBOW:
      case RAINBOW_V2:
      case RAINBOW_FULL:
        if (interval == 0)
          interval = 500;
        animationThread.setInterval(interval);
        break;
      case FIRE2012:
        if (interval == 0)
          interval = 10;
        animationThread.setInterval(interval);
        break;
      case HYPERION_UDP:
        if (!autoswitch)
          udpLed.begin();
          if (interval == 0)
            interval = 10;
          blendThread.setInterval(interval);
    }
    if (interval > 0)
      Log.debug("Interval set to %ims", interval);
    jsonServer.setActiveMode(activeMode);
  }
}

void updateLed(int id, byte r, byte g, byte b) {
  if (activeMode == HYPERION_UDP) {
    Log.verbose("LED %i, r=%i, g=%i, b=%i", id + 1, r, g, b);
    ledStrip.next_leds[id].setRGB(r, g, b);
  }
}
void refreshLeds(void) {
  if (activeMode == HYPERION_UDP) {
    Log.debug("refresh LEDs");
    ledStrip.initBlend(10);
    blendThread.reset();
    if (autoswitch)
      resetThread.reset();
  } else if (autoswitch) {
    changeMode(HYPERION_UDP);
    Log.info("Autoswitch to HYPERION_UDP");
  }
}

void ledColorWipe(byte r, byte g, byte b) {
  Log.debug("LED color wipe: r=%i, g=%i, b=%i", r, g, b);
  changeMode(STATIC_COLOR);
  ledStrip.initSolid(CRGB(r, g, b));
}


void resetMode(void) {
  Log.info("Reset Mode");
  #ifdef CONFIG_ENABLE_WEBCONFIG
    changeMode(static_cast<Mode>(Config::getConfig()->led.idleMode));
  #else
    changeMode(CONFIG_LED_STANDARD_MODE);
  #endif
  resetThread.enabled = false;
  blendThread.enabled = false;
}


void handleNoMotion(void) {
  if (activeMode == LIGHTNING) {
    initDark();
  }
  mqttClient.publish("ambilight/motion", "false");
  motionDetected = false;
}


void resetApIdle(void) {
  if (wifi.isAPConnected()) {
    Log.info("AP is used, keeping it alive...");
    apThread.reset();
  } else {
    Log.error("Restarting because nobody connected via ap...");
    delay(1000);
    ESP.restart();
  }
}

void initConfig(void) {
  #if defined(CONFIG_OVERWRITE_WEBCONFIG) && defined(CONFIG_ENABLE_WEBCONFIG)
    Config::loadStaticConfig();
  #endif

  const char* ssid;
  const char* password;
  const char* hostname;
  const byte* ip;
  const byte* subnet;
  const byte* dns;
  uint16_t jsonServerPort;
  uint16_t udpLedPort;
  UdpProtocol udpProtocol;
  uint16_t ledCount;

  #ifdef CONFIG_ENABLE_WEBCONFIG
    //TODO Fallback
    ConfigStruct* cfg = Config::getConfig();
    
    ssid = cfg->wifi.ssid;
    password = cfg->wifi.password;
    hostname = cfg->wifi.hostname;
    ip = Config::cfg2ip(cfg->wifi.ip);
    subnet = Config::cfg2ip(cfg->wifi.subnet);
    dns = Config::cfg2ip(cfg->wifi.dns);
    jsonServerPort = cfg->ports.jsonServer;
    udpLedPort = cfg->ports.udpLed;
    udpProtocol = static_cast<UdpProtocol>(cfg->misc.udpProtocol);
    autoswitch = cfg->led.autoswitch;
    ledCount = cfg->led.count;
    
    Log.info("CFG=%s", "EEPROM config loaded");
    Config::logConfig();
  #else
    ssid = CONFIG_WIFI_SSID;
    password = CONFIG_WIFI_PASSWORD;
    hostname = CONFIG_WIFI_HOSTNAME;
    #ifdef CONFIG_WIFI_STATIC_IP
      ip = CONFIG_WIFI_IP;
      subnet = CONFIG_WIFI_SUBNET;
      dns = CONFIG_WIFI_DNS;
    #else
      const byte empty[4] = {0};
      ip = empty;
    #endif
    jsonServerPort = CONFIG_PORT_JSON_SERVER;
    udpLedPort = CONFIG_PORT_UDP_LED;
    udpProtocol = CONFIG_PROTOCOL_UDP;
    autoswitch = CONFIG_LED_HYPERION_AUTOSWITCH;
    ledCount = CONFIG_LED_COUNT;
    
    Log.info("CFG=%s", "Static config loaded");
  #endif
  
  wifi = WrapperWiFi(ssid, password, ip, subnet, dns, hostname);
  udpLed = WrapperUdpLed(ledCount, udpLedPort, udpProtocol);
  jsonServer = WrapperJsonServer(ledCount, jsonServerPort);
}

void handleEvents(void) {
  ota.handle();
  udpLed.handle();
  jsonServer.handle();
  mqttClient.handle();
  #ifdef CONFIG_ENABLE_WEBCONFIG
    if (wifi.isAP())
      dnsServer.processNextRequest();
    if (webServer.handle() && wifi.isAPConnected()) {
      apThread.reset();
    }
  #endif


  int val = digitalRead(D7);
  //low = no motion, high = motion
  if (digitalRead(D7) == HIGH) {
    if (!motionDetected) {
      if (activeMode == LIGHTNING) {
        initStaticColor();
      }
      mqttClient.publish("ambilight/motion", "true");
      motionDetected = true;
    }
    motionThread.reset();
  }
  threadController.run();
}

void setup(void) {
  LoggerInit loggerInit = LoggerInit(115200);
  
  initConfig();
  ota = WrapperOTA();
  ledStrip = WrapperLedControl();

  statusThread.onRun(statusInfo);
  statusThread.setInterval(5000);
  threadController.add(&statusThread);

  animationThread.onRun(animationStep);
  animationThread.setInterval(1000);
  
  resetThread.onRun(resetMode);
  #ifdef CONFIG_ENABLE_WEBCONFIG
    resetThread.setInterval(Config::getConfig()->led.timeoutMs);
  #else
    resetThread.setInterval(CONFIG_LED_STANDARD_MODE_TIMEOUT_MS);
  #endif
  resetThread.enabled = false;
  threadController.add(&resetThread);
  
  motionThread.onRun(handleNoMotion);
  motionThread.setInterval(CONFIG_LED_LIGHTNING_MODE_TIMEOUT_MS);
  threadController.add(&motionThread);

  blendThread.onRun(animationStep);
  blendThread.setInterval(10);
  blendThread.enabled = false;
  threadController.add(&blendThread);

  #ifdef CONFIG_ENABLE_WEBCONFIG
    ledStrip.begin(Config::getConfig()->led.count);
  #else
    ledStrip.begin(CONFIG_LED_COUNT);
  #endif
  resetMode();
  animationStep();

  wifi.begin();
  if (wifi.isAP()) {
    apThread.onRun(resetApIdle);
    apThread.setInterval(60*1000);
    apThread.reset();
    threadController.add(&apThread);

    dnsServer.start(53, "*", IPAddress(192, 168, 4, 1)); //Port 53 - standard port

  } else {
    apThread.enabled = false;
  }

  #ifdef CONFIG_ENABLE_WEBCONFIG
    webServer = WrapperWebconfig();
    webServer.begin();
    ota.begin(Config::getConfig()->wifi.hostname);
  #else
    ota.begin(CONFIG_WIFI_HOSTNAME); 
  #endif

  if (autoswitch || activeMode == HYPERION_UDP)
    udpLed.begin();
    
  udpLed.onUpdateLed(updateLed);
  udpLed.onRefreshLeds(refreshLeds);

  jsonServer.begin();
  jsonServer.onLedColorWipe(ledColorWipe);
  jsonServer.onClearCmd(resetMode);
  jsonServer.onEffectChange(changeMode);
  mqttClient.onLedColorWipe(ledColorWipe);
  mqttClient.onEffectChange(changeMode);
  mqttClient.begin();

  pinMode(LED, OUTPUT);   // LED pin as output.
  Log.info("HEAP=%i", ESP.getFreeHeap());
}

void loop(void) {
  handleEvents();
  switch (activeMode) {
    case RAINBOW:
    case FIRE2012:
    case RAINBOW_V2:
    case RAINBOW_FULL:
    case OFF:
    case LIGHTNING:
    case STATIC_COLOR:
      animationThread.runIfNeeded();
      break;
    case HYPERION_UDP:
      break;
  }
}
