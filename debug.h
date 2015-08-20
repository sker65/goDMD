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
#define DPRINTF2( msg )  printf ( "%s %s:\t" msg, __FILE__, __func__ )
#define DPRINTF( fmt, ... )  printf ( "%s %s:\t" fmt, __FILE__, __func__, __VA_ARGS__ )
#define DPRINT(...)  printf ( __VA_ARGS__ )

#else

#define DPRINTF(...)
#define DPRINT(...)

#endif


#endif /* DEBUG_H_ */
