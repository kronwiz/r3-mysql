#include "utils.h"

/*
Flexible and Economical UTF-8 Decoder
=====================================

License

Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.
*/

#define UTF8_ACCEPT 0
#define UTF8_REJECT 12

static const uint8_t utf8d[] = {
  // The first part of the table maps bytes to character classes that
  // to reduce the size of the transition table and create bitmasks.
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,  9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
   7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
   8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  10,3,3,3,3,3,3,3,3,3,3,3,3,4,3,3, 11,6,6,6,5,8,8,8,8,8,8,8,8,8,8,8,

  // The second part is a transition table that maps a combination
  // of a state of the automaton and a character class to a state.
   0,12,24,36,60,96,84,12,12,12,48,72, 12,12,12,12,12,12,12,12,12,12,12,12,
  12, 0,12,12,12,12,12, 0,12, 0,12,12, 12,24,12,12,12,12,12,24,12,24,12,12,
  12,12,12,12,12,12,12,24,12,12,12,12, 12,24,12,12,12,12,12,12,12,24,12,12,
  12,12,12,12,12,12,12,36,12,36,12,12, 12,36,12,12,12,12,12,36,12,36,12,12,
  12,36,12,12,12,12,12,12,12,12,12,12, 
};

inline uint32_t
decode(uint32_t* state, uint32_t* codep, uint32_t byte) {
  uint32_t type = utf8d[byte];

  *codep = (*state != UTF8_ACCEPT) ?
    (byte & 0x3fu) | (*codep << 6) :
    (0xff >> type) & (byte);

  *state = utf8d[256 + *state + type];
  return *state;
}

/* ======================== Decoder end =========================== */

void string_utf8_to_rebol_unicode ( uint8_t* s, REBSER* rebstr ) {
  uint32_t codepoint;
  uint32_t state = 0;

	//printf ( "C: printCodePoints: from = %s\n", s );

	int i = 0;
  for (; *s; ++s)
    if (!decode(&state, &codepoint, *s)) {
      //printf("i: %d -> U+%04X\n", i, codepoint);
			RL_SET_CHAR ( rebstr, i, codepoint );
			i++;
		}

  if (state != UTF8_ACCEPT)
    printf("The string is not well-formed\n");

}


/*
UTF8 Encoder
============

Got inspiration from: http://www.cprogramming.com/tutorial/unicode.html

by Jeff Bezanson
placed in the public domain Fall 2005
*/

/* string_rebol_unicode_to_utf8 ( char *dest, int dest_size, REBSER *src )

Converts a Rebol unicode string to a UTF8 encoded byte sequence.

Parameters:

- dest: destination buffer;
- dest_size: length in bytes of the destination buffer;
- src: Rebol unicode string.

Dest will only be '\0'-terminated if there is enough space.

Returns the actual utf8 string length in bytes.
*/
int string_rebol_unicode_to_utf8 ( char *dest, int dest_size, REBSER *src )
{
    int character;
    int i = 0;
		char *dest_start = dest;
    char *dest_end = dest + dest_size;

    while ( ( character = RL_GET_CHAR ( src, i++ ) ) != -1 ) {
        if ( character < 0x80 ) {
            if ( dest >= dest_end ) return i;
            *dest++ = (char) character;
        }
        else if ( character < 0x800 ) {
            if ( dest >= dest_end - 1 ) return i;
            *dest++ = ( character >> 6 ) | 0xC0;
            *dest++ = ( character & 0x3F ) | 0x80;
        }
        else if ( character < 0x10000 ) {
            if ( dest >= dest_end - 2 ) return i;
            *dest++ = ( character >> 12 ) | 0xE0;
            *dest++ = ( ( character >> 6 ) & 0x3F ) | 0x80;
            *dest++ = ( character & 0x3F ) | 0x80;
        }
        else if ( character < 0x110000 ) {
            if ( dest >= dest_end - 3 ) return i;
            *dest++ = ( character >> 18 ) | 0xF0;
            *dest++ = ( ( character >> 12 ) & 0x3F) | 0x80;
            *dest++ = ( ( character >> 6 ) & 0x3F) | 0x80;
            *dest++ = ( character & 0x3F ) | 0x80;
        }
    }
    if ( dest < dest_end ) *dest = '\0';
    return dest - dest_start;
}

