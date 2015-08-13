/*
 * Menu.cpp
 * implements a service menu to config go dmd clock
 *
 * Created on: 08.02.2015
 *      Author: sr
 */

// we replace EEPROM store through sd card store #include <EEPROM.h>
#include <SD.h>
#include "Menu.h"
#include "debug.h"

// menu texts
const char* mmText[] = {
	//   0123456789012345
		"Helligkeit      ",
		"0","1","2","3","4","5","6",
		".",
		"Zeit-Stunden    ",
		"0","1","2","3","4","5","6","7","8","9",
		"10","11","12","13","14","15","16","17","18","19",
		"20", "21", "22", "23",
		".",
		"Zeit-Min  10er  ",
		"0","1","2","3","4","5",
		".",
		"Zeit-Min  1er   ",
		"0","1","2","3","4","5","6","7","8","9",
		".",
		"Datum-Jahr      ",
		"2015","2016","2017","2018","2019","2020",
		".",
		"Datum-Monat     ",//1-12
		"1","2","3","4","5","6","7","8","9","10","11","12",
		".",
		"Datum-Tag       ",//1-31
		"1","2","3","4","5","6","7","8","9",
		"10","11","12","13","14","15","16","17","18","19",
		"20", "21", "22", "23","24","25","26","27","28","29","30","31",
		".",
		"Zeit-Mode       ",
		"H:M", "H:M:S",
		".",
		"Dauer-Zeit-Anz. ",//1-10
		"1","2","3","4","5","6","7","8","9",
		".",
		"Dauer Date Anz. ",
		"0","1","2","3","4",
		".",
		"Farbe-Ani.      ",
		"rot", "gruen", "amber", "wechsel",
		".",
		"Farbe-Uhr       ",//rot / grun / amber / wechsel
		"rot", "gruen", "amber", "wechsel",
		".",
		"Dauer-Temp Anz. ",
		"0","1","2","3","4",
		".",
		"PIR Mode        ",
		"inaktiv", "dim", "no ani", "switchoff",
		".",
		"FSK Filter      ",
		"18", "16", "12", "6",
		".",
		"Uhr Blink       ",
		"an", "aus",
		".",
		"Uhr 12/24H      ",
		"24", "12",
		".",
		"Ani Reihenfolge ",
		"fest", "zufaellig",
		".",
		"Schrift         ",
		"0","1","2","3","4",
		".",
		"LED Indicator   ",
		"an","aus",
		".",
		"PIR Hold Time   ",
		"1","2","3","4",
		".",
		"Display AniName ",
		"an","aus",
		".",
		"Temp Offset     ",
		"-7","-6","-5","-4","-3","-2","-1","0","+1","+2","+3",
		".",
		"Net Config      ",
		"no wifi ext",
		".",
		"." // end mark
};


Menu::Menu(LEDMatrixPanel* panel, Clock* clock, SDClass* sd, NodeMcu* node ) :
		panel( panel ), clock(clock), sd(sd), node(node)
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
	DPRINTF("menu: %s\n",mmText[0]);
	while(j<NMENU+5) {
		if( mmText[i][0] == '.' ) {
			DPRINTF("options: %d\n",i-k);
			nOptions[j-1] = i-k;
			k = i+2;
			if( mmText[i+1][0] == '.') {
				break;
			}
			titleIndex[j] = i+1;
			DPRINTF("menu: %s\n", mmText[i+1]);
			j++;
		}
		i++;
	}

	loadOptions();
	clockDirty = false;
	netMenu = IP;
	pActPasswdChar = netPasswd;
	passChar = 'A';
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

// these are the codes from the IR receiver that the irrecv method decodes
// must be adapted for the given ir controller


#include "ir-codes.h"

#define RED_BUT   BUT_CH_MINUS
#define GREEN_BUT BUT_CH
#define AMBER_BUT BUT_CH_PLUS

// clock color
#define CLOCK_RED BUT_PREV
#define CLOCK_GREEN BUT_NEXT
#define CLOCK_AMBER BUT_PLAY

#define SEL_BUT BUT_PLUS
#define OPTION_BUT BUT_MINUS
#define MENU_BUT BUT_EQ

#define CYCLE_FONTS BUT_0
#define BRIGHTNESS_UP BUT_8
#define BRIGHTNESS_DOWN BUT_7

int Menu::lookupCode(unsigned long code) {
	return code;
}

/**
 * IR Decoder notifies events here
 */
void Menu::notifyEvent(unsigned long code) {
	int event = lookupCode(code);
	DPRINTF("Menu notify: 0x%06lx\n", event );
	if( option[LED_INDICATOR] == 0) {
		panel->setPixel(0,0,2); // light pixel
	}
	uint8_t newFont;
	switch( event ) {
		case BRIGHTNESS_DOWN:
			panel->setBrightness(panel->getBrightness()-1);
			DPRINTF("set brightness to %d\n",panel->getBrightness());
			break;
		case BRIGHTNESS_UP:
			panel->setBrightness(panel->getBrightness()+1);
			DPRINTF("set brightness to %d\n",panel->getBrightness());
			break;
		case CYCLE_FONTS:
			newFont = clock->getActualFont()+1;
			clock->requestFont(newFont);
			DPRINTF("clock->requestFont(%d)\n", newFont );
			break;

		case RED_BUT:
			panel->setAnimationColor(0);
			DPRINTF("panel->setAnimationColor(%d)\n", 0 );
			break;
		case GREEN_BUT:
			panel->setAnimationColor(1);
			DPRINTF("panel->setAnimationColor(%d)\n", 1 );
			break;
		case AMBER_BUT:
			panel->setAnimationColor(2);
			DPRINTF("panel->setAnimationColor(%d)\n", 2 );
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
			this->buttonReleased(BUTTON_MENU, 2);
			break;
		case OPTION_BUT:
			this->buttonReleased(BUTTON_MENU, 0);
			break;
		case SEL_BUT:
			this->buttonReleased(BUTTON_MENU, 1);
			break;
			
	}
}

/**
 * called when main menu changes. stores configured item, if changed
 */
void Menu::saveOption() {
	if( actOption != option[actMenu] ) {
		dirty = true;
		DPRINTF("Setting option[%d]=%d\n",actMenu,actOption);
		if( actMenu >= SET_TIME_HOURS && actMenu <= SET_DATE_DAY) {
			clockDirty = true; // force rtc adjust
			DPRINTF("and force RTC sync\n");
		}
	}
	option[actMenu] = actOption;
}

#define OPTION_DAT "option.dat"

/**
 * load menu options from sd card
 */
void Menu::loadOptions() {

	// load options from SD card
	SdFile f;
	if( f.open(SD.root, OPTION_DAT, O_READ) ) {
		int r = f.read((void*)option, NMENU);
		DPRINTF("Menu::loadOptions loaded %d bytes\n",r);
		f.close();
	} else {
		DPRINTF("Menu::loadOptions %s not found\n",OPTION_DAT);
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

	for(int i = 0; i<NMENU;i++) {
		DPRINTF("Menu::loadOptions option[%d]=%d\n",i,option[i]);
	}
}


void Menu::doNetConfig(uint8_t n, uint8_t longClick) {
	char* ip;
	switch(netMenu) {
	case IP:
		ip = node->getIp();
		strncpy(netOption,"IP-Addr.",17);
		strncpy(netValue,ip,17);
		break;

	case LISTAP:
		if( !aplistRead ) {
			actAp = apList = node->getApList();
			aplistRead = true;
		}
		// keep actual ap name pointer / name
		strncpy(netOption,"SSID",17);
		if( actAp != NULL && actAp->line != NULL) {
			strncpy(actualSsid, actAp->line,17);
		}
		strncpy(netValue,actualSsid,17);
		if( n == BUT_PLUS ) {
			if( actAp != NULL && actAp->next != NULL ) {
				actAp = actAp->next;
			} else {
				actAp = apList;
			}
		}
		break;

	case PASSWD:
		strncpy(netOption,"Password",17);
		strncpy(netValue,netPasswd,17);
		if( n == BUT_2 ) {
			if( passChar++ == 128 ) passChar = 32;
		}
		if( n == BUT_8 ) {
			if( passChar-- == 31 ) passChar = 127;
		}
		if( n == BUT_6 ) {
			*pActPasswdChar++ = passChar;
		}
		if( n == BUT_4 ) {
			*pActPasswdChar = 0;
			if( pActPasswdChar > netPasswd ) pActPasswdChar--;
		}
		if( n == BUT_PLUS) {
			node->configAp(actualSsid, netPasswd);
			netMenu = IP;
		}
		break;
	}

}

/**
 * callback with is called from buttons
 * TODO: actually two buttons are expected, but piguino micro has only
 * one. must be fixed to implement one button control.
 * not needed actually as we have ir control support
 */
void Menu::buttonReleased(uint8_t n, uint8_t longClick) {
	DPRINTF("button %d %d \n", n, longClick);
	if( active ) {
		if( netConfig ) {
			if( n==BUTTON_MENU && longClick==1 ) {
				netConfig = false;
			} else {
				doNetConfig(n,longClick);
				return;
			}
		}
		redrawNeeded = true;
		if( n==BUTTON_MENU && longClick==2 ) {
			saveOption();
			DPRINTF("leave menu\n");
			leaveMenu();
		}
		if( n==BUTTON_MENU && longClick==1 ) {
			// advance & save leaved menu
			saveOption();

			actMenu++;
			if( actMenu >= NMENU ) {
				actMenu=0;
			}
			DPRINTF("menu %s\n",mmText[titleIndex[actMenu]]);
			// load active option from next menu
			actOption = option[actMenu];
			// ensure werte bereich
			DPRINTF("actOption: %d\n", actOption);
			DPRINTF("nOptions[actMenu]: %d\n", nOptions[actMenu]);
			if( actMenu<0 || actOption >= nOptions[actMenu] ) actOption = 0;
			DPRINTF("trimmed actOption: %d\n", actOption);
			if( actMenu == MENU_NET_CONFIG && node->isNodeMcuDetected() ) {
				netConfig=true;
				redrawNeeded = true;
			}
		}
		if( n==BUTTON_MENU && longClick==0 ) {
			actOption++;
			if( actOption >= nOptions[actMenu]) {
				actOption=0;
			}
		}

	} else {
		if( n==BUTTON_MENU && longClick==2 ) {
			enterMenu();
		}
	}
}


void Menu::enterMenu() {
	loadOptions();
	panel->clear();
	redrawNeeded=true;
	active=true;
	// load active option from next menu
	actOption = option[actMenu];
}

/**
 * redraws whole menu if item changes
 */
void Menu::redrawMenu() {
	char op[17] = "                ";
	int i = titleIndex[actMenu];
	strncpy(op,mmText[i+actOption+1],strlen(mmText[i+actOption+1]));
	//panel.writeText(op,0,8,16);
	if( active ) {
		panel->writeText("Config Menu",0,0,16);
		if( netConfig ) {
			panel->writeText(netOption,0,8,16);
			panel->writeText(netValue,0,16,16);
		} else {
			panel->writeText(mmText[i],0,8,16);
			panel->writeText(op,0,16,16);
		}
		/* farb streifen
		if( actMenu == SET_COLOR_CLOCK) {
			if( actOption<15) {
				panel->drawRect(0,24,128,8,actOption+1);
			} else {
				for( int i = 0; i < 15; i++ ) {
					panel->drawRect(i*8,24,8,8,i+1);
				}
			}
		} else {
			panel->drawRect(0,24,128,8,0);
		}*/
	}
}

void Menu::leaveMenu() {

	// check RTC settings
	if( clockDirty ) {
		//uint16_t year, uint8_t month, uint8_t day,
		// uint8_t hour =0, uint8_t min =0, uint8_t sec =0
		DPRINTF("setting rtc.\n");
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

	for(int i = 0; i<NMENU;i++) {
		DPRINTF("Menu::leaveMenu option[%d]=%d\n",i,option[i]);
	}
	SdFile f;
	if( f.open(SD.root, OPTION_DAT, O_CREAT|O_WRITE) ) {
		int r = f.write((void*)option, NMENU);
		DPRINTF("Menu::leaveMenu written %d bytes\n",r);
		f.close();
	} else {
		DPRINTF("Menu::leaveMenu: open for write %s failed.", OPTION_DAT);
	}
	active = false;
}

