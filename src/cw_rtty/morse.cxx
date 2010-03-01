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

/* ---------------------------------------------------------------------- */

/*
 * Morse code characters table.  This table allows lookup of the Morse
 * shape of a given alphanumeric character.  Shapes are held as a string,
 * with '-' representing dash, and '.' representing dot.  The table ends
 * with a NULL entry.
 *
 * This is the main table from which the other tables are computed.
 */


static CW_TABLE cw_table[] = {
	/* ASCII 7bit letters */
	{"Aa",		".-",		CW_ENTRY_NORMAL},
	{"Bb",		"-...",		CW_ENTRY_NORMAL},
	{"Cc",		"-.-.",		CW_ENTRY_NORMAL},
	{"Dd",		"-..",		CW_ENTRY_NORMAL},
	{"Ee",		".",		CW_ENTRY_NORMAL},
	{"Ff",		"..-.",		CW_ENTRY_NORMAL},
	{"Gg",		"--.",		CW_ENTRY_NORMAL},
	{"Hh",		"....",		CW_ENTRY_NORMAL},
	{"Ii",		"..",		CW_ENTRY_NORMAL},
	{"Jj",		".---",		CW_ENTRY_NORMAL},
	{"Kk",		"-.-",		CW_ENTRY_NORMAL},
	{"Ll",		".-..",		CW_ENTRY_NORMAL},
	{"Mm",		"--",		CW_ENTRY_NORMAL},
	{"Nn",		"-.",		CW_ENTRY_NORMAL},
	{"Oo",		"---",		CW_ENTRY_NORMAL},
	{"Pp",		".--.",		CW_ENTRY_NORMAL},
	{"Qq",		"--.-",		CW_ENTRY_NORMAL},
	{"Rr",		".-.",		CW_ENTRY_NORMAL},
	{"Ss",		"...",		CW_ENTRY_NORMAL},
	{"Tt",		"-",		CW_ENTRY_NORMAL},
	{"Uu",		"..-",		CW_ENTRY_NORMAL},
	{"Vv",		"...-",		CW_ENTRY_NORMAL},
	{"Ww",		".--",		CW_ENTRY_NORMAL},
	{"Xx",		"-..-",		CW_ENTRY_NORMAL},
	{"Yy",		"-.--",		CW_ENTRY_NORMAL},
	{"Zz",		"--..",		CW_ENTRY_NORMAL},
	/* Numerals */
	{"0",		"-----",	CW_ENTRY_NORMAL},
	{"1",		".----",	CW_ENTRY_NORMAL},
	{"2",		"..---",	CW_ENTRY_NORMAL},
	{"3",		"...--",	CW_ENTRY_NORMAL},
	{"4",		"....-",	CW_ENTRY_NORMAL},
	{"5",		".....",	CW_ENTRY_NORMAL},
	{"6",		"-....",	CW_ENTRY_NORMAL},
	{"7",		"--...",	CW_ENTRY_NORMAL},
	{"8",		"---..",	CW_ENTRY_NORMAL},
	{"9",		"----.",	CW_ENTRY_NORMAL},
	/* Punctuation */
	{"\"",		".-..-.",	CW_ENTRY_NORMAL},
	{"'",		".----.",	CW_ENTRY_NORMAL},
	{"$",		"...-..-",	CW_ENTRY_NORMAL},
	{"(",		"-.--.",	CW_ENTRY_NORMAL},
	{")",		"-.--.-",	CW_ENTRY_NORMAL},
//	{"+",		".-.-.",	CW_ENTRY_NORMAL},
	{",",		"--..--",	CW_ENTRY_NORMAL},
	{"-",		"-....-",	CW_ENTRY_NORMAL},
	{".",		".-.-.-",	CW_ENTRY_NORMAL},
	{"/",		"-..-.",	CW_ENTRY_NORMAL},
	{":",		"---...",	CW_ENTRY_NORMAL},
	{";",		"-.-.-.",	CW_ENTRY_NORMAL},
	{"=",		"-...-",	CW_ENTRY_NORMAL},
	{"?",		"..--..",	CW_ENTRY_NORMAL},
	{"_",		"..--.-",	CW_ENTRY_NORMAL},
	{"@",		".--.-.",	CW_ENTRY_NORMAL},
	{"!",		"-.-.--",	CW_ENTRY_NORMAL},
	/* special characters */
	{"~",     ".-.-",       CW_ENTRY_NORMAL},
	{"<AA>",  ".-.-",       CW_ENTRY_EXTENDED},
	{"%",     ".-...",      CW_ENTRY_NORMAL},
	{"<AS>",  ".-...",      CW_ENTRY_EXTENDED},
	{"^",     "....--",     CW_ENTRY_NORMAL},
	{"<HM>",  "....--",     CW_ENTRY_EXTENDED},
	{"&",     "..-.-",      CW_ENTRY_NORMAL},
	{"<INT>", "..-.-",      CW_ENTRY_EXTENDED},
	{">",     "...-.-", 	CW_ENTRY_NORMAL},
	{"<SK>",  "...-.-",     CW_ENTRY_EXTENDED},
//	{"<",     "-.--.",  	CW_ENTRY_NORMAL},
//	{"<KN>",	"-.--.",	CW_ENTRY_EXTENDED},
	{"}",     ".-.-.",  	CW_ENTRY_NORMAL},
	{"<AR>",	".-.-.",	CW_ENTRY_EXTENDED},
	{"{",     "...-.",  	CW_ENTRY_NORMAL},
	{"<VE>",	"...-.",	CW_ENTRY_EXTENDED},
	/* Sentinel end of table value */
	{NULL, NULL, 0}
};
	/* ISO 8859-1 accented characters */
//	{"\334\374",	"..--",	CW_ENTRY_NORMAL},  /* U diaeresis */
//	{"\304\344",	".-.-",	CW_ENTRY_NORMAL},  /* A diaeresis */
//	{"\307\347",	"-.-..",CW_ENTRY_NORMAL},  /* C cedilla */
//	{"\326\366",	"---.",	CW_ENTRY_NORMAL},  /* O diaeresis */
//	{"\311\351",	"..-..",CW_ENTRY_NORMAL},  /* E acute */
//	{"\310\350",	".-..-",CW_ENTRY_NORMAL},  /* E grave */
//	{"\305\345",	".--.-",CW_ENTRY_NORMAL},  /* A ring */
//	{"\321\361",	"--.--",CW_ENTRY_NORMAL},  /* N tilde */
	/* ISO 8859-2 accented characters */
//	{"\252",	"----", 	CW_ENTRY_NORMAL},  /* S cedilla */
//	{"\256",	"--..-",	CW_ENTRY_NORMAL},  /* Z dot above */


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
unsigned int morse::tokenize_representation(const char *representation)
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

bool morse::init()
{
	CW_TABLE *cw;	/* Pointer to table entry */
	unsigned int i;
	const char *p;
	long code;
	int len;

	// Clear the RX & TX tables
	for (i = 0; i < MorseTableSize; i++) {
		cw_tx_lookup[i] = 0x04;
		cw_rx_lookup[i] = 0;
	}

// For each main table entry, create a token entry.
	for (cw = cw_table; cw->chr != NULL; cw++) {
		i = tokenize_representation(cw->rpr);
		if (i != 0)
			cw_rx_lookup[i] = cw;
//		else
//			return false;
	}

// Build TX table 
	for (cw = cw_table; cw->chr != NULL; cw++) {
		if (cw->type == CW_ENTRY_EXTENDED)
			continue;
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
		for (p = cw->chr; *p != 0; p++)
			cw_tx_lookup[(int)*p] = code;
	}
	return true;
}

const char *morse::rx_lookup(char *r)
{
	static char chr[2];	/* FIXME */
	int			token;
	CW_TABLE *cw;		/* Pointer to table entry */

	if ((token = tokenize_representation(r)) == 0)
		return NULL;

	if ((cw = cw_rx_lookup[token]) == NULL)
		return NULL;

	if (cw->type == CW_ENTRY_EXTENDED)
		return cw->chr;

	chr[0] = cw->chr[0];
	chr[1] = 0;

	return chr;
}

unsigned long morse::tx_lookup(int c)
{
	return cw_tx_lookup[c];
}

/* ---------------------------------------------------------------------- */

