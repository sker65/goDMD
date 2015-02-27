/*
 * Animation.h
 *
 *  Created on: 01.02.2015
 *      Author: sr
 */

#ifndef ANIMATION_H_
#define ANIMATION_H_
#include "LEDMatrixPanel.h"
#include <SD.h>
#include "Clock.h"

class Animation {
public:
	Animation(SDClass& sd, LEDMatrixPanel& panel, Clock& clock);
	virtual ~Animation();
	void readNextAnimation();
	void readNextFrame(long now, bool mask);
	boolean update(long now);
	boolean begin();

protected:
	Clock& clock;
	SDClass& sd;
	File ani;
	LEDMatrixPanel& panel;
	uint16_t numberOfAnimations;
	uint16_t numberOfFrames;

	uint16_t actAnimation;
	uint16_t actFrame;

	uint8_t cycles;
	uint8_t holdCycles;
	uint16_t actFilePos;
	uint8_t refreshDelay;
	uint16_t clockFrom;

	long nextAnimationUpdate;
	boolean hold;
	boolean clockInFront;
};

#endif /* ANIMATION_H_ */
