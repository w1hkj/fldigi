// =====================================================================
//
// TOD_clock.cxx
//
// interface to tcpip application fdserver.tcl
//   fdserver is a multiple client tcpip server
//
// Copyright (C) 2016
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
// =====================================================================

#include <iostream>
#include <cmath>
#include <cstring>
#include <vector>
#include <list>
#include <stdlib.h>

#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Text_Buffer.H>

#include "fl_digi.h"
#include "rigsupport.h"
#include "modem.h"
#include "trx.h"
#include "configuration.h"
#include "main.h"
#include "waterfall.h"
#include "macros.h"
#include "qrunner.h"
#include "debug.h"
#include "status.h"
#include "icons.h"

#include "logsupport.h"
#include "fd_logger.h"
#include "fd_view.h"

#include "confdialog.h"

LOG_FILE_SOURCE(debug::LOG_FD);

using namespace std;

static pthread_t TOD_thread;
//static pthread_mutex_t TOD_mutex     = PTHREAD_MUTEX_INITIALIZER;

static char ztbuf[20] = "20120602 123000";

const char* zdate(void) { return ztbuf; }
const char* ztime(void) { return ztbuf + 9; }
const char* zshowtime(void) {
	static char s[5];
	strncpy(s, &ztbuf[9], 4);
	s[4] = 0;
	return (const char *)s;
}

static char tx_time[6];
static int tx_mins;
static int tx_secs;

static bool TOD_exit = false;
static bool TOD_enabled = false;

static bool   tx_timer_active = false;

void show_tx_timer()
{
	if (!tx_timer) return;
	if (progdefaults.show_tx_timer && tx_timer_active) {
		snprintf(tx_time, sizeof(tx_time),"%02d:%02d", tx_mins, tx_secs);
		tx_timer->color(FL_DARK_RED);
		tx_timer->labelcolor(FL_YELLOW);
		tx_timer->label(tx_time);
		tx_timer->redraw_label();
		tx_timer->redraw();
	} else {
		tx_timer->color(FL_BACKGROUND_COLOR);
		tx_timer->labelcolor(FL_BACKGROUND_COLOR);
		tx_timer->redraw_label();
		tx_timer->redraw();
	}
}

void start_tx_timer()
{
	tx_mins = 0; tx_secs = 0;
	tx_timer_active = true;
	REQ(show_tx_timer);
}

void stop_tx_timer()
{
	if (!tx_timer) return;
	tx_timer_active = false;
}

void update_tx_timer()
{
	tx_secs++;
	if (tx_secs == 60) {
		tx_secs = 0;
		tx_mins++;
	}
	show_tx_timer();
}

void ztimer(void *)
{
	struct tm tm;
	time_t t_temp;
	struct timeval tv;
	gettimeofday(&tv, NULL);

	t_temp=(time_t)tv.tv_sec;
	gmtime_r(&t_temp, &tm);
	if (!strftime(ztbuf, sizeof(ztbuf), "%Y%m%d %H%M%S", &tm))
		memset(ztbuf, 0, sizeof(ztbuf));
	else
		ztbuf[8] = '\0';

	if (!inpTimeOff1) return;

	update_tx_timer();

	inpTimeOff1->value(zshowtime());
	inpTimeOff2->value(zshowtime());
	inpTimeOff3->value(zshowtime());
	inpTimeOff1->redraw();
	inpTimeOff2->redraw();
	inpTimeOff3->redraw();
}

//======================================================================
// TOD Thread loop
//======================================================================
static bool first_call = true;

void *TOD_loop(void *args)
{
	SET_THREAD_ID(TOD_TID);

	int count = 20;
	while(1) {

		if (TOD_exit) break;

		if (first_call) {
			struct timeval tv;
			gettimeofday(&tv, NULL);
			double st = 1000.0 - tv.tv_usec / 1e3;
			MilliSleep(st);
			first_call = false;
		} else
			MilliSleep(50);
			if (--count == 0) {
				Fl::awake(ztimer);
				count = 20;
			}
		}

		
// exit the TOD thread
	SET_THREAD_CANCEL();
	return NULL;
}

//======================================================================
//
//======================================================================
void TOD_init(void)
{
	TOD_exit = false;

	if (pthread_create(&TOD_thread, NULL, TOD_loop, NULL) < 0) {
		LOG_ERROR("%s", "pthread_create failed");
		return;
	}

	LOG_INFO("%s", "Time Of Day thread started");

	TOD_enabled = true;
}

//======================================================================
//
//======================================================================
void TOD_close(void)
{
	if (!TOD_enabled) return;

	TOD_exit = true;
	pthread_join(TOD_thread, NULL);
	TOD_enabled = false;

	LOG_INFO("%s", "Time Of Day thread terminated. ");

}

