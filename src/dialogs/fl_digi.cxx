// ----------------------------------------------------------------------------
//
//	fl_digi.cxx
//
// Copyright (C) 2006
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.
//
// fldigi is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// ----------------------------------------------------------------------------

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include <stdlib.h>
#include <string>

#include "fl_digi.h"

#include <FL/fl_ask.H>
#include <FL/Fl_Pixmap.H>
#include <FL/Fl_Image.H>
#include <FL/Fl_Tile.H>
#include <FL/x.H>

#include "version.h"

#include "waterfall.h"
#include "raster.h"
#include "main.h"
#include "threads.h"
#include "trx.h"
#ifndef NOHAMLIB
	#include "hamlib.h"
#endif
#include "rigio.h"
#include "rigMEM.h"
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

#include "ascii.h"
#include "globals.h"
#include "misc.h"
//#include "help.h"

#include "Config.h"
#include "configuration.h"
#include "macros.h"
#include "macroedit.h"
#include "logger.h"
#include "qrzcall.h"

#include "combo.h"
#include "font_browser.h"
#include "fldigi-icon-48.xpm"

#include "status.h"

#include "rigsupport.h"

#include "qrunner.h"

Fl_Double_Window	*fl_digi_main=(Fl_Double_Window *)0;

cMixer mixer;

bool useCheckButtons = false;

Fl_Button			*btnTune = (Fl_Button *)0;
Fl_Tile_check				*TiledGroup = 0;
ReceiveWidget			*ReceiveText = 0;
TransmitWidget			*TransmitText = 0;
Fl_Text_Buffer		*rcvBuffer = (Fl_Text_Buffer *)0;
Fl_Text_Buffer		*xmtBuffer = (Fl_Text_Buffer *)0;
Raster				*FHdisp;
Fl_Box				*StatusBar = (Fl_Box *)0;
Fl_Box				*Status2 = (Fl_Box *)0;
Fl_Box				*Status1 = (Fl_Box *)0;
Fl_Box				*WARNstatus = (Fl_Box *)0;
Fl_Button			*MODEstatus = (Fl_Button *)0;
Fl_Button 			*btnMacro[10];
Fl_Button			*btnAltMacros;
Fl_Light_Button		*afconoff;
Fl_Light_Button		*sqlonoff;
Fl_Check_Button		*chk_afconoff;
Fl_Check_Button		*chk_sqlonoff;
Fl_Input			*inpFreq;
Fl_ComboBox			*cboBand;
Fl_Button			*btnSideband;
Fl_Input			*inpTime;
Fl_Input			*inpCall;
Fl_Input			*inpName;
Fl_Input			*inpRstIn;
Fl_Input			*inpRstOut;
Fl_Input			*inpQth;
Fl_Input			*inpLoc;
Fl_Input			*inpNotes;
Fl_Input			*inpAZ;	// WA5ZNU
Fl_Button			*qsoTime;
Fl_Button			*qsoClear;
Fl_Button			*qsoSave;
Fl_Button			*btnMacroTimer;
Fl_Button			*btnQRZ;
Fl_Slider			*valRcvMixer;
Fl_Slider			*valXmtMixer;

bool				altMacros = false;
bool				bSaveFreqList = false;
string				strMacroName[10];


waterfall			*wf = (waterfall *)0;
Digiscope			*digiscope = (Digiscope *)0;
Fl_Slider			*sldrSquelch = (Fl_Slider *)0;
Fl_Progress			*pgrsSquelch = (Fl_Progress *)0;

Fl_RGB_Image		*feld_image = 0;

Pixmap				fldigi_icon_pixmap;


int IMAGE_WIDTH = DEFAULT_IMAGE_WIDTH;
int Hwfall = DEFAULT_HWFALL;
int HNOM = DEFAULT_HNOM;
int WNOM = DEFAULT_WNOM;


void clearStatus()
{
	clear_StatusMessages();
}

void startup_modem(modem *m)
{
	trx_start_modem(m);

	restoreFocus();

	FL_LOCK_D();
	if (m == feld_modem ||
		m == feld_FMmodem ||
		m == feld_FM105modem ) {
		ReceiveText->Hide();
		FHdisp->show();
	} else {
		ReceiveText->Show();
		FHdisp->hide();
	}
	FL_UNLOCK_D();
	FL_AWAKE_D();

}

void cb_mnuOpenMacro(Fl_Menu_*, void*) {
	macros.openMacroFile();
	restoreFocus();
}

void cb_mnuSaveMacro(Fl_Menu_*, void*) {
	macros.saveMacroFile();
	restoreFocus();
}

bool logging = false;
void cb_mnuLogFile(Fl_Menu_ *, void *) {
	logging = !logging;
	restoreFocus();
}

void clean_exit() {
	if (progdefaults.changed == true) {
		if (fl_choice("Configuration changed, Save", "No", "Yes", 0) == 1)
			progdefaults.saveDefaults();
	}
	if (macros.changed == true) {
		if (fl_choice("Macros changed, Save", "No", "Yes", 0) == 1)
			macros.saveMacroFile();
	}
	if (Maillogfile)
		Maillogfile->log_to_file_stop();
	if (logfile)
		logfile->log_to_file_stop();
	
	if (bSaveFreqList)
		saveFreqList();
		
	progStatus.saveLastState();

#ifndef NOHAMLIB
	hamlib_close();
#endif
	rigCAT_close();
	rigMEM_close();

	mixer.closeMixer();
	active_modem->set_stopflag(true);
	while (trx_state != STATE_RX)
		MilliSleep(100);
		
//	fl_lock (&trx_mutex);
//	if (active_modem) {
//		active_modem->shutdown();
//		MilliSleep(100);
//		delete active_modem;
//	}
//	active_modem = (modem *) 0;
//	fl_unlock (&trx_mutex);

//#ifndef NOHAMLIB	
//	delete xcvr;
//#endif
//	delete push2talk;
	
	exit(0);
}

void cb_E(Fl_Menu_*, void*) {
	clean_exit();
}

void cb_wMain( Fl_Widget *, void * ) 
{
	if (Fl::event_key(FL_Escape)) {
		TransmitText->clear();
		active_modem->set_stopflag(true);
	} else
		clean_exit();
}

void initMFSK8()
{
	clearStatus();
	if (!mfsk8_modem)
		mfsk8_modem = new mfsk(MODE_MFSK8);
	startup_modem (mfsk8_modem);
	progStatus.saveModeState(MODE_MFSK8);
}

void cb_mnuMFSK8(Fl_Menu_*, void*) {
	initMFSK8();
}

void initMFSK16()
{
	clearStatus();
	if (!mfsk16_modem) 
		mfsk16_modem = new mfsk(MODE_MFSK16);
	startup_modem (mfsk16_modem);
	progStatus.saveModeState(MODE_MFSK16);
}

void cb_mnuMFSK16(Fl_Menu_*, void*) {
	initMFSK16();
}

void initPSK31()
{
	clearStatus();
	if (!psk31_modem)
		psk31_modem = new psk(MODE_BPSK31);
	startup_modem (psk31_modem);
	progStatus.saveModeState(MODE_BPSK31);
}

void cb_mnuPSK31(Fl_Menu_*, void*) {
	initPSK31();
}

void initPSK63()
{
	clearStatus();
	if(!psk63_modem)
		psk63_modem = new psk(MODE_PSK63);
	startup_modem (psk63_modem);
	progStatus.saveModeState(MODE_PSK63);
}

void cb_mnuPSK63(Fl_Menu_*, void*) {
	initPSK63();
}

void initPSK125()
{
	clearStatus();
	if(!psk125_modem)
		psk125_modem = new psk(MODE_PSK125);
	startup_modem (psk125_modem);
	progStatus.saveModeState(MODE_PSK125);
}

void cb_mnuPSK125(Fl_Menu_*, void*) {
	initPSK125();
}

void initPSK250()
{
	clearStatus();
	if(!psk250_modem)
		psk250_modem = new psk(MODE_PSK250);
	startup_modem (psk250_modem);
	progStatus.saveModeState(MODE_PSK250);
}

void cb_mnuPSK250(Fl_Menu_*, void*) {
	initPSK250();
}

void initQPSK31()
{
	clearStatus();
	if (!qpsk31_modem)
		qpsk31_modem = new psk(MODE_QPSK31);
	startup_modem (qpsk31_modem);
	progStatus.saveModeState(MODE_QPSK31);
}

void cb_mnuQPSK31(Fl_Menu_*, void*) {
	initQPSK31();
}

void initQPSK63()
{
	clearStatus();
	if (!qpsk63_modem)
		qpsk63_modem = new psk(MODE_QPSK63);
	startup_modem (qpsk63_modem);
	progStatus.saveModeState(MODE_QPSK63);
}

void cb_mnuQPSK63(Fl_Menu_*, void*) {
	initQPSK63();
}

void initQPSK125()
{
	clearStatus();
	if (!qpsk125_modem)
		qpsk125_modem = new psk(MODE_QPSK125);
	startup_modem (qpsk125_modem);
	progStatus.saveModeState(MODE_QPSK125);
}

void cb_mnuQPSK125(Fl_Menu_*, void*) {
	initQPSK125();
}

void initQPSK250()
{
	clearStatus();
	if (!qpsk250_modem)
		qpsk250_modem = new psk(MODE_QPSK250);
	startup_modem (qpsk250_modem);
	progStatus.saveModeState(MODE_QPSK250);
}

void cb_mnuQPSK250(Fl_Menu_*, void*) {
	initQPSK250();
}

void initCW()
{
	clearStatus();
	if (!cw_modem)
		cw_modem = new cw();
	startup_modem (cw_modem);
	progStatus.saveModeState(MODE_CW);
}

void cb_mnuCW(Fl_Menu_*, void*) {
	initCW();
}

void initRTTY()
{
	clearStatus();
	if (!rtty_modem)
		rtty_modem = new rtty(MODE_RTTY);
	startup_modem (rtty_modem);
	progStatus.saveModeState(MODE_RTTY);
}

void cb_mnuRTTY(Fl_Menu_*, void*) {
	initRTTY();
}

void initOLIVIA()
{
	clearStatus();
	if (!olivia_modem)
		olivia_modem = new olivia();
	startup_modem (olivia_modem);
	progStatus.saveModeState(MODE_OLIVIA);
}

void cb_mnuOlivia(Fl_Menu_*, void*) {
	initOLIVIA();
}

void initDOMINOEX4()
{
	clearStatus();
	if (!dominoex4_modem) {
		dominoex4_modem = new dominoex(MODE_DOMINOEX4);
	}
	startup_modem (dominoex4_modem);
	progStatus.saveModeState(MODE_DOMINOEX4);
}

void cb_mnuDOMINOEX4(Fl_Menu_ *, void *) {
	initDOMINOEX4();
}

void initDOMINOEX5()
{
	clearStatus();
	if (!dominoex5_modem) {
		dominoex5_modem = new dominoex(MODE_DOMINOEX5);
	}
	startup_modem (dominoex5_modem);
	progStatus.saveModeState(MODE_DOMINOEX5);
}

void cb_mnuDOMINOEX5(Fl_Menu_ *, void *) {
	initDOMINOEX5();
}

void initDOMINOEX8()
{
	clearStatus();
	if (!dominoex8_modem)
		dominoex8_modem = new dominoex(MODE_DOMINOEX8);
	startup_modem (dominoex8_modem);
	progStatus.saveModeState(MODE_DOMINOEX8);
}

void cb_mnuDOMINOEX8(Fl_Menu_ *, void *) {
	initDOMINOEX8();
}

void initDOMINOEX11()
{
	clearStatus();
	if (!dominoex11_modem)
		dominoex11_modem = new dominoex(MODE_DOMINOEX11);
	startup_modem (dominoex11_modem);
	progStatus.saveModeState(MODE_DOMINOEX11);
}

void cb_mnuDOMINOEX11(Fl_Menu_ *, void *) {
	initDOMINOEX11();
}

void initDOMINOEX16()
{
	clearStatus();
	if (!dominoex16_modem)
		dominoex16_modem = new dominoex(MODE_DOMINOEX16);
	startup_modem (dominoex16_modem);
	progStatus.saveModeState(MODE_DOMINOEX16);
}

void cb_mnuDOMINOEX16(Fl_Menu_ *, void *) {
	initDOMINOEX16();
}

void initDOMINOEX22()
{
	clearStatus();
	if (!dominoex22_modem)
		dominoex22_modem = new dominoex(MODE_DOMINOEX22);
	startup_modem (dominoex22_modem);
	progStatus.saveModeState(MODE_DOMINOEX22);
}

void cb_mnuDOMINOEX22(Fl_Menu_ *, void *) {
	initDOMINOEX22();
}

void initFELDHELL()
{
	clearStatus();
	FHdisp->clear();
	if (!feld_modem)
		feld_modem = new feld(MODE_FELDHELL);
	startup_modem (feld_modem);
	progStatus.saveModeState(MODE_FELDHELL);
}

void cb_mnuFELDHELL(Fl_Menu_ *, void *) {
	initFELDHELL();
}	

void initFSKHELL()
{
	clearStatus();
	FHdisp->clear();
	if (!feld_FMmodem)
		feld_FMmodem = new feld(MODE_FSKHELL);
	startup_modem (feld_FMmodem);
	progStatus.saveModeState(MODE_FSKHELL);
}

void cb_mnuFSKHELL(Fl_Menu_ *, void *) {
	initFSKHELL();
}	

void initFSKHELL105()
{
	clearStatus();
	FHdisp->clear();
	if (!feld_FM105modem)
		feld_FM105modem = new feld(MODE_FSKH105);
	startup_modem (feld_FM105modem);
	progStatus.saveModeState(MODE_FSKH105);
}

void cb_mnuFSKHELL105(Fl_Menu_ *, void *) {
	initFSKHELL105();
}	

//void cb_mnuCMTHELL(Fl_Menu_ *, void *) {
//	clearStatus();
//	FHdisp->clear();
//	if (!feld_CMTmodem)
//		feld_CMTmodem = new feld(MODE_FMCMT);
//	startup_modem (feld_CMTmodem);
//}	

void initTHROB1()
{
	clearStatus();
	if (!throb1_modem)
		throb1_modem = new throb(MODE_THROB1);
	startup_modem (throb1_modem);
	progStatus.saveModeState(MODE_THROB1);
}

void cb_mnuTHROB1(Fl_Menu_ *, void *) {
	initTHROB1();
}

void initTHROB2()
{
	clearStatus();
	if (!throb2_modem)
		throb2_modem = new throb(MODE_THROB2);
	startup_modem (throb2_modem);
	progStatus.saveModeState(MODE_THROB2);
}

void cb_mnuTHROB2(Fl_Menu_ *, void *) {
	initTHROB2();
}

void initTHROB4()
{
	clearStatus();
	if (!throb4_modem)
		throb4_modem = new throb(MODE_THROB4);
	startup_modem (throb4_modem);
	progStatus.saveModeState(MODE_THROB4);
}

void cb_mnuTHROB4(Fl_Menu_ *, void *) {
	initTHROB4();
}

void initTHROBX1()
{
	clearStatus();
	if (!throbx1_modem)
		throbx1_modem = new throb(MODE_THROBX1);
	startup_modem (throbx1_modem);
	progStatus.saveModeState(MODE_THROBX1);
}

void cb_mnuTHROBX1(Fl_Menu_ *, void *) {
	initTHROBX1();
}

void initTHROBX2()
{
	clearStatus();
	if (!throbx2_modem)
		throbx2_modem = new throb(MODE_THROBX2);
	startup_modem (throbx2_modem);
	progStatus.saveModeState(MODE_THROBX2);
}

void cb_mnuTHROBX2(Fl_Menu_ *, void *) {
	initTHROBX2();
}

void initTHROBX4()
{
	clearStatus();
	if (!throbx4_modem)
		throbx4_modem = new throb(MODE_THROBX4);
	startup_modem (throbx4_modem);
	progStatus.saveModeState(MODE_THROBX4);
}

void cb_mnuTHROBX4(Fl_Menu_ *, void *) {
	initTHROBX4();
}

void initWWV()
{
	clearStatus();
	if (!wwv_modem)
		wwv_modem = new wwv();
	startup_modem (wwv_modem);
	progStatus.saveModeState(MODE_WWV);
}

void cb_mnuWWV(Fl_Menu_ *, void *) {
	initWWV();
}

void initANALYSIS()
{
	clearStatus();
	if (!anal_modem)
		anal_modem = new anal();
	startup_modem (anal_modem);
	progStatus.saveModeState(MODE_ANALYSIS);
}

void cb_mnuANALYSIS(Fl_Menu_ *, void *) {
	initANALYSIS();
}

void restoreFocus()
{
	FL_LOCK_D();
	TransmitText->take_focus();
	FL_UNLOCK_D();
	FL_AWAKE_D();
}

void macro_cb(Fl_Widget *w, void *v)
{
	int b = (int)(reinterpret_cast<long> (v));
	b += (altMacros ? 10 : 0);
	int mouse = Fl::event_button();
	if (mouse == 1 && !macros.text[b].empty())
		macros.execute(b);
	else if (mouse == 3)
		editMacro(b);
	restoreFocus();
}

void altmacro_cb(Fl_Widget *w, void *v)
{
	altMacros = !altMacros;
	FL_LOCK_D();
	for (int i = 0; i < 10; i++)
		btnMacro[i]->label(macros.name[i + (altMacros ? 10: 0)].c_str());
	FL_UNLOCK_D();
	restoreFocus();
}

void cb_mnuConfigOperator(Fl_Menu_*, void*) {
	progdefaults.loadDefaults();
	tabsConfigure->value(tabOperator);
	dlgConfig->show();
}

void cb_mnuConfigWaterfall(Fl_Menu_*, void*) {
	progdefaults.loadDefaults();
	tabsConfigure->value(tabWaterfall);
	dlgConfig->show();
}

void cb_mnuConfigRigCtrl(Fl_Menu_*, void*) {
	progdefaults.loadDefaults();
	tabsConfigure->value(tabRig);
	dlgConfig->show();
}

void cb_mnuConfigSoundCard(Fl_Menu_*, void*) {
	progdefaults.loadDefaults();
	tabsConfigure->value(tabSoundCard);
	dlgConfig->show();
}

void cb_mnuConfigModems(Fl_Menu_*, void*) {
	progdefaults.loadDefaults();
	tabsConfigure->value(tabModems);
	dlgConfig->show();
}

bool capval = false;
bool genval = false;
bool playval = false;
void cb_mnuCapture(Fl_Widget *w, void *d)
{
	Fl_Menu_Item *m = (Fl_Menu_Item *)(((Fl_Menu_*)w)->mvalue());
	if (!scard) return;
	if (playval || genval) {
		m->flags &= ~FL_MENU_VALUE;
		return;
	}
	capval = m->value();
	if(!scard->Capture(capval)) {
		m->flags &= ~FL_MENU_VALUE;
		capval = false;
	}
}

void cb_mnuGenerate(Fl_Widget *w, void *d)
{
	Fl_Menu_Item *m = (Fl_Menu_Item *)(((Fl_Menu_*)w)->mvalue());
	if (!scard) return;
	if (capval || playval) {
		m->flags &= ~FL_MENU_VALUE;
		return;
	}
	genval = m->value();
	if (!scard->Generate(genval)) {
		m->flags &= ~FL_MENU_VALUE;
		genval = false;
	}
}

void cb_mnuPlayback(Fl_Widget *w, void *d)
{
	Fl_Menu_Item *m = (Fl_Menu_Item *)(((Fl_Menu_*)w)->mvalue());
	if (!scard) return;
	if (capval || genval) {
		m->flags &= ~FL_MENU_VALUE;
		return;
	}
	playval = m->value();
	if(!scard->Playback(playval)) {
		m->flags &= ~FL_MENU_VALUE;
		playval = false;
	}
}


void cb_FontBrowser(Font_Browser*, void* v)
{
	Font_Browser *ft= (Font_Browser*)v;

	Fl_Font fnt = ft->fontNumber();
	int size = ft->fontSize();

	ReceiveText->setFont(fnt);
	ReceiveText->setFontSize(size);
	
	TransmitText->setFont(fnt);
	TransmitText->setFontSize(size);
	
	progdefaults.Fontnbr = (int)(fnt);
	progdefaults.FontSize = size;
	
	ft->hide();
}

void cb_mnuConfigFonts(Fl_Menu_*, void *) {
	static Font_Browser *b = (Font_Browser *)0;
	if (!b) {
		b = new Font_Browser;
		b->fontNumber((Fl_Font)progdefaults.Fontnbr);
		b->fontSize(progdefaults.FontSize);
//		b->fontColor(progdefaults.FontColor);
	}
	b->callback((Fl_Callback*)cb_FontBrowser, (void*)(b));
	b->show();
}

void cb_mnuSaveConfig(Fl_Menu_ *, void *) {
	progdefaults.saveDefaults();
	restoreFocus();
}

//void cb_mnuHelp(Fl_Menu_*,void*) {
//	show_help();
//	restoreFocus();
//}

void cb_mnuAbout(Fl_Menu_*,void*) {
	fl_message ("fldigi @@W1HKJ\n\nw1hkj@@w1hkj.com\n\nVersion - %s", FLDIGI_VERSION);
	restoreFocus();
}

void cbTune(Fl_Widget *w, void *) {
	Fl_Button *b = (Fl_Button *)w;
	if (active_modem == wwv_modem || active_modem == anal_modem) {
		b->value(0);
		return;
	}
	if (b->value() == 1) {
		b->labelcolor(FL_RED);
		fl_lock(&trx_mutex);
			trx_state = STATE_TUNE;
		fl_unlock(&trx_mutex);
	} else {
		b->labelcolor(FL_BLACK);
		fl_lock(&trx_mutex);
			trx_state = STATE_RX;
		fl_unlock(&trx_mutex);
	}
	restoreFocus();
}

void cb_mnuRig(Fl_Menu_ *, void *) {
	if (!rigcontrol)
		createRigDialog();
	rigcontrol->show();
}

void closeRigDialog() {
	rigcontrol->hide();
}

void cb_sldrSquelch(Fl_Slider* o, void*) {
	active_modem->set_squelch(o->value());
	progdefaults.sldrSquelchValue = o->value();
	restoreFocus();
}

char *zuluTime()
{
	struct tm *tm;
	time_t t;
	static char logtime[10];
	time(&t);
    tm = gmtime(&t);
	strftime(logtime, sizeof(logtime), "%H%M", tm);
	return logtime;
}

void qsoTime_cb(Fl_Widget *b, void *)
{
	FL_LOCK_D();
	inpTime->value(zuluTime());
	FL_UNLOCK_D();
	FL_AWAKE_D();
	restoreFocus();
}

void clearQSO()
{
	FL_LOCK_D();
		inpTime->value(zuluTime());
		inpCall->value("");
		inpName->value("");
		inpRstIn->value("");
		inpRstOut->value("");
		inpQth->value("");
		inpLoc->value("");
		inpAZ->value(""); // WA5ZNU
		inpNotes->value("");
	FL_UNLOCK_D();
}

void qsoClear_cb(Fl_Widget *b, void *)
{
	if (fl_choice ("Confirm Clear", "Cancel", "OK", NULL) == 1) {
		clearQSO();
		FL_AWAKE();
	}
	restoreFocus();
}

void qsoSave_cb(Fl_Widget *b, void *)
{
	submit_log();
	restoreFocus();
}

void cb_QRZ(Fl_Widget *b, void *)
{
	QRZquery();
}

void status_btn_right_click()
{
	progdefaults.loadDefaults();
	tabsConfigure->value(tabModems);
	switch (active_modem->get_mode()) {
		case MODE_CW : tabsModems->value(tabCW); break;
		case MODE_OLIVIA : tabsModems->value(tabOlivia); break;
		case MODE_RTTY: tabsModems->value(tabRTTY); break;
		case MODE_FELDHELL :
		case MODE_FSKHELL :
		case MODE_FSKH105 : tabsModems->value(tabFeld); break;
		case MODE_DOMINOEX4 :
		case MODE_DOMINOEX5 :
		case MODE_DOMINOEX8 :
		case MODE_DOMINOEX11 :
		case MODE_DOMINOEX16 :
		case MODE_DOMINOEX22 : tabsModems->value(tabDomEX); break;
		case MODE_BPSK31 :
		case MODE_QPSK31 :
		case MODE_PSK63 :
		case MODE_QPSK63 :
		case MODE_PSK125 :
		case MODE_QPSK125 :
		case MODE_PSK250 :
		case MODE_QPSK250 :
			tabsModems->value(tabPSK); break;
		case MODE_MFSK16:
		case MODE_MFSK8:
		case MODE_THROB1 :
		case MODE_THROB2 :
		case MODE_THROB4 :
		case MODE_THROBX1 :
		case MODE_THROBX2 :
		case MODE_THROBX4 :
		default :
			tabsModems->value(tabCW);
	}
	dlgConfig->show();
}

Fl_Menu_Item quick_change_psk[] = {
	{"psk 31", 0,  (Fl_Callback*)cb_mnuPSK31 },
	{"psk 63", 0,  (Fl_Callback*)cb_mnuPSK63 },
	{"psk 125", 0,  (Fl_Callback*)cb_mnuPSK125 },
	{"psk 250", 0,  (Fl_Callback*)cb_mnuPSK250 },
	{"No change", 0, 0 },
	{ 0 }
};

Fl_Menu_Item quick_change_qpsk[] = {
	{"qpsk 31", 0,  (Fl_Callback*)cb_mnuQPSK31 },
	{"qpsk 63", 0,  (Fl_Callback*)cb_mnuQPSK63 },
	{"qpsk 125", 0,  (Fl_Callback*)cb_mnuQPSK125 },
	{"qpsk 250", 0,  (Fl_Callback*)cb_mnuQPSK250, 0, FL_MENU_DIVIDER },
	{"No change", 0, 0 },
	{ 0 }
};

Fl_Menu_Item quick_change_mfsk[] = {
	{"mfsk 8", 0,  (Fl_Callback*)cb_mnuMFSK8 },
	{"mfsk 16", 0,  (Fl_Callback*)cb_mnuMFSK16, 0, FL_MENU_DIVIDER },
	{"No change", 0, 0 },
	{ 0 }
};

Fl_Menu_Item quick_change_domino[] = {
	{"dominoex 4", 0,  (Fl_Callback*)cb_mnuDOMINOEX4 },
	{"dominoex 5", 0,  (Fl_Callback*)cb_mnuDOMINOEX5 },
	{"dominoex 8", 0,  (Fl_Callback*)cb_mnuDOMINOEX8 },
	{"dominoex 11", 0,  (Fl_Callback*)cb_mnuDOMINOEX11 },
	{"dominoex 16", 0,  (Fl_Callback*)cb_mnuDOMINOEX16 },
	{"dominoex 22", 0,  (Fl_Callback*)cb_mnuDOMINOEX22, 0, FL_MENU_DIVIDER },
	{"No change", 0, 0 },
	{ 0 }
};

Fl_Menu_Item quick_change_feld[] = {
	{"Feld-Hell", 0,  (Fl_Callback*)cb_mnuFELDHELL, },
	{"FSK-Hell", 0,  (Fl_Callback*)cb_mnuFSKHELL },
	{"FSK-Hell-105", 0,  (Fl_Callback*)cb_mnuFSKHELL105, 0, FL_MENU_DIVIDER },
	{"No change", 0, 0 },
	{ 0 }
};

Fl_Menu_Item quick_change_throb[] = {
	{"Throb 1", 0,  (Fl_Callback*)cb_mnuTHROB1 },
	{"Throb 2", 0,  (Fl_Callback*)cb_mnuTHROB2 },
	{"Throb 4", 0,  (Fl_Callback*)cb_mnuTHROB4 },
	{"ThrobX 1", 0,  (Fl_Callback*)cb_mnuTHROBX1 },
	{"ThrobX 2", 0,  (Fl_Callback*)cb_mnuTHROBX2 },
	{"ThrobX 4", 0,  (Fl_Callback*)cb_mnuTHROBX4, 0, FL_MENU_DIVIDER },
	{"No change", 0, 0 },
	{ 0 }
};


void status_btn_left_click()
{
	int xpos = Fl::event_x();
	int ypos = Fl::event_y();
	const Fl_Menu_Item * m;
	switch (active_modem->get_mode()) {
		case MODE_BPSK31:
		case MODE_PSK63 :
		case MODE_PSK125 :
		case MODE_PSK250 :
			m = quick_change_psk->popup(xpos, ypos, 0, 0, 0);
			if (!m) break;
			if (m->callback_ == 0) break;
			m->callback_(0,0);
			break;
		case MODE_QPSK31 :
		case MODE_QPSK63 :
		case MODE_QPSK125 :
		case MODE_QPSK250 :
			m = quick_change_qpsk->popup(xpos, ypos, 0, 0, 0);
			if (!m) break;
			if (m->callback_ == 0) break;
			m->callback_(0,0);
			break;
		case MODE_MFSK16:
		case MODE_MFSK8:
			m = quick_change_mfsk->popup(xpos, ypos, 0, 0, 0);
			if (!m) break;
			if (m->callback_ == 0) break;
			m->callback_(0,0);
			break;
		case MODE_DOMINOEX4 :
		case MODE_DOMINOEX5 :
		case MODE_DOMINOEX8 :
		case MODE_DOMINOEX11 :
		case MODE_DOMINOEX16 :
		case MODE_DOMINOEX22 :
			m = quick_change_domino->popup(xpos, ypos, 0, 0, 0);
			if (!m) break;
			if (m->callback_ == 0) break;
			m->callback_(0,0);
			break;
		case MODE_FELDHELL :
		case MODE_FSKHELL :
		case MODE_FSKH105 :
			m = quick_change_feld->popup(xpos, ypos, 0, 0, 0);
			if (!m) break;
			if (m->callback_ == 0) break;
			m->callback_(0,0);
			break;
		case MODE_THROB1 :
		case MODE_THROB2 :
		case MODE_THROB4 :
		case MODE_THROBX1 :
		case MODE_THROBX2 :
		case MODE_THROBX4 :
			m = quick_change_throb->popup(xpos, ypos, 0, 0, 0);
			if (!m) break;
			if (m->callback_ == 0) break;
			m->callback_(0,0);
			break;
		default :
			status_btn_right_click();
	}
}

void status_cb(Fl_Widget *b, void *)
{
	int btn = Fl::event_key();
	if (btn == FL_Button+1)
		status_btn_left_click();
	else if (btn == FL_Button+3)
		status_btn_right_click();
}

void cb_cboBand(Fl_Widget *w, void *d) 
{
	Fl_ComboBox *cbBox = (Fl_ComboBox *) w;
	wf->rfcarrier(atoi(cbBox->value())*1000L);
}

void afconoff_cb(Fl_Widget *w, void *vi)
{
	FL_LOCK_D();
	Fl_Button *b = (Fl_Button *)w;
//	Fl_Light_Button *b = (Fl_Light_Button *)w;
	int v = b->value();
	FL_UNLOCK_D();
	active_modem->set_afcOnOff( v ? true : false );
	progdefaults.afconoff = v ? true : false;
}

void sqlonoff_cb(Fl_Widget *w, void *vi)
{
	FL_LOCK_D();
	Fl_Button *b = (Fl_Button *)w;
//	Fl_Light_Button *b = (Fl_Light_Button *)w;
	int v = b->value();
	FL_UNLOCK_D();
	active_modem->set_sqlchOnOff( v ? true : false );
	progdefaults.sqlonoff = v ? true : false;
}


void cb_btnSideband(Fl_Widget *w, void *d)
{
	Fl_Button *b = (Fl_Button *)w;
	FL_LOCK_D();
	progdefaults.btnusb = !progdefaults.btnusb;
	if (progdefaults.btnusb) { 
		b->label("U");
		wf->USB(true);
	} else {
		b->label("L");
		wf->USB(false);
	}
	b->redraw();
	FL_UNLOCK_D();
}

void cbMacroTimerButton(Fl_Widget *w, void *d)
{
	Fl_Button *b = (Fl_Button *)w;
	progdefaults.useTimer = false;
	FL_LOCK_D();
	b->hide();
	FL_UNLOCK_D();
	restoreFocus();
}

void cb_RcvMixer(Fl_Widget *w, void *d)
{
	progdefaults.RcvMixer = valRcvMixer->value();
	mixer.setRcvGain(progdefaults.RcvMixer);
}

void cb_XmtMixer(Fl_Widget *w, void *d)
{
	progdefaults.XmtMixer = valXmtMixer->value();
	mixer.setXmtLevel(progdefaults.XmtMixer);
}


// XPM Calendar Label
static const char *cal_16[] = {
// width height num_colors chars_per_pixel
"    15    14        3            1",
// colors
". c #000000",
"d c none",
"e c #ffffff",
// pixels
"ddddddddddddddd",
"d.............d",
"d.eeeeeeeeeee.d",
"d.............d",
"d.e.e.e.e.e.e.d",
"d.............d",
"d.e.e.e.e.e.e.d",
"d.............d",
"d.e.e.e.e.e.e.d",
"d.............d",
"d.e.e.e.e.e.e.d",
"d.............d",
"d.e.e.e.e.e.e.d",
"ddddddddddddddd",
};


Fl_Menu_Item menu_[] = {
{"&Files", 0,  0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0}, // 0
{"Open Macros", 0,  (Fl_Callback*)cb_mnuOpenMacro, 0, 0, FL_NORMAL_LABEL, 0, 14, 0}, // 1
{"Save Macros", 0,  (Fl_Callback*)cb_mnuSaveMacro, 0, FL_MENU_DIVIDER, FL_NORMAL_LABEL, 0, 14, 0}, // 2
{"Log File", 0, (Fl_Callback*)cb_mnuLogFile, 0, FL_MENU_DIVIDER | FL_MENU_TOGGLE, FL_NORMAL_LABEL, 0, 14, 0}, // 3
{"E&xit", 0,  (Fl_Callback*)cb_E, 0, 0, FL_NORMAL_LABEL, 0, 14, 0}, // 4
{0,0,0,0,0,0,0,0,0}, // 5
{"Op &Mode", 0,  0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0}, // 6
{"CW", 0,  (Fl_Callback*)cb_mnuCW, 0, 0, FL_NORMAL_LABEL, 0, 14, 0}, // 7
{"DominoEX", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0}, // 8
{"dominoex 4", 0,  (Fl_Callback*)cb_mnuDOMINOEX4, 0, 0, FL_NORMAL_LABEL, 0, 14, 0}, // 9
{"dominoex 5", 0,  (Fl_Callback*)cb_mnuDOMINOEX5, 0, 0, FL_NORMAL_LABEL, 0, 14, 0}, // 10
{"dominoex 8", 0,  (Fl_Callback*)cb_mnuDOMINOEX8, 0, 0, FL_NORMAL_LABEL, 0, 14, 0}, // 11
{"dominoex 11", 0,  (Fl_Callback*)cb_mnuDOMINOEX11, 0, 0, FL_NORMAL_LABEL, 0, 14, 0}, // 12
{"dominoex 16", 0,  (Fl_Callback*)cb_mnuDOMINOEX16, 0, 0, FL_NORMAL_LABEL, 0, 14, 0}, // 13
{"dominoex 22", 0,  (Fl_Callback*)cb_mnuDOMINOEX22, 0, 0, FL_NORMAL_LABEL, 0, 14, 0}, // 14
{0,0,0,0,0,0,0,0,0}, // 15
{"Hell", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0}, // 16
{"Feld-Hell", 0,  (Fl_Callback*)cb_mnuFELDHELL, 0, 0, FL_NORMAL_LABEL, 0, 14, 0}, // 17
{"FSK-Hell", 0,  (Fl_Callback*)cb_mnuFSKHELL, 0, 0, FL_NORMAL_LABEL, 0, 14, 0}, // 18
{"FSK-Hell-105", 0,  (Fl_Callback*)cb_mnuFSKHELL105, 0, 0, FL_NORMAL_LABEL, 0, 14, 0}, // 19
{0,0,0,0,0,0,0,0,0}, // 20
{"MFSK", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0}, // 21
{"mfsk 8", 0,  (Fl_Callback*)cb_mnuMFSK8, 0, 0, FL_NORMAL_LABEL, 0, 14, 0}, // 22
{"mfsk 16", 0,  (Fl_Callback*)cb_mnuMFSK16, 0, 0, FL_NORMAL_LABEL, 0, 14, 0}, // 23
{0,0,0,0,0,0,0,0,0}, // 24
{"Psk", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0}, // 25
{"psk 31", 0,  (Fl_Callback*)cb_mnuPSK31, 0, 0, FL_NORMAL_LABEL, 0, 14, 0}, // 26
{"qpsk 31", 0,  (Fl_Callback*)cb_mnuQPSK31, 0, 0, FL_NORMAL_LABEL, 0, 14, 0}, // 27
{"psk 63", 0,  (Fl_Callback*)cb_mnuPSK63, 0, 0, FL_NORMAL_LABEL, 0, 14, 0}, // 28
{"qpsk 63", 0,  (Fl_Callback*)cb_mnuQPSK63, 0, 0, FL_NORMAL_LABEL, 0, 14, 0}, // 29
{"psk 125", 0,  (Fl_Callback*)cb_mnuPSK125, 0, 0, FL_NORMAL_LABEL, 0, 14, 0}, // 30
{"qpsk 125", 0,  (Fl_Callback*)cb_mnuQPSK125, 0, 0, FL_NORMAL_LABEL, 0, 14, 0}, // 31
{"psk 250", 0,  (Fl_Callback*)cb_mnuPSK250, 0, 0, FL_NORMAL_LABEL, 0, 14, 0}, // 32
{"qpsk 250", 0,  (Fl_Callback*)cb_mnuQPSK250, 0, 0, FL_NORMAL_LABEL, 0, 14, 0}, // 33
{0,0,0,0,0,0,0,0,0}, // 34
{"Olivia", 0,  (Fl_Callback*)cb_mnuOlivia, 0, 0, FL_NORMAL_LABEL, 0, 14, 0}, // 35
{"rtty", 0,  (Fl_Callback*)cb_mnuRTTY, 0, 0, FL_NORMAL_LABEL, 0, 14, 0}, // 36
{"Throb", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0}, // 37
{"Throb 1", 0,  (Fl_Callback*)cb_mnuTHROB1, 0, 0, FL_NORMAL_LABEL, 0, 14, 0}, // 38
{"Throb 2", 0,  (Fl_Callback*)cb_mnuTHROB2, 0, 0, FL_NORMAL_LABEL, 0, 14, 0}, // 39
{"Throb 4", 0,  (Fl_Callback*)cb_mnuTHROB4, 0, 0, FL_NORMAL_LABEL, 0, 14, 0}, // 40
{"ThrobX 1", 0,  (Fl_Callback*)cb_mnuTHROBX1, 0, 0, FL_NORMAL_LABEL, 0, 14, 0}, // 41
{"ThrobX 2", 0,  (Fl_Callback*)cb_mnuTHROBX2, 0, 0, FL_NORMAL_LABEL, 0, 14, 0}, // 42
{"ThrobX 4", 0,  (Fl_Callback*)cb_mnuTHROBX4, 0, 0, FL_NORMAL_LABEL, 0, 14, 0}, // 43
{0,0,0,0,0,0,0,0,0}, // 44
{"WWV", 0,  (Fl_Callback*)cb_mnuWWV, 0, 0, FL_NORMAL_LABEL, 0, 14, 0}, // 45
{"Freq Analysis", 0,  (Fl_Callback*)cb_mnuANALYSIS, 0, 0, FL_NORMAL_LABEL, 0, 14, 0}, // 46
{0,0,0,0,0,0,0,0,0}, // 47
{"Configure", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0}, // 48
{"Defaults",  0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0}, // 49
{"Fonts", 0, (Fl_Callback*)cb_mnuConfigFonts, 0, 0, FL_NORMAL_LABEL, 0, 14, 0}, // 50
{"Operator", 0, (Fl_Callback*)cb_mnuConfigOperator, 0, 0, FL_NORMAL_LABEL, 0, 14, 0}, // 51
{"Rig Ctrl", 0, (Fl_Callback*)cb_mnuConfigRigCtrl, 0, 0, FL_NORMAL_LABEL, 0, 14, 0}, // 52
{"Sound Card", 0, (Fl_Callback*)cb_mnuConfigSoundCard, 0, 0, FL_NORMAL_LABEL, 0, 14, 0}, // 53
{"Waterfall", 0,  (Fl_Callback*)cb_mnuConfigWaterfall, 0, 0, FL_NORMAL_LABEL, 0, 14, 0}, // 54
{0,0,0,0,0,0,0,0,0}, // 55
{"Modems", 0, (Fl_Callback*)cb_mnuConfigModems, 0, 0, FL_NORMAL_LABEL, 0, 14, 0}, // 56
{"Save Config", 0, (Fl_Callback*)cb_mnuSaveConfig, 0, 0, FL_NORMAL_LABEL, 0, 14, 0}, // 57
{0,0,0,0,0,0,0,0,0}, // 58
{"     ", 0, 0, 0, FL_MENU_INACTIVE, FL_NORMAL_LABEL, 0, 14, 0}, // 59
{"Rig", 0, (Fl_Callback*)cb_mnuRig, 0, 0, FL_NORMAL_LABEL, 0, 14, 0}, // 60
{"     ", 0, 0, 0, FL_MENU_INACTIVE, FL_NORMAL_LABEL, 0, 14, 0}, // 61
{"Help", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0}, // 62
{"About", 0, (Fl_Callback*)cb_mnuAbout, 0, 0, FL_NORMAL_LABEL, 0, 14, 0}, // 63
{0,0,0,0,0,0,0,0,0}, // 64
{"  ", 0, 0, 0, FL_MENU_INACTIVE, FL_NORMAL_LABEL, 0, 14, 0}, // 65
{"Wav", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0}, // 66
{"Rx capture",  0, (Fl_Callback*)cb_mnuCapture,  0, FL_MENU_TOGGLE, FL_NORMAL_LABEL, 0, 14, 0},//67
{"Tx generate", 0, (Fl_Callback*)cb_mnuGenerate, 0, FL_MENU_TOGGLE, FL_NORMAL_LABEL, 0, 14, 0},//68
{"Playback",    0, (Fl_Callback*)cb_mnuPlayback, 0, FL_MENU_TOGGLE, FL_NORMAL_LABEL, 0, 14, 0},//69
{0,0,0,0,0,0,0,0,0}, // 70
{0,0,0,0,0,0,0,0,0}, // 71
};

Fl_Menu_Bar *mnu;

void activate_rig_menu_item(bool b)
{
	if (b) {
		bSaveFreqList = true;
		menu_[60].activate();
		
	} else {
		menu_[60].deactivate();
		if (rigcontrol)
			rigcontrol->hide();
	}
	mnu->redraw();
}

void activate_test_menu_item(bool b)
{
	if (b)
		menu_[60].show();
	else
		menu_[60].hide();
	mnu->redraw();
}

void make_pixmap(Pixmap *xpm, const char **data)
{
	// We need a displayed window to provide a GC for X_CreatePixmap
	Fl_Window w(0, 0, FLDIGI_NAME);
	w.xclass(FLDIGI_NAME);
	w.show();
	w.make_current();

	Fl_Pixmap icon(data);
	int maxd = max(icon.w(), icon.h());
	*xpm = fl_create_offscreen(maxd, maxd);

	fl_begin_offscreen(*xpm);
	fl_color(FL_BACKGROUND_COLOR);
	fl_rectf(0, 0, maxd, maxd);
	icon.draw(maxd - icon.w(), maxd - icon.h());
	fl_end_offscreen();
}

void create_fl_digi_main() {
	int Y = 0;
	fl_digi_main = new Fl_Double_Window(WNOM, HNOM, "fldigi");
			mnu = new Fl_Menu_Bar(0, 0, WNOM - 142, Hmenu);
			// FL_NORMAL_SIZE may have changed; update the menu items
			for (size_t i = 0; i < sizeof(menu_)/sizeof(menu_[0]); i++)
				if (menu_[i].text)
					menu_[i].labelsize_ = FL_NORMAL_SIZE;
			mnu->menu(menu_);

#ifndef USE250
			menu_[32].hide();
			menu_[33].hide();
#endif	
			btnTune = new Fl_Button(WNOM - 142, 0, 60, Hmenu, "TUNE");
			btnTune->type(FL_TOGGLE_BUTTON);
			btnTune->callback(cbTune, 0);
			
			btnMacroTimer = new Fl_Button(WNOM - 82, 0, 80, Hmenu);
			btnMacroTimer->color(fl_rgb_color(255, 255, 100));
			btnMacroTimer->labelcolor(FL_RED);
			btnMacroTimer->callback(cbMacroTimerButton, 0);
			btnMacroTimer->hide();
			Fl_Box *bx = new Fl_Box(WNOM - 82, 0, 80, Hmenu,"");
			bx->box(FL_UP_BOX);
			
		
		Y += Hmenu;

		Fl_Group *qsoFrame = new Fl_Group(0,Y, WNOM, Hqsoframe);
			inpFreq = new Fl_Input(2,Y + Hqsoframe/2 - 2, 85, Hqsoframe/2,"Freq: ");
			inpFreq->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);
			inpTime = new Fl_Input(89,Y + Hqsoframe/2 - 2, 45, Hqsoframe/2,"Time: ");
			inpTime->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);
			qsoTime = new Fl_Button(136, Y + Hqsoframe/2 - 2, 24, Hqsoframe/2);
			Fl_Image *pixmap = new Fl_Pixmap(cal_16);
			qsoTime->image(pixmap);
			qsoTime->callback(qsoTime_cb, 0);
			inpCall = new Fl_Input(162,Y + Hqsoframe/2 - 2, 80,Hqsoframe/2,"Call: ");
			inpCall->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);
			inpName = new Fl_Input(244,Y + Hqsoframe/2 - 2, 100,Hqsoframe/2,"Name: ");
			inpName->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);
			inpRstIn = new Fl_Input(346,Y + Hqsoframe/2 - 2, 40,Hqsoframe/2,"Rst(r): ");
			inpRstIn->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);
			inpRstOut = new Fl_Input(388,Y + Hqsoframe/2 - 2, 40,Hqsoframe/2,"Rst(s): ");
			inpRstOut->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);
			inpQth = new Fl_Input(430,Y + Hqsoframe/2 - 2, WNOM - 430 - 126,Hqsoframe/2,"Qth: ");
			inpQth->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);
			inpLoc = new Fl_Input(WNOM - 124,Y + Hqsoframe/2 - 2, 80,Hqsoframe/2,"Loc: ");
			inpLoc->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);
			qsoClear = new Fl_Button(WNOM - 42, Y + Hqsoframe/2 + 1, 40, Hqsoframe/2 - 2, "Clear");
			qsoClear->callback(qsoClear_cb, 0);
			btnQRZ = new Fl_Button( WNOM - 42, Y + 1, 40, Hqsoframe/2 - 2, "QRZ");
			btnQRZ->callback(cb_QRZ, 0);
				
			qsoFrame->resizable(inpQth);
		qsoFrame->end();
		Y += Hqsoframe;

		Fl_Group *qsoFrame2 = new Fl_Group(0,Y, WNOM, Hnotes);

			inpNotes = new Fl_Input(136, Y, WNOM - 136 - 44 - 120, Hnotes,"Notes: ");
			inpNotes->align(FL_ALIGN_LEFT);
			
			cboBand  = new Fl_ComboBox(2, Y, 110, Hnotes, "");
			cboBand->hide();
			btnSideband = new Fl_Button(112, Y+1, Hnotes-2, Hnotes-2, "U");
			btnSideband->callback(cb_btnSideband, 0);
			btnSideband->hide();
			
			inpAZ = new Fl_Input(WNOM - 44 - 80, Y, 80, Hnotes, "AZ:"); // WA5ZNU
			inpAZ->align(FL_ALIGN_LEFT);

			qsoSave = new Fl_Button(WNOM - 42, Y + 1, 40, Hnotes- 2, "Save");
			qsoSave->callback(qsoSave_cb, 0);
			qsoFrame2->resizable(inpNotes);
		qsoFrame2->end();
		Y += Hnotes;
		
		int sw = 15;
		Fl_Group *MixerFrame = new Fl_Group(0,Y,sw, Hrcvtxt + Hxmttxt);
			valRcvMixer = new Fl_Slider(0, Y, sw, (Htext)/2, "");
			valRcvMixer->type(FL_VERT_NICE_SLIDER);
			valRcvMixer->color(fl_rgb_color(0,110,30));
			valRcvMixer->labeltype(FL_ENGRAVED_LABEL);
			valRcvMixer->selection_color(fl_rgb_color(255,255,0));
			valRcvMixer->range(1.0,0.0);
			valRcvMixer->callback( (Fl_Callback *)cb_RcvMixer);
			valXmtMixer = new Fl_Slider(0, Y + (Htext)/2, sw, (Htext)/2, "");
			valXmtMixer->type(FL_VERT_NICE_SLIDER);
			valXmtMixer->color(fl_rgb_color(110,0,30));
			valXmtMixer->labeltype(FL_ENGRAVED_LABEL);
			valXmtMixer->selection_color(fl_rgb_color(255,255,0));
			valXmtMixer->range(1.0,0.0);
			valXmtMixer->callback( (Fl_Callback *)cb_XmtMixer);
			valRcvMixer->deactivate();
			valXmtMixer->deactivate();
		MixerFrame->end();

		TiledGroup = new Fl_Tile_check(sw, Y, WNOM-sw, Htext);
            int minRxHeight = Hrcvtxt;
            int minTxHeight;
            if (minRxHeight < 66) minRxHeight = 66;
            minTxHeight = Htext - minRxHeight;

			Fl_Box *minbox = new Fl_Box(sw,Y + 66, WNOM-sw, Htext - 66 - 32);
			minbox->hide();

			if (progdefaults.alt_text_widgets)
				ReceiveText = new FTextView(sw, Y, WNOM-sw, minRxHeight, "");
			else
				ReceiveText = new TextView(sw, Y, WNOM-sw, minRxHeight, "");
		
			FHdisp = new Raster(sw, Y, WNOM-sw, minRxHeight);
			FHdisp->hide();
			Y += minRxHeight;

			if (progdefaults.alt_text_widgets)
				TransmitText = new FTextEdit(sw, Y, WNOM-sw, minTxHeight);
			else
				TransmitText = new TextEdit(sw, Y, WNOM-sw, minTxHeight);
			Y += minTxHeight;

			TiledGroup->resizable(minbox);
		TiledGroup->end();
		Fl_Group::current()->resizable(TiledGroup);

		
		Fl_Box *macroFrame = new Fl_Box(0, Y, WNOM, Hmacros);
			macroFrame->box(FL_ENGRAVED_FRAME);
			int Wbtn = (WNOM - 30 - 8 - 4)/10;
			int xpos = 2;
			for (int i = 0; i < 10; i++) {
				if (i == 4 || i == 8) {
					bx = new Fl_Box(xpos, Y+2, 5, Hmacros - 4);
					bx->box(FL_FLAT_BOX);
					bx->color(FL_BLACK);
					xpos += 4;
				}
				btnMacro[i] = new Fl_Button(xpos, Y+2, Wbtn, Hmacros - 4);
				btnMacro[i]->callback(macro_cb, (void *)i);
				btnMacro[i]->label( (macros.name[i]).c_str());
				xpos += Wbtn;
			}
			bx = new Fl_Box(xpos, Y+2, WNOM - 32 - xpos, Hmacros - 4);
			bx->box(FL_FLAT_BOX);
			bx->color(FL_BLACK);
			btnAltMacros = new Fl_Button(WNOM-32, Y+2, 30, Hmacros - 4, "Alt");
			btnAltMacros->callback(altmacro_cb, 0);
				
		Y += Hmacros;

		Fl_Pack *wfpack = new Fl_Pack(0, Y, WNOM, Hwfall);
			wfpack->type(1);
			wf = new waterfall(0, Y, Wwfall, Hwfall);
			wf->end();
			Fl_Pack *ypack = new Fl_Pack(WNOM-(Hwfall-24), Y, Hwfall-24, Hwfall);
				ypack->type(0);

				digiscope = new Digiscope (WNOM-(Hwfall-24), Y, Hwfall-24, Hwfall-24);
	
				pgrsSquelch = new Fl_Progress(
					WNOM-(Hwfall-24), Y + Hwfall - 24,
					Hwfall - 24, 12, "");
				pgrsSquelch->color(FL_BACKGROUND2_COLOR, FL_DARK_GREEN);
				sldrSquelch = new Fl_Slider(
					FL_HOR_NICE_SLIDER, 
					WNOM-(Hwfall-24), Y + Hwfall - 12, 
					Hwfall - 24, 12, "");
							
				sldrSquelch->minimum(0);
				sldrSquelch->maximum(100);
				sldrSquelch->step(1);
				sldrSquelch->value(progdefaults.sldrSquelchValue);
				sldrSquelch->callback((Fl_Callback*)cb_sldrSquelch);
				sldrSquelch->color(FL_INACTIVE_COLOR);

			ypack->end();
			Fl_Group::current()->resizable(wf);
		wfpack->end();
		Y += (Hwfall + 2);

		Fl_Pack *hpack = new Fl_Pack(0, Y, WNOM, Hstatus);
			hpack->type(1);
			MODEstatus = new Fl_Button(0,Hmenu+Hrcvtxt+Hxmttxt+Hwfall, Wmode, Hstatus, "");
			MODEstatus->box(FL_DOWN_BOX);
			MODEstatus->color(FL_BACKGROUND2_COLOR);
			MODEstatus->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
			MODEstatus->callback(status_cb, (void *)0);
			MODEstatus->tooltip("Left clk - change mode\nRight clk - Modem Tab");
			Status1 = new Fl_Box(Wmode,Hmenu+Hrcvtxt+Hxmttxt+Hwfall, Ws2n, Hstatus, "");
			Status1->box(FL_DOWN_BOX);
			Status1->color(FL_BACKGROUND2_COLOR);
			Status1->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
			Status2 = new Fl_Box(Wmode+Ws2n, Hmenu+Hrcvtxt+Hxmttxt+Hwfall, Wimd, Hstatus, "");
			Status2->box(FL_DOWN_BOX);
			Status2->color(FL_BACKGROUND2_COLOR);
			Status2->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
			StatusBar = new Fl_Box(
                Wmode+Wimd+Ws2n, Hmenu+Hrcvtxt+Hxmttxt+Hwfall, Wstatus, Hstatus, "");
			StatusBar->box(FL_DOWN_BOX);
			StatusBar->color(FL_BACKGROUND2_COLOR);
			StatusBar->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

			WARNstatus = new Fl_Box(
                Wmode+Wimd+Ws2n+Wstatus, Hmenu+Hrcvtxt+Hxmttxt+Hwfall, 
                Wwarn, Hstatus, "");
			WARNstatus->box(FL_DIAMOND_DOWN_BOX);
			WARNstatus->color(FL_BACKGROUND_COLOR);
			WARNstatus->labelcolor(FL_RED);
			WARNstatus->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE);

			if (useCheckButtons) {
				chk_afconoff = new Fl_Check_Button(
								WNOM - bwAfcOnOff - bwSqlOnOff, 
								Hmenu+Hrcvtxt+Hxmttxt+Hwfall, 
								bwAfcOnOff, Hstatus, "Afc");
				chk_afconoff->callback(afconoff_cb, 0);
				chk_afconoff->value(1);
				chk_afconoff->tooltip("AFC on/off");
				chk_sqlonoff = new Fl_Check_Button(
								WNOM - bwSqlOnOff, 
								Hmenu+Hrcvtxt+Hxmttxt+Hwfall, 
								bwSqlOnOff, Hstatus, "Sql");
				chk_sqlonoff->callback(sqlonoff_cb, 0);
				chk_sqlonoff->value(1);
				chk_sqlonoff->tooltip("SQL on/off");
			} else {
				afconoff = new Fl_Light_Button(
								WNOM - bwAfcOnOff - bwSqlOnOff, 
								Hmenu+Hrcvtxt+Hxmttxt+Hwfall, 
								bwAfcOnOff, Hstatus, "Afc");
				afconoff->callback(afconoff_cb, 0);
				afconoff->value(1);
				afconoff->tooltip("AFC on/off");
				sqlonoff = new Fl_Light_Button(
								WNOM - bwSqlOnOff, 
								Hmenu+Hrcvtxt+Hxmttxt+Hwfall, 
								bwSqlOnOff, Hstatus, "Sql");
				sqlonoff->callback(sqlonoff_cb, 0);
				sqlonoff->value(1);
				sqlonoff->tooltip("SQL on/off");
			}
				
			Fl_Group::current()->resizable(StatusBar);
		hpack->end();

		fl_digi_main->size_range(WNOM, HNOM);
	fl_digi_main->end();
	fl_digi_main->callback(cb_wMain);

	make_pixmap(&fldigi_icon_pixmap, fldigi_icon_48_xpm);
	fl_digi_main->icon((char *)fldigi_icon_pixmap);

	fl_digi_main->xclass(FLDIGI_NAME);
}

void put_freq(double frequency)
{
	wf->carrier((int)floor(frequency + 0.5));
}

void put_Bandwidth(int bandwidth)
{
	wf->Bandwidth ((int)bandwidth);
}

void display_metric(double metric)
{
	FL_LOCK_D();
	QUEUE(CMP_CB(&Fl_Progress::value, pgrsSquelch, metric)); //pgrsSquelch->value(metric);
	FL_UNLOCK_D();
	FL_AWAKE_D();
}

void put_cwRcvWPM(double wpm)
{
	int U = progdefaults.CWupperlimit;
	int L = progdefaults.CWlowerlimit;
	double dWPM = 100.0*(wpm - L)/(U - L);
	FL_LOCK_D();
	QUEUE(CMP_CB(&Fl_Progress::value, prgsCWrcvWPM, dWPM)); //prgsCWrcvWPM->value(dWPM);
	QUEUE(CMP_CB(&Fl_Value_Output::value, valCWrcvWPM, (int)wpm)); //valCWrcvWPM->value((int)wpm);
	FL_UNLOCK_D();
	FL_AWAKE_D();
}

void set_scope(double *data, int len, bool autoscale)
{
	if (digiscope)
		digiscope->data(data, len, autoscale);
}

void set_phase(double phase, bool highlight)
{
	if (digiscope)
		digiscope->phase(phase, highlight);
}

void set_rtty(double flo, double fhi, double amp)
{
	if (digiscope)
		digiscope->rtty(flo, fhi, amp);
}

void set_video(double *data, int len)
{
	if (digiscope)
		digiscope->video(data, len);
}

void put_rx_char(unsigned int data)
{
	static unsigned int last = 0;
	const char **asc = ascii;
	rxmsgid = msgget( (key_t) progdefaults.rx_msgid, 0666);
	if (mailclient || mailserver || rxmsgid != -1)
		asc = ascii2;
	if (active_modem->get_mode() == MODE_RTTY ||
		active_modem->get_mode() == MODE_CW)
		asc = ascii;

	int style = ReceiveWidget::RECV;
	data &= 0x7F;
	switch (data) {
	case '\n':
		if (last == '\r')
			break;
		// or fall-through to insert '\n'
	case '\r':
		data = '\n';
	default:
		if (asc == ascii2 && iscntrl(data))
			style = ReceiveWidget::CTRL;
		if (wf->tmp_carrier())
			style = ReceiveWidget::ALTR;
		QUEUE(CMP_CB(&ReceiveWidget::addchr, ReceiveText, data, style));
	}
	last = data;

	if ( rxmsgid != -1) {
		rxmsgst.msg_type = 1;
		rxmsgst.c = data;
		msgsnd (rxmsgid, (void *)&rxmsgst, 1, IPC_NOWAIT);
	}

	if (Maillogfile)
		Maillogfile->log_to_file(cLogfile::LOG_RX, ascii2[data]);

	if (logging)
		logfile->log_to_file(cLogfile::LOG_RX, ascii2[data]);
}

string strSecText = "";

void put_sec_char( char chr )
{
	if (chr >= ' ' && chr <= 'z') {
		strSecText.append(1, chr);
		if (strSecText.length() > 60)
			strSecText.erase(0,1);
		FL_LOCK_D();
		QUEUE(CMP_CB(&Fl_Box::label, StatusBar, strSecText.c_str())); //StatusBar->label(strSecText.c_str());
		FL_UNLOCK_D();
		FL_AWAKE_D();
	}
}

void put_status(const char *msg)
{
	static char m[60];
	strncpy(m, msg, sizeof(m));
	m[sizeof(m) - 1] = '\0';

	FL_LOCK_D();
	QUEUE(CMP_CB(&Fl_Box::label, StatusBar, m)); // StatusBar->label(m);
	FL_UNLOCK_D();
	FL_AWAKE_D();
}

void put_Status2(const char *msg)
{
	static char m[60];
	strncpy(m, msg, sizeof(m));
	m[sizeof(m) - 1] = '\0';

	FL_LOCK_D();
	QUEUE(CMP_CB(&Fl_Box::label, Status2, m)); //Status2->label(m);
	FL_UNLOCK_D();
	FL_AWAKE_D();
}

void put_Status1(const char *msg)
{
	static char m[60];
	strncpy(m, msg, sizeof(m));
	m[sizeof(m) - 1] = '\0';

	FL_LOCK_D();
	QUEUE(CMP_CB(&Fl_Box::label, Status1, m)); //Status1->label(m);
	FL_UNLOCK_D();
	FL_AWAKE_D();
}


void put_WARNstatus(double val)
{
	FL_LOCK_D();
	if (val < 0.05)
		WARNstatus->color(FL_BLACK);
    if (val > 0.05)
        WARNstatus->color(FL_DARK_GREEN);
    if (val > 0.9)
        WARNstatus->color(FL_YELLOW);
    if (val > 0.98)
        WARNstatus->color(FL_DARK_RED);
	WARNstatus->redraw();
	FL_UNLOCK_D();
}


void set_CWwpm()
{
	FL_LOCK();
	sldrCWxmtWPM->value(progdefaults.CWspeed);
	FL_UNLOCK();
}

void clear_StatusMessages()
{
	FL_LOCK_E();
	StatusBar->label("");
	Status1->label("");
	Status2->label("");
	FL_UNLOCK_E();
	FL_AWAKE_E();
}

	
void put_MODEstatus(trx_mode mode)
{
	FL_LOCK_D();
	QUEUE(CMP_CB(&Fl_Button::label, MODEstatus, mode_names[mode])); //MODEstatus->label(mode_names[mode]);
	FL_UNLOCK_D();
	FL_AWAKE_D();
}

void put_rx_data(int *data, int len)
{
 	FHdisp->data(data, len);
}

int get_tx_char(void)
{
	if (pskmail_text_available)
		return pskmail_get_char();

	int c;
	static int pending = -1;
	if (pending >= 0) {
		c = pending;
		pending = -1;
		return c;
	}

	enum { STATE_CHAR, STATE_CTRL };
	static int state = STATE_CHAR;

	switch (c = TransmitText->nextChar()) {
	case '\n':
		pending = '\n';
		return '\r';
	case '^':
		if (state == STATE_CTRL)
			break;
		state = STATE_CTRL;
		return -1;
	case 'r': case 'R':
		if (state != STATE_CTRL)
			break;
		QUEUE_SYNC(CMP_CB(&TransmitWidget::clear, TransmitText));
		state = STATE_CHAR;
		c = 3; // ETX
		break;
	case -1:
		break;
	default:
		if (state == STATE_CTRL) {
			state = STATE_CHAR;
			pending = c;
			return '^';
		}
	}

	pending = -1;
	return c;
}

void put_echo_char(unsigned int data)
{
	static unsigned int last = 0;
	const char **asc = ascii;
	
	if (mailclient || mailserver || arqmode)
		asc = ascii2;
	if (active_modem->get_mode() == MODE_RTTY ||
		active_modem->get_mode() == MODE_CW)
		asc = ascii;

	data &= 0x7F;
	if (data == '\r' && last == '\r') // reject multiple CRs
		return;

	last = data;

	int style = ReceiveWidget::XMIT;
	if (asc == ascii2 && iscntrl(data))
		style = ReceiveWidget::CTRL;
	QUEUE(CMP_CB(&ReceiveWidget::addchr, ReceiveText, data, style));

	if (Maillogfile)
		Maillogfile->log_to_file(cLogfile::LOG_TX, ascii2[data & 0x7F]);
	if (logging)
		logfile->log_to_file(cLogfile::LOG_TX, ascii2[data & 0x7F]);
}

void resetRTTY() {
	if (active_modem->get_mode() != MODE_RTTY) return;
	trx_reset(scDevice.c_str());
	active_modem->restart();
}

void resetOLIVIA() {
	if (active_modem->get_mode() != MODE_OLIVIA) return;
	trx_reset(scDevice.c_str());
	active_modem->restart();
}

void resetDOMEX() {
	trx_mode md = active_modem->get_mode();
	if (md == MODE_DOMINOEX4 ||
		md == MODE_DOMINOEX5 ||
		md == MODE_DOMINOEX8 ||
		md == MODE_DOMINOEX11 ||
		md == MODE_DOMINOEX16 ||
		md == MODE_DOMINOEX22 ) {

		trx_reset(scDevice.c_str());
		active_modem->restart();
	}
}

void enableMixer(bool on)
{
	FL_LOCK_D();
	if (on) {
		progdefaults.EnableMixer = true;
		mixer.openMixer(progdefaults.MXdevice.c_str());

		mixer.PCMVolume(progdefaults.PCMvolume);
		mixer.setXmtLevel(valXmtMixer->value());
		mixer.setRcvGain(valRcvMixer->value());
		if (progdefaults.LineIn == true)
			setMixerInput(1);
		else if (progdefaults.MicIn == true)
			setMixerInput(2);
		else
			setMixerInput(0);
	}else{
		progdefaults.EnableMixer = false;
		mixer.closeMixer();
	}
        resetMixerControls();
	FL_UNLOCK_D();
}

void resetMixerControls()
{
    if (progdefaults.EnableMixer) {
	    valRcvMixer->activate();
	    valXmtMixer->activate();
	    menuMix->activate();
	    btnLineIn->activate();
	    btnMicIn->activate();
        btnMixer->value(1);
	    valPCMvolume->activate();
    }
    else {
	    valRcvMixer->deactivate();
	    valXmtMixer->deactivate();
	    menuMix->deactivate();
	    btnLineIn->deactivate();
	    btnMicIn->deactivate();
        btnMixer->value(0);
	    valPCMvolume->deactivate();
    }
}

void setPCMvolume(double vol)
{
	mixer.PCMVolume(vol);
	progdefaults.PCMvolume = vol;
}

void setMixerInput(int dev)
{
	int n= -1;
	switch (dev) {
		case 0: n = mixer.InputSourceNbr("Vol");
				break;
		case 1: n = mixer.InputSourceNbr("Line");
				break;
		case 2: n = mixer.InputSourceNbr("Mic");
				break;
		default: n = mixer.InputSourceNbr("Vol");
	}
	if (n != -1)
		mixer.SetCurrentInputSource(n);
}

void resetSoundCard()
{
    bool mixer_enabled = progdefaults.EnableMixer;
	enableMixer(false);
	trx_reset(scDevice.c_str());
	progdefaults.SCdevice = scDevice;
    if (mixer_enabled)
        enableMixer(true);
}

void setReverse(int rev) {
	active_modem->set_reverse(rev);
}

/*
void setAfcOnOff(bool b) {
	FL_LOCK();
	afconoff->value(b);
	FL_UNLOCK();
	FL_AWAKE();
}	

void setSqlOnOff(bool b) {
	FL_LOCK();
	sqlonoff->value(b);
	FL_UNLOCK();
	FL_AWAKE();
}

bool QueryAfcOnOff() {
	FL_LOCK_E();
	int v = afconoff->value();
	FL_UNLOCK_E();
	return v;
}

bool QuerySqlOnOff() {
	FL_LOCK_E();
	int v = sqlonoff->value();
	FL_UNLOCK_E();
	return v;
}
*/


