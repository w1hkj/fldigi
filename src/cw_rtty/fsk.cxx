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

#include <config.h>
#include <errno.h>

#include "trx.h"
#include "fsk.h"
#include "serial.h"

#include <iostream>

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <stdio.h>

#ifdef __WIN32__
#  include <windows.h>
//#  include <chrono>
#  include <math.h>
#else
#  ifdef __APPLE__
//#    include <chrono>
#    include <sys/event.h>
#    include <sys/time.h>
#  else
//#    include <chrono>
#    include <sys/timerfd.h>
#  endif
#endif

#include "threads.h"
#include "util.h"
#include "configuration.h"

#include "debug.h"

static pthread_mutex_t fsk_mutex = PTHREAD_MUTEX_INITIALIZER;

//using namespace std;
//using namespace std::chrono;

//static std::chrono::time_point<std::chrono::steady_clock> tp_start, tp_end;

char FSK::letters[32] = {
	'\0',	'E',	'\n',	'A',	' ',	'S',	'I',	'U',
	'\r',	'D',	'R',	'J',	'N',	'F',	'C',	'K',
	'T',	'Z',	'L',	'W',	'H',	'Y',	'P',	'Q',
	'O',	'B',	'G',	' ',	'M',	'X',	'V',	' '
};

/*
 * U.S. version of the figures case.
 */
char FSK::figures[32] = {
	'\0',	'3',	'\n',	'-',	' ',	'\a',	'8',	'7',
	'\r',	'$',	'4',	'\'',	',',	'!',	':',	'(',
	'5',	'"',	')',	'2',	'#',	'6',	'0',	'1',
	'9',	'?',	'&',	' ',	'.',	'/',	';',	' '
};

const char * FSK::ascii[256] = {
	"<NUL>", "<SOH>", "<STX>", "<ETX>", "<EOT>", "<ENQ>", "<ACK>", "<BEL>",
	"<BS>",  "<TAB>", "<LF>",   "<VT>",  "<FF>",  "<CR>",  "<SO>",  "<SI>",
	"<DLE>", "<DC1>", "<DC2>", "<DC3>", "<DC4>", "<NAK>", "<SYN>", "<ETB>",
	"<CAN>", "<EM> ", "<SUB>", "<FIG>", "<FS>",  "<GS>",  "<RS>",  "<LTR>",
	" ", "!", "\"","#", "$", "%", "&", "\'",
	"(", ")", "*", "+", ",", "-", ".", "/",
	"0", "1", "2", "3", "4", "5", "6", "7",
	"8", "9", ":", ";", "<", "=", ">", "?",
	"@", "A", "B", "C", "D", "E", "F", "G",
	"H", "I", "J", "K", "L", "M", "N", "O",
	"P", "Q", "R", "S", "T", "U", "V", "W",
	"X", "Y", "Z", "[", "\\","]", "^", "_",
	"`", "a", "b", "c", "d", "e", "f", "g",
	"h", "i", "j", "k", "l", "m", "n", "o",
	"p", "q", "r", "s", "t", "u", "v", "w",
	"x", "y", "z", "{", "|", "}", "~", "<DEL>",
	"<128>", "<129>", "<130>", "<131>", "<132>", "<133>", "<134>", "<135>",
	"<136>", "<137>", "<138>", "<139>", "<140>", "<141>", "<142>", "<143>",
	"<144>", "<145>", "<146>", "<147>", "<148>", "<149>", "<150>", "<151>",
	"<152>", "<153>", "<154>", "<155>", "<156>", "<157>", "<158>", "<159>",
	"<160>", "<161>", "<162>", "<163>", "<164>", "<165>", "<166>", "<167>",
	"<168>", "<169>", "<170>", "<171>", "<172>", "<173>", "<174>", "<175>",
	"<176>", "<177>", "<178>", "<179>", "<180>", "<181>", "<182>", "<183>",
	"<184>", "<185>", "<186>", "<187>", "<188>", "<189>", "<190>", "<191>",
	"<192>", "<193>", "<194>", "<195>", "<196>", "<197>", "<198>", "<199>",
	"<200>", "<201>", "<202>", "<203>", "<204>", "<205>", "<206>", "<207>",
	"<208>", "<209>", "<210>", "<211>", "<212>", "<213>", "<214>", "<215>",
	"<216>", "<217>", "<218>", "<219>", "<220>", "<221>", "<222>", "<223>",
	"<224>", "<225>", "<226>", "<227>", "<228>", "<229>", "<230>", "<231>",
	"<232>", "<233>", "<234>", "<235>", "<236>", "<237>", "<238>", "<239>",
	"<240>", "<241>", "<242>", "<243>", "<244>", "<245>", "<246>", "<247>",
	"<248>", "<249>", "<250>", "<251>", "<252>", "<253>", "<254>", "<255>"
};


FSK::FSK()
{
	str_buff.clear();
	start_bits = 0;
	stop_bits = 0;
	chr_bits = 0;
	chr_out = 0;
	shift_state  = FSK_FIGURES;
	shift = 0;
	_shift_on_space = false;

	init_fsk_thread();

	shared_port = false;
}

FSK::~FSK()
{
	exit_fsk_thread();
	if (shared_port)
		return;
	fsk_port->ClosePort();
}

void FSK::open_port(std::string device_name)
{
	if (shared_port)
		return;

	serial_device = device_name;
	fsk_port->Device (serial_device.c_str());
	fsk_port->OpenPort();
}

void FSK::fsk_shares_port(Cserial *shared_device)
{
	shared_port = true;
	fsk_port = shared_device;
}

bool FSK::sending() {
	return str_buff.length() > 0;
}

void FSK::send(const char ch) {
	str_buff.clear();
	str_buff += ch;
}

void FSK::append(const char ch) {
	str_buff += ch;
}

void FSK::send(std::string s) {
	str_buff = s;
}

void FSK::append(std::string s) {
	str_buff.append(s);
}

void FSK::abort() {
	str_buff.clear();
}

void FSK::fsk_out (bool state) {
	if (_dtr) {
		fsk_port->SetDTR(_reverse ? state : !state);
	} else {
		fsk_port->SetRTS(_reverse ? state : !state);
	}
}

int FSK::baudot_enc(int data) {
	data &= 0xFF;
	int i;

	if (islower(data))
		data = toupper(data);

	if (data == ' ')  // always force space to be a LETTERS char
		return FSK_LETTERS | 4;

	for (i = 0; i < 32; i++) {
		if (data == letters[i]) {
			return (i | FSK_LETTERS);
		}
		if (data == figures[i]) {
			return (i | FSK_FIGURES);
		}
	}
	return shift_state | 4;
}

void FSK::send_baudot(int ch)
{
	if (ch == LTRS) shift_state = FSK_LETTERS;
	else if (ch == FIGS) shift_state = FSK_FIGURES;

// 1 start bit
	fsk_out(FSK_SPACE);
	accu_sleep(BITLEN);

// 5 data bits
	fsk_out((ch & 0x01) == 0x01 ? FSK_MARK : FSK_SPACE);
	accu_sleep(BITLEN);

	fsk_out((ch & 0x02) == 0x02 ? FSK_MARK : FSK_SPACE);
	accu_sleep(BITLEN);

	fsk_out((ch & 0x04) == 0x04 ? FSK_MARK : FSK_SPACE);
	accu_sleep(BITLEN);

	fsk_out((ch & 0x08) == 0x08 ? FSK_MARK : FSK_SPACE);
	accu_sleep(BITLEN);

	fsk_out((ch & 0x10) == 0x10 ? FSK_MARK : FSK_SPACE);
	accu_sleep(BITLEN);

// 1.5 or 2 stop bits
	fsk_out(FSK_MARK);
	accu_sleep(BITLEN * 
		(progdefaults.fsk_STOPBITS ? 1.5 : 2.0) );
}

extern state_t trx_state;
static int idles = 4;

int FSK::callback_method()
{
	if (trx_state != STATE_TX) {
		shift_state = FSK_UNKNOWN; // will force at least one LTRS byte
		shift = 0;
		stop_bits = 0;
		idles = 4;
		accu_sleep(BITLEN);
		return 0;
	}

	if (str_buff.empty() || idles) {
		send_baudot(LTRS);
		if (idles) idles--;
	} else {
		chr_out = baudot_enc(str_buff[0]);
		if ((chr_out & 0x300) != shift_state) {
			shift_state = chr_out & 0x300;
			if (shift_state == FSK_LETTERS) {
				send_baudot(LTRS);
			} else {
				send_baudot(FIGS);
			}
		}
		str_buff.erase(0,1);
		send_baudot(chr_out & 0x1F);
	}
	return 0;
}

void *fsk_loop(void *data)
{
	SET_THREAD_ID(FSK_TID);

	FSK *fsk = (FSK *)data;
	while (1) {
		fsk->callback_method();
		{
			guard_lock tlock (&fsk_mutex);
			if (fsk->fsk_loop_terminate) goto _exit;
		}
	}
_exit:
	return NULL;
}

int FSK::init_fsk_thread()
{
	fsk_loop_terminate = false;

	if(pthread_mutex_init(&fsk_mutex, NULL)) {
		LOG_ERROR("FSK tabortimer thread create fail (pthread_mutex_init)");
		return 0;
	}
	if (pthread_create(&fsk_thread, NULL, fsk_loop, this)) {
		LOG_ERROR("FSK timer thread create fail (pthread_create)");
		return 0;
	}
	return 1;
}

void FSK::exit_fsk_thread()
{
	{
		guard_lock tlock (&fsk_mutex);
		fsk_loop_terminate = true;
		MilliSleep(50);
	}
	pthread_join(fsk_thread, NULL);

	fsk_loop_terminate = false;
}

