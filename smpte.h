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

#ifndef __SMPTE_H
#define __SMPTE_H

typedef struct {
    unsigned short hour, minute, second, frames;
    int dropframe;
    int fps;
} smpte_t;

#define SMPTE_STR_LEN 12

smpte_t* smpte_new(int dropframe, int fps);
void smpte_free(smpte_t* smpte);
void smpte_incr_frame(smpte_t* smpte);
int smpte_format(smpte_t* smpte, char* buf);

#endif /* ndef __SMPTE_H */
