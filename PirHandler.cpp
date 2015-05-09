/*
 * PirHandler.cpp
 *
 *  Created on: 18.03.2015
 *      Author: sr
 */

#include "PirHandler.h"
#include <WProgram.h>

PirHandler::PirHandler(uint8_t pin) : pin(pin)
{
	pinMode(pin, OUTPUT);
	lastProbe = lastTimeSomebodyWasThere=0;
	lastResult = false;
	delay = 45000;
}

bool PirHandler::actual() {
	return digitalRead(pin)==HIGH;
}

void PirHandler::update(long now) {
	lastResult = digitalRead(pin);  // true / high means active -> somebody is here
	lastProbe = now;
	if( lastResult ) {
		lastTimeSomebodyWasThere = now;
	}
}

PirHandler::~PirHandler() {
}

bool PirHandler::somebodyHere() {
	return( lastResult || lastTimeSomebodyWasThere + delay > millis() );
}
