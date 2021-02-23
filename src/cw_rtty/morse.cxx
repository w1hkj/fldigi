/*
 *    morse.c  --  morse code tables
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

#include <config.h>
#include <cstring>
#include <iostream>

#include "morse.h"
#include "configuration.h"

/* ---------------------------------------------------------------------- */

/*
 * Morse code characters table.  This table allows lookup of the Morse
 * shape of a given alphanumeric character.  Shapes are held as a string,
 * with "-' representing dash, and ".' representing dot.  The table ends
 * with a NULL entry.
 *
 * This is the main table from which the other tables are computed.
 *
 * The Prosigns are also defined in the configuration.h file
 * The user can specify the character which substitutes for the prosign
 */

bool CW_table_changed = false;

CWstruct cMorse::cw_table[] = {
// Prosigns
	{1, "=",	"<BT>",   "-...-" }, // 0
	{0, "~",	"<AA>",   ".-.-" }, // 1
	{1, "<",	"<AS>",   ".-..." }, // 2
	{1, ">",	"<AR>",   ".-.-." }, // 3
	{1, "%",	"<SK>",   "...-.-" }, // 4
	{1, "+",	"<KN>",   "-.--." }, // 5
	{1, "&",	"<INT>",  "..-.-" }, // 6
	{1, "{",	"<HM>",   "....--" }, // 7
	{1, "}",	"<VE>",   "...-." }, // 8
// ASCII 7bit letters
	{1, "A",	"A",	".-" },
	{1, "B",	"B",	"-..." },
	{1, "C",	"C",	"-.-." },
	{1, "D",	"D",	"-.." },
	{1, "E",	"E",	"."	 },
	{1, "F",	"F",	"..-." },
	{1, "G",	"G",	"--." },
	{1, "H",	"H",	"...." },
	{1, "I",	"I",	".." },
	{1, "J",	"J",	".---" },
	{1, "K",	"K",	"-.-" },
	{1, "L",	"L",	".-.." },
	{1, "M",	"M",	"--" },
	{1, "N",	"N",	"-." },
	{1, "O",	"O",	"---" },
	{1, "P",	"P",	".--." },
	{1, "Q",	"Q",	"--.-" },
	{1, "R",	"R",	".-." },
	{1, "S",	"S",	"..." },
	{1, "T",	"T",	"-"	 },
	{1, "U",	"U",	"..-" },
	{1, "V",	"V",	"...-" },
	{1, "W",	"W",	".--" },
	{1, "X",	"X",	"-..-" },
	{1, "Y",	"Y",	"-.--" },
	{1, "Z",	"Z",	"--.." },
//
	{1, "a",	"A",	".-" },
	{1, "b",	"B",	"-..." },
	{1, "c",	"C",	"-.-." },
	{1, "d",	"D",	"-.." },
	{1, "e",	"E",	"."	 },
	{1, "f",	"F",	"..-." },
	{1, "g",	"G",	"--." },
	{1, "h",	"H",	"...." },
	{1, "i",	"I",	".." },
	{1, "j",	"J",	".---" },
	{1, "k",	"K",	"-.-" },
	{1, "l",	"L",	".-.." },
	{1, "m",	"M",	"--" },
	{1, "n",	"N",	"-." },
	{1, "o",	"O",	"---" },
	{1, "p",	"P",	".--." },
	{1, "q",	"Q",	"--.-" },
	{1, "r",	"R",	".-." },
	{1, "s",	"S",	"..." },
	{1, "t",	"T",	"-"	 },
	{1, "u",	"U",	"..-" },
	{1, "v",	"V",	"...-" },
	{1, "w",	"W",	".--" },
	{1, "x",	"X",	"-..-" },
	{1, "y",	"Y",	"-.--" },
	{1, "z",	"Z",	"--.." },
// Numerals
	{1, "0",	"0",	"-----" },
	{1, "1",	"1",	".----" },
	{1, "2",	"2",	"..---" },
	{1, "3",	"3",	"...--" },
	{1, "4",	"4",	"....-" },
	{1, "5",	"5",	"....." },
	{1, "6",	"6",	"-...." },
	{1, "7",	"7",	"--..." },
	{1, "8",	"8",	"---.." },
	{1, "9",	"9",	"----." },
// Punctuation
	{1, "\\",	"\\",	".-..-." },
	{1, "\'",	"'",	".----." },
	{1, "$",	"$",	"...-..-" },
	{1, "(",	"(",	"-.--."	 },
	{1, ")",	")",	"-.--.-" },
	{1, ",",	",",	"--..--" },
	{1, "-",	"-",	"-....-" },
	{1, ".",	".",	".-.-.-" },
	{1, "/",	"/",	"-..-."	 },
	{1, ":",	":",	"---..." },
	{1, ";",	";",	"-.-.-." },
	{1, "?",	"?",	"..--.." },
	{1, "_",	"_",	"..--.-" },
	{1, "@",	"@",	".--.-." },
	{1, "!",	"!",	"-.-.--" },
// accented characters
	{1, "Ä", "Ä",	".-.-" },	// A umlaut
	{1, "ä", "Ä",	".-.-" },	// A umlaut
	{0, "Æ", "Æ",	".-.-" },	// A aelig
	{0, "æ", "Æ",	".-.-" },	// A aelig
	{0, "Å", "Å",	".--.-" },	// A ring
	{0, "å", "Å",	".--.-" },	// A ring
	{1, "Ç", "Ç",	"-.-.." },	// C cedilla
	{1, "ç", "Ç",	"-.-.." },	// C cedilla
	{0, "È", "È",	".-..-" },	// E grave
	{0, "è", "È",	".-..-" },	// E grave
	{1, "É", "É",	"..-.." },	// E acute
	{1, "é", "É",	"..-.." },	// E acute
	{0, "Ó", "Ó",	"---." },	// O acute
	{0, "ó", "Ó",	"---." },	// O acute
	{1, "Ö", "Ö",	"---." },	// O umlaut
	{1, "ö", "Ö",	"---." },	// O umlaut
	{0, "Ø", "Ø",	"---." },	// O slash
	{0, "ø", "Ø",	"---." },	// O slash
	{1, "Ñ", "Ñ",	"--.--" },	// N tilde
	{1, "ñ", "Ñ",	"--.--" },	// N tilde
	{1, "Ü", "Ü",	"..--" },	// U umlaut
	{1, "ü", "Ü",	"..--" },	// U umlaut
	{0, "Û", "Û",	"..--" },	// U circ
	{0, "û", "Û",	"..--" },	// U circ
// array termination
	{0, "", "", ""}
};

/* ---------------------------------------------------------------------- */

void cMorse::enable(std::string s, bool val)
{
	for (int i = 0; cw_table[i].rpr.length(); i++) {
		if (cw_table[i].chr == s || cw_table[i].prt == s) {
			cw_table[i].enabled = val;
			return;
		}
	}
}

void cMorse::init()
{
// Update the char / prosign relationship
	if (progdefaults.CW_prosigns.length() == 9) {
		for (int i = 0; i < 9; i++) {
			cw_table[i].chr = progdefaults.CW_prosigns[i];
		}
	}
	enable("<AA>", 1);
	enable("Ä", 0); enable("ä", 0);
	enable("Æ", 0); enable("æ", 0);
	enable("Å", 0); enable("å", 0);
	enable("Ç", 0); enable("ç", 0);
	enable("È", 0); enable("è", 0);
	enable("É", 0); enable("é", 0);
	enable("Ó", 0); enable("ó", 0);
	enable("Ö", 0); enable("ö", 0);
	enable("Ø", 0); enable("ø", 0);
	enable("Ñ", 0); enable("ñ", 0);
	enable("Ü", 0); enable("ü", 0);
	enable("Û", 0); enable("û", 0);

	if (progdefaults.A_umlaut)
		{ enable("Ä", 1); enable("ä", 1); enable("<AA>", 0); }
	if (progdefaults.A_aelig)
		{ enable("Æ", 1); enable("æ", 1); enable("<AA>", 0); }
	if (progdefaults.A_ring)
		{ enable("Å", 1); enable("å", 1); }
	if (progdefaults.C_cedilla)
		{ enable("Ç", 1); enable("ç", 1); }
	if (progdefaults.E_grave)
		{ enable("È", 1); enable("è", 1); }
	if (progdefaults.E_acute)
		{ enable("É", 1); enable("é", 1); }
	if (progdefaults.O_acute)
		{ enable("Ó", 1); enable("ó", 1); }
	if (progdefaults.O_umlaut)
		{ enable("Ö", 1); enable("ö", 1); }
	if (progdefaults.O_slash)
		{ enable("Ø", 1); enable("ø", 1); }
	if (progdefaults.N_tilde)
		{ enable("Ñ", 1); enable("ñ", 1); }
	if (progdefaults.U_umlaut)
		{ enable("Ü", 1); enable("ü", 1); }
	if (progdefaults.U_circ)
		{ enable("Û", 1); enable("û", 1); }

	enable ("\\", progdefaults.CW_backslash);
	enable ("\'", progdefaults.CW_single_quote);
	enable ("$", progdefaults.CW_dollar_sign);
	enable ("(", progdefaults.CW_open_paren);
	enable (")", progdefaults.CW_close_paren);
	enable (":", progdefaults.CW_colon);
	enable (";", progdefaults.CW_semi_colon);
	enable ("_", progdefaults.CW_underscore);
	enable ("@", progdefaults.CW_at_symbol);
	enable ("!", progdefaults.CW_exclamation);

	CW_table_changed = false;
	utf8.reserve(4);
	utf8.clear();
	ptr = 0;
	toprint.clear();
}

std::string cMorse::rx_lookup(std::string rx)
{
	if (CW_table_changed) init();
	for (int i = 0; cw_table[i].rpr.length(); i++) {
		if (rx == cw_table[i].rpr) {
			if (cw_table[i].enabled) {
				if (progdefaults.CW_prosign_display)
					return cw_table[i].chr;
				return cw_table[i].prt;
			}
		}
	}
	return "";
}

std::string cMorse::tx_lookup(int c)
{
	if (CW_table_changed) init();
	toprint.clear();

	c &= 0xFF;
	utf8 += c;
//	if (ptr < 4) utf8[ptr++] = c;

//	if ( c > 0x7F && ptr == 1 )
//		return "";
	if (((utf8[0] & 0xFF) > 0x7F) && (utf8.length() == 1)) {
		return "";
	}

	for (int i = 0; cw_table[i].rpr.length(); i++) {
		if (utf8 == cw_table[i].chr) {
			if (!cw_table[i].enabled) {
				utf8.clear();
				ptr = 0;
				return "";
			}
			toprint = cw_table[i].prt;
			utf8.clear();
			ptr = 0;
			return cw_table[i].rpr;
		}
	}
	utf8.clear();
	ptr = 0;
	return "";

}

int cMorse::tx_length(int c)
{
	if (c == ' ') return 4;
	std::string ms = tx_lookup(c);
	if (ms.empty()) return 0;
	int len = 0;
	for (size_t i = 0; i < ms.length(); i++)
		if (ms[i] == '.') len += 2;
		else              len += 4;
	len += 2;
	return len;
}


/* ---------------------------------------------------------------------- */

