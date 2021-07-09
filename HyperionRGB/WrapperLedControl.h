#ifndef WrapperLedControl_h
#define WrapperLedControl_h

#include "BaseHeader.h"

#include <FastLED.h>

class WrapperLedControl {
  public:
    void
      begin(uint16_t ledCount),
      show(void),
      initBlend(void),
      initSolid(CRGB color),
      clear(void),
      fillSolid(CRGB color),
      fillSolid(byte r, byte g, byte b),
      fadeToBlackStep(void),
      blendStep(void),
      rainbowStep(void),
      rainbowV2Step(void),
      rainbowFullStep(void),
      fire2012Step(void);

    CRGB* leds;
    CRGB* next_leds;
      
  private:       
    CRGB wheel(byte wheelPos);
    byte _rainbowStepState;
    uint16_t _rainbowV2StepState;
    boolean _fire2012Direction;
    byte* _fire2012Heat;
    uint16_t _ledCount;
    uint8_t blend_step;
    uint8_t blend_steps;
    CRGB* _previous_leds;
};

#endif
