/*
 * NtpCallbackHandler.cpp
 *
 *  Created on: 17.08.2015
 *      Author: sr
 */

#include "NtpCallbackHandler.h"
#include "Clock.h"
#include <RTClib.h>
#include "utility/TimeUtil.h"

NtpCallbackHandler::NtpCallbackHandler(Clock* clock,int tz) : NtpCallback() {
	this->clock = clock;
	this->tz = tz;
}

void NtpCallbackHandler::setUtcTime(uint32_t utcTime) {
	TimeUtil timeUtil;
	timeUtil.setUTCtime(utcTime);
	utcTime += tz * 3600*24;
	if( timeUtil.isDaylightSaving()) {
		utcTime += 3600*24;
	}
	DateTime dt(utcTime);
	clock->adjust(&dt);
}
