/* led-candle library by Darryl Hodgins <darrylh@darrylh.ca>
 */

#include "led-candle.h"

/**
 * Get the rgb colour that this candle wants
 */
uint32_t LedCandle::getColour() {
	uint8_t r, g, b;

	getRgb(&r, &g, &b);

	return ((r & 0xff) << 16) | ((g & 0xff) << 8) | b;
}

void LedCandle::getRgb(uint8_t *r, uint8_t *g, uint8_t *b) {
	_updateIndexValue();
	uint8_t index = MAXVAL(MINVAL(_index, LEDCANDLE_BRIGHTNESS_MAX), LEDCANDLE_BRIGHTNESS_BOTTOM);
	*r = index;
	*g = (index >= LEDCANDLE_BRIGHTNESS_MIN) ? ((index * 3) / 8) : ((index * 3.25) / 8);
	*b = 0;
}


/**
 * Get a monochromatic value that represents the brightness of the candle
 */
uint8_t LedCandle::getMonochromaticValue() {
	_updateIndexValue();
	uint8_t index = MAXVAL(MINVAL(_index, LEDCANDLE_BRIGHTNESS_MAX), LEDCANDLE_BRIGHTNESS_BOTTOM);
	return index;
}

void LedCandle::_updateIndexValue() {
	unsigned long current_time = millis();

	switch (_state) {
		case LEDCANDLE_STATE_BRIGHT:
			_flicker_msecs = random(LEDCANDLE_DOWN_MAX_MSECS - LEDCANDLE_DOWN_MIN_MSECS) + LEDCANDLE_DOWN_MIN_MSECS;
			_flicker_start = current_time;
			_index_start = _index_end;
			if ((_index_start > LEDCANDLE_BRIGHTNESS_BOTTOM) && (random(100) < LEDCANDLE_BRIGHTNESS_BOTTOM_PERCENT)) {
				_index_end = random(_index_start - LEDCANDLE_BRIGHTNESS_BOTTOM) + LEDCANDLE_BRIGHTNESS_BOTTOM;
			} else {
				_index_end = random(_index_start - LEDCANDLE_BRIGHTNESS_MIN) + LEDCANDLE_BRIGHTNESS_MIN;
			}
			_state = LEDCANDLE_STATE_DOWN;
			break;
		
		case LEDCANDLE_STATE_DIM:
			_flicker_msecs = random(LEDCANDLE_UP_MAX_MSECS - LEDCANDLE_UP_MIN_MSECS) + LEDCANDLE_UP_MIN_MSECS;
			_flicker_start = current_time;
			_index_start = _index_end;
			_index_end = random(LEDCANDLE_BRIGHTNESS_MAX - _index_start) + LEDCANDLE_BRIGHTNESS_MIN;
			_state = LEDCANDLE_STATE_UP;
			break;

		case LEDCANDLE_STATE_BRIGHT_HOLD:
		case LEDCANDLE_STATE_DIM_HOLD:
			if ((current_time - _flicker_start) >= _flicker_msecs) {
				_state = (_state == LEDCANDLE_STATE_BRIGHT_HOLD) ? LEDCANDLE_STATE_BRIGHT : LEDCANDLE_STATE_DIM;
			}
			break;

		case LEDCANDLE_STATE_UP:
		case LEDCANDLE_STATE_DOWN:
			if ((current_time - _flicker_start) < _flicker_msecs) {
				_index = (_index_start + ((_index_end - _index_start) * (((current_time - _flicker_start) * 1.0) / _flicker_msecs)));
			} else {
				_index = _index_end;
				if (_state == LEDCANDLE_STATE_DOWN) {
					if (random(100) < LEDCANDLE_DIM_HOLD_PERCENT) {
						_flicker_start = current_time;
						_flicker_msecs = random(LEDCANDLE_DIM_HOLD_MAX_MSECS - LEDCANDLE_DIM_HOLD_MIN_MSECS) + LEDCANDLE_DIM_HOLD_MIN_MSECS;
						_state = LEDCANDLE_STATE_DIM_HOLD;
					} else {
						_state = LEDCANDLE_STATE_DIM;
					}
				} else {
					if (random(100) < LEDCANDLE_BRIGHT_HOLD_PERCENT) {
						_flicker_start = current_time;
						_flicker_msecs = random(LEDCANDLE_BRIGHT_HOLD_MAX_MSECS - LEDCANDLE_BRIGHT_HOLD_MIN_MSECS) + LEDCANDLE_BRIGHT_HOLD_MIN_MSECS;
						_state = LEDCANDLE_STATE_BRIGHT_HOLD;
					} else {
						_state = LEDCANDLE_STATE_BRIGHT;
					}
				}
			}
			break;
	}
}
