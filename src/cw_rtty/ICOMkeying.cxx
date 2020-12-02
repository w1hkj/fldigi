// ----------------------------------------------------------------------------
// ICOMkeying.cxx   serial string CW interface to Elecraft transceivers
//
// Copyright (C) 2020
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

#include <iostream>
#include <string>

#include "ICOMkeying.h"
#include "configuration.h"
#include "rigio.h"
#include "threads.h"
#include "debug.h"
#include "rigsupport.h"
#include "morse.h"
#include "fl_digi.h"
#include "util.h"

int ICOMwpm = 0;
bool use_ICOMkeyer = false;
static std::string ICOMcmd;
static cMorse ICOMmorse;
char lastICOMchar = 0;

static std::string hexvals = "0123456789ABCDEF";
int CIVaddr(std::string s)
{
	int val = 0;
	if (s.length() != 2) return 0;
	if (hexvals.find(s[0]) == std::string::npos) return 0;
	if (hexvals.find(s[1]) == std::string::npos) return 0;
	val = hexvals.find(s[0]) * 16 + hexvals.find(s[1]);
	return val;
}

std::string ICOMheader()
{
	std::string s;
	s.assign("\xFE\xFE");
	s += ((char)CIVaddr(progdefaults.ICOMcivaddr));
	s.append("\xE0");
	return s;
}

std::string hexstr(std::string s)
{
	std::string hex;
	for (size_t i = 0; i < s.length(); i++) {
		hex.append(" x");
		hex += hexvals[(s[i] & 0xFF) >> 4];
		hex += hexvals[s[i] & 0xF];
	}
	return hex;
}


void set_ICOMkeyer() {
	ICOMwpm = progdefaults.CWspeed;
	if (ICOMwpm < 6) ICOMwpm = 6;
	if (ICOMwpm > 48) ICOMwpm = 48;
	progdefaults.CWspeed = ICOMwpm;
	int hexwpm = (ICOMwpm - 6) * 6 + 2;
	ICOMcmd.assign(ICOMheader());
	ICOMcmd.append("\x14\x0C");
	ICOMcmd += (char)(hexwpm / 100);
	ICOMcmd += (char)(((hexwpm % 100) / 10) * 16 + (hexwpm % 10));
	ICOMcmd += '\xFD';
	if (progdefaults.fldigi_client_to_flrig) {
		xmlrpc_priority(hexstr(ICOMcmd));
	} else {
		guard_lock ser_guard( &rigCAT_mutex);
		rigio.WriteBuffer((unsigned char *)ICOMcmd.c_str(), ICOMcmd.length());
	}
	MilliSleep(100);
}

void ICOMkeyer_send_char(int c)
{
	if (ICOMwpm != progdefaults.CWspeed) {
		set_ICOMkeyer();
	}

	if (c == GET_TX_CHAR_NODATA || c == 0x0d) {
		MilliSleep(50);
		return;
	}

	int set_time = 0;

	c = toupper(c);
	if (c < ' ') c = ' ';
	if (c > 'Z') c = ' ';

	int tc = 1200 / progdefaults.CWspeed;
	if (progdefaults.CWusefarnsworth && (progdefaults.CWspeed > progdefaults.CWfarnsworth))
		tc = 1200 / progdefaults.CWfarnsworth;

	if (c == ' ') {
		if (lastICOMchar == ' ')
			tc *= 7;
		else
			tc *= 5;
	} else {
		tc *= (ICOMmorse.tx_length(c));

		ICOMcmd.assign(ICOMheader());
		ICOMcmd.append("\x17");
		ICOMcmd += (char)(c);
		ICOMcmd += '\xFD';

		if (progdefaults.fldigi_client_to_flrig) {
			xmlrpc_priority(ICOMcmd);
		} else if (progdefaults.chkUSERIGCATis) {
			guard_lock ser_guard( &rigCAT_mutex);
			rigio.WriteBuffer((unsigned char *)ICOMcmd.c_str(), ICOMcmd.length());
		}
	}

	tc -= progdefaults.CATkeying_compensation / (progdefaults.CWspeed  * 6);
	tc = int(tc);
	if (set_time < tc)
		MilliSleep((int)(tc - set_time));

	lastICOMchar = c;
}
