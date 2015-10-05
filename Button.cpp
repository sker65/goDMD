/*
 * Button.cpp
 *
 *  Created on: 08.02.2015
 *      Author: sr
 */

#include <Arduino.h>
#include "Button.h"

Button::Button(uint8_t pin, uint8_t no, ClickHandler* handler)
	{
	this->pin = pin;
	this->no = no;
	this->handler = handler;
	state = false;
	lastPress=0;
	last=0;
	pinMode(pin,INPUT_PULLUP); // inactive HIGH
}

Button::~Button() {
}

void Button::update(uint32_t now) {
	int b = digitalRead(pin);
	if( state && b==HIGH ) { // release
		uint32_t delta = now-lastPress;
		if( delta > SUPERLONGPRESS ) handler->buttonReleased(no, 2);
		else if( delta > LONGPRESS )  handler->buttonReleased(no, 1);
		else handler->buttonReleased(no, 0);

		state=false;
	}
	if( !state && b==LOW ) { // press
		if( now  - last > DEBOUNCE ) {
			state = true;
			lastPress = now;
		}
		last = now;
	}
}
