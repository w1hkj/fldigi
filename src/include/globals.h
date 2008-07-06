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

#include <stdint.h>
#include <string>

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
	MODE_SLOWHELL,
	MODE_HELLX5,
	MODE_HELLX9,
	MODE_FSKHELL,
	MODE_FSKH105,
	MODE_HELL80,

	MODE_MFSK8,
	MODE_MFSK11,
	MODE_MFSK16,
	MODE_MFSK22,
	MODE_MFSK32,

	MODE_MT63_500,
	MODE_MT63_1000,
	MODE_MT63_2000,

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

	MODE_THOR4,
	MODE_THOR5,
	MODE_THOR8,
	MODE_THOR11,
//	MODE_TSOR11,
	MODE_THOR16,
	MODE_THOR22,

	MODE_THROB1,
	MODE_THROB2,
	MODE_THROB4,
	MODE_THROBX1,
	MODE_THROBX2,
	MODE_THROBX4,

	MODE_WWV,
	MODE_ANALYSIS,

	NUM_MODES
};

typedef intptr_t trx_mode;

struct mode_info_t {
	trx_mode mode;
	class modem **modem;
	const char *sname;
	const char *name;
	const char *pskmail_name;
	const char *adif_name;
};
extern const struct mode_info_t mode_info[NUM_MODES];

class qrg_mode_t
{
public:
	long long rfcarrier;
	std::string rmode;
	int carrier;
	trx_mode mode;

	qrg_mode_t() : rfcarrier(0), rmode("NONE"), carrier(0), mode(NUM_MODES) { }
	qrg_mode_t(long long rfc_, std::string rm_, int c_, trx_mode m_)
                : rfcarrier(rfc_), rmode(rm_), carrier(c_), mode(m_) { }
	bool operator<(const qrg_mode_t& rhs) const
        {
		return rfcarrier < rhs.rfcarrier;
	}
	bool operator==(const qrg_mode_t& rhs) const
	{
		return rfcarrier == rhs.rfcarrier && rmode == rhs.rmode &&
		       carrier == rhs.carrier && mode == rhs.mode;
	}
        std::string str(void);
};
std::ostream& operator<<(std::ostream& s, const qrg_mode_t& m);
std::istream& operator>>(std::istream& s, qrg_mode_t& m);

#endif
