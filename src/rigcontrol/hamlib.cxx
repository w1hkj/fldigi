//
// hamlib.cxx  --  Hamlib (rig control) interface for fldigi
#include "configuration.h"
#include "Config.h"
#include <FL/fl_ask.H>

#include "rigclass.h"

#include "threads.h"
#include "misc.h"

#include "fl_digi.h"
#include "main.h"
#include "misc.h"

#include "rigsupport.h"
#include "rigdialog.h"

using namespace std;

static Fl_Mutex		hamlib_mutex = PTHREAD_MUTEX_INITIALIZER;
static Fl_Thread	hamlib_thread;

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

static int dummy = 0;

static void *hamlib_loop(void *args);

void show_error(const char * a, const char * b)
{
	string msg = a;
	msg.append(": ");
	msg.append(b);
	put_status((char*)msg.c_str());
}

bool hamlib_init(bool bPtt)
{
	rig_model_t model;
	freq_t freq;
	rmode_t mode;
	pbwidth_t width;

	string	port, spd;
	
	hamlib_ptt = bPtt;

	port = progdefaults.HamRigDevice;
	spd = progdefaults.strBaudRate();

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
		return -1;
	}

	try {
		model = (*prig)->rig_model;
		xcvr->init(model);
		xcvr->setConf("rig_pathname", port.c_str());
		xcvr->setConf("serial_speed", spd.c_str());
		xcvr->open();
	}
	catch (RigException Ex) {
		show_error("Init", Ex.message);
		return -1;
	}

	MilliSleep(100);

	try {
		need_freq = true;
		freq = xcvr->getFreq();
	}
	catch (RigException Ex) {
		show_error("Get Freq",Ex.message);
		need_freq = false;
	}
	try {
		need_mode = true;
		mode = xcvr->getMode(width);
	}
	catch (RigException Ex) {
		show_error("Get Mode", Ex.message);
		need_mode = false;
	}
	try {
		if (hamlib_ptt == true)
		xcvr->setPTT(RIG_PTT_OFF);
	}
	catch (RigException Ex) {
		show_error("Set Ptt", Ex.message);
		hamlib_ptt = false;
	}

	if (need_freq == false && need_mode == false && hamlib_ptt == false ) {
		xcvr->close();
		return false;
	}

	hamlib_freq = 0;
	hamlib_rmode = RIG_MODE_USB;

	if (fl_create_thread(hamlib_thread, hamlib_loop, &dummy) < 0) {
		show_error("Hamlib init:", "pthread_create failed");
		xcvr->close();
		return false;
	} 
//	std::cout << "Hamlib on" << std::endl; fflush(stdout);

	init_Hamlib_RigDialog();
	
	hamlib_closed = false;
	hamlib_exit = false;
	return true;
}

void hamlib_close(void)
{
	int count = 100;
	if (xcvr->isOnLine() == false || hamlib_closed == true)
		return;

	hamlib_exit = true;

//	std::cout << "Waiting for hamlib to exit "; fflush(stdout);
	while (hamlib_closed == false) {
//		std::cout << "."; fflush(stdout);
		MilliSleep(50);
		count--;
		if (!count) {
			std::cout << "\nHamlib stuck\n"; fflush(stdout);
			exit(0);
		}
	}

	xcvr->close();
//	std::cout << "\nexit OK" << std::endl; fflush(stdout);
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
	fl_lock(&hamlib_mutex);
		try {
			xcvr->setPTT(ptt ? RIG_PTT_ON : RIG_PTT_OFF);
			hamlib_bypass = ptt;
		}
		catch (RigException Ex) {
			show_error("Rig Ptt", Ex.message);
			hamlib_ptt = false;
		}
	fl_unlock(&hamlib_mutex);
}

void hamlib_set_qsy(void)
{
	long int fmid;
	long int f;
	if (xcvr->isOnLine() == false) 
		return;
	fl_lock(&hamlib_mutex);
// this is the sweet spot frequency that is the center of the PBF in the rig
		if (active_modem->get_mode() == MODE_CW)
			fmid = (long int)progdefaults.CWsweetspot;
		else if (active_modem->get_mode() == MODE_RTTY)
			fmid = (long int)progdefaults.RTTYsweetspot;
		else
			fmid = (long int)progdefaults.PSKsweetspot;
		f = wf->rfcarrier();
		if (wf->USB())
			f += (wf->carrier() - fmid);
		else
			f -= (wf->carrier() - fmid);
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
		catch (RigException Ex) {
			show_error("QSY", Ex.message);
			hamlib_passes = 0;
		}
	fl_unlock(&hamlib_mutex);
}

int hamlib_setfreq(long f)
{
	if (xcvr->isOnLine() == false)
		return -1;
	hamlib_freq = f;
	fl_lock(&hamlib_mutex);
		try {
			xcvr->setFreq(hamlib_freq);
			wf->rfcarrier(hamlib_freq);
		}
		catch (RigException Ex) {
		show_error("SetFreq", Ex.message);
			hamlib_passes = 0;
		}
	fl_unlock(&hamlib_mutex);
	return 1;
}

int hamlib_setmode(rmode_t m)
{
	if (xcvr->isOnLine() == false)
		return -1;
	fl_lock(&hamlib_mutex);
		try {
			hamlib_rmode = xcvr->getMode(hamlib_pbwidth);
			xcvr->setMode(m, hamlib_pbwidth);
			hamlib_rmode = m;
		}
		catch (RigException Ex) {
		show_error("Set Mode", Ex.message);
			hamlib_passes = 0;
		}
	fl_unlock(&hamlib_mutex);
	return 1;
}

int hamlib_setwidth(pbwidth_t w)
{
	if (xcvr->isOnLine() == false)
		return -1;
	fl_lock(&hamlib_mutex);
		try {
			hamlib_rmode = xcvr->getMode(hamlib_pbwidth);
			xcvr->setMode(hamlib_rmode, w);
			hamlib_pbwidth = w;
		}
		catch (RigException Ex) {
			show_error("Set Width", Ex.message);
			hamlib_passes = 0;
		}
	fl_unlock(&hamlib_mutex);
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
	long int freq = 0L;
	rmode_t  numode = RIG_MODE_USB;
	bool freqok = false, modeok = false;
	
	for (;;) {
		MilliSleep(100);
		if (hamlib_exit)
			break;
		if (hamlib_bypass)
			continue;
// hamlib locked while accessing hamlib serial i/o
		fl_lock (&hamlib_mutex);
		
		if (need_freq) {
			freq_t f;
			try {
				f = xcvr->getFreq();
				freq = (long int) f;
				freqok = true;
			}
			catch (RigException Ex) {
				show_error("Hamlib loop Get Freq", Ex.message);
				freqok = false;
			}
		}
		if (need_mode) {
			try {
				numode = xcvr->getMode(hamlib_pbwidth);
				modeok = true;
			}
			catch (RigException Ex) {
				show_error("Hamlib loop Get Mode", Ex.message);
				modeok = false;
			}
		}
		fl_unlock (&hamlib_mutex);

		if (hamlib_exit)
			break;
		if (hamlib_bypass)
			continue;

		if (freqok && freq && (freq != hamlib_freq)) {
			hamlib_freq = freq;
			Fl::lock();
				FreqDisp->value(hamlib_freq);
			Fl::unlock();
			wf->rfcarrier(hamlib_freq);
		}
		
		if (modeok && (hamlib_rmode != numode)) {
			hamlib_rmode = numode;
			selMode(hamlib_rmode);
			if (progdefaults.RTTY_USB == true && 
				hamlib_rmode == RIG_MODE_RTTYR)
				wf->USB(false);
			else if (progdefaults.RTTY_USB == false && 
				hamlib_rmode == RIG_MODE_RTTY)
				wf->USB(false);
			else if (hamlib_rmode == RIG_MODE_LSB ||
					hamlib_rmode == RIG_MODE_CWR ||	
					hamlib_rmode == RIG_MODE_PKTLSB ||
					hamlib_rmode == RIG_MODE_ECSSLSB)
				wf->USB(false);
			else
				wf->USB(true);
		}
		
		if (hamlib_exit)
			break;
	}
	hamlib_closed = true;
	return NULL;
}


