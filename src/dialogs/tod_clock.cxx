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

#include "timeops.h"
#include "nanoIO.h"

LOG_FILE_SOURCE(debug::LOG_FD);

using namespace std;

static pthread_t TOD_thread;
static pthread_mutex_t TX_mutex     = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t time_mutex   = PTHREAD_MUTEX_INITIALIZER;

static char ztbuf[20] = "20120602 123000";
static char ltbuf[20] = "20120602 123000";

static struct timeval tx_start_val;
static struct timeval tx_last_val;
static struct timeval now_val;

extern void xmtrcv_cb(Fl_Widget *, void *);

static int tx_timeout = 0;

static int macro_time = -1;

void kill_tx(void *)
{
	wf->xmtrcv->value(0);
	xmtrcv_cb(wf->xmtrcv, 0);
	fl_alert2("TX timeout expired!\nAre you awake?");
}

void service_deadman()
{
	guard_lock txlock(&TX_mutex);
	if (!tx_timeout) return;
	if (--tx_timeout == 0) {
		Fl::awake(kill_tx);
	}
}

void start_deadman()
{
	guard_lock txlock(&TX_mutex);
	tx_timeout = 60 * progdefaults.tx_timeout;
}

void stop_deadman()
{
	guard_lock txlock(&TX_mutex);
	tx_timeout = 0;
}

void start_macro_time()
{
	macro_time = 0;
}

int stop_macro_time()
{
	return macro_time;
}

const timeval tmval(void)
{
	struct timeval t1;
	{
		guard_lock lk(&time_mutex);
		gettimeofday(&t1, NULL);
	}
	return t1;
}

const double zusec(void)
{
	struct timeval t1;
	{
		guard_lock lk(&time_mutex);
		gettimeofday(&t1, NULL);
	}
	double usecs = t1.tv_sec * 1000000L;
	usecs += t1.tv_usec;
	return usecs;
}

const unsigned long zmsec(void)
{
	struct timeval t1;
	{
		guard_lock lk(&time_mutex);
		gettimeofday(&t1, NULL);
	}
	unsigned long msecs = t1.tv_sec * 1000000L;
	msecs += t1.tv_usec;
	msecs /= 1000L;
	return msecs;
}

const char* zdate(void)
{
	return ztbuf;
}

const char* ztime(void)
{
	return ztbuf + 9;
}

const char* ldate(void)
{
	return ltbuf;
}

const char *ltime(void)
{
	return ltbuf + 9;
}

const char* zshowtime(void) {
	static char s[5];
	strncpy(s, &ztbuf[9], 4);
	s[4] = 0;
	return (const char *)s;
}

static char tx_time[20];

static bool TOD_exit = false;
static bool TOD_enabled = false;

static bool   tx_timer_active = false;

void show_tx_timer()
{
	if (!tx_timer) return;
	if (progdefaults.show_tx_timer && tx_timer_active) {
		snprintf(tx_time, sizeof(tx_time),"%02d:%02d", 
			(int)((now_val.tv_sec - tx_start_val.tv_sec)/60),
			(int)((now_val.tv_sec - tx_start_val.tv_sec) % 60 ));
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
	tx_last_val = tx_start_val = now_val;
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
	if (tx_last_val.tv_sec == now_val.tv_sec) return;
	tx_last_val = now_val;
	show_tx_timer();
	service_deadman();
	macro_time++;
}

//void ztimer(void *)
static void show_ztimer()
{
	if (!inpTimeOff1) return;

	update_tx_timer();

	inpTimeOff1->value(zshowtime());
	inpTimeOff2->value(zshowtime());
	inpTimeOff3->value(zshowtime());
	inpTimeOff1->redraw();
	inpTimeOff2->redraw();
	inpTimeOff3->redraw();

}

static void ztimer()
{
	struct tm ztm, ltm;
	time_t t_temp;

	t_temp=(time_t)now_val.tv_sec;
	gmtime_r(&t_temp, &ztm);
	if (!strftime(ztbuf, sizeof(ztbuf), "%Y%m%d %H%M%S", &ztm))
		memset(ztbuf, 0, sizeof(ztbuf));
	else
		ztbuf[8] = '\0';

	localtime_r(&t_temp, &ltm);
	if (!strftime(ltbuf, sizeof(ltbuf), "%Y%m%d %H%M%S", &ltm))
		memset(ltbuf, 0, sizeof(ltbuf));
	else
		ltbuf[8] = '\0';

	REQ(show_ztimer);
}

//======================================================================
// TOD Thread loop
//======================================================================
void *TOD_loop(void *args)
{
	SET_THREAD_ID(TOD_TID);
#define LOOP  250
	int cnt = 0;
	while(1) {

		if (TOD_exit) break;

		if (++cnt == 4) {
			guard_lock tmlock(&time_mutex);
			gettimeofday(&now_val, NULL);
			ztimer();
			cnt = 0;
		}
		REQ(nanoIO_read_pot);
		MilliSleep(LOOP);
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

