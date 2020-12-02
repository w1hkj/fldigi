// ----------------------------------------------------------------------------
// FTkeying.cxx   serial string CW interface to Elecraft transceivers
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

#include "YAESUkeying.h"
#include "configuration.h"
#include "rigio.h"
#include "threads.h"
#include "debug.h"
#include "rigsupport.h"
#include "morse.h"
#include "fl_digi.h"

#include "qrunner.h"

int FTwpm = 0;
bool use_FTkeyer = false;
static cMorse FTmorse;
static char lastFTchar = 0;

void set_FTkeyer()
{
	FTwpm = progdefaults.CWspeed;
	if (FTwpm < 4) FTwpm = 4;
	if (FTwpm > 60) FTwpm = 60;
	progdefaults.CWspeed = FTwpm;
	char cmd[10];
	snprintf(cmd, sizeof(cmd), "KS%03d;", FTwpm);
	if (progdefaults.fldigi_client_to_flrig) {
		xmlrpc_priority(cmd);
	} else {
		guard_lock ser_guard( &rigCAT_mutex);
		rigio.WriteBuffer((unsigned char *)cmd, strlen(cmd));
	}
	MilliSleep(50);
}

void FTkeyer_send_char(int c)
{
	if (FTwpm != progdefaults.CWspeed) {
		set_FTkeyer();
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
		if (lastFTchar == ' ')
			tc *= 7;
		else
			tc *= 5;
	} else {
		tc *= FTmorse.tx_length(c);
		char cmd[20];
		memset(cmd, 0, 20);
		snprintf(cmd, sizeof(cmd), "KM2%c;KY7;", (char)c);
		if (progdefaults.fldigi_client_to_flrig) {
			xmlrpc_priority(cmd);
		} else if (progdefaults.chkUSERIGCATis) {
			guard_lock ser_guard( &rigCAT_mutex);
			rigio.WriteBuffer((unsigned char *)cmd, strlen(cmd));
		}
	}

	if (progdefaults.CATkeying_compensation / (progdefaults.CWspeed  * 6) < tc)
		tc -= progdefaults.CATkeying_compensation / (progdefaults.CWspeed  * 6);

	MilliSleep(tc);

	lastFTchar = c;
}

