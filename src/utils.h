#include <stdio.h>
#include <stdint.h>

#ifndef REB_HOST_H
#define REB_HOST_H
#include "reb-host.h"
#endif

void string_utf8_to_rebol_unicode ( uint8_t* s, REBSER* rebstr );
int string_rebol_unicode_to_utf8 ( char *dest, int dest_size, REBSER *src );

