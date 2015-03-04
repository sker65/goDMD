/*
 * debug.h
 *
 *  Created on: 04.03.2015
 *      Author: sr
 */

#ifndef DEBUG_H_
#define DEBUG_H_

#ifdef _DEBUG

#define DPRINTF(...) printf ( __VA_ARGS__)

#else

#define DPRINTF(...)

#endif


#endif /* DEBUG_H_ */
