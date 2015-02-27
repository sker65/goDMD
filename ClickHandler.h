/*
 * ClickHandler.h
 *
 *  Created on: 08.02.2015
 *      Author: sr
 */

#ifndef CLICKHANDLER_H_
#define CLICKHANDLER_H_

#include <Arduino.h>

class ClickHandler {
public:
	ClickHandler();
	virtual ~ClickHandler();

	virtual void buttonReleased(uint8_t no, bool longPress)=0;
};

#endif /* CLICKHANDLER_H_ */
