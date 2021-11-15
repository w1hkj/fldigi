// ----------------------------------------------------------------------------
// fsk.cxx  --  FSK signal generator
//
// Copyright (C) 2021
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.
//
// This code bears some resemblance to code contained in gmfsk from which
// it originated.  Much has been changed, but credit should still be
// given to Tomi Manninen (oh2bns@sral.fi), who so graciously distributed
// his gmfsk modem under the GPL.
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

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include <iostream>
#include <string>

//#include "io_timer.h"
#include "serial.h"

//#include "io_timer.h"

#ifndef FSK_H
#define FSK_H

class FSK
{
// time (msec) for one symbollen @ 45.45 baud
#define BITLEN 0.022 //22

#define FSK_UNKNOWN	0x000
#define	FSK_LETTERS	0x100
#define	FSK_FIGURES	0x200

#define LTRS 0x1F
#define FIGS 0x1B

#define FSK_MARK  1
#define FSK_SPACE 0

public:
	FSK();
	~FSK();

	bool open();
	bool close();
	void abort();

	bool sending();

	void send(const char ch);
	void send(std::string s);
	void append(const char ch);
	void append(std::string s);

	void shift_on_space(bool b) { _shift_on_space = b; }
	bool shift_on_space() { return _shift_on_space; }

	void dtr(bool b) { _dtr = b; }
	bool dtr() { return _dtr; }

	void rts(bool b) { _dtr = !b; }
	bool rts() { return !_dtr; }

	void reverse(bool b) { _reverse = b; }
	bool reverse() { return _reverse; }

	void   open_port(std::string device_name);

	void device(std::string device_name) {
		serial_device = device_name;
	}

	void fsk_shares_port(Cserial *shared_device);

//	size_t io_timer_id;

private:

	Cserial		*fsk_port;

	std::string serial_device;

	bool   shared_port; // default is false
	static char letters[];
	static char figures[];
	static const char *ascii[];

	int  shift;
	bool _shift_on_space;
	bool _dtr;
	bool _reverse;

	int mode;
	int  shift_state;
	int start_bits;
	int stop_bits;
	int chr_bits;

    std::string str_buff;
    int   chr_out;

	int baudot_enc(int);
	void send_baudot(int ch);
	void fsk_out (bool);

public:

	int callback_method();

	int init_fsk_thread();
	void exit_fsk_thread();

	bool	fsk_loop_terminate;

	pthread_t fsk_thread;

friend
	void *fsk_loop(void *data);

};

#endif
