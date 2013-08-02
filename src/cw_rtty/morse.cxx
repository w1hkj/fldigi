/*
 *    morse.c  --  morse code tables
 *
 *    Copyright (C) 2004
 *      Lawrence Glaister (ve7it@shaw.ca)
 *    This modem borrowed heavily from other gmfsk modems and
 *    also from the unix-cw project. I would like to thank those
 *    authors for enriching my coding experience by providing
 *    and supporting open source.
 *
 *    This file is part of fldigi. Copied from the gMFSK source code
 *    distribution.
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

#include <config.h>

#include <cstring>

#include "morse.h"
#include "configuration.h"

/* ---------------------------------------------------------------------- */

/*
 * Morse code characters table.  This table allows lookup of the Morse
 * shape of a given alphanumeric character.  Shapes are held as a string,
 * with '-' representing dash, and '.' representing dot.  The table ends
 * with a NULL entry.
 *
 * This is the main table from which the other tables are computed.
 * 
 * The Prosigns are also defined in the configuration.h file
 * The user can specify the character which substitutes for the prosign
 */

static CW_TABLE cw_table[] = {
	/* Prosigns */
	{'=',	"<BT>",   "-...-"	}, // 0
	{'~',	"<AA>",   ".-.-"	}, // 1
	{'<',	"<AS>",   ".-..."	}, // 2
	{'>',	"<AR>",   ".-.-."	}, // 3
	{'%',	"<SK>",   "...-.-"	}, // 4
	{'+',	"<KN>",   "-.--."	}, // 5
	{'&',	"<INT>",  "..-.-"	}, // 6
	{'{',	"<HM>",   "....--"	}, // 7
	{'}',	"<VE>",   "...-."	}, // 8
	/* ASCII 7bit letters */
	{'A',	"A",	".-"	},
	{'B',	"B",	"-..."	},
	{'C',	"C",	"-.-."	},
	{'D',	"D",	"-.."	},
	{'E',	"E",	"."		},
	{'F',	"F",	"..-."	},
	{'G',	"G",	"--."	},
	{'H',	"H",	"...."	},
	{'I',	"I",	".."	},
	{'J',	"J",	".---"	},
	{'K',	"K",	"-.-"	},
	{'L',	"L",	".-.."	},
	{'M',	"M",	"--"	},
	{'N',	"N",	"-."	},
	{'O',	"O",	"---"	},
	{'P',	"P",	".--."	},
	{'Q',	"Q",	"--.-"	},
	{'R',	"R",	".-."	},
	{'S',	"S",	"..."	},
	{'T',	"T",	"-"		},
	{'U',	"U",	"..-"	},
	{'V',	"V",	"...-"	},
	{'W',	"W",	".--"	},
	{'X',	"X",	"-..-"	},
	{'Y',	"Y",	"-.--"	},
	{'Z',	"Z",	"--.."	},
	/* Numerals */
	{'0',	"0",	"-----"	},
	{'1',	"1",	".----"	},
	{'2',	"2",	"..---"	},
	{'3',	"3",	"...--"	},
	{'4',	"4",	"....-"	},
	{'5',	"5",	"....."	},
	{'6',	"6",	"-...."	},
	{'7',	"7",	"--..."	},
	{'8',	"8",	"---.."	},
	{'9',	"9",	"----."	},
	/* Punctuation */
	{'\\',	"\\",	".-..-."	},
	{'\'',	"'",	".----."	},
	{'$',	"$",	"...-..-"	},
	{'(',	"(",	"-.--."		},
	{')',	")",	"-.--.-"	},
	{',',	",",	"--..--"	},
	{'-',	"-",	"-....-"	},
	{'.',	".",	".-.-.-"	},
	{'/',	"/",	"-..-."		},
	{':',	":",	"---..."	},
	{';',	";",	"-.-.-."	},
	{'?',	"?",	"..--.."	},
	{'_',	"_",	"..--.-"	},
	{'@',	"@",	".--.-."	},
	{'!',	"!",	"-.-.--"	},
	{0, NULL, NULL}
};

// ISO 8859-1 accented characters
//	{"\334\374",	"\334\374",	"..--"},	// U diaeresis
//	{"\304\344",	"\304\344",	".-.-"},	// A diaeresis
//	{"\307\347",	"\307\347",	"-.-.."},	// C cedilla 
//	{"\326\366",	"\325\366",	"---."},	// O diaeresis
//	{"\311\351",	"\311\351",	"..-.."},	// E acute
//	{"\310\350",	"\310\350",".-..-"},	// E grave
//	{"\305\345",	"\305\345",	".--.-"},	// A ring
//	{"\321\361",	"\321\361",	"--.--"},	// N tilde
//	ISO 8859-2 accented characters
//	{"\252",		"\252",		"----" },	// S cedilla
//	{"\256",		"\256",		"--..-"},	// Z dot above


/**
 * cw_tokenize_representation()
 *
 * Return a token value, in the range 2-255, for a lookup table representation.
 * The routine returns 0 if no valid token could be made from the string.  To
 * avoid casting the value a lot in the caller (we want to use it as an array
 * index), we actually return an unsigned int.
 *
 * This token algorithm is designed ONLY for valid CW representations; that is,
 * strings composed of only '.' and '-', and in this case, strings shorter than
 * eight characters.  The algorithm simply turns the representation into a
 * 'bitmask', based on occurrences of '.' and '-'.  The first bit set in the
 * mask indicates the start of data (hence the 7-character limit).  This mask
 * is viewable as an integer in the range 2 (".") to 255 ("-------"), and can
 * be used as an index into a fast lookup array.
 */
unsigned int cMorse::tokenize_representation(const char *representation)
{
	unsigned int token;	/* Return token value */
	const char *sptr;	/* Pointer through string */

	/*
	 * Our algorithm can handle only 6 characters of representation.
	 * And we insist on there being at least one character, too.
	 */
	if (strlen(representation) > 6 || strlen(representation) < 1)
		return 0;

	/*
	 * Build up the token value based on the dots and dashes.  Start the
	 * token at 1 - the sentinel (start) bit.
	 */
	for (sptr = representation, token = 1; *sptr != 0; sptr++) {
		/* 
		 * Left-shift the sentinel (start) bit.
		 */
		token <<= 1;

		/*
		 * If the next element is a dash, OR in another bit.  If it is
		 * not a dash or a dot, then there is an error in the repres-
		 * entation string.
		 */
		if (*sptr == CW_DASH_REPRESENTATION)
			token |= 1;
		else if (*sptr != CW_DOT_REPRESENTATION)
			return 0;
	}

	/* Return the value resulting from our tokenization of the string. */
	return token;
}

/* ---------------------------------------------------------------------- */

void cMorse::init()
{
	CW_TABLE *cw;	/* Pointer to table entry */
	unsigned int i;
	long code;
	int len;
// Update the char / prosign relationship
	if (progdefaults.CW_prosigns.length() == 9) {
		for (int i = 0; i < 9; i++) {
			cw_table[i].chr = progdefaults.CW_prosigns[i];
		}
	}
// Clear the RX & TX tables
	for (i = 0; i < MorseTableSize; i++) {
		cw_tx_lookup[i].code = 0x04;
		cw_tx_lookup[i].prt = 0;
		cw_rx_lookup[i] = 0;
	}
// For each main table entry, create a token entry.
	for (cw = cw_table; cw->chr != 0; cw++) {
		if ((cw->chr == '(') && !progdefaults.CW_use_paren) continue;
		if ((cw->chr == '<') && progdefaults.CW_use_paren) continue;
		i = tokenize_representation(cw->rpr);
		if (i != 0)
			cw_rx_lookup[i] = cw;
	}
// Build TX table 
	for (cw = cw_table; cw->chr != 0; cw++) {
		if ((cw->chr == '(') && !progdefaults.CW_use_paren) continue;
		if ((cw->chr == '<') && progdefaults.CW_use_paren) continue;
		len = strlen(cw->rpr);
		code = 0x04;
		while (len-- > 0) {
			if (cw->rpr[len] == CW_DASH_REPRESENTATION) {
				code = (code << 1) | 1;
				code = (code << 1) | 1;
				code = (code << 1) | 1;
			} else
				code = (code << 1) | 1;
			code <<= 1;
		}
		cw_tx_lookup[(int)cw->chr].code = code;
		cw_tx_lookup[(int)cw->chr].prt = cw->prt;
	}
}

const char *cMorse::rx_lookup(char *r)
{
	int			token;
	CW_TABLE *cw;

	if ((token = tokenize_representation(r)) == 0)
		return NULL;

	if ((cw = cw_rx_lookup[token]) == NULL)
		return NULL;

	return cw->prt;
}

const char *cMorse::tx_print(int c)
{
	if (cw_tx_lookup[toupper(c)].prt)
		return cw_tx_lookup[toupper(c)].prt;
	else
		return "";
}

unsigned long cMorse::tx_lookup(int c)
{
	return cw_tx_lookup[toupper(c)].code;
}


/* ---------------------------------------------------------------------- */

