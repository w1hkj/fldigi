/*
 *    rttym.c  --  BAUDOT encoder/decoder
 *
 *    Copyright (C) 2001, 2002, 2003
 *      Tomi Manninen (oh2bns@sral.fi)
 *
 *    This file is part of gMFSK.
 *
 *    gMFSK is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    gMFSK is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with gMFSK; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <ctype.h>

#include "rttym.h"

static unsigned char letters[32] = {
	'·',    'T',    '\r',   'O',	' ',	'H',	'N',	'M',
	'\n',	'L',	'R',	'G',    'I',	'P',	'C',	'V',	
	'E',	'Z',	'D',	'B',    'S',	'Y',	'F',	'X',	
	'A',    'W',	'J',	'·',    'U',	'Q',	'K',	'\0' // idle
};

static unsigned char figures[32] = {
	'·',	'5',	'\r',	'9',	' ',	'#',	',',	'.',
	'\n',	')',	'4',	'&',	'8',	'0',	':',	'=',
	'3',	'+',	'$',	'?',	'\'',	'6',	'!',	'/',
	'-',	'2',	'@',	'·',	'7',	'1',	'(',	'\0'
};

int rttym_enc(unsigned char data)
{
	int mode = 0;
	int c	 = -1;
	int i;

	if (islower(data))
		data = toupper(data);

	for (i = 0; i < 32; ++ i) {
		if (data == letters[i]) {
			mode |= RTTYM_LETS;
			c = i;
		}
		if (data == figures[i]) {
			mode |= RTTYM_FIGS;
			c = i;
		}
		if (c != -1)
			return mode | c;
	}

	return -1;
}

int rttym_dec(int *mode, unsigned char data)
{
	switch (data) {
	case 0x00:		/* letters */
		*mode = RTTYM_LETS;
		return -1;
	case 0x1B:		/* figures */
		*mode = RTTYM_FIGS;
		return -1;
	case 0x04:		/* unshift-on-space */
		*mode = RTTYM_LETS;
		return ' ';
	default:
		return (*mode == RTTYM_LETS) ? letters[data] : figures[data];
	}
}
