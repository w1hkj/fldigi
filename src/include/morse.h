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
 *    This file is part of fldigi.
 *
 *    Fldigi is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    Fldigi is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with fldigi.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _MORSE_H
#define _MORSE_H

#define	MorseTableSize	256
 
#define	CW_DOT_REPRESENTATION	'.'
#define	CW_DASH_REPRESENTATION	'-'


struct CW_TABLE {
	char chr;	/* The character(s) represented */
	const char *prt;	/* The printable representation of the character */
	const char *rpr;	/* Dot-dash shape of the character */
};

struct CW_XMT_TABLE {
	unsigned long code;
	const    char *prt;
};

class cMorse {
private:
	CW_TABLE 		*cw_rx_lookup[256];
	CW_XMT_TABLE 	cw_tx_lookup[256];
	unsigned int 	tokenize_representation(const char *representation);
public:
	cMorse() { 
		init(); 
	}
	~cMorse() {
	}
	void init();
	const char	*rx_lookup(char *r);
	unsigned long	tx_lookup(int c);
	const char *tx_print(int c);
};

#endif
