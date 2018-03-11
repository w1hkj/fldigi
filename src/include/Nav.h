// ----------------------------------------------------------------------------
// Nav.cxx  --  Interface to Arduino Nano Nav keyer
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

#ifndef _USNAVFSK_H
#define _USNAVFSK_H

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

extern bool use_Nav;

extern bool open_NavFSK();
extern void close_NavFSK();

extern bool open_NavConfig();
extern void close_NavConfig();

extern bool Nav_read_byte(Cserial &serial, unsigned char &byte);
extern std::string Nav_read_string(Cserial &serial, int msec_wait, std::string find);

extern void Nav_send_char(int c);
extern void Nav_sendString (Cserial &serial, const std::string &s);

extern void Nav_PTT(int val);
extern void Nav_write_eeprom();
extern void Nav_restore_eeprom();

extern std::string Nav_serial_read(Cserial &serial);

extern void Nav_set_channel_1_att(int);
extern void Nav_set_channel_2_att(int);
extern void Nav_set_rf_att(int);
extern void Nav_set_led(int);
extern void Nav_set_cat_led(int);
extern void Nav_set_wk_ptt(int);

extern void Nav_set_baud(int);
extern void Nav_set_stopbits(int);
extern void Nav_set_polarity(int);

extern void Nav_set_sidetone(int);
extern void Nav_set_ptt(int);

#endif
