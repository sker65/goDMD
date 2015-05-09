/*
 * PirHandler.h
 *
 *  Created on: 18.03.2015
 *      Author: sr
 */

#ifndef PIRHANDLER_H_
#define PIRHANDLER_H_

#include <stdint.h>

class PirHandler {
public:
	PirHandler(uint8_t pin);
	virtual ~PirHandler();

	void update(long now);
	bool somebodyHere();
	bool actual();

	long getDelay() const {
		return delay;
	}

	void setDelay(long delay) {
		this->delay = delay;
	}

protected:
	uint8_t pin;
	bool lastResult;
	long lastProbe;
	long lastTimeSomebodyWasThere;
	long delay;
};

#endif /* PIRHANDLER_H_ */
