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


/*
 * defines an overflow save compare:
 * if now > when -> true
 * if not now must have had an overflow, so decrement when by ULONG_MAX
 * and compare again
 */
#define SAVECMP( now, when ) ( now > when )

#endif /* MACROS_H_ */
