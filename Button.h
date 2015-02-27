/*
 * Button.h
 *
 *  Created on: 08.02.2015
 *      Author: sr
 */

#ifndef BUTTON_H_
#define BUTTON_H_

#include "ClickHandler.h"

#define DEBOUNCE 200
#define LONGPRESS 1500

class Button {
public:
	Button(uint8_t pin, uint8_t no, ClickHandler* handler );
	virtual ~Button();

	void update(long now);

protected:
	uint8_t pin;
	uint8_t no;
	ClickHandler* handler;
	bool state;
	long lastPress;
	long last;
};

#endif /* BUTTON_H_ */
