#pragma once

#ifdef SPARK_PLATFORM
#include "Particle.h"
#else
#ifndef ARDUINO
#include <stdint.h>
#elif ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
#endif

// States for the LED's finite state machine
#define LEDCANDLE_STATE_BRIGHT 0
#define LEDCANDLE_STATE_UP 1
#define LEDCANDLE_STATE_DOWN 2
#define LEDCANDLE_STATE_DIM 3
#define LEDCANDLE_STATE_BRIGHT_HOLD 4
#define LEDCANDLE_STATE_DIM_HOLD 5

// Percent chance the LED will suddenly fall to minimum brightness
#ifndef LEDCANDLE_BRIGHTNESS_BOTTOM_PERCENT
#define LEDCANDLE_BRIGHTNESS_BOTTOM_PERCENT 10
#endif

// Absolute minimum red value (green value is a function of red's value)
#ifndef LEDCANDLE_BRIGHTNESS_BOTTOM
#define LEDCANDLE_BRIGHTNESS_BOTTOM 64
#endif

// Minimum red value during "normal" flickering (not a dramatic change)
#ifndef LEDCANDLE_BRIGHTNESS_MIN
#define LEDCANDLE_BRIGHTNESS_MIN 64
#endif

// Maximum red value
#ifndef LEDCANDLE_BRIGHTNESS_MAX
#define LEDCANDLE_BRIGHTNESS_MAX 128
#endif

// Decreasing brightness will take place over a number of milliseconds in this range
#ifndef LEDCANDLE_DOWN_MIN_MSECS
#define LEDCANDLE_DOWN_MIN_MSECS 20
#endif

#ifndef LEDCANDLE_DOWN_MAX_MSECS
#define LEDCANDLE_DOWN_MAX_MSECS 250
#endif

// Increasing brightness will take place over a number of milliseconds in this range
#ifndef LEDCANDLE_UP_MIN_MSECS
#define LEDCANDLE_UP_MIN_MSECS 20
#endif

#ifndef LEDCANDLE_UP_MAX_MSECS
#define LEDCANDLE_UP_MAX_MSECS 250
#endif

// Percent chance the color will hold unchanged after brightening
#ifndef LEDCANDLE_BRIGHT_HOLD_PERCENT
#define LEDCANDLE_BRIGHT_HOLD_PERCENT 20
#endif

// When holding after brightening, hold for a number of milliseconds in this range
#ifndef LEDCANDLE_BRIGHT_HOLD_MIN_MSECS
#define LEDCANDLE_BRIGHT_HOLD_MIN_MSECS 0
#endif

#ifndef LEDCANDLE_BRIGHT_HOLD_MAX_MSECS
#define LEDCANDLE_BRIGHT_HOLD_MAX_MSECS 100
#endif

// Percent chance the color will hold unchanged after dimming
#ifndef LEDCANDLE_DIM_HOLD_PERCENT
#define LEDCANDLE_DIM_HOLD_PERCENT 5
#endif

// When holding after dimming, hold for a number of milliseconds in this range
#ifndef LEDCANDLE_DIM_HOLD_MIN_MSECS
#define LEDCANDLE_DIM_HOLD_MIN_MSECS 0
#endif

#ifndef LEDCANDLE_DIM_HOLD_MAX_MSECS
#define LEDCANDLE_DIM_HOLD_MAX_MSECS 50
#endif

#define MINVAL(A,B)             (((A) < (B)) ? (A) : (B))
#define MAXVAL(A,B)             (((A) > (B)) ? (A) : (B))

// This is your main class that users will import into their application
class LedCandle {
	private:
		uint8_t _state = LEDCANDLE_STATE_BRIGHT;
		uint16_t _index = 255;
		unsigned long _flicker_msecs;
		unsigned long _flicker_start;
		uint8_t _index_start = 255;
		uint8_t _index_end = 255;

		/**
		 * Do the math
		 */
		void _updateIndexValue();

	public:
		/**
		 * Constructor
		 */
		LedCandle() {};

		uint32_t getColour();

		void getRgb(uint8_t* r, uint8_t* g, uint8_t* b);

		uint8_t getMonochromaticValue();
};
