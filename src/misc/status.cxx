#include <iostream>
#include <fstream>

#include "status.h"
#include "configuration.h"

#include "modem.h"
#include "psk.h"
#include "cw.h"
#include "mfsk.h"
#include "rtty.h"
#include "olivia.h"
#include "dominoex.h"
#include "feld.h"
#include "throb.h"
#include "wwv.h"
#include "analysis.h"

extern void startup_modem(modem *m);

status progStatus = {
	MODE_BPSK31		// trx_mode	lastmode;
};

	
void status::saveModeState(trx_mode m)
{
	progStatus.lastmode = m;
	string deffname = HomeDir;
	deffname.append("fldigi.status");
	ofstream deffile(deffname.c_str(), ios::out);
	deffile << (int)lastmode << endl;
	deffile.close();
}

void status::readLastState()
{
	int iMode;
	string deffname = HomeDir;
	deffname.append("fldigi.status");
	ifstream deffile(deffname.c_str(), ios::in);
	if (deffile) {
		deffile >> iMode;
		deffile.close();
		lastmode = (trx_mode)iMode;
	}
}

void status::initLastState()
{
	switch (lastmode) {
		case MODE_CW : 			initCW(); break;
		case MODE_MFSK8 :		initMFSK8(); break;
		case MODE_MFSK16 :		initMFSK16(); break;
		case MODE_PSK63 :		initPSK63(); break;
		case MODE_PSK125 :		initPSK125(); break;
		case MODE_QPSK31 :		initQPSK31(); break;
		case MODE_QPSK63 :		initQPSK63(); break;
		case MODE_QPSK125 :		initQPSK125(); break;
		case MODE_RTTY :		initRTTY(); break;
		case MODE_OLIVIA :		initOLIVIA(); break;
		case MODE_DOMINOEX4 :	initDOMINOEX4(); break;
		case MODE_DOMINOEX5 :	initDOMINOEX5(); break;
		case MODE_DOMINOEX8 : 	initDOMINOEX8(); break;
		case MODE_DOMINOEX11 :	initDOMINOEX11(); break;
		case MODE_DOMINOEX16 :	initDOMINOEX16(); break;
		case MODE_DOMINOEX22 :	initDOMINOEX22(); break;
		case MODE_FELDHELL :	initFELDHELL(); break;
		case MODE_FSKHELL :		initFSKHELL(); break;
		case MODE_FSKH105 :		initFSKHELL105(); break;
		case MODE_THROB1 :		initTHROB1(); break;
		case MODE_THROB2 :		initTHROB2(); break;
		case MODE_THROB4 :		initTHROB4(); break;
		case MODE_THROBX1 :		initTHROBX1(); break;
		case MODE_THROBX2 :		initTHROBX2(); break;
		case MODE_THROBX4 :  	initTHROBX4(); break;
		case MODE_WWV :			initWWV(); break;
		case MODE_ANALYSIS: 	initANALYSIS(); break;
		case MODE_BPSK31 : 
		default: 				initPSK31();
	}
	while (!active_modem) MilliSleep(50);
	if (lastmode == MODE_CW)
		active_modem->set_freq(progdefaults.CWsweetspot);
	else if (lastmode == MODE_RTTY)
		active_modem->set_freq(progdefaults.RTTYsweetspot);
	else 
		active_modem->set_freq(progdefaults.PSKsweetspot);
}
