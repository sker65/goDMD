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
#include "debug.h"

NtpCallbackHandler::NtpCallbackHandler(Clock* clock,int tz) : NtpCallback() {
	this->clock = clock;
	this->tz = tz;
}

void NtpCallbackHandler::setUtcTime(uint32_t utcTime) {
	TimeUtil timeUtil;
	DPRINTF("utc timestamp received: %ld\n", utcTime);
	utcTime += tz * 3600;
	timeUtil.setUTCtime(utcTime);
	DPRINTF("timeutil: month: %d, day: %d, wday: %d, hour: %d\n",
			timeUtil.month, timeUtil.day, timeUtil.wday, timeUtil.hour);
	if( timeUtil.isDaylightSaving()) {
		DPRINTF2("timeutil: isDaylightSaving()->true\n");
		utcTime += 3600;
	}
	DateTime dt(utcTime);
	DPRINTF("dt: %d:%d:%d\n", dt.hour(), dt.minute(), dt.second());
	clock->adjust(&dt);
}
