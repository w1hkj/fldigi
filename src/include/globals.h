// ----------------------------------------------------------------------------
// globals.h  --  constants, variables, arrays & functions that need to be
//                  outside of any thread
//
// Copyright (C) 2006
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.  Adapted in part from code contained in gmfsk 
// source code distribution.
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
// along with fldigi; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// ----------------------------------------------------------------------------

#ifndef _GLOBALS_H
#define _GLOBALS_H

enum state_t {
	STATE_PAUSE = 0,
	STATE_RX,
	STATE_TX,
	STATE_RESTART,
	STATE_TUNE,
	STATE_ABORT,
	STATE_FLUSH,
	STATE_NOOP,
	STATE_EXIT,
	STATE_ENDED,
	STATE_IDLE,
	STATE_NEW_MODEM
};
extern const char *state_names[];

enum {
	MODE_PREV = -2,
	MODE_NEXT,

	MODE_CW,

	MODE_DOMINOEX4,
	MODE_DOMINOEX5,
	MODE_DOMINOEX8,
	MODE_DOMINOEX11,
	MODE_DOMINOEX16,
	MODE_DOMINOEX22,

	MODE_FELDHELL,
	MODE_FSKHELL,
	MODE_FSKH105,

	MODE_MFSK8,
	MODE_MFSK16,

	MODE_BPSK31,
	MODE_QPSK31,
	MODE_PSK63,
	MODE_QPSK63,
	MODE_PSK125,
	MODE_QPSK125,
	MODE_PSK250,
	MODE_QPSK250,

	MODE_OLIVIA,

	MODE_RTTY,

	MODE_THROB1,
	MODE_THROB2,
	MODE_THROB4,
	MODE_THROBX1,
	MODE_THROBX2,
	MODE_THROBX4,

//	MODE_MT63,

	MODE_WWV,
	MODE_ANALYSIS,

	NUM_MODES
};
typedef int trx_mode;

struct mode_info_t {
	trx_mode mode;
	class modem **modem;
	const char *sname;
	const char *name;
	const char *pskmail_name;
};

extern const struct mode_info_t mode_info[NUM_MODES];


#endif
