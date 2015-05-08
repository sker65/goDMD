/*
 * LEDMatrixPanel.h
 *
 *  Created on: 29.01.2015
 *      Author: sr
 */

#ifndef LEDMATRIXPANEL_H_
#define LEDMATRIXPANEL_H_
#include "stdheader.h"
/**
 * class to drive led matric panel (china stuff) via HUB08 etc. code is partly copied from Adafruit RGBMatrixPanel
 * see https://github.com/adafruit/RGB-matrix-Panel
 * @author Stefan Rinke (C) 2015
 */
class LEDMatrixPanel {
public:
	LEDMatrixPanel(uint8_t a, uint8_t b, uint8_t c, uint8_t d,
		    uint8_t sclk, uint8_t latch, uint8_t oe,
		    uint8_t width, uint8_t height, uint8_t planes,
		    uint8_t colorChannels, bool useDoubleBuffering);

	virtual ~LEDMatrixPanel();

	volatile uint8_t** getBuffers();

	void enableLEDs();

	void disableLEDs();

	uint8_t getNumberOfBuffers() { return nBuffers; }
	uint16_t getSizeOfBufferInByte() { return buffersize; }

	void updateScreen();

	void begin();

	void clear();
	void clearTime();
	
	void setTimeBrightness(uint8_t bright) { timeBright = bright;}

	void setPixel(uint8_t x, uint8_t y, uint8_t v);
	void drawRect(int x, int y, int w, int h, int col );

	void writeText(const char* text, uint8_t x, uint8_t y, int len);
	void println(const char* text);
	void scrollUp();
	void setTimeColor(uint8_t col);// { timeColor = col; }
	void setAnimationColor(uint8_t col);// { aniColor = col; }

	void inline setupTimer();
	void inline resetTimer();

	uint16_t getHeight() const {
		return height;
	}

	uint16_t getWidth() const {
		return width;
	}

	uint32_t getISRCalls() const {
		return isrCalls;
	}

	void resetISRCalls() {
		isrCalls=0;
	}

	void setBrightness(volatile uint8_t brightness);
	// used for double buffering
	void swap( bool vsync );
	bool bufFree( );

	volatile uint8_t getBrightness() const {
		return brightness;
	}

private:

	void selectPlane(uint8_t p);
	void swapInternal();

	// extended to uint32_t to match 32 bit architecture
	// PORT register pointers, pin bitmasks, pin numbers:
	volatile uint32_t 	*sclkport, *latport, *oeport, *addraport, *addrbport, *addrcport,
			*addrdport;

	uint8_t sclkpin, latpin, oepin, addrapin, addrbpin, addrcpin, addrdpin,
			_sclk, // clock
			_latch, // latch pin
			_oe, // output enable pin
			_a, _b, _c, _d; // row adress pins

	volatile uint8_t timeBright;
	volatile uint8_t **buffptr; // array of back buffers

	volatile bool swapPending;
	volatile uint8_t bufoffset;
	bool useDoubleBuffering;

    uint16_t duration;

	uint8_t planes; // planes / scanning pattern 1/16 scanning means 4 planes, 1/8 means means 3 planes

	uint16_t width;
	uint16_t height;

	uint16_t buffersize; // size in byte of one buffer

	uint8_t nBuffers; // how many buffers in use

	uint8_t colorsChannels; // color channels to use: 1 = monochrom, 2 = RG, 3 = RGB

	char textbuffer[4][16]; // text buffer for text output (boot / menü)
	uint8_t col, row;

//#define NSCAN 10
	uint8_t scan[16];

	volatile uint32_t isrCalls;

	volatile uint8_t plane;

	volatile uint8_t actBuffer;
	volatile uint8_t bIndex;

	volatile uint8_t timeColor;
	volatile uint8_t aniColor;
	volatile uint8_t brightness;
	volatile bool on;

};

#endif /* LEDMATRIXPANEL_H_ */
