/*
 *    morse.h  --  morse code tables
 *
 *    Copyright (C) 2017
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

#include <string>

#define	MorseTableSize	256
 
#define	CW_DOT_REPRESENTATION	'.'
#define	CW_DASH_REPRESENTATION	'-'

struct CWstruct {
	bool			enabled;	// true if character is active
	std::string		chr;		// utf-8 string representation of character
	std::string		prt;		// utf-8 printable representation
	std::string		rpr;		// Dot-dash code representation
};

class cMorse {
private:
	static CWstruct	cw_table[];
	std::string utf8;
	std::string toprint;
	int ptr;
public:
	cMorse() { 
		init(); 
	}
	~cMorse() {
	}
	void init();
	void enable(std::string, bool);
	std::string		rx_lookup(std::string);
	std::string		tx_lookup(int);
	std::string		tx_print() { return toprint; }
	int  tx_length(int);
};

#endif
