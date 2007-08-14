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

#include "version.h"

#include "waterfall.h"
#include "raster.h"
#include "main.h"
#include "threads.h"
#include "trx.h"
#ifndef NOHAMLIB
	#include "hamlib.h"
#endif
#include "rigCAT.h"
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

#include "status.h"

#include "rigsupport.h"


Fl_Double_Window	*fl_digi_main=(Fl_Double_Window *)0;

cMixer mixer;

Fl_Button			*btnTune = (Fl_Button *)0;
Fl_Tile				*TiledGroup = (Fl_Tile *)0;
//Fl_Group				*TiledGroup = (Fl_Group *)0;
TextView			*ReceiveText=(TextView *)0;
TextEdit			*TransmitText=(TextEdit *)0;
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

void clearStatus()
{
	clear_StatusMessages();
}

void startup_modem(modem *m)
{
	trx_start_modem(m);

	restoreFocus();

	Fl::lock();
	if (m == feld_modem ||
		m == feld_FMmodem ||
		m == feld_FM105modem ) {
		ReceiveText->Hide();
		FHdisp->show();
	} else {
		ReceiveText->Show();
		FHdisp->hide();
	}
	Fl::unlock();
	Fl::awake();

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

	mixer.closeMixer();
	active_modem->set_stopflag(true);
	MilliSleep(100);

	fl_lock (&trx_mutex);
	if (active_modem) {
		active_modem->shutdown();
		delete active_modem;
	}
	active_modem = (modem *) 0;
	fl_unlock (&trx_mutex);

#ifndef NOHAMLIB	
	delete xcvr;
#endif
	delete push2talk;
	
//	if (KeyLine)
//		delete KeyLine;

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
	Fl::lock();
	Fl::focus(TransmitText);
	TransmitText->cursorON();
	TransmitText->redraw();	
	Fl::unlock();
	Fl::awake();
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
	Fl::lock();
	for (int i = 0; i < 10; i++)
		btnMacro[i]->label(macros.name[i + (altMacros ? 10: 0)].c_str());
	Fl::unlock();
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

void cb_mnuConfigInterface(Fl_Menu_*, void*) {
	progdefaults.loadDefaults();
	tabsConfigure->value(tabInterface);
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
void cb_mnuCapture(Fl_Menu_ *m, void *d)
{
	if (!scard) return;
	capval = !capval;
	scard->Capture(capval);
}

bool genval = false;
void cb_mnuGenerate(Fl_Menu_ *m, void *d)
{
	if (!scard) return;
	genval = !genval;
	scard->Generate(genval);
}

bool playval = false;
void cb_mnuPlayback(Fl_Menu_ *m, void *d)
{
	if (!scard) return;
	playval = !playval;
	scard->Playback(playval);
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
//	progdefaults.FontColor = (int)clr;
	
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

double sldrSquelchValue;

void cb_sldrSquelch(Fl_Slider* o, void*) {
	active_modem->set_squelch(o->value());
	sldrSquelchValue = o->value();
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
	Fl::lock();
	inpTime->value(zuluTime());
	Fl::unlock();
	Fl::awake();
	restoreFocus();
}

void clearQSO()
{
	Fl::lock();
		inpTime->value(zuluTime());
		inpCall->value("");
		inpName->value("");
		inpRstIn->value("");
		inpRstOut->value("");
		inpQth->value("");
		inpLoc->value("");
		inpNotes->value("");
	Fl::unlock();
}

void qsoClear_cb(Fl_Widget *b, void *)
{
	clearQSO();
	Fl::awake();
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

void status_cb(Fl_Widget *b, void *)
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

void cb_cboBand(Fl_Widget *w, void *d) 
{
	Fl_ComboBox *cbBox = (Fl_ComboBox *) w;
	wf->rfcarrier(atoi(cbBox->value())*1000L);
}

void afconoff_cb(Fl_Widget *w, void *vi)
{
	Fl::lock();
	Fl_Light_Button *b = (Fl_Light_Button *)w;
	int v = b->value();
	Fl::unlock();
	active_modem->set_afcOnOff( v ? true : false );
}

void sqlonoff_cb(Fl_Widget *w, void *vi)
{
	Fl::lock();
	Fl_Light_Button *b = (Fl_Light_Button *)w;
	int v = b->value();
	Fl::unlock();
	active_modem->set_sqlchOnOff( v ? true : false );
}


void cb_btnSideband(Fl_Widget *w, void *d)
{
	Fl_Button *b = (Fl_Button *)w;
	Fl::lock();
	progdefaults.btnusb = !progdefaults.btnusb;
	if (progdefaults.btnusb) { 
		b->label("U");
		wf->USB(true);
	} else {
		b->label("L");
		wf->USB(false);
	}
	b->redraw();
	Fl::unlock();
}

void cbMacroTimerButton(Fl_Widget *w, void *d)
{
	Fl_Button *b = (Fl_Button *)w;
	progdefaults.useTimer = false;
	Fl::lock();
	b->hide();
	Fl::unlock();
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
{"Interface", 0, (Fl_Callback*)cb_mnuConfigInterface, 0, 0, FL_NORMAL_LABEL, 0, 14, 0}, // 51
{"Operator", 0, (Fl_Callback*)cb_mnuConfigOperator, 0, 0, FL_NORMAL_LABEL, 0, 14, 0}, // 52
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
{0,0,0,0,0,0,0,0,0}, // 65
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

void create_fl_digi_main() {
	int Y = 0;
	fl_digi_main = new Fl_Double_Window(WNOM, HNOM, "fldigi");
			mnu = new Fl_Menu_Bar(0, 0, WNOM - 142, Hmenu);
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
			inpNotes = new Fl_Input(89 + 22, Y, WNOM - 89 - 44 - 22, Hnotes,"Notes:");
			inpNotes->align(FL_ALIGN_LEFT);
			cboBand  = new Fl_ComboBox(2, Y, 85, Hnotes, "");
			cboBand->hide();
			btnSideband = new Fl_Button(88, Y+1, 22, 22, "U");
			btnSideband->callback(cb_btnSideband, 0);
			btnSideband->hide();
			qsoSave = new Fl_Button(WNOM - 42, Y + 1, 40, Hnotes- 2, "Save");
			qsoSave->callback(qsoSave_cb, 0);
			qsoFrame2->resizable(inpNotes);
		qsoFrame2->end();
		Y += Hnotes;
		
		int sw = 15;
		Fl_Group *MixerFrame = new Fl_Group(0,Y,sw, Hrcvtxt + Hxmttxt);
//			valRcvMixer = new Fl_Slider(0, Y, sw, (Hrcvtxt + Hxmttxt)/2 - 15, "R");
			valRcvMixer = new Fl_Slider(0, Y, sw, (Htext)/2, "");
			valRcvMixer->type(FL_VERT_NICE_SLIDER);
			valRcvMixer->color(fl_rgb_color(0,110,30));
			valRcvMixer->labeltype(FL_ENGRAVED_LABEL);
			valRcvMixer->selection_color(fl_rgb_color(255,255,0));
			valRcvMixer->range(1.0,0.0);
			valRcvMixer->callback( (Fl_Callback *)cb_RcvMixer);
//			valXmtMixer = new Fl_Slider(0, Y + (Hrcvtxt + Hxmttxt)/2, sw, (Hrcvtxt + Hxmttxt)/2 - 15, "T");
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

		Fl_Tile *TiledGroup = new Fl_Tile(sw, Y, WNOM-sw, Htext);
            int minRxHeight = Hrcvtxt;
            int minTxHeight;
            if (minRxHeight < 66) minRxHeight = 66;
            minTxHeight = Htext - minRxHeight;

			Fl_Box *minbox = new Fl_Box(sw,Y + 66, WNOM-sw, Htext - 66 - 32);
			minbox->hide();

			ReceiveText = new TextView(sw, Y, WNOM-sw, minRxHeight, "");
		
			FHdisp = new Raster(sw, Y, WNOM-sw, minRxHeight);
			FHdisp->hide();
			Y += minRxHeight;

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
				sldrSquelch->value(20);
				sldrSquelchValue = 20.0;
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
			MODEstatus->tooltip("Open Modem Tab");
			Status1 = new Fl_Box(Wmode,Hmenu+Hrcvtxt+Hxmttxt+Hwfall, Ws2n, Hstatus, "");
			Status1->box(FL_DOWN_BOX);
			Status1->color(FL_BACKGROUND2_COLOR);
			Status1->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
			Status2 = new Fl_Box(Wmode+Ws2n, Hmenu+Hrcvtxt+Hxmttxt+Hwfall, Wimd, Hstatus, "");
			Status2->box(FL_DOWN_BOX);
			Status2->color(FL_BACKGROUND2_COLOR);
			Status2->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
			StatusBar = new Fl_Box(Wmode+Wimd+Ws2n, Hmenu+Hrcvtxt+Hxmttxt+Hwfall, Wstatus, Hstatus, "");
			StatusBar->box(FL_DOWN_BOX);
			StatusBar->color(FL_BACKGROUND2_COLOR);
			StatusBar->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
			WARNstatus = new Fl_Box(Wmode+Wimd+Ws2n+Wstatus, Hmenu+Hrcvtxt+Hxmttxt+Hwfall, Wwarn, Hstatus, "");
			WARNstatus->box(FL_DIAMOND_DOWN_BOX);
			WARNstatus->color(FL_BACKGROUND_COLOR);
			WARNstatus->labelcolor(FL_RED);
			WARNstatus->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE);
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
				
			Fl_Group::current()->resizable(StatusBar);
		hpack->end();

		fl_digi_main->size_range(WNOM, HNOM);
	fl_digi_main->end();
	fl_digi_main->callback(cb_wMain);
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
	Fl::lock();
	pgrsSquelch->value(metric);
	Fl::unlock();
	Fl::awake();
}

void put_cwRcvWPM(double wpm)
{
//	if (!prgsCWrcvWPM) return;
	int U = progdefaults.CWupperlimit;
	int L = progdefaults.CWlowerlimit;
	double dWPM = 100.0*(wpm - L)/(U - L);
	Fl::lock();
	prgsCWrcvWPM->value(dWPM);
	valCWrcvWPM->value((int)wpm);
	Fl::unlock();
	Fl::awake();
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
	static bool nulinepending = false;
	const char **asc = ascii;
	rxmsgid = msgget( (key_t) 9876, 0666);
	if (mailclient || mailserver || rxmsgid != -1)
		asc = ascii2;
	if (data == '\r') {
		ReceiveText->add(asc['\n' & 0x7F],1);
		nulinepending = true;
	} else if (nulinepending && data == '\r') {
		ReceiveText->add(asc['\n' & 0x7F],1);
	} else if (nulinepending && data == '\n') {
		nulinepending = false;
	} else if (nulinepending && data != '\n') {
		ReceiveText->add(asc[data & 0x7F], 1);
		nulinepending = false;
	} else {
		ReceiveText->add(asc[data & 0x7F],1);
	}
	if ( rxmsgid != -1) {
		rxmsgst.msg_type = 1;
		rxmsgst.c = data & 0x7F;
		msgsnd (rxmsgid, (void *)&rxmsgst, 1, IPC_NOWAIT);
	}

	if (Maillogfile)
		Maillogfile->log_to_file(cLogfile::LOG_RX, asc[data & 0x7F]);

	if (logging)
		logfile->log_to_file(cLogfile::LOG_RX, asc[data & 0x7F]);
}

string strSecText = "";

void put_sec_char( char chr )
{
	if (chr >= ' ' && chr <= 'z') {
		strSecText.append(1, chr);
		if (strSecText.length() > 60)
			strSecText.erase(0,1);
		Fl::lock();
		StatusBar->label(strSecText.c_str());
		Fl::unlock();
		Fl::awake();
	}
}

void put_status(const char *msg)
{
	if (!msg) return;
	Fl::lock();
	StatusBar->label(msg);
	Fl::unlock();
	Fl::awake();
}

void put_Status2(char *msg)
{
	if (!msg) return;
	if (strlen(msg) > 60) msg[60] = 0;
	Fl::lock();
	Status2->label(msg);
	Fl::unlock();
	Fl::awake();
}

void put_Status1(char *msg)
{
	if (!msg) return;
	if (strlen(msg) > 60) msg[60] = 0;
	Fl::lock();
	Status1->label(msg);
	Fl::unlock();
	Fl::awake();
}

void put_WARNstatus(bool on)
{
	Fl::lock();
	if (on)
		WARNstatus->color(FL_RED);
	else
		WARNstatus->color(FL_BACKGROUND_COLOR);
	WARNstatus->redraw();
	Fl::unlock();
}

void set_CWwpm()
{
	Fl::lock();
	sldrCWxmtWPM->value(progdefaults.CWspeed);
	Fl::unlock();
}

void clear_StatusMessages()
{
	Fl::lock();
	StatusBar->label("");
	Status1->label("");
	Status2->label("");
	WARNstatus->label("");
	Fl::unlock();
	Fl::awake();
}

	
void put_MODEstatus(trx_mode mode)
{
	Fl::lock();
	MODEstatus->label(mode_names[mode]);
	Fl::unlock();
	Fl::awake();
}

void put_rx_data(int *data, int len)
{
	FHdisp->data(data, len);
}

char get_tx_char(void)
{
	char chr;
	static bool lfpending = false;
	static bool ctlpending = false;

	if (pskmail_text_available == true)
		return pskmail_get_char();
	
	if (lfpending == true) {
		lfpending = false;
		return '\n';
	}
	chr = TransmitText->nextChar();
	
	if (chr == '\n') {
		lfpending = true;
		return '\r';
	}
	if (ctlpending == true) {
		switch (chr) {
		case 0x00: 
			break;
		case 'r':
		case 'R' :
			chr = 0x03;
			ctlpending = false;
			TransmitText->clear();
			break;
		case '^' :
			ctlpending = false;
			break;
		default :
			ctlpending = false;
			chr = 0x00;
		}
		return chr;
	}
	if (chr == '^') {
		ctlpending = true;
		chr = 0x00;
	}
	return chr;
}


void put_echo_char(unsigned int data)
{
	static bool nulinepending = false;
	const char **asc = ascii;
	if (mailclient || mailserver || arqmode)
		asc = ascii2;
	if (data == '\r' && nulinepending) // reject multiple CRs
		return;
	if (data == '\r') nulinepending = true;
	if (nulinepending && data == '\n') {
		nulinepending = false;
	}
	ReceiveText->add(asc[data & 0x7F], 4);
	if (Maillogfile)
		Maillogfile->log_to_file(cLogfile::LOG_TX, asc[data & 0x7F]);
	if (logging)
		logfile->log_to_file(cLogfile::LOG_TX, asc[data & 0x7F]);
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
	Fl::lock();
	if (on) {
		progdefaults.EnableMixer = true;
		mixer.openMixer(progdefaults.MXdevice.c_str());

		mixer.PCMVolume(progdefaults.PCMvolume/100.0);
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
	Fl::unlock();
}

void resetMixerControls()
{
    if (progdefaults.EnableMixer) {
	    valRcvMixer->activate();
	    valXmtMixer->activate();
	    menuMix->activate();
	    btnLineIn->activate();
	    btnMicIn->activate();
	    valPCMvolume->activate();
    }
    else {
	    valRcvMixer->deactivate();
	    valXmtMixer->deactivate();
	    menuMix->deactivate();
	    btnLineIn->deactivate();
	    btnMicIn->deactivate();
	    valPCMvolume->deactivate();
    }
}

void setPCMvolume(double vol)
{
	mixer.PCMVolume(vol/100.0);
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
	enableMixer(false);
	trx_reset(scDevice.c_str());
	progdefaults.SCdevice = scDevice;
        if (progdefaults.EnableMixer)
            enableMixer(true);
}

void setReverse(int rev) {
	active_modem->set_reverse(rev);
}

void setAfcOnOff(bool b) {
	Fl::lock();
	afconoff->value(b);
	Fl::unlock();
	Fl::awake();
}	

void setSqlOnOff(bool b) {
	Fl::lock();
	sqlonoff->value(b);
	Fl::unlock();
	Fl::awake();
}

bool QueryAfcOnOff() {
	Fl::lock();
	int v = afconoff->value();
	Fl::unlock();
	return v;
}

bool QuerySqlOnOff() {
	Fl::lock();
	int v = sqlonoff->value();
	Fl::unlock();
	return v;
}


