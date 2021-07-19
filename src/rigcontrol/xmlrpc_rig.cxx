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

LOG_FILE_SOURCE(debug::debug::LOG_RPC_CLIENT);

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

static double timeout = 5.0;

static int wait_bws_timeout = 0;

//======================================================================

void xmlrpc_rig_set_qsy(long long rfc)
{
	unsigned long int freq = static_cast<unsigned long int>(rfc);
	set_flrig_freq(freq);
	wf->rfcarrier(freq);
	wf->movetocenter();
	show_frequency(freq);
	LOG_VERBOSE("set qsy: %lu", freq);
}

//======================================================================
// set / get pairs
//======================================================================

//----------------------------------------------------------------------
// push to talk
//----------------------------------------------------------------------
static int  ptt_state = 0;

static int  new_ptt = -1;

void exec_flrig_ptt() {
	if (!connected_to_flrig) {
		new_ptt = -1;
		return;
	}

	XmlRpcValue val, result, result2;

	try {
		bool ret;
		{
			guard_lock flrig_lock(&mutex_flrig);
			ret = flrig_client->execute("rig.set_ptt", new_ptt, result, timeout);
		}
		if (ret) {
			LOG_VERBOSE("ptt %s", ptt_state ? "ON" : "OFF");
			new_ptt = -1;
			return;
		}
	} catch (...) {
		new_ptt = -1;
		return;
	}

	LOG_ERROR("fldigi/flrig PTT %s failure", new_ptt ? "ON" : "OFF");
	new_ptt = -1;
	return;
}

void set_flrig_ptt(int on) {
	if (!active_modem)
		return;

	if (active_modem->get_mode() == MODE_CW) {
		if (progdefaults.use_FLRIGkeying) {
			flrig_cwio_ptt(on);
			if (progdefaults.CATkeying_disable_ptt)
				return;
		}
		else if (progdefaults.CATkeying_disable_ptt && 
				active_modem->get_mode() == MODE_CW &&
				( progdefaults.use_ELCTkeying ||
				  progdefaults.use_ICOMkeying ||
				  progdefaults.use_KNWDkeying ||
				  progdefaults.use_YAESUkeying ||
				  progStatus.nanoCW_online ) )
			return;
	}
	new_ptt = on;
}

pthread_mutex_t mutex_flrig_ptt = PTHREAD_MUTEX_INITIALIZER;

void xmlrpc_rig_show_ptt(void *data)
{
	guard_lock flrig_lock(&mutex_flrig_ptt);
	int on = reinterpret_cast<intptr_t>(data);
	if (wf && (trx_state != STATE_TUNE)) {
		wf->xmtrcv->value(on);
		wf->xmtrcv->do_callback();
	}
}

void flrig_get_ptt()
{
	try {
		XmlRpcValue result;
		bool ret;
		{
			guard_lock flrig_lock(&mutex_flrig);
			ret = flrig_client->execute("rig.get_ptt", XmlRpcValue(), result, timeout);
		}
		if (ret) {
			int val = (int)result;
			ptt_state = val;
LOG_VERBOSE("get_ptt: %s", ptt_state ? "ON" : "OFF");
			return;
		}
		connected_to_flrig = false;
		LOG_ERROR("%s", "get_ptt failed!");
	} catch (...) {}
}

void flrig_get_wpm()
{
	if (!connected_to_flrig) {
		return;
	}
	try {
		XmlRpcValue result;
		bool ret;
		{
			guard_lock flrig_lock(&mutex_flrig);
			ret = flrig_client->execute("rig.cwio_get_wpm", XmlRpcValue(), result, timeout);
		}
		if (ret) {
			int val = (int)result;
			progdefaults.CWspeed = val;
LOG_VERBOSE("rig.cwio_get_wpm = %d", val);
			return;
		}
		connected_to_flrig = false;
		LOG_ERROR("%s failed!", "flrig_get_wpm");
	} catch (...) {
LOG_ERROR("rig.cwio_get_wpm FAILED");
	}
}

void flrig_set_wpm() {
	if (!connected_to_flrig) {
		return;
	}
	XmlRpcValue val, result;
	try {
		bool ret;
		val = (int)static_cast<int>(progdefaults.CWspeed);
		{
			guard_lock flrig_lock(&mutex_flrig);
			ret = flrig_client->execute("rig.cwio_set_wpm", val, result, timeout);
		}
		if (ret) {
LOG_VERBOSE("set wpm %f", progdefaults.CWspeed);
			return;
		}
	} catch (...) {
	}

	LOG_ERROR("set wpm failed");
	return;
}

//----------------------------------------------------------------------
// transceiver radio frequency
//----------------------------------------------------------------------

static bool wait_freq = false; // wait for transceiver to respond
static int  wait_freq_timeout = 5; // 5 polls and then disable wait
static unsigned long int  xcvr_freq = 0;

pthread_mutex_t mutex_flrig_freq = PTHREAD_MUTEX_INITIALIZER;

void xmlrpc_rig_show_freq(void * fr)
{
	guard_lock flrig_lock(&mutex_flrig_freq);
	if (!wf) return;
	unsigned long int freq = reinterpret_cast<intptr_t>(fr);
LOG_VERBOSE("xmlrpc_rig_show_freq %lu", freq);
	wf->rfcarrier(freq);
	wf->movetocenter();
	show_frequency(freq);
}

void set_flrig_freq(unsigned long int fr)
{
	if (!connected_to_flrig) return;

	XmlRpcValue val, result;
	try {
		val = (double)fr;
		bool ret;
		{
			guard_lock flrig_lock(&mutex_flrig);
			ret = flrig_client->execute("rig.set_vfo", val, result, timeout);
		}
		if (ret) {
			LOG_VERBOSE("set freq: %lu", fr);
			return;
		}
		LOG_ERROR("%s", "rig.set_vfo failed");
		wait_freq = false;
		wait_freq_timeout = 0;
	} catch (...) {
LOG_ERROR("rig.set_vfo FAILED");
	}
}

void flrig_get_frequency()
{
	XmlRpcValue result;
	try {
		if (!freq_posted) return;
		bool ret;
		{
			guard_lock flrig_lock(&mutex_flrig);
			ret = flrig_client->execute("rig.get_vfo", XmlRpcValue(), result, timeout);
		}
		if (ret) {
			str_freq = (string)result;
			unsigned long int fr = atoll(str_freq.c_str());

			if (!wait_freq && (fr != xcvr_freq)) {
				xcvr_freq = fr;
				guard_lock flrig_lock(&mutex_flrig_freq);
				Fl::awake(xmlrpc_rig_show_freq, reinterpret_cast<void*>(fr));
				LOG_VERBOSE("get freq: %lu", fr);
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

		bool ret;
		{
			guard_lock flrig_lock(&mutex_flrig);
			ret = flrig_client->execute("rig.set_mode", val, result, timeout);
		}
		if (ret) {
			posted_mode = md;
			need_sideband = true;
			bws_posted = false;
			wait_mode = true;
			wait_mode_timeout = 10;
			wait_bws_timeout = 5;
			qso_opBW->hide();
			qso_opGROUP->hide();
			LOG_VERBOSE("set mode: %s", md);
		} else {
			LOG_ERROR("%s", "rig.set_mode failed");
			wait_mode = false;
			wait_mode_timeout = 10;
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
	XmlRpcValue res;
	try {
		bool ret;
		{
			guard_lock flrig_lock(&mutex_flrig);
			ret = flrig_client->execute("rig.get_mode", XmlRpcValue(), res, timeout);
		}
		if (ret) {
			static string md;
			md = (string)res;
			bool posted = (md == posted_mode);
			if (!wait_mode && (!posted || need_sideband)) {
				posted_mode = md;
				guard_lock flrig_modelock(&mutex_flrig_mode);
				{
					guard_lock flrig_lock(&mutex_flrig);
					ret = flrig_client->execute("rig.get_sideband", XmlRpcValue(), res, timeout);
				}
				if (ret) {
					static string sb;
					sb = (string)res;
					xml_USB = (sb[0] == 'U');
				} else {
					xml_USB = true;
				}
				if (posted) {
					need_sideband = false;
					return;
				}
				Fl::awake(xmlrpc_rig_post_mode, reinterpret_cast<void*>(&md));
				LOG_VERBOSE("get mode: %s:%s", md.c_str(), xml_USB ? "USB" : "LSB");
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

	std::string smodes;
	for (int i = 0; i < nargs; i++)
		smodes.append((string)modes_result[i]).append("|");
	qso_opMODE->add(smodes.c_str());

	qso_opMODE->index(0);
	qso_opMODE->activate();

	modes_posted = true;
	bws_posted = false;
}

void flrig_get_modes()
{
	try {
		bool ret;
		{
			guard_lock flrig_lock(&mutex_flrig);
			ret = flrig_client->execute("rig.get_modes", XmlRpcValue(), modes_result, timeout);
		}
		if (ret) {
			guard_lock flrig_lock(&mutex_flrig_modes);
			Fl::awake(xmlrpc_rig_post_modes);
			posted_mode = posted_bw = posted_bw1 = posted_bw2 = "GETME";
			{
				int nargs = modes_result.size();
				static string debugstr;
				debugstr.assign("Mode table: ");
				for (int i = 0; i < nargs - 1; i++)
					debugstr.append((string)modes_result[i]).append(",");
				debugstr.append(modes_result[nargs-1]);
				LOG_VERBOSE("%s", debugstr.c_str());
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

		LOG_VERBOSE("set_flrig_bw %04X", ival);
		bool ret;
		{
			guard_lock flrig_lock(&mutex_flrig);
			ret = flrig_client->execute("rig.set_bw", val, result, timeout);
		}
		if (ret) {
			wait_bw = true;
			wait_bw_timeout = 5;
		} else {
			LOG_ERROR("%s", "rig.set_bw failed");
			wait_bw = false;
			wait_bw_timeout = 0;
		}
	} catch (...) {}
}

pthread_mutex_t mutex_flrig_bw = PTHREAD_MUTEX_INITIALIZER;
void xmlrpc_rig_post_bw(void *)
{
	guard_lock flrig_lock(&mutex_flrig_bw);
	if (!qso_opBW) return;

	if (posted_bw != (std::string)(qso_opBW->value())) {
		qso_opBW->value(posted_bw);//.c_str());
		qso_opBW->redraw();
		LOG_VERBOSE("Update BW %s", posted_bw.c_str());
	}
	qso_opBW->show();
}

void xmlrpc_rig_post_bw1(void *)
{
	guard_lock flrig_lock(&mutex_flrig_bw);
	if (!qso_opBW1) return;

	if (posted_bw1 != (std::string)(qso_opBW1->value())) {
		qso_opBW1->value(posted_bw1);//.c_str());
		qso_opBW1->redraw();
		LOG_VERBOSE("Update combo BW1 %s", posted_bw1.c_str());
	}
	qso_opGROUP->show();
}

void xmlrpc_rig_post_bw2(void *)
{
	guard_lock flrig_lock(&mutex_flrig_bw);
	if (!qso_opBW2) return;

	if (posted_bw2 != (std::string)(qso_opBW2->value())) {
		qso_opBW2->value(posted_bw2);//.c_str());
		qso_opBW2->redraw();
		LOG_VERBOSE("Update combo BW2 %s", posted_bw2.c_str());
	}
	qso_opGROUP->show();
}

void do_flrig_get_bw()
{
	XmlRpcValue res;
	try {
		bool ret;
		{
			guard_lock flrig_lock(&mutex_flrig);
			ret = flrig_client->execute("rig.get_bw", XmlRpcValue(), res, timeout);
		}
		if (ret) {
			static string s1;
			static string s2;

			s2 = (string)res[0];
			s1 = (string)res[1];
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
	} catch (...) {
		}
}

void flrig_get_bw()
{
	if (wait_bw_timeout) {
		wait_bw_timeout--;
		return;
	}
	do_flrig_get_bw();
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
			bwstr = (string)bws_result[1][i];
			qso_opBW1->add(bwstr.c_str());
		}

		string labels1 = (string)bws_result[1][0];
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
				debugstr.append((string)bws_result[1][i]).append(", ");
			debugstr.append((string)bws_result[1][nargs - 1]).append("\n");
			debugstr.append(labels1);
			LOG_VERBOSE("%s", debugstr.c_str());
		}

		try {
			nargs = bws_result[0].size();

			static string bwstr;
			qso_opBW2->clear();
			for (int i = 1; i < nargs; i++) {
				bwstr = (string)bws_result[0][i];
				qso_opBW2->add(bwstr.c_str());
			}

			string labels2 = (string)bws_result[0][0];
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
					debugstr.append((string)bws_result[0][i]).append(", ");
				debugstr.append((string)bws_result[0][nargs - 1]).append("\n");
				debugstr.append(labels2);
				LOG_VERBOSE("%s", debugstr.c_str());
			}

		} catch ( XmlRpcException err) {
			bws_posted = false;
			return;
		}
		qso_opBW->hide();
		bws_posted = true;
		return;
	} catch (XmlRpcException err) {
		try { // one BW control
			nargs = bws_result[0].size();
			string bwstr;
			qso_opBW->clear();
			for (int i = 1; i < nargs; i++) {
				bwstr.append((string)bws_result[0][i]).append("|");
			}
			qso_opBW->add(bwstr.c_str());
			qso_opBW->index(0);
			qso_opBW->activate();
			qso_opBW->tooltip("xcvr bandwidth");
			qso_opGROUP->hide();

			{
				static string debugstr;
				debugstr.assign("BW table: ");
				for (int i = 1; i < nargs-1; i++)
					debugstr.append((string)bws_result[0][i]).append(", ");
				debugstr.append((string)bws_result[0][nargs - 1]);
				LOG_VERBOSE("%s", debugstr.c_str());
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
do_flrig_get_bw();
}

void flrig_get_bws()
{
	if (bws_posted)
		return;
	if (wait_bws_timeout) {
		wait_bws_timeout--;
		return;
	}
	XmlRpcValue result;
	try {
		bool ret;
		{
			guard_lock flrig_lock(&mutex_flrig);
			ret = flrig_client->execute("rig.get_bws", XmlRpcValue(), bws_result, timeout);
		}
		if (ret) {
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
}

void show_A(void *)
{
}

void show_B(void *)
{
}

void flrig_get_vfo()
{
	XmlRpcValue result;
	try {
		bool ret;
		{
			guard_lock flrig_lock(&mutex_flrig);
			ret = flrig_client->execute("rig.get_AB", XmlRpcValue(), result, timeout);
		}
		if (ret) {
		} else {
			connected_to_flrig = false;
		}
	} catch (...) {}
}

//==============================================================================
// transceiver set / get notch
//==============================================================================
static int  wait_notch_timeout = 0; // # polls and then disable wait

void set_flrig_notch()
{
	if (!connected_to_flrig) return;

	XmlRpcValue val, result;
	try {
		val = (int)notch_frequency;
		bool ret;
		{
			guard_lock flrig_lock(&mutex_flrig);
			ret = flrig_client->execute("rig.set_notch", val, result, timeout);
		}
		if (ret) {
			wait_notch_timeout = 2;
		} else {
			LOG_ERROR("%s", "rig.set_notch failed");
			wait_notch_timeout = 0;
		}
	} catch (...) {}
}

void flrig_get_notch()
{
	if (wait_notch_timeout == 0)
	try {
		bool ret;
		{
			guard_lock flrig_lock(&mutex_flrig);
			ret = flrig_client->execute("rig.get_notch", XmlRpcValue(), notch_result, timeout);
		}
		if (ret) {
			notch_frequency = (int)notch_result;
LOG_VERBOSE("rig_get_notch: %d", notch_frequency);
		}
	} catch (...) {
LOG_ERROR("rig.get_notch FAILED");
	}
	else
		wait_notch_timeout--;
}

//==============================================================================
// transceiver get smeter
// transceiver get power meter
//==============================================================================

pthread_mutex_t mutex_flrig_smeter = PTHREAD_MUTEX_INITIALIZER;
static void xmlrpc_rig_set_smeter(void *data)
{
	guard_lock flrig_lock(&mutex_flrig_smeter);

	if (smeter && progStatus.meters) {
		if (!smeter->visible()) {
			if (pwrmeter) pwrmeter->hide();
			smeter->show();
		}
		int val = reinterpret_cast<intptr_t>(data);
		smeter->value(val);
	}
}

void flrig_get_smeter()
{
	XmlRpcValue result;
	try {
		bool ret;
		{
			guard_lock flrig_lock(&mutex_flrig);
			ret = flrig_client->execute("rig.get_smeter", XmlRpcValue(), result, timeout);
		}
		if (ret) {
			std::string smeter = (string)result;
			int sm = atoll(smeter.c_str());
			guard_lock lck(&mutex_flrig_smeter);
			Fl::awake(xmlrpc_rig_set_smeter, reinterpret_cast<void*>(sm));
LOG_VERBOSE("rig.get_smeter: %d", sm);
		}
	} catch (...) {
LOG_ERROR("rig.get_smeter FAILED");
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
		int val = reinterpret_cast<intptr_t>(data);
		pwrmeter->value(val);
	}
}

void flrig_get_pwrmeter()
{
	XmlRpcValue val, result;
	try {
		bool ret;
		{
			guard_lock flrig_lock(&mutex_flrig);
			ret = flrig_client->execute("rig.get_pwrmeter", val, result, timeout);
		}
		if (ret) {
			std::string meter = (string)result;
			int sm = atoll(meter.c_str());
			guard_lock lck(&mutex_flrig_pwrmeter);
			Fl::awake(xmlrpc_rig_set_pwrmeter, reinterpret_cast<void*>(sm));
		}
	} catch (...) {
LOG_ERROR("rig.get_pwrmeter FAILED");
	}
}

//==============================================================================
// transceiver get name
//==============================================================================

static void xmlrpc_rig_show_xcvr_name(void *)
{
	xcvr_title = xcvr_name;
	setTitle();
}

static void no_rig_init(void *)
{
	init_NoRig_RigDialog();
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
			string nuxcvr = (string)result;
			if (nuxcvr != xcvr_name) {
				xcvr_name = nuxcvr;
				modes_posted = false;
				bws_posted = false;
				flrig_get_modes();
				flrig_get_bws();
				flrig_get_mode();
				flrig_get_bw();
				Fl::awake(xmlrpc_rig_show_xcvr_name);
LOG_VERBOSE("flrig_get_xcvr %s", nuxcvr.c_str());
			}
			return true;
		} else {
			if (xcvr_name != "") {
				xcvr_name = "";
				Fl::awake(xmlrpc_rig_show_xcvr_name);
				Fl::awake(no_rig_init);
			}
			connected_to_flrig = false;
		}
	} catch (XmlRpcException *err) {
		if (xcvr_name != "") {
			xcvr_name = "";
			Fl::awake(xmlrpc_rig_show_xcvr_name);
			Fl::awake(no_rig_init);
LOG_ERROR("rig.get_xcvr FAILED");
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
			LOG_VERBOSE("%s", method_str.c_str());
			connected_to_flrig = true;
			poll_interval = 20; // every 200 msec
			flrig_get_xcvr();
			Fl::awake(flrig_setQSY);
		} else {
			LOG_VERBOSE("%s", "Waiting for flrig");
			connected_to_flrig = false;
			poll_interval = 200; // every 2 seconds
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
		LOG_VERBOSE("created flrig xmlrpc client  %s, %ld",
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
	int poll = poll_interval;
	while (run_flrig_thread) {

		MilliSleep(10);

		if (connected_to_flrig) {
			if (new_ptt > -1) {
				exec_flrig_ptt();
				continue;
			}
		}

		if (--poll == 0) {
			poll = poll_interval;
			if (progdefaults.fldigi_client_to_flrig) {
				if (!flrig_client)
					connect_to_flrig();
				else {
					if (!connected_to_flrig)
						flrig_connection();
					else {
						if (progdefaults.flrig_keys_modem) flrig_get_ptt();
						if (trx_state == STATE_RX) {
							flrig_get_frequency();
							flrig_get_smeter();
							flrig_get_notch();
							flrig_get_wpm();

							if (modes_posted) 
								flrig_get_mode();
							else
								flrig_get_modes();

							if (bws_posted)
								flrig_get_bw();
							else 
								flrig_get_bws();
						}
						else {
							flrig_get_pwrmeter();
							flrig_get_wpm();
						}
					}
				}
			}
		}
	}
	return NULL;
}

void FLRIG_start_flrig_thread()
{
	flrig_thread = new pthread_t;
	poll_interval = 100;  // every second
	if (pthread_create(flrig_thread, NULL, flrig_thread_loop, NULL)) {
		LOG_ERROR("%s", "flrig_thread create");
		exit(EXIT_FAILURE);
	}
}

void stop_flrig_thread()
{
	if (!flrig_client) return;
	LOG_INFO("%s", "lock mutex_flrig");
	pthread_mutex_lock(&mutex_flrig);
		run_flrig_thread = false;
	pthread_mutex_unlock(&mutex_flrig);
	LOG_INFO("%s", "wait for joined thread");
	pthread_join(*flrig_thread, NULL);
	LOG_INFO("%s", "flrig thread closed");
	LOG_INFO("%s", "flrig_client->close()");
	flrig_client->close();
}

void reconnect_to_flrig()
{
	flrig_client->close();
	guard_lock flrig_lock(&mutex_flrig);
	delete flrig_client;
	flrig_client = (XmlRpcClient *)0;
	connected_to_flrig = false;
}

unsigned long st, et;

void xmlrpc_send_command(std::string cmd)
{
	if (!connected_to_flrig) return;

	XmlRpcValue val, result;
	try {
		guard_lock flrig_lock(&mutex_flrig);
		val = std::string(cmd);
		flrig_client->execute("rig.cat_string", val, result, timeout);
		std::string ans = std::string(result);
	} catch (...) {}
	return;
}

void xmlrpc_priority(std::string cmd)
{
	if (!connected_to_flrig) return;

	XmlRpcValue val, result;
	try {
		guard_lock flrig_lock(&mutex_flrig);
		val = std::string(cmd);
		flrig_client->execute("rig.cat_priority", val, result, 0.20);//timeout);
	} catch (...) {}
	return;
}

void xmlrpc_shutdown_flrig()
{
	if (!connected_to_flrig) return;

	XmlRpcValue val, result;
	try {
		guard_lock flrig_lock(&mutex_flrig);
		if (!flrig_client->execute("rig.shutdown", XmlRpcValue(), result, timeout)) {
			LOG_ERROR("%s", "rig.shutdown failed");
		} else {
			LOG_VERBOSE("%s", "rig.shutdown OK");
		}
	} catch (...) {}
}

void flrig_cwio_ptt(int on)
{
	if (!connected_to_flrig) return;

	XmlRpcValue val, result;
	try {
		guard_lock flrig_lock(&mutex_flrig);
		val = (int)on;
		flrig_client->execute("rig.cwio_send", val, result, 0.20);//timeout);
	} catch (...) {
}
	return;
}

void flrig_cwio_send_text(string s)
{
	if (!connected_to_flrig) return;

	XmlRpcValue val, result;
	try {
		guard_lock flrig_lock(&mutex_flrig);
		val = std::string(s);
		flrig_client->execute("rig.cwio_text", val, result, 0.20);//timeout);
	} catch (...) {
	}
	return;
}

