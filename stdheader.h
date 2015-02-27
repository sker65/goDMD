#ifndef STDHEADER_H_
#define STDHEADER_H_

// use this file to include to most common headers in an
// platform independent way

#ifdef __AVR__
#include <Arduino.h>
#include <avr/pgmspace.h>
#endif

#ifdef __PIC32MX__
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <WProgram.h>
#endif

#endif