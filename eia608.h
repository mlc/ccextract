/*
 * EIA-608 Closed Caption Decoder Library
 * Copyright 2007 Michael Castleman
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __EIA608_H
#define __EIA608_H

#include <inttypes.h>
#include <wchar.h>

typedef struct __eia608_struct eia608_t;

/* constants to control which stream you are interested in decoding */
#define EIA608_CC1   0x00 /* default */
#define EIA608_CC2   0x01
#define EIA608_CC3   0x02
#define EIA608_CC4   0x03
#define EIA608_TEXT1 0x10
#define EIA608_TEXT2 0x11
#define EIA608_TEXT3 0x12
#define EIA608_TEXT4 0x13

/* an attribute consists of a color, optionally OR'd with UNDERLINE
   and/or ITALIC */
#define EIA608_WHITE     0x00
#define EIA608_GREEN     0x01
#define EIA608_BLUE      0x02
#define EIA608_CYAN      0x03
#define EIA608_RED       0x04
#define EIA608_YELLOW    0x05
#define EIA608_MAGENTA   0x06
#define EIA608_UNDERLINE 0x10
#define EIA608_ITALIC    0x20

/* screen size -- specified by the standard */
#define EIA608_ROWS      15
#define EIA608_COLUMNS   32

#ifdef __cplusplus
extern "C" {
#endif

/* create and initialize a new EIA-608 decoder object */
eia608_t* eia608_new();

/* free memory used by a previously created decoder */
void eia608_free(eia608_t* eia608);

/* choose the CC/TEXT stream in which we are interested */
int eia608_set_wanted(eia608_t* eia608, int wanted);

/* input four bytes (two for each field) of data */
void eia608_input(eia608_t* eia608, const uint8_t* bytes);

/* returns non-zero iff the screen is different since the last time
   this function was called */
int eia608_has_changed(eia608_t* eia608);

/* return the current screen and attributes */
wchar_t** eia608_get_screen(eia608_t* eia608);
int** eia608_get_attributes(eia608_t* eia608);

#ifdef __cplusplus
}
#endif

#endif /* ndef __EIA608_H */
