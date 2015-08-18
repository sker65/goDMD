/*
 * NtpCallbackHandler.h
 *
 *  Created on: 17.08.2015
 *      Author: sr
 */

#ifndef NTPCALLBACKHANDLER_H_
#define NTPCALLBACKHANDLER_H_

#include "NodeMcu.h"

class Clock;

class NtpCallbackHandler : public NtpCallback {
public:
	NtpCallbackHandler(Clock* clock, int tz = 0);
	virtual ~NtpCallbackHandler() {}
	virtual void setUtcTime(uint32_t utcTime);

	int getTz() const {
		return tz;
	}

	void setTz(int tz) {
		this->tz = tz;
	}

private:
	Clock* clock;
	int tz;
};

#endif /* NTPCALLBACKHANDLER_H_ */
