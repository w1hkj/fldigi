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

#ifndef __ax25_decode__
#define __ax25_decode__

#include <iostream>

// 70 bytes addr + 256 payload + 2 FCS + 1 Control + 1 Protocol ID
#define MAXOCTETS 340
// 136 bits minimum (including start and end flags) - AX.25 v2.2 section 3.9
//  == 15 octets.  we count only one of the two flags, though.
#define MINOCTETS  14

enum PKT_MicE_field {
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

struct PKT_PHG_table {
	const char *s;
	unsigned char l;
};

//static void do_put_rx_char(unsigned char *cp);
static void expand_MicE(unsigned char *cpI, unsigned char *cpE);
static void expand_PHG(unsigned char *cpI);
static void expand_Cmp(unsigned char *cpI);
inline void put_rx_const(const char s[]);
void ax25_decode(unsigned char *buffer, size_t count, bool pad, bool tx_flag);

#endif /* defined(__ax25_decode__) */
