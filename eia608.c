/*
 * EIA-608 Closed Caption Decoder Library
 * Copyright © 2007 Michael Castleman
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

/*
 * Primary reference:
 * Robson, Gary D., _The Closed Captioning Handbook_. ISBN 0240805615.
 */

#include <string.h>
#include <wchar.h>
#include <stdlib.h>
#include <assert.h>

#include "eia608.h"

/* the following constants are from Robson, p. 70 */
#define CC_RCL 0x20 /* resume caption loading */
#define CC_BS  0x21 /* backspace */
#define CC_AOF 0x22 /* not used */
#define CC_AON 0x23 /* not used */
#define CC_DER 0x24 /* delete to end of row */
#define CC_RU2 0x25 /* roll-up, 2 rows */
#define CC_RU3 0x26 /* roll-up, 3 rows */
#define CC_RU4 0x27 /* roll-up, 4 rows */
#define CC_FON 0x28 /* flash on */
#define CC_RDC 0x29 /* resume direct captioning */
#define CC_TR  0x2A /* text restart */
#define CC_RTD 0x2B /* resume text display */
#define CC_EDM 0x2C /* erase displayed memory */
#define CC_CR  0x2D /* carriage return */
#define CC_ENM 0x2E /* erase nondisplayed memory */
#define CC_EOC 0x2F /* end of caption */

struct __eia608_struct {
    int x,y;
    int wanted;
    int active;
    int cur_attribute;
    int in_back;
    wchar_t** display; wchar_t** back_display;
    int** attributes; int** back_attributes;
    int changed;
    int rolluplines;
    uint8_t last_b1, last_b2;
};

/* make sure to compile in UTF-8 mode for these to be interpreted correctly! */
static const wchar_t* basictab = L" !\"#$%&'()á+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[é]íóúabcdefghijklmnopqrstuvwxyzç÷Ññ█";

static const wchar_t* exttab1 = L"®°½¿™¢£♪à\0èâêîôû";
static const wchar_t* exttab2 = L"ÁÉÓÚÜü`¡*'–©℠•“”ÀÂÇÈÊËëÎÏïÔÙùÛ«»";
static const wchar_t* exttab3 = L"ÃãÍÌìÒòÕõ{}\\^_|~ÄäÖöß¥¤│ÅåØø┌┐└┘";

/* PAC byte 1 provides a hint as to which row we should move to */
/* may add one row depending on value of byte 2 */
/* these values are one less than in the Robson book because we number
   starting from zero rather than 1 */
static const int pac_lines[] = {10, 0, 2, 11, 13, 4, 6, 8};

/* parity-checking is fun! */
static const int eight_bit_parity[] = {0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 
1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 
1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 
1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 
0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 
1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 
1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 
1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 
1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 
1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 
1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 
1, 0, 1, 1, 0};

static inline void** alloc_2d_array(int rows, int columns, size_t size) {
    void** arr = calloc(rows, sizeof(void*));
    int i;

    for (i = 0; i < rows; ++i) {
	arr[i] = calloc(columns, size);
    }

    return arr;
}

static inline void free_2d_array(void* ptr, int rows) {
    int i;

    for (i = 0; i < rows; ++i) {
	free(((void**)ptr)[i]);
    }
    free(ptr);
}

eia608_t* eia608_new() {
    eia608_t* eia608 = malloc(sizeof(eia608_t));
    memset(eia608, 0, sizeof(eia608_t));

    eia608->wanted = EIA608_CC1;
    eia608->display = (wchar_t**)alloc_2d_array(EIA608_ROWS, EIA608_COLUMNS, sizeof(wchar_t));
    eia608->back_display = (wchar_t**)alloc_2d_array(EIA608_ROWS, EIA608_COLUMNS, sizeof(wchar_t));
    eia608->attributes = (int**)alloc_2d_array(EIA608_ROWS, EIA608_COLUMNS, sizeof(int));
    eia608->back_attributes = (int**)alloc_2d_array(EIA608_ROWS, EIA608_COLUMNS, sizeof(int));

    return eia608;
}

void eia608_free(eia608_t* eia608) {
    free_2d_array(eia608->display, EIA608_ROWS);
    free_2d_array(eia608->back_display, EIA608_ROWS);
    free_2d_array(eia608->attributes, EIA608_ROWS);
    free_2d_array(eia608->back_attributes, EIA608_ROWS);
    free(eia608);
}

int eia608_set_wanted(eia608_t* eia608, int wanted) {
    if ((wanted & 0xEC) == 0) {
	eia608->wanted = wanted;
	return eia608->wanted;
    }
    eia608->active = eia608->wanted & 0x2;
    return -1;
}

static void append_char(eia608_t* context, wchar_t ch) {
    if (context->in_back) {
	context->back_display[context->x][context->y] = ch;
	context->back_attributes[context->x][context->y] = context->cur_attribute;
    } else {
	context->display[context->x][context->y] = ch;
	context->attributes[context->x][context->y] = context->cur_attribute;
	context->changed = 1;
    }
    if (context->y < (EIA608_COLUMNS-1))
	context->y++;
}

static inline void backspace(eia608_t* context) {
    if (context->y > 0)
	context->y--;
}

/* interpret a preamble address code (PAC) */
/* this give a row to go to, and maybe a color, indent, or underline */
static void interpret_pac(eia608_t* context, uint8_t b1, uint8_t b2) {
    assert(b1 >= 0x10 && b1 <= 0x17);
    assert(b2 >= 0x40 && b2 <= 0x7f);

    context->x = pac_lines[b1 & 0x0f];

    if (b2 & 0x20) { /* codes from 0x60 to 0x7F indicate "next line" */
	context->x++;
    }

    if (b2 & 0x10) { /* codes from 0x50-0x5F and 0x70-0x7F are white indent */
	context->y = (b2 & 0x0E) << 1;
	context->cur_attribute = EIA608_WHITE;
    } else if ((b2 & 0x0E) == 0x0E) { /* italics */
	context->cur_attribute = EIA608_WHITE | EIA608_ITALIC;
    } else { /* color */
	context->cur_attribute = (b2 & 0x0E) >> 1;
    }

    if (b2 & 0x01) {
	context->cur_attribute |= EIA608_UNDERLINE;
    }
}

static void interpret_attribute(eia608_t* context, uint8_t attr) {
    if ((attr & 0xFE) == 0x2E) { /* italics are special */
	context->cur_attribute |= EIA608_ITALIC;
    } else {
	context->cur_attribute = (attr & 0xf) >> 1;
    }
    if (attr & 0x1) {
	context->cur_attribute = EIA608_UNDERLINE;
    }
}

static void swap_memories(eia608_t* context) {
    wchar_t **tmp_display;
    int **tmp_attributes;

    tmp_display = context->display;
    context->display = context->back_display;
    context->back_display = tmp_display;

    tmp_attributes = context->attributes;
    context->attributes = context->back_attributes;
    context->back_attributes = tmp_attributes;

    context->changed = 1;
}

static void clear_memories(wchar_t** display, int** attributes) {
    int i;

    for (i = 0; i < EIA608_ROWS; ++i) {
	wmemset(display[i], 0, EIA608_COLUMNS);
	memset(attributes[i], 0, EIA608_COLUMNS * sizeof(int));
    }
}

static void carriage_return(eia608_t* context) {
    int i;
    int row = context->x;

    for (i = 0; i < (context->rolluplines - 1); ++i) {
	wmemcpy(context->display[row-1], context->display[row], EIA608_COLUMNS);
	memcpy(context->attributes[row-1], context->attributes[row], EIA608_COLUMNS*sizeof(int));
    }
    memset(context->display[context->x], 0, sizeof(wchar_t)*EIA608_COLUMNS);
    memset(context->attributes[context->x], 0, sizeof(int)*EIA608_COLUMNS);
    context->changed = 1;
}

static void interpret_command(eia608_t* context, uint8_t command) {
    switch(command) {
    case CC_RCL:
	/* resume caption loading -- enter pop-on mode */
	context->in_back = 1;
	break;

    case CC_BS:
	/* back space */
	backspace(context);
	(context->in_back ? context->back_display : context->display)[context->x][context->y] = 0;
	if (!context->in_back)
	    context->changed = 1;
	break;

    case CC_AOF:
    case CC_AON:
	/* unused codes -- formerly Alarm Off and Alarm On */
	/* do nothing */
	break;

    case CC_DER:
	/* delete to end of row */
	wmemset(context->display[context->x] + context->y, 0,
		EIA608_COLUMNS - context->y);
	break;

    case CC_RU2:
    case CC_RU3:
    case CC_RU4:
	/* roll-up; 2, 3, or 4 rows */
	context->rolluplines = command - CC_RU2 + 2;
	context->in_back = 0;
	if (context->rolluplines > context->x)
	    context->x = 14;
	break;

    case CC_FON:
	/* flash on */
	/* ignore */
	break;

    case CC_RDC:
	/* resume direct captioning -- enter paint-on mode */
	context->in_back = 0;
	break;

    case CC_TR:
	/* text restart */
	/* text mode is more-or-less like roll-up mode with many lines */
	context->in_back = 0;
	context->rolluplines = 15;
	interpret_pac(context, 0x14, 0x60); /* reset cursor */
	break;

    case CC_RTD:
	/* resume text display */
	/* nothing to do here */
	break;

    case CC_EDM:
	/* erase displayed memory */
	clear_memories(context->display, context->attributes);
	context->changed = 1;
	break;

    case CC_CR:
	carriage_return(context);
	break;

    case CC_ENM:
	/* erase nondisplayed memory */
	clear_memories(context->back_display, context->back_attributes);
	break;

    case CC_EOC:
	/* end of caption */
	swap_memories(context);
	break;

    default:
	/* shouldn't happen */
	assert(0);
	break;
    }
}

void eia608_input(eia608_t* context, const uint8_t* bytes) {
    uint8_t input[2];

    /* look in field 1 for CC1, 2; field 2 for CC3,4 */
    memcpy(input, bytes + (context->wanted & 0x02 ? 2 : 0), 2);

    /* require odd parity on each byte */
    if (!(eight_bit_parity[input[0]] && eight_bit_parity[input[1]]))
        return;

    /* ok; that done, proceed to ignore parity */
    *((uint16_t*)input) &= 0x7f7f;

    if (input[0] >= 0x20 && (context->wanted == context->active)) {
	/* basic character(s) */
	append_char(context, basictab[input[0] - 0x20]);
	if (input[1] >= 0x20)
	    append_char(context, basictab[input[1] - 0x20]);

	context->last_b1 = context->last_b2 = 0;
    } else if (input[0] >= 0x10 && input[0] < 0x20) {
	if (input[0] == context->last_b1 && input[1] == context->last_b2) {
	    context->last_b1 = context->last_b2 = 0;
	    return;
	}
	context->last_b1 = input[0];
	context->last_b2 = input[1];

	/* first determine if the command should make us switch modes. */
	if (input[0] & 0x08) { /* CC2, CC4, TEXT2, TEXT4 */
	    context->active |= 0x01;
	    input[0] &= ~0x08;
	} else {
	    context->active &= ~0x01;
	}
	
	if (input[0] == 0x14) {
	    switch(input[1]) {
	    case CC_RCL: /* pop-on */
	    case CC_RU2: /* roll-up */
	    case CC_RU3: /* roll-up */
	    case CC_RU4: /* roll-up */
	    case CC_RDC: /* paint-on */
		context->active &= ~0x10; /* CC mode */
		break;
	    case CC_TR:
	    case CC_RTD:
		context->active |= 0x10; /* TEXT mode */
		break;
	    }
	}

	/* ok, now proceed iff the mode is right */
	if (context->active == context->wanted) {
	    if (input[0] >= 0x10 && input[0] <= 0x17 && input[1] >= 0x40) {
		interpret_pac(context, input[0], input[1]);
	    } else if (input[0] == 0x11 && input[1] >= 0x30 && input[1] <= 0x3F) {
		append_char(context, exttab1[input[1] - 0x30]);
	    } else if (input[0] == 0x12 && input[1] >= 0x20 && input[1] <= 0x3F) {
		backspace(context);
		append_char(context, exttab2[input[1] - 0x20]);
	    } else if (input[0] == 0x13 && input[1] >= 0x20 && input[1] <= 0x3F) {
		backspace(context);
		append_char(context, exttab3[input[1] - 0x20]);
	    } else if (input[0] == 0x11 && input[1] >= 0x20 && input[1] <= 0x2F) {
		interpret_attribute(context, input[1]);
	    } else if (input[0] == 0x17 && input[1] >= 0x21 && input[1] <= 0x23) {
		/* TO1, TO2, TO3: Tab offset, 1-3 columns */
		context->y += (input[1] & 0x03);
	    } else if (input[0] == 0x14 && input[1] >= 0x20 && input[1] <= 0x2F) {
		interpret_command(context, input[1]);
	    }
	}
    }
}

int eia608_has_changed(eia608_t* context) {
    int tmp = context->changed;
    context->changed = 0;
    return tmp;
}

wchar_t** eia608_get_screen(eia608_t* context) {
    return context->display;
}

int** eia608_get_attributes(eia608_t* context) {
    return context->attributes;
}

/*
 * Local variables:
 *  coding: utf-8
 * End:
 */
