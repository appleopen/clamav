/*
 *  Generic text normalizer.
 *
 *  Copyright (C) 2008 Sourcefire, Inc.
 *
 *  Authors: Török Edvin
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301, USA.
 */

#if HAVE_CONFIG_H
#include "clamav-config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "clamav.h"
#include "textnorm.h"

int text_normalize_init(struct text_norm_state *state, unsigned char *out, size_t out_len)
{
	if(!state) {
		return CL_ENULLARG;
	}
	state->out = out;
	state->out_len = out_len;
	state->out_pos = 0;
	state->space_written = 0;
	return CL_SUCCESS;
}

void text_normalize_reset(struct text_norm_state* state)
{
	state->out_pos = 0;
	state->space_written = 0;
}

enum normalize_action {
	NORMALIZE_COPY,
	NORMALIZE_SKIP,
	NORMALIZE_AS_WHITESPACE,
	NORMALIZE_ADD_32
};

/* use shorter names in the table */
#define IGN NORMALIZE_SKIP
#define WSP NORMALIZE_AS_WHITESPACE
#define A32 NORMALIZE_ADD_32
#define NOP NORMALIZE_COPY

/*
 * whitespace: \t, \n, \f, \v, \r, [ ]
 * nop: all characters 0x20 < c < 0x80, that are not A32 and WSP
 * tolowercase: all uppercase characters
 * ignore: control character < 0x20 that are not whitespace, and all > 0x7f
 */

static const enum normalize_action char_action[256] = {
	IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, WSP, WSP, WSP, WSP, WSP, IGN, IGN,
	IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN,
	WSP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,/* 0x20 - 0x2f */
	NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,
	NOP, A32, A32, A32, A32, A32, A32, A32, A32, A32, A32, A32, A32, A32, A32, A32,
        A32, A32, A32, A32, A32, A32, A32, A32, A32, A32, A32, NOP, NOP, NOP, NOP, NOP,
	NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,
	NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,/* 0x70 - 0x7f */
	IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN,
	IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN,
	IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN,
	IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN,
	IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN,
	IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN,
	IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN,
	IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN
};

/* Normalizes the text at @buf of length @buf_len, @buf can include \0 characters.
 * Stores the normalized text in @state's buffer. 
 * Returns how many bytes it consumed of the input. */
size_t text_normalize_buffer(struct text_norm_state *state, const unsigned char *buf, const size_t buf_len)
{
	size_t i;
	const unsigned char *out_end = state->out + state->out_len;
	unsigned char *p = state->out + state->out_pos;

	for(i=0; i < buf_len && p < out_end; i++) {
		unsigned char c = buf[i];
		switch(char_action[c]) {
			case NORMALIZE_SKIP:
				continue;
			case NORMALIZE_AS_WHITESPACE:
				/* convert consecutive whitespaces to a single space */
				if(!state->space_written) {
					*p++ = ' ';
				}
				state->space_written = 1;
				continue;
			case NORMALIZE_ADD_32:
				/* aka uppercase to lowercase */
				c += 32;
				/* fall through */
			case NORMALIZE_COPY:
				state->space_written = 0;
				*p++ = c;
		}
	}
	state->out_pos = p - state->out;
	return i;
}

