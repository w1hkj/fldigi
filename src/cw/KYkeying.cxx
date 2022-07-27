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

#if !HAVE_CLOCK_GETTIME
#  ifdef __APPLE__
#    include <mach/mach_time.h>
#    define CLOCK_REALTIME 0
#    define CLOCK_MONOTONIC 6
#  endif
#  if TIME_WITH_SYS_TIME
#    include <sys/time.h>
#  endif
#endif


int KYwpm = 0;
bool use_KYkeyer = false;
static std::string KYcmd = "KY  ;";
static std::string KNWDcmd = "KY                         ;";
//                            0123456789012345678901234567
//                            0         1         2
static std::string cmd;
static cMorse *KYmorse = 0;
static char lastKYchar = 0;

double KY_now()
{
	static struct timespec tp;

#if HAVE_CLOCK_GETTIME
	clock_gettime(CLOCK_MONOTONIC, &tp); 
#elif defined(__WIN32__)
	DWORD msec = GetTickCount();
	tp.tv_sec = msec / 1000;
	tp.tv_nsec = (msec % 1000) * 1000000;
#elif defined(__APPLE__)
	static mach_timebase_info_data_t info = { 0, 0 };
	if (unlikely(info.denom == 0))
		mach_timebase_info(&info);
	uint64_t t = mach_absolute_time() * info.numer / info.denom;
	tp.tv_sec = t / 1000000000;
	tp.tv_nsec = t % 1000000000;
#endif

	return 1.0 * tp.tv_sec + tp.tv_nsec * 1e-9;
}

void KY_sleep(double secs)
{
	static struct timespec tv = { 0, 1000000L};
	static double end1 = 0;
	static double end2 = 0;
	static double t1 = 0;
	static double t2 = 0;
	static double t3 = 0;
	int loop1 = 0;
	int loop2 = 0;
	int n1 = secs*1e3;
#ifdef __WIN32__
	timeBeginPeriod(1);
#endif
	t1 = KY_now();
	end2 = t1 + secs - 0.0001;
	end1 = end2 - 0.005;

	t2 = KY_now();
	while (t2 < end1 && (++loop1 < n1)) {
		nanosleep(&tv, NULL);
		t2 = KY_now();
	}
	t3 = t2;
	while (t3 <= end2) {
		loop2++;
		t3 = KY_now();
	}

#ifdef __WIN32__
	timeEndPeriod(1);
#endif

}

void set_KYkeyer()
{
	KYwpm = progdefaults.CWspeed;
	if (KYwpm < 8) KYwpm = 8;
	if (KYwpm > 100) KYwpm = 100;
	progdefaults.CWspeed = KYwpm;
	char cmd[10];
	snprintf(cmd, sizeof(cmd), "KS%03d;", KYwpm);
	if (progdefaults.fldigi_client_to_flrig) {
		xmlrpc_priority(cmd);
	} else {
		guard_lock ser_guard( &rigCAT_mutex);
		rigio.WriteBuffer((unsigned char *)cmd, strlen(cmd));
	}
	KY_sleep(0.050);
}

void KYkeyer_send_char(int c)
{
	if (KYmorse == 0) KYmorse = new cMorse;

	if (KYwpm != progdefaults.CWspeed) {
		set_KYkeyer();
	}

	if (c == GET_TX_CHAR_NODATA || c == 0x0d) {
		KY_sleep(0.050);
		return;
	}

	c = toupper(c);
	if (c < ' ') c = ' ';
	if (c > 'Z') c = ' ';

	float tc = 1.2 / progdefaults.CWspeed;
	if (progdefaults.CWusefarnsworth && (progdefaults.CWspeed > progdefaults.CWfarnsworth))
		tc = 1.2 / progdefaults.CWfarnsworth;

	if (c == ' ') {
		if (lastKYchar == ' ')
			tc *= 7;
		else
			tc *= 5;
	} else {
		tc *= KYmorse->tx_length(c);
		if (progdefaults.use_KNWDkeying) {
			cmd = KNWDcmd;
			cmd[3] = (char)c;
		} else {
			cmd = KYcmd;
			cmd[3] = (char)c;
		}
		if (progdefaults.fldigi_client_to_flrig) {
			xmlrpc_priority(cmd);
		} else if (progdefaults.chkUSERIGCATis) {
			guard_lock ser_guard( &rigCAT_mutex);
			rigio.WriteBuffer((unsigned char *)cmd.c_str(), cmd.length());
		}
	}
	tc -= (progdefaults.CATkeying_compensation / (progdefaults.CWspeed  * 6)) * 1e-3;

	KY_sleep(tc);

	lastKYchar = c;
}
