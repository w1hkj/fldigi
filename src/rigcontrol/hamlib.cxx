//
// hamlib.cxx  --  Hamlib (rig control) interface for fldigi
#include <config.h>

#include <cstdlib>
#include <string>
#include <vector>
#include <algorithm>

#include <hamlib/rig.h>

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
#include "rigdialog.h"

#include "stacktrace.h"
#ifdef __WOE32__
#  include "serial.h"
#endif
#include "debug.h"
#include "re.h"

LOG_FILE_SOURCE(debug::LOG_RIGCONTROL);

using namespace std;

static pthread_mutex_t	hamlib_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t	hamlib_thread;

static bool hamlib_exit = false;

static bool hamlib_ptt = false;
static bool hamlib_qsy = false;
static bool need_freq = false;
static bool need_mode = false;
static bool hamlib_bypass = false;
static bool hamlib_closed = false;

static long int hamlib_freq;
static rmode_t hamlib_rmode = RIG_MODE_USB;
static pbwidth_t hamlib_pbwidth = 3000;

typedef std::vector<const struct rig_caps *> rig_list_t;
rig_list_t hamlib_rigs;

enum { SIDEBAND_RIG, SIDEBAND_LSB, SIDEBAND_USB };

static void *hamlib_loop(void *args);

void hamlib_get_defaults()
{
	char szParam[40];

	progdefaults.HamRigModel = hamlib_get_rig_model(cboHamlibRig->index());

	xcvr->init(progdefaults.HamRigModel);

	if (xcvr->getCaps()->port_type != RIG_PORT_SERIAL) {
		xcvr->close();
		return;
	}

	xcvr->getConf("serial_speed", szParam);
	progdefaults.HamRigBaudrate = progdefaults.nBaudRate(szParam);
	mnuBaudRate->value(progdefaults.HamRigBaudrate);

	xcvr->getConf("post_write_delay", szParam);
	sscanf(szParam, "%d", &progdefaults.HamlibWait);
	cntHamlibWait->value(progdefaults.HamlibWait);

	xcvr->getConf("write_delay", szParam);
	sscanf(szParam, "%d", &progdefaults.HamlibWriteDelay);
	cntHamlibWriteDelay->value(progdefaults.HamlibWriteDelay);

	xcvr->getConf("timeout", szParam);
	sscanf(szParam, "%d", &progdefaults.HamlibTimeout);
	cntHamlibTimeout->value(progdefaults.HamlibTimeout);

	xcvr->getConf("retry", szParam);
	sscanf(szParam, "%d", &progdefaults.HamlibRetries);
	cntHamlibRetries->value(progdefaults.HamlibRetries);

	xcvr->getConf("rts_state", szParam);
	if (strcmp(szParam, "ON") == 0)
		progdefaults.HamlibRTSplus = true;
	else
		progdefaults.HamlibRTSplus = false;
	chkHamlibRTSplus->value(progdefaults.HamlibRTSplus);

	xcvr->getConf("dtr_state", szParam);
	if (strcmp(szParam, "ON") == 0)
		progdefaults.HamlibDTRplus = true;
	else
		progdefaults.HamlibDTRplus = false;
	btnHamlibDTRplus->value(progdefaults.HamlibDTRplus);

	progdefaults.HamlibRTSCTSflow = false;
	progdefaults.HamlibXONXOFFflow = false;
	xcvr->getConf("serial_handshake", szParam);
	if (strcmp(szParam, "Hardware") == 0) progdefaults.HamlibRTSCTSflow = true;
	if (strcmp(szParam, "XONXOFF") == 0) progdefaults.HamlibXONXOFFflow = true;
	chkHamlibRTSCTSflow->value(progdefaults.HamlibRTSCTSflow);
	chkHamlibXONXOFFflow->value(progdefaults.HamlibXONXOFFflow);

	xcvr->getConf("stop_bits", szParam);
	progdefaults.HamRigStopbits = 2;
	if (strcmp(szParam, "1") == 0) progdefaults.HamRigStopbits = 1;
	valHamRigStopbits->value(progdefaults.HamRigStopbits);

	xcvr->close();
}

bool hamlib_init(bool bPtt)
{
	freq_t freq;
	rmode_t mode;
	pbwidth_t width;

	hamlib_ptt = bPtt;
	hamlib_closed = true;

#ifdef __CYGWIN__
	string port = progdefaults.HamRigDevice;
	com_to_tty(port);
#endif

	if (progdefaults.HamRigModel == 0) {
		LOG_ERROR("No such hamlib rig model");
		return false;
	}

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

		if (progdefaults.HamlibRTSplus)
			xcvr->setConf("rts_state", "ON");
		else
			xcvr->setConf("rts_state", "OFF");

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

	if (!xcvr->open()) {
		LOG_ERROR("open failed");
		xcvr->close();
		return false;
	}

	MilliSleep(500);
	need_freq = true;
	freq = xcvr->getFreq(need_freq);
	if (freq == 0 || !need_freq) {
		xcvr->close();
		LOG_ERROR("Rig not responding ");
		return false;
	}

	need_mode = true;
	mode = xcvr->getMode(need_mode, width);
	if (!need_mode) {
		LOG_ERROR("Get Mode ");
	}

	if (hamlib_ptt)
		LOG_INFO("trying PTT");
		if (!xcvr->setPTT(RIG_PTT_OFF)) {
			LOG_ERROR("Set Ptt ");
			hamlib_ptt = false;
		}

	if (!need_freq && !need_mode && !hamlib_ptt) {
		xcvr->close();
		LOG_INFO("Failed freq/mode/ptt");
		return false;
	}

	hamlib_freq = 0;
	hamlib_rmode = RIG_MODE_NONE;

	hamlib_exit = false;
	hamlib_bypass = false;

	if (pthread_create(&hamlib_thread, NULL, hamlib_loop, NULL) < 0) {
		LOG_ERROR("pthread_create failed");
		xcvr->close();
		return false;
	}

	init_Hamlib_RigDialog();

	hamlib_closed = false;
	return true;
}

void hamlib_close(void)
{
	ENSURE_THREAD(FLMAIN_TID);

	if (hamlib_closed || !xcvr->isOnLine())
		return;

	int count = 20;

	pthread_mutex_lock(&hamlib_mutex);
		hamlib_exit = true;
	pthread_mutex_unlock(&hamlib_mutex);

	while (!hamlib_closed) {
		MilliSleep(50);
		if (!count--) {
			LOG_ERROR("Hamlib thread not responding, forcing shutdown");
			xcvr->close();
			diediedie();
		}
	}

	xcvr->close();
	if (rigcontrol)
		rigcontrol->hide();
	wf->USB(true);
}

bool hamlib_active(void)
{
	return (xcvr->isOnLine());
}

void hamlib_set_ptt(int ptt)
{
	if (!xcvr->isOnLine())
		return;
	if (!hamlib_ptt)
		return;
	pthread_mutex_lock(&hamlib_mutex);
		hamlib_ptt = xcvr->setPTT(ptt ? RIG_PTT_ON : RIG_PTT_OFF);
	pthread_mutex_unlock(&hamlib_mutex);

	if (!hamlib_ptt)
		LOG_ERROR("%s", xcvr->error());
}

void hamlib_set_qsy(long long f, long long fmid)
{
	if (!xcvr->isOnLine())
		return;

	double fdbl = f;
	hamlib_qsy = false;

	bool qsyerr = false;

	pthread_mutex_lock(&hamlib_mutex);
		qsyerr = xcvr->setFreq(fdbl);
	pthread_mutex_unlock(&hamlib_mutex);

	if (qsyerr)
		LOG_ERROR("%s", xcvr->error());

	if (active_modem->freqlocked()) {
		active_modem->set_freqlock(false);
		active_modem->set_freq((int)fmid);
		active_modem->set_freqlock(true);
	} else
		active_modem->set_freq((int)fmid);

	wf->rfcarrier(f);
	wf->movetocenter();

}

int hamlib_setfreq(long f)
{
	if (!xcvr->isOnLine())
		return -1;

	int retval;
	pthread_mutex_lock(&hamlib_mutex);
		retval = xcvr->setFreq(f);
	pthread_mutex_unlock(&hamlib_mutex);

	if (!retval) {
		LOG_ERROR("%s", xcvr->error());
		return -1;
	}

	return 1;
}

int hamlib_setmode(rmode_t m)
{
	if (!need_mode)
		return -1;

	if (!xcvr->isOnLine())
		return -1;

	bool setok = true;
	pthread_mutex_lock(&hamlib_mutex);
		hamlib_rmode = xcvr->getMode(setok, hamlib_pbwidth);
		if (setok) setok = xcvr->setMode(m, hamlib_pbwidth);
	hamlib_rmode = m;
	pthread_mutex_unlock(&hamlib_mutex);

	if (!setok) {
		LOG_ERROR("%s", xcvr->error());
		return -1;
	}

	return 1;
}

int hamlib_setwidth(pbwidth_t w)
{
	if (!xcvr->isOnLine())
		return -1;

	bool setok = true;
	pthread_mutex_lock(&hamlib_mutex);
		hamlib_rmode = xcvr->getMode(setok, hamlib_pbwidth);
		if (setok) setok = xcvr->setMode(hamlib_rmode, w);
		hamlib_pbwidth = w;
	pthread_mutex_unlock(&hamlib_mutex);

	if (!setok) {
		LOG_ERROR("%s", xcvr->error());
		return -1;
	}

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
				freq_t f = xcvr->getFreq(freqok);
				if (freqok) {
					freq = (long int) f;
					freqok = (freq > 0);
				} else
					LOG_ERROR("%s", xcvr->error());
			}

			if (need_mode && hamlib_rmode == numode) {
				numode = xcvr->getMode(modeok, hamlib_pbwidth);
				if (!modeok)
					LOG_ERROR("%s", xcvr->error());
			}

		pthread_mutex_unlock(&hamlib_mutex);

		if (hamlib_exit)
			break;
		if (hamlib_bypass)
			continue;

		if (freqok && (freq != hamlib_freq)) {
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
