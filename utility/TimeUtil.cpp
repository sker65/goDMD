/*
 * TimeUtil.cpp
 *
 *  Created on: 09.08.2015
 *      Author: sr
 */

#include "TimeUtil.h"

#define FIRSTYEAR  2000      // start year
#define FIRSTDAY  6      // 0 = Sunday

const uint8_t DayOfMonth[] = { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

TimeUtil::TimeUtil() {
	this->day = 0;
	this->hour = 0;
	this->minute = 0;
	this->month = 0;
	this->second = 0;
	this->wday = 0;
	this->year = 0;
	this->utcOffset = 0;
}

TimeUtil::~TimeUtil() {
	// TODO Auto-generated destructor stub
}

void TimeUtil::doSummertimeAdjust() {
	uint8_t hour, day, wday, month;      // locals for faster access

	hour = this->hour;
	day = this->day;
	wday = this->wday;
	month = this->month;

	if (month < 3 || month > 10)      // month 1, 2, 11, 12
		return;          // -> Winter

	if (day - wday >= 25 && (wday || hour >= 2)) { // after last Sunday 2:00
		if (month == 10)        // October -> Winter
			return;
	} else {          // before last Sunday 2:00
		if (month == 3)        // March -> Winter
			return;
	}
	hour++;          // add one hour
	if (hour == 24) {        // next day
		hour = 0;
		wday++;          // next weekday
		if (wday == 7)
			wday = 0;
		if (day == DayOfMonth[month - 1]) {    // next month
			day = 0;
			month++;
		}
		day++;
	}
	this->month = month;
	this->hour = hour;
	this->day = day;
	this->wday = wday;
}

void TimeUtil::setUTCtime(uint32_t sec) {
	uint16_t day;
	uint8_t year;
	uint16_t dayofyear;
	uint8_t leap400;
	uint8_t month;

	sec += (int)utcOffset * 3600;

	this->second = sec % 60;
	sec /= 60;
	this->minute = sec % 60;
	sec /= 60;
	this->hour = sec % 24;
	day = sec / 24;

	this->wday = (day + FIRSTDAY) % 7;    // weekday

	year = FIRSTYEAR % 100;      // 0..99
	leap400 = 4 - ((FIRSTYEAR - 1) / 100 & 3);  // 4, 3, 2, 1

	for (;;) {
		dayofyear = 365;
		if ((year & 3) == 0) {
			dayofyear = 366;          // leap year
			if (year == 0 || year == 100 || year == 200)  // 100 year exception
				if (--leap400)          // 400 year exception
					dayofyear = 365;
		}
		if (day < dayofyear)
			break;
		day -= dayofyear;
		year++;          // 00..136 / 99..235
	}
	this->year = year + FIRSTYEAR / 100 * 100;  // + century

	if ((dayofyear & 1) && day > 58)    // no leap year and after 28.2.
		day++;          // skip 29.2.

	for (month = 1; day >= DayOfMonth[month - 1]; month++)
		day -= DayOfMonth[month - 1];

	this->month = month;        // 1..12
	this->day = day + 1;        // 1..31
}



