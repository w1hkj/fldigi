//
// hamlib.cxx  --  Hamlib (rig control) interface for fldigi
#include <config.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <string>

#include "configuration.h"
#include "confdialog.h"
#include <FL/fl_ask.H>

#include "rigclass.h"

#include "threads.h"
#include "misc.h"

#include "fl_digi.h"
#include "main.h"
#include "misc.h"

#include "rigsupport.h"
#include "rigdialog.h"

#include "stacktrace.h"
#include "re.h"
#include "debug.h"

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
static 	int hamlib_passes = 20;

static long int hamlib_freq;
static rmode_t hamlib_rmode = RIG_MODE_USB;
static pbwidth_t hamlib_pbwidth = 3000;

static void *hamlib_loop(void *args);

void show_error(const char* msg1, const char* msg2 = 0)
{
	string error = msg1;
	if (msg2)
		error.append(": ").append(msg2);
	put_status(error.c_str(), 10.0);
	LOG_ERROR("%s", error.c_str());
}

#ifdef __CYGWIN__
// convert COMx to /dev/ttySy with y = x - 1
static void adjust_port(string& port)
{
	re_t re("com([0-9]+)", REG_EXTENDED | REG_ICASE);
	const char* s;
	if (!(re.match(port.c_str()) && (s = re.submatch(1))))
		return;
	stringstream ss;
	int n;
	ss << s;
	ss >> n;
	if (--n < 0)
		n = 0;
	ss.clear(); ss.str("");
	ss << "/dev/ttyS" << n;
	ss.seekp(0);
	port = ss.str();
}
#endif

bool hamlib_init(bool bPtt)
{
	rig_model_t model;
	freq_t freq;
	rmode_t mode;
	pbwidth_t width;

	string	port, spd;
	
	hamlib_ptt = bPtt;
	hamlib_closed = true;

	port = progdefaults.HamRigDevice;
	spd = progdefaults.strBaudRate();

#ifdef __CYGWIN__
	adjust_port(port);
#endif

	list<string>::iterator pstr = (xcvr->rignames).begin();
	list< const struct rig_caps *>::iterator prig = (xcvr->riglist).begin();

	string sel = cboHamlibRig->value();

	while (pstr != (xcvr->rignames).end()) {
		if ( sel == *pstr )
			break;
		++pstr;
		++prig;
	}
	if (pstr == (xcvr->rignames).end()) {
		fl_message("Rig not in list");
		return false;
	}

	try {
		model = (*prig)->rig_model;
		xcvr->init(model);
		xcvr->setConf("rig_pathname", port.c_str());
		xcvr->setConf("serial_speed", spd.c_str());
		if (progdefaults.RTSplus)
			xcvr->setConf("rts_state", "ON");
		if (progdefaults.DTRplus)
			xcvr->setConf("dtr_state", "ON");
		xcvr->open();
	}
	catch (const RigException& Ex) {
		show_error(__func__, Ex.what());
		xcvr->close();
		return false;
	}

	MilliSleep(200);

	try {
		need_freq = true;
		freq = xcvr->getFreq();
		if (freq == 0) {
			xcvr->close();
			show_error(__func__, "Rig not responding");
			return false;
		}
	}
	catch (const RigException& Ex) {
		show_error("Get Freq", Ex.what());
		need_freq = false;
	}
	try {
		need_mode = true;
		mode = xcvr->getMode(width);
	}
	catch (const RigException& Ex) {
		show_error("Get Mode", Ex.what());
		need_mode = false;
	}
	try {
		if (hamlib_ptt == true)
		xcvr->setPTT(RIG_PTT_OFF);
	}
	catch (const RigException& Ex) {
		show_error("Set Ptt", Ex.what());
		hamlib_ptt = false;
	}

	if (need_freq == false && need_mode == false && hamlib_ptt == false ) {
		xcvr->close();
		return false;
	}

	hamlib_freq = 0;
	hamlib_rmode = RIG_MODE_NONE;//RIG_MODE_USB;

	hamlib_exit = false;
	hamlib_bypass = false;
	
	if (pthread_create(&hamlib_thread, NULL, hamlib_loop, NULL) < 0) {
		show_error(__func__, "pthread_create failed");
		xcvr->close();
		return false;
	} 

	init_Hamlib_RigDialog();
	
	hamlib_closed = false;
	return true;
}

void hamlib_close(void)
{
	if (hamlib_closed || !xcvr->isOnLine())
		return;

	hamlib_exit = true;
	int count = 20;
	while (!hamlib_closed) {
		MilliSleep(50);
		if (!count--) {
			show_error(__func__, "Hamlib stuck, transceiver on fire");
			xcvr->close();
			diediedie();
		}
	}
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

void hamlib_set_qsy(long long f, long long fmid)
{
	if (xcvr->isOnLine() == false) 
		return;
	pthread_mutex_lock(&hamlib_mutex);
	double fdbl = f;
	hamlib_qsy = false;
	try {
		xcvr->setFreq(fdbl);
		if (active_modem->freqlocked() == true) {
			active_modem->set_freqlock(false);
			active_modem->set_freq((int)fmid);
			active_modem->set_freqlock(true);
		} else
			active_modem->set_freq((int)fmid);
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
			xcvr->setFreq(f);
			wf->rfcarrier(f);//(hamlib_freq);
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
			FreqDisp->value(hamlib_freq);
			wf->rfcarrier(hamlib_freq);
		}
		
		if (modeok && (hamlib_rmode != numode)) {
			hamlib_rmode = numode;
			selMode(hamlib_rmode);
			if (hamlib_rmode == RIG_MODE_LSB ||
					hamlib_rmode == RIG_MODE_CWR ||	
					hamlib_rmode == RIG_MODE_PKTLSB ||
					hamlib_rmode == RIG_MODE_ECSSLSB ||
					hamlib_rmode == RIG_MODE_RTTYR)
				wf->USB(false);
			else
				wf->USB(true);
		}
		
		if (hamlib_exit)
			break;
	}

	xcvr->close();
	hamlib_closed = true;

	if (rigcontrol)
		rigcontrol->hide();
	wf->USB(true);
	wf->rfcarrier(atoi(cboBand->value())*1000L);
	FL_LOCK();
	wf->setQSY(0);
	cboBand->show();
	btnSideband->show();
	activate_rig_menu_item(false);
	FL_UNLOCK();

	return NULL;
}


