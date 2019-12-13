// ----------------------------------------------------------------------------
// nanoIO.h  --  Interface to Arduino Nano keyer
//
// Copyright (C) 2018
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
// ----------------------------------------------------------------------------

#ifndef _TINYIO_H
#define _TINYIO_H

#include <math.h>
#include <string>
#include <cstring>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>

#include <FL/Fl.H>

#include "debug.h"

#include "fl_digi.h"
#include "confdialog.h"

#include "status.h"
#include "serial.h"
#include "qrunner.h"
#include "threads.h"
#include "FTextRXTX.h"

extern bool use_nanoIO;
extern bool nanoIO_isCW;

extern bool open_nanoIO();
extern void close_nanoIO();

extern bool open_nanoCW();

extern void close_nanoIO();

extern char nano_read_byte(int &);
extern std::string nano_readString();

extern void nano_send_char(int c);
extern void nano_sendString (const std::string &s);

extern void nano_set_baud(int bd);
extern void nano_mark_polarity(int v);

extern void nano_PTT(int val);
extern void nano_cancel_transmit();

extern std::string nano_serial_read();
extern int  nano_serial_write(char c);

extern void set_nanoIO();
extern void set_nanoCW();
extern void set_nanoWPM(int wpm);
extern void set_nano_keyerWPM(int wpm);
extern void set_nanoIO_keyer(int indx);

extern void set_nano_dash2dot(float wt);
extern void nano_CW_query();
extern void nano_help();
extern void nano_CW_save();
extern void nanoCW_tune(int val);
extern void set_nanoIO_incr();
extern void nanoIO_set_cw_ptt();
extern void nanoIO_use_pot();
extern void set_nanoIO_min_max();
extern void nanoIO_read_pot();
extern void set_paddle_WPM(int);
extern void nanoIO_correction();

extern void nano_serial_flush();

extern void nanoIO_wpm_cal();

#endif
