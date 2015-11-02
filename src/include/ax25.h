// ---------------------------------------------------------------------
// ax25.h
//
//
// This file is a part of fldigi.
// Adapted very liberally from rtty.h, with many thanks to John Hansen, 
// W2FS, who wrote 'dcc.doc' and 'dcc2.doc', GNU Octave, GNU Radio 
// Companion, and finally Bartek Kania (bk.gnarf.org) whose 'aprs.c' 
// expository coding style helped shape this implementation.
//
// Copyright (C) 2010
//    Dave Freese, W1HKJ
//    Chris Sylvain, KB3CS
//
// fldigi is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
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

#ifndef AX25_H
#define AX25_H

#include "complex.h"
#include "modem.h"
#include "globals.h"
#include "debug.h"
#include "filters.h"
#include "fftfilt.h"

// 70 bytes addr + 256 payload + 2 FCS + 1 Control + 1 Protocol ID
#define MAXOCTETS 340
// 136 bits minimum (including start and end flags) - AX.25 v2.2 section 3.9
//  == 15 octets.  we count only one of the two flags, though.
#define MINOCTETS  14

enum AX25_MicE_field {
	Null = 0x00,
	Space = 0x20,
	Zero = 0x30,
	One,
	Two,
	Three,
	Four,
	Five,
	Six,
	Seven,
	Eight,
	Nine,
	P0 = 0x40,
	P100,
	North = 0x50,
	East,
	South,
	West,
	Invalid = 0xFF
};

struct AX25_PHG_table {
	const char *s;
	unsigned char l;
};

class ax25 {
public:

	int mode;
	unsigned char rxbuf[MAXOCTETS+4];

	ax25();
	~ax25() {}

	unsigned int computeFCS(unsigned char *h, unsigned char *t);
	bool	checkFCS(unsigned char *cp);

// MicE encodings:
//   3 char groups,
//     max 12 char values per group,
//       5 encodings per char value
	static AX25_MicE_field	MicE_table[][12][5];
	static AX25_PHG_table	PHG_table[];

	void	expand_MicE(unsigned char *I, unsigned char *E);
	void	expand_PHG(unsigned char *I);
	void	expand_Cmp(unsigned char *I);

	inline void put_ax25_char(unsigned char c);
	inline void put_rx_const(const char s[]);
	inline void put_rx_hex(unsigned char c);

	void clear_rxbuf();
	void do_put_rx_char(unsigned char *cp, size_t count = 0);

	void decode(unsigned char *buffer, size_t count, bool pad, bool tx_flag);
};

#endif
