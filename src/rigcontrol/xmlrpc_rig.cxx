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
#include "icons.h"

LOG_FILE_SOURCE(debug::debug::LOG_XMLRPC_RIG);

using namespace XmlRpc;
using namespace std;

static int xmlrpc_verbosity = 0;

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

//----------------------------------------------------------------------
// declared as extern in rigsupport.h
//----------------------------------------------------------------------
bool connected_to_flrig = false;
//----------------------------------------------------------------------

static bool bws_posted = false;
static bool modes_posted = false;
static bool freq_posted = true;

static string xcvr_name;
static string str_freq;
static string mode_result;
static XmlRpcValue modes_result;
static XmlRpcValue bws_result;
static XmlRpcValue bw_result;
static XmlRpcValue notch_result;

static double timeout = 2.0;
//======================================================================

void xmlrpc_rig_set_qsy(long long rfc)
{
	set_flrig_freq(static_cast<long>(rfc));
	wf->rfcarrier(rfc);
	wf->movetocenter();
	show_frequency(rfc);
	LOG_INFO("set qsy: %d", (int)rfc);
}

//======================================================================
// set / get pairs
//======================================================================

//----------------------------------------------------------------------
// To prevent a FLTK library thread deadlock on MacOSX
//----------------------------------------------------------------------
static void ptt_on_off_failure(void * ptt_flag)
{
    int flag = *((int *) ptt_flag);
    fl_alert2("fldigi/flrig PTT %s failure", flag ? "ON" : "OFF");
}

//----------------------------------------------------------------------
// push to talk
//----------------------------------------------------------------------
static bool wait_ptt = false; // wait for transceiver to respond
static int  wait_ptt_timeout = 5; // 5 polls and then disable wait
static int  ptt_state = 0;

static int  new_ptt = -1;
static int  last_new_ptt = -1;

void exec_flrig_ptt() {
	if (!connected_to_flrig) {
		new_ptt = -1;
		return;
	}

	XmlRpcValue val, result;
	int resp, res;

	try {
// PTT on/off is critical 5 attempts with 10 verify reads per attempt
		for (int i = 0; i < 5; i++) {
			res = flrig_client->execute("rig.set_ptt", new_ptt, result, timeout);
			if (res) {
				for (int j = 0; j < 10; j++) {
					MilliSleep(20);
					res = flrig_client->execute("rig.get_ptt", XmlRpcValue(), result, 10);
					if (res) {
						resp = (int)result;
						if (resp == new_ptt) {
							wait_ptt = true;
							wait_ptt_timeout = 10;
							ptt_state = new_ptt;
							LOG_INFO("ptt %s in %d msec",
								ptt_state ? "ON" : "OFF",
								i*50 + (j + 1)*20);
							new_ptt = -1;
							return;
						}
					}
				}
			}
		}
	} catch (...) {}

	wait_ptt = false;
	wait_ptt_timeout = 0;
	LOG_ERROR("%s", "rig.set_ptt failed (3)");
    // FLTK thread dead lock on MacOSX. Call in main thread.
    // fl_alert2("fldigi/flrig PTT %s failure", new_ptt ? "ON" : "OFF");
    last_new_ptt = new_ptt;
    REQ(ptt_on_off_failure, (void *) &last_new_ptt);
	new_ptt = -1;
	return;
}

void set_flrig_ptt(int on) {
	guard_lock flrig_lock(&mutex_flrig);
	new_ptt = on;
}

pthread_mutex_t mutex_flrig_ptt = PTHREAD_MUTEX_INITIALIZER;

void xmlrpc_rig_show_ptt(void *data)
{
	guard_lock flrig_lock(&mutex_flrig_ptt);
	int on = reinterpret_cast<long>(data);
	if (wf) {
		wf->xmtrcv->value(on);
		wf->xmtrcv->do_callback();
	}
}

void flrig_get_ptt()
{
	guard_lock flrig_lock(&mutex_flrig);
	try {
		XmlRpcValue result;
		if (flrig_client->execute("rig.get_ptt", XmlRpcValue(), result, timeout) ) {
			int val = int(result);
			if (!wait_ptt && (val != ptt_state)) {
				ptt_state = val;
				guard_lock flrig_lock(&mutex_flrig_ptt);
				Fl::awake(xmlrpc_rig_show_ptt, reinterpret_cast<void*>(val) );
				LOG_INFO("get_ptt: %s", ptt_state ? "ON" : "OFF");
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
			LOG_ERROR("%s", "get_ptt failed!");
		}
	} catch (...) {}
}

//----------------------------------------------------------------------
// transceiver radio frequency
//----------------------------------------------------------------------

static bool wait_freq = false; // wait for transceiver to respond
static int  wait_freq_timeout = 5; // 5 polls and then disable wait
static long int  xcvr_freq = 0;

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

void set_flrig_freq(long int fr)
{
	if (!connected_to_flrig) return;

	guard_lock flrig_lock(&mutex_flrig);

	XmlRpcValue val, result;
	try {
		val = double(fr);
		if (!flrig_client->execute("rig.set_vfo", val, result, timeout)) {
			LOG_ERROR("%s", "rig.set_vfo failed");
			wait_freq = false;
			wait_freq_timeout = 0;
		} else {
			wait_freq = true;
			wait_freq_timeout = 5;
			xcvr_freq = fr;
			Fl::awake(xmlrpc_rig_show_freq, reinterpret_cast<void*>(fr));
			LOG_INFO("set freq: %d", (int)fr);
		}
	} catch (...) {}
}

void flrig_get_frequency()
{
	guard_lock flrig_lock(&mutex_flrig);
	XmlRpcValue result;
	try {
		if (!freq_posted) return;
		if (flrig_client->execute("rig.get_vfo", XmlRpcValue(), result, timeout) ) {
			str_freq = string(result);
			int fr = atoi(str_freq.c_str());

			if (!wait_freq && (fr != xcvr_freq)) {
				xcvr_freq = fr;
				guard_lock flrig_lock(&mutex_flrig_freq);
				Fl::awake(xmlrpc_rig_show_freq, reinterpret_cast<void*>(fr));
				LOG_INFO("get freq: %d", fr);
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
			LOG_ERROR("%s", "get freq failed");
		}
	} catch (...) {}
}

//----------------------------------------------------------------------
// transceiver set / get mode
// transceiver get modes (mode table)
//----------------------------------------------------------------------

static bool wait_mode = false; // wait for transceiver to respond
static int  wait_mode_timeout = 5; // 5 polls and then disable wait
static string posted_mode = "";

static bool wait_bw = false; // wait for transceiver to respond
static int  wait_bw_timeout = 5; // 5 polls and then disable wait
static bool need_sideband = false;
static string  posted_bw = "";
static string  posted_bw1 = "";
static string  posted_bw2 = "";

void set_flrig_mode(const char *md)
{
	if (!connected_to_flrig) return;

	XmlRpcValue val, result;
	try {
		val = string(md);

		guard_lock flrig_lock(&mutex_flrig);
		if (!flrig_client->execute("rig.set_mode", val, result, timeout)) {
			LOG_ERROR("%s", "rig.set_mode failed");
			wait_mode = false;
			wait_mode_timeout = 5;
		} else {
			posted_mode = md;
			need_sideband = true;
			bws_posted = false;
			wait_mode = true;
			wait_mode_timeout = 5;
			LOG_INFO("set mode: %s", md);
		}
	} catch (...) {}
}

pthread_mutex_t mutex_flrig_mode = PTHREAD_MUTEX_INITIALIZER;
static bool xml_USB = true;

bool xmlrpc_USB()
{
	return xml_USB;
}

void xmlrpc_rig_post_mode(void *data)
{
	guard_lock flrig_lock(&mutex_flrig_mode);
	if (!qso_opMODE) return;
	string *s = reinterpret_cast<string *>(data);
	qso_opMODE->value(s->c_str());
	bws_posted = false;
	need_sideband = false;
}

void flrig_get_mode()
{
	guard_lock flrig_lock(&mutex_flrig);
	XmlRpcValue res;
	try {
		if (flrig_client->execute("rig.get_mode", XmlRpcValue(), res, timeout) ) {
			static string md;
			md = string(res);
			bool posted = (md == posted_mode);
			if (!wait_mode && (!posted || need_sideband)) {
				posted_mode = md;
				guard_lock flrig_modelock(&mutex_flrig_mode);
				if (flrig_client->execute("rig.get_sideband", XmlRpcValue(), res, timeout) ) {
					static string sb;
					sb = string(res);
					xml_USB = (sb[0] == 'U');
				} else {
					xml_USB = true;
				}
				Fl::awake(xmlrpc_rig_post_mode, reinterpret_cast<void*>(&md));
				LOG_INFO("get mode: %s:%s", md.c_str(), xml_USB ? "USB" : "LSB");
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
			LOG_ERROR("%s", "get mode failed");
		}
	} catch (...) {}
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
	bws_posted = false;
}

void flrig_get_modes()
{
	guard_lock flrig_lock(&mutex_flrig);
	try {
		if (flrig_client->execute("rig.get_modes", XmlRpcValue(), modes_result, timeout) ) {
			guard_lock flrig_lock(&mutex_flrig_modes);
			Fl::awake(xmlrpc_rig_post_modes);
			posted_mode = posted_bw = posted_bw1 = posted_bw2 = "GETME";
			{
				int nargs = modes_result.size();
				static string debugstr;
				debugstr.assign("Mode table: ");
				for (int i = 0; i < nargs - 1; i++)
					debugstr.append(modes_result[i]).append(",");
				debugstr.append(modes_result[nargs-1]);
				LOG_INFO("%s", debugstr.c_str());
			}
		} else {
			LOG_ERROR("%s", "get modes failed");
		}
	} catch (...) {}
}

//----------------------------------------------------------------------
// transceiver get / set bandwidth
// transceiver get bandwidth table
//----------------------------------------------------------------------

void set_flrig_bw(int bw2, int bw1)
{
	if (!connected_to_flrig) return;

	XmlRpcValue val, result;
	try {
		int ival = bw2;
		if (bw1 > -1) ival = 256*(bw1+128) + bw2;
		val = ival;

		guard_lock flrig_lock(&mutex_flrig);
		LOG_INFO("set_flrig_bw %04X", ival);
		if (!flrig_client->execute("rig.set_bw", val, result, timeout)) {
			LOG_ERROR("%s", "rig.set_bw failed");
			wait_bw = false;
			wait_bw_timeout = 0;
		} else {
			wait_bw = true;
			wait_bw_timeout = 5;
		}
	} catch (...) {}
}

pthread_mutex_t mutex_flrig_bw = PTHREAD_MUTEX_INITIALIZER;
void xmlrpc_rig_post_bw(void *)
{
	guard_lock flrig_lock(&mutex_flrig_bw);
	if (!qso_opBW) return;

	if (posted_bw != (std::string)(qso_opBW->value())) {
		qso_opBW->value(posted_bw.c_str());
		qso_opBW->redraw();
		LOG_INFO("Update BW %s", posted_bw.c_str());
	}
}

void xmlrpc_rig_post_bw1(void *)
{
	guard_lock flrig_lock(&mutex_flrig_bw);
	if (!qso_opBW1) return;

	if (posted_bw1 != (std::string)(qso_opBW1->value())) {
		qso_opBW1->value(posted_bw1.c_str());
		qso_opBW1->redraw();
		LOG_INFO("Update combo BW1 %s", posted_bw1.c_str());
	}
}

void xmlrpc_rig_post_bw2(void *)
{
	guard_lock flrig_lock(&mutex_flrig_bw);
	if (!qso_opBW2) return;
	if (posted_bw2 != (std::string)(qso_opBW2->value())) {
		qso_opBW2->value(posted_bw2.c_str());
		qso_opBW2->redraw();
		LOG_INFO("Update combo BW2 %s", posted_bw2.c_str());
	}
}

void flrig_get_bw()
{
	guard_lock flrig_lock(&mutex_flrig);
	XmlRpcValue res;
	try {
		if (wait_bw_timeout) {
			wait_bw_timeout--;
			return;
		}

		if (flrig_client->execute("rig.get_bw", XmlRpcValue(), res, timeout) ) {
			static string s1;
			static string s2;

			s2 = string(res[0]);
			s1 = string(res[1]);
			if (!s1.empty())  {
				posted_bw1 = s1;
				Fl::awake(xmlrpc_rig_post_bw1);
				posted_bw2 = s2;
				Fl::awake(xmlrpc_rig_post_bw2);
			} else {
				if (!s2.empty()) {
					posted_bw = s2;
					Fl::awake(xmlrpc_rig_post_bw);
				}
			}
			wait_bw_timeout = 0;
		} else {
			connected_to_flrig = false;
			wait_bw_timeout = 0;
			LOG_ERROR("%s", "get bw failed!");
		}
	} catch (...) {}
}

pthread_mutex_t mutex_flrig_bws = PTHREAD_MUTEX_INITIALIZER;
void xmlrpc_rig_post_bws(void *)
{
	if (!qso_opBW) return;

	int nargs;

	try { // two BW controls
		nargs = bws_result[1].size();

		static string bwstr;
		qso_opBW1->clear();
		for (int i = 1; i < nargs; i++) {
			bwstr = string(bws_result[1][i]);
			qso_opBW1->add(bwstr.c_str());
		}

		string labels1 = bws_result[1][0];
		static char btn1_label[2];
		btn1_label[0] = labels1[0]; btn1_label[1] = 0;
		qso_btnBW1->label(btn1_label);
		qso_btnBW1->redraw_label();
		qso_btnBW1->redraw();
		static char tooltip1[20];
		snprintf(tooltip1,sizeof(tooltip1),"%s",labels1.substr(2).c_str());
		qso_opBW1->tooltip(tooltip1);
		qso_opBW1->index(0);
		qso_opBW1->redraw();

		{
			static string debugstr;
			debugstr.assign("\nBW1 table: ");
			for (int i = 1; i < nargs-1; i++)
				debugstr.append(string(bws_result[1][i])).append(", ");
			debugstr.append(string(bws_result[1][nargs - 1])).append("\n");
			debugstr.append(labels1);
			LOG_INFO("%s", debugstr.c_str());
		}

		try {
			nargs = bws_result[0].size();

			static string bwstr;
			qso_opBW2->clear();
			for (int i = 1; i < nargs; i++) {
				bwstr = string(bws_result[0][i]);
				qso_opBW2->add(bwstr.c_str());
			}

			string labels2 = bws_result[0][0];
			static char btn2_label[2];
			btn2_label[0] = labels2[0]; btn2_label[1] = 0;
			qso_btnBW2->label(btn2_label);
			qso_btnBW2->redraw_label();
			qso_btnBW2->redraw();
			static char tooltip2[20];
			snprintf(tooltip2,sizeof(tooltip2),"%s",labels2.substr(2).c_str());
			qso_opBW2->tooltip(tooltip2);
			qso_opBW2->index(0);
			qso_opBW2->redraw();

			{
				static string debugstr;
				debugstr.assign("\nBW2 table: ");
				for (int i = 1; i < nargs-1; i++)
					debugstr.append(string(bws_result[0][i])).append(", ");
				debugstr.append(string(bws_result[0][nargs - 1])).append("\n");
				debugstr.append(labels2);
				LOG_INFO("%s", debugstr.c_str());
			}

		} catch ( XmlRpcException err) {
			bws_posted = false;
			return;
		}
		qso_opBW->hide();
		qso_opGROUP->show();
		bws_posted = true;
		return;
	} catch (XmlRpcException err) {
		try { // one BW control
			nargs = bws_result[0].size();
			string bwstr;
			qso_opBW->clear();
			for (int i = 1; i < nargs; i++) {
				bwstr = string(bws_result[0][i]);
				qso_opBW->add(bwstr.c_str());
			}
			qso_opBW->index(0);
			qso_opBW->activate();
			qso_opBW->tooltip("xcvr bandwidth");
			qso_opBW->show();
			qso_opGROUP->hide();

			{
				static string debugstr;
				debugstr.assign("BW table: ");
				for (int i = 1; i < nargs-1; i++)
					debugstr.append(string(bws_result[0][i])).append(", ");
				debugstr.append(string(bws_result[0][nargs - 1]));
				LOG_INFO("%s", debugstr.c_str());
			}

		} catch (XmlRpcException err) {
			LOG_ERROR("%s", "no bandwidths specified");
			qso_opBW->add("");
			qso_opBW->index(0);
			qso_opBW->deactivate();
			return;
		}
	}
	bws_posted = true;
}

void flrig_get_bws()
{
	if (bws_posted) return;
	XmlRpcValue result;
	try {
		if (flrig_client->execute("rig.get_bws", XmlRpcValue(), bws_result, timeout) ) {
			bws_posted = false;
			wait_bw = true;
			wait_bw_timeout = 5;
			posted_bw.clear();
			Fl::awake(xmlrpc_rig_post_bws);
		} else {
			LOG_ERROR("%s", "get bws failed");
		}
	} catch (...) {}
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
//	if (!flrig_client->execute("rig.set_AB", val, result, timeout))
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
	try {
		if (flrig_client->execute("rig.get_AB", XmlRpcValue(), result, timeout) ) {
//		string str_vfo = string(result[0]);
//		if (str_vfo == "A" && !btn_vfoA->value()) REQ(FLRIG_show_A);
//		else if (str_vfo == "B" && !btn_vfoB->value()) REQ(FLRIG_show_B);
		} else {
			connected_to_flrig = false;
		}
	} catch (...) {}
}

//==============================================================================
// transceiver set / get notch
//==============================================================================
static bool wait_notch = false; // wait for transceiver to respond
static int  wait_notch_timeout = 5; // 5 polls and then disable wait
static int  xcvr_notch = 0;

void set_flrig_notch()
{
	if (!connected_to_flrig) return;

	guard_lock flrig_lock(&mutex_flrig);

	XmlRpcValue val, result;
	try {
		val = (double)(notch_frequency);
		if (flrig_client->execute("rig.set_notch", val, result, timeout)) {
			wait_notch = true;
			wait_notch_timeout = 5;
			xcvr_notch = notch_frequency;
		} else {
			LOG_ERROR("%s", "rig.set_notch failed");
			wait_notch = 0;
			wait_notch_timeout = 0;
			xcvr_notch = 0;
		}
	} catch (...) {}
}

void flrig_get_notch()
{
	guard_lock flrig_lock(&mutex_flrig);
	try {
		if (flrig_client->execute("rig.get_notch", XmlRpcValue(), notch_result, timeout) ) {
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
	} catch (...) {}
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
	try {
		if (flrig_client->execute("rig.get_smeter", val, result, timeout)) {
			int val = (int)(result);
			guard_lock flrig_lock(&mutex_flrig_smeter);
			Fl::awake(xmlrpc_rig_set_smeter, reinterpret_cast<void*>(val));
		}
	} catch (...) {}
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
	try {
		if (flrig_client->execute("rig.get_pwrmeter", val, result, timeout)) {
			int val = (int)(result);
			guard_lock flrig_lock(&mutex_flrig_pwrmeter);
			Fl::awake(xmlrpc_rig_set_pwrmeter, reinterpret_cast<void*>(val));
		}
	} catch (...) {}
}

//==============================================================================
// transceiver get name
//==============================================================================

void xmlrpc_rig_show_xcvr_name(void *)
{
	xcvr_title = xcvr_name;
	setTitle();
}

bool flrig_get_xcvr()
{
	XmlRpcValue result;
	try {
		bool ret;
		{
			guard_lock flrig_lock(&mutex_flrig);
			ret = flrig_client->execute("rig.get_xcvr", XmlRpcValue(), result, timeout);
		}
		if (ret) {
			string nuxcvr = string(result);
			if (nuxcvr != xcvr_name) {
				xcvr_name = nuxcvr;
				modes_posted = false;
				bws_posted = false;
				flrig_get_modes();
				flrig_get_bws();
				flrig_get_mode();
				flrig_get_bw();
				Fl::awake(xmlrpc_rig_show_xcvr_name);
			}
			return true;
		} else {
			if (xcvr_name != "") {
				xcvr_name = "";
				Fl::awake(xmlrpc_rig_show_xcvr_name);
				init_NoRig_RigDialog();
			}
			connected_to_flrig = false;
		}
	} catch (XmlRpcException *err) {
		if (xcvr_name != "") {
			xcvr_name = "";
			Fl::awake(xmlrpc_rig_show_xcvr_name);
			init_NoRig_RigDialog();
		}
		connected_to_flrig = false;
	}
	return false;
}

//======================================================================
// xmlrpc read polling thread
//======================================================================
static bool run_flrig_thread = true;
static int poll_interval = 100; // 1 second

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
	XmlRpcValue noArgs, result;
	try {
		bool ret;
		{
			guard_lock flrig_lock(&mutex_flrig);
			ret = flrig_client->execute("system.listMethods", noArgs, result, timeout);
		}
		if (ret) {
			int nargs = result.size();
			string method_str = "\nMethods:\n";
			for (int i = 0; i < nargs; i++)
				method_str.append("    ").append(result[i]).append("\n");
			LOG_INFO("%s", method_str.c_str());
			connected_to_flrig = true;
			poll_interval = 20; // every 200 msec
			flrig_get_xcvr();
			Fl::awake(flrig_setQSY);
		} else {
			LOG_VERBOSE("%s", "Waiting for flrig");
			connected_to_flrig = false;
			poll_interval = 500; // every 5 seconds
		}
	} catch (...) {
		LOG_ERROR("%s", "failure in flrig_client");
	}
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
				progdefaults.flrig_ip_address.c_str(),
				atol(progdefaults.flrig_ip_port.c_str()));
		LOG_INFO("created flrig xmlrpc client  %s, %ld",
				progdefaults.flrig_ip_address.c_str(),
				atol(progdefaults.flrig_ip_port.c_str()));
		flrig_connection();
	} catch (...) {
		LOG_ERROR("Cannot create flrig xmlrpc client %s, %s",
					progdefaults.flrig_ip_address.c_str(),
					progdefaults.flrig_ip_port.c_str());
	}
}

void * flrig_thread_loop(void *d)
{
	while(run_flrig_thread) {
		for (int i = 0; i < poll_interval; i++) {
			if (!run_flrig_thread) {
//				LOG_INFO("Exiting thread - 1");
				return NULL;
			}
			MilliSleep(10);
		}

		if (!run_flrig_thread) break;

		if (progdefaults.fldigi_client_to_flrig) {
			if (!flrig_client)
				connect_to_flrig();
			if (!connected_to_flrig) flrig_connection();
			else if (flrig_get_xcvr()) {
				if (new_ptt > -1) {
					exec_flrig_ptt();
					continue;
				}
				if (progdefaults.flrig_keys_modem) flrig_get_ptt();
				if (trx_state == STATE_RX) {
					flrig_get_frequency();
					flrig_get_smeter();
					flrig_get_notch();
					if (!modes_posted) flrig_get_modes();
					if (modes_posted)  flrig_get_mode();
					if (!bws_posted)   flrig_get_bws();
					if (bws_posted)    flrig_get_bw();
				}
				else
					flrig_get_pwrmeter();
			}
		}
	}
//	LOG_INFO("Exiting thread - 2");
	return NULL;
}

void FLRIG_start_flrig_thread()
{
	flrig_thread = new pthread_t;
	poll_interval = 100;
	if (pthread_create(flrig_thread, NULL, flrig_thread_loop, NULL)) {
		LOG_ERROR("%s", "flrig_thread create");
		exit(EXIT_FAILURE);
	}
}

void stop_flrig_thread()
{
	if (!flrig_client) return;
	LOG_INFO("%s", "stopping flrig thread");
	flrig_client->close();
	pthread_mutex_lock(&mutex_flrig);
		run_flrig_thread = false;
	pthread_mutex_unlock(&mutex_flrig);
	pthread_join(*flrig_thread, NULL);
	LOG_INFO("%s", "flrig thread closed");
}

void reconnect_to_flrig()
{
	flrig_client->close();
	pthread_mutex_lock(&mutex_flrig);
	delete flrig_client;
	flrig_client = (XmlRpcClient *)0;
	connected_to_flrig = false;
	pthread_mutex_unlock(&mutex_flrig);
}

