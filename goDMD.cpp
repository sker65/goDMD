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
#include <RTClib.h>
#include <DallasTemperature.h>

#include "NtpCallbackHandler.h"
#include "LEDMatrixPanel.h"
#include "Clock.h"
#include "Animation.h"
#include "Menu.h"
#include "PirHandler.h"
#include "NodeCallbackHandler.h"
#include "debug.h"
#include "version.h"
#include <malloc.h>
#include "macros.h"

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

// set time zone to 1 as default for germany
NtpCallbackHandler ntpCallbackHandler(&clock, 1);
NodeCallbackHandler nodeCallbackHandler(&panel);

NodeMcu node(&ntpCallbackHandler,&nodeCallbackHandler);

Menu menu(&panel, &clock, &SD, &node);

IRrecv irrecv(RECV_PIN);
decode_results results;

PirHandler pir(PIR_PIN);

long nextPixel = 0;
int x = 0;
int y = 0;
bool on=false;


extern "C" char* sbrk(int incr);
int getFreeRam() {
  char top;
  return &top - reinterpret_cast<char*>(sbrk(0));
}

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

#include "ir-codes.h"

void selftest() {
	panel.println("SELFTEST");
	panel.println(VERSION);
	waitBut();
	panel.println("PANELTEST");
	int b = 0;
	boolean run = true;
	boolean seenHigh = false;
	int col = 0;
	int br = 3;
	panel.setAnimationColor(col);
	panel.drawRect(0,0,128,32,br++);
	while( run) {
		if( irrecv.decode(&results) ) {
			if( results.value == BUT_7 && b>0 ) {
				b--;
			}
			if( results.value == BUT_8 && b<10 ) {
				b++;
			}
			if( results.value == BUT_9 ) run = false;
			if( results.value == BUT_1 ) {
				DPRINTF("draw col=%d br=%d\n",col,br);
				panel.setAnimationColor(col);
				panel.drawRect(0,0,128,32,br++);
				if( br >= 4) { //  4
					br = 1;
					col++;
					if( col == 3) col = 0;
				}
			}
			panel.setBrightness(b);
			DPRINTF("set Brightness to %d\n",b);
			irrecv.resume();
		}
		if( digitalRead(PIN_BTN1)==HIGH ) seenHigh = true;
		if( seenHigh && digitalRead(PIN_BTN1)==LOW ) run=false;
	}

	// one pager
	run = true;
	seenHigh = false;
	long update = 0;
	uint32_t lastIr = 0;
	panel.clear();
	while(run) {
		uint32_t now = millis();
		// just hand over
		if( Serial.available() ) {
			int in = Serial.read();
			Serial1.write(in);
		}
		if( Serial1.available()) {
			int out = Serial1.read();
			Serial.write(out);
		}
		if( SAVECMP(now, update) ) {
			update = now+1000;
			if( pir.actual() ) panel.writeText("PIR: true   ",0,0,12);
			else panel.writeText("PIR: false  ",0,0,12);
			char buf[16];
			sprintf(buf,"0x%08x",lastIr);
			panel.writeText(buf,0,8,10);
			sensor.requestTemperatures();
			float actTemp = sensor.getTempCByIndex(0);
			sprintf(buf, "%03.1f *C",actTemp);
			panel.writeText(buf,0,16,7);
			DateTime n = rtc.now();
			sprintf(buf,"%02d:%02d:%02d %02d%02d%02d",
					n.hour(),n.minute(),n.second(),
					n.day(),n.month(),n.year()%1000);
			panel.writeText(buf,0,24,15);
		}
		if( irrecv.decode(&results) ) {
			lastIr = results.value;
			DPRINTF("ircode received %04x\n",lastIr);
			irrecv.resume();
			if( results.value == BUT_9 ) run = false;
		}
		if( digitalRead(PIN_BTN1)==HIGH ) seenHigh = true;
		if( seenHigh && digitalRead(PIN_BTN1)==LOW ) run=false;
	}
	run = true;
	seenHigh = false;
	panel.clear();
	node.start();
	node.setEnableBackgroundCmds(false);
	while(run) {
		uint32_t now = millis();
		node.update(now);
		if( SAVECMP(now,update) ) {
			update = now+1000;
			if( node.isNodeMcuDetected()) {
				panel.writeText("wifi ext active",0,0,15);
				char* ip = node.syncRequestIp();
				char buf[16];
				sprintf(buf, "%-16s",ip);
				panel.writeText(buf,0,8,16);
			} else {
				panel.writeText("no wifi ext.",0,0,12);
			}


		}
		if( digitalRead(PIN_BTN1)==HIGH ) seenHigh = true;
		if( seenHigh && digitalRead(PIN_BTN1)==LOW ) run=false;
	}
	panel.println("press but to cont");
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


void setup() {
	Serial.begin(9600);
#ifdef _DXEBUG
	for( int i =0; i<5;i++) {
		DPRINTF("wait %lu \n", millis());
		delay(1000);
	}
#endif
	DPRINTF2("calling panel begin\n");
	panel.begin();

	panel.setAnimationColor(1);
	panel.setTimeColor(1);
	DPRINTF2("\n");

	node.begin();

	// start ir receiver
	irrecv.enableIRIn();

	panel.println(VERSION);

	DPRINTF2("calling wire begin\n");
	panel.println("start wire ..");
	Wire.begin();
	DPRINTF2("calling rtc begin\n");
	panel.println("start rtc ..");
	rtc.begin();
	
	DPRINTF2("calling sd card begin\n");
	panel.println("start sd card ..");

	if( !SD.begin(INTERNAL_SD_SELECT) ){
	pinMode(INTERNAL_SD_SELECT, OUTPUT);
		DPRINTF2("sd card begin failed\n");
		panel.println("sd card failed");
		selftest();
	} else {
		DPRINTF2("sd card begin success\n");
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

	uint32_t pclock = getPeripheralClock();
	DPRINTF("system running with periphal clock : %ld\n",pclock);

	sensor.begin();
	DPRINTF("sensors found: %d\n",sensor.getDeviceCount());

//	DPRINTF("Running at: %d\n", System.get());

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
	panel.setAnimationColor(menu.getOption(SET_COLOR_ANI));
	panel.setTimeColor(menu.getOption(SET_COLOR_CLOCK));
	panel.setBrightness(menu.getOption(SET_BRIGHTNESS));
	DPRINTF("panel.setBrightness: %d\n", menu.getOption(SET_BRIGHTNESS));

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
	pir.setDelay((30+15*menu.getOption(PIR_HOLDTIME))*1000);
	animation.setShowName(menu.getOption(DISPLAY_ANINAME)==0);
	clock.setTempOffset(menu.getOption(TEMP_OFFSET)-7);
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
	pirNobodyThere, showTemp
};

void loop() {
	uint32_t now = millis();

	State state = showTime;

	//testScreen();

	menu.loadOptions();
	reloadConfig();

	DPRINTF("free ram %d\n", getFreeRam());

	uint32_t ledInterval = 3000;

	uint32_t switchToAni = now + clockShowTime;

	uint32_t nextColorChange = 0;

	int stateCycle = 0;
	int dateTemp = 0;

	/*int next = millis();

	while( true ) {
		long now = millis();
		if( now > next ) {
			next = now + 5000;
			Serial1.println("=node.info()");
		}
		// just hand over
		if( Serial.available() ) {
			int in = Serial.read();
			Serial1.write(in);
		}
		if( Serial1.available()) {
			int out = Serial1.read();
			Serial.write(out);
		}
	}*/

	node.start();

/*	int t = 0;
	while(t++ < 20) {
		long now = millis();
		node.update(now);
		delay(100);
	}
	DPRINTF("ip: %s\n",node.getIp());
	while(t++ < 20) {
		long now = millis();
		node.update(now);
		delay(100);
	}
	Result* r = node.getApList();
	while( r != NULL ) {
		DPRINTF("AP: %s\n", r->line);
		r = r->next;
	}*/
	bool hasNtpRequested = false;
	bool wifipresetRequested = false;

	while(true) {
		uint32_t now = millis();

		node.update(now);
		if( now > 1000 && node.isNodeMcuDetected() && menu.hasWifiPreset() && wifipresetRequested == false) {
			wifipresetRequested = true;
			DPRINTF("wifi preset ssid: '%s'\n",menu.getPreset_ssid());
			DPRINTF("wifi preset pw: '%s'\n",menu.getPreset_pw());
			node.configAp( menu.getPreset_ssid(), menu.getPreset_pw());
		}

		if( now > 40000 && node.isNodeMcuDetected() && hasNtpRequested==false ) {
			hasNtpRequested = true;
			node.requestNtpSync(now);
		}

		if( Serial.available() ) {
			String ps = Serial.readStringUntil('\n');
			const char* p = ps.c_str();
			DPRINTF("received string: '%s'\n",p);
			// date +SETTIME%s > /dev/ttyACM0
			// SETTIME<seconds since 1970 utc>
			if( strncmp(p,"SETTIME",7)==0) {
				p += 7;
				long unixtime = 0;
				while( *p != 0) {
					unixtime *= 10;
					if( *p >= '0' && *p <= '9') {
						unixtime += *p -'0';
					}
					p++;
				}
				// simple hack to adjust tz CEST +2 hours
				unixtime += 60*60*2;
				DateTime dt(unixtime);
				DPRINTF("setting rtc clock to: %ld \n", unixtime);
				clock.adjust(&dt);
			}
		} // read from serial

		// handle IR receiver
		if( irrecv.decode(&results) ) {
			menu.notifyEvent(results.value);
			irrecv.resume();
		}
		
		// show with blinking led ISR is running
		// cannot be switched on as it interferes with card reader
		if( panel.getISRCalls() > ledInterval ) {
			panel.resetISRCalls();
			on = !on;
			//digitalWrite(PIN_LED2,on);
			ledInterval = on ? 10000:10000;
			//DPRINTF("free ram %d\n", getFreeRam());
		}

		pir.update(now);
		clock.update(now);
		menu.update(now);

		if( menu.isActive() ) {
			clock.off();
			state = showMenu;
		}

		// ani -> time -> ani
		// or ani -> date -> ( ani -> time -> ) 5x

		switch( state ) {
		case showTime:
			clock.on();
			if( SAVECMP( now , switchToAni) ) {
				DPRINTF("free ram %d\n", getFreeRam());
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
			clock.update(now);
			break;

		case showTemp:
			clock.setMode(Clock::TEMP);
			clock.on();
			if( SAVECMP( now , switchToAni ) ) {
				state = showAni;
				clock.off();
			}
			clock.update(now);
			break;

		case showDate:
			clock.setMode(Clock::DATE);
			clock.on();
			if( SAVECMP( now , switchToAni ) ) {
				state = showAni;
				clock.off();
			}
			clock.update(now);
			break;

		case showAni:
			if( animation.update(now) ) { // true means ani finished

				if( stateCycle++ > 5 ) { // every five times
					stateCycle = 0;
					dateTemp++;
					// check want to show next temp ot date or simply time
					if( tempMode>0 && dateTemp % 3 == 0 ) {
						state=showTemp;
						clock.setMode(Clock::TEMP);
						switchToAni = now + 3000 + tempMode * 1500;
					} else {
						if( dateMode > 0 ) {
							state = showDate;
							clock.setMode(Clock::DATE);
							switchToAni = now + 3000 + dateMode * 1500;
						}
					}
				} else {
					switchToAni = now + clockShowTime; // millis to show clock
					state = showTime;
					clock.setMode(menu.getOption(SET_TIME_MODE)==1?Clock::TIMESEC:Clock::TIME);
				}

				// check color change
				if( SAVECMP( now , nextColorChange ) ) {
					nextColorChange = now + 60000;
					if( menu.getOption(SET_COLOR_ANI)==COLOR_CHANGE ) {
						panel.setAnimationColor(random(3));
					}
					if( menu.getOption(SET_COLOR_CLOCK)==COLOR_CHANGE ) {
						panel.setTimeColor(random(3));
					}
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
			}
			break;
		case freeze:

			break;
		}
		
	}
}

