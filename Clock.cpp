/*
 * Clock.cpp
 *
 *  Created on: 29.01.2015
 *      Author: sr
 */

#include "Clock.h"
#include "LEDMatrixPanel.h"
#include "DallasTemperature.h"
#include "debug.h"
#include <SD.h>
#include "utility/bitprims.h"


#define maxBrightness 2
//#define clockPlane 2


Clock::Clock(LEDMatrixPanel& p, RTC_DS1307& rtc, SDClass* sd,
		DallasTemperature* sensor) :
		panel(p), nextClockRefresh(0), nextRtcSync(0), nextTempSync(0),
		sd(sd), sensor(sensor),
		actTemp(0.0) {
	this->rtc = &rtc;
	brightness = 0;
	active = false;
	mode = TIME;
	lastRtcSync = 0;
	digits = 0L;
	nextTempSync = 0;
	blinkingTick = true;
	hour24 = true;
	actualFont = 0;
	requestedFont = 0;
	bigFontSize=0;
	smallFontSize=0;
	fontLoaded = false;
}

Clock::~Clock() {
}

void Clock::readFont(File* f, Digit* p, int _size) {
	int j = 0;
	int size = _size;
	while(size-- > 0) {
		int ch = f->read(); // ignore char
		p[j].width = f->read();
		p[j].height = f->read();
		p[j].sizeInBytes = f->read()*256+f->read();
		DPRINTF("reading char '%c': w:%d, h:%d, bytes:%d\n",
				ch,p[j].width,p[j].height,p[j].sizeInBytes);
		// create struct for font data
		p[j].data = (byte*)malloc(p[j].sizeInBytes);
		f->read(p[j].data,p[j].sizeInBytes);
		j++;
	}
	// now read mask
	j = 0;
	size = f->read();
	if( _size != size ) {
		DPRINTF("size of mask does not match data!!\n");
	}
	while(size-- > 0) {
		f->read(); // ignore char
		f->seekCur(4); // skip dimensions
		p[j].mask = (byte*)malloc(p[j].sizeInBytes);
		f->read(p[j].mask,p[j].sizeInBytes);
		j++;
	}

}

/** free memory used by current font.*/
boolean Clock::freeFont() {
	if( fontLoaded ) {
		DPRINTF("free front\n");
		for( int i = smallFontSize-1; i>=0; i--) {
			free(smallDigits[i].data);
		}
		for( int i = bigFontSize-1; i>=0; i--) {
			free(digits[i].data);
		}
		free(smallDigits);
		free(digits);
	}
	return true;
}

/**
 * load font from file 'fontX.dat' where X is 0 .. n
 */
boolean Clock::loadFont(uint8_t n) {
	char buf[10];
	sprintf(buf,"font%d.dat",n);
	File f = sd->open(buf);
	if( !f ) return false;
	DPRINTF("reading %s\n",buf);
	// size of map -> how many chars
	bigFontSize = f.read();
	DPRINTF("big font uses %d chars\n",bigFontSize);
	digits = (Digit*)malloc(sizeof(Digit)*bigFontSize);
	readFont(&f, digits, bigFontSize);
	smallFontSize = f.read();
	smallDigits = (Digit*)malloc(sizeof(Digit)*smallFontSize);
	DPRINTF("small font uses %d chars\n",smallFontSize);
	readFont(&f, smallDigits, smallFontSize);
	f.close();
	fontLoaded=true;
	return true;
}

boolean Clock::begin() {
	return loadFont(actualFont);
}

void Clock::setMode( Mode newMode ) {
	if( this->mode != newMode ) {
		panel.clearTime();
		this->mode = newMode;
	}
}

void Clock::adjust(DateTime* dt) {
	DPRINTF("adjust clock: %d:%d:%d\n", dt->hour(), dt->minute(), dt->second());
	rtc->adjust(*dt);
	nextRtcSync = millis(); // force sync
}

/**
 * updates the clock cyclic
 * @param now actual time in millis
 */
void Clock::update(long now) {
	if (nextRtcSync < now) {
		nextRtcSync = now + 10L * 60L * 1000L;  // 10 min
		n = rtc->now();
		lastRtcSync = millis();
	}
	if ( nextClockRefresh < now) {
		nextClockRefresh = now + 500;
		switch (mode) {
		case TIME:
		case TIMESEC:
			writeTime(now);
			break;
		case DATE:
			writeDate(now);
			break;
		case TEMP:
			if( nextTempSync < now ) {
				sensor->requestTemperatures();
				actTemp= sensor->getTempCByIndex(0);
				nextTempSync = now + 40 * 1000L; // every 40 sec
			}
			writeTemp(actTemp);
			break;
		}
		// by forcing updates max bright is reached faster
		if (brightness < maxBrightness) {
			brightness++;
			nextClockRefresh = now + 200;
			panel.setTimeBrightness(brightness);
		}
	}
}

#define DIGITS "0123456789: .C*"

void dumpDigit(Digit* d) {
	DPRINTF("w: %d, h: %d, size: %d\n", d->width, d->height, d->sizeInBytes);
	DPRINTF("data: \n");
	for(int i= 0; i<d->sizeInBytes; i++) {
		DPRINTF("0x%02x, ", d->data[i] );
		if( (i % 16) == 0 ) {
			DPRINTF("\n");
		}
	}
}

// default mode _BS_alu_copy
// mask mode is _BS_alu_andReverse
int Clock::writeDigit(int digit, int x, int y, Digit* charset, byte* buffer, _BS_alu mode, boolean useMask ) {
	Digit d = charset[digit];
	//DPRINTF("write digit: %d, pos: %d\n", digit, x);
	int srow = 0;
	for (int row = y; row < min(y+d.height,panel.getHeight()); row++) {
		volatile uint8_t* src = (useMask ? d.mask : d.data) + srow++ * (d.sizeInBytes/d.height);
		volatile uint8_t* pdest = (buffer==NULL ? panel.getBuffers()[2] : buffer)
		                                             + (row*(panel.getWidth()/8)) + x/8;
		_BS_blt( mode, (volatile uint8_t* )pdest, x%8,
				(volatile uint8_t* )src, 0, d.width);
	}
	return x+d.width;
}

void Clock::writeText(const char* text, int x, int y, Digit* charset,
		byte* buffer, _BS_alu alumode, boolean useMask ) {
	const char* p = text;
	const char* b = DIGITS;
	int xo = x;
	while(*p) {
		char* q = strchr(b,*p);
		if( q ) {
			//DPRINTF("char: %ld\n", q-b );
			// breite in bytes = digits[q-p].width / 4;
			xo = writeDigit(q-b,xo,y,charset, buffer, alumode, useMask);
		}
		p++;
	}
}


void Clock::writeTemp(float actTemp, byte* buffer) {
	if (active) {
		char buf[7];
		sprintf(buf, "%03.1f *C",actTemp+(float)tempOffset);
		DPRINTF("showing temp: %s\n",buf);
		writeText(buf,20,0,digits,buffer);
	}
}

void Clock::writeDate(long now, byte* buffer) {
	if (active) {
		DateTime dateTime(n.unixtime() + (now - lastRtcSync) / 1000);
		char date[10];
		sprintf(date,"%02d.%02d.%02d",
				dateTime.day(),dateTime.month(),dateTime.year() % 1000);
		writeText(date,10,0,digits,buffer);
	}
}

void Clock::formatTime(char* time, long now) {
	DateTime dateTime(n.unixtime() + ( (now - lastRtcSync) / 1000 ));
	boolean tick = !blinkingTick ? true : (now % 1000) > 500;
	int hour = dateTime.hour();
	if( !hour24 ) {
		if( hour == 0 ) hour = 12;
		else if( hour > 12 ) hour -= 12;
	}
	if (mode==TIME || font == Small) { // small display always without seconds
		sprintf(time,tick?"%02d:%02d":"%02d %02d",
				hour,dateTime.minute());
	} else {
		sprintf(time,tick?"%02d:%02d:%02d":"%02d %02d %02d",
				hour,dateTime.minute(),dateTime.second());
	}
}

void Clock::writeTimeIntern(long now, byte* buffer, _BS_alu alumode, boolean useMask) {
	char time[10];
	formatTime(time,now);
	switch(font) {
	case Big:
		writeText(time,(mode==TIMESEC) ? 10 : 28,0,digits, buffer, alumode, useMask);
		break;
	case Small:
		writeText(time,xoffset,yoffset,smallDigits, buffer, alumode, useMask);
		break;
	}
}

void Clock::writeTime(long now, byte* buffer) {
	if (active) {
		writeTimeIntern(now,buffer);
	}
}


void Clock::off() {
	if (active) {
		active = false;
		panel.setTimeBrightness(0);
		panel.clearTime();
	}
}

void Clock::on() {
	if (!active) {
		if( requestedFont != actualFont ) {
			DPRINTF("Clock::on: req: %d, act: %d\n",requestedFont,actualFont);
			freeFont();
			if( loadFont(requestedFont) ) {
				actualFont = requestedFont;
			} else {
				actualFont = requestedFont = 0;
				loadFont(requestedFont);
			}
			DPRINTF("Clock::on: loaded %d\n",actualFont);
		}
		active = true;
		brightness = 0;
		update(millis());
	}
}

DateTime& Clock::getActualTime() {
	DateTime p(n.unixtime() + (millis() - lastRtcSync) / 1000);
	return p;
}
