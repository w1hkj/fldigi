// ---------------------------------------------------------------------
// ax25_decode.cxx  --  AX25 Packet disassembler.
//
// This file is a proposed part of fldigi.  Adapted very liberally from
// rtty.cxx, with many thanks to John Hansen, W2FS, who wrote
// 'dcc.doc' and 'dcc2.doc', GNU Octave, GNU Radio Companion, and finally
// Bartek Kania (bk.gnarf.org) whose 'aprs.c' expository coding style helped
// shape this implementation.
//
// Copyright (C) 2010, 2014
//	Dave Freese, W1HKJ
//	Chris Sylvain, KB3CS
//	Robert Stiles, KK5VD
//
// fldigi is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi; if not, write to the
//
//  Free Software Foundation, Inc.
//  51 Franklin Street, Fifth Floor
//  Boston, MA  02110-1301 USA.
//
// ---------------------------------------------------------------------

#include "fl_digi.h"
#include "modem.h"
#include "misc.h"
#include "confdialog.h"
#include "configuration.h"
#include "status.h"
#include "timeops.h"
#include "debug.h"
#include "qrunner.h"
#include "threads.h"
#include "ax25_decode.h"

static PKT_MicE_field MicE_table[][12][5] =  {
	{
		{ Zero,  Zero, South, P0, East },
		{ One,   Zero, South, P0, East },
		{ Two,   Zero, South, P0, East },
		{ Three, Zero, South, P0, East },
		{ Four,  Zero, South, P0, East },
		{ Five,  Zero, South, P0, East },
		{ Six,   Zero, South, P0, East },
		{ Seven, Zero, South, P0, East },
		{ Eight, Zero, South, P0, East },
		{ Nine,  Zero, South, P0, East },
		{ Invalid, Null, Null, Null, Null },
		{ Invalid, Null, Null, Null, Null }
	},
	{ // ['A'..'K'] + 'L'
		{ Zero,  One,  Null, Null, Null }, // custom A/B/C msg codes
		{ One,   One,  Null, Null, Null },
		{ Two,   One,  Null, Null, Null },
		{ Three, One,  Null, Null, Null },
		{ Four,  One,  Null, Null, Null },
		{ Five,  One,  Null, Null, Null },
		{ Six,   One,  Null, Null, Null },
		{ Seven, One,  Null, Null, Null },
		{ Eight, One,  Null, Null, Null },
		{ Nine,  One,  Null, Null, Null },
		{ Space, One,  Null, Null, Null },
		{ Space, Zero, South, P0, East }
	},
	{ // ['P'..'Z']
		{ Zero,  One,  North, P100, West }, // standard A/B/C msg codes
		{ One,   One,  North, P100, West },
		{ Two,   One,  North, P100, West },
		{ Three, One,  North, P100, West },
		{ Four,  One,  North, P100, West },
		{ Five,  One,  North, P100, West },
		{ Six,   One,  North, P100, West },
		{ Seven, One,  North, P100, West },
		{ Eight, One,  North, P100, West },
		{ Nine,  One,  North, P100, West },
		{ Space, One,  North, P100, West },
		{ Invalid, Null, Null, Null, Null }
	} };

static PKT_PHG_table PHG_table[] = {
	{ "Omni", 4 },
	{ "NE", 2 },
	{ "E",  1 },
	{ "SE", 2 },
	{ "S",  1 },
	{ "SW", 2 },
	{ "W",  1 },
	{ "NW", 2 },
	{ "N",  1 }
};

static unsigned char rxbuf[MAXOCTETS+4];

int mode = FTextBase::RECV;

#define put_rx_char  put_rx_local_char

inline void put_rx_local_char(char value)
{
	put_rx_processed_char(value, mode);
}

inline void put_rx_hex(unsigned char c)
{
	char v[3];

	snprintf(&v[0], 3, "%02x", c);

	put_rx_char(v[0]);
	put_rx_char(v[1]);
}


inline void put_rx_const(const char s[])
{
	unsigned char *p = (unsigned char *) &s[0];
	for( ; *p; p++)  put_rx_char(*p);
}

static void expand_Cmp(unsigned char *cpI)
{
	// APRS Spec 1.0.1 Chapter 9 - Compressed Position Report format

	unsigned char *cp, tc, cc;
	unsigned char Cmpbuf[96], *bp = &Cmpbuf[0];
	unsigned char *tbp = bp;

	double Lat, Lon, td;
	bool sign;

	cp = cpI+1; // skip past Symbol Table ID char

	// Latitude as base91 number
	tc = *cp++ - 33;
	Lat = tc * 91 * 91 * 91;  // fourth digit ==> x * 91^3
	tc = *cp++ - 33;
	Lat += tc * 91 * 91;  // third digit ==> x * 91^2
	tc = *cp++ - 33;
	Lat += tc * 91;  // second digit ==> x * 91^1
	tc = *cp++ - 33;
	Lat += tc;  // units digit ==> x * 91^0

	Lat = 90.0 - Lat / 380926.0;  // - ==> S, + ==> N

	// Longitude as base91 number
	tc = *cp++ - 33;
	Lon = tc * 91 * 91 * 91;  // 4th digit
	tc = *cp++ - 33;
	Lon += tc * 91 * 91;  // 3rd digit
	tc = *cp++ - 33;
	Lon += tc * 91;  // 2nd digit
	tc = *cp++ - 33;
	Lon += tc;  // units digit

	Lon = -180.0 + Lon / 190463.0;  // - ==> W, + ==> E

	if (Lat < 0) {
		sign = 1; // has sign (is negative)
		Lat *= -1;
	}
	else  sign = 0;

	td = Lat - floor(Lat);
	cc = snprintf((char *)bp, 3, "%2.f", (Lat - td)); // DD
	bp += cc;
	cc = snprintf((char *)bp, 6, "%05.2f", td*60); // MM.MM
	bp += cc;

	if (sign)  *bp++ = 'S';
	else  *bp++ = 'N';

	*bp++ = ' ';

	if (Lon < 0) {
		sign = 1;
		Lon *= -1;
	}
	else  sign = 0;

	td = Lon - floor(Lon);
	cc = snprintf((char *)bp, 4, "%03.f", (Lon - td)); // DDD
	bp += cc;
	cc = snprintf((char *)bp, 6, "%5.2f", td*60); // MM.MM
	bp += cc;

	if (sign)  *bp++ = 'W';
	else  *bp++ = 'E';

	cp += 1; // skip past Symbol Code char

	if (*cp != ' ') { // still more
		if ((*(cp + 2) & 0x18) == 0x10) { // NMEA source = GGA sentence
										  // compressed Altitude uses chars in the same range
										  // as CSE/SPD but the Compression Type ID takes precedence
										  // when it indicates the NMEA source is a GGA sentence.
										  // so check on this one first and CSE/SPD last.
			double Altitude;

			tc = *cp++ - 33;  // 2nd digit
			Altitude = tc * 91;
			tc = *cp++ - 33;
			Altitude += tc;

			// this compressed posit field is not very useful as spec'ed,
			// since it cannot produce a possible negative altitude!
			// the NMEA GGA sentence is perfectly capable of providing
			// a negative altitude value.  Mic-E gets this right.

			// Since the example given in the APRS 1.0.1 Spec uses a value
			// in excess of 10000, this field should be re-spec'ed as a
			// value in meters relative to 10km below mean sea level (just
			// as done in Mic-E).
			Altitude = pow(1.002, Altitude);

			if (progdefaults.PKT_unitsSI)
				cc = snprintf((char *)bp, 11, " %-.1fm", Altitude*0.3048);
			else // units per Spec
				cc = snprintf((char *)bp, 12, " %-.1fft", Altitude);
			bp += cc;
		}
		else if (*cp == '{') { // pre-calculated radio range
			double Range;

			cp += 1; // skip past ID char

			tc = *cp++ - 33; // range
			Range = pow(1.08, (double)tc) * 2;

			if (progdefaults.PKT_unitsSI)
				cc = snprintf((char *)bp, 24, " Est. Range = %-.1fkm", Range*1.609);
			else // units per Spec
				cc = snprintf((char *)bp, 24, " Est. Range = %-.1fmi", Range);
			bp += cc;
		}
		else if (*cp >= '!' && *cp <= 'z') { // compressed CSE/SPD
			int Speed;

			tc = *cp++ - 33; // course
			cc = snprintf((char *)bp, 8, " %03ddeg", tc*4);
			bp += cc;

			tc = *cp++ - 33; // speed
			Speed = (int)floor(pow(1.08, (double)tc) - 1); // 1.08^tc - 1 kts

			if (progdefaults.PKT_unitsSI)
				cc = snprintf((char *)bp, 8, " %03dkph", (int)floor(Speed*1.852+0.5));
			else if (progdefaults.PKT_unitsEnglish)
				cc = snprintf((char *)bp, 8, " %03dmph", (int)floor(Speed*1.151+0.5));
			else // units per Spec
				cc = snprintf((char *)bp, 8, " %03dkts", Speed);
			bp += cc;
		}
	}
	if (progdefaults.PKT_RXTimestamp)
		put_rx_const("           ");

	put_rx_const(" [Cmp] ");

	for(; tbp < bp; tbp++)  put_rx_char(*tbp);

	put_rx_char('\r');

	if (debug::level >= debug::VERBOSE_LEVEL) {
		cp = cpI+12; // skip to Compression Type ID char

		if (*(cp - 2) != ' ') { // Cmp Type ID is valid
			tbp = bp = &Cmpbuf[0];

			tc = *cp - 33; // T

			cc = snprintf((char *)bp, 4, "%02x:", tc);
			bp += cc;

			strcpy((char *)bp, " GPS Fix = ");
			bp += 11;

			if ((tc & 0x20) == 0x20) {
				strcpy((char *)bp, "old");
				bp += 3;
			}
			else {
				strcpy((char *)bp, "current");
				bp += 7;
			}

			strcpy((char *)bp, ", NMEA Source = ");
			bp += 16;

			switch (tc & 0x18) {
				case 0x00:
					strcpy((char *)bp, "other");
					bp += 5;
					break;
				case 0x08:
					strcpy((char *)bp, "GLL");
					bp += 3;
					break;
				case 0x10:
					strcpy((char *)bp, "GGA");
					bp += 3;
					break;
				case 0x18:
					strcpy((char *)bp, "RMC");
					bp += 3;
					break;
				default:
					strcpy((char *)bp, "\?\?");
					bp += 2;
					break;
			}

			strcpy((char *)bp, ", Cmp Origin = ");
			bp += 15;

			switch (tc & 0x07) {
				case 0x00:
					strcpy((char *)bp, "Compressed");
					bp += 10;
					break;
				case 0x01:
					strcpy((char *)bp, "TNC BText");
					bp += 9;
					break;
				case 0x02:
					strcpy((char *)bp, "Software (DOS/Mac/Win/+SA)");
					bp += 26;
					break;
				case 0x03:
					strcpy((char *)bp, "[tbd]");
					bp += 5;
					break;
				case 0x04:
					strcpy((char *)bp, "KPC3");
					bp += 4;
					break;
				case 0x05:
					strcpy((char *)bp, "Pico");
					bp += 4;
					break;
				case 0x06:
					strcpy((char *)bp, "Other tracker [tbd]");
					bp += 19;
					break;
				case 0x07:
					strcpy((char *)bp, "Digipeater conversion");
					bp += 21;
					break;
				default:
					strcpy((char *)bp, "\?\?");
					bp += 2;
					break;
			}

			if (progdefaults.PKT_RXTimestamp)
				put_rx_const("           ");

			put_rx_const(" [CmpType] ");

			for(; tbp < bp; tbp++)  put_rx_char(*tbp);

			put_rx_char('\r');
		}
	}
}

static void expand_PHG(unsigned char *cpI)
{
	// APRS Spec 1.0.1 Chapter 6 - Time and Position format
	// APRS Spec 1.0.1 Chapter 7 - PHG Extension format

	bool hasPHG = false;
	unsigned char *cp, tc, cc;
	unsigned char PHGbuf[64], *bp = &PHGbuf[0];
	unsigned char *tbp = bp;

	switch (*cpI) {
		case '!':
		case '=': // simplest posits

			cp = cpI+1; // skip past posit ID char

			if (*cp != '/') { // posit not compressed
				cp += 19; // skip past posit data
			}
			else { // posit is compressed
				cp += 1; // skip past compressed posit ID char
				cp += 12; // skip past compressed posit data
			}

			if (strncmp((const char *)cp, "PHG", 3) == 0) { // strings match
				unsigned char ndigits;
				int power, height;
				double gain, range;

				cp += 3; // skip past Data Extension ID chars

				// get span of chars in cp which are only digits
				ndigits = strspn((const char *)cp, "0123456789");

				switch (ndigits) {
						//case 1: H might be larger than '9'. code below will work.
						//  must also check that P.GD are all '0'-'9'
						//break;
					case 4: // APRS Spec 1.0.1 Chapter 7 page 28
					case 5: // PHGR proposed for APRS Spec 1.2
						hasPHG = true;

						tc = *cp++ - '0'; // P
						power = tc * tc; // tc^2
						cc = snprintf((char *)bp, 5, "%dW,", power);
						bp += cc;

						tc = *cp++ - '0'; // H
						*bp++ = ' ';
						if (tc < 30) { // constrain Height to signed 32bit value
							height = 10 * (1 << tc); // 10 * 2^tc

							if (progdefaults.PKT_unitsSI)
								cc = snprintf((char *)bp, 11, "%dm", (int)floor(height*0.3048+0.5));
							else // units per Spec
								cc = snprintf((char *)bp, 12, "%dft", height);
							bp += cc;
						}
						else {
							height = 0;
							strcpy((char *)bp, "-\?\?-");
							bp += 4;
						}
						strcpy((char *)bp, " HAAT,");
						bp += 6;

						tc = *cp++; // G
						gain = pow(10, ((double)(tc - '0') / 10));
						cc = snprintf((char *)bp, 6, " %cdB,", tc);
						bp += cc;

						tc = *cp++ - '0'; // D
						*bp++ = ' ';
						if (tc < 9) {
							strcpy((char *)bp, PHG_table[tc].s);
							bp += PHG_table[tc].l;
						}
						else {
							strcpy((char *)bp, "-\?\?-");
							bp += 4;
						}
						*bp++ = ',';

						range = sqrt(2 * height * sqrt(((double)power / 10) * (gain / 2)));
						if (progdefaults.PKT_unitsSI)
							cc = snprintf((char *)bp, 24, " Est. Range = %-.1fkm", range*1.609);
						else // units per Spec
							cc = snprintf((char *)bp, 24, " Est. Range = %-.1fmi", range);
						bp += cc;

						if (ndigits == 5 && *(cp + 1) == '/') {
							// PHGR: http://www.aprs.org/aprs12/probes.txt
							// '1'-'9' and 'A'-'Z' are actually permissible.
							// does anyone send 10 ('A') or more beacons per hour?
							strcpy((char *)bp, ", ");
							bp += 2;

							tc = *cp++; // R
							cc = snprintf((char *)bp, 14, "%c beacons/hr", tc);
							bp += cc;
						}
						break;
					default: // switch(ndigits)
						break;
				}
			}
			break;
		default: // switch(*cpI)
			break;
	}

	if (hasPHG) {
		if (progdefaults.PKT_RXTimestamp)
			put_rx_const("           ");

		put_rx_const(" [PHG] ");

		for(; tbp < bp; tbp++)  put_rx_char(*tbp);

		put_rx_char('\r');
	}
}

static void expand_MicE(unsigned char *cpI, unsigned char *cpE)
{
	// APRS Spec 1.0.1 Chapter 10 - Mic-E Data format

	bool isMicE = true;
	bool msgstd = false, msgcustom = false;

	// decoding starts at first AX.25 dest addr
	unsigned char *cp = &rxbuf[1], tc, cc;
	unsigned char MicEbuf[64], *bp = &MicEbuf[0];
	unsigned char *tbp = bp;
	unsigned int msgABC = 0;
	PKT_MicE_field Lat = North, LonOffset = Zero, Lon = West;

	for (int i = 0; i < 3; i++) {
		// remember: AX.25 dest addr chars are shifted left by one
		tc = *cp++ >> 1;

		switch (tc & 0xF0) {
			case 0x30: // MicE_table[0]
				cc = tc - '0';
				if (cc < 10) {
					*bp++ = MicE_table[0][cc][0];
				}
				else  isMicE = false;
				break;
			case 0x40: // MicE_table[1]
				cc = tc - 'A';
				if (cc < 12) {
					bool t = MicE_table[1][cc][1]-'0';
					if (t)  {
						msgABC |= t << (2-i);
						msgcustom = true;
					}
					else  msgABC &= ~(1 << (2-i));
					*bp++ = MicE_table[1][cc][0];
				}
				else  isMicE = false;
				break;
			case 0x50: // MicE_table[2]
				cc = tc - 'P';
				if (cc < 11) {
					msgABC |= (MicE_table[2][cc][1]-'0') << (2-i);
					msgstd = true;
					*bp++ = MicE_table[2][cc][0];
				}
				else  isMicE = false;
				break;
			default:   // Invalid
				isMicE = false;
				break;
		}
	}

	for (int i = 3; i < 6; i++) {
		// remember: AX.25 dest addr chars are shifted left by one
		tc = *cp++ >> 1;

		switch (i) {
			case 3:
				switch (tc & 0xF0) {
					case 0x30: // MicE_table[0]
						cc = tc - '0';
						if (cc < 10) {
							Lat = MicE_table[0][cc][2];
							*bp++ = MicE_table[0][cc][0];
						}
						else  isMicE = false;
						break;
					case 0x40: // MicE_table[1]
						cc = tc - 'A';
						if (cc == 11) {
							Lat = MicE_table[1][cc][2];
							*bp++ = MicE_table[1][cc][0];
						}
						else  isMicE = false;
						break;
					case 0x50: // MicE_table[2]
						cc = tc - 'P';
						if (cc < 11) {
							Lat = MicE_table[2][cc][2];
							*bp++ = MicE_table[2][cc][0];
						}
						else  isMicE = false;
						break;
					default:   // Invalid
						isMicE = false;
						break;
				}
				break;
			case 4:
				switch (tc & 0xF0) {
					case 0x30: // MicE_table[0]
						cc = tc - '0';
						if (cc < 10) {
							LonOffset = MicE_table[0][cc][3];
							*bp++ = MicE_table[0][cc][0];
						}
						else  isMicE = false;
						break;
					case 0x40: // MicE_table[1]
						cc = tc - 'A';
						if (cc == 11) {
							LonOffset = MicE_table[1][cc][3];
							*bp++ = MicE_table[1][cc][0];
						}
						else  isMicE = false;
						break;
					case 0x50: // MicE_table[2]
						cc = tc - 'P';
						if (cc < 11) {
							LonOffset = MicE_table[2][cc][3];
							*bp++ = MicE_table[2][cc][0];
						}
						else  isMicE = false;
						break;
					default:   // Invalid
						isMicE = false;
						break;
				}
				break;
			case 5:
				switch (tc & 0xF0) {
					case 0x30: // MicE_table[0]
						cc = tc - '0';
						if (cc < 10) {
							Lon = MicE_table[0][cc][4];
							*bp++ = MicE_table[0][cc][0];
						}
						else  isMicE = false;
						break;
					case 0x40: // MicE_table[1]
						cc = tc - 'A';
						if (cc == 11) {
							Lon = MicE_table[1][cc][4];
							*bp++ = MicE_table[1][cc][0];
						}
						else  isMicE = false;
						break;
					case 0x50: // MicE_table[2]
						cc = tc - 'P';
						if (cc < 11) {
							Lon = MicE_table[2][cc][4];
							*bp++ = MicE_table[2][cc][0];
						}
						else  isMicE = false;
						break;
					default:   // Invalid
						isMicE = false;
						break;
				}
				break;
			default:   // Invalid
				isMicE = false;
				break;
		}
	}

	if (isMicE) {
		int Speed = 0, Course = 0;

		if (progdefaults.PKT_RXTimestamp)
			put_rx_const("           ");

		put_rx_const(" [Mic-E] ");

		if (msgstd && msgcustom)
			put_rx_const("Unknown? ");
		else if (msgcustom) {
			put_rx_const("Custom-");
			put_rx_char((7 - msgABC)+'0');
			put_rx_const(". ");
		}
		else {
			switch (msgABC) { // APRS Spec 1.0.1 Chapter 10 page 45
				case 0:
					put_rx_const("Emergency");
					break;
				case 1:
					put_rx_const("Priority");
					break;
				case 2:
					put_rx_const("Special");
					break;
				case 3:
					put_rx_const("Committed");
					break;
				case 4:
					put_rx_const("Returning");
					break;
				case 5:
					put_rx_const("In Service");
					break;
				case 6:
					put_rx_const("En Route");
					break;
				case 7:
					put_rx_const("Off Duty");
					break;
				default:
					put_rx_const("-\?\?-");
					break;
			}
			if (msgABC)  put_rx_char('.');
			else  put_rx_char('!'); // Emergency!

			put_rx_char(' ');
		}

		for (; tbp < bp; tbp++) {
			put_rx_char(*tbp);
			if (tbp == (bp - 3))  put_rx_char('.');
		}

		if (Lat == North)  put_rx_char('N');
		else if (Lat == South)  put_rx_char('S');
		else  put_rx_char('\?');

		put_rx_char(' ');

		cp = cpI+1; // one past the Data Type ID char

		// decode Lon degrees - APRS Spec 1.0.1 Chapter 10 page 48
		tc = *cp++ - 28;
		if (LonOffset == P100) tc += 100;
		if (tc > 179 && tc < 190)  tc -= 80;
		else if (tc > 189 && tc < 200)  tc -= 190;

		cc = snprintf((char *)bp, 4, "%03d", tc);
		bp += cc;

		// decode Lon minutes
		tc = *cp++ - 28;
		if (tc > 59)  tc -= 60;

		cc = snprintf((char *)bp, 3, "%02d", tc);
		bp += cc;

		// decode Lon hundredths of a minute
		tc = *cp++ - 28;

		cc = snprintf((char *)bp, 3, "%02d", tc);
		bp += cc;

		for (; tbp < bp; tbp++) {
			put_rx_char(*tbp);
			if (tbp == (bp - 3))  put_rx_char('.');
		}

		if (Lon == East)  put_rx_char('E');
		else if (Lon == West)  put_rx_char('W');
		else  put_rx_char('\?');

		// decode Speed and Course - APRS Spec 1.0.1 Chapter 10 page 52
		tc = *cp++ - 28; // speed: hundreds and tens

		if (tc > 79)  tc -= 80;
		Speed = tc * 10;

		tc = *cp++ - 28; // speed: units and course: hundreds

		Course = (tc % 10); // remainder from dividing by 10
		tc -= Course; tc /= 10; // tc is now quotient from dividing by 10
		Speed += tc;

		if (Course > 3)  Course -= 4;
		Course *= 100;

		tc = *cp++ - 28; // course: tens and units

		Course += tc;

		if (progdefaults.PKT_unitsSI)
			cc = snprintf((char *)bp, 8, " %03dkph", (int)floor(Speed*1.852+0.5));
		else if (progdefaults.PKT_unitsEnglish)
			cc = snprintf((char *)bp, 8, " %03dmph", (int)floor(Speed*1.151+0.5));
		else // units per Spec
			cc = snprintf((char *)bp, 8, " %03dkts", Speed);
		bp += cc;

		cc = snprintf((char *)bp, 8, " %03ddeg", Course);
		bp += cc;

		for (; tbp < bp; tbp++) {
			put_rx_char(*tbp);
		}

		cp += 2; // skip past Symbol and Symbol Table ID chars

		if (cp <= cpE) { // still more

			if (*cp == '>') {
				cp += 1;
				put_rx_const(" TH-D7");
			}
			else if (*cp == ']' && *cpE == '=') {
				cp += 1;
				cpE -= 1;
				put_rx_const(" TM-D710");
			}
			else if (*cp == ']') {
				cp += 1;
				put_rx_const(" TM-D700");
			}
			else if (*cp == '\'' && *(cpE - 1) == '|' && *cpE == '3') {
				cp += 1;
				cpE -= 2;
				put_rx_const(" TT3");
			}
			else if (*cp == '\'' && *(cpE - 1) == '|' && *cpE == '4') {
				cp += 1;
				cpE -= 2;
				put_rx_const(" TT4");
			}
			else if (*cp == '`' && *(cpE - 1) == '_' && *cpE == ' ') {
				cp += 1;
				cpE -= 2;
				put_rx_const(" VX-8");
			}
			else if (*cp == '`' && *(cpE - 1) == '_' && *cpE == '#') {
				cp += 1;
				cpE -= 2;
				put_rx_const(" VX-8D/G"); // VX-8G for certain. guessing.
			}
			else if (*cp == '`' && *(cpE - 1) == '_' && *cpE == '\"') {
				cp += 1;
				cpE -= 2;
				put_rx_const(" FTM-350");
			}
			else if ((*cp == '\'' || *cp == '`') && *(cp + 4) == '}') {
				cp += 1;
				// tracker? rig? ID codes are somewhat ad hoc.
				put_rx_const(" MFR\?");
			}

			if (cp < cpE) {
				if (*(cp + 3) == '}') { // station altitude as base91 number
					int Altitude = 0;

					tc = *cp++ - 33; // third digit ==> x * 91^2
					Altitude = tc * 91 * 91;
					tc = *cp++ - 33; // second digit ==> x * 91^1 ==> x * 91
					Altitude += tc * 91;
					tc = *cp++ - 33; // unit digit ==> x * 91^0 ==> x * 1
					Altitude += tc;

					Altitude -= 10000; // remove offset from datum

					*bp++ = ' ';
					if (Altitude >= 0)  *bp++ = '+';

					if (progdefaults.PKT_unitsEnglish)
						cc = snprintf((char *)bp, 12, "%dft", (int)floor(Altitude*3.281+0.5));
					else // units per Spec
						cc = snprintf((char *)bp, 11, "%dm", Altitude);
					bp += cc;

					for (; tbp < bp; tbp++) {
						put_rx_char(*tbp);
					}

					cp += 1; // skip past '}'
				}
			}

			if (cp < cpE)  put_rx_char(' ');

			for (; cp <= cpE; cp++)  put_rx_char(*cp);
		}

		put_rx_char('\r');
	}
}

static void do_put_rx_char(unsigned char *cp, size_t count)
{
	int i, j;
	unsigned char c;
	bool isMicE = false;
	unsigned char *cpInfo;

	for (i = 8; i < 14; i++) { // src callsign is second in AX.25 frame
		c = rxbuf[i] >> 1;
		if (c != ' ') put_rx_char(c); // skip past padding (if any)
	}

	// bit  7   = command/response bit
	// bits 6,5 = 1
	// bits 4-1 = src SSID
	// bit  0   = last callsign flag
	c = (rxbuf[14] & 0x7f) >> 1;

	if (c > 0x30) {
		put_rx_char('-');
		if (c < 0x3a)
			put_rx_char(c);
		else {
			put_rx_char('1');
			put_rx_char(c-0x0a);
		}
	}

	put_rx_char('>');

	for (i = 1; i < 7; i++) { // dest callsign is first in AX.25 frame
		c = rxbuf[i] >> 1;
		if (c != ' ') put_rx_char(c);
	}

	c = (rxbuf[7] & 0x7f) >> 1;
	if (c > 0x30) {
		put_rx_char('-');
		if (c < 0x3a)
			put_rx_char(c);
		else {
			put_rx_char('1');
			put_rx_char(c-0x0a);
		}
	}

	j=8;
	if ((rxbuf[14] & 0x01) != 1) { // check last callsign flag
		do {
			put_rx_char(',');

			j += 7;
			for (i = j; i < (j+6); i++) {
				c = rxbuf[i] >> 1;
				if (c != ' ') put_rx_char(c);
			}

			c = (rxbuf[j+6] & 0x7f) >> 1;
			if (c > 0x30) {
				put_rx_char('-');
				if (c < 0x3a)
					put_rx_char(c);
				else {
					put_rx_char('1');
					put_rx_char(c-0x0a);
				}
			}

		} while ((rxbuf[j+6] & 0x01) != 1);

		if (rxbuf[j+6] & 0x80) // packet gets no more hops
			put_rx_char('*');
	}

	if (debug::level < debug::VERBOSE_LEVEL) {
		// skip past CTRL and PID to INFO bytes when I_FRAME
		// puts buffer pointer in FCS when U_FRAME and S_FRAME
		// (save CTRL byte for possible MicE decoding)
		j += 7;
		c = rxbuf[j];
		j += 2;
	}
	else { // show more frame info when .ge. VERBOSE debug level
		j += 7;
		put_rx_char(';');

		c = rxbuf[j]; // CTRL present in all frames

		if ((c & 0x01) == 0) { // I_FRAME
			unsigned char p = rxbuf[j+1]; // PID present only in I_FRAME

			if (debug::level == debug::DEBUG_LEVEL) {
				put_rx_hex(c);
				put_rx_char(' ');
				put_rx_hex(p);
				put_rx_char(';');
			}

			put_rx_const("I/");

			put_rx_hex( (c & 0xE0) >> 5 ); // AX.25 v2.2 para 2.3.2.1
			if (c & 0x10)  put_rx_char('*'); // P/F bit
			else  put_rx_char('.');
			put_rx_hex( (c & 0x0E) >> 1 );

			put_rx_char('/');

			switch (p) { // AX.25 v2.2 para 2.2.4
				case 0x01:
					put_rx_const("X.25PLP");
					break;
				case 0x06:
					put_rx_const("C-TCPIP");
					break;
				case 0x07:
					put_rx_const("U-TCPIP");
					break;
				case 0x08:
					put_rx_const("FRAG");
					break;
				case 0xC3:
					put_rx_const("TEXNET");
					break;
				case 0xC4:
					put_rx_const("LQP");
					break;
				case 0xCA:
					put_rx_const("ATALK");
					break;
				case 0xCB:
					put_rx_const("ATALK-ARP");
					break;
				case 0xCC:
					put_rx_const("ARPA-IP");
					break;
				case 0xCD:
					put_rx_const("ARPA-AR");
					break;
				case 0xCE:
					put_rx_const("FLEXNET");
					break;
				case 0xCF:
					put_rx_const("NET/ROM");
					break;
				case 0xF0:
					put_rx_const("NO-L3");
					break;

				case 0xFF:
					put_rx_const("L3ESC=");
					put_rx_hex(rxbuf[++j]);
					break;
					
				default:
					if ((p & 0x30) == 0x10)  put_rx_const("L3V1");
					else if ((p & 0x30) == 0x20)  put_rx_const("L3V2");
					else  put_rx_const("L3-RSVD");
					
					put_rx_char('=');
					put_rx_hex(p);
					break;
			}
		}
		else if ((c & 0x03) == 0x01) { // S_FRAME
			
			if (debug::level == debug::DEBUG_LEVEL) {
				put_rx_hex(c);
				put_rx_char(';');
			}
			
			put_rx_const("S/");
			
			put_rx_hex( (c & 0xE0) >> 5 );
			if (c & 0x10)  put_rx_char('*');
			else  put_rx_char('.');
			put_rx_char('/');
			
			switch (c & 0x0C) { // AX.25 v2.2 para 2.3.4.2
				case 0x00:
					put_rx_const("RR");
					break;
				case 0x04:
					put_rx_const("RNR");
					break;
				case 0x08:
					put_rx_const("REJ");
					break;
				case 0x0C:
				default:
					put_rx_const("UNK");
					break;
			}
		}
		else if ((c & 0x03) == 0x03) { // U_FRAME
			
			if (debug::level == debug::DEBUG_LEVEL) {
				put_rx_hex(c);
				put_rx_char(';');
			}
			
			put_rx_char('U');
			
			if (c & 0x10)  put_rx_char('*');
			else  put_rx_char('.');
			
			switch (c & 0xEC) { // AX.25 v2.2 para 2.3.4.3
				case 0x00:
					put_rx_const("UI");
					break;
				case 0x0E:
					put_rx_const("DM");
					break;
				case 0x1E:
					put_rx_const("SABM");
					break;
				case 0x20:
					put_rx_const("DISC");
					break;
				case 0x30:
					put_rx_const("UA");
					break;
				case 0x81:
					put_rx_const("FRMR");
					break;
				default:
					put_rx_const("UNK");
					break;
			}
		}
		j+=2;
	}
	put_rx_char(':');
	
	// ptr to first info field char
	cpInfo = &rxbuf[j];
	
	if ((c & 0x03) == 0x03 && (c & 0xEC) == 0x00
		&& (*cpInfo == '\'' || *cpInfo == '`'
			|| *cpInfo == 0x1C || *cpInfo == 0x1D)
		&& (cp - cpInfo) > 7) {
		/*
		 Mic-E must have at least 8 info chars + Data Type ID char
		 cp - (cpInfo - 1) > 8 ==> cp - cpInfo > 7
		 */
		// this is very probably a Mic-E encoded packet
		isMicE = true;
	}
	
	// offset between last info char (not FCS) and bufhead
	//i = (cp - &rxbuf[0]); // (cp - &rxbuf[1]) + 1 ==> (cp - &rxbuf[0])
	i = count; // (cp - &rxbuf[1]) + 1 ==> (cp - &rxbuf[0])
	
	while (j < i)  put_rx_char(rxbuf[j++]);
	
	if (*(cp-1) != '\r')
		put_rx_char('\r'); // <cr> only for packets not ending with <cr>
	
	// cp points to FCS, so (cp-X) is last info field char
	if ((progdefaults.PKT_expandMicE || debug::level >= debug::VERBOSE_LEVEL)
		&& isMicE)
		expand_MicE(cpInfo, (*(cp-1) == '\r' ? cp-2 : cp-1));
	
	// need to deal with posits having timestamps ('/' and '@' leading char)
	if (*cpInfo == '!' || *cpInfo == '=') {
		if ((progdefaults.PKT_expandCmp || debug::level >= debug::VERBOSE_LEVEL)
			&& (*(cpInfo + 1) == '/' || *(cpInfo + 1) == '\\'))
			// compressed posit
			expand_Cmp(cpInfo+1);
		
		if (progdefaults.PKT_expandPHG
			|| debug::level >= debug::VERBOSE_LEVEL) // look for PHG data
			expand_PHG(cpInfo);
	}
	
	if (*(cp-1) == '\r')
		put_rx_char('\r'); // for packets ending with <cr>: show it on-screen
}

extern bool PERFORM_CPS_TEST;
static pthread_mutex_t decode_lock_mutex = PTHREAD_MUTEX_INITIALIZER;

void ax25_decode(unsigned char *buffer, size_t count, bool pad, bool tx_flag)
{
	guard_lock decode_lock(&decode_lock_mutex);
	
	if(!buffer && !count) return;
	
	if(count > MAXOCTETS)
		count = MAXOCTETS;
	
	memset(rxbuf, 0, sizeof(rxbuf));
	
	if(pad) {
		rxbuf[0] = 0xfe;
		memcpy(&rxbuf[1], buffer, count);
		count++;
	} else {
		memcpy(rxbuf, buffer, count);
	}
	
	if(tx_flag) {
		mode = FTextBase::XMIT;
	} else {
		mode = FTextBase::RECV;
	}
	
	do_put_rx_char(&rxbuf[count], count);
}
