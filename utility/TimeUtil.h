/*
 * TimeUtil.h
 *
 *  Created on: 09.08.2015
 *      Author: sr
 */

#ifndef TIMEUTIL_H_
#define TIMEUTIL_H_

#include <stdint.h>

class TimeUtil {
public:
	TimeUtil();
	virtual ~TimeUtil();

	uint8_t second;
	uint8_t minute;
	uint8_t hour;
	uint8_t day;
	uint8_t month;
	uint16_t year;
	uint8_t wday;
	int utcOffset;

	void setUTCtime(uint32_t sec);
	void doSummertimeAdjust();

	int getUtcOffset() const {
		return utcOffset;
	}

	void setUtcOffset(int utcOffset) {
		this->utcOffset = utcOffset;
	}

private:

};

#endif /* TIMEUTIL_H_ */
