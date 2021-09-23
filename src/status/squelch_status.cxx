// ---------------------------------------------------------------------
// squelch_status.cxx
//
// Copyright (C) 2021
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
// ---------------------------------------------------------------------
//
// Save all floating point values as integers
//
// int_fval = fval * NNN where NNN is a factor of 10
//
// restore using fval = int_fval / NNN
//
// A work around for a bug in class preferences.  Read/Write of floating
// point values fails on read if locale is not EN_...
//
//----------------------------------------------------------------------
#include <config.h>

#include <iostream>
#include <fstream>
#include <string>

#include <FL/Fl_Preferences.H>

#include "squelch_status.h"
#include "configuration.h"
#include "status.h"
#include "main.h"

// { mode, squelch_level, squelch_state, tx_atten, afc_state, reverse }

struct struct_mode_state mode_state[NUM_MODES] = {
{ MODE_NULL, 10, STATE_ON, -30, STATE_ON, STATE_ON },

{ MODE_CW, 10, STATE_ON, -30, STATE_ON, STATE_ON },

{ MODE_CONTESTIA, 10, STATE_ON, -30, STATE_ON, STATE_ON },

{ MODE_CONTESTIA_4_125, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_CONTESTIA_4_250, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_CONTESTIA_4_500, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_CONTESTIA_4_1000, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_CONTESTIA_4_2000, 10, STATE_ON, -30, STATE_ON, STATE_ON },

{ MODE_CONTESTIA_8_125, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_CONTESTIA_8_250, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_CONTESTIA_8_500, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_CONTESTIA_8_1000, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_CONTESTIA_8_2000, 10, STATE_ON, -30, STATE_ON, STATE_ON },

{ MODE_CONTESTIA_16_250, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_CONTESTIA_16_500, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_CONTESTIA_16_1000, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_CONTESTIA_16_2000, 10, STATE_ON, -30, STATE_ON, STATE_ON },

{ MODE_CONTESTIA_32_1000, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_CONTESTIA_32_2000, 10, STATE_ON, -30, STATE_ON, STATE_ON },

{ MODE_CONTESTIA_64_500, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_CONTESTIA_64_1000, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_CONTESTIA_64_2000, 10, STATE_ON, -30, STATE_ON, STATE_ON },

{ MODE_DOMINOEXMICRO, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_DOMINOEX4, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_DOMINOEX5, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_DOMINOEX8, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_DOMINOEX11, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_DOMINOEX16, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_DOMINOEX22, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_DOMINOEX44, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_DOMINOEX88, 10, STATE_ON, -30, STATE_ON, STATE_ON },

{ MODE_FELDHELL, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_SLOWHELL, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_HELLX5, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_HELLX9, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_FSKH245, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_FSKH105, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_HELL80, 10, STATE_ON, -30, STATE_ON, STATE_ON },

{ MODE_MFSK8, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_MFSK16, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_MFSK32, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_MFSK4, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_MFSK11, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_MFSK22, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_MFSK31, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_MFSK64, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_MFSK128, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_MFSK64L, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_MFSK128L, 10, STATE_ON, -30, STATE_ON, STATE_ON },

{ MODE_WEFAX_576, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_WEFAX_288, 10, STATE_ON, -30, STATE_ON, STATE_ON },

{ MODE_NAVTEX, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_SITORB, 10, STATE_ON, -30, STATE_ON, STATE_ON },

{ MODE_MT63_500S, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_MT63_500L, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_MT63_1000S, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_MT63_1000L, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_MT63_2000S, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_MT63_2000L, 10, STATE_ON, -30, STATE_ON, STATE_ON },

{ MODE_PSK31, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_PSK63, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_PSK63F, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_PSK125, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_PSK250, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_PSK500, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_PSK1000, 10, STATE_ON, -30, STATE_ON, STATE_ON },

{ MODE_12X_PSK125, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_6X_PSK250, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_2X_PSK500, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_4X_PSK500, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_2X_PSK800, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_2X_PSK1000, 10, STATE_ON, -30, STATE_ON, STATE_ON },

{ MODE_QPSK31, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_QPSK63, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_QPSK125, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_QPSK250, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_QPSK500, 10, STATE_ON, -30, STATE_ON, STATE_ON },

{ MODE_8PSK125, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_8PSK125FL, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_8PSK125F, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_8PSK250, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_8PSK250FL, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_8PSK250F, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_8PSK500, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_8PSK500F, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_8PSK1000, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_8PSK1000F, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_8PSK1200F, 10, STATE_ON, -30, STATE_ON, STATE_ON },

{ MODE_OFDM_500F, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_OFDM_750F, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_OFDM_2000F, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_OFDM_2000, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_OFDM_3500, 10, STATE_ON, -30, STATE_ON, STATE_ON },

{ MODE_OLIVIA, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_OLIVIA_4_125, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_OLIVIA_4_250, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_OLIVIA_4_500, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_OLIVIA_4_1000, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_OLIVIA_4_2000, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_OLIVIA_8_125, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_OLIVIA_8_250, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_OLIVIA_8_500, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_OLIVIA_8_1000, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_OLIVIA_8_2000, 10, STATE_ON, -30, STATE_ON, STATE_ON },

{ MODE_OLIVIA_16_500, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_OLIVIA_16_1000, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_OLIVIA_16_2000, 10, STATE_ON, -30, STATE_ON, STATE_ON },

{ MODE_OLIVIA_32_1000, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_OLIVIA_32_2000, 10, STATE_ON, -30, STATE_ON, STATE_ON },

{ MODE_OLIVIA_64_500, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_OLIVIA_64_1000, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_OLIVIA_64_2000, 10, STATE_ON, -30, STATE_ON, STATE_ON },

{ MODE_RTTY, 10, STATE_ON, -30, STATE_ON, STATE_ON },

{ MODE_THORMICRO, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_THOR4, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_THOR5, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_THOR8, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_THOR11, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_THOR16, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_THOR22, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_THOR25x4, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_THOR50x1, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_THOR50x2, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_THOR100, 10, STATE_ON, -30, STATE_ON, STATE_ON },

{ MODE_THROB1, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_THROB2, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_THROB4, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_THROBX1, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_THROBX2, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_THROBX4, 10, STATE_ON, -30, STATE_ON, STATE_ON },

{ MODE_PSK125R, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_PSK250R, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_PSK500R, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_PSK1000R, 10, STATE_ON, -30, STATE_ON, STATE_ON },

{ MODE_4X_PSK63R, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_5X_PSK63R, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_10X_PSK63R, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_20X_PSK63R, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_32X_PSK63R, 10, STATE_ON, -30, STATE_ON, STATE_ON },

{ MODE_4X_PSK125R, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_5X_PSK125R, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_10X_PSK125R, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_12X_PSK125R, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_16X_PSK125R, 10, STATE_ON, -30, STATE_ON, STATE_ON },

{ MODE_2X_PSK250R, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_3X_PSK250R, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_5X_PSK250R, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_6X_PSK250R, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_7X_PSK250R, 10, STATE_ON, -30, STATE_ON, STATE_ON },

{ MODE_2X_PSK500R, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_3X_PSK500R, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_4X_PSK500R, 10, STATE_ON, -30, STATE_ON, STATE_ON },

{ MODE_2X_PSK800R, 10, STATE_ON, -30, STATE_ON, STATE_ON },

{ MODE_2X_PSK1000R, 10, STATE_ON, -30, STATE_ON, STATE_ON },

{ MODE_FSQ, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_IFKP, 10, STATE_ON, -30, STATE_ON, STATE_ON },

{ MODE_SSB, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_WWV, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_ANALYSIS, 10, STATE_ON, -30, STATE_ON, STATE_ON },
{ MODE_FMT, 10, STATE_ON, -30, STATE_ON, STATE_ON }

};

void set_mode_squelch( trx_mode mode, double sqlch)
{
	mode_state[mode].squelch_level = sqlch;
}

double get_mode_squelch( trx_mode mode )
{
	return mode_state[mode].squelch_level;
}

void set_mode_squelch_onoff( trx_mode mode, int sql)
{
	mode_state[mode].squelch_state = sql;
}

int get_mode_squelch_onoff( trx_mode mode )
{
	return mode_state[mode].squelch_state;
}

void set_mode_txlevel( trx_mode mode, double sqlch)
{
	mode_state[mode].tx_atten = sqlch * 10;
}

double get_mode_txlevel( trx_mode mode )
{
	return mode_state[mode].tx_atten / 10.0;
}

void set_mode_afc( trx_mode mode, int on)
{
	mode_state[mode].afc_state = on;
}

int get_mode_afc( trx_mode mode )
{
	return mode_state[mode].afc_state;
}

void set_mode_reverse( trx_mode mode, int on)
{
	mode_state[mode].reverse = on;
}

int get_mode_reverse( trx_mode mode )
{
	return mode_state[mode].reverse;
}

static int firstuse = 1;

void load_mode_state()
{
	int val = 10;
	int state = STATE_ON;
	Fl_Preferences spref(HomeDir.c_str(), "w1hkj.com", "mode_state");

	spref.get("firstuse", firstuse, firstuse);
	if (firstuse) {
		for (size_t n = MODE_NULL; n <= NUM_RXTX_MODES; n++) {
			mode_state[n].squelch_level = progStatus.sldrSquelchValue;
			mode_state[n].squelch_state = progStatus.sqlonoff;
			mode_state[n].tx_atten = 10 * progStatus.txlevel;
			mode_state[n].afc_state = progStatus.afconoff;
			mode_state[n].reverse = progStatus.reverse;
		}
		return;
	}

	for (size_t n = MODE_NULL; n <= NUM_RXTX_MODES; n++) {

		spref.get(
			std::string( mode_info[n].name).append(".squelch_level").c_str(),
			val,
			mode_state[n].squelch_level);
		mode_state[n].squelch_level = val;

		spref.get(
			std::string( mode_info[n].name).append(".squelch_state").c_str(),
			state,
			mode_state[n].squelch_state);
		mode_state[n].squelch_state = state;

		spref.get(
			std::string( mode_info[n].name).append(".tx_atten").c_str(),
			val,
			mode_state[n].tx_atten);
		mode_state[n].tx_atten = val;

		spref.get(
			std::string( mode_info[n].name).append(".afc_state").c_str(),
			state,
			mode_state[n].afc_state);
		mode_state[n].afc_state = state;

		spref.get(
			std::string( mode_info[n].name).append(".reverse").c_str(),
			state,
			mode_state[n].reverse);
		mode_state[n].reverse = state;

	}
}

void save_mode_state()
{
	Fl_Preferences spref(HomeDir.c_str(), "w1hkj.com", "mode_state");

	spref.set("firstuse", 0);

	for (size_t n = MODE_NULL; n <= NUM_RXTX_MODES; n++) {

		spref.set(
			std::string( mode_info[n].name).append(".squelch_level").c_str(),
			mode_state[n].squelch_level);

		spref.set( std::string(
			mode_info[n].name).append(".squelch_state").c_str(),
			mode_state[n].squelch_state);

		spref.set(
			std::string( mode_info[n].name).append(".tx_atten").c_str(),
			
			mode_state[n].tx_atten);

		spref.set( std::string(
			mode_info[n].name).append(".afc_state").c_str(),
			mode_state[n].afc_state);

		spref.set( std::string(
			mode_info[n].name).append(".reverse").c_str(),
			mode_state[n].reverse);
	}

}



