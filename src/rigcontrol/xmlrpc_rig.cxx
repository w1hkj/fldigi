// =====================================================================
//
// xmlrpc_rig.cxx
//
// connect to flrig xmlrpc server
//
// Copyright (C) 2007-2009
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

#include <FL/Fl.H>
#include <FL/filename.H>
#include <FL/fl_ask.H>

#include "rigsupport.h"
#include "modem.h"
#include "trx.h"
#include "fl_digi.h"
#include "configuration.h"
#include "main.h"
#include "waterfall.h"
#include "macros.h"
#include "qrunner.h"
#include "debug.h"
#include "status.h"

using namespace XmlRpc;
using namespace std;

int xmlrpc_verbosity = 0;

//======================================================================
// flrig xmlrpc client
//======================================================================

pthread_t *flrig_thread = 0;
pthread_mutex_t mutex_flrig = PTHREAD_MUTEX_INITIALIZER;

void movFreq(Fl_Widget *w, void *d);
void show_freq(void *);
void flrig_get_vfo();
void flrig_get_frequency();
void post_mode(void *);
void flrig_get_mode();
void post_modes(void *);
void flrig_get_modes();
void post_bw(void *);
void flrig_get_bw();
void post_bws(void *);
void flrig_get_bws();
void flrig_get_notch();
void flrig_set_notch(int);

bool flrig_get_xcvr();
void flrig_connection();
void set_ptt();

void connect_to_flrig();

XmlRpcClient *flrig_client = (XmlRpcClient *)0;

bool bws_posted = false;
bool bw_posted = false;
bool mode_posted = false;
bool modes_posted = false;
bool freq_posted = true;

string xcvr_name;
string str_freq;
string mode_result;
XmlRpcValue modes_result;
XmlRpcValue bws_result;
XmlRpcValue bw_result;
XmlRpcValue notch_result;

bool connected_to_flrig = false;
double exec_timeout = 1.0;

//======================================================================

void xmlrpc_rig_set_qsy(long long rfc)
{
	set_flrig_freq(static_cast<long>(rfc));
	wf->rfcarrier(rfc);
	wf->movetocenter();
	show_frequency(rfc);
}

//======================================================================
// set / get pairs
//======================================================================

//----------------------------------------------------------------------
// push to talk
//----------------------------------------------------------------------
bool wait_ptt = false; // wait for transceiver to respond
int  wait_ptt_timeout = 5; // 5 polls and then disable wait
int  ptt_state = 0;

void set_flrig_ptt(int on) {
	if (!connected_to_flrig) return;

	XmlRpcValue val, result;
	val = int(on);

	guard_lock flrig_lock(&mutex_flrig);
	if (flrig_client->execute("rig.set_ptt", val, result, exec_timeout)) {
		wait_ptt = true;
		wait_ptt_timeout = 10;
		ptt_state = on;
	} else {
		wait_ptt = false;
		wait_ptt_timeout = 0;
		LOG_ERROR("%s", "rig.set_vfo failed");
	}
	return;
}

pthread_mutex_t mutex_flrig_ptt = PTHREAD_MUTEX_INITIALIZER;
void xmlrpc_rig_show_ptt(void *data)
{
	guard_lock flrig_lock(&mutex_flrig_ptt);
	int on = reinterpret_cast<long>(data);
	if (wf) wf->xmtrcv->value(on);
}

void flrig_get_ptt()
{
	guard_lock flrig_lock(&mutex_flrig);
	XmlRpcValue result;
	if (flrig_client->execute("rig.get_ptt", XmlRpcValue(), result, exec_timeout) ) {
		int val = int(result);
		if (!wait_ptt && (val != ptt_state)) {
			ptt_state = val;
			guard_lock flrig_lock(&mutex_flrig_ptt);
			Fl::awake(xmlrpc_rig_show_ptt, reinterpret_cast<void*>(val) );
		} else if (wait_ptt && (val == ptt_state)) {
			wait_ptt = false;
			wait_ptt_timeout = 0;
		} else if (wait_ptt_timeout == 0) {
			wait_ptt = false;
		} else if (wait_ptt_timeout) {
			--wait_ptt_timeout;
		}
	} else {
		connected_to_flrig = false;
		wait_ptt = false;
		wait_ptt_timeout = 5;
	}
}

//----------------------------------------------------------------------
// transceiver radio frequency
//----------------------------------------------------------------------

bool wait_freq = false; // wait for transceiver to respond
int  wait_freq_timeout = 5; // 5 polls and then disable wait
long int  xcvr_freq = 0;

void set_flrig_freq(long int fr)
{
	if (!connected_to_flrig) return;

	guard_lock flrig_lock(&mutex_flrig);

	XmlRpcValue val, result;
	val = double(fr);
	if (!flrig_client->execute("rig.set_vfo", val, result, exec_timeout)) {
		LOG_ERROR("%s", "rig.set_vfo failed");
		wait_freq = false;
		wait_freq_timeout = 0;
	} else {
		wait_freq = true;
		wait_freq_timeout = 5;
		xcvr_freq = fr;
	}
}

pthread_mutex_t mutex_flrig_freq = PTHREAD_MUTEX_INITIALIZER;
void xmlrpc_rig_show_freq(void * fr)
{
	guard_lock flrig_lock(&mutex_flrig_freq);
	if (!wf) return;
	long freq = reinterpret_cast<long>(fr);
	wf->rfcarrier(freq);
	wf->movetocenter();
	show_frequency(freq);
}

void flrig_get_frequency()
{
	guard_lock flrig_lock(&mutex_flrig);
	XmlRpcValue result;
	if (!freq_posted) return;
	if (flrig_client->execute("rig.get_vfo", XmlRpcValue(), result, exec_timeout) ) {
		str_freq = string(result);
		int fr = atoi(str_freq.c_str());

		if (!wait_freq && (fr != xcvr_freq)) {
			xcvr_freq = fr;
			guard_lock flrig_lock(&mutex_flrig_freq);
			Fl::awake(xmlrpc_rig_show_freq, reinterpret_cast<void*>(fr));
		} else if (wait_freq && (fr == xcvr_freq)) {
			wait_freq = false;
			wait_freq_timeout = 0;
		} else if (wait_freq_timeout == 0) {
			wait_freq = false;
		} else if (wait_freq_timeout)
			--wait_freq_timeout;
	} else {
		connected_to_flrig = false;
		wait_freq = false;
		wait_freq_timeout = 5;
	}
}

//----------------------------------------------------------------------
// transceiver set / get mode
// transceiver get modes (mode table)
//----------------------------------------------------------------------

bool wait_mode = false; // wait for transceiver to respond
int  wait_mode_timeout = 5; // 5 polls and then disable wait
string posted_mode = "";

void set_flrig_mode(const char *md)
{
	if (!connected_to_flrig) return;

	XmlRpcValue val, result;
	val = string(md);

	guard_lock flrig_lock(&mutex_flrig);
	if (!flrig_client->execute("rig.set_mode", val, result, exec_timeout)) {
		LOG_ERROR("%s", "rig.set_mode failed");
		wait_mode = false;
		wait_mode_timeout = 5;
	} else {
		posted_mode = md;
		wait_mode = true;
		wait_mode_timeout = 5;
	}
}

pthread_mutex_t mutex_flrig_mode = PTHREAD_MUTEX_INITIALIZER;
void xmlrpc_rig_post_mode(void *data)
{
	guard_lock flrig_lock(&mutex_flrig_mode);
	if (!qso_opMODE) return;
	string *s = reinterpret_cast<string *>(data);
	qso_opMODE->value(s->c_str());
	bws_posted = false;
}

void flrig_get_mode()
{
	guard_lock flrig_lock(&mutex_flrig);
	XmlRpcValue res;
	if (flrig_client->execute("rig.get_mode", XmlRpcValue(), res, exec_timeout) ) {
		static string md;
		md = string(res);
		bool posted = (md == posted_mode);
		if (!wait_mode && !posted) {
			posted_mode = md;
			guard_lock flrig_lock(&mutex_flrig_mode);
			Fl::awake(xmlrpc_rig_post_mode, reinterpret_cast<void*>(&md));
		} else if (wait_mode && posted) {
			wait_mode = false;
			wait_mode_timeout = 0;
		} else if (wait_mode_timeout == 0) {
			wait_mode = false;
		} else if (wait_mode_timeout)
			--wait_mode_timeout;
	} else {
		connected_to_flrig = false;
		wait_mode = false;
		wait_freq_timeout = 0;
	}
}

pthread_mutex_t mutex_flrig_modes = PTHREAD_MUTEX_INITIALIZER;
void xmlrpc_rig_post_modes(void *)
{
	guard_lock flrig_lock(&mutex_flrig_modes);
	if (!qso_opMODE) return;

	int nargs = modes_result.size();

	qso_opMODE->clear();

	if (nargs == 0) {
		qso_opMODE->add("");
		qso_opMODE->index(0);
		qso_opMODE->deactivate();
		return;
	}

	for (int i = 0; i < nargs; i++)
		qso_opMODE->add(string(modes_result[i]).c_str());

	qso_opMODE->index(0);
	qso_opMODE->activate();

	modes_posted = true;
}

void flrig_get_modes()
{
	guard_lock flrig_lock(&mutex_flrig);
	if (flrig_client->execute("rig.get_modes", XmlRpcValue(), modes_result, exec_timeout) ) {
		guard_lock flrig_lock(&mutex_flrig_modes);
		Fl::awake(xmlrpc_rig_post_modes);
	}
}

//----------------------------------------------------------------------
// transceiver get / set bandwidth
// transceiver get bandwidth table
//----------------------------------------------------------------------
bool wait_bw = false; // wait for transceiver to respond
int  wait_bw_timeout = 5; // 5 polls and then disable wait
string  posted_bw = "";

void set_flrig_bw(int bw1, int bw2)
{
	if (!connected_to_flrig) return;

	XmlRpcValue val, result;
	val = 256*bw2 + bw1;

	guard_lock flrig_lock(&mutex_flrig);
	if (!flrig_client->execute("rig.set_bw", val, result, exec_timeout)) {
		LOG_ERROR("%s", "rig.set_bw failed");
		wait_bw = false;
		wait_bw_timeout = 0;
	} else {
		posted_bw = qso_opBW->value();
		wait_bw = true;
		wait_bw_timeout = 5;
	}
}

pthread_mutex_t mutex_flrig_bw = PTHREAD_MUTEX_INITIALIZER;
void xmlrpc_rig_post_bw(void *data)
{
	guard_lock flrig_lock(&mutex_flrig_bw);
	if (!qso_opBW) return;

	string *s = reinterpret_cast<string *>(data);
	size_t p = s->find("|");
	if (p != string::npos) s->erase(p);

	qso_opBW->value(s->c_str());
	qso_opBW->redraw();
}

void flrig_get_bw()
{
	guard_lock flrig_lock(&mutex_flrig);
	XmlRpcValue res;
	if (flrig_client->execute("rig.get_bw", XmlRpcValue(), res, exec_timeout) ) {
		static string s1;
		static string s2;

		s1 = string(res[0]);
		s2 = string(res[1]);

		bool posted = ((s1 == posted_bw));
		if (!wait_bw && !posted) {
			posted_bw = s1;
			guard_lock flrig_lock(&mutex_flrig_bw);
			Fl::awake(xmlrpc_rig_post_bw, reinterpret_cast<void*>(&s1));
		} else if (wait_bw && !posted) {
			wait_bw = false;
			wait_bw_timeout = 0;
		} else if (wait_bw_timeout == 0) {
			wait_bw = false;
		} else if (wait_bw_timeout)
			--wait_bw_timeout;
	} else {
		connected_to_flrig = false;
		wait_bw = false;
		wait_bw_timeout = 0;
	}
}

pthread_mutex_t mutex_flrig_bws = PTHREAD_MUTEX_INITIALIZER;
void xmlrpc_rig_post_bws(void *)
{
	guard_lock flrig_lock(&mutex_flrig_bws);
	if (!qso_opBW) return;

	int nargs;

	try {
		nargs = bws_result[0].size();

		qso_opBW->clear();

		if (nargs == 0) {
			qso_opBW->add("");
			qso_opBW->index(0);
			qso_opBW->deactivate();
			return;
		}

		for (int i = 1; i < nargs; i++)
			qso_opBW->add(string(bws_result[0][i]).c_str());

		qso_opBW->index(0);
		qso_opBW->activate();

		qso_opBW->tooltip("xcvr bandwidth");

	} catch (XmlRpcException err) {
		;
	}
/*
	string label;
	size_t p;
// add later
	try {
		nargs = bws_result[1].size();
		if (nargs > 1) {
			label = string(bws_result[1][0]);
			if ( (p = label.find("|")) != string::npos)
				label.erase(0, p + 1);
			bw2Label->value(label.c_str());
			for (int i = 1; i < nargs; i++) {
				bw2->add(string(bws_result[1][i]).c_str());
			}
			bw2Label->redraw();
			bw2->activate();
		} else
			bw2->add("");
	} catch (XmlRpcException err) {
		bw2->deactivate();
	}
	bw1->redraw_label();
	bw1->redraw();
	bw2->redraw_label();
	bw2->redraw();
*/

	bws_posted = true;
}

void flrig_get_bws()
{
	if (bws_posted) return;
	XmlRpcValue result;
	if (flrig_client->execute("rig.get_bws", XmlRpcValue(), bws_result, exec_timeout) ) {
		bws_posted = false;
		wait_bw = true;
		wait_bw_timeout = 5;
		posted_bw.clear();
		guard_lock flrig_lock(&mutex_flrig_bws);
		Fl::awake(xmlrpc_rig_post_bws);
	}
}

//----------------------------------------------------------------------
// transceiver get / set vfo A / B
//----------------------------------------------------------------------

void set_flrig_ab(int n)
{
//	if (n) btn_vfoA->clear();
//	else   btn_vfoB->clear();

//	if (!connected_to_flrig) return;

//	XmlRpcValue val = string(n == 0 ? "A" : "B");
//	XmlRpcValue result;

//	guard_lock flrig_lock(&mutex_flrig);
//	if (!flrig_client->execute("rig.set_AB", val, result, exec_timeout))
//		printf("rig.set_mode failed\n");
//	else {
//		skip = 2;
//	}
//	return;
}

void show_A(void *)
{
//	btn_vfoA->set();
//	btn_vfoB->clear();
}

void show_B(void *)
{
//	btn_vfoA->clear();
//	btn_vfoB->set();
}

void flrig_get_vfo()
{
	guard_lock flrig_lock(&mutex_flrig);
	XmlRpcValue result;
	if (flrig_client->execute("rig.get_AB", XmlRpcValue(), result, exec_timeout) ) {
//		string str_vfo = string(result[0]);
//		if (str_vfo == "A" && !btn_vfoA->value()) REQ(FLRIG_show_A);
//		else if (str_vfo == "B" && !btn_vfoB->value()) REQ(FLRIG_show_B);
	} else {
		connected_to_flrig = false;
	}
}

//==============================================================================
// transceiver set / get notch
//==============================================================================
bool wait_notch = false; // wait for transceiver to respond
int  wait_notch_timeout = 5; // 5 polls and then disable wait
int  xcvr_notch = 0;

void set_flrig_notch()
{
	if (!connected_to_flrig) return;

	guard_lock flrig_lock(&mutex_flrig);

	XmlRpcValue val, result;
	val = (double)(notch_frequency);
	if (flrig_client->execute("rig.set_notch", val, result, exec_timeout)) {
		wait_notch = true;
		wait_notch_timeout = 5;
		xcvr_notch = notch_frequency;
	} else {
		LOG_ERROR("%s", "rig.set_notch failed");
		wait_notch = 0;
		wait_notch_timeout = 0;
		xcvr_notch = 0;
	}
}

void flrig_get_notch()
{
	guard_lock flrig_lock(&mutex_flrig);
	if (flrig_client->execute("rig.get_notch", XmlRpcValue(), notch_result, exec_timeout) ) {
		int nu_notch = (int)(notch_result);
		if (nu_notch != notch_frequency) {
			notch_frequency = nu_notch;
		}

		bool posted = (nu_notch == xcvr_notch);

		if (!wait_notch && !posted) {
			xcvr_notch = nu_notch;
			notch_frequency = nu_notch;
		} else if (wait_notch && posted) {
			wait_notch = false;
			wait_notch_timeout = 0;
		} else if (wait_notch_timeout == 0) {
			wait_notch = false;
		} else if (wait_notch_timeout)
			--wait_notch_timeout;
	} else {
		wait_notch = false;
		wait_notch_timeout = 0;
	}
}

//==============================================================================
// transceiver get smeter
// transceiver get power meter
//==============================================================================

pthread_mutex_t mutex_flrig_smeter = PTHREAD_MUTEX_INITIALIZER;
static void xmlrpc_rig_set_smeter(void *data)
{
	guard_lock flrig_lock(&mutex_flrig_smeter);
	if (!smeter && !pwrmeter) return;

	if (smeter && progStatus.meters) {
		if (!smeter->visible()) {
			pwrmeter->hide();
			smeter->show();
		}
		int val = reinterpret_cast<long>(data);
		smeter->value(val);
	}
}

void flrig_get_smeter()
{
	XmlRpcValue val, result;
	if (flrig_client->execute("rig.get_smeter", val, result, exec_timeout)) {
		int val = (int)(result);
		guard_lock flrig_lock(&mutex_flrig_smeter);
		Fl::awake(xmlrpc_rig_set_smeter, reinterpret_cast<void*>(val));
	}
}

pthread_mutex_t mutex_flrig_pwrmeter = PTHREAD_MUTEX_INITIALIZER;
static void xmlrpc_rig_set_pwrmeter(void *data)
{
	guard_lock flrig_lock(&mutex_flrig_pwrmeter);
	if (!smeter && !pwrmeter) return;

	if (pwrmeter && progStatus.meters) {
		if (!pwrmeter->visible()) {
			smeter->hide();
			pwrmeter->show();
		}
		int val = reinterpret_cast<long>(data);
		pwrmeter->value(val);
	}
}

void flrig_get_pwrmeter()
{
	XmlRpcValue val, result;
	if (flrig_client->execute("rig.get_pwrmeter", val, result, exec_timeout)) {
		int val = (int)(result);
		guard_lock flrig_lock(&mutex_flrig_pwrmeter);
		Fl::awake(xmlrpc_rig_set_pwrmeter, reinterpret_cast<void*>(val));
	}
}

//==============================================================================
// transceiver get name
//==============================================================================

pthread_mutex_t mutex_flrig_xcvr_name = PTHREAD_MUTEX_INITIALIZER;
void xmlrpc_rig_show_xcvr_name(void *)
{
	guard_lock flrig_lock(&mutex_flrig_xcvr_name);
	windowTitle = xcvr_name.c_str();
	if (main_window_title.find(windowTitle) == string::npos)
		setTitle();
}

bool flrig_get_xcvr()
{
	guard_lock flrig_lock(&mutex_flrig);
	XmlRpcValue result;
	try {
		if (flrig_client->execute("rig.get_xcvr", XmlRpcValue(), result, exec_timeout) ) {
			string nuxcvr = string(result);
			if (nuxcvr != xcvr_name) {
				xcvr_name = nuxcvr;
				guard_lock flrig_lock(&mutex_flrig_xcvr_name);
				Fl::awake(xmlrpc_rig_show_xcvr_name);
				modes_posted = false;
				bws_posted = false;
			}
			return true;
		} else {
			connected_to_flrig = false;
		}
	} catch (XmlRpcException *err) {
		connected_to_flrig = false;
	}
	return false;
}

//======================================================================
// xmlrpc read polling thread
//======================================================================
bool run_flrig_thread = true;
int poll_interval = 100; // milliseconds

//----------------------------------------------------------------------
// Set QSY to true if xmlrpc client connection is OK
//----------------------------------------------------------------------

void flrig_setQSY(void *)
{
	if (!wf) return;
	wf->setQSY(true);
}

void flrig_connection()
{
	guard_lock flrig_lock(&mutex_flrig);
	XmlRpcValue noArgs, result;

	// MacOSX mutex dead lock. Timeout in order to release
	// mutex lock for program exit to occur.

	try {
		if (flrig_client->execute("system.listMethods", noArgs, result, exec_timeout)) {
			int nargs = result.size();
			string method_str = "\nMethods:\n";
			for (int i = 0; i < nargs; i++)
				method_str.append("    ").append(result[i]).append("\n");
			LOG_INFO("%s", method_str.c_str());
			connected_to_flrig = true;
			poll_interval = 100;
			Fl::awake(flrig_setQSY);
		} else {
			connected_to_flrig = false;
			poll_interval = 500;
		}
	} catch (...) {}
}

void connect_to_flrig()
{
	XmlRpc::setVerbosity(xmlrpc_verbosity);
	if (flrig_client) {
		delete flrig_client;
		flrig_client = (XmlRpcClient *)0;
	}
	try {
		flrig_client = new XmlRpcClient(
				"localhost",
				atol("12345"));
		flrig_connection();
	} catch (...) {
			LOG_ERROR("Cannot connect to %s, %s",
						"localhost",
						"12345");
	}
}

void * flrig_thread_loop(void *d)
{
	for(;;) {
		MilliSleep( poll_interval );

		if (!run_flrig_thread) break;

		if (!flrig_client)
			connect_to_flrig();
		if (!connected_to_flrig) flrig_connection();
		else if (flrig_get_xcvr()) {
			flrig_get_frequency();
			if (!modes_posted) flrig_get_modes();
			if (modes_posted)  flrig_get_mode();
			if (!bws_posted)   flrig_get_bws();
			if (bws_posted)    flrig_get_bw();
///			flrig_get_vfo();
			flrig_get_notch();
			flrig_get_ptt();
			if (trx_state == STATE_RX) flrig_get_smeter();
			else  flrig_get_pwrmeter();
		}
	}
	return NULL;
}

void FLRIG_start_flrig_thread()
{
	flrig_thread = new pthread_t;
	poll_interval = 500;
	if (pthread_create(flrig_thread, NULL, flrig_thread_loop, NULL)) {
		LOG_ERROR("%s", "flrig_thread create");
		exit(EXIT_FAILURE);
	}
}

void stop_flrig_thread()
{
	pthread_mutex_lock(&mutex_flrig);
		run_flrig_thread = false;
	pthread_mutex_unlock(&mutex_flrig);
	pthread_join(*flrig_thread, NULL);
}

