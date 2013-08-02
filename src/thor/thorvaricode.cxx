// ----------------------------------------------------------------------------
//
//	thorvaricode.cxx  --  THOR Varicode
//
// Copyright (C) 2008
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.
//
// Fldigi is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#include <config.h>

#include "mfskvaricode.h"
#include "thorvaricode.h"

// THOR varicode is an extended set of the IZ8BLY MFSK varicode that uses the
// unallocated remaining 12 bit codes for a secondary character set.

// Primary character set (same as MFSK)

// extended 12 bit codes for secondary characters
// 90 used, leaving 10 for possible special use

// encoding table

static const char *thor_varicode[] = {
	"101110000000",		/* 032 - <SPC>	*/ 
	"101110100000",		/* 033 - !	*/
	"101110101000",		/* 034 - '"'	*/
	"101110101100",		/* 035 - #	*/
	"101110110000",		/* 036 - $	*/
	"101110110100",		/* 037 - %	*/
	"101110111000",		/* 038 - &	*/
	"101110111100",		/* 039 - '	*/
	"101111000000",		/* 040 - (	*/
	"101111010000",		/* 041 - )	*/
	"101111010100",		/* 042 - *	*/
	"101111011000",		/* 043 - +	*/
	"101111011100",		/* 044 - ,	*/
	"101111100000",		/* 045 - -	*/
	"101111101000",		/* 046 - .	*/
	"101111101100",		/* 047 - /	*/
	"101111110000",		/* 048 - 0	*/
	"101111110100",		/* 049 - 1	*/
	"101111111000",		/* 050 - 2	*/
	"101111111100",		/* 051 - 3	*/
	"110000000000",		/* 052 - 4	*/
	"110100000000",		/* 053 - 5	*/
	"110101000000",		/* 054 - 6	*/
	"110101010100",		/* 055 - 7	*/
	"110101011000",		/* 056 - 8	*/
	"110101011100",		/* 057 - 9	*/
	"110101100000",		/* 058 - :	*/
	"110101101000",		/* 059 - ;	*/
	"110101101100",		/* 060 - <	*/
	"110101110000",		/* 061 - =	*/
	"110101110100",		/* 062 - >	*/
	"110101111000",		/* 063 - ?	*/
	"110101111100",		/* 064 - @	*/
	"110110000000",		/* 065 - A	*/
	"110110100000",		/* 066 - B	*/
	"110110101000",		/* 067 - C	*/
	"110110101100",		/* 068 - D	*/
	"110110110000",		/* 069 - E	*/
	"110110110100",		/* 070 - F	*/
	"110110111000",		/* 071 - G	*/
	"110110111100",		/* 072 - H	*/
	"110111000000",		/* 073 - I	*/
	"110111010000",		/* 074 - J	*/
	"110111010100",		/* 075 - K	*/
	"110111011000",		/* 076 - L	*/
	"110111011100",		/* 077 - M	*/
	"110111100000",		/* 078 - N	*/
	"110111101000",		/* 079 - O	*/
	"110111101100",		/* 080 - P	*/
	"110111110000",		/* 081 - Q	*/
	"110111110100",		/* 082 - R	*/
	"110111111000",		/* 083 - S	*/
	"110111111100",		/* 084 - T	*/
	"111000000000",		/* 085 - U	*/
	"111010000000",		/* 086 - V	*/
	"111010100000",		/* 087 - W	*/
	"111010101100",		/* 088 - X	*/
	"111010110000",		/* 089 - Y	*/
	"111010110100",		/* 090 - Z	*/
	"111010111000",		/* 091 - [	*/
	"111010111100",		/* 092 - \	*/
	"111011000000",		/* 093 - ]	*/
	"111011010000",		/* 094 - ^	*/
	"111011010100",		/* 095 - _	*/
	"111011011000",		/* 096 - `	*/
	"111011011100",		/* 097 - a	*/
	"111011100000",		/* 098 - b	*/
	"111011101000",		/* 099 - c	*/
	"111011101100",		/* 100 - d	*/
	"111011110000",		/* 101 - e	*/
	"111011110100",		/* 102 - f	*/
	"111011111000",		/* 103 - g	*/
	"111011111100",		/* 104 - h	*/
	"111100000000",		/* 105 - i	*/
	"111101000000",		/* 106 - j	*/
	"111101010000",		/* 107 - k	*/
	"111101010100",		/* 108 - l	*/
	"111101011000",		/* 109 - m	*/
	"111101011100",		/* 110 - n	*/
	"111101100000",		/* 111 - o	*/
	"111101101000",		/* 112 - p	*/
	"111101101100",		/* 113 - q	*/
	"111101110000",		/* 114 - r	*/
	"111101110100",		/* 115 - s	*/
	"111101111000",		/* 116 - t	*/
	"111101111100",		/* 117 - u	*/
	"111110000000",		/* 118 - v	*/
	"111110100000",		/* 119 - w	*/
	"111110101000",		/* 120 - x	*/
	"111110101100",		/* 121 - y	*/
	"111110110000"		/* 122 - z	*/
};

// unused 12 bit varicodes
/*
static char *unused[] = {
	"111110110100",
	"111110111000",
	"111110111100",
	"111111000000",
	"111111010100",
	"111111011000",
	"111111011100",
	"111111100000",
	"111111101000",
	"111111101100",
	"111111110000",
	"111111110100",
	"111111111100"
};
*/

// decoding table

static const unsigned int thor_varidecode[] = {
	0xB80, 0xBA0, 0xBA8, 0xBAC, 0xBB0, 0xBB4, 0xBB8, 0xBBC,
	0xBC0, 0xBD0, 0xBD4, 0xBD8, 0xBDC, 0xBE0, 0xBE8, 0xBEC,
	0xBF0, 0xBF4, 0xBF8, 0xBFC, 0xC00, 0xD00, 0xD40, 0xD54,
	0xD58, 0xD5C, 0xD60, 0xD68, 0xD6C, 0xD70, 0xD74, 0xD78,
	0xD7C, 0xD80, 0xDA0, 0xDA8, 0xDAC, 0xDB0, 0xDB4, 0xDB8,
	0xDBC, 0xDC0, 0xDD0, 0xDD4, 0xDD8, 0xDDC, 0xDE0, 0xDE8,
	0xDEC, 0xDF0, 0xDF4, 0xDF8, 0xDFC, 0xE00, 0xE80, 0xEA0,
	0xEAC, 0xEB0, 0xEB4, 0xEB8, 0xEBC, 0xEC0, 0xED0, 0xED4,
	0xED8, 0xEDC, 0xEE0, 0xEE8, 0xEEC, 0xEF0, 0xEF4, 0xEF8,
	0xEFC, 0xF00, 0xF40, 0xF50, 0xF54, 0xF58, 0xF5C, 0xF60,
	0xF68, 0xF6C, 0xF70, 0xF74, 0xF78, 0xF7C, 0xF80, 0xFA0,
	0xFA8, 0xFAC, 0xFB0
};
static int limit = sizeof(thor_varidecode)/sizeof(unsigned int);

const char *thorvarienc(int c, int sec)
{
	if (sec == 0)
		return varienc(c);       // mfsk varicode
	else
		if (c >= ' ' && c <= 'z')
			return thor_varicode[c - ' '];
		
	return varienc(0);        // return code for NULL if not in tables
}

int thorvaridec(unsigned int symbol)
{
	int i;

	if (symbol < 0xB80)
		return varidec(symbol);  // find in the MFSK decode table	

	for (i = 0; i < limit; i++)
		if (symbol == thor_varidecode[i])
			return (' ' + i + 0x100);  // found in the extended decode table
			
	return -1;                   // not found
}

