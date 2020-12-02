// ----------------------------------------------------------------------------
// KYkeying.cxx   serial string CW interface to Elecraft transceivers
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

#include "KYkeying.h"
#include "configuration.h"
#include "rigio.h"
#include "threads.h"
#include "debug.h"
#include "rigsupport.h"
#include "morse.h"
#include "fl_digi.h"

int KYwpm = 0;
bool use_KYkeyer = false;
static std::string KYcmd = "KY  ;";
static std::string KNWDcmd = "KY                         ;";
static std::string cmd;
static cMorse KYmorse;
static char lastKYchar = 0;

void set_KYkeyer()
{
	KYwpm = progdefaults.CWspeed;
	if (KYwpm < 8) KYwpm = 8;
	if (KYwpm > 50) KYwpm = 50;
	progdefaults.CWspeed = KYwpm;
	char cmd[10];
	snprintf(cmd, sizeof(cmd), "KS%03d;", KYwpm);
	if (progdefaults.fldigi_client_to_flrig) {
		xmlrpc_priority(cmd);
	} else {
		guard_lock ser_guard( &rigCAT_mutex);
		rigio.WriteBuffer((unsigned char *)cmd, strlen(cmd));
	}
	MilliSleep(50);
}

void KYkeyer_send_char(int c)
{
	if (KYwpm != progdefaults.CWspeed) {
		set_KYkeyer();
	}

	if (c == GET_TX_CHAR_NODATA || c == 0x0d) {
		MilliSleep(50);
		return;
	}

	c = toupper(c);
	if (c < ' ') c = ' ';
	if (c > 'Z') c = ' ';

	float tc = 1200.0 / progdefaults.CWspeed;
	if (progdefaults.CWusefarnsworth && (progdefaults.CWspeed > progdefaults.CWfarnsworth))
		tc = 1200.0 / progdefaults.CWfarnsworth;

	if (c == ' ') {
		if (lastKYchar == ' ')
			tc *= 7;
		else
			tc *= 5;
	} else {
		tc *= KYmorse.tx_length(c);
		if (progdefaults.use_KNWDkeying) {
			KNWDcmd[3] = (char)c;
			cmd = KNWDcmd;
		} else {
			KYcmd[3] = (char)c;
			cmd = KYcmd;
		}
		if (progdefaults.fldigi_client_to_flrig) {
			xmlrpc_priority(cmd);
		} else if (progdefaults.chkUSERIGCATis) {
			guard_lock ser_guard( &rigCAT_mutex);
			rigio.WriteBuffer((unsigned char *)cmd.c_str(), cmd.length());
		}
	}
	tc -= progdefaults.CATkeying_compensation / (progdefaults.CWspeed  * 6);
	tc = int(tc);
	MilliSleep(tc);

	lastKYchar = c;
}
