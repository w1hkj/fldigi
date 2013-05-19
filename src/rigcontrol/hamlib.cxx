// ----------------------------------------------------------------------------
// hamlib.cxx  --  Hamlib (rig control) interface for fldigi
//
// Copyright (C) 2007-2009
//		Dave Freese, W1HKJ
// Copyright (C) 2008-2009
//		Stelios Bounanos, M0GLD
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

#include <config.h>

#include <cstdlib>
#include <string>
#include <vector>
#include <algorithm>

#include "trx.h"
#include "configuration.h"
#include "confdialog.h"

#include "rigclass.h"

#include "threads.h"
#include "misc.h"

#include "fl_digi.h"
#include "main.h"
#include "misc.h"

#include "rigsupport.h"

#include "stacktrace.h"
#ifdef __WOE32__
#  include "serial.h"
#endif
#include "debug.h"
#include "re.h"

LOG_FILE_SOURCE(debug::LOG_RIGCONTROL);

using namespace std;

static pthread_mutex_t	hamlib_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t	*hamlib_thread = 0;

static bool hamlib_exit = false;

static bool hamlib_ptt = false;
static bool hamlib_qsy = false;
static bool need_freq = false;
static bool need_mode = false;
static bool hamlib_bypass = false;
static bool hamlib_closed = true;//false;
static 	int hamlib_passes = 20;

static long int hamlib_freq;
static rmode_t hamlib_rmode = RIG_MODE_USB;
static pbwidth_t hamlib_pbwidth = 3000;

typedef std::vector<const struct rig_caps *> rig_list_t;
rig_list_t hamlib_rigs;

enum { SIDEBAND_RIG, SIDEBAND_LSB, SIDEBAND_USB };

static void *hamlib_loop(void *args);

void show_error(const char* msg1, const char* msg2 = 0)
{
	string error = msg1;
	if (msg2)
		error.append(": ").append(msg2);
	put_status(error.c_str(), 10.0);
	LOG_ERROR("%s", error.c_str());
}

void hamlib_get_defaults()
{
	char szParam[40];
	int i;
	Rig testrig;
	rig_model_t rigmodel;

	rigmodel = hamlib_get_rig_model(cboHamlibRig->index());
	testrig.init(rigmodel);

	if (testrig.getCaps()->port_type != RIG_PORT_SERIAL) {
		testrig.close();
		return;
	}

	testrig.getConf("serial_speed", szParam);
	mnuBaudRate->value(progdefaults.nBaudRate(szParam));

	testrig.getConf("post_write_delay", szParam);
	sscanf(szParam, "%d", &i);
	cntHamlibWait->value(i);

	testrig.getConf("write_delay", szParam);
	sscanf(szParam, "%d", &i);
	cntHamlibWriteDelay->value(i);

	testrig.getConf("timeout", szParam);
	sscanf(szParam, "%d", &i);
	cntHamlibTimeout->value(i);

	testrig.getConf("retry", szParam);
	sscanf(szParam, "%d", &i);
	cntHamlibRetries->value(i);

	testrig.getConf("rts_state", szParam);
	chkHamlibRTSplus->value( strcmp(szParam, "ON") == 0 ? true : false);

	testrig.getConf("dtr_state", szParam);
	btnHamlibDTRplus->value( strcmp(szParam, "ON") == 0 ? true : false);

	testrig.getConf("serial_handshake", szParam);
	chkHamlibRTSCTSflow->value(strcmp(szParam, "Hardware") == 0 ? true : false);
	chkHamlibXONXOFFflow->value(strcmp(szParam, "XONXOFF") == 0 ? true : false);

	testrig.getConf("stop_bits", szParam);
	valHamRigStopbits->value(strcmp(szParam, "1") == 0 ? 1 : 2);

	if (!testrig.canSetPTT()) {
		btnHamlibCMDptt->value(0);
		btnHamlibCMDptt->deactivate();
	} else {
		btnHamlibCMDptt->value(1);
		btnHamlibCMDptt->activate();
	}
	inpHamlibConfig->value("");

	testrig.close();
}

void hamlib_restore_defaults()
{
	cboHamlibRig->index(hamlib_get_index(progdefaults.HamRigModel));
	inpRIGdev->value(progdefaults.HamRigDevice.c_str());
	cntHamlibRetries->value(progdefaults.HamlibRetries);
	cntHamlibTimeout->value(progdefaults.HamlibTimeout);
	cntHamlibWriteDelay->value(progdefaults.HamlibWriteDelay);
	cntHamlibWait->value(progdefaults.HamlibWait);
	btnHamlibCMDptt->value(progdefaults.HamlibCMDptt);
	btnHamlibDTRplus->value(progdefaults.HamlibDTRplus);
	chkHamlibRTSCTSflow->value(progdefaults.HamlibRTSCTSflow);
	if (chkHamlibRTSCTSflow->value()) chkHamlibRTSplus->deactivate();
	chkHamlibRTSplus->value(progdefaults.HamlibRTSplus);
	chkHamlibXONXOFFflow->value(progdefaults.HamlibXONXOFFflow);
	mnuSideband->value(progdefaults.HamlibSideband);
	valHamRigStopbits->value(progdefaults.HamRigStopbits);
	mnuBaudRate->value(progdefaults.HamRigBaudrate);
	if (xcvr && xcvr->canSetPTT()) btnHamlibCMDptt->activate();
	else btnHamlibCMDptt->deactivate();
	btnHamlibCMDptt->value(progdefaults.HamlibCMDptt);
	inpHamlibConfig->value(progdefaults.HamConfig.c_str());

	btnInitHAMLIB->labelcolor(FL_FOREGROUND_COLOR);
	btnInitHAMLIB->redraw_label();
	btnRevertHAMLIB->deactivate();
	btnRevertHAMLIB->redraw();
}

void hamlib_init_defaults()
{
	progdefaults.HamRigModel = hamlib_get_rig_model(cboHamlibRig->index());
	progdefaults.HamRigDevice = inpRIGdev->value();
	progdefaults.HamlibRetries = static_cast<int>(cntHamlibRetries->value());
	progdefaults.HamlibTimeout = static_cast<int>(cntHamlibTimeout->value());
	progdefaults.HamlibWriteDelay = static_cast<int>(cntHamlibWriteDelay->value());
	progdefaults.HamlibWait = static_cast<int>(cntHamlibWait->value());
	progdefaults.HamlibCMDptt = btnHamlibCMDptt->value();
	progdefaults.HamlibDTRplus = btnHamlibDTRplus->value();
	progdefaults.HamlibRTSCTSflow = chkHamlibRTSCTSflow->value();
	progdefaults.HamlibRTSplus = chkHamlibRTSplus->value();
	progdefaults.HamlibXONXOFFflow = chkHamlibXONXOFFflow->value();
	progdefaults.HamlibSideband = mnuSideband->value();
	progdefaults.HamRigStopbits = static_cast<int>(valHamRigStopbits->value());
	progdefaults.HamRigBaudrate = mnuBaudRate->value();
	progdefaults.HamlibCMDptt = btnHamlibCMDptt->value();
	progdefaults.HamConfig = inpHamlibConfig->value();
}

bool hamlib_init(bool bPtt)
{
	freq_t freq;
//	rmode_t mode;
//	pbwidth_t width;

	hamlib_ptt = bPtt;

	hamlib_init_defaults();

#ifdef __CYGWIN__
	string port = progdefaults.HamRigDevice;
	com_to_tty(port);
#endif

	if (progdefaults.HamRigModel == 0) {
		LOG_ERROR("No such hamlib rig model");
		return false;
	}

	try {
		char szParam[20];

		xcvr->init(progdefaults.HamRigModel);

#ifdef __CYGWIN__
		xcvr->setConf("rig_pathname", port.c_str());
#else
		xcvr->setConf("rig_pathname", progdefaults.HamRigDevice.c_str());
#endif
		snprintf(szParam, sizeof(szParam), "%d", progdefaults.HamlibWait);
		xcvr->setConf("post_write_delay", szParam);

		snprintf(szParam, sizeof(szParam), "%d", progdefaults.HamlibWriteDelay);
		xcvr->setConf("write_delay", szParam);

		snprintf(szParam, sizeof(szParam), "%d", progdefaults.HamlibTimeout);
		xcvr->setConf("timeout", szParam);

		snprintf(szParam, sizeof(szParam), "%d", progdefaults.HamlibRetries);
		xcvr->setConf("retry", szParam);

		if (xcvr->getCaps()->port_type == RIG_PORT_SERIAL) {
			xcvr->setConf("serial_speed", progdefaults.strBaudRate());

			if (progdefaults.HamlibDTRplus)
				xcvr->setConf("dtr_state", "ON");
			else
				xcvr->setConf("dtr_state", "OFF");

			if (progdefaults.HamlibRTSCTSflow)
				xcvr->setConf("serial_handshake", "Hardware");
			else if (progdefaults.HamlibXONXOFFflow)
				xcvr->setConf("serial_handshake", "XONXOFF");
			else
				xcvr->setConf("serial_handshake", "None");

			if (!progdefaults.HamlibRTSCTSflow) {
				if (progdefaults.HamlibRTSplus)
					xcvr->setConf("rts_state", "ON");
				else
					xcvr->setConf("rts_state", "OFF");
			}

			xcvr->setConf("stop_bits", progdefaults.HamRigStopbits == 1 ? "1" : "2");
		}

		string::size_type c = progdefaults.HamConfig.find('#');
		if (c != string::npos)
			progdefaults.HamConfig.erase(c);
		if (!progdefaults.HamConfig.empty()) {
			re_t re("([^, =]+) *= *([^, =]+)", REG_EXTENDED);
			const char* conf = progdefaults.HamConfig.c_str();
			int end;
			while (re.match(conf)) {
				xcvr->setConf(re.submatch(1).c_str(), re.submatch(2).c_str());
				re.suboff(0, NULL, &end);
				conf += end;
			}
		}
		xcvr->open();
	}
	catch (const RigException& Ex) {
		show_error(__func__, Ex.what());
		xcvr->close();
		return false;
	}

	LOG_DEBUG("trying frequency request");
	try {
		if ( !xcvr->canGetFreq() ) need_freq = false; // getFreq will return setFreq value
		else {
			need_freq = true;
			freq = xcvr->getFreq();
			if (freq <= 0) {
				xcvr->close();
				show_error(__func__, "Rig not responding");
				return false;
			}
		}
	}
	catch (const RigException& Ex) {
		show_error("Get Freq", Ex.what());
		need_freq = false;
	}
	if (!need_freq) {
		xcvr->close();
		LOG_VERBOSE("Failed freq test");
		return false;
	}

	LOG_DEBUG("trying mode request");
	try {
		if ( !xcvr->canGetMode() ) need_mode = false;
		else {
			need_mode = true;
//			mode = xcvr->getMode(width);
		}
	}
	catch (const RigException& Ex) {
		show_error("Get Mode", Ex.what());
		need_mode = false;
	}

	try {
		if (hamlib_ptt == true) {
			LOG_VERBOSE("trying PTT");
			if (!xcvr->canSetPTT())
				hamlib_ptt = false;
			else
				xcvr->setPTT(RIG_PTT_OFF);
		}
	}
	catch (const RigException& Ex) {
		show_error("Set Ptt", Ex.what());
		hamlib_ptt = false;
	}

	hamlib_freq = 0;
	hamlib_rmode = RIG_MODE_NONE;

	hamlib_exit = false;
	hamlib_bypass = false;

	hamlib_thread = new pthread_t;

	if (pthread_create(hamlib_thread, NULL, hamlib_loop, NULL) < 0) {
		show_error(__func__, "pthread_create failed");
		xcvr->close();
		hamlib_thread = 0;
		return false;
	}

	init_Hamlib_RigDialog();

	hamlib_closed = false;
	return true;
}

void hamlib_close(void)
{
	ENSURE_THREAD(FLMAIN_TID);

	if (hamlib_closed)
		return;

	pthread_mutex_lock(&hamlib_mutex);
		hamlib_exit = true;
	pthread_mutex_unlock(&hamlib_mutex);

	pthread_join(*hamlib_thread, NULL);
	delete hamlib_thread;
	hamlib_thread = 0;

	if (xcvr->isOnLine()) xcvr->close();
	wf->USB(true);

}

bool hamlib_active(void)
{
	return (xcvr->isOnLine());
}

void hamlib_set_ptt(int ptt)
{
	if (xcvr->isOnLine() == false)
		return;
	if (!hamlib_ptt)
		return;
	pthread_mutex_lock(&hamlib_mutex);
		try {
			xcvr->setPTT(ptt ? RIG_PTT_ON : RIG_PTT_OFF);
			hamlib_bypass = ptt ? true : false;
		}
		catch (const RigException& Ex) {
			show_error("Rig PTT", Ex.what());
			hamlib_ptt = false;
		}
	pthread_mutex_unlock(&hamlib_mutex);
}

void hamlib_set_qsy(long long f)
{
	if (xcvr->isOnLine() == false)
		return;
	pthread_mutex_lock(&hamlib_mutex);
	double fdbl = f;
	hamlib_qsy = false;
	try {
		xcvr->setFreq(fdbl);
		wf->rfcarrier(f);
		wf->movetocenter();
	}
	catch (const RigException& Ex) {
		show_error("QSY", Ex.what());
		hamlib_passes = 0;
	}
	pthread_mutex_unlock(&hamlib_mutex);
}

int hamlib_setfreq(long f)
{
	if (xcvr->isOnLine() == false)
		return -1;
	pthread_mutex_lock(&hamlib_mutex);
		try {
			LOG_DEBUG("%ld", f);
			xcvr->setFreq(f);
		}
		catch (const RigException& Ex) {
			show_error("SetFreq", Ex.what());
			hamlib_passes = 0;
		}
	pthread_mutex_unlock(&hamlib_mutex);
	return 1;
}

int hamlib_setmode(rmode_t m)
{
	if (need_mode == false)
		return -1;
	if (xcvr->isOnLine() == false)
		return -1;
	pthread_mutex_lock(&hamlib_mutex);
		try {
			hamlib_rmode = xcvr->getMode(hamlib_pbwidth);
			xcvr->setMode(m, hamlib_pbwidth);
			hamlib_rmode = m;
		}
		catch (const RigException& Ex) {
			show_error("Set Mode", Ex.what());
			hamlib_passes = 0;
		}
	pthread_mutex_unlock(&hamlib_mutex);
	return 1;
}

int hamlib_setwidth(pbwidth_t w)
{
	if (xcvr->isOnLine() == false)
		return -1;
	pthread_mutex_lock(&hamlib_mutex);
		try {
			hamlib_rmode = xcvr->getMode(hamlib_pbwidth);
			xcvr->setMode(hamlib_rmode, w);
			hamlib_pbwidth = w;
		}
		catch (const RigException& Ex) {
			show_error("Set Width", Ex.what());
			hamlib_passes = 0;
		}
	pthread_mutex_unlock(&hamlib_mutex);
	return 1;
}

rmode_t hamlib_getmode()
{
	return hamlib_rmode;
}

pbwidth_t hamlib_getwidth()
{
	return hamlib_pbwidth;
}

static void *hamlib_loop(void *args)
{
	SET_THREAD_ID(RIGCTL_TID);

	long int freq = 0L;
	rmode_t  numode = RIG_MODE_NONE;
	bool freqok = false, modeok = false;

	for (;;) {
		MilliSleep(100);
		if (hamlib_exit)
			break;
		if (hamlib_bypass)
			continue;
// hamlib locked while accessing hamlib serial i/o
		pthread_mutex_lock(&hamlib_mutex);

		if (need_freq) {
			freq_t f;
			try {
				f = xcvr->getFreq();
				freq = (long int) f;
				freqok = true;
				if (freq == 0) {
					pthread_mutex_unlock(&hamlib_mutex);
					continue;
				}
			}
			catch (const RigException& Ex) {
				show_error(__func__, "Rig not responding: freq");
				freqok = false;
			}
		}
		if (hamlib_exit)
			break;

		if (need_mode && hamlib_rmode == numode) {
			try {
				numode = xcvr->getMode(hamlib_pbwidth);
				modeok = true;
			}
			catch (const RigException& Ex) {
				show_error(__func__, "Rig not responding: mode");
				modeok = false;
			}
		}
		pthread_mutex_unlock(&hamlib_mutex);

		if (hamlib_exit)
			break;
		if (hamlib_bypass)
			continue;

		if (freqok && freq && (freq != hamlib_freq)) {
			hamlib_freq = freq;
			show_frequency(hamlib_freq);
			wf->rfcarrier(hamlib_freq);
		}

		if (modeok && (hamlib_rmode != numode)) {
			hamlib_rmode = numode;
			show_mode(modeString(hamlib_rmode));
			if (progdefaults.HamlibSideband != SIDEBAND_RIG)
				wf->USB(progdefaults.HamlibSideband == SIDEBAND_USB);
			else
				wf->USB(!(hamlib_rmode == RIG_MODE_LSB ||
					  hamlib_rmode == RIG_MODE_CWR ||
					  hamlib_rmode == RIG_MODE_PKTLSB ||
					  hamlib_rmode == RIG_MODE_ECSSLSB ||
					  hamlib_rmode == RIG_MODE_RTTY));
		}

		if (hamlib_exit)
			break;
	}

	hamlib_closed = true;

	return NULL;
}

static int add_to_list(const struct rig_caps* rc, void*)
{
	hamlib_rigs.push_back(rc);
	return 1;
}

static bool rig_cmp(const struct rig_caps* rig1, const struct rig_caps* rig2)
{
	int ret;

	ret = strcasecmp(rig1->mfg_name, rig2->mfg_name);
	if (ret > 0) return false;
	if (ret < 0) return true;
	ret = strcasecmp(rig1->model_name, rig2->model_name);
	if (ret > 0) return false;
	if (ret <= 0) return true;
	if (rig1->rig_model > rig2->rig_model)
		return false;
	return true;
}

void hamlib_get_rigs(void)
{
	if (!hamlib_rigs.empty())
		return;

	enum rig_debug_level_e dblv = RIG_DEBUG_NONE;
#ifndef NDEBUG
	const char* hd = getenv("FLDIGI_HAMLIB_DEBUG");
	if (hd) {
		dblv = static_cast<enum rig_debug_level_e>(strtol(hd, NULL, 10));
		dblv = CLAMP(dblv, RIG_DEBUG_NONE, RIG_DEBUG_TRACE);
	}
#endif
	rig_set_debug(dblv);

	rig_load_all_backends();
	rig_list_foreach(add_to_list, 0);
	sort(hamlib_rigs.begin(), hamlib_rigs.end(), rig_cmp);
}

rig_model_t hamlib_get_rig_model_compat(const char* name)
{
	for (rig_list_t::const_iterator i = hamlib_rigs.begin(); i != hamlib_rigs.end(); ++i)
		if (strstr(name, (*i)->mfg_name) && strstr(name, (*i)->model_name))
			return (*i)->rig_model;
	return 0;
}

size_t hamlib_get_index(rig_model_t model)
{
	for (rig_list_t::const_iterator i = hamlib_rigs.begin(); i != hamlib_rigs.end(); ++i)
		if ((*i)->rig_model == model)
			return i - hamlib_rigs.begin();
	return hamlib_rigs.size();
}

rig_model_t hamlib_get_rig_model(size_t i)
{
	try {
		return hamlib_rigs.at(i)->rig_model;
	}
	catch (...) {
		return 0;
	}
}

void hamlib_get_rig_str(int (*func)(const char*))
{
	string rigstr;
	for (rig_list_t::const_iterator i = hamlib_rigs.begin(); i != hamlib_rigs.end(); ++i) {
		rigstr.clear();
		rigstr.append((*i)->mfg_name).append(" ").append((*i)->model_name);
		rigstr.append("  (").append(rig_strstatus((*i)->status)).append(")");
		if (!(*func)(rigstr.c_str()))
			break;
	}
}
