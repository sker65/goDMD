/*
 * Menu.h
 *
 *  Created on: 08.02.2015
 *      Author: sr
 */

#ifndef MENU_H_
#define MENU_H_

#include "LEDMatrixPanel.h"
#include "Clock.h"
#include "Button.h"
#include "ClickHandler.h"
#include "NodeMcu.h"

// not on pic micro #define PIN_B1 3
// not on PIC micro #define PIN_B2 5

#define BUTTON_MENU 1
#define BUTTON_SEL 2

#define SET_BRIGHTNESS 0
#define SET_TIME_HOURS 1
#define SET_TIME_TENMIN 2
#define SET_TIME_ONEMIN 3
#define SET_DATE_YEAR 4
#define SET_DATE_MON 5
#define SET_DATE_DAY 6

#define SET_TIME_MODE 7

#define SET_TIME_DURATION 8
#define SET_DATE_MODE 9
#define SET_COLOR_ANI 10
#define SET_COLOR_CLOCK 11
#define COLOR_CHANGE 3 // we use 3 normal colors 0,1,2 so 4 is change

#define SET_TEMP_MODE 12
#define SET_PIR_MODE 13
#define FSK_MODE 14
#define BLINK_MODE 15
#define H2412_MODE 16
#define ANIORDER_MODE 17
#define CURRENT_FONT 18
#define LED_INDICATOR 19
#define PIR_HOLDTIME 20
#define DISPLAY_ANINAME 21
#define TEMP_OFFSET 22
#define MENU_NET_CONFIG 23

#define BLINK_ON 0
#define BLINK_OFF 1

#define MODE_24H 0
#define MODE_12H 1

#define NMENU 24

#define PIR_INACTIVE 0
#define PIR_DIM 1
#define PIR_NOANI 2
#define PIR_SWITCHOFF 3

class SDClass;

class Menu : public ClickHandler {

public:
	Menu(LEDMatrixPanel* panel, Clock* clock, SDClass* sd, NodeMcu* node);

	virtual ~Menu();

	void update(uint32_t now);

	bool isActive() {return active; }

	void loadOptions();

	void enterMenu();

	void redrawMenu();

	void leaveMenu();

	void saveOption();

	uint8_t getOption(int index) {return option[index];}

	void buttonReleased(uint8_t no, uint8_t longPress);
	bool doNetConfig(uint32_t event);

	enum NetMenu { IP, LISTAP, PASSWD };

	void notifyEvent(uint32_t event);
	int lookupCode(unsigned long code);

private:
	static const char* mm[];
	LEDMatrixPanel* panel;
	Clock* clock;
	SDClass* sd;
	NodeMcu* node;

	char netOption[128];
	char netValue[128];
	char netOptionDisplay[16];
	char netValueDisplay[16];
	uint32_t nextWobble;
	bool wobbleForward;
	bool passCharBlink;
	int wobbleOffset;
	NetMenu netMenu;
	bool aplistRead;
	char actualSsid[128];
	char netPasswd[64];
	char* pActPasswdChar;
	char passChar;
	Result* apList;
	Result* actAp;

	Button menuButton;
	//Button selButton;
	bool active;
	bool dirty; // needs reconfig
	bool redrawNeeded;

	bool clockDirty;
	bool netConfig;

	uint8_t actMenu;
	uint8_t actOption;

	uint8_t titleIndex[NMENU];
	uint8_t nOptions[NMENU];

	uint8_t option[NMENU];
};

#endif /* MENU_H_ */
