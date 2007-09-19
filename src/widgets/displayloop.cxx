//
//  Display loop thread
//
// Copyright W1HKJ, Dave Freese 2006
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA.
//
// Please report all bugs and problems to "w1hkj@w1hkj.com".

#include <string>

#include "waterfall.h"
#include "threads.h"
#include "main.h"
#include "Config.h"
#include "configuration.h"
#include "fl_digi.h"

using namespace std;

// Display loop is used to separate the calls to the fltk gui updates from the
// modem decoder / encoder.
// fltk call to Fl::lock() disrupts the signal processing loop if made directly
// from the decoder / encoder thread

// loop interval for the display thread (msec)
#define DISPLOOP_TIME 50

Fl_Thread	display_thread;
Fl_Mutex	disp_mutex = PTHREAD_MUTEX_INITIALIZER;

extern void add2rxtext( const char *str, int attr);
extern void dl_clear_StatusMessages();
extern void dl_put_status(const char *msg);
extern void dl_put_Status2(const char *msg);
extern void dl_put_Status1(const char *msg);
extern void dl_display_metric( double metric);
extern void dl_restoreFocus();
extern void dl_put_cwRcvWPM(double wpm);
extern void dl_put_MODEstatus(trx_mode mode);
extern void dl_put_WARNstatus(double val);
extern void dl_put_sec_char();

static int dummy = 0;
bool scope_redraw = false;
bool wf_redraw = false;
bool b_restoreFocus = false;
bool b2_restoreFocus = false;

// rx text
string char2rxtext;
string attr2rxtext;
string tempchar;
string tempattr;

// metric 
double d_metric, d2_metric;
bool b_metric = false, b2_metric = false;

// clear status messages
bool b_clearStatus = false;
bool b2_clearStatus = false;

// status message
string d_status = "";
string d2_status = "";

// Status2 message
string d_status2 = "";
string d2_status2 = "";

// Status1 message
string d_status1 = "";
string d2_status1 = "";

// cw wpm value
double d_wpm = 0, d2_wpm = 0;
bool   b_wpm = 0, b2_wpm = 0;

// display trx mode
trx_mode d_mode, d2_mode;
bool b_mode = false, b2_mode = false;

// WARN status
double d_warn, d2_warn;
bool b_warn = false, b2_warn = false;

// secondary text update
string strSecText = "", d2_strSecText;
bool b_strSecText = false, b2_strSecText = true;

void	*display_loop(void *args)
{
	for(;;) {
// check for waterfall and/or digiscrope redraw
		if (wf_redraw || scope_redraw) {
			Fl::lock();
			if (wf_redraw)
				wf->redraw();
			if (scope_redraw)
				digiscope->redraw();
			Fl::unlock();
			Fl::awake();
		}
		if (b_restoreFocus) {
			b2_restoreFocus = true;
			b_restoreFocus = false;
		}
		fl_lock(&disp_mutex);
// check for Receive text in the holding buffer
			if (!char2rxtext.empty()) {
				tempchar = char2rxtext;
				tempattr = attr2rxtext;
				char2rxtext = "";
				attr2rxtext = "";
			}
// check for display metric value			
			if (b_metric) {
				d2_metric = d_metric;
				b2_metric = true;
				b_metric = false;
			}
// check to clear the status messages
			if (b_clearStatus) {
				b2_clearStatus = true;
				b_clearStatus = false;
			}
// check for status update			
			if (!d_status.empty()) {
				d2_status = d_status;
				d_status = "";
			}
// check for Status1 update
			if (!d_status1.empty()) {
				d2_status1 = d_status1;
				d_status1 = "";
			}
// check for Status2 update			
			if (!d_status2.empty()) {
				d2_status2 = d_status2;
				d_status2 = "";
			}
// check for cw wpm update			
			if (b_wpm) {
				d2_wpm = d_wpm;
				b2_wpm = true;
				b_wpm = false;
			}
			if (b_mode) {
				d2_mode = d_mode;
				b2_mode = true;
				b_mode = false;
			}
			if (b_warn) {
				d2_warn = d_warn;
				b2_warn = true;
				b_warn = false;
			}
			if (b_strSecText) {
				d2_strSecText = strSecText;
				b2_strSecText = true;
				b_strSecText = false;
			}
		fl_unlock(&disp_mutex);

		if (!tempchar.empty()) {
			for (size_t i = 0; i < tempchar.length(); i++)
				ReceiveText->add(tempchar[i], tempattr[i]);
			tempchar = "";
			tempattr = "";
		}
		if (b2_restoreFocus) {
			dl_restoreFocus();
			b2_restoreFocus = false;
		}
		if (b2_metric) {
			dl_display_metric( d2_metric);
			b2_metric = false;
		}
		if (b2_clearStatus) {
			dl_clear_StatusMessages();
			b2_clearStatus = false;
		}
		if (!d2_status.empty()) {
			dl_put_status(d2_status.c_str());
			d2_status.clear();
		}
		if (!d2_status1.empty()) {
			dl_put_Status1(d2_status1.c_str());
			d2_status1.clear();
		}
		if (!d2_status2.empty()) {
			dl_put_Status2(d2_status2.c_str());
			d2_status2.clear();
		}
		if (!b2_wpm) {
			dl_put_cwRcvWPM(d2_wpm);
			b2_wpm = false;
		}
		if (b2_mode) {
			dl_put_MODEstatus(d2_mode);
			b2_mode = false;
		}	
		if (b2_warn) {
			dl_put_WARNstatus(d2_warn);
			b2_warn = false;
		}		
		if (b2_strSecText) {
			dl_put_sec_char();
			b2_strSecText = false;
		}
		MilliSleep(DISPLOOP_TIME);
	}
}

void start_display_loop()
{
	if (fl_create_thread(display_thread, display_loop, &dummy) < 0) {
		std::cout <<  "display pthread_create:" << std::endl; fflush(stdout);
		exit(1);
	} 
}

void add2rxtext( const char *str, int attr)
{
	fl_lock(&disp_mutex);
		char2rxtext += str;
		for (unsigned int i = 0; i < strlen(str); i++)
			attr2rxtext += attr;
	fl_unlock(&disp_mutex);
}

// these functions were moved from fl_digi.cxx

void restoreFocus()
{
	fl_lock(&disp_mutex);
		b_restoreFocus = true;
	fl_unlock(&disp_mutex);
}

void dl_restoreFocus()
{
	Fl::lock();
	Fl::focus(TransmitText);
	TransmitText->cursorON();
//	TransmitText->redraw();
	TransmitText->damage(2);	
	Fl::unlock();
	Fl::awake();
}

void display_metric(double metric)
{
	fl_lock(&disp_mutex);
		d_metric = metric;
		b_metric = true;
	fl_unlock(&disp_mutex);
}

void dl_display_metric( double metric)
{
	Fl::lock();
	pgrsSquelch->value(metric);
	Fl::unlock();
	Fl::awake();
}

void put_cwRcvWPM(double wpm)
{
	fl_lock(&disp_mutex);
		d_wpm = wpm;
		b_wpm = true;
	fl_unlock(&disp_mutex);
}

void dl_put_cwRcvWPM(double wpm)
{
	int U = progdefaults.CWupperlimit;
	int L = progdefaults.CWlowerlimit;
	double dWPM = 100.0*(wpm - L)/(U - L);
	Fl::lock();
	prgsCWrcvWPM->value(dWPM);
	valCWrcvWPM->value((int)wpm);
	Fl::unlock();
	Fl::awake();
}

void put_sec_char( char chr )
{
	fl_lock(&disp_mutex);
		if (chr >= ' ' && chr <= 'z') {
			strSecText.append(1, chr);
			if (strSecText.length() > 60)
				strSecText.erase(0,1);
			b_strSecText = true;
		}
	fl_unlock(&disp_mutex);
}

void dl_put_sec_char()
{
	Fl::lock();
	StatusBar->label(d2_strSecText.c_str());
	Fl::unlock();
	Fl::awake();
}

void put_status(const char *msg)
{
	fl_lock(&disp_mutex);
		d_status = msg;
	fl_unlock(&disp_mutex);
}

void dl_put_status(const char *msg)
{
	static char m[60];
	strncpy(m,msg,59); m[60] = 0;
	Fl::lock();
	StatusBar->label(m);
	StatusBar->redraw_label();
	Fl::unlock();
	Fl::awake();
}

void put_Status1(char *msg)
{
	fl_lock(&disp_mutex);
		d_status1 = msg;
	fl_unlock(&disp_mutex);
}

void dl_put_Status1(const char *msg)
{
	static char m[60];
	strncpy(m,msg,59); m[60] = 0;
	Fl::lock();
	Status1->label(m);
	Status1->redraw_label();
	Fl::unlock();
	Fl::awake();
}

void put_Status2(char *msg)
{
	fl_lock(&disp_mutex);
		d_status2 = msg;
	fl_unlock(&disp_mutex);
}

void dl_put_Status2(const char *msg)
{
	static char m[60];
	strncpy(m,msg,59); m[60] = 0;
	Fl::lock();
	Status2->label(m);
	Status2->redraw_label();
	Fl::unlock();
	Fl::awake();
}

void put_WARNstatus(double val)
{
	fl_lock(&disp_mutex);
		d_warn = val;
		b_warn = true;
	fl_unlock(&disp_mutex);
}

void dl_put_WARNstatus(double val)
{
	Fl::lock();
	if (val < 0.05)
		WARNstatus->color(FL_BLACK);
   if (val > 0.05)
        WARNstatus->color(FL_DARK_GREEN);
    if (val > 0.9)
        WARNstatus->color(FL_YELLOW);
    if (val > 0.98)
        WARNstatus->color(FL_DARK_RED);
	WARNstatus->redraw();
	Fl::unlock();
}


void set_CWwpm()
{
	Fl::lock();
	sldrCWxmtWPM->value(progdefaults.CWspeed);
	Fl::unlock();
}

void clear_StatusMessages()
{
	fl_lock(&disp_mutex);
		b_clearStatus = true;
	fl_unlock(&disp_mutex);
}

void dl_clear_StatusMessages()
{
	Fl::lock();
	StatusBar->label("");
	StatusBar->redraw_label();
	Status2->label("");
	Status2->redraw_label();
	Status1->label("");
	Status1->redraw_label();
	Fl::unlock();
	Fl::awake();
}

void put_MODEstatus(trx_mode mode)
{
	fl_lock(&disp_mutex);
		d_mode = mode;
		b_mode = true;
	fl_unlock(&disp_mutex);
}

void dl_put_MODEstatus(trx_mode mode)
{
	Fl::lock();
	MODEstatus->label(mode_names[mode]);
	MODEstatus->redraw_label();
	Fl::unlock();
	Fl::awake();
}

