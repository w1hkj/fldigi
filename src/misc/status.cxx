#include <config.h>

#include <iostream>
#include <fstream>

#include "status.h"
#include "configuration.h"
#include "waterfall.h"

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

#include "rigsupport.h"

extern void startup_modem(modem *m);

status progStatus = {
	(int)MODE_BPSK31,	// trx_mode	lastmode;
	0,					// int mainX;
	0,					// int mainY;
	WNOM,				// int mainW;
	HNOM,				// int mainH;
	Hrcvtxt,			// int RxTextHeight;
	false,				// bool rigShown;
	0,					// int rigX;
	0,					// int rigY;
	1000,				// int carrier;
	1,					// int mag;
	NORMAL,				// WFdisp::WFspeed
	0,					// reflevel
	-60					// ampspan
};

	
void status::saveModeState(trx_mode m)
{
	lastmode = (int)m;
}

void status::saveLastState()
{
	mainX = fl_digi_main->x();
	mainY = fl_digi_main->y();
	mainW = fl_digi_main->w();
	mainH = fl_digi_main->h();
	RxTextHeight = ReceiveText->h();
	rigShown = false;
	rigX = 0;
	rigY = 0;
	carrier = wf->Carrier();
	mag = wf->Mag();
	speed = wf->Speed();
	reflevel = progdefaults.wfRefLevel;
	ampspan = progdefaults.wfAmpSpan;
	
	if (rigcontrol)
		if (rigcontrol->visible()) {
			rigShown = rigcontrol->visible();
			rigX = rigcontrol->x();
			rigY = rigcontrol->y();
		}
	string deffname = HomeDir;
	deffname.append("fldigi.status");
	ofstream deffile(deffname.c_str(), ios::out);
	deffile << lastmode << endl;
	deffile << mainX << endl;
	deffile << mainY << endl;
	deffile << mainW << endl;
	deffile << mainH << endl;
	deffile << rigShown << endl;
	deffile << rigX << endl;
	deffile << rigY << endl;
	deffile << RxTextHeight << endl;
	deffile << carrier << endl;
	deffile << mag << endl;
	deffile << speed << endl;
	deffile << reflevel << endl;
	deffile << ampspan << endl;	
	deffile.close();
}

void status::initLastState()
{
	string deffname = HomeDir;
	deffname.append("fldigi.status");
	ifstream deffile(deffname.c_str(), ios::in);
	if (deffile) {
		deffile >> lastmode;
		deffile >> mainX;
		deffile >> mainY;
		deffile >> mainW;
		deffile >> mainH;
		deffile >> rigShown;
		deffile >> rigX;
		deffile >> rigY;
		deffile >> RxTextHeight;
		deffile >> carrier;
		deffile >> mag;
		deffile >> speed;
		deffile >> reflevel;
		deffile >> ampspan;
		deffile.close();
		progdefaults.wfRefLevel = reflevel;
		progdefaults.wfAmpSpan = ampspan;
	}
	init_modem((trx_mode)lastmode);

	while (!active_modem) MilliSleep(100);

 	wf->opmode();
	wf->Mag(mag);
	wf->Speed(speed);
	wf->setRefLevel();
	wf->setAmpSpan();
	wf->movetocenter();
	
//	if (IMAGE_WIDTH == DEFAULT_IMAGE_WIDTH && Hwfall == DEFAULT_HWFALL &&
//	    HNOM == DEFAULT_HNOM && WNOM == DEFAULT_WNOM) 
	{
		fl_digi_main->resize(mainX, mainY, mainW, mainH);

		int X, Y, W, H, Yx, Hx;
		X = ReceiveText->x();
		Y = ReceiveText->y();
		W = ReceiveText->w();
		H = ReceiveText->h();
		Yx = TransmitText->y();
		Hx = TransmitText->h();	

		ReceiveText->resize(X,Y,W,RxTextHeight);
		FHdisp->resize(X,Y,W,RxTextHeight);
		TransmitText->resize(X, Y + RxTextHeight, W, H + Hx - RxTextHeight);
	}

	if (rigShown == true) {
		if (!rigcontrol)
			createRigDialog();
		int rdW = rigcontrol->w();
		int rdH = rigcontrol->h();
		rigcontrol->resize(rigX, rigY, rdW, rdH);
		rigcontrol->show();
	}

}
