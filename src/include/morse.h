/*
 *    morse.h  --  morse code tables
 *
 *    Copyright (C) 2004
 *      Lawrence Glaister (ve7it@shaw.ca)
 *      Tomi Manninen (oh2bns@sral.fi)
 *
 *    This modem borrowed heavily from other gmfsk modems and
 *    also from the unix-cw project. I would like to thank those
 *    authors for enriching my coding experience by providing
 *    and supporting open source.
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

#ifndef _MORSE_H
#define _MORSE_H

#include <stdlib.h>
#include <string.h>
#include <ctype.h>


#define	MorseTableSize	256
 
#define CW_ENTRY_NULL		0
#define	CW_ENTRY_NORMAL		1
#define	CW_ENTRY_EXTENDED	2
#define	CW_DOT_REPRESENTATION	'.'
#define	CW_DASH_REPRESENTATION	'-'


struct CW_TABLE{
	const char *chr;	/* The character(s) represented */
	const char *rpr;	/* Dot-dash shape of the character */
	int type;	       	/* Type of the entry */
};

class morse {
private:
//	static CW_TABLE		cw_table[];
	CW_TABLE 			*cw_rx_lookup[256];
	unsigned long 		cw_tx_lookup[256];
	unsigned int 	tokenize_representation(const char *representation);
	bool init();
public:
	morse() { 
		init(); 
	}
	~morse() {
	}
	const char	*rx_lookup(char *r);
	unsigned long	tx_lookup(int c);
};


#endif
