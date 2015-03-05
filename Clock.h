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
	void writeText(char* text, int offset, byte* buffer = NULL);
	void off();

	void clear();

	boolean begin();

	boolean isClockOn() {
		return active;
	}
	void adjust(DateTime* dt);
	DateTime& getActualTime();
	void setMode( Mode newMode);
	Mode getMode() const { return mode; }

protected:
	void readFont(File* f, Digit* d, int size);
	void writeDigit(int digit, int xoffset, uint8_t nBytes, byte* buffer = NULL);
	int writeDoubleDigit(int digit, int x, byte* buffer);

	Digit* digits;
	Digit* smallDigits;

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
	Mode mode;
};

#endif /* CLOCK_H_ */
