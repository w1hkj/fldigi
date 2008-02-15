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

static int ioctl_bits;

static int dummy = 0;

static void *hamlib_loop(void *args);

void show_error(const char * a, const char * b)
{
	string msg = a;
	msg.append(": ");
	msg.append(b);
	put_status((char*)msg.c_str());
	std::cout << msg.c_str() << std::endl; std::cout.flush();
}

bool hamlib_setRTSDTR()
{
	if (progdefaults.RTSplus == false && progdefaults.DTRplus == false)
		return true;
		
	int hamlibfd =  open(progdefaults.HamRigDevice.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);

	if (hamlibfd < 0) {
		show_error("Cannot open PTT device ", progdefaults.HamRigDevice.c_str());
		return false;
	}

	ioctl(hamlibfd, TIOCMGET, &ioctl_bits);

	if (progdefaults.RTSplus)
		ioctl_bits |= TIOCM_RTS;		// set RTS bit
	else
		ioctl_bits &= ~TIOCM_RTS;		// clear RTS bit
	if (progdefaults.DTRplus)
		ioctl_bits |= TIOCM_DTR;		// set DTR bit
	else
		ioctl_bits &= ~TIOCM_DTR;		// clear DTR bit
std::cout << "TIOCM_RTS " << TIOCM_RTS << ", RTSplus " << progdefaults.RTSplus;
std::cout << std::endl;
std::cout << "TIOCM_DTR " << TIOCM_DTR << ", DTRplus " << progdefaults.DTRplus;
std::cout << std::endl;
std::cout << ioctl_bits;
	ioctl(hamlibfd, TIOCMSET, &ioctl_bits);
	close(hamlibfd);
	
std::cout << "=> setRTSDTR() => " << ioctl_bits << std::endl; std::cout.flush();
	
	return true;
}

bool hamlib_init(bool bPtt)
{
	rig_model_t model;
	freq_t freq;
	rmode_t mode;
	pbwidth_t width;

	string	port, spd;
	
	hamlib_ptt = bPtt;
	hamlib_closed = true;
	
	if (progdefaults.HamRigDevice == "COM1")
		port = "/dev/ttyS0";
	else if (progdefaults.HamRigDevice == "COM2")
		port = "/dev/ttyS1";
	else if (progdefaults.HamRigDevice == "COM3")
		port = "/dev/ttyS2";
	else if (progdefaults.HamRigDevice == "COM4")
		port = "/dev/ttyS3";
	else
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

//	if (hamlib_setRTSDTR() == false) {
//		return -1;
//	}
	
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
	catch (RigException Ex) {
		show_error("Init", Ex.message);
		return -1;
	}

//	if (hamlib_setRTSDTR() == false)
//		return -1;
		
	MilliSleep(200);
//char temp[80];
//xcvr->getConf("dtr_state", temp);
//std::cout << "Hamlib DTR " << temp << std::endl; std::cout.flush();

	try {
		need_freq = true;
		freq = xcvr->getFreq();
		if (freq == 0) {
			xcvr->close();
			show_error("Transceiver not responding", "");
			return -1;
		}
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
	hamlib_rmode = RIG_MODE_NONE;//RIG_MODE_USB;

	if (fl_create_thread(hamlib_thread, hamlib_loop, &dummy) < 0) {
		std::cout << "Hamlib init:  pthread_create failed\n"; std::cout.flush();
		xcvr->close();
		return false;
	} 

	init_Hamlib_RigDialog();
	
	hamlib_closed = false;
	hamlib_exit = false;
	return true;
}

void hamlib_close(void)
{
	int count = 20;
	if (xcvr->isOnLine() == false)
		return;

	if (hamlib_closed == true)
		return;
		
	hamlib_exit = true;

	while (hamlib_closed == false) {
//		std::cout << "."; cout.flush();
		MilliSleep(50);
		count--;
		if (!count) {
			std::cout << "\nHamlib stuck\n"; cout.flush();
			xcvr->close();
			exit(0);
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

void hamlib_set_qsy(long long f, long long fmid)
{
	if (xcvr->isOnLine() == false) 
		return;
	fl_lock(&hamlib_mutex);
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
	fl_lock(&hamlib_mutex);
		try {
			xcvr->setFreq(f);
			wf->rfcarrier(f);//(hamlib_freq);
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
	SET_THREAD_ID(RIGCTL_TID);

	long int freq = 0L;
	rmode_t  numode = RIG_MODE_NONE;
	bool freqok = false, modeok = false;
	
	for (;;) {
loop:
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
				if (freq == 0) {
					fl_unlock (&hamlib_mutex);
					goto loop;
//					hamlib_exit = true;
				}
			}
			catch (RigException Ex) {
				show_error("No transceiver comms", "");
				freqok = false;
				hamlib_exit = true;
			}
		}
		if (hamlib_exit)
			break;
			
		if (need_mode && hamlib_rmode == numode) {
			try {
				numode = xcvr->getMode(hamlib_pbwidth);
				modeok = true;
			}
			catch (RigException Ex) {
				show_error("No transceiver comms", "");
				modeok = false;
				hamlib_exit = true;
			}
		}
		fl_unlock (&hamlib_mutex);

		if (hamlib_exit)
			break;
		if (hamlib_bypass)
			continue;

		if (freqok && freq && (freq != hamlib_freq)) {
			hamlib_freq = freq;
			FL_LOCK_D();
				FreqDisp->value(hamlib_freq);
			FL_UNLOCK_D();
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


