/* Copyright (C) 1994 Free Software Foundation

This file is part of the GNU BitString Library.  This library is free
software; you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the
Free Software Foundation; either version 2, or (at your option)
any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this library; see the file COPYING.  If not, write to the Free
Software Foundation, 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

As a special exception, if you link this library with files
compiled with a GNU compiler to produce an executable, this does not cause
the resulting executable to be covered by the GNU General Public License.
This exception does not however invalidate any other reasons why
the executable file might be covered by the GNU General Public License. */

/*  Written by Per Bothner (bothner@cygnus.com) */

#include "bitprims.h"

#if !defined(inline) && !defined(__GNUC__) && !defined(__cplusplus)
#define inline
#endif

#if 0
 /*
  * This function counts the number of bits in a long.
  * It is limited to 63 bit longs, but a minor mod can cope with 511 bits.
  *
  * It is so magic, an explanation is required:
  * Consider a 3 bit number as being
  *      4a+2b+c
  * if we shift it right 1 bit, we have
  *      2a+b
  * subtracting this from the original gives
  *      2a+b+c
  * if we shift the original 2 bits right we get
  *      a
  * and so with another subtraction we have
  *      a+b+c
  * which is the number of bits in the original number.
  * Suitable masking allows the sums of the octal digits in a
  * 32 bit number to appear in each octal digit.  This isn't much help
  * unless we can get all of them summed together.
  * This can be done by modulo arithmetic (sum the digits in a number by molulo
  * the base of the number minus one) the old "casting out nines" trick they
  * taught in school before calculators were invented.
  * Now, using mod 7 wont help us, because our number will very likely have
  * more than 7 bits set.  So add the octal digits together to get base64
  * digits, and use modulo 63.
  * (Those of you with 64 bit machines need to add 3 octal digits together to
  * get base512 digits, and use mod 511.)
  *
  * This is HAKMEM 169, as used in X11 sources.
  */

static inline int
_BS_count_word (word)
     register _BS_word n;
{
  register unsigned long tmp;

  tmp = n - ((n >> 1) & 033333333333) - ((n >> 2) & 011111111111);
  return ((tmp + (tmp >> 3)) & 030707070707) % 63;
}
#else
/* bit_count[I] is number of '1' bits in I. */
static const unsigned char
four_bit_count[16] = {
    0, 1, 1, 2,
    1, 2, 2, 3,
    1, 2, 2, 3,
    2, 3, 3, 4};

static inline int
_BS_count_word (word)
     register _BS_word word;
{
  register int count = 0;
  while (word > 0)
    {
      count += four_bit_count[word & 15];
      word >>= 4;
    }
  return count;
}
#endif

int
_BS_count (ptr, offset, length)
     register const _BS_word *ptr;
     int offset;
     _BS_size_t length;
{
  register int count = 0;
#undef DOIT
#define DOIT(WORD, MASK) count += _BS_count_word ((WORD) & (MASK));
#include "bitdo1.h"
  return count;
}

#if 0
From: Tommy.Thorn@irisa.fr (Tommy Thorn)
#ifdef __GNUC__
inline
#endif
static unsigned
bitcount(unsigned x)
{
  /* Clasical way to count set bits in a word, sligtly optimized */
  /* This takes 16 SPARC cycles, when done in a loop ;^)         */
  /* Assumes a 32-bit architecture                               */

  x =     (x >> 1  & 0x55555555) + (x & 0x55555555);
  x =    ((x >> 2) & 0x33333333) + (x & 0x33333333);
  x =    ((x >> 4) + x) & 0x0f0f0f0f;
  x =    ((x >> 8) + x);
  return (x + (x >> 16)) & 0xff;
}
#endif
