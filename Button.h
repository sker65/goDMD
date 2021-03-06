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
#define LONGPRESS 1000
#define SUPERLONGPRESS 3000

class Button {
public:
	Button(uint8_t pin, uint8_t no, ClickHandler* handler );
	virtual ~Button();

	void update(uint32_t now);

protected:
	uint8_t pin;
	uint8_t no;
	ClickHandler* handler;
	bool state;
	uint32_t lastPress;
	uint32_t last;
};

#endif /* BUTTON_H_ */
