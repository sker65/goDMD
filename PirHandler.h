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

	void update(uint32_t now);
	bool somebodyHere();
	bool actual();

	uint32_t getDelay() const {
		return delay;
	}

	void setDelay(uint32_t delay) {
		this->delay = delay;
	}

protected:
	uint8_t pin;
	bool lastResult;
	uint32_t lastProbe;
	uint32_t lastTimeSomebodyWasThere;
	uint32_t delay;
};

#endif /* PIRHANDLER_H_ */
