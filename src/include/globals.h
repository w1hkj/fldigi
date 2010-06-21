// ----------------------------------------------------------------------------
// globals.h  --  constants, variables, arrays & functions that need to be
//                  outside of any thread
//
// Copyright (C) 2006-2007
//		Dave Freese, W1HKJ
// Copyright (C) 2007-2010
//		Stelios Bounanos, M0GLD
//
// This file is part of fldigi.  Adapted in part from code contained in gmfsk
// source code distribution.
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

enum {
	MODE_PREV = -2,
	MODE_NEXT,

	MODE_CW,

	MODE_CONTESTIA,

	MODE_DOMINOEX4,
	MODE_DOMINOEX5,
	MODE_DOMINOEX8,
	MODE_DOMINOEX11,
	MODE_DOMINOEX16,
	MODE_DOMINOEX22,
	MODE_DOMINOEX_FIRST = MODE_DOMINOEX4,
	MODE_DOMINOEX_LAST = MODE_DOMINOEX22,

	MODE_FELDHELL,
	MODE_SLOWHELL,
	MODE_HELLX5,
	MODE_HELLX9,
	MODE_FSKHELL,
	MODE_FSKH105,
	MODE_HELL80,
	MODE_HELL_FIRST = MODE_FELDHELL,
	MODE_HELL_LAST = MODE_HELL80,

	MODE_MFSK8,
	MODE_MFSK16,
	MODE_MFSK32,
// experimental modes
	MODE_MFSK4,
	MODE_MFSK11,
	MODE_MFSK22,
	MODE_MFSK31,
	MODE_MFSK64,
	MODE_MFSK_FIRST = MODE_MFSK8,
	MODE_MFSK_LAST = MODE_MFSK64,

	MODE_MT63_500,
	MODE_MT63_1000,
	MODE_MT63_2000,
	MODE_MT63_FIRST = MODE_MT63_500,
	MODE_MT63_LAST = MODE_MT63_2000,

	MODE_PSK31,
	MODE_PSK63,
	MODE_PSK63F,
	MODE_PSK125,
	MODE_PSK250,
	MODE_PSK500,
	MODE_QPSK31,
	MODE_QPSK63,
	MODE_QPSK125,
	MODE_QPSK250,
	MODE_QPSK500,
	MODE_PSK125R,
	MODE_PSK250R,
	MODE_PSK500R,
	MODE_PSK_FIRST = MODE_PSK31,
	MODE_PSK_LAST = MODE_PSK500R,

	MODE_OLIVIA,

	MODE_RTTY,

	MODE_THOR4,
	MODE_THOR5,
	MODE_THOR8,
	MODE_THOR11,
//	MODE_TSOR11,
	MODE_THOR16,
	MODE_THOR22,
	MODE_THOR_FIRST = MODE_THOR4,
	MODE_THOR_LAST = MODE_THOR22,

	MODE_THROB1,
	MODE_THROB2,
	MODE_THROB4,
	MODE_THROBX1,
	MODE_THROBX2,
	MODE_THROBX4,
	MODE_THROB_FIRST = MODE_THROB1,
	MODE_THROB_LAST = MODE_THROBX4,

	MODE_SSB,
	MODE_WWV,
	MODE_ANALYSIS,

	NUM_MODES,
	NUM_RXTX_MODES = NUM_MODES - 2
};

typedef intptr_t trx_mode;

struct mode_info_t {
	trx_mode mode;
	class modem **modem;
	const char *sname;
	const char *name;
	const char *pskmail_name;
	const char *adif_name;
	const char *vid_name;
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

#include <bitset>
class mode_set_t : public std::bitset<NUM_RXTX_MODES> { };

enum band_t {
	BAND_160M, BAND_80M, BAND_75M, BAND_60M, BAND_40M, BAND_30M, BAND_20M,
	BAND_17M, BAND_15M, BAND_12M, BAND_10M, BAND_6M, BAND_4M, BAND_2M, BAND_125CM,
	BAND_70CM, BAND_33CM, BAND_23CM, BAND_13CM, BAND_9CM, BAND_6CM, BAND_3CM, BAND_125MM,
	BAND_6MM, BAND_4MM, BAND_2P5MM, BAND_2MM, BAND_1MM, BAND_OTHER, NUM_BANDS
};

band_t band(long long freq_hz);
band_t band(const char* freq_mhz);
const char* band_name(band_t b);
const char* band_name(const char* freq_mhz);
const char* band_freq(band_t b);
const char* band_freq(const char* band_name);

#endif
