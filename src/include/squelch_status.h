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

#include "globals.h"

enum SQL_STATE { STATE_OFF, STATE_ON };

struct struct_mode_state {
	trx_mode mode;
	int      squelch_level;
	int      squelch_state;
	int      tx_atten;
	int      afc_state;
	int      reverse;
};

extern void set_mode_squelch( trx_mode mode, double sqlch);
extern double get_mode_squelch( trx_mode mode );

extern void set_mode_squelch_onoff( trx_mode mode, int sql);
extern int get_mode_squelch_onoff( trx_mode mode );

extern void set_mode_txlevel( trx_mode mode, double sqlch);
extern double get_mode_txlevel( trx_mode mode );

extern void set_mode_afc( trx_mode mode, int sql);
extern int get_mode_afc( trx_mode mode );

extern void set_mode_reverse( trx_mode mode, int rev );
extern int get_mode_reverse( trx_mode mode );

extern void save_mode_state();
extern void load_mode_state();

#endif
