/*
 * Clock.h
 *
 *  Created on: 29.01.2015
 *      Author: sr
 */

#ifndef CLOCK_H_
#define CLOCK_H_

#include <WProgram.h>
#include <Wire.h>
#include "RTClib.h"

class LEDMatrixPanel;
class SDClass;
class File;
class DallasTemperature;

struct Digit {
	uint8_t width;
	uint8_t height;
	uint8_t sizeInBytes;
	byte* data;
	byte* mask;
};

class Clock {
public:
	enum Mode { TIME, TIMESEC, DATE, TEMP };
	enum Font { Big, Small };

	Clock(LEDMatrixPanel& panel, RTC_DS1307& rtc, SDClass* sd, DallasTemperature* sensor);
	void update(long now);
	virtual ~Clock();
	void on();
	// writes actual time (optinally with seconds)
	void writeTime(long now, byte* buffer = NULL);
	// writes date
	void writeDate(long now, byte* buffer = NULL);
	// writes temperature
	void writeTemp(float actTemp, byte* buffer = NULL);
	// switch clock off
	void writeText(const char* text, int x, int y, Digit* charset, byte* buffer = NULL);

	void formatTime(char* buffer, long now);

	void off();

	boolean begin();

	boolean isClockOn() {
		return active;
	}
	void adjust(DateTime* dt);
	DateTime& getActualTime();
	void setMode( Mode newMode);
	Mode getMode() const { return mode; }
	//void writeDigit(int digit, int xoffset, uint8_t nBytes, byte* buffer = NULL);
	int writeDigit(int digit, int x, int y, Digit* charset );
	//int writeDoubleDigit(int digit, int x, byte* buffer);

	Digit* digits;
	Digit* smallDigits;

	void setFont(Font font) {this->font = font;}
	void setXoffset(uint16_t xo){ this->xoffset = xo; }
	void setYoffset(uint16_t yo){ this->yoffset = yo; }

	boolean getBlinkingTick() const {
		return blinkingTick;
	}

	void setBlinkingTick(boolean blinkingTick) {
		this->blinkingTick = blinkingTick;
	}

	boolean getHour24() const {
		return hour24;
	}

	void setHour24(boolean hour24) {
		this->hour24 = hour24;
	}

protected:
	void readFont(File* f, Digit* d, int size);

	RTC_DS1307* rtc;
	SDClass* sd;
	LEDMatrixPanel& panel;
	DallasTemperature* sensor;
	long nextClockRefresh;
	long nextRtcSync;
	long nextTempSync;
	float actTemp;
	long lastRtcSync;
	uint8_t brightness;
	DateTime n;
	boolean active;
	// diaplay mode
	Mode mode;
	// clock display location, when small font is used
	uint16_t xoffset;
	uint16_t yoffset;
	// whether to use big font or small font (or even more than one)
	Font font;
	boolean blinkingTick;
	boolean hour24;
};

#endif /* CLOCK_H_ */
