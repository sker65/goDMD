/*
 * Menu.cpp
 *
 *  Created on: 08.02.2015
 *      Author: sr
 */

// we replace EEPROM store through sd card store #include <EEPROM.h>
#include <SD.h>
#include "Menu.h"


const char* mmText[] = {
		"Helligkeit       ",
		"dunkel","mittel","hell",
		".",
		"Zeit-Stunden     ",
		"0","1","2","3","4","5","6","7","8","9",
		"10","11","12","13","14","15","16","17","18","19",
		"20", "21", "22", "23",
		".",
		"Zeit-Min  10er    ",
		"0","1","2","3","4","5",
		".",
		"Zeit-Min  1er     ",
		"0","1","2","3","4","5","6","7","8","9",
		".",
		"Datum-Jahr        ",
		"2015","2016","2017","2018","2019","2020",
		".",
		"Datum-Monat       ",//1-12
		"1","2","3","4","5","6","7","8","9","10","11","12",
		".",
		"Datum-Tag         ",//1-31
		"1","2","3","4","5","6","7","8","9",
		"10","11","12","13","14","15","16","17","18","19",
		"20", "21", "22", "23","24","25","26","27","28","29","30","31",
		".",
		"Zeit-Mode         ",
		"H:M", "H:M:S",
		".",
		"Dauer-Zeit-Anz.   ",//1-10
		"1","2","3","4","5","6","7","8","9",
		".",
		"Datum-Anzeige     ",
		"an", "aus",
		".",
		"Farbe-Ani.        ",
		"rot", "gruen", "amber", "wechsel",
		".",
		"Farbe-Uhr         ",//rot / grun / amber / wechsel
		"rot", "gruen", "amber", "wechsel",
		".",
		"Temperatur        ",//an / aus
		"an", "aus",
		".",
		"PIR Mode          ",
		"inaktiv", "dim", "no ani", "switchoff",
		".",
		"." // end mark
};


Menu::Menu(LEDMatrixPanel* panel, Clock* clock, SDClass* sd) :
		panel( panel ), clock(clock), sd(sd)
		,menuButton(PIN_BTN1, BUTTON_MENU, this)
		//,selButton(PIN_B2, BUTTON_SEL,this)
		{

	active = false;
	dirty=false;
	redrawNeeded=false;
	actMenu=0;
	actOption=0;

	// fill index

	int i = 0;
	int j = 1;
	titleIndex[0] = 0;
	int k = 1;
	Serial.print("menu: "); Serial.println(mmText[0]);
	while(j<NMENU+5) {
		if( mmText[i][0] == '.' ) {
			Serial.print("options: "); Serial.println(i-k);
			nOptions[j-1] = i-k;
			k = i+2;
			if( mmText[i+1][0] == '.') {
				break;
			}
			titleIndex[j] = i+1;
			Serial.print("menu: "); Serial.println(mmText[i+1]);
			j++;
		}
		i++;
	}

	loadOptions();
	clockDirty = false;
}

Menu::~Menu() {
}

void Menu::update(long now) {

	menuButton.update(now);
//	selButton.update(now);

	if( active && redrawNeeded ) {
		redrawMenu();
		redrawNeeded=false;
	}
}

// ani colors
#define RED_BUT 0xE85952E1
#define GREEN_BUT 0x78CDA4DD
#define AMBER_BUT 0xA2672345

#define CLOCK_RED 0xD3FD9A81	
#define CLOCK_GREEN 0x6471EC7D
#define CLOCK_AMBER 0x9D52009D

#define SEL_BUT 0xA7315F7D
#define OPTION_BUT 0xB9C07541
#define MENU_BUT 0xdcc45be1

/**
 * IR Decoder notifies events here
 */
void Menu::notifyEvent(unsigned long event) {
	printf("Menu notify: 0x%06x\n", event );
	switch( event ) {
		case RED_BUT:
			panel->setAnimationColor(0);
			break;
		case GREEN_BUT:
			panel->setAnimationColor(1);
			break;
		case AMBER_BUT:
			panel->setAnimationColor(2);
			break;
		case CLOCK_RED:
			panel->setTimeColor(0);
			break;
		case CLOCK_GREEN:
			panel->setTimeColor(1);
			break;
		case CLOCK_AMBER:
			panel->setTimeColor(2);
			break;
		case MENU_BUT:
			this->buttonReleased(BUTTON_MENU, true);
			break;
		case OPTION_BUT:
			this->buttonReleased(BUTTON_MENU, false);
			break;
		case SEL_BUT:
			this->buttonReleased(BUTTON_SEL, false);
			break;
			
	}
}

void Menu::saveOption() {
	if( actOption != option[actMenu] ) {
		dirty = true;
		if( actMenu >= SET_TIME_HOURS && actMenu <= SET_DATE_DAY) {
			clockDirty = true; // force rtc adjust
		}
	}
	option[actMenu] = actOption;
}

#define OPTION_DAT "OPTION.DAT"

void Menu::loadOptions() {

	// load options from SD card
	SdFile f;
	if( f.open(SD.root, OPTION_DAT, O_READ) ) {
		f.read((void*)option, NMENU);
		f.close();
	}
	
	// load all settings from eeprom and display menu
	// time / date is loaded from clock
	DateTime n = clock->getActualTime();
	option[SET_TIME_HOURS] = n.hour();
	option[SET_TIME_TENMIN] = n.minute()/10;
	option[SET_TIME_ONEMIN] = n.minute()%10;

	option[SET_DATE_DAY] = n.day()-1;
	option[SET_DATE_MON] = n.month()-1;
	option[SET_DATE_YEAR] = n.year()-2015;

}

void Menu::buttonReleased(uint8_t n, bool longClick) {
	printf("button %d %d \n", n, longClick);
	if( active ) {
		redrawNeeded = true;
		if( n==BUTTON_MENU && longClick ) {
			saveOption();
			printf("leave menu\n");
			leaveMenu();
		}
		if( n==BUTTON_MENU && !longClick ) {
			// advance & save leaved menu
			saveOption();

			actMenu++;
			if( actMenu >= NMENU ) {
				actMenu=0;
			}
			Serial.print("menu "); Serial.println(mmText[titleIndex[actMenu]]);
			// load active option from next menu
			actOption = option[actMenu];
			// ensure werte bereich
			Serial.print("option: ");Serial.println(actOption);
			Serial.print("option[actMenu]: ");Serial.println(nOptions[actMenu]);
			if( actMenu<0 || actOption >= nOptions[actMenu] ) actOption = 0;

			Serial.print("option "); Serial.println(actOption);
		}
		if( n==BUTTON_SEL && !longClick ) {
			actOption++;
			if( actOption >= nOptions[actMenu]) {
				actOption=0;
			}
		}

	} else {
		if( n==BUTTON_MENU && longClick ) {
			enterMenu();
		}
	}
}


void Menu::enterMenu() {
	loadOptions();
	panel->clear();
	clock->clear();
	redrawNeeded=true;
	active=true;
}

void Menu::redrawMenu() {
	char op[17] = "                ";
	int i = titleIndex[actMenu];
	strncpy(op,mmText[i+actOption+1],strlen(mmText[i+actOption+1]));
	//panel.writeText(op,0,8,16);
	if( active ) {
		panel->writeText("Config Menu",0,0,16);
		panel->writeText(mmText[i],0,8,16);
		panel->writeText(op,0,16,16);
	}
}

void Menu::leaveMenu() {

	// check RTC settings
	if( clockDirty ) {
		//uint16_t year, uint8_t month, uint8_t day,
		// uint8_t hour =0, uint8_t min =0, uint8_t sec =0
		Serial.println("setting rtc");
		DateTime dt(
				option[SET_DATE_YEAR]+2015,
				option[SET_DATE_MON]+1,
				option[SET_DATE_DAY]+1,
				option[SET_TIME_HOURS],
				option[SET_TIME_TENMIN]*10+option[SET_TIME_ONEMIN]
				);
		clock->adjust(&dt);
		clockDirty = false;
	}

	SdFile f;
	if( f.open(SD.root, OPTION_DAT, O_CREAT|O_WRITE) ) {
		f.write((void*)option, NMENU);
		f.close();
	}

	active = false;
}

