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

class Clock {
public:
	Clock(LEDMatrixPanel& panel, RTC_DS1307& rtc);
	void update(long now);
	virtual ~Clock();
	void on();
	void writeTime(long now, byte* buffer = NULL);
	void writeDate(long now, byte* buffer = NULL);
	void off();
	void clear();
	boolean isClockOn() {
		return active;
	}
	void adjust(DateTime* dt);
	DateTime& getActualTime();
	void setShowSeconds(boolean show);

	void setIsShowingDate(boolean isShowingDate);

	boolean getIsShowingDate() const {
		return isShowingDate;
	}

protected:

	void writeDigit(int digit, int xoffset, uint8_t nBytes, byte* buffer = NULL);
	int writeDoubleDigit(int digit, int x, byte* buffer);
	RTC_DS1307* rtc;
	LEDMatrixPanel& panel;
	long nextClockRefresh;
	long nextRtcSync;
	long lastRtcSync;
	uint8_t brightness;
	DateTime n;
	boolean active;
	boolean showSeconds;
	boolean isShowingDate;

};

#endif /* CLOCK_H_ */
