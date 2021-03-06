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
#include "utility/bitprims.h"

class LEDMatrixPanel;
class SDClass;
class File;
class DallasTemperature;
class DHT;

struct Digit {
	uint8_t width;
	uint8_t height;
	uint8_t sizeInBytes;
	byte* data;
	byte* mask;
};

class Clock {
public:
	enum Mode { TIME, TIMESEC, DATE, TEMP, HUMI };
	enum Font { Big, Small };

	Clock(LEDMatrixPanel& panel, RTC_DS1307& rtc, SDClass* sd, DallasTemperature* sensor, DHT* dht);
	void update(uint32_t now);
	virtual ~Clock();
	void on();
	// writes actual time (optinally with seconds)
	void writeTime(uint32_t now, byte* buffer = NULL);
	void writeTimeIntern(uint32_t now, byte* buffer=NULL,
			_BS_alu mode = _BS_alu_copy, boolean useMask = false );
	// writes date
	void writeDate(uint32_t now, byte* buffer = NULL);
	// writes temperature
	void writeTemp(float actTemp, byte* buffer = NULL);
	void writeHumi(int actHumi, byte* buffer = NULL);
	// switch clock off
	void writeText(const char* text, int x, int y, Digit* charset, byte* buffer = NULL,
			_BS_alu mode = _BS_alu_copy, boolean useMask=false);

	void formatTime(char* buffer, uint32_t now);

	void off();

	uint32_t getUnixTime();

	boolean begin();

	boolean isClockOn() {
		return active;
	}
	void adjust(DateTime* dt);
	DateTime& getActualTime();
	void setMode( Mode newMode);
	Mode getMode() const { return mode; }
	//void writeDigit(int digit, int xoffset, uint8_t nBytes, byte* buffer = NULL);
	int writeDigit(int digit, int x, int y, Digit* charset,
			byte* buffer=NULL, _BS_alu mode = _BS_alu_copy, boolean useMask=false);
	//int writeDoubleDigit(int digit, int x, byte* buffer);


	Digit* digits;
	Digit* smallDigits;

	void setFont(Font font) {this->font = font;}
	void setXoffset(uint16_t xo){ this->xoffset = xo; }
	void setYoffset(uint16_t yo){ this->yoffset = yo; }

	Font getFont() const {
		return this->font;
	}

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

	void requestFont(uint8_t font) {
		requestedFont = font;
	}

	uint8_t getActualFont() const {
		return actualFont;
	}

	void setTempOffset(int8_t tempOffset) {
		this->tempOffset = tempOffset;
	}

protected:
	void readFont(File* f, Digit* d, int size);
	boolean loadFont(uint8_t);
	boolean freeFont();
	uint8_t bigFontSize;
	uint8_t smallFontSize;
	boolean fontLoaded;
	int8_t tempOffset;

	RTC_DS1307* rtc;
	SDClass* sd;
	LEDMatrixPanel& panel;
	DallasTemperature* sensor;
	DHT* dht;
	uint32_t nextClockRefresh;
	uint32_t nextRtcSync;
	uint32_t nextTempSync;
	float actTemp;
	int actHumi;

	uint32_t lastRtcSync;
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
	uint8_t actualFont; // number of font to display 0 - n
	uint8_t requestedFont;
};

#endif /* CLOCK_H_ */
