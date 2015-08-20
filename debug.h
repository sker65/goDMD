/*
 * debug.h
 *
 *  Created on: 04.03.2015
 *      Author: sr
 */

#ifndef DEBUG_H_
#define DEBUG_H_

#ifdef _DEBUG
//{ printf( __FILE__ );printf( "\t" );printf( __func__ ); printf("\t"); }
#define DPRINTF(...)  printf ( __VA_ARGS__ )
#define DPRINT(...)  printf ( __VA_ARGS__ )

#else

#define DPRINTF(...)
#define DPRINT(...)

#endif


#endif /* DEBUG_H_ */
