// ----------------------------------------------------------------------------
//
// squelch_status.h
//
// Copyright (C) 2021
//              David Freese, W1HKJ
//
// This file is part of fldigi
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
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#ifndef MODE_STATUS_H
#define MODE_STATUS_H

#include <string>

#include "configuration.h"
#include "status.h"
#include "main.h"
#include "fl_digi.h"
#include "trx.h"
#include "globals.h"
#include "qrunner.h"

extern void show_band_mode_change();

// C++ code support

class ModeBand {

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

struct MODEBAND { int txlevel; int rev; int afc; int sqstate; int sqlevel; };

enum SQL_STATE { STATE_OFF, STATE_ON };

enum BANDS {
	BND_LO, BND_160, BND_80, BND_75, BND_40, BND_30, BND_20, 
	BND_17, BND_15, BND_12, BND_10, BND_6, BND_2, BND_220, BND_440, BND_HI, NUMBANDS
};

static const char *bands[];

private:
	MODEBAND mode_bands[NUM_MODES][NUMBANDS];

	trx_mode mode_in_use;
	int band_in_use;

	bool band_changed();
	bool mode_changed();

	void init();

public:
	ModeBand() {
		mode_in_use = -1;
		band_in_use = -1;
		init();
		load_mode_state();
	}
	~ModeBand() {
		save_mode_state();
	}

	void set_mode_squelch( double sqlch);
	double get_mode_squelch();

	void set_mode_squelch_onoff(int sql);
	int get_mode_squelch_onoff();

	void set_mode_txlevel(double sqlch);
	double get_mode_txlevel();

	void set_mode_afc(int sql);
	int get_mode_afc();

	void set_mode_reverse(int rev );
	int get_mode_reverse();

	void band_mode_change();
	void save_mode_state();
	void load_mode_state();

};

extern ModeBand modeband;

#endif
