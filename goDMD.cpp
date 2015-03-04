/*
 * go DMD main.
 *
 *  Created on: 08.02.2015
 *      Author: sr
 *      (c) 2015 by Stefan Rinke
 */

#include <IRremote.h>
#include <Wire.h>
#include <SD.h>
#include <DallasTemperature.h>
#include <RTClib.h>

#include "LEDMatrixPanel.h"
#include "Clock.h"
#include "Animation.h"
#include "Menu.h"

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
#define COLORCHANNELS 1

#define INTERNAL_SD_SELECT 51

#define RECV_PIN CON1_14
#define PIR_PIN CON1_15

LEDMatrixPanel panel( A,B,C,D, CLK, LAT, OE, WIDTH, HEIGHT, PLANES, COLORCHANNELS);

RTC_DS1307 rtc;
Clock clock(panel, rtc);
Menu menu(&panel, &clock,&SD);

IRrecv irrecv(RECV_PIN);
decode_results results;

OneWire oneWire(DS18B20_PIN);          // OneWire Referenz setzen
DallasTemperature sensors(&oneWire);   // DS18B20 initialisieren

Animation animation(SD, panel, clock);

long nextPixel = 0;
int x = 0;
int y = 0;
bool on=false;

#define VERSION "go dmd v1.0.1"

// define a char out hook for printf
extern "C"
{
  void _mon_putc(char s)
  {
   Serial.write(s);
  }
}

void setup() {
	Serial.begin(9600);
	Serial.println("calling panel begin");
	panel.begin();

	panel.setAnimationColor(1);

	// start ir receiver
	irrecv.enableIRIn();
	
	panel.println(VERSION);
	Serial.println("calling wire begin");
	panel.println("start wire ..");
	Wire.begin();
	Serial.println("calling rtc begin");
	panel.println("start rtc ..");
	rtc.begin();
	
	pinMode(INTERNAL_SD_SELECT, OUTPUT);
	Serial.println("calling sd card begin");
	panel.println("start sd card ..");

	if( !SD.begin(INTERNAL_SD_SELECT) ){
		Serial.println("sd card begin failed");
		panel.println("sd card failed");
		delay(12000);
	} else {
		Serial.println("sd card begin success");
		delay(500);
		panel.println("boot ok!");
		panel.println("(c)2015 by Steve");
	}

	if( !animation.begin() ) {
		panel.println("init anis failed");
		panel.println("*.ani not found");
	}
	
	pinMode(PIR_PIN,OUTPUT);

	delay(1000);
	panel.clear();
	//pinMode(PIN_LED1,OUTPUT);
	//rtc.adjust(DateTime(__DATE__, __TIME__));
}


int clockShowTime = 2000; // millis to show clock
// viewing mode (with date or not)
int dateMode = 0;

// TODO no global function this way
void reloadConfig() {
	panel.setAnimationColor(menu.getOption(SET_COLOR_ANI));
	panel.setTimeColor(menu.getOption(SET_COLOR_CLOCK));
	panel.setBrightness(menu.getOption(SET_BRIGHTNESS));
#ifdef _DEBUG
	Serial.print("set time mode: ");Serial.println(menu.getOption(SET_TIME_MODE));
#endif
	clock.setShowSeconds(menu.getOption(SET_TIME_MODE)==1);
	clockShowTime = 1500 * (menu.getOption(SET_TIME_DURATION)+1);
	dateMode = menu.getOption(SET_DATE_MODE);

}

void drawRect(int x, int y, int w, int h, int col ) {
	for(int xi = x; xi < x+w; xi++ ) {
		for( int yi=y; yi < y+h; yi++ ) {
			panel.setPixel(xi,yi,col);
		}
	}
}

void testScreen() {
	drawRect(0,0,32,32,1);
	drawRect(32,0,32,32,2);
	drawRect(64,0,32,32,3);
	while(true);
}

// states
enum State { showTime, showDate, showAni, showMenu, freeze,
	pirNobodyThere,
};

void loop() {
	long now = millis();
	long switchToAni = now + 2000;
	State state = showTime;

	//testScreen();

	clock.on();

	reloadConfig();

	long switchToDate = 0;
	long switchToTime = 0;

	boolean pir = false; // true / high means active

	while(true) {
		long now = millis();

		if( irrecv.decode(&results) ) {
			menu.notifyEvent(results.value);
			irrecv.resume();
		}
		
		// show with blinking led ISR is running
		if( panel.getISRCalls() > 1500 ) {
			panel.resetISRCalls();
			on = !on;
			//digitalWrite(PIN_LED1,on);		
		}

		pir = digitalRead(PIR_PIN);
		if( pir == false && menu.getOption(SET_PIR_MODE) != 0 ) {
			state = pirNobodyThere;
		}

		clock.update(now);
		menu.update(now);

		if( menu.isActive() ) {
			clock.off();
			state = showMenu;
		}

		switch( state ) {
		case showTime:
			if( now > switchToAni ) {
				state = showAni;
				clock.off();
			}
			if( dateMode==1 && now > switchToDate ) {
				state = showDate;
				clock.setIsShowingDate(true);
				switchToTime = now + 2000; // viewing duration date
			}
			clock.update(now);
			break;
		case showDate:
			if( now > switchToAni ) {
				state = showAni;
				clock.off();
			}
			if( now > switchToTime ) {
				switchToDate = switchToAni + 20000; // weit weg
				clock.setIsShowingDate(false);
				state = showTime;
			}
			clock.update(now);
			break;
		case showAni:
			if( animation.update(now) ) { // true means ani finished
				switchToAni = now + clockShowTime; // millis to show clock
				clock.on();
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
			if( pir ) {
				clock.off();
				state = showAni;
			} else {
				int mode = menu.getOption(SET_PIR_MODE);
				if( mode == PIR_NOANI || mode == PIR_DIM ) {
					// TODO leave clock as dimmed
					clock.on();
				} else {
					clock.off();
				}
			}
			break;
		case freeze:

			break;
		}
		
	}

}