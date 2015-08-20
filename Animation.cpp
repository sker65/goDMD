/*
 * Animation.cpp
 *
 *  Created on: 01.02.2015
 *      Author: sr
 */

#include "Animation.h"
#include "debug.h"
#include "macros.h"


Animation::Animation(SDClass& sd, LEDMatrixPanel& panel, Clock& clock) :
sd(sd), panel(panel), clock(clock) {
	actAnimation = 0;
	actFrame = 0;
	nextAnimationUpdate=0;
	numberOfAnimations=0;
	numberOfFrames=0;
	fskFilter = 18;
	seenAllAnimations = false;
	clearAfterAni = true;
}

boolean Animation::begin() {
	ani = sd.open("/FOO.ANI");
	if( !ani ) return false;
	//delay(3000);
	// read 4 header ANIM
	ani.seekCur(4);
	// read 2 byte version (1)
	version = ani.read()*256+ani.read();
	DPRINTF("version: %d\n",version);
	// read number of anis
	numberOfAnimations  = ani.read()*256+ani.read();
	DPRINTF("number of animations: %d\n",numberOfAnimations);
	// pos is now 8

	aniIndex = (uint32_t*)malloc( sizeof(uint32_t*)*numberOfAnimations);
	randomSeed((int)millis());
	return true;
}

char* readString(File& f, char* buf, int buflen) {
	int len = f.read()*256+f.read();
	for(int i=0; i < len; i++) {
		if( len<buflen) buf[i] = f.read();
		else f.read();
	}
	buf[len<buflen?len:buflen] = '\0';
	DPRINT("%s\n",buf);
	return buf;
}

boolean Animation::readNextAnimation() {
	int c = 0;
	char buf[40];
	seenMaskFrame = false;
	if( randomOrder && seenAllAnimations ) {
		actAnimation = random(numberOfAnimations);
		ani.seek(aniIndex[actAnimation]);
	}
	while(true) {
		actFilePos = ani.position();
		aniIndex[actAnimation] = actFilePos;
		DPRINTF2("name: ");
		strcpy(aniname, (const char*)readString(ani,buf,40)); // read & copy name
		if( version > 1 ) {
			DPRINTF2("transition: ");
			readString(ani,buf,40); // read transition name
		}
		// cycles 2
		int cyc = ani.read()*256+ani.read();
		if( cycles == 0 ) { // beim loop mit mehreren cycles, soll nur
			cycles = cyc;
		}
		DPRINTF("cycles: %d\n",cycles);
		// hold cycles 2
		holdCycles = ani.read()*256+ani.read();
		DPRINTF("hold: %d\n", hold);

		// clock from 2
		clockFrom = ani.read()*256+ani.read();
		//clockFrom = 5;
		DPRINTF("clockFrom: %d\n",clockFrom);
		// clock small (1 Byte)
		clockSmall = ani.read()!=0;
		// clock in front (1 Byte)
		clockInFront = ani.read() != 0;
		// clock x / y offset je 2
		xoffset = ani.read()*256+ani.read();
		yoffset = ani.read()*256+ani.read();
		DPRINTF("small: %d, front: %d, x: %d, y: %d\n",
				clockSmall, clockInFront, xoffset, yoffset );
		// refresh delay 2
		refreshDelay = ani.read()*256+ani.read();
		DPRINTF("refreshDelay: %d\n",refreshDelay);
		// type (1 Byte)
		ani.seekCur(1);
		// 15 Byte insgesamt
		fsk = ani.read();
		DPRINTF("fsk: %d\n",fsk);
		// frameset count
		numberOfFrames = ani.read()*256+ani.read();
		DPRINTF("number of frames: %d\n",numberOfFrames);
		actFrame = 0;
		actAnimation++;
		clearAfterAni = true;

		if( fsk <= fskFilter ) break;

		skipAllFrames(ani);
		if( actAnimation >= numberOfAnimations ) {
			ani.seek(8); // reset
			actAnimation = 0;
			seenAllAnimations = true;
		}
		if( c++ >= numberOfAnimations){
			DPRINTF("skipping loop exceeded number of animations: %d\n",numberOfAnimations);
			panel.println("no ani left");
			panel.println("check fsk set.");
			DPRINTF("actual filepos: %ld\n", ani.position() );
			return false;
		}
	} // at least one must remain
	return true;
}

void Animation::skipAllFrames(File& f) {
	for( int i = 0; i < numberOfFrames; i++) {
		DPRINTF("skipping frame: %d\n", i);
		uint16_t len = f.read()*256+f.read();
		f.read();f.read(); // 2 bytes delay new in version 1.02
		byte numberOfPlanes = ani.read();
		byte buf[len];
		for( int j = 0; j < numberOfPlanes; j++) {
			ani.read(); // type
			f.readBytes(buf, len);
		}
	}
}

/**
 * reads the next frame from animation stream and do masking of clock
 * @param: now the actual time used for time / date rendering
 * @maskClock: if set render clock in the background
 * if animation stream contains a mask frame then clock is rendered
 * and clock is masked out.
 */
uint16_t Animation::readNextFrame(long now, bool maskClock) {
	uint16_t buflen = ani.read()*256+ani.read();
	uint16_t delay = ani.read()*256+ani.read();
	byte numberOfPlanes = ani.read();
	// hold mask plane for transition
	byte mask[buflen];
	bool useMask = false;
	for( int i = 0; i < numberOfPlanes; i++) {
		byte buf[buflen];
		byte planeType = ani.read();
		if( panel.getSizeOfBufferInByte() == buflen ) {
			if( planeType == 0x6D ) { // 'm' for masking plane for time
				ani.readBytes(mask, buflen);
				useMask = true;
				seenMaskFrame = true;
			} else {

				ani.readBytes(buf, buflen);

				// there was a masking plane -> use it
				if( useMask ) {
					clock.setMode(Clock::TIME); // fixes #27
					clearAfterAni = false;
					// allocate buffer for time to mask
					byte timebuf[buflen];
					memset(timebuf,0,buflen);
					clock.writeTimeIntern(now,timebuf); // render time

					byte* pDst = timebuf;
					byte* pMask = mask;
					// mask time
					for(int j = 0; j< buflen; j++) {
						*pDst++ &= ~( *pMask++ ); // mask in
					}
					// copy masked time to time plane
					memcpy((void*)panel.getBuffers()[2],timebuf,buflen);
					pDst = buf;
					pMask = mask;
					// now mask animation
					for(int j = 0; j< buflen; j++) {
						*pDst++ &= *pMask++; // mask out
					}
				}

				//  do not used normal digits, but mask digits
				if( maskClock ) clock.writeTimeIntern(now,buf,_BS_alu_andInverted, true);

				// TODO to put clock in background this should be or
				// but until now its a different plane

				// copy planes 0,1 into panel buffer
				if( planeType < panel.getNumberOfBuffers()) {
					memcpy((void*)panel.getBuffers()[planeType],buf,buflen);
				}
			} // not a mask plane
		} // for planes
	}
	panel.swap(true);
	if( showName ) panel.writeText(aniname,0,0,16); // show name of animation
	actFrame++;
	return delay;
}

boolean Animation::update(long now) {
	if( SAVECMP( now , nextAnimationUpdate) ) {
		if( hold ) {
			hold = false;
			// check for transition small to big, if so clear anyway
			if( clock.getFont() == Clock::Small ) {
				clock.setFont(Clock::Big);
				panel.clearTime();
			} else {
				if( clearAfterAni ) {
					panel.clearTime();
				}
			}

			panel.clear();
			return true;
		}
		if( actFrame==0 ) {
			if( actAnimation >= numberOfAnimations ) {
				ani.seek(8); // reset
				actAnimation = 0;
			}
			if( !readNextAnimation() ) {
				// there was an error
				nextAnimationUpdate = now+15000;
				hold = true;
				return false;
			}
		}

		uint16_t delay = readNextFrame(now, clockInFront & (actFrame >= clockFrom));
		nextAnimationUpdate = now + (delay>0?delay:refreshDelay);

		if( actFrame >= clockFrom ) {
			if( clockSmall ) {
				clock.setXoffset(xoffset);
				clock.setYoffset(yoffset);
				clock.setFont(Clock::Small);
			}
			clock.setMode( Clock::TIME ); // always choose simple time display, while ani playing

			// debug   panel.setPixel(0,0,true);
			clock.on();
			// if clock is in front force update
			clock.update(now);
			// write with mask / not only with update

		}
		// todo raus ziehen in den Handler
		if( actFrame >= numberOfFrames ) { // ende der ani, check auf cycle
			if(--cycles>0) {
				ani.seek(actFilePos);
				actFrame = 0;
				actAnimation--;
			} else {
				// gehe in den Hold status
				actFrame = 0;
				// TODO es darf nicht über die lange Zeit gewartet werden
				// sondern es werden die cycles gezählt, damit die Uhr
				// einen update bekommt
				nextAnimationUpdate = now + 200*holdCycles; // hold
				hold = true;
			}
		}
		if( seenMaskFrame && holdCycles > 0 ) {
			nextAnimationUpdate += 200*holdCycles;
			holdCycles = 0;
		}
	}
	return false;
}

Animation::~Animation() {
}

void Animation::setFskMode(int mode) {
	uint8_t fsk[] = {18,16,12,6};
	this->fskFilter = fsk[mode];
	DPRINTF("setting fsk to %d\n",fskFilter);
}
