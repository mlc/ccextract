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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "smpte.h"

smpte_t* smpte_new(int dropframe, int fps) {
    smpte_t* p = (smpte_t*)malloc(sizeof(smpte_t));

    if(!p)
	return NULL;

    memset(p, 0, sizeof(smpte_t));
    p->dropframe = dropframe;
    p->fps = fps;

    return p;
}

void smpte_free(smpte_t* smpte) {
    free(smpte);
}

void smpte_incr_frame(smpte_t* smpte) {
    smpte->frames++;

    if (smpte->frames == smpte->fps) {
	smpte->frames = 0;
	smpte->second++;

	if (smpte->second == 60) {
	    smpte->second = 0;
	    smpte->minute++;

	    if (smpte->dropframe && (smpte->minute % 10) != 0) {
		smpte->frames = 2;
	    }

	    if (smpte->minute == 60) {
		smpte->minute = 0;
		smpte->hour++;
	    }
	}
    }
}

int smpte_format(smpte_t* smpte, char* buf) {
    return snprintf(buf, SMPTE_STR_LEN, "%02hu:%02hu:%02hu%c%02hu",
		    smpte->hour,
		    smpte->minute,
		    smpte->second,
		    smpte->dropframe ? ';' : ':',
		    smpte->frames);
}
