/*
 * go DMD main.
 *
 *  Created on: 08.02.2015
 *      Author: sr
 *      (c) 2015 by Stefan Rinke
 */

#include <IRremote.h>
#include <OneWire.h>
#include <SD.h>
#include <DallasTemperature.h>
#include <RTClib.h>

#include "LEDMatrixPanel.h"
#include "Clock.h"
#include "Animation.h"
#include "Menu.h"
#include "PirHandler.h"
#include "debug.h"

#define DS18B20_PIN CON2_1

#define CLK CON2_13
#define OE CON2_2
#define LAT CON2_12 // schwarz 
#define A CON1_16 // RB0
#define B CON2_16 // RB1
#define C CON2_15 // RB2
#define D CON2_14 // RB3

#define WIDTH 128
#define HEIGHT 32
#define PLANES 16
#define COLORCHANNELS 2

#define INTERNAL_SD_SELECT 51

#define RECV_PIN CON1_14
#define PIR_PIN CON1_15

LEDMatrixPanel panel( A,B,C,D, CLK, LAT, OE, WIDTH, HEIGHT, PLANES,
		COLORCHANNELS, false);

OneWire oneWire(DS18B20_PIN);          // OneWire Referenz setzen
DallasTemperature sensor(&oneWire);   // DS18B20 initialisieren
RTC_DS1307 rtc;

Clock clock(panel, rtc, &SD, &sensor);
Animation animation(SD, panel, clock);

Menu menu(&panel, &clock, &SD);

IRrecv irrecv(RECV_PIN);
decode_results results;

PirHandler pir(PIR_PIN);

long nextPixel = 0;
int x = 0;
int y = 0;
bool on=false;

#define VERSION "goDmd v1.01-rc1"

// define a char out hook for printf
extern "C"
{
  void _mon_putc(char s)
  {
   Serial.write(s);
  }
}

void waitBut() {
	panel.println("press button");
	while(digitalRead(PIN_BTN1)==LOW);
	while(digitalRead(PIN_BTN1)==HIGH);
}

void selftest() {
	panel.println("SELFTEST");
	panel.println(VERSION);
	waitBut();
	panel.println("PANELTEST");
	int col = 0;
	int br = 1;
	while( true ){
		panel.setAnimationColor(col);
		panel.drawRect(0,0,128,32,br++);
		if( br == 4) { //  4
			br = 1;
			col++;
			if( col == 3) col = 0;
		}
		delay(700);
		if( digitalRead(PIN_BTN1)==FALSE ) break;
	}
	panel.println("press but to cont");
	waitBut();
	//panel test
	panel.println("IR TEST");
	panel.println("press buttons");
	// loop to receive
	if( irrecv.decode(&results) ) {
		char buf[12];
		sprintf(buf,"0x%08x",results.value);
		panel.println(buf);
		irrecv.resume();
	}


}


/*void plotPoints() {
	int v = 1;
	while( true ) {
		for( y = 0; y<32;y++) {
			for( int x = 0; x<128;x++) {
				panel.setPixel(x,y,v);
				delay(5);
			}
		}
		v ^= 1;
	}
}*/

void setup() {
	Serial.begin(9600);
#ifdef _DxEBUG
	for( int i =0; i<5;i++) {
		DPRINTF("wait %lu \n", millis());
		delay(1000);
	}
#endif
	DPRINTF("calling panel begin\n");
	panel.begin();

	panel.setAnimationColor(1);
	panel.setTimeColor(1);

	// start ir receiver
	irrecv.enableIRIn();

	panel.println(VERSION);

	DPRINTF("calling wire begin\n");
	panel.println("start wire ..");
	Wire.begin();
	DPRINTF("calling rtc begin\n");
	panel.println("start rtc ..");
	rtc.begin();
	
	pinMode(INTERNAL_SD_SELECT, OUTPUT);
	DPRINTF("calling sd card begin\n");
	panel.println("start sd card ..");

	if( !SD.begin(INTERNAL_SD_SELECT) ){
		DPRINTF("sd card begin failed\n");
		panel.println("sd card failed");
		selftest();
	} else {
		DPRINTF("sd card begin success\n");
		delay(500);
		panel.println("boot ok!");
		panel.println("(c)2015 by Steve");
	}

	if( !clock.begin() ) {
		panel.println("init clock fail");
		panel.println("font not found");
	}

	if( !animation.begin() ) {
		panel.println("init anis fail");
		panel.println("*.ani not found");
	}

	sensor.begin();
	DPRINTF("sensors found: %d\n",sensor.getDeviceCount());

	if(digitalRead(PIN_BTN1)==FALSE) {
		selftest();
	}

	panel.clear();
	panel.clearTime();

	//pinMode(PIN_LED2,OUTPUT);

	//rtc.adjust(DateTime(__DATE__, __TIME__));
}

int clockShowTime = 2000; // millis to show clock
// viewing mode (with date or not)
int dateMode = 0;
int tempMode = 0;

// TODO no global function this way
void reloadConfig() {
	panel.setAnimationColor(menu.getOption(SET_COLOR_ANI)+1);
	panel.setTimeColor(menu.getOption(SET_COLOR_CLOCK));
	panel.setBrightness(menu.getOption(SET_BRIGHTNESS));

	clock.setMode(menu.getOption(SET_TIME_MODE)==1?Clock::TIMESEC:Clock::TIME);
	DPRINTF("set time mode: %d\n", menu.getOption(SET_TIME_MODE));

	clockShowTime = 1500 * (menu.getOption(SET_TIME_DURATION)+1);
	dateMode = menu.getOption(SET_DATE_MODE);
	tempMode = menu.getOption(SET_TEMP_MODE);
	animation.setFskMode(menu.getOption(FSK_MODE));
	animation.setRandomOrder(menu.getOption(ANIORDER_MODE)==1);
	clock.setHour24(menu.getOption(H2412_MODE)==MODE_24H);
	clock.setBlinkingTick(menu.getOption(BLINK_MODE)==BLINK_ON);
	clock.requestFont(menu.getOption(CURRENT_FONT));
}

void testScreen() {
	for( int col = 0; col<16; col++) {
		panel.drawRect(col*8,0,8,32,col);
	}
	while(true);
}

// push main loop behaviour to class

// states
enum State { showTime, showDate, showAni, showMenu, freeze,
	pirNobodyThere,
};


void loop() {
	long now = millis();
	long switchToAni = 0;
	State state = showTime;

	//testScreen();

	menu.loadOptions();
	reloadConfig();

	switchToAni = now + clockShowTime;

	long switchToDate = 0;
	long switchToTime = 0;
	int dateShowCount = 0;

	uint32_t ledInterval = 3000;

	while(true) {
		long now = millis();

		if( irrecv.decode(&results) ) {
			menu.notifyEvent(results.value);
			irrecv.resume();
		}
		
		// show with blinking led ISR is running
		if( panel.getISRCalls() > ledInterval ) {
			panel.resetISRCalls();
			on = !on;
			//digitalWrite(PIN_LED2,on);
		}

		pir.update(now);
		clock.update(now);
		menu.update(now);

		if( menu.isActive() ) {
			clock.off();
			state = showMenu;
		}

		switch( state ) {
		case showTime:
			clock.on();
			if( now > switchToAni ) {
				if(pir.somebodyHere()) {
					state = showAni;
					clock.off();
					panel.setBrightness(menu.getOption(SET_BRIGHTNESS));
				} else {
					switch( menu.getOption(SET_PIR_MODE) ) {
					case PIR_DIM:
						panel.setBrightness(1);
						// run through is intended
					case PIR_INACTIVE:
						state = showAni;
						clock.off();
						break;
					case PIR_NOANI:
						switchToAni = now + clockShowTime;
						// TODO check date show
						// define transitions as methods
						break;
					case PIR_SWITCHOFF:
						clock.off();
						panel.clear();
						state = pirNobodyThere;
						break;
					}
				}
			}
			if( dateMode==0 && now > switchToDate ) {
				state = showDate;
				if( tempMode == 0 && dateShowCount > 2) {
					dateShowCount=0;
					clock.setMode(Clock::TEMP);
				} else {
					clock.setMode(Clock::DATE);
				}
				switchToTime = now + 2000; // viewing duration date
				dateShowCount++;
			}
			else if( tempMode == 0 && now > switchToDate ) {
				state = showDate;
				clock.setMode(Clock::TEMP);
				switchToTime = now + 2000;
			}
			clock.update(now);
			break;
		case showDate:
			if( now > switchToAni ) {
				state = showAni;
				clock.off();
			}
			if( now > switchToTime ) {
				switchToDate = switchToAni + 50000; // weit weg
				clock.setMode(menu.getOption(SET_TIME_MODE)==1?Clock::TIMESEC:Clock::TIME);
				state = showTime;
			}
			clock.update(now);
			break;
		case showAni:
			if( animation.update(now) ) { // true means ani finished
				switchToAni = now + clockShowTime; // millis to show clock
				state = showTime;
				if( dateMode == 1 ) {
					switchToDate = now + clockShowTime/2; // nach der halben Zeit kommt das Datum
				}
			}
			break;
		case showMenu:
			if( !menu.isActive()) {
				// menu is finish / clock is set, but reconfige ani / panel
				reloadConfig();
				state = showTime;
			}
			break;
		case pirNobodyThere:
			ledInterval = 6000;
			if( pir.somebodyHere()) {
				ledInterval = 3000;
				switchToAni = now + clockShowTime; // millis to show clock
				state = showTime;
				if( dateMode == 1 ) {
					switchToDate = now + clockShowTime/2; // nach der halben Zeit kommt das Datum
				}
			}
			break;
		case freeze:

			break;
		}
		
	}
}

