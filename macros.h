/*
 * macros.h
 *
 *  Created on: 12.08.2015
 *      Author: sr
 */

#ifndef MACROS_H_
#define MACROS_H_

// day in millis
#define DAY 86400000

#include <stdint.h>

/*
 * defines an overflow save compare:
 * if now > when -> true
 * simply use difference
 */
#define SAVECMP( now, when ) ( (long)now - (long)when > 0 )

#endif /* MACROS_H_ */
