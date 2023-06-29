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

static int firstuse = 1;

#define SS_160  1800000
#define SS_80   3500000
#define SS_75   3800000
#define SS_40   7000000
#define SS_30   10100000
#define SS_20   14000000
#define SS_17   18068000
#define SS_15   21000000
#define SS_12   24890000
#define SS_10   28000000
#define SS_6    50000000
#define SS_2    144000000
#define SS_220  222000000
#define SS_440  420000000
#define SS_HI   902000000

ModeBand modeband;

void show_band_mode_change()
{
	if (progdefaults.txlevel_by_mode) {
		progStatus.txlevel = modeband.get_mode_txlevel();
		cntTxLevel->value(progStatus.txlevel);
	}

	if (progdefaults.reverse_by_mode) {
		progStatus.reverse = modeband.get_mode_reverse();
		if (active_modem->get_cap() & modem::CAP_REV) {
			wf->btnRev->value(progStatus.reverse);
			wf->btnRev->activate();
		}
		else {
			wf->btnRev->value(0);
			wf->btnRev->deactivate();
		}
	}

	if (progdefaults.afc_by_mode) {
		progStatus.afconoff = modeband.get_mode_afc();
		if (active_modem->get_cap() & modem::CAP_AFC) {
			btnAFC->value(progStatus.afconoff);
			btnAFC->activate();
		}
		else {
			btnAFC->value(0);
			btnAFC->deactivate();
		}
	}

	if (progdefaults.sqlch_by_mode) {
		progStatus.sldrSquelchValue = modeband.get_mode_squelch();
		progStatus.sqlonoff = modeband.get_mode_squelch_onoff();
		sldrSquelch->value(progStatus.sldrSquelchValue);
		btnSQL->value(progStatus.sqlonoff);
	}

}

const char * ModeBand::bands[NUMBANDS] = {
	"lo", "160", "80",  "75", "40", "30", "20", 
	"17", "15", "12", "10", "6", "2", "220", "440", "hi"
};

bool ModeBand::band_changed() {
	int band = -1;

	if (wf->rfcarrier() < SS_160) band = BND_LO;
	else if (wf->rfcarrier() < SS_80) band = BND_160;
	else if (wf->rfcarrier() < SS_75) band = BND_80;
	else if (wf->rfcarrier() < SS_40) band = BND_75;
	else if (wf->rfcarrier() < SS_30) band = BND_40;
	else if (wf->rfcarrier() < SS_20) band = BND_30;
	else if (wf->rfcarrier() < SS_17) band = BND_20;
	else if (wf->rfcarrier() < SS_15) band = BND_17;
	else if (wf->rfcarrier() < SS_12) band = BND_15;
	else if (wf->rfcarrier() < SS_10) band = BND_12;
	else if (wf->rfcarrier() < SS_6)  band = BND_10;
	else if (wf->rfcarrier() < SS_2)  band = BND_6;
	else if (wf->rfcarrier() < SS_220) band = BND_2;
	else if (wf->rfcarrier() < SS_440) band = BND_220;
	else if (wf->rfcarrier() < SS_HI) band = BND_440;
	else band = BND_HI;

	if (band_in_use != band) {
		band_in_use = band;
		return true;
	}
	return false;
}

bool ModeBand::mode_changed() {
	trx_mode mode = active_modem->get_mode();
	if (mode != mode_in_use) {
		mode_in_use = mode;
		return true;
	}
	return false;
}

void ModeBand::band_mode_change() {
	if (!active_modem) {
		std::cout << "oh shit!!" << std::endl;
		return;
	}
	bool bc = band_changed();
	bool mc = mode_changed();
	if (bc || mc ) {
		REQ(show_band_mode_change);
	}
}

void ModeBand::init() {
	for (size_t m = 0; m < NUM_MODES; m++) {
		for (size_t b = 0; b < NUMBANDS; b++) {
			mode_bands[m][b].txlevel = -120;
			mode_bands[m][b].rev = false;
			mode_bands[m][b].afc = true;
			mode_bands[m][b].sqstate = true;
			mode_bands[m][b].sqlevel = 300;
		}
	}
}

void ModeBand::save_mode_state() 
{
	std::string pref;
	Fl_Preferences spref(HomeDir.c_str(), "w1hkj.com", "mode_state");

	spref.set("firstuse", 0);

	for (size_t m = 0; m < NUM_MODES; m++) {
		for (size_t b = 0; b < NUMBANDS; b++) {

			pref.assign(mode_info[m].sname).append(".tx_").append(bands[b]);
			spref.set(pref.c_str(), mode_bands[m][b].txlevel);

			pref.assign(mode_info[m].sname).append(".rev_").append(bands[b]);
			spref.set(pref.c_str(), mode_bands[m][b].rev);

			pref.assign(mode_info[m].sname).append(".afc_").append(bands[b]);
			spref.set(pref.c_str(), mode_bands[m][b].afc);

			pref.assign(mode_info[m].sname).append(".sqstate_").append(bands[b]);
			spref.set(pref.c_str(), mode_bands[m][b].sqstate);

			pref.assign(mode_info[m].sname).append(".sqlevel_").append(bands[b]);
			spref.set(pref.c_str(), mode_bands[m][b].sqlevel);
		}
	}
}

void ModeBand::load_mode_state() {
	std::string pref;
	Fl_Preferences spref(HomeDir.c_str(), "w1hkj.com", "mode_state");

	spref.get("firstuse", firstuse, firstuse);
	if (firstuse)
		return;

	int val = 0;

	for (size_t m = MODE_NULL; m <= NUM_RXTX_MODES; m++) {
		for (size_t b = BND_LO; b <= BND_HI; b++) {

			pref.assign(mode_info[m].sname).append(".tx_").append(bands[b]);
			spref.get(pref.c_str(), val, mode_bands[m][b].txlevel);
			mode_bands[m][b].txlevel = val;

			pref.assign(mode_info[m].sname).append(".rev_").append(bands[b]);
			spref.get(pref.c_str(), val, mode_bands[m][b].rev);
			mode_bands[m][b].rev = val;

			pref.assign(mode_info[m].sname).append(".afc_").append(bands[b]);
			spref.get(pref.c_str(), val, mode_bands[m][b].afc);
			mode_bands[m][b].afc = val;

			pref.assign(mode_info[m].sname).append(".sqstate_").append(bands[b]);
			spref.get(pref.c_str(), val, mode_bands[m][b].sqstate);
			mode_bands[m][b].sqstate = val;

			pref.assign(mode_info[m].sname).append(".sqlevel_").append(bands[b]);
			spref.get(pref.c_str(), val, mode_bands[m][b].sqlevel);
			mode_bands[m][b].sqlevel = val;

		}
	}
}

void ModeBand::set_mode_squelch(double val)
{
	size_t mode = (size_t)active_modem->get_mode();

	if (mode < MODE_NULL || mode > NUM_RXTX_MODES) return;
	if (band_in_use < BND_LO || band_in_use > BND_HI) return;

	mode_bands[mode][band_in_use].sqlevel = val * 10;
}

double ModeBand::get_mode_squelch()
{
	size_t mode = (size_t)active_modem->get_mode();

	if (mode < MODE_NULL || mode > NUM_RXTX_MODES) return 0;
	if (band_in_use < BND_LO || band_in_use > BND_HI) return 0;

	return mode_bands[mode][band_in_use].sqlevel / 10.0;
}

void ModeBand::set_mode_squelch_onoff(int val)
{
	size_t mode = (size_t)active_modem->get_mode();

	if (mode < MODE_NULL || mode > NUM_RXTX_MODES) return;
	if (band_in_use < BND_LO || band_in_use > BND_HI) return;

	mode_bands[mode][band_in_use].sqstate = val;
}

int ModeBand::get_mode_squelch_onoff()
{
	size_t mode = (size_t)active_modem->get_mode();

	if (mode < MODE_NULL || mode > NUM_RXTX_MODES) return 0;
	if (band_in_use < BND_LO || band_in_use > BND_HI) return 0;

	return mode_bands[mode][band_in_use].sqstate;
}

void ModeBand::set_mode_txlevel(double val)
{
	size_t mode = (size_t)active_modem->get_mode();

	if (mode < MODE_NULL || mode > NUM_RXTX_MODES) return;
	if (band_in_use < BND_LO || band_in_use > BND_HI) return;

	mode_bands[mode][band_in_use].txlevel = val * 10.0;
}

double ModeBand::get_mode_txlevel()
{
	size_t mode = (size_t)active_modem->get_mode();

	if (mode < MODE_NULL || mode > NUM_RXTX_MODES) return 0;
	if (band_in_use < BND_LO || band_in_use > BND_HI) return 0;

	return mode_bands[mode][band_in_use].txlevel / 10;
}

void ModeBand::set_mode_afc(int val)
{
	size_t mode = (size_t)active_modem->get_mode();

	if (mode < MODE_NULL || mode > NUM_RXTX_MODES) return;
	if (band_in_use < BND_LO || band_in_use > BND_HI) return;

	mode_bands[mode][band_in_use].afc = val;
}

int ModeBand::get_mode_afc() { 
	size_t mode = (size_t)active_modem->get_mode();

	if (mode < MODE_NULL || mode > NUM_RXTX_MODES) return 0;
	if (band_in_use < BND_LO || band_in_use > BND_HI) return 0;

	return mode_bands[mode][band_in_use].afc;
}

void ModeBand::set_mode_reverse(int val )
{
	size_t mode = (size_t)active_modem->get_mode();

	if (mode < MODE_NULL || mode > NUM_RXTX_MODES) return;
	if (band_in_use < BND_LO || band_in_use > BND_HI) return;

	mode_bands[mode][band_in_use].rev = val;
}

int ModeBand::get_mode_reverse()
{
	size_t mode = (size_t)active_modem->get_mode();

	if (mode < MODE_NULL || mode > NUM_RXTX_MODES) return 0;
	if (band_in_use < BND_LO || band_in_use > BND_HI) return 0;

	return mode_bands[mode][band_in_use].rev;
}

