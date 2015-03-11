/*
 * Animation.cpp
 *
 *  Created on: 01.02.2015
 *      Author: sr
 */

#include "Animation.h"
#include "debug.h"


Animation::Animation(SDClass& sd, LEDMatrixPanel& panel, Clock& clock) :
sd(sd), panel(panel), clock(clock) {
	actAnimation = 0;
	actFrame = 0;
	nextAnimationUpdate=0;
	numberOfAnimations=0;
	numberOfFrames=0;
	fskFilter = 18;
}

boolean Animation::begin() {
	ani = sd.open("/FOO.ANI");
	if( !ani ) return false;
	//delay(3000);
	// read 4 header ANIM
	ani.seekCur(4);
	// read 2 byte version (1)
	uint16_t version = ani.read()*256+ani.read();
	DPRINTF("version: %d\n",version);
	// read number of anis
	numberOfAnimations  = ani.read()*256+ani.read();
	DPRINTF("number of animations: %d\n",numberOfAnimations);
	// pos is now 8
	return true;
}

void readString(File& f) {
	char name[40];
	int len = f.read()*256+f.read();
	for(int i=0;i<40 && i < len;i++) {
		name[i] = f.read();
	}
	name[len] = '\0';
	DPRINTF("name: %s\n",name);
}

void Animation::readNextAnimation() {
	int c = 0;
	while(true) {
		actFilePos = ani.position();

		readString(ani);
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
		if( fsk <= fskFilter ) break;

		skipAllFrames(ani);
		if( actAnimation >= numberOfAnimations ) {
			ani.seek(8); // reset
			actAnimation = 0;
		}
		if( c++ >= numberOfAnimations) break;;
	}; // at least one must remain
}

void Animation::skipAllFrames(File& f) {
	for( int i = 0; i < numberOfFrames; i++) {
		DPRINTF("skipping frame: %d\n", i);
		uint16_t len = f.read()*256+f.read();
		byte buf[len];
		f.readBytes(buf, len);
		f.readBytes(buf, len);
	}
}

void Animation::readNextFrame(long now, bool mask) {
	uint16_t buflen = ani.read()*256+ani.read();
	if( panel.getSizeOfBufferInByte() == buflen ) {
		byte buf[buflen];
		int r = ani.readBytes(buf, buflen);
		if( mask ) clock.writeTime(now,buf);
		memmove((void*)panel.getBuffers()[0],buf,buflen);
		r += ani.readBytes(buf, buflen);
		if( mask ) clock.writeTime(now,buf);
		memmove((void*)panel.getBuffers()[1],buf,buflen);
		//      Serial.print("reading 2 buffers len: ");Serial.print(buflen);Serial.println(r);
		//		Serial.print("reading frame: ");Serial.println(actFrame);
	}
	actFrame++;
}

boolean Animation::update(long now) {
	if( now> nextAnimationUpdate) {
		if( hold ) {
			hold = false;
			clock.setFont(Clock::Big);
			panel.clearTime();
			panel.clear();
			return true;
		}
		nextAnimationUpdate = now + refreshDelay;
		if( actFrame==0 ) {
			if( actAnimation >= numberOfAnimations ) {
				ani.seek(8); // reset
				actAnimation = 0;
			}
			readNextAnimation();
		}

		readNextFrame(now, clockInFront);
		if( actFrame >= clockFrom ) {
			if( clockSmall ) {
				clock.setXoffset(xoffset);
				clock.setYoffset(yoffset);
				clock.setFont(Clock::Small);
			}

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
