#ifndef BaseHeader_h
#define BaseHeader_h

#include <Arduino.h>
#include <Logging.h>
#include "Config.h"

#define min(a,b) ((a)<(b)?(a):(b))
/*
 #define min(X, Y)                \
 ({ typeof (X) x_ = (X);          \
    typeof (Y) y_ = (Y);          \
    (x_ < y_) ? x_ : y_; })*/

enum Mode { OFF, HYPERION_UDP, LIGHTNING, STATIC_COLOR, RAINBOW, FIRE2012, RAINBOW_V2, RAINBOW_FULL, MODE_NONE };
static const char* modeStr[9] = {"Off", "Hyperion UDP", "Lightning", "Static Color", "Rainbow swirl", "Fire2012", "Rainbow swirl v2", "Rainbow full", "None"};
static const Mode allModes[] = { OFF, HYPERION_UDP, LIGHTNING, STATIC_COLOR, RAINBOW, FIRE2012, RAINBOW_V2, RAINBOW_FULL, MODE_NONE };
enum UdpProtocol { UDP_RAW, UDP_FILLER, UDP_FRAGMENT, UDP_TPM2 };

#endif
