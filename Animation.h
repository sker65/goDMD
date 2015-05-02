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
	uint16_t readNextFrame(long now, bool mask);
	boolean update(long now);
	boolean begin();
	void setFskMode(int mode);

	void setRandomOrder(boolean randomOrder) {
		this->randomOrder = randomOrder;
	}

protected:
	void skipAllFrames(File& f);

	Clock& clock;
	SDClass& sd;
	File ani;
	LEDMatrixPanel& panel;
	uint16_t numberOfAnimations;
	uint16_t numberOfFrames;

	uint16_t actAnimation;
	uint16_t actFrame;

	uint8_t fskFilter;
	uint8_t cycles;
	uint8_t holdCycles;
	uint32_t actFilePos;
	uint16_t refreshDelay;
	uint16_t clockFrom;
	uint16_t xoffset;
	uint16_t yoffset;
	uint8_t fsk;

	uint32_t* aniIndex;

	long nextAnimationUpdate;
	boolean clearAfterAni;
	boolean hold;
	boolean clockInFront;
	boolean clockSmall;

	boolean randomOrder;
	boolean seenAllAnimations; // switches to true, when all animations scanned in the first run
};

#endif /* ANIMATION_H_ */
