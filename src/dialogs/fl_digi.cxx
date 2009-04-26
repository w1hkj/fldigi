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

#include <config.h>

#include <sys/types.h>

#ifdef __WOE32__
#  ifdef __CYGWIN__
#    include <w32api/windows.h>
#  else
#    include <windows.h>
#  endif
#endif

#include <cstdlib>
#include <cstdarg>
#include <string>
#include <algorithm>

#include "gettext.h"
#include "fl_digi.h"

#include <FL/Fl.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Pixmap.H>
#include <FL/Fl_Image.H>
#include <FL/Fl_Tile.H>
#include <FL/x.H>
#include <FL/Fl_Help_Dialog.H>
#include <FL/Fl_Progress.H>
#include <FL/Fl_Tooltip.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Multiline_Input.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Pack.H>

#include "waterfall.h"
#include "raster.h"
#include "progress.h"
#include "rigdialog.h"

#include "main.h"
#include "threads.h"
#include "trx.h"
#if USE_HAMLIB
	#include "hamlib.h"
#endif
#include "rigio.h"
#include "rigMEM.h"
#include "psk.h"
#include "cw.h"
#include "mfsk.h"
#include "mt63.h"
#include "rtty.h"
#include "olivia.h"
#include "thor.h"
#include "dominoex.h"
#include "feld.h"
#include "throb.h"
#include "wwv.h"
#include "analysis.h"

#include "ascii.h"
#include "globals.h"
#include "misc.h"
//#include "help.h"

#include "confdialog.h"
#include "configuration.h"
#include "colorsfonts.h"
#include "status.h"

#include "macros.h"
#include "macroedit.h"
#include "logger.h"
#include "lookupcall.h"

#include "font_browser.h"

#include "icons.h"

#include "status.h"

#include "rigsupport.h"

#include "qrunner.h"

#include "Viewer.h"
#include "soundconf.h"

#include "htmlstrings.h"
#if USE_XMLRPC
#	include "xmlrpc.h"
#endif
#if BENCHMARK_MODE
#	include "benchmark.h"
#endif
#include "debug.h"
#include "re.h"
#include "network.h"
#include "spot.h"
#include "dxcc.h"
#include "locator.h"

#include "logbook.h"

#include "rx_extract.h"
#include "speak.h"

using namespace std;

Fl_Double_Window	*fl_digi_main=(Fl_Double_Window *)0;
Fl_Help_Dialog 		*help_dialog = (Fl_Help_Dialog *)0;
Fl_Double_Window	*scopeview = (Fl_Double_Window *)0;

MixerBase* mixer = 0;

//bool	useCheckButtons;

Fl_Group			*mnuFrame;
Fl_Menu_Bar 		*mnu;

Fl_Light_Button		*btnAutoSpot = (Fl_Light_Button *)0;
Fl_Light_Button		*btnTune = (Fl_Light_Button *)0;
Fl_Light_Button		*btnRSID = (Fl_Light_Button *)0;
Fl_Button		*btnMacroTimer;

Fl_Tile_check		*TiledGroup = 0;
FTextView			*ReceiveText = 0;
FTextEdit			*TransmitText = 0;
Raster				*FHdisp;
Fl_Box				*StatusBar = (Fl_Box *)0;
Fl_Box				*Status2 = (Fl_Box *)0;
Fl_Box				*Status1 = (Fl_Box *)0;
Fl_Box				*WARNstatus = (Fl_Box *)0;
Fl_Button			*MODEstatus = (Fl_Button *)0;
Fl_Button 			*btnMacro[NUMMACKEYS];
Fl_Button			*btnAltMacros;
Fl_Button			*btn_afconoff;
Fl_Button			*btn_sqlonoff;
Fl_Input2			*inpFreq;
Fl_Input2			*inpTimeOff;
Fl_Input2			*inpTimeOn;
Fl_Button           *btnTimeOn;
Fl_Input2			*inpCall;
Fl_Input2			*inpName;
Fl_Input2			*inpRstIn;
Fl_Input2			*inpRstOut;
Fl_Input2			*inpQth;
Fl_Input2			*inpLoc;
Fl_Input2			*inpState;
Fl_Input2			*inpCountry;
Fl_Input2			*inpSerNo;
Fl_Input2			*outSerNo;
Fl_Input2			*inpXchgIn;
Fl_Box				*lblDup;
Fl_Input2			*inpVEprov;
Fl_Input2			*inpNotes;
Fl_Input2			*inpAZ;	// WA5ZNU
Fl_Button			*qsoTime;
Fl_Button			*qsoClear;
Fl_Button			*qsoSave;
Fl_Box				*txtRigName = (Fl_Box *)0;
cFreqControl 		*qsoFreqDisp = (cFreqControl *)0;
Fl_ComboBox			*qso_opMODE = (Fl_ComboBox *)0;
Fl_ComboBox			*qso_opBW = (Fl_ComboBox *)0;
Fl_Button			*qso_opPICK = (Fl_Button *)0;

Fl_Group			*TopFrame = (Fl_Group *)0;
Fl_Group			*RigControlFrame = (Fl_Group *)0;
Fl_Group			*RigViewerFrame = (Fl_Group *)0;
Fl_Group			*QsoInfoFrame = (Fl_Group *)0;
Fl_Group			*QsoInfoFrame1 = (Fl_Group *)0;
Fl_Group			*QsoInfoFrame1A = (Fl_Group *)0;
Fl_Group			*QsoInfoFrame1B = (Fl_Group *)0;
Fl_Group			*QsoInfoFrameLeft = (Fl_Group *)0;
Fl_Group			*QsoInfoFrameCenter = (Fl_Group *)0;
Fl_Group			*QsoInfoFrameRight = (Fl_Group *)0;
Fl_Group			*QsoInfoFrame2 = (Fl_Group *)0;
Fl_Group			*QsoButtonFrame = (Fl_Group *)0;

Fl_Browser			*qso_opBrowser = (Fl_Browser *)0;
Fl_Button			*qso_btnAddFreq = (Fl_Button *)0;
Fl_Button			*qso_btnSelFreq = (Fl_Button *)0;
Fl_Button			*qso_btnDelFreq = (Fl_Button *)0;
Fl_Button			*qso_btnClearList = (Fl_Button *)0;
Fl_Button			*qso_btnAct = 0;
Fl_Input2			*qso_inpAct = 0;

Fl_Button			*btnQRZ;
Fl_Group			*MixerFrame;
Fl_Value_Slider			*valRcvMixer;
Fl_Value_Slider			*valXmtMixer;

#define FREQWIDTH 172  // FREQWIDTH should be a multiple of 9 + 10
#define FREQHEIGHT 30
#define BTNWIDTH 30
int pad = 1;
int x_qsoframe = BTNWIDTH;	
			
int w_inpFreq	= 80;
int w_inpTime	= 40;
int w_inpCall	= 120;
int w_inpName  	= 90;
int w_inpRstIn	= 30;
int w_inpRstOut = 30;
int w_SerNo	= 40;
int wf1 = pad + w_inpFreq + pad + 2*w_inpTime +  pad + w_inpCall + 
          pad + w_inpName + pad + w_inpRstIn + pad + w_inpRstOut + pad;

int w_fm1 		= 25;
int w_fm2 		= 15;
int w_fm3 		= 15;
int w_fm4 		= 25;
int w_fm5 		= 25;
int w_fm6		= 30;
int w_fm7       = 35;
int w_inpState 	= 25;
int w_inpProv	= 25;
int w_inpCountry = 60;
int w_inpLOC   	= 55;
int w_inpAZ    	= 30;
int w_inpQth 	= wf1 - w_fm1 - w_fm2 - w_fm3 - w_fm4 - w_fm5 - w_fm6 -
                  w_inpState - w_inpProv - w_inpLOC - w_inpAZ - w_inpCountry;
int w_Xchg      = wf1 - 2*w_fm7 - w_fm5 - 2*pad - 2 * w_SerNo;

int qh = Hqsoframe / 2;
int rig_control_width = FREQWIDTH + 4;

int IMAGE_WIDTH;
int Hwfall;
int HNOM;
// WNOM must be large enough to contain ALL of the horizontal widgets 
// when the main dialog is initially created.
int WNOM = 650;//progStatus.mainW ? progStatus.mainW : WMIN;
int Wwfall;

int					altMacros = 0;
bool				bSaveFreqList = false;
string				strMacroName[NUMMACKEYS];


waterfall			*wf = (waterfall *)0;
Digiscope			*digiscope = (Digiscope *)0;
Digiscope			*wfscope = (Digiscope *)0;

Fl_Slider			*sldrSquelch = (Fl_Slider *)0;
Progress			*pgrsSquelch = (Progress *)0;

Fl_RGB_Image		*feld_image = 0;
Fl_Pixmap 			*addrbookpixmap = 0;
Fl_Pixmap 			*closepixmap = 0;

#if !defined(__APPLE__) && !defined(__WOE32__)
Pixmap				fldigi_icon_pixmap;
#endif

Fl_Menu_Item *getMenuItem(const char *caption, Fl_Menu_Item* submenu = 0);
bool clean_exit(void);

void cb_init_mode(Fl_Widget *, void *arg);

void cb_oliviaA(Fl_Widget *w, void *arg);
void cb_oliviaB(Fl_Widget *w, void *arg);
void cb_oliviaC(Fl_Widget *w, void *arg);
void cb_oliviaD(Fl_Widget *w, void *arg);
void cb_oliviaCustom(Fl_Widget *w, void *arg);

void cb_rtty45(Fl_Widget *w, void *arg);
void cb_rtty50(Fl_Widget *w, void *arg);
void cb_rtty75(Fl_Widget *w, void *arg);
void cb_rttyCustom(Fl_Widget *w, void *arg);

Fl_Widget *modem_config_tab;
Fl_Menu_Item *quick_change;

Fl_Menu_Item quick_change_psk[] = {
	{ mode_info[MODE_BPSK31].name, 0, cb_init_mode, (void *)MODE_BPSK31 },
	{ mode_info[MODE_PSK63].name, 0, cb_init_mode, (void *)MODE_PSK63 },
	{ mode_info[MODE_PSK125].name, 0, cb_init_mode, (void *)MODE_PSK125 },
	{ mode_info[MODE_PSK250].name, 0, cb_init_mode, (void *)MODE_PSK250 },
	{ 0 }
};

Fl_Menu_Item quick_change_qpsk[] = {
	{ mode_info[MODE_QPSK31].name, 0, cb_init_mode, (void *)MODE_QPSK31 },
	{ mode_info[MODE_QPSK63].name, 0, cb_init_mode, (void *)MODE_QPSK63 },
	{ mode_info[MODE_QPSK125].name, 0, cb_init_mode, (void *)MODE_QPSK125 },
	{ mode_info[MODE_QPSK250].name, 0, cb_init_mode, (void *)MODE_QPSK250 },
	{ 0 }
};

Fl_Menu_Item quick_change_mfsk[] = {
	{ mode_info[MODE_MFSK4].name, 0, cb_init_mode, (void *)MODE_MFSK4 },
	{ mode_info[MODE_MFSK8].name, 0, cb_init_mode, (void *)MODE_MFSK8 },
	{ mode_info[MODE_MFSK16].name, 0, cb_init_mode, (void *)MODE_MFSK16 },
	{ mode_info[MODE_MFSK11].name, 0, cb_init_mode, (void *)MODE_MFSK11 },
	{ mode_info[MODE_MFSK22].name, 0, cb_init_mode, (void *)MODE_MFSK22 },
	{ mode_info[MODE_MFSK31].name, 0, cb_init_mode, (void *)MODE_MFSK31 },
	{ mode_info[MODE_MFSK32].name, 0, cb_init_mode, (void *)MODE_MFSK32 },
	{ mode_info[MODE_MFSK64].name, 0, cb_init_mode, (void *)MODE_MFSK64 },
	{ 0 }
};

Fl_Menu_Item quick_change_mt63[] = {
	{ mode_info[MODE_MT63_500].name, 0, cb_init_mode, (void *)MODE_MT63_500 },
	{ mode_info[MODE_MT63_1000].name, 0, cb_init_mode, (void *)MODE_MT63_1000 },
	{ mode_info[MODE_MT63_2000].name, 0, cb_init_mode, (void *)MODE_MT63_2000 },
	{ 0 }
};

Fl_Menu_Item quick_change_thor[] = {
	{ mode_info[MODE_THOR4].name, 0, cb_init_mode, (void *)MODE_THOR4 },
	{ mode_info[MODE_THOR5].name, 0, cb_init_mode, (void *)MODE_THOR5 },
	{ mode_info[MODE_THOR8].name, 0, cb_init_mode, (void *)MODE_THOR8 },
	{ mode_info[MODE_THOR11].name, 0, cb_init_mode, (void *)MODE_THOR11 },
	{ mode_info[MODE_THOR16].name, 0, cb_init_mode, (void *)MODE_THOR16 },
	{ mode_info[MODE_THOR22].name, 0, cb_init_mode, (void *)MODE_THOR22 },
	{ 0 }
};

Fl_Menu_Item quick_change_domino[] = {
	{ mode_info[MODE_DOMINOEX4].name, 0, cb_init_mode, (void *)MODE_DOMINOEX4 },
	{ mode_info[MODE_DOMINOEX5].name, 0, cb_init_mode, (void *)MODE_DOMINOEX5 },
	{ mode_info[MODE_DOMINOEX8].name, 0, cb_init_mode, (void *)MODE_DOMINOEX8 },
	{ mode_info[MODE_DOMINOEX11].name, 0, cb_init_mode, (void *)MODE_DOMINOEX11 },
	{ mode_info[MODE_DOMINOEX16].name, 0, cb_init_mode, (void *)MODE_DOMINOEX16 },
	{ mode_info[MODE_DOMINOEX22].name, 0, cb_init_mode, (void *)MODE_DOMINOEX22 },
	{ 0 }
};

Fl_Menu_Item quick_change_feld[] = {
	{ mode_info[MODE_FELDHELL].name, 0, cb_init_mode, (void *)MODE_FELDHELL },
	{ mode_info[MODE_SLOWHELL].name, 0, cb_init_mode, (void *)MODE_SLOWHELL },
	{ mode_info[MODE_HELLX5].name,   0, cb_init_mode, (void *)MODE_HELLX5 },
	{ mode_info[MODE_HELLX9].name,   0, cb_init_mode, (void *)MODE_HELLX9 },
	{ mode_info[MODE_FSKHELL].name,  0, cb_init_mode, (void *)MODE_FSKHELL },
	{ mode_info[MODE_FSKH105].name,  0, cb_init_mode, (void *)MODE_FSKH105 },
	{ mode_info[MODE_HELL80].name,   0, cb_init_mode, (void *)MODE_HELL80 },
	{ 0 }
};

Fl_Menu_Item quick_change_throb[] = {
	{ mode_info[MODE_THROB1].name, 0, cb_init_mode, (void *)MODE_THROB1 },
	{ mode_info[MODE_THROB2].name, 0, cb_init_mode, (void *)MODE_THROB2 },
	{ mode_info[MODE_THROB4].name, 0, cb_init_mode, (void *)MODE_THROB4 },
	{ mode_info[MODE_THROBX1].name, 0, cb_init_mode, (void *)MODE_THROBX1 },
	{ mode_info[MODE_THROBX2].name, 0, cb_init_mode, (void *)MODE_THROBX2 },
	{ mode_info[MODE_THROBX4].name, 0, cb_init_mode, (void *)MODE_THROBX4 },
	{ 0 }
};

Fl_Menu_Item quick_change_olivia[] = {
	{ "8/250", 0, cb_oliviaD, (void *)MODE_OLIVIA },
	{ "8/500", 0, cb_oliviaA, (void *)MODE_OLIVIA },
	{ "16/500", 0, cb_oliviaB, (void *)MODE_OLIVIA },
	{ "32/1000", 0, cb_oliviaC, (void *)MODE_OLIVIA },
	{ _("Custom..."), 0, cb_oliviaCustom, (void *)MODE_OLIVIA },
	{ 0 }
};

Fl_Menu_Item quick_change_rtty[] = {
	{ "RTTY-45", 0, cb_rtty45, (void *)MODE_RTTY },
	{ "RTTY-50", 0, cb_rtty50, (void *)MODE_RTTY },
	{ "RTTY-75", 0, cb_rtty75, (void *)MODE_RTTY },
	{ _("Custom..."), 0, cb_rttyCustom, (void *)MODE_RTTY },
	{ 0 }
};

void set_olivia_tab_widgets()
{
	mnuOlivia_Bandwidth->value(progdefaults.oliviabw);
	mnuOlivia_Tones->value(progdefaults.oliviatones);
}

void cb_oliviaA(Fl_Widget *w, void *arg)
{
	progdefaults.oliviatones = 2;
	progdefaults.oliviabw = 2;
	set_olivia_tab_widgets();
	cb_init_mode(w, arg);
}

void cb_oliviaB(Fl_Widget *w, void *arg)
{
	progdefaults.oliviatones = 3;
	progdefaults.oliviabw = 2;
	set_olivia_tab_widgets();
	cb_init_mode(w, arg);
}

void cb_oliviaC(Fl_Widget *w, void *arg)
{
	progdefaults.oliviatones = 4;
	progdefaults.oliviabw = 3;
	set_olivia_tab_widgets();
	cb_init_mode(w, arg);
}

void cb_oliviaD(Fl_Widget *w, void *arg)
{
	progdefaults.oliviatones = 2;
	progdefaults.oliviabw = 1;
	set_olivia_tab_widgets();
	cb_init_mode(w, arg);
}

void cb_oliviaCustom(Fl_Widget *w, void *arg)
{
	modem_config_tab = tabOlivia;
	tabsConfigure->value(tabModems);
	tabsModems->value(modem_config_tab);
	dlgConfig->show();
	cb_init_mode(w, arg);	
}

void set_rtty_tab_widgets()
{
	progdefaults.rtty_parity = 0;
	progdefaults.rtty_stop = 1;
	selShift->value(progdefaults.rtty_shift);
	selBits->value(progdefaults.rtty_bits);
	selBaud->value(progdefaults.rtty_baud);
	selParity->value(progdefaults.rtty_parity);
	selStopBits->value(progdefaults.rtty_stop);
}

void cb_rtty45(Fl_Widget *w, void *arg)
{
	progdefaults.rtty_baud = 1;
	progdefaults.rtty_bits = 0;
	progdefaults.rtty_shift = 3;
	set_rtty_tab_widgets();
	cb_init_mode(w, arg);	
}

void cb_rtty50(Fl_Widget *w, void *arg)
{
	progdefaults.rtty_baud = 2;
	progdefaults.rtty_bits = 0;
	progdefaults.rtty_shift = 3;
	set_rtty_tab_widgets();
	cb_init_mode(w, arg);	
}

void cb_rtty75(Fl_Widget *w, void *arg)
{
	progdefaults.rtty_baud = 4;
	progdefaults.rtty_bits = 0;
	progdefaults.rtty_shift = 9;
	set_rtty_tab_widgets();
	cb_init_mode(w, arg);	
}

void cb_rttyCustom(Fl_Widget *w, void *arg)
{
	modem_config_tab = tabRTTY;
	tabsConfigure->value(tabModems);
	tabsModems->value(modem_config_tab);
	dlgConfig->show();
	cb_init_mode(w, arg);
}

static void busy_cursor(void*)
{
	Fl::first_window()->cursor(FL_CURSOR_WAIT);
}
static void default_cursor(void*)
{
	Fl::first_window()->cursor(FL_CURSOR_DEFAULT);
}

void startup_modem(modem *m)
{
	trx_start_modem(m);
#if BENCHMARK_MODE
	return;
#endif

	restoreFocus();

	FL_LOCK_D();
	if (m == feld_modem ||
		m == feld_slowmodem ||
		m == feld_x5modem ||
		m == feld_x9modem ||
		m == feld_FMmodem ||
		m == feld_FM105modem ||
		m == feld_80modem ) {
		ReceiveText->hide();
		FHdisp->show();
		sldrHellBW->value(progdefaults.HELL_BW);
	} else {
		ReceiveText->show();
		FHdisp->hide();
	}
	if (m == rtty_modem) {
	    sldrRTTYbandwidth->value(progdefaults.RTTY_BW);
    }
    if (m >= psk31_modem && m <= qpsk250_modem)
        m->set_sigsearch(SIGSEARCH);

	if (m->get_cap() & modem::CAP_AFC) {
		btn_afconoff->value(progStatus.afconoff);
		btn_afconoff->activate();
	}
	else {
		btn_afconoff->value(0);
		btn_afconoff->deactivate();
	}

	wf->btnRev->value(wf->Reverse());
	if (m->get_cap() & modem::CAP_REV) {
		wf->btnRev->value(wf->Reverse());
		wf->btnRev->activate();
	}
	else {
		wf->btnRev->value(0);
		wf->btnRev->deactivate();
	}

	FL_UNLOCK_D();
	FL_AWAKE_D();

}

void cb_mnuOpenMacro(Fl_Menu_*, void*) {
	if (macros.changed) {
		switch (fl_choice2(_("Save changed macros?"), _("Cancel"), _("Save"), _("Don't save"))) {
		case 0:
			return;
		case 1:
			macros.saveMacroFile();
			// fall through
		case 2:
			break;
		}
	}
	macros.openMacroFile();
	macros.changed = false;
	restoreFocus();
}

void cb_mnuSaveMacro(Fl_Menu_*, void*) {
	macros.saveMacroFile();
	restoreFocus();
}

void cb_E(Fl_Menu_*, void*) {
	fl_digi_main->do_callback();
}

void cb_wMain(Fl_Widget*, void*)
{
	if (!clean_exit())
		return;
	// hide all shown windows
	Fl::first_window(fl_digi_main);
	for (Fl_Window* w = Fl::next_window(fl_digi_main); w; w = Fl::next_window(w)) {
		w->do_callback();
		w = fl_digi_main;
	}
	// this will make Fl::run return
	fl_digi_main->hide();
}

void init_modem(trx_mode mode)
{
	ENSURE_THREAD(FLMAIN_TID);

#if !BENCHMARK_MODE
       quick_change = 0;
       modem_config_tab = tabsModems->child(0);
#endif

	switch (mode) {
	case MODE_NEXT:
		if ((mode = active_modem->get_mode() + 1) == NUM_MODES)
			mode = 0;
		return init_modem(mode);
	case MODE_PREV:
		if ((mode = active_modem->get_mode() - 1) < 0)
			mode = NUM_MODES - 1;
		return init_modem(mode);

	case MODE_CW:
		startup_modem(*mode_info[mode].modem ? *mode_info[mode].modem :
			      *mode_info[mode].modem = new cw);
		modem_config_tab = tabCW;
		break;

	case MODE_THOR4: case MODE_THOR5: case MODE_THOR8:
	case MODE_THOR11:case MODE_THOR16: case MODE_THOR22: 
		startup_modem(*mode_info[mode].modem ? *mode_info[mode].modem :
			      *mode_info[mode].modem = new thor(mode));
		quick_change = quick_change_thor;
		modem_config_tab = tabTHOR;
		break;

	case MODE_DOMINOEX4: case MODE_DOMINOEX5: case MODE_DOMINOEX8:
	case MODE_DOMINOEX11: case MODE_DOMINOEX16: case MODE_DOMINOEX22:
		startup_modem(*mode_info[mode].modem ? *mode_info[mode].modem :
			      *mode_info[mode].modem = new dominoex(mode));
		quick_change = quick_change_domino;
		modem_config_tab = tabDomEX;
		break;

	case MODE_FELDHELL:
	case MODE_SLOWHELL:
	case MODE_HELLX5:
	case MODE_HELLX9: 
	case MODE_FSKHELL: 
	case MODE_FSKH105: 
	case MODE_HELL80:
		startup_modem(*mode_info[mode].modem ? *mode_info[mode].modem :
			      *mode_info[mode].modem = new feld(mode));
		quick_change = quick_change_feld;
		modem_config_tab = tabFeld;
		break;

	case MODE_MFSK4:
	case MODE_MFSK11: 
	case MODE_MFSK22:
	case MODE_MFSK31:
	case MODE_MFSK64:
	case MODE_MFSK8: 
	case MODE_MFSK16: 
	case MODE_MFSK32:
		startup_modem(*mode_info[mode].modem ? *mode_info[mode].modem :
			      *mode_info[mode].modem = new mfsk(mode));
		quick_change = quick_change_mfsk;
		break;

	case MODE_MT63_500: case MODE_MT63_1000: case MODE_MT63_2000 :
		startup_modem(*mode_info[mode].modem ? *mode_info[mode].modem :
			      *mode_info[mode].modem = new mt63(mode));
		quick_change = quick_change_mt63;
		modem_config_tab = tabMT63;
		break;

	case MODE_BPSK31: case MODE_PSK63: case MODE_PSK125: case MODE_PSK250:
		startup_modem(*mode_info[mode].modem ? *mode_info[mode].modem :
			      *mode_info[mode].modem = new psk(mode));
		quick_change = quick_change_psk;
		modem_config_tab = tabPSK;
		break;
	case MODE_QPSK31: case MODE_QPSK63: case MODE_QPSK125: case MODE_QPSK250:
		startup_modem(*mode_info[mode].modem ? *mode_info[mode].modem :
			      *mode_info[mode].modem = new psk(mode));
		quick_change = quick_change_qpsk;
		modem_config_tab = tabPSK;
		break;

	case MODE_OLIVIA:
		startup_modem(*mode_info[mode].modem ? *mode_info[mode].modem :
			      *mode_info[mode].modem = new olivia);
		modem_config_tab = tabOlivia;
		quick_change = quick_change_olivia;
		break;

	case MODE_RTTY:
		startup_modem(*mode_info[mode].modem ? *mode_info[mode].modem :
			      *mode_info[mode].modem = new rtty(mode));
		modem_config_tab = tabRTTY;
		quick_change = quick_change_rtty;
		break;

	case MODE_THROB1: case MODE_THROB2: case MODE_THROB4:
	case MODE_THROBX1: case MODE_THROBX2: case MODE_THROBX4:
		startup_modem(*mode_info[mode].modem ? *mode_info[mode].modem :
			      *mode_info[mode].modem = new throb(mode));
		quick_change = quick_change_throb;
		break;

	case MODE_WWV:
		startup_modem(*mode_info[mode].modem ? *mode_info[mode].modem :
			      *mode_info[mode].modem = new wwv);
		break;

	case MODE_ANALYSIS:
		startup_modem(*mode_info[mode].modem ? *mode_info[mode].modem :
			      *mode_info[mode].modem = new anal);
		break;

	default:
		LOG_ERROR("Unknown mode: %" PRIdPTR, mode);
		return init_modem(MODE_BPSK31);
	}

#if BENCHMARK_MODE
	return;
#endif

	clear_StatusMessages();
	progStatus.lastmode = mode;
	
	if (wf->xmtlock->value() == 1 && !mailserver) {
		wf->xmtlock->value(0);
		wf->xmtlock->damage();
		active_modem->set_freqlock(false);
	}		
}

void init_modem_sync(trx_mode m)
{
	ENSURE_THREAD(FLMAIN_TID);

	if (trx_state != STATE_RX)
		TRX_WAIT(STATE_RX, abort_tx());

	TRX_WAIT(STATE_RX, init_modem(m));
	REQ_FLUSH(TRX_TID);
}

void cb_init_mode(Fl_Widget *, void *mode)
{
	init_modem(reinterpret_cast<trx_mode>(mode));
}


void restoreFocus(Fl_Widget* w)
{
	// if w is not NULL, give focus to TransmitText only if the last event
	// was an Enter keypress
	if (!w)
		TransmitText->take_focus();
	else if (Fl::event() == FL_KEYBOARD) {
		int k = Fl::event_key();
		if (k == FL_Enter || k == FL_KP_Enter)
			TransmitText->take_focus();
	}
}

void macro_cb(Fl_Widget *w, void *v)
{
	int b = (int)(reinterpret_cast<long> (v));
	b += altMacros * NUMMACKEYS;
	int mouse = Fl::event_button();
	if (mouse == FL_LEFT_MOUSE && !macros.text[b].empty()) {
		stopMacroTimer();
		macros.execute(b);
	}
	else if (mouse == FL_RIGHT_MOUSE)
		editMacro(b);
	restoreFocus();
}

void colorize_macro(int i) 
{
	if (progdefaults.useGroupColors == true) {
		if (i < NUMKEYROWS){
			btnMacro[i]->color(fl_rgb_color(
				progdefaults.btnGroup1.R, 
				progdefaults.btnGroup1.G, 
				progdefaults.btnGroup1.B));
		} else if (i < 8) {
			btnMacro[i]->color(fl_rgb_color(
				progdefaults.btnGroup2.R, 
				progdefaults.btnGroup2.G, 
				progdefaults.btnGroup2.B));
		} else {
			btnMacro[i]->color(fl_rgb_color(
				progdefaults.btnGroup3.R, 
				progdefaults.btnGroup3.G, 
				progdefaults.btnGroup3.B));
		}
		btnMacro[i]->labelcolor(
			fl_rgb_color(
				progdefaults.btnFkeyTextColor.R,
				progdefaults.btnFkeyTextColor.G,
				progdefaults.btnFkeyTextColor.B ));
	} else {
		btnMacro[i]->color(FL_BACKGROUND_COLOR);
		btnMacro[i]->labelcolor(FL_FOREGROUND_COLOR);
	}
}

void colorize_macros()
{
	FL_LOCK_D();
	for (int i = 0; i < NUMMACKEYS; i++) {
		colorize_macro(i);
		btnMacro[i]->redraw_label();
	}
	FL_UNLOCK_D();
}

void altmacro_cb(Fl_Widget *w, void *v)
{
	static char alt_text[NUMKEYROWS];

	intptr_t arg = reinterpret_cast<intptr_t>(v);
	if (arg)
		altMacros += arg;
	else
		altMacros = altMacros + (Fl::event_button() == FL_RIGHT_MOUSE ? -1 : 1);
	altMacros = WCLAMP(altMacros, 0, 3);

	snprintf(alt_text, sizeof(alt_text), "%d", altMacros + 1);
	FL_LOCK_D();
	for (int i = 0; i < NUMMACKEYS; i++)
		btnMacro[i]->label(macros.name[i + (altMacros * NUMMACKEYS)].c_str());
	btnAltMacros->label(alt_text);
	btnAltMacros->redraw_label();
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

void cb_mnuConfigID(Fl_Menu_*, void*) {
	progdefaults.loadDefaults();
	tabsConfigure->value(tabID);
	dlgConfig->show();
}

void cb_mnuConfigQRZ(Fl_Menu_*, void*) {
	progdefaults.loadDefaults();
	tabsConfigure->value(tabQRZ);
	dlgConfig->show();
}

void cb_mnuConfigMisc(Fl_Menu_*, void*) {
	progdefaults.loadDefaults();
	tabsConfigure->value(tabMisc);
	dlgConfig->show();
}

void cb_mnuUI(Fl_Menu_*, void *) {
	progdefaults.loadDefaults();
	tabsConfigure->value(tabUI);
	dlgConfig->show();
}

void cb_mnuConfigContest(Fl_Menu_*, void*) {
	progdefaults.loadDefaults();
	tabsConfigure->value(tabUI);
	tabsUI->value(tabContest);
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

void cb_logfile(Fl_Widget* w, void*)
{
	progStatus.LOGenabled = reinterpret_cast<Fl_Menu_*>(w)->mvalue()->value();
	if (progStatus.LOGenabled == true) {
    	Date tdy;
	    string lfname = HomeDir;
	    lfname.append("fldigi");
	    lfname.append(tdy.szDate(2));
	    lfname.append(".log");
	   	logfile = new cLogfile(lfname);
    	logfile->log_to_file_start();
    } else {
        logfile->log_to_file_stop();
        delete logfile;
        logfile = 0;
    }
}

#if USE_SNDFILE
bool capval = false;
bool genval = false;
bool playval = false;
void cb_mnuCapture(Fl_Widget *w, void *d)
{
	if (!scard) return;
	Fl_Menu_Item *m = getMenuItem(((Fl_Menu_*)w)->mvalue()->label()); //eek
	if (playval || genval) {
		m->clear();
		return;
	}
	capval = m->value();
	if(!scard->Capture(capval)) {
		m->clear();
		capval = false;
	}
}

void cb_mnuGenerate(Fl_Widget *w, void *d)
{
	if (!scard) return;
	Fl_Menu_Item *m = getMenuItem(((Fl_Menu_*)w)->mvalue()->label());
	if (capval || playval) {
		m->clear();
		return;
	}
	genval = m->value();
	if (!scard->Generate(genval)) {
		m->clear();
		genval = false;
	}
}

void cb_mnuPlayback(Fl_Widget *w, void *d)
{
	if (!scard) return;
	Fl_Menu_Item *m = getMenuItem(((Fl_Menu_*)w)->mvalue()->label());
	if (capval || genval) {
		m->clear();
		return;
	}
	playval = m->value();
	if(!scard->Playback(playval)) {
		m->clear();
		playval = false;
	}
	else if (btnAutoSpot->value()) {
		put_status(_("Spotting disabled"), 3.0);
		btnAutoSpot->value(0);
		btnAutoSpot->do_callback();
	}
}
#endif // USE_SNDFILE

void cb_mnuConfigFonts(Fl_Menu_*, void *) {
	selectColorsFonts();
}

void cb_mnuSaveConfig(Fl_Menu_ *, void *) {
	progdefaults.saveDefaults();
	restoreFocus();
}

// This function may be called by the QRZ thread
void cb_mnuVisitURL(Fl_Widget*, void* arg)
{
	const char* url = reinterpret_cast<const char *>(arg);
#ifndef __WOE32__
	const char* browsers[] = {
		getenv("FLDIGI_BROWSER"),
#  ifdef __APPLE__
		"open"
#  else
		"xdg-open", getenv("BROWSER"), "sensible-brower", "firefox", "mozilla"
#  endif
	};
	switch (fork()) {
	case 0:
#  ifndef NDEBUG
		unsetenv("MALLOC_CHECK_");
		unsetenv("MALLOC_PERTURB_");
#  endif
		for (size_t i = 0; i < sizeof(browsers)/sizeof(browsers[0]); i++)
			if (browsers[i])
				execlp(browsers[i], browsers[i], url, (char*)0);
		exit(EXIT_FAILURE);
	case -1:
		fl_alert2(_("Could not run a web browser:\n%s\n\n"
			 "Open this URL manually:\n%s"),
			 strerror(errno), url);
	}
#else
	// gurgle... gurgle... HOWL
	// "The return value is cast as an HINSTANCE for backward
	// compatibility with 16-bit Windows applications. It is
	// not a true HINSTANCE, however. The only thing that can
	// be done with the returned HINSTANCE is to cast it to an
	// int and compare it with the value 32 or one of the error
	// codes below." (Error codes omitted to preserve sanity).
	if ((int)ShellExecute(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL) <= 32)
		fl_alert2(_("Could not open url:\n%s\n"), url);
#endif
}

void cb_mnuVisitPSKRep(Fl_Widget*, void*)
{
	cb_mnuVisitURL(0, (void*)string("http://pskreporter.info/pskmap?").append(progdefaults.myCall).c_str());
}

void html_help( const string &Html)
{
	if (!help_dialog)
		help_dialog = new Fl_Help_Dialog;
	help_dialog->value(Html.c_str());
	help_dialog->show();
	restoreFocus();
}

void cb_mnuBeginnersURL(Fl_Widget*, void*)
{
	string deffname = HelpDir;
	deffname.append("beginners.html");
	ofstream f(deffname.c_str());
	if (!f)
		return;
	f << szBeginner;
	f.close();
#ifndef __WOE32__
	cb_mnuVisitURL(NULL, (void *)deffname.insert(0, "file://").c_str());
#else
	cb_mnuVisitURL(NULL, (void *)deffname.c_str());
#endif
}

void cb_mnuCheckUpdate(Fl_Widget*, void*)
{
	struct {
		const char* url;
		const char* re;
		string version_str;
		unsigned long version;
	} sites[] = {
		{ PACKAGE_DL, "fldigi-distro/fldigi-([0-9.]+).tar.gz", "", 0 },
		{ PACKAGE_PROJ, "fldigi/fldigi-([0-9.]+).tar.gz", "", 0 }
	}, *latest;
	string reply;

	put_status(_("Checking for updates..."));
	for (size_t i = 0; i < sizeof(sites)/sizeof(*sites); i++) { // fetch .url, grep for .re
		reply.clear();
		if (!fetch_http_gui(sites[i].url, reply, 20.0, busy_cursor, 0, default_cursor, 0))
			continue;
		re_t re(sites[i].re, REG_EXTENDED | REG_ICASE | REG_NEWLINE);
		if (!re.match(reply.c_str()) || re.nsub() != 2)
			continue;

		sites[i].version = ver2int((sites[i].version_str = re.submatch(1)).c_str());
	}
	put_status("");

	latest = sites[1].version > sites[0].version ? &sites[1] : &sites[0];
	if (sites[0].version == 0 && sites[1].version == 0) {
		fl_alert2(_("Could not check for updates:\n%s"), reply.c_str());
		return;
	}
	if (latest->version > ver2int(PACKAGE_VERSION)) {
		switch (fl_choice2(_("Version %s is available at\n\n%s\n\nWhat would you like to do?"),
				  _("Close"), _("Visit URL"), _("Copy URL"),
				  latest->version_str.c_str(), latest->url)) {
		case 1:
			cb_mnuVisitURL(NULL, (void*)latest->url);
			break;
		case 2:
			size_t n = strlen(latest->url);
			Fl::copy(latest->url, n, 0);
			Fl::copy(latest->url, n, 1);
		}
	}
	else
		fl_message2(_("You are running the latest version"));
}

void cb_mnuAboutURL(Fl_Widget*, void*)
{
	if (!help_dialog)
		help_dialog = new Fl_Help_Dialog;
	help_dialog->value(szAbout);
	help_dialog->resize(help_dialog->x(), help_dialog->y(), help_dialog->w(), 440);
	help_dialog->show();
	restoreFocus();
}

void fldigi_help(const string& theHelp)
{
	string htmlHelp = 
"<HTML>"
"<HEAD>"
"<TITLE>" PACKAGE " Help</TITLE>"
"</HEAD>"
"<BODY>"
"<FONT FACE=fixed>"
"<P><TT>";

	for (size_t i = 0; i < theHelp.length(); i++) {
		if (theHelp[i] == '\n') {
			if (theHelp[i+1] == '\n') {
				htmlHelp += "</TT></P><P><TT>";
				i++;
			}
			else
				htmlHelp += "<BR>";
		} else if (theHelp[i] == ' ' && theHelp[i+1] == ' ') {
			htmlHelp += "&nbsp;&nbsp;";
			i++;
		} else
			htmlHelp += theHelp[i];
	}
	htmlHelp += 
"</TT></P>"
"</BODY>"
"</HTML>";
	html_help(htmlHelp);
}

void cb_mnuCmdLineHelp(Fl_Widget*, void*)
{
	extern string option_help;
	fldigi_help(option_help);
	restoreFocus();
}

void cb_mnuBuildInfo(Fl_Widget*, void*)
{
	extern string build_text;
	fldigi_help(build_text);
	restoreFocus();
}

void cb_mnuDebug(Fl_Widget*, void*)
{
	debug::show();
}

#ifndef NDEBUG
void cb_mnuFun(Fl_Widget*, void*)
{
        fl_message2(_("Sunspot creation underway!"));
}
#endif

void cb_mnuAudioInfo(Fl_Widget*, void*)
{
        if (progdefaults.btnAudioIOis != SND_IDX_PORT) {
                fl_alert2(_("Audio device information is only available for the PortAudio backend"));
                return;
        }

#if USE_PORTAUDIO
	size_t ndev;
        string devtext[2], headers[2];
	SoundPort::devices_info(devtext[0], devtext[1]);
	if (devtext[0] != devtext[1]) {
		headers[0] = _("Capture device");
		headers[1] = _("Playback device");
		ndev = 2;
	}
	else {
		headers[0] = _("Capture and playback devices");
		ndev = 1;
	}

	string audio_info;
	for (size_t i = 0; i < ndev; i++) {
		audio_info.append("<center><h4>").append(headers[i]).append("</h4>\n<table border=\"1\">\n");

		string::size_type j, n = 0;
		while ((j = devtext[i].find(": ", n)) != string::npos) {
			audio_info.append("<tr>")
				  .append("<td align=\"center\">")
				  .append(devtext[i].substr(n, j-n))
				  .append("</td>");

			if ((n = devtext[i].find('\n', j)) == string::npos) {
				devtext[i] += '\n';
				n = devtext[i].length() - 1;
			}

			audio_info.append("<td align=\"center\">")
				  .append(devtext[i].substr(j+2, n-j-2))
				  .append("</td>")
				  .append("</tr>\n");
		}
		audio_info.append("</table></center><br>\n");
	}

	fldigi_help(audio_info);
#endif
}

void cb_ShowConfig(Fl_Widget*, void*)
{
	cb_mnuVisitURL(0, (void*)HomeDir.c_str());
}

void cbTune(Fl_Widget *w, void *) {
	Fl_Button *b = (Fl_Button *)w;
	if (active_modem == wwv_modem || active_modem == anal_modem) {
		b->value(0);
		return;
	}
	if (b->value() == 1) {
		b->labelcolor(FL_RED);
		trx_tune();
	} else {
		b->labelcolor(FL_FOREGROUND_COLOR);
		trx_receive();
	}
	restoreFocus();
}

void cbRSID(Fl_Widget *w, void *) {
	if (trx_state == STATE_TX || trx_state == STATE_TUNE) {
		btnRSID->value(0);
		return;
	}
	if (progdefaults.rsid == true) {
		progdefaults.rsid = false;
		wf->xmtrcv->activate();
		btnTune->activate();
	} else {
		ReedSolomon->reset();
		progdefaults.rsid = true;
		wf->xmtrcv->deactivate();
		btnTune->deactivate();
	}
	restoreFocus();
}

void cbAutoSpot(Fl_Widget* w, void*)
{
	progStatus.spot_recv = static_cast<Fl_Light_Button*>(w)->value();
}

void toggleRSID()
{
	btnRSID->value(0);
	cbRSID(NULL, NULL);
}

void cb_mnuDigiscope(Fl_Menu_ *w, void *d) {
	if (scopeview)
		scopeview->show();
}

void cb_mnuRig(Fl_Menu_ *, void *) {
	if (!rigcontrol)
		createRigDialog();
	rigcontrol->show();
}

void cb_mnuViewer(Fl_Menu_ *, void *) {
	openViewer();
}

void cb_mnuContest(Fl_Menu_ *m, void *) {
	if (QsoInfoFrame1A->visible()) {
		QsoInfoFrame1A->hide();
		QsoInfoFrame1B->show();
	} else {
		QsoInfoFrame1B->hide();
		QsoInfoFrame1A->show();
	}
	progStatus.contest = m->mvalue()->value();
}

void cb_mnuPicViewer(Fl_Menu_ *, void *) {
	if (picRxWin) {
		picRx->redraw();
		picRxWin->show();
	}
}

void closeRigDialog() {
	rigcontrol->hide();
}

void cb_sldrSquelch(Fl_Slider* o, void*) {
	progStatus.sldrSquelchValue = o->value();
	restoreFocus();
}


static char ztbuf[14];

const char* zdate(void) { return ztbuf; }
const char* ztime(void) { return ztbuf + 9; }

void ztimer(void* first_call)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);

	double st = 60.0 - tv.tv_sec % 60 - tv.tv_usec / 1e6;
	if (!first_call) {
		tv.tv_sec = (int)(60.0 * round(tv.tv_sec / 60.0));
		if (st < 1.0)
			st += 60.0;
	}
	Fl::repeat_timeout(st, ztimer);

	struct tm tm;
	gmtime_r(&tv.tv_sec, &tm);
	if (!strftime(ztbuf, sizeof(ztbuf), "%Y%m%d %H%M", &tm))
		memset(ztbuf, 0, sizeof(ztbuf));
	else
		ztbuf[8] = '\0';

	inpTimeOff->value(ztbuf + 9);
}


bool oktoclear = true;

void updateOutSerNo()
{
	if (contest_count.count) {
		char szcnt[10] = "";
		contest_count.Format(progdefaults.ContestDigits, progdefaults.UseLeadingZeros);
		snprintf(szcnt, sizeof(szcnt), contest_count.fmt.c_str(), contest_count.count);
		outSerNo->value(szcnt);
	} else
		outSerNo->value("");
}

string old_call;
string new_call;

void clearQSO()
{
	Fl_Input* in[] = { 
		inpCall, inpName, inpTimeOn, inpRstIn, inpRstOut,
		inpQth, inpLoc, inpAZ, inpState, inpVEprov, inpCountry,
		inpSerNo, outSerNo, inpXchgIn, inpNotes };
	for (size_t i = 0; i < sizeof(in)/sizeof(*in); i++)
		in[i]->value("");
	if (progdefaults.fixed599) {
		inpRstIn->value("599");
		inpRstOut->value("599");
	}
	updateOutSerNo();
	inpSearchString->value ("");
	lblDup->hide();
	old_call.clear();
	new_call.clear();
}

void cb_ResetSerNbr()
{
	contest_count.count = progdefaults.ContestStart;
	updateOutSerNo();
}

void cb_btnTimeOn(Fl_Widget* w, void*)
{
	inpTimeOn->value(inpTimeOff->value(), inpTimeOff->size());
	sDate_on = zdate();
	restoreFocus();
}

void cb_loc(Fl_Widget* w, void*)
{
	if ((oktoclear = !inpLoc->size()) || !progdefaults.autofill_qso_fields)
		return restoreFocus(w);

	double lon[2], lat[2], distance, azimuth;
	if (locator2longlat(&lon[0], &lat[0], progdefaults.myLocator.c_str()) == RIG_OK &&
	    locator2longlat(&lon[1], &lat[1], inpLoc->value()) == RIG_OK &&
	    qrb(lon[0], lat[0], lon[1], lat[1], &distance, &azimuth) == RIG_OK) {
		char az[4];
		snprintf(az, sizeof(az), "%3.0f", azimuth);
		inpAZ->value(az);
	}
	restoreFocus(w);
}

void cb_call(Fl_Widget* w, void*)
{
	if (progdefaults.calluppercase) {
		int pos = inpCall->position();
		char* uc = new char[inpCall->size()];
		transform(inpCall->value(), inpCall->value() + inpCall->size(), uc,
			  static_cast<int (*)(int)>(std::toupper));
		inpCall->value(uc, inpCall->size());
		inpCall->position(pos);
		delete [] uc;
	}
	new_call = inpCall->value();
	
	if (old_call == new_call)
		return restoreFocus(w);
			
	old_call = new_call;
	oktoclear = false;
	
	inpTimeOn->value(inpTimeOff->value(), inpTimeOff->size());
	sDate_on = zdate();
	lblDup->hide();

	if (progdefaults.EnableDupCheck) {
		if (!lblDup->visible())
			DupCheck();
		return restoreFocus(w);
	}

	SearchLastQSO(inpCall->value());

	if (inpAZ->value()[0])
		return restoreFocus(w);
		
	if (!progdefaults.autofill_qso_fields)
		return restoreFocus(w);
	const struct dxcc* e = dxcc_lookup(inpCall->value());
	if (!e)
		return restoreFocus(w);
	double lon, lat, distance, azimuth;
	if (locator2longlat(&lon, &lat, progdefaults.myLocator.c_str()) == RIG_OK &&
	    qrb(lon, lat, -e->longitude, e->latitude, &distance, &azimuth) == RIG_OK) {
		char az[4];
		snprintf(az, sizeof(az), "%3.0f", azimuth);
		inpAZ->value(az, sizeof(az) - 1);
	}
	inpCountry->value(e->country);
	inpCountry->position(0);

	restoreFocus(w);
}

void cb_log(Fl_Widget* w, void*)
{
	Fl_Input2 *inp = (Fl_Input2 *) w;
	if (inp->value()[0])
		oktoclear = false;
	if (progdefaults.EnableDupCheck) {
		lblDup->hide();
		DupCheck();
	}
	restoreFocus(w);
}

void qsoClear_cb(Fl_Widget *b, void *)
{
	bool clearlog = true;
	if (progdefaults.NagMe && !oktoclear)
		clearlog = (fl_choice2(_("Clear log fields?"), _("Cancel"), _("OK"), NULL) == 1);
	if (clearlog) {
		clearQSO();
		oktoclear = true;
	}
	restoreFocus();
}

void qsoSave_cb(Fl_Widget *b, void *)
{
	submit_log();
	if (progdefaults.ClearOnSave)
		clearQSO();
	oktoclear = true;
	restoreFocus();
}

void cb_QRZ(Fl_Widget *b, void *)
{
	if (!*inpCall->value())
		return restoreFocus();

	switch (Fl::event_button()) {
	case FL_LEFT_MOUSE:
		CALLSIGNquery();
		oktoclear = false;
		break;
	case FL_RIGHT_MOUSE:
		if (quick_choice(string("Spot \"").append(inpCall->value()).append("\"?").c_str(),
				 2, "Confirm", "Cancel", NULL) == 1)
			spot_manual(inpCall->value(), inpLoc->value());
		break;
	default:
		break;
	}
	restoreFocus();
}

void status_cb(Fl_Widget *b, void *arg)
{
    if (Fl::event_button() == FL_RIGHT_MOUSE) {
   		progdefaults.loadDefaults();
        tabsConfigure->value(tabModems);
        tabsModems->value(modem_config_tab);
        dlgConfig->show();
    }
    else {
        if (!quick_change)
            return;
        const Fl_Menu_Item *m = quick_change->popup(Fl::event_x(), Fl::event_y());
        if (m && m->callback())
            m->do_callback(0);
    }
	static_cast<Fl_Button*>(b)->clear();
	restoreFocus();
}

void afconoff_cb(Fl_Widget *w, void *vi)
{
	FL_LOCK_D();
	Fl_Button *b = (Fl_Button *)w;
	int v = b->value();
	FL_UNLOCK_D();
	progStatus.afconoff = v;
}

void sqlonoff_cb(Fl_Widget *w, void *vi)
{
	FL_LOCK_D();
	Fl_Button *b = (Fl_Button *)w;
	int v = b->value();
	FL_UNLOCK_D();
	progStatus.sqlonoff = v ? true : false;
}

void startMacroTimer()
{
	ENSURE_THREAD(FLMAIN_TID);

	btnMacroTimer->color(fl_rgb_color(255, 255, 100));
	btnMacroTimer->clear_output();
	Fl::add_timeout(0.0, macro_timer);
}

void stopMacroTimer()
{
	ENSURE_THREAD(FLMAIN_TID);

	progStatus.timer = 0;
	Fl::remove_timeout(macro_timer);

	btnMacroTimer->label(0);
	btnMacroTimer->color(FL_BACKGROUND_COLOR);
	btnMacroTimer->set_output();
}

void macro_timer(void*)
{
	char buf[8];
	snprintf(buf, sizeof(buf), "%d", progStatus.timer);
	btnMacroTimer->copy_label(buf);

	if (progStatus.timer-- == 0) {
		stopMacroTimer();
		macros.execute(progStatus.timerMacro);
	}
	else
		Fl::repeat_timeout(1.0, macro_timer);
}

void cbMacroTimerButton(Fl_Widget*, void*)
{
	stopMacroTimer();
	restoreFocus();
}

void cb_RcvMixer(Fl_Widget *w, void *d)
{
	progStatus.RcvMixer = valRcvMixer->value() / 100.0;
	mixer->setRcvGain(progStatus.RcvMixer);
}

void cb_XmtMixer(Fl_Widget *w, void *d)
{
	progStatus.XmtMixer = valXmtMixer->value() / 100.0;
	mixer->setXmtLevel(progStatus.XmtMixer);
}

int default_handler(int event)
{
	if (event != FL_SHORTCUT)
		return 0;

    if (RigViewerFrame)
    	if (Fl::event_key() == FL_Escape &&
	        Fl::event_inside(RigViewerFrame) && RigViewerFrame->visible()) {
		    CloseQsoView();
		    return 1;
	    }

	Fl_Widget* w = Fl::focus();

	if (w == fl_digi_main || w->window() == fl_digi_main) {
		int key = Fl::event_key();
		if (key == FL_Escape || (key >= FL_F && key <= FL_F_Last)) {
			TransmitText->take_focus();
			TransmitText->handle(FL_KEYBOARD);
			w->take_focus(); // remove this to leave tx text focused
			return 1;
		}
	}
	else if (w == dlgLogbook || w->window() == dlgLogbook)
		return log_search_handler(event);

	return 0;
}

bool clean_exit(void) {
	if (progdefaults.changed) {
		switch (fl_choice2(_("Save changed configuration before exiting?"),
				  _("Cancel"), _("Save"), _("Don't save"))) {
		case 0:
			return false;
		case 1:
			progdefaults.saveDefaults();
			// fall through
		case 2:
			break;
		}
	}
	if (!oktoclear && progdefaults.NagMe) {
		switch (fl_choice2(_("Save log before exiting?"),
				  _("Cancel"), _("Save"), _("Don't save"))) {
		case 0:
			return false;
		case 1:
			qsoSave_cb(0, 0);
			// fall through
		case 2:
			break;
		}
	}
	if (macros.changed) {
		switch (fl_choice2(_("Save changed macros before exiting?"),
				  _("Cancel"), _("Save"), _("Don't save"))) {
		case 0:
			return false;
		case 1:
			macros.saveMacroFile();
			// fall through
		case 2:
			break;
		}
	}
	if (Maillogfile)
		Maillogfile->log_to_file_stop();
	if (logfile)
		logfile->log_to_file_stop();

//	if (bSaveFreqList)
		saveFreqList();

	progStatus.saveLastState();

	delete push2talk;
#if USE_HAMLIB
	hamlib_close();
#endif
	rigCAT_close();
	rigMEM_close();

	if (mixer)
		mixer->closeMixer();

	if (trx_state == STATE_RX || trx_state == STATE_TX || trx_state == STATE_TUNE)
		trx_state = STATE_ABORT;
	else {
		LOG_ERROR("trx in unexpected state %d", trx_state);
		exit(1);
	}
	while (trx_state != STATE_ENDED) {
		REQ_FLUSH(GET_THREAD_ID());
		MilliSleep(10);
	}

	if (dlgConfig) {
		dlgConfig->hide();
		delete cboHamlibRig;
		delete dlgConfig;
	}
	
#if USE_HAMLIB
	if (xcvr) delete xcvr;
#endif

	close_logbook();

	return true;
}

#define LOG_TO_FILE_MLABEL _("Log all RX/TX text")
#define RIGCONTROL_MLABEL _("Rig Control")
#define VIEW_MLABEL _("View")
#define MFSK_IMAGE_MLABEL _("MFSK Image")
#define CONTEST_MLABEL _("Contest")
#define CONTEST_FIELDS_MLABEL _("Contest fields")

Fl_Menu_Item menu_[] = {
{_("&File"), 0,  0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{ make_icon_label(_("Open macros..."), file_open_icon), 0,  (Fl_Callback*)cb_mnuOpenMacro, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{ make_icon_label(_("Save macros..."), save_as_icon), 0,  (Fl_Callback*)cb_mnuSaveMacro, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL, 0, 14, 0},
{ make_icon_label(_("Show config"), folder_open_icon), 0, cb_ShowConfig, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL, 0, 14, 0},

{ make_icon_label(_("Logs")), 0, 0, 0, FL_MENU_DIVIDER | FL_SUBMENU, _FL_MULTI_LABEL, 0, 14, 0},
{ make_icon_label(_("New logbook")), 0, (Fl_Callback*)cb_mnuNewLogbook, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{ make_icon_label(_("Open logbook...")), 0, (Fl_Callback*)cb_mnuOpenLogbook, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{ make_icon_label(_("Save logbook")), 0, (Fl_Callback*)cb_mnuSaveLogbook, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL, 0, 14, 0},
{ make_icon_label(_("Merge ADIF...")), 0, (Fl_Callback*)cb_mnuMergeADIF_log, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL, 0, 14, 0},
{ make_icon_label(_("Export ADIF")), 0, (Fl_Callback*)cb_mnuExportADIF_log, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{ make_icon_label(_("Export Text")), 0, (Fl_Callback*)cb_mnuExportTEXT_log, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{ make_icon_label(_("Export CSV")), 0, (Fl_Callback*)cb_mnuExportCSV_log, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL, 0, 14, 0},
{ make_icon_label(_("Cabrillo Rpt")), 0, (Fl_Callback*)cb_Export_Cabrillo, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL, 0, 14, 0},
{ LOG_TO_FILE_MLABEL, 0, cb_logfile, 0, FL_MENU_TOGGLE, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

#if USE_SNDFILE
{ make_icon_label(_("Audio")), 0, 0, 0, FL_MENU_DIVIDER | FL_SUBMENU, _FL_MULTI_LABEL, 0, 14, 0},
{_("RX capture"),  0, (Fl_Callback*)cb_mnuCapture,  0, FL_MENU_TOGGLE, FL_NORMAL_LABEL, 0, 14, 0},
{_("TX generate"), 0, (Fl_Callback*)cb_mnuGenerate, 0, FL_MENU_TOGGLE, FL_NORMAL_LABEL, 0, 14, 0},
{_("Playback"),    0, (Fl_Callback*)cb_mnuPlayback, 0, FL_MENU_TOGGLE, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},
#endif
{ make_icon_label(_("E&xit"), log_out_icon), 0,  (Fl_Callback*)cb_E, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},
{_("Op &Mode"), 0,  0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},

{ mode_info[MODE_CW].name, 0, cb_init_mode, (void *)MODE_CW, 0, FL_NORMAL_LABEL, 0, 14, 0},

{"DominoEX", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_DOMINOEX4].name, 0, cb_init_mode, (void *)MODE_DOMINOEX4, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_DOMINOEX5].name, 0, cb_init_mode, (void *)MODE_DOMINOEX5, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_DOMINOEX8].name, 0, cb_init_mode, (void *)MODE_DOMINOEX8, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_DOMINOEX11].name, 0, cb_init_mode, (void *)MODE_DOMINOEX11, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_DOMINOEX16].name, 0, cb_init_mode, (void *)MODE_DOMINOEX16, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_DOMINOEX22].name, 0, cb_init_mode, (void *)MODE_DOMINOEX22, 0, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{"Hell", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_FELDHELL].name, 0, cb_init_mode, (void *)MODE_FELDHELL, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_SLOWHELL].name, 0,  cb_init_mode, (void *)MODE_SLOWHELL, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_HELLX5].name, 0,  cb_init_mode, (void *)MODE_HELLX5, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_HELLX9].name, 0,  cb_init_mode, (void *)MODE_HELLX9, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_FSKHELL].name, 0, cb_init_mode, (void *)MODE_FSKHELL, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_FSKH105].name, 0, cb_init_mode, (void *)MODE_FSKH105, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_HELL80].name, 0, cb_init_mode, (void *)MODE_HELL80, 0, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{"MFSK", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_MFSK4].name, 0,  cb_init_mode, (void *)MODE_MFSK4, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_MFSK8].name, 0,  cb_init_mode, (void *)MODE_MFSK8, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_MFSK11].name, 0,  cb_init_mode, (void *)MODE_MFSK11, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_MFSK16].name, 0,  cb_init_mode, (void *)MODE_MFSK16, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_MFSK22].name, 0,  cb_init_mode, (void *)MODE_MFSK22, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_MFSK31].name, 0,  cb_init_mode, (void *)MODE_MFSK31, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_MFSK32].name, 0,  cb_init_mode, (void *)MODE_MFSK32, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_MFSK64].name, 0,  cb_init_mode, (void *)MODE_MFSK64, 0, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{"MT63", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_MT63_500].name, 0,  cb_init_mode, (void *)MODE_MT63_500, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_MT63_1000].name, 0,  cb_init_mode, (void *)MODE_MT63_1000, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_MT63_2000].name, 0,  cb_init_mode, (void *)MODE_MT63_2000, 0, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{"Olivia", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{ "8/250", 0, cb_oliviaD, (void *)MODE_OLIVIA, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ "8/500", 0, cb_oliviaA, (void *)MODE_OLIVIA, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ "16/500", 0, cb_oliviaB, (void *)MODE_OLIVIA, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ "32/1000", 0, cb_oliviaC, (void *)MODE_OLIVIA, FL_MENU_DIVIDER, FL_NORMAL_LABEL, 0, 14, 0},
{ _("Custom..."), 0, cb_oliviaCustom, (void *)MODE_OLIVIA, 0, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{"PSK", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_BPSK31].name, 0, cb_init_mode, (void *)MODE_BPSK31, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_QPSK31].name, 0, cb_init_mode, (void *)MODE_QPSK31, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_PSK63].name, 0, cb_init_mode, (void *)MODE_PSK63, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_QPSK63].name, 0, cb_init_mode, (void *)MODE_QPSK63, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_PSK125].name, 0, cb_init_mode, (void *)MODE_PSK125, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_QPSK125].name, 0, cb_init_mode, (void *)MODE_QPSK125, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_PSK250].name, 0, cb_init_mode, (void *)MODE_PSK250, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_QPSK250].name, 0, cb_init_mode, (void *)MODE_QPSK250, 0, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{"RTTY", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{ "RTTY-45", 0, cb_rtty45, (void *)MODE_RTTY, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ "RTTY-50", 0, cb_rtty50, (void *)MODE_RTTY, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ "RTTY-75", 0, cb_rtty75, (void *)MODE_RTTY, FL_MENU_DIVIDER, FL_NORMAL_LABEL, 0, 14, 0},
{ _("Custom..."), 0, cb_rttyCustom, (void *)MODE_RTTY, 0, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{"THOR", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_THOR4].name, 0, cb_init_mode, (void *)MODE_THOR4, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_THOR5].name, 0, cb_init_mode, (void *)MODE_THOR5, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_THOR8].name, 0, cb_init_mode, (void *)MODE_THOR8, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_THOR11].name, 0, cb_init_mode, (void *)MODE_THOR11, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_THOR16].name, 0, cb_init_mode, (void *)MODE_THOR16, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_THOR22].name, 0, cb_init_mode, (void *)MODE_THOR22, 0, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{"Throb", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_THROB1].name, 0, cb_init_mode, (void *)MODE_THROB1, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_THROB2].name, 0, cb_init_mode, (void *)MODE_THROB2, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_THROB4].name, 0, cb_init_mode, (void *)MODE_THROB4, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_THROBX1].name, 0, cb_init_mode, (void *)MODE_THROBX1, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_THROBX2].name, 0, cb_init_mode, (void *)MODE_THROBX2, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_THROBX4].name, 0, cb_init_mode, (void *)MODE_THROBX4, 0, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{"NBEMS modes", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_DOMINOEX11].name, 0, cb_init_mode, (void *)MODE_DOMINOEX11, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_DOMINOEX22].name, 0, cb_init_mode, (void *)MODE_DOMINOEX22, FL_MENU_DIVIDER, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_MFSK16].name, 0,  cb_init_mode, (void *)MODE_MFSK16, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_MFSK32].name, 0,  cb_init_mode, (void *)MODE_MFSK32, FL_MENU_DIVIDER, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_PSK125].name, 0, cb_init_mode, (void *)MODE_PSK125, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_PSK250].name, 0, cb_init_mode, (void *)MODE_PSK250, 0, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{ mode_info[MODE_WWV].name, 0, cb_init_mode, (void *)MODE_WWV, 0, FL_NORMAL_LABEL, 0, 14, 0},

{ mode_info[MODE_ANALYSIS].name, 0, cb_init_mode, (void *)MODE_ANALYSIS, 0, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{_("Configure"), 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{ make_icon_label(_("Operator"), system_users_icon), 0, (Fl_Callback*)cb_mnuConfigOperator, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{ make_icon_label(_("Colors && Fonts"), preferences_desktop_font_icon), 0, (Fl_Callback*)cb_mnuConfigFonts, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{ make_icon_label(_("User Interface")), 0,  (Fl_Callback*)cb_mnuUI, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{ make_icon_label(_("Waterfall"), waterfall_icon), 0,  (Fl_Callback*)cb_mnuConfigWaterfall, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL, 0, 14, 0},
{ make_icon_label(_("Modems"), emblems_system_icon), 0, (Fl_Callback*)cb_mnuConfigModems, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{ make_icon_label(RIGCONTROL_MLABEL, multimedia_player_icon), 0, (Fl_Callback*)cb_mnuConfigRigCtrl, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{ make_icon_label(_("Sound Card"), audio_card_icon), 0, (Fl_Callback*)cb_mnuConfigSoundCard, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL, 0, 14, 0},
{ make_icon_label(_("IDs")), 0,  (Fl_Callback*)cb_mnuConfigID, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{ make_icon_label(_("Misc")), 0,  (Fl_Callback*)cb_mnuConfigMisc, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{ make_icon_label(CONTEST_MLABEL), 0,  (Fl_Callback*)cb_mnuConfigContest, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{ make_icon_label(_("QRZ"), net_icon), 0,  (Fl_Callback*)cb_mnuConfigQRZ, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL, 0, 14, 0},
{ make_icon_label(_("Save Config"), save_icon), 0, (Fl_Callback*)cb_mnuSaveConfig, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{ VIEW_MLABEL, 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{ make_icon_label(_("Digiscope"), utilities_system_monitor_icon), 0, (Fl_Callback*)cb_mnuDigiscope, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{ make_icon_label(MFSK_IMAGE_MLABEL, image_icon), 0, (Fl_Callback*)cb_mnuPicViewer, 0, FL_MENU_INACTIVE, _FL_MULTI_LABEL, 0, 14, 0},
{ make_icon_label(_("PSK Browser")), 0, (Fl_Callback*)cb_mnuViewer, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{ make_icon_label(RIGCONTROL_MLABEL, multimedia_player_icon), 0, (Fl_Callback*)cb_mnuRig, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{ make_icon_label(_("Logbook")), 0, (Fl_Callback*)cb_mnuShowLogbook, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{ CONTEST_FIELDS_MLABEL, 0, (Fl_Callback*)cb_mnuContest, 0, FL_MENU_TOGGLE, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{"     ", 0, 0, 0, FL_MENU_INACTIVE, FL_NORMAL_LABEL, 0, 14, 0},
{_("Help"), 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
#ifndef NDEBUG
// settle the gmfsk vs fldigi argument once and for all
{ make_icon_label(_("Create sunspots"), weather_clear_icon), 0, cb_mnuFun, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL, 0, 14, 0},
#endif
{ make_icon_label(_("Beginners' Guide"), start_here_icon), 0, cb_mnuBeginnersURL, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{ make_icon_label(_("Online documentation..."), help_browser_icon), 0, cb_mnuVisitURL, (void *)PACKAGE_DOCS, 0, _FL_MULTI_LABEL, 0, 14, 0},
{ make_icon_label(_("Fldigi web site..."), net_icon), 0, cb_mnuVisitURL, (void *)PACKAGE_HOME, 0, _FL_MULTI_LABEL, 0, 14, 0},
{ make_icon_label(_("Reception reports..."), pskr_icon), 0, cb_mnuVisitPSKRep, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL, 0, 14, 0},
{ make_icon_label(_("Command line options"), utilities_terminal_icon), 0, cb_mnuCmdLineHelp, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{ make_icon_label(_("Audio device info"), audio_card_icon), 0, cb_mnuAudioInfo, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{ make_icon_label(_("Build info"), executable_icon), 0, cb_mnuBuildInfo, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{ make_icon_label(_("Event log"), dialog_information_icon), 0, cb_mnuDebug, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL, 0, 14, 0},
{ make_icon_label(_("Check for updates..."), system_software_update_icon), 0, cb_mnuCheckUpdate, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{ make_icon_label(_("About"), help_about_icon), 0, cb_mnuAboutURL, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{"  ", 0, 0, 0, FL_MENU_INACTIVE, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},
};


Fl_Menu_Item *getMenuItem(const char *caption, Fl_Menu_Item* submenu)
{
	if (submenu == 0 || !(submenu->flags & FL_SUBMENU))
		submenu = menu_;

	int size = submenu->size() - 1;
	Fl_Menu_Item *item = 0;
	const char* label;
	for (int i = 0; i < size; i++) {
		label = (submenu[i].labeltype() == _FL_MULTI_LABEL) ?
			get_icon_label_text(&submenu[i]) : submenu[i].text;
		if (label && !strcmp(label, caption)) {
			item = submenu + i;
			break;
		}
	}
	if (!item)
		LOG_ERROR("FIXME: could not find menu \"%s\"", caption);
	return item;
}

void activate_rig_menu_item(bool b)
{
	Fl_Menu_Item *rig = getMenuItem(RIGCONTROL_MLABEL, getMenuItem(VIEW_MLABEL));
	if (!rig)
		return;
	if (b) {
		bSaveFreqList = true;
		rig->show();
		
	} else {
		rig->hide();
		if (rigcontrol)
			rigcontrol->hide();
	}
	mnu->redraw();
}

void activate_mfsk_image_item(bool b)
{
	Fl_Menu_Item *mfsk_item = getMenuItem(MFSK_IMAGE_MLABEL);
	if (mfsk_item)
		set_active(mfsk_item, b);
}

#if !defined(__APPLE__) && !defined(__WOE32__)
void make_pixmap(Pixmap *xpm, const char **data)
{
	// We need a displayed window to provide a GC for X_CreatePixmap
	Fl_Window w(0, 0, PACKAGE_NAME);
	w.xclass(PACKAGE_NAME);
	w.border(0);
	w.show();

	Fl_Pixmap icon(data);
	int maxd = MAX(icon.w(), icon.h());
	w.make_current();
	*xpm = fl_create_offscreen(maxd, maxd);
	w.hide();

	fl_begin_offscreen(*xpm);
	fl_color(FL_BACKGROUND_COLOR);
	fl_rectf(0, 0, maxd, maxd);
	icon.draw(maxd - icon.w(), maxd - icon.h());
	fl_end_offscreen();
}
#endif

int rightof(Fl_Widget* w)
{
	int a = w->align();
	if (a == FL_ALIGN_CENTER || a & FL_ALIGN_INSIDE)
		return w->x() + w->w();

	fl_font(FL_HELVETICA, FL_NORMAL_SIZE);
	int lw = static_cast<int>(ceil(fl_width(w->label())));

	if (a & (FL_ALIGN_TOP | FL_ALIGN_BOTTOM)) {
		if (a & FL_ALIGN_LEFT)
			return w->x() + MAX(w->w(), lw);
		else if (a & FL_ALIGN_RIGHT)
			return w->x() + w->w();
		else
			return w->x() + ((lw > w->w()) ? (lw - w->w())/2 : w->w());
	}
	else
		return w->x() + w->w() + lw;
}

int leftof(Fl_Widget* w)
{
	int a = w->align();
	if (a == FL_ALIGN_CENTER || a & FL_ALIGN_INSIDE)
		return w->x();

	fl_font(FL_HELVETICA, FL_NORMAL_SIZE);
	int lw = static_cast<int>(ceil(fl_width(w->label())));

	if (a & (FL_ALIGN_TOP | FL_ALIGN_BOTTOM)) {
		if (a & FL_ALIGN_LEFT)
			return w->x();
		else if (a & FL_ALIGN_RIGHT)
			return w->x() - (lw > w->w() ? lw - w->w() : 0);
		else
			return w->x() - (lw > w->w() ? (lw - w->w())/2 : 0);
	}
	else {
		if (a & FL_ALIGN_LEFT)
			return w->x() - lw;
		else
			return w->x();
	}
}

int above(Fl_Widget* w)
{
	int a = w->align();
	if (a == FL_ALIGN_CENTER || a & FL_ALIGN_INSIDE)
		return w->y();

	return (a & FL_ALIGN_TOP) ? w->y() + FL_NORMAL_SIZE : w->y();
}

int below(Fl_Widget* w)
{
	int a = w->align();
	if (a == FL_ALIGN_CENTER || a & FL_ALIGN_INSIDE)
		return w->y() + w->h();

	return (a & FL_ALIGN_BOTTOM) ? w->y() + w->h() + FL_NORMAL_SIZE : w->y() + w->h();
}

string main_window_title;
void update_main_title()
{
	main_window_title = PACKAGE_TARNAME " - ";
	main_window_title += (progdefaults.myCall.empty() ? _("NO CALLSIGN SET") : progdefaults.myCall.c_str());
	if (fl_digi_main != NULL)
		fl_digi_main->label(main_window_title.c_str());
}

void showOpBrowserView(Fl_Widget *, void *)
{
	if (RigViewerFrame->visible())
		return CloseQsoView();
	QsoInfoFrame1->hide();
	QsoInfoFrame2->hide();
	QsoButtonFrame->hide();
	RigViewerFrame->show();
	qso_opPICK->image(closepixmap);
	qso_opPICK->redraw_label();
	qso_opPICK->tooltip(_("Close List"));
}

void CloseQsoView()
{
	RigViewerFrame->hide();
	QsoInfoFrame1->show();
	QsoInfoFrame2->show();
	QsoButtonFrame->show();
	qso_opPICK->image(addrbookpixmap);
	qso_opPICK->redraw_label();
	qso_opPICK->tooltip(_("Open List"));
}

void cb_qso_btnSelFreq(Fl_Widget *, void *)
{
	qso_selectFreq();
}

void cb_qso_btnDelFreq(Fl_Widget *, void *)
{
	qso_delFreq();
}

void cb_qso_btnAddFreq(Fl_Widget *, void *)
{
	qso_addFreq();
}

void cb_qso_btnClearList(Fl_Widget *, void *)
{
	if (quick_choice("Clear list?", 2, "Confirm", "Cancel", NULL) == 1)
		clearList();
}

void cb_qso_inpAct(Fl_Widget*, void*)
{
	string data, url;
	data.reserve(128);
	url = "http://pskreporter.info/cgi-bin/psk-freq.pl";
	if (qso_inpAct->size())
		url.append("?grid=").append(qso_inpAct->value());
	else if (progdefaults.myLocator.length() > 2)
		url.append("?grid=").append(progdefaults.myLocator, 0, 2);

	string::size_type i;
	if (!fetch_http_gui(url, data, 10.0, busy_cursor, 0, default_cursor, 0) ||
	    (i = data.find("\r\n\r\n")) == string::npos) {
		LOG_ERROR("Error while fetching \"%s\": %s", url.c_str(), data.c_str());
		return;
	}

	i += strlen("\r\n\r\n");
	re_t re("([[:digit:]]{6,}) [[:digit:]]+ ([[:digit:]]+)[[:space:]]+", REG_EXTENDED);

	const size_t menu_max = 8;
	Fl_Menu_Item menu[menu_max + 1];
	string str[menu_max];
	size_t j = 0;
	memset(menu, 0, sizeof(menu));

	while (re.match(data.c_str() + i) && j < menu_max) {
		i += re.submatch(0).length();
		str[j].assign(re.submatch(1)).append(" (").append(re.submatch(2)).
			append(" ").append(atoi(re.submatch(2).c_str()) == 1 ? _("report") : _("reports")).append(")");
		menu[j].label(str[j].c_str());
		menu[++j].label(NULL);
	}

	if ((i = data.find(" grid ", i)) != string::npos)
		data.assign(data, i + strlen(" grid"), 3);
	else
		data = " (?)";
	if (j)
		data.insert(0, _("Recent activity for grid"));
	else
		data = "No recent activity";

	if ((j = quick_choice_menu(data.c_str(), 1, menu)))
		qsy(strtoll(str[j - 1].erase(str[j - 1].find(' ')).c_str(), NULL, 10));
}

void cb_qso_opBrowser(Fl_Browser*, void*)
{
	if (!qso_opBrowser->value())
		return;

	switch (Fl::event_button()) {
	case FL_LEFT_MOUSE:
		if (Fl::event_clicks()) { // double click
			qso_selectFreq();
			CloseQsoView();
		}
		break;
	case FL_RIGHT_MOUSE:
		qso_setFreq();
		break;
	case FL_MIDDLE_MOUSE:
		qso_delFreq();
		qso_addFreq();
		break;
	default:
		break;
	}
}

void show_frequency(long long freq)
{
	if (progdefaults.docked_rig_control)
		qsoFreqDisp->value(freq);
	else
		FreqDisp->value(freq); // REQ is built in to the widget
}

void show_mode(const string& sMode)
{
	if (progdefaults.docked_rig_control)
		REQ_SYNC(&Fl_ComboBox::put_value, qso_opMODE, sMode.c_str());
	else
		REQ_SYNC(&Fl_ComboBox::put_value, opMODE, sMode.c_str());
}

void show_bw(const string& sWidth)
{
	if (progdefaults.docked_rig_control)
		REQ_SYNC(&Fl_ComboBox::put_value, qso_opBW, sWidth.c_str());
	else
		REQ_SYNC(&Fl_ComboBox::put_value, opBW, sWidth.c_str());
}


void show_spot(bool v)
{
	if (v) {
		mnu->size(btnAutoSpot->x(), mnu->h());
		btnAutoSpot->value(progStatus.spot_recv);
		btnAutoSpot->show();
	}
	else {
		btnAutoSpot->hide();
		btnAutoSpot->value(v);
		btnAutoSpot->do_callback();
		mnu->size(btnRSID->x(), mnu->h());
	}
	mnu->redraw();
}

void setTabColors()
{
	tabsColors->selection_color(progdefaults.TabsColor);
	tabsConfigure->selection_color(progdefaults.TabsColor);
	tabsUI->selection_color(progdefaults.TabsColor);
	tabsWaterfall->selection_color(progdefaults.TabsColor);
	tabsModems->selection_color(progdefaults.TabsColor);
	tabsCW->selection_color(progdefaults.TabsColor);
	tabsPSK->selection_color(progdefaults.TabsColor);
	tabsRig->selection_color(progdefaults.TabsColor);
	tabsSoundCard->selection_color(progdefaults.TabsColor);
	tabsMisc->selection_color(progdefaults.TabsColor);
	if (dlgConfig->visible()) dlgConfig->redraw();
	if (dlgColorFont->visible()) dlgColorFont->redraw();
}

void showMacroSet() {
	if (progdefaults.DisplayMacroFilename) {
		string Macroset = "\n<<<===== Macro File ";
		Macroset.append(progStatus.LastMacroFile);
		Macroset.append(" Loaded =====>>>\n");
		ReceiveText->add(Macroset.c_str());
	}
}

void create_fl_digi_main() {

	int Y = 0;

	if (progdefaults.docked_rig_control)
		x_qsoframe += rig_control_width;

	IMAGE_WIDTH = progdefaults.wfwidth;
	Hwfall = progdefaults.wfheight;
	HNOM = DEFAULT_HNOM;

	if (progdefaults.docked_scope)	
		Wwfall = WNOM - 2 * BEZEL - Hwfall + 24;
	else
		Wwfall = WNOM - 2 * BEZEL - 2 * DEFAULT_SW;
	
    update_main_title();
    fl_digi_main = new Fl_Double_Window(WNOM, HNOM, main_window_title.c_str());
    
    	mnuFrame = new Fl_Group(0,0,WNOM, Hmenu);
			mnu = new Fl_Menu_Bar(0, 0, WNOM - 150, Hmenu);
			// do some more work on the menu
			for (size_t i = 0; i < sizeof(menu_)/sizeof(menu_[0]); i++) {
				// FL_NORMAL_SIZE may have changed; update the menu items
				if (menu_[i].text) {
					menu_[i].labelsize_ = FL_NORMAL_SIZE;
				}
				// set the icon label for items with the multi label type
				if (menu_[i].labeltype() == _FL_MULTI_LABEL)
					set_icon_label(&menu_[i]);
			}
			mnu->menu(menu_);

			// reset the message dialog font
			fl_message_font(FL_HELVETICA, FL_NORMAL_SIZE);

			// reset the tooltip font
			Fl_Tooltip::font(FL_HELVETICA);
			Fl_Tooltip::size(FL_NORMAL_SIZE);
			Fl_Tooltip::enable(progdefaults.tooltips);

			btnAutoSpot = new Fl_Light_Button(WNOM - 200 - pad, 0, 50, Hmenu, "Spot");
			btnAutoSpot->selection_color(FL_GREEN);
			btnAutoSpot->callback(cbAutoSpot, 0);
			btnAutoSpot->hide();

			btnRSID = new Fl_Light_Button(WNOM - 150 - pad, 0, 50, Hmenu, "RSID");
			btnRSID->selection_color(FL_GREEN);
			btnRSID->callback(cbRSID, 0);
			
			btnTune = new Fl_Light_Button(WNOM - 100 - pad, 0, 50, Hmenu, "TUNE");
			btnTune->selection_color(FL_RED);
			btnTune->callback(cbTune, 0);
			
			btnMacroTimer = new Fl_Button(WNOM - 50 - pad, 0, 50, Hmenu);
			btnMacroTimer->labelcolor(FL_RED);
			btnMacroTimer->callback(cbMacroTimerButton);
			btnMacroTimer->set_output();

			mnuFrame->resizable(mnu);
		mnuFrame->end();
				
		Fl_Group *TopFrame = new Fl_Group(0, Hmenu, WNOM, Hqsoframe + Hnotes);

		if (progdefaults.docked_rig_control) {

			RigControlFrame = new Fl_Group(0, Hmenu,
									rig_control_width, Hqsoframe + Hnotes);

				txtRigName = new Fl_Box(2, Hmenu, FREQWIDTH, Hqsoframe - FREQHEIGHT);
				txtRigName->align(FL_ALIGN_CENTER);
				txtRigName->color(FL_BACKGROUND_COLOR);
				txtRigName->label(_("No rig specified"));
			
				qsoFreqDisp = new cFreqControl(2, Hmenu + Hqsoframe - FREQHEIGHT,
								FREQWIDTH, FREQHEIGHT, "");

				qsoFreqDisp->box(FL_DOWN_BOX);
				qsoFreqDisp->color(FL_BACKGROUND_COLOR);
				qsoFreqDisp->selection_color(FL_BACKGROUND_COLOR);
				qsoFreqDisp->labeltype(FL_NORMAL_LABEL);
				qsoFreqDisp->labelfont(0);
				qsoFreqDisp->labelsize(12);
				qsoFreqDisp->labelcolor(FL_FOREGROUND_COLOR);
				qsoFreqDisp->align(FL_ALIGN_CENTER);
				qsoFreqDisp->when(FL_WHEN_RELEASE);
				qsoFreqDisp->setCallBack(qso_movFreq);
				qsoFreqDisp->SetONOFFCOLOR(
					fl_rgb_color(	progdefaults.FDforeground.R, 
									progdefaults.FDforeground.G, 
									progdefaults.FDforeground.B),
					fl_rgb_color(	progdefaults.FDbackground.R, 
									progdefaults.FDbackground.G, 
									progdefaults.FDbackground.B));	
				qsoFreqDisp->value(145580000);

				Y = Hmenu + Hqsoframe + 1;

				int w_mng = BTNWIDTH;
				int w_pmb = (FREQWIDTH - w_mng) / 2;
			
				qso_opMODE = new Fl_ComboBox(2, Hmenu + Hqsoframe + 1, w_pmb, Hnotes - 2);
				qso_opMODE->box(FL_DOWN_BOX);
				qso_opMODE->color(FL_BACKGROUND2_COLOR);
				qso_opMODE->selection_color(FL_BACKGROUND_COLOR);
				qso_opMODE->labeltype(FL_NORMAL_LABEL);
				qso_opMODE->labelfont(0);
				qso_opMODE->labelsize(14);
				qso_opMODE->labelcolor(FL_FOREGROUND_COLOR);
				qso_opMODE->callback((Fl_Callback*)cb_qso_opMODE);
				qso_opMODE->align(FL_ALIGN_TOP);
				qso_opMODE->when(FL_WHEN_RELEASE);
				qso_opMODE->end();

				qso_opBW = new Fl_ComboBox(rightof(qso_opMODE), Hmenu + Hqsoframe + 1, w_pmb, Hnotes - 2);
				qso_opBW->box(FL_DOWN_BOX);
				qso_opBW->color(FL_BACKGROUND2_COLOR);
				qso_opBW->selection_color(FL_BACKGROUND_COLOR);
				qso_opBW->labeltype(FL_NORMAL_LABEL);
				qso_opBW->labelfont(0);
				qso_opBW->labelsize(14);
				qso_opBW->labelcolor(FL_FOREGROUND_COLOR);
				qso_opBW->callback((Fl_Callback*)cb_qso_opBW);
				qso_opBW->align(FL_ALIGN_TOP);
				qso_opBW->when(FL_WHEN_RELEASE);
				qso_opBW->end();

				qso_opPICK = new Fl_Button(rightof(qso_opBW), Hmenu + Hqsoframe + 1,
				                 w_mng, Hnotes - 2);
				addrbookpixmap = new Fl_Pixmap(address_book_icon);
				closepixmap = new Fl_Pixmap(close_icon);
	 			qso_opPICK->image(addrbookpixmap);
				qso_opPICK->callback(showOpBrowserView, 0);
				qso_opPICK->tooltip(_("Open List"));
			RigControlFrame->resizable(NULL);
			
			RigControlFrame->end();

			int BV_h = Hqsoframe + Hnotes;
			int opB_w = 280;
			int qFV_w = opB_w + 2 * BTNWIDTH + 6;
		
			RigViewerFrame = new Fl_Group(rightof(RigControlFrame), Hmenu, qFV_w, BV_h);
		
				qso_btnSelFreq = new Fl_Button(
									rightof(RigControlFrame), Hmenu + 1, 
									20, Hnotes - 2);
				qso_btnSelFreq->image(new Fl_Pixmap(left_arrow_icon));
				qso_btnSelFreq->tooltip(_("Select"));
				qso_btnSelFreq->callback((Fl_Callback*)cb_qso_btnSelFreq);

   				qso_btnAddFreq = new Fl_Button(
	 								rightof(qso_btnSelFreq) + pad, Hmenu + 1, 
   									20, Hnotes - 2);
				qso_btnAddFreq->image(new Fl_Pixmap(plus_icon));
   				qso_btnAddFreq->tooltip(_("Add current frequency"));
				qso_btnAddFreq->callback((Fl_Callback*)cb_qso_btnAddFreq);

				qso_btnClearList = new Fl_Button(
			   						rightof(RigControlFrame), Hmenu + qh + 1, 
			   						20, Hnotes - 2);
				qso_btnClearList->image(new Fl_Pixmap(trash_icon));
				qso_btnClearList->tooltip(_("Clear list"));
				qso_btnClearList->callback((Fl_Callback*)cb_qso_btnClearList);

				qso_btnDelFreq = new Fl_Button(
				   					rightof(qso_btnClearList) + pad, Hmenu + qh + 1, 
			   						20, Hnotes - 2);
				qso_btnDelFreq->image(new Fl_Pixmap(minus_icon));
				qso_btnDelFreq->tooltip(_("Delete from list"));
				qso_btnDelFreq->callback((Fl_Callback*)cb_qso_btnDelFreq);

				qso_btnAct = new Fl_Button(rightof(RigControlFrame), Hmenu + 2*qh + 1, 20, Hnotes - 2);
				qso_btnAct->image(new Fl_Pixmap(chat_icon));
				qso_btnAct->callback(cb_qso_inpAct);
				qso_btnAct->tooltip("Show active frequencies");

				qso_inpAct = new Fl_Input2(rightof(qso_btnAct) + pad, Hmenu + 2*qh + 1, 20 - 1 + 1, Hnotes - 2);
				qso_inpAct->when(FL_WHEN_ENTER_KEY | FL_WHEN_NOT_CHANGED);
				qso_inpAct->callback(cb_qso_inpAct);
				qso_inpAct->tooltip("Grid prefix for activity list");

				qso_opBrowser = new Fl_Browser(rightof(qso_btnDelFreq) + pad,  Hmenu + 1, opB_w, BV_h - 1 );
			    qso_opBrowser->tooltip(_("Select operating parameters"));
			    qso_opBrowser->callback((Fl_Callback*)cb_qso_opBrowser);
				qso_opBrowser->type(2);
				qso_opBrowser->box(FL_DOWN_BOX);
				qso_opBrowser->labelfont(4);
				qso_opBrowser->labelsize(12);
				qso_opBrowser->textfont(4);
				RigViewerFrame->resizable(NULL);
			
			RigViewerFrame->end();
			RigViewerFrame->hide();

			QsoButtonFrame = new Fl_Group(rightof(RigControlFrame), Hmenu, BTNWIDTH, Hqsoframe + Hnotes);
				btnQRZ = new Fl_Button(rightof(RigControlFrame) + pad, Hmenu + 1,
							BTNWIDTH - 2*pad, qh - pad);
				btnQRZ = new Fl_Button(rightof(RigControlFrame) + pad, Hmenu + 1,
							BTNWIDTH - 2*pad, qh - pad);
	 			btnQRZ->image(new Fl_Pixmap(net_icon));
				btnQRZ->callback(cb_QRZ, 0);
				btnQRZ->tooltip(_("QRZ"));

				qsoClear = new Fl_Button(rightof(RigControlFrame) + pad, Hmenu + qh + 1, 
							BTNWIDTH - 2*pad, qh - pad);
	 			qsoClear->image(new Fl_Pixmap(edit_clear_icon));
				qsoClear->callback(qsoClear_cb, 0);
				qsoClear->tooltip(_("Clear"));
 
				qsoSave = new Fl_Button(rightof(RigControlFrame) + pad, Hmenu + Hqsoframe + 1, 
							BTNWIDTH - 2*pad, qh - pad);
	 			qsoSave->image(new Fl_Pixmap(save_icon));
				qsoSave->callback(qsoSave_cb, 0);
				qsoSave->tooltip(_("Save"));
			QsoButtonFrame->end();

		} else {

			QsoButtonFrame = new Fl_Group(0, Hmenu, BTNWIDTH, Hqsoframe + Hnotes);
				btnQRZ = new Fl_Button(pad, Hmenu + 1,
							BTNWIDTH - 2*pad, qh - pad);
	 			btnQRZ->image(new Fl_Pixmap(net_icon));
				btnQRZ->callback(cb_QRZ, 0);
				btnQRZ->tooltip(_("QRZ"));

				qsoClear = new Fl_Button(pad, Hmenu + qh + 1, 
							BTNWIDTH - 2*pad, qh - pad);
	 			qsoClear->image(new Fl_Pixmap(edit_clear_icon));
				qsoClear->callback(qsoClear_cb, 0);
				qsoClear->tooltip(_("Clear"));
 
				qsoSave = new Fl_Button(pad, Hmenu + Hqsoframe + 1, 
							BTNWIDTH - 2*pad, qh - pad);
	 			qsoSave->image(new Fl_Pixmap(save_icon));
				qsoSave->callback(qsoSave_cb, 0);
				qsoSave->tooltip(_("Save"));
			QsoButtonFrame->end();
		}
					
		int y2 = Hmenu + qh + 1;
		int y3 = Hmenu + Hqsoframe + 1;

		QsoInfoFrame = new Fl_Group(x_qsoframe, Hmenu, 
						WNOM - rightof(QsoButtonFrame) - pad, Hqsoframe + Hnotes);

			QsoInfoFrame1 = new Fl_Group(x_qsoframe, Hmenu, wf1, Hqsoframe + Hnotes);
				
				inpFreq = new Fl_Input2(x_qsoframe + pad, y2, w_inpFreq, qh - pad, _("QSO Freq"));
				inpFreq->type(FL_NORMAL_OUTPUT);
				inpFreq->tooltip("");
				inpFreq->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);

				inpTimeOn = new Fl_Input2(rightof(inpFreq) + pad, y2, w_inpTime, qh - pad, "");
				inpTimeOn->tooltip("");
				inpTimeOn->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);
				inpTimeOn->type(FL_INT_INPUT);
				
				btnTimeOn = new Fl_Button(leftof(inpTimeOn), Hmenu + pad, w_inpTime, qh, _("On"));
				btnTimeOn->align(FL_ALIGN_LEFT | FL_ALIGN_BOTTOM | FL_ALIGN_INSIDE);
				btnTimeOn->tooltip(_("Press to update"));
				btnTimeOn->box(FL_NO_BOX);
				btnTimeOn->callback(cb_btnTimeOn);

				inpTimeOff = new Fl_Input2(rightof(inpTimeOn) + pad, y2, w_inpTime, qh - pad, _("Off"));
				inpTimeOff->tooltip("");
				inpTimeOff->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);
				inpTimeOff->type(FL_NORMAL_OUTPUT);

				inpCall = new Fl_Input2(rightof(inpTimeOff) + pad, y2, w_inpCall, qh - pad, _("Call"));
				inpCall->tooltip("");
				inpCall->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);

				inpName = new Fl_Input2(rightof(inpCall) + pad, y2, w_inpName, qh - pad, _("Name"));
				inpName->tooltip("");
				inpName->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);
			
				inpRstIn = new Fl_Input2(rightof(inpName) + pad, y2, w_inpRstIn, qh - pad, _("In"));
				inpRstIn->tooltip("");
				inpRstIn->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);

				inpRstOut = new Fl_Input2(rightof(inpRstIn) + pad, y2, w_inpRstOut, qh - pad, _("Out"));
				inpRstOut->tooltip("");
				inpRstOut->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);
				
				lblDup = new Fl_Box(rightof(inpCall) - w_inpCall/2 - 40, Hmenu + 1, 80, qh - pad, _("*** DUP ***"));
				lblDup->labelcolor(FL_RED);
				lblDup->hide();
						
				QsoInfoFrame1A = new Fl_Group (x_qsoframe, y3 - 1, wf1, Hnotes); 
					Fl_Box *fm1box = new Fl_Box(x_qsoframe, y3, w_fm1, qh - pad, _("QTH"));
					fm1box->align(FL_ALIGN_INSIDE);
					inpQth = new Fl_Input2( rightof(fm1box), y3, w_inpQth, qh - pad, "");
					inpQth->tooltip(_("City"));
					inpQth->align(FL_ALIGN_INSIDE);
					
					Fl_Box *fm2box = new Fl_Box(rightof(inpQth), y3, w_fm2, qh - pad, _("St"));
					fm2box->align(FL_ALIGN_INSIDE);
					inpState = new Fl_Input2(rightof(fm2box), y3, w_inpState, qh - pad, "");
					inpState->tooltip(_("US State"));
					inpState->align(FL_ALIGN_INSIDE);

					Fl_Box *fm3box = new Fl_Box(rightof(inpState), y3, w_fm3, qh - pad, _("Pr"));
					fm3box->align(FL_ALIGN_INSIDE);
					inpVEprov = new Fl_Input2(rightof(fm3box), y3, w_inpProv, qh - pad, "");
					inpVEprov->tooltip(_("Can. Province"));
					inpVEprov->align(FL_ALIGN_INSIDE);

					Fl_Box *fm11box = new Fl_Box(rightof(inpVEprov), y3, w_fm6, qh - pad, _("Cnty"));
					fm11box->align(FL_ALIGN_INSIDE);
					inpCountry = new Fl_Input2(rightof(fm11box), y3, w_inpCountry, qh - pad, "");
					inpCountry->tooltip(_("Country"));
					inpCountry->align(FL_ALIGN_INSIDE);
					
					Fl_Box *fm4box = new Fl_Box(rightof(inpCountry), y3, w_fm4, qh - pad, _("Loc"));
					fm4box->align(FL_ALIGN_INSIDE);
					inpLoc = new Fl_Input2(rightof(fm4box), y3, w_inpLOC, qh - pad, "");
					inpLoc->tooltip("");
					inpLoc->align(FL_ALIGN_INSIDE);

					Fl_Box *fm5box = new Fl_Box(rightof(inpLoc), y3, w_fm5, qh - pad, _("Az"));
					fm5box->align(FL_ALIGN_INSIDE);
					inpAZ = new Fl_Input2(rightof(fm5box), y3, 
					    rightof(inpRstOut) - rightof(fm5box), qh - pad, "");
					inpAZ->tooltip("");
					inpAZ->align(FL_ALIGN_INSIDE);
					
				QsoInfoFrame1A->end();
				
				QsoInfoFrame1B = new Fl_Group (x_qsoframe, y3 - 1, wf1, Hnotes); 
					Fl_Box *fm6box = new Fl_Box(x_qsoframe, y3, w_fm7, qh - pad, _("#Out"));
					fm6box->align(FL_ALIGN_INSIDE);
					outSerNo = new Fl_Input2(rightof(fm6box), y3, w_SerNo, qh - pad, "");
					outSerNo->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);
					outSerNo->tooltip(_("Sent serial number (read only)"));
					outSerNo->type(FL_NORMAL_OUTPUT);

					Fl_Box *fm7box = new Fl_Box(rightof(outSerNo) + pad, y3, w_fm5, qh - pad, _("#In"));
					fm7box->align(FL_ALIGN_INSIDE);
					inpSerNo = new Fl_Input2(rightof(fm7box), y3, w_SerNo, qh - pad, "");
					inpSerNo->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);
					inpSerNo->tooltip(_("Received serial number"));

					Fl_Box *fm8box = new Fl_Box(rightof(inpSerNo) + pad, y3, w_fm7, qh - pad, _("Xchg"));
					fm8box->align(FL_ALIGN_INSIDE);
					inpXchgIn = new Fl_Input2(rightof(fm8box), y3, 
					    rightof(inpRstOut) - rightof(fm8box), qh - pad, "");
					inpXchgIn->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);
					inpXchgIn->tooltip(_("Contest exchange in"));
									
				QsoInfoFrame1B->end();
				QsoInfoFrame1B->hide();
				
				QsoInfoFrame1->resizable(NULL);
			QsoInfoFrame1->end();

			QsoInfoFrame2 = new Fl_Group(x_qsoframe + wf1 + pad, Hmenu, 
				WNOM - rightof(QsoInfoFrame1) - 2*pad, Hqsoframe + Hnotes); 
					
				inpNotes = new Fl_Input2(x_qsoframe + wf1 + pad, y2, 
					WNOM - rightof(QsoInfoFrame1) - 2*pad, qh + Hnotes - pad, _("Notes"));
				inpNotes->type(FL_MULTILINE_INPUT);
				inpNotes->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);

					Fl_Group::current()->resizable(inpNotes);
			QsoInfoFrame2->end();
		
			Fl_Group::current()->resizable(QsoInfoFrame2);
		QsoInfoFrame->end();
			
		Fl_Group::current()->resizable(QsoInfoFrame);

	TopFrame->end();
	
		Y = Hmenu + Hqsoframe + Hnotes + pad;

		Fl_Widget* logfields[] = { inpCall, inpName, inpTimeOn, inpTimeOff, inpRstIn, inpRstOut,
				inpQth, inpState, inpVEprov, inpCountry, inpLoc, inpAZ, inpNotes,
				inpSerNo, outSerNo, inpXchgIn };
		for (size_t i = 0; i < sizeof(logfields)/sizeof(*logfields); i++) {
			logfields[i]->callback(cb_log);
			logfields[i]->when(FL_WHEN_CHANGED | FL_WHEN_NOT_CHANGED | FL_WHEN_ENTER_KEY | FL_WHEN_RELEASE );
		}
		// exceptions
		inpCall->callback(cb_call);
		inpCall->when(FL_WHEN_CHANGED | FL_WHEN_NOT_CHANGED | FL_WHEN_ENTER_KEY | FL_WHEN_RELEASE );
		inpLoc->callback(cb_loc);
		inpNotes->when(FL_WHEN_RELEASE);

		int sw = DEFAULT_SW;
		MixerFrame = new Fl_Group(0,Y,sw, Hrcvtxt + Hxmttxt);
			valRcvMixer = new Fl_Value_Slider(0, Y, sw, (Htext)/2, "");
			valRcvMixer->type(FL_VERT_NICE_SLIDER);
			valRcvMixer->color(fl_rgb_color(0,110,30));
			valRcvMixer->labeltype(FL_ENGRAVED_LABEL);
			valRcvMixer->selection_color(fl_rgb_color(255,255,0));
			valRcvMixer->textcolor(FL_WHITE);
			valRcvMixer->range(100.0,0.0);
			valRcvMixer->value(100.0);
			valRcvMixer->step(1.0);
			valRcvMixer->callback( (Fl_Callback *)cb_RcvMixer);
			valXmtMixer = new Fl_Value_Slider(0, Y + (Htext)/2, sw, (Htext)/2, "");
			valXmtMixer->type(FL_VERT_NICE_SLIDER);
			valXmtMixer->color(fl_rgb_color(110,0,30));
			valXmtMixer->labeltype(FL_ENGRAVED_LABEL);
			valXmtMixer->selection_color(fl_rgb_color(255,255,0));
			valXmtMixer->textcolor(FL_WHITE);
			valXmtMixer->range(100.0,0.0);
			valXmtMixer->value(100.0);
			valXmtMixer->step(1.0);
			valXmtMixer->callback( (Fl_Callback *)cb_XmtMixer);
		MixerFrame->end();

		TiledGroup = new Fl_Tile_check(sw, Y, WNOM-sw, Htext);
            int minRxHeight = Hrcvtxt;
            int minTxHeight;
            if (minRxHeight < 66) minRxHeight = 66;
            minTxHeight = Htext - minRxHeight;

			ReceiveText = new FTextView(sw, Y, WNOM-sw, minRxHeight, "");
			ReceiveText->color(
				fl_rgb_color(
					progdefaults.RxColor.R,
					progdefaults.RxColor.G,
					progdefaults.RxColor.B));		



			ReceiveText->setFont(progdefaults.RxFontnbr);
			ReceiveText->setFontSize(progdefaults.RxFontsize);
			ReceiveText->setFontColor(progdefaults.RxFontcolor, FTextBase::RECV);
			ReceiveText->setFontColor(progdefaults.XMITcolor, FTextBase::XMIT);
			ReceiveText->setFontColor(progdefaults.CTRLcolor, FTextBase::CTRL);
			ReceiveText->setFontColor(progdefaults.SKIPcolor, FTextBase::SKIP);
			ReceiveText->setFontColor(progdefaults.ALTRcolor, FTextBase::ALTR);
			showMacroSet();
			
			TiledGroup->add_resize_check(FTextView::wheight_mult_tsize, ReceiveText);
			FHdisp = new Raster(sw, Y, WNOM-sw, minRxHeight);
			FHdisp->hide();

			TransmitText = new FTextEdit(sw, Y + minRxHeight, WNOM-sw, minTxHeight);
			TransmitText->color(
				fl_rgb_color(
					progdefaults.TxColor.R,
					progdefaults.TxColor.G,
					progdefaults.TxColor.B));		

			TransmitText->setFont(progdefaults.TxFontnbr);
			TransmitText->setFontSize(progdefaults.TxFontsize);
			TransmitText->setFontColor(progdefaults.TxFontcolor, FTextBase::RECV);
			TransmitText->setFontColor(progdefaults.XMITcolor, FTextBase::XMIT);
			TransmitText->setFontColor(progdefaults.CTRLcolor, FTextBase::CTRL);
			TransmitText->setFontColor(progdefaults.SKIPcolor, FTextBase::SKIP);
			TransmitText->setFontColor(progdefaults.ALTRcolor, FTextBase::ALTR);

			Fl_Box *minbox = new Fl_Box(sw,Y + 66, WNOM-sw, Htext - 66 - 32);
			minbox->hide();
			TiledGroup->resizable(minbox);

			Y += Htext;
			
		TiledGroup->end();
		Fl_Group::current()->resizable(TiledGroup);

		Fl::add_handler(default_handler);

		Fl_Box *bx;
		Fl_Box *macroFrame = new Fl_Box(0, Y, WNOM, Hmacros);
			macroFrame->box(FL_ENGRAVED_FRAME);
			int Wbtn = (WNOM - 30 - 8 - 4)/NUMMACKEYS;
			int xpos = 2;
			for (int i = 0; i < NUMMACKEYS; i++) {
				if (i == 4 || i == 8) {
					bx = new Fl_Box(xpos, Y+2, 5, Hmacros - 4);
					bx->box(FL_FLAT_BOX);
					bx->color(FL_BLACK);
					xpos += 4;
				}
				btnMacro[i] = new Fl_Button(xpos, Y+2, Wbtn, Hmacros - 4, macros.name[i].c_str());
				btnMacro[i]->callback(macro_cb, (void *)i);
				btnMacro[i]->tooltip(_("Left Click - execute\nRight Click - edit"));
				colorize_macro(i);
				xpos += Wbtn;
			}
			bx = new Fl_Box(xpos, Y+2, WNOM - 32 - xpos, Hmacros - 4);
			bx->box(FL_FLAT_BOX);
			bx->color(FL_BLACK);
			btnAltMacros = new Fl_Button(WNOM-32, Y+2, 30, Hmacros - 4, "1");
			btnAltMacros->callback(altmacro_cb, 0);
			btnAltMacros->tooltip(_("Change macro set"));
				
		Y += Hmacros;

		if (progdefaults.docked_scope) {
			Fl_Pack *wfpack = new Fl_Pack(0, Y, WNOM, Hwfall);
				wfpack->type(1);
				wf = new waterfall(0, Y, Wwfall, Hwfall);
				wf->end();
				Fl_Pack *ypack = new Fl_Pack(
					rightof(wf), Y,
					Hwfall - 24, Hwfall);

					ypack->type(0);

					wfscope = new Digiscope (
						rightof(wf), Y,
						Hwfall - 24, Hwfall - 24);
		
					pgrsSquelch = new Progress(
						rightof(wf), Y + Hwfall - 24,
						Hwfall - 24, 12,
						"");
					pgrsSquelch->color(FL_BACKGROUND2_COLOR, FL_DARK_GREEN);
					sldrSquelch = new Fl_Slider( FL_HOR_NICE_SLIDER, 
						rightof(wf), Y + Hwfall - 12, Hwfall - 24, 12, "");
							
					sldrSquelch->minimum(0);
					sldrSquelch->maximum(100);
					sldrSquelch->step(1);
					sldrSquelch->value(progStatus.sldrSquelchValue);
					sldrSquelch->callback((Fl_Callback*)cb_sldrSquelch);
					sldrSquelch->color(FL_INACTIVE_COLOR);

				ypack->end();
				Fl_Group::current()->resizable(wf);
			wfpack->end();
		} else {
			Fl_Pack *wfpack = new Fl_Pack(0, Y, WNOM, Hwfall);
				wfpack->type(1);

				wf = new waterfall(0, Y, Wwfall, Hwfall);
				wf->end();

				pgrsSquelch = new Progress( 
					rightof(wf), Y + 4, 
					DEFAULT_SW, Hwfall - 8, 
					"");
				pgrsSquelch->color(FL_BACKGROUND2_COLOR, FL_DARK_GREEN);
				pgrsSquelch->type(Progress::VERTICAL);
				pgrsSquelch->tooltip(_("Detected signal level"));

				sldrSquelch = new Fl_Slider( 
					rightof(pgrsSquelch), Y + 4, 
					DEFAULT_SW, Hwfall - 8, 
					"");
				sldrSquelch->minimum(100);
				sldrSquelch->maximum(0);
				sldrSquelch->step(1);
				sldrSquelch->value(progStatus.sldrSquelchValue);
				sldrSquelch->callback((Fl_Callback*)cb_sldrSquelch);
				sldrSquelch->color(FL_INACTIVE_COLOR);
				sldrSquelch->tooltip(_("Squelch level"));
									
				Fl_Group::current()->resizable(wf);
			wfpack->end();
		}
		Y += (Hwfall + 2);

		Fl_Pack *hpack = new Fl_Pack(0, Y, WNOM, Hstatus);
			hpack->type(1);
			MODEstatus = new Fl_Button(0,Hmenu+Hrcvtxt+Hxmttxt+Hwfall, Wmode+30, Hstatus, "");
			MODEstatus->box(FL_DOWN_BOX);
			MODEstatus->color(FL_BACKGROUND2_COLOR);
			MODEstatus->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
			MODEstatus->callback(status_cb, (void *)0);
			MODEstatus->when(FL_WHEN_CHANGED);
			MODEstatus->tooltip(_("Left click: change mode\nRight click: configure"));

			Status1 = new Fl_Box(rightof(MODEstatus), Hmenu+Hrcvtxt+Hxmttxt+Hwfall, Ws2n, Hstatus, "");
			Status1->box(FL_DOWN_BOX);
			Status1->color(FL_BACKGROUND2_COLOR);
			Status1->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
			
			Status2 = new Fl_Box(rightof(Status1), Hmenu+Hrcvtxt+Hxmttxt+Hwfall, Wimd, Hstatus, "");
			Status2->box(FL_DOWN_BOX);
			Status2->color(FL_BACKGROUND2_COLOR);
			Status2->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

			StatusBar = new Fl_Box(
                rightof(Status2), Hmenu+Hrcvtxt+Hxmttxt+Hwfall, 
                WNOM - bwSqlOnOff - bwAfcOnOff - Wwarn - rightof(Status2) - 2 * pad,// - 60, 
                Hstatus, "");
			StatusBar->box(FL_DOWN_BOX);
			StatusBar->color(FL_BACKGROUND2_COLOR);
			StatusBar->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
			
			WARNstatus = new Fl_Box(
				rightof(StatusBar) + pad, Hmenu+Hrcvtxt+Hxmttxt+Hwfall, 
                Wwarn, Hstatus, "");
			WARNstatus->box(FL_DIAMOND_DOWN_BOX);
			WARNstatus->color(FL_BACKGROUND_COLOR);
			WARNstatus->labelcolor(FL_RED);
			WARNstatus->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE);
				
			int sql_width = bwSqlOnOff;
#ifdef __APPLE__
			sql_width -= 15; // leave room for resize handle
#endif
			if (progdefaults.useCheckButtons) {
				btn_afconoff = new Fl_Check_Button(
								WNOM - bwSqlOnOff - bwAfcOnOff, 
								Hmenu+Hrcvtxt+Hxmttxt+Hwfall, 
								bwAfcOnOff, Hstatus, "AFC");
				btn_sqlonoff = new Fl_Check_Button(
								WNOM - bwSqlOnOff, 
								Hmenu+Hrcvtxt+Hxmttxt+Hwfall, 
								sql_width, Hstatus, "SQL");
			} else {
				btn_afconoff = new Fl_Light_Button(
								WNOM - bwSqlOnOff - bwAfcOnOff, 
								Hmenu+Hrcvtxt+Hxmttxt+Hwfall, 
								bwAfcOnOff, Hstatus, "AFC");
				btn_sqlonoff = new Fl_Light_Button(
								WNOM - bwSqlOnOff, 
								Hmenu+Hrcvtxt+Hxmttxt+Hwfall, 
								sql_width, Hstatus, "SQL");
			}
			btn_afconoff->callback(afconoff_cb, 0);
			btn_afconoff->value(1);
			btn_afconoff->tooltip(_("Automatic Frequency Control"));
			btn_sqlonoff->callback(sqlonoff_cb, 0);
			btn_sqlonoff->value(1);
			btn_sqlonoff->tooltip(_("Squelch"));


			Fl_Group::current()->resizable(StatusBar);
		hpack->end();

	fl_digi_main->end();
	fl_digi_main->callback(cb_wMain);

#if defined(__WOE32__)
#  ifndef IDI_ICON
#    define IDI_ICON 101
#  endif
	fl_digi_main->icon((char*)LoadIcon(fl_display, MAKEINTRESOURCE(IDI_ICON)));
#elif !defined(__APPLE__)
	make_pixmap(&fldigi_icon_pixmap, fldigi_icon);
	fl_digi_main->icon((char *)fldigi_icon_pixmap);
#endif

	fl_digi_main->xclass(PACKAGE_NAME);
	fl_digi_main->size_range(WMIN, HMIN);

	scopeview = new Fl_Double_Window(0,0,140,140, _("Scope"));
	scopeview->xclass(PACKAGE_NAME);
	digiscope = new Digiscope (0, 0, 140, 140);
	scopeview->resizable(digiscope);
	scopeview->size_range(SCOPEWIN_MIN_WIDTH, SCOPEWIN_MIN_HEIGHT);
	scopeview->end();
	scopeview->hide();	

	if (progdefaults.docked_rig_control)
		activate_rig_menu_item(false);

	if (!progdefaults.menuicons)
		toggle_icon_labels();

	// ztimer must be run by FLTK's timeout handler
	Fl::add_timeout(0.0, ztimer, (void*)true);

	// Set the state of toggle menu items
	struct {
		bool var; const char* label;
	} toggles[] = {
		{ progStatus.LOGenabled, LOG_TO_FILE_MLABEL },
		{ progStatus.contest, CONTEST_FIELDS_MLABEL }
	};
	Fl_Menu_Item* toggle;
	for (size_t i = 0; i < sizeof(toggles)/sizeof(*toggles); i++) {
		if (toggles[i].var && (toggle = getMenuItem(toggles[i].label))) {
			toggle->set();
			if (toggle->callback()) {
				mnu->value(toggle);
				toggle->do_callback(reinterpret_cast<Fl_Widget*>(mnu));
			}
		}
	}
}

void put_freq(double frequency)
{
	wf->carrier((int)floor(frequency + 0.5));
}

void put_Bandwidth(int bandwidth)
{
	wf->Bandwidth ((int)bandwidth);
}

static void set_metric(double metric)
{
	pgrsSquelch->value(metric);
	static Fl_Color sqlcol = btn_sqlonoff->selection_color();
	if (!progStatus.sqlonoff)
		return;
	if (metric < progStatus.sldrSquelchValue)
		btn_sqlonoff->selection_color(sqlcol);
	else
	        btn_sqlonoff->selection_color(FL_GREEN);
	btn_sqlonoff->redraw_label();
}

void display_metric(double metric)
{
	FL_LOCK_D();
	REQ_DROP(set_metric, metric);
	FL_UNLOCK_D();
	FL_AWAKE_D();
}

void put_cwRcvWPM(double wpm)
{
	int U = progdefaults.CWupperlimit;
	int L = progdefaults.CWlowerlimit;
	double dWPM = 100.0*(wpm - L)/(U - L);
	FL_LOCK_D();
	REQ_DROP(static_cast<void (Fl_Progress::*)(float)>(&Fl_Progress::value), prgsCWrcvWPM, dWPM);
	REQ_DROP(static_cast<int (Fl_Value_Output::*)(double)>(&Fl_Value_Output::value), valCWrcvWPM, (int)wpm);
	FL_UNLOCK_D();
	FL_AWAKE_D();
}

void set_scope_mode(Digiscope::scope_mode md)
{
	if (digiscope) {
		digiscope->mode(md);
		REQ(&Fl_Window::size_range, scopeview, SCOPEWIN_MIN_WIDTH, SCOPEWIN_MIN_HEIGHT,
		    0, 0, 0, 0, (md == Digiscope::PHASE || md == Digiscope::XHAIRS));
	}
	if (wfscope)
		wfscope->mode(md);
}

void set_scope(double *data, int len, bool autoscale)
{
	if (digiscope)
		digiscope->data(data, len, autoscale);
	if (wfscope)
		wfscope->data(data, len, autoscale);
}

void set_phase(double phase, double quality, bool highlight)
{
	if (digiscope)
		digiscope->phase(phase, quality, highlight);
	if (wfscope)
		wfscope->phase(phase, quality, highlight);
}

void set_rtty(double flo, double fhi, double amp)
{
	if (digiscope)
		digiscope->rtty(flo, fhi, amp);
	if (wfscope)
		wfscope->rtty(flo, fhi, amp);
}

void set_video(double *data, int len, bool dir)
{
	if (digiscope)
		digiscope->video(data, len, dir);
	if (wfscope)
		wfscope->video(data, len, dir);
}

void set_zdata(complex *zarray, int len)
{
	if (digiscope)
		digiscope->zdata(zarray, len);
	if (wfscope)
		wfscope->zdata(zarray, len);
}

void put_rx_char(unsigned int data)
{
#if BENCHMARK_MODE
	if (!benchmark.output.empty()) {
		if (unlikely(benchmark.buffer.length() + 16 > benchmark.buffer.capacity()))
			benchmark.buffer.reserve(benchmark.buffer.capacity() + BUFSIZ);
		benchmark.buffer += (char)data;
	}
	return;
#endif
	static unsigned int last = 0;
	const char **asc = ascii;
	trx_mode mode = active_modem->get_mode();

	if (mailclient || mailserver || arqmode)
		asc = ascii2;
	if (mode == MODE_RTTY || mode == MODE_CW)
		asc = ascii;

	int style = FTextBase::RECV;
	if (asc == ascii2 && iscntrl(data))
		style = FTextBase::CTRL;
	if (wf->tmp_carrier())
		style = FTextBase::ALTR;

	if (progdefaults.autoextract == true) rx_extract_add(data);
	if (progdefaults.speak == true) speak(data);
	
	switch (data) {
		case '\n':
			if (last == '\r')
				break;
		case '\r':
			REQ(&FTextBase::addchr, ReceiveText, '\n', style);
			break;
		default:
			REQ(&FTextBase::addchr, ReceiveText, data, style);
	}
	last = data;

	WriteARQ(data);
	
	string s;
	if (iscntrl(data))
		s = ascii2[data & 0x7F];
	else {
		s += data;
		bool viewer = (mode >= MODE_PSK_FIRST && mode <= MODE_PSK_LAST && dlgViewer && dlgViewer->visible());
		if (progStatus.spot_recv && !viewer)
			spot_recv(data);
	}
	if (Maillogfile)
		Maillogfile->log_to_file(cLogfile::LOG_RX, s);

	if (progStatus.LOGenabled)
		logfile->log_to_file(cLogfile::LOG_RX, s);
}

string strSecText = "";

void put_sec_char( char chr )
{
	fl_font(FL_HELVETICA, FL_NORMAL_SIZE);
	char s[2] = "W";
	int lc = (int)ceil(fl_width(s));
	int w = StatusBar->w();
	int lw = (int)ceil(fl_width(StatusBar->label()));
	int over = 2 * lc + lw - w;
	
	if (chr >= ' ' && chr <= 'z') {
		if ( over > 0 )
			strSecText.erase(0, (int)(1.0 * over / lc + 0.5));
		strSecText.append(1, chr);
		FL_LOCK_D();
		REQ(static_cast<void (Fl_Box::*)(const char *)>(&Fl_Box::label), StatusBar, strSecText.c_str());
		WARNstatus->damage();
		FL_UNLOCK_D();
		FL_AWAKE_D();
	}
}


static void clear_status_cb(void* arg)
{
	reinterpret_cast<Fl_Box*>(arg)->label("");
}
static void dim_status_cb(void* arg)
{
	reinterpret_cast<Fl_Box*>(arg)->deactivate();
}
static void (*const timeout_action[STATUS_NUM])(void*) = { clear_status_cb, dim_status_cb };

static void put_status_msg(Fl_Box* status, const char* msg, double timeout, status_timeout action)
{
	status->activate();
	status->label(msg);
	if (timeout > 0.0) {
		Fl::remove_timeout(timeout_action[action], status);
		Fl::add_timeout(timeout, timeout_action[action], status);
	}
}

void put_status(const char *msg, double timeout, status_timeout action)
{
	static char m[50];
	strncpy(m, msg, sizeof(m));
	m[sizeof(m) - 1] = '\0';

	REQ(put_status_msg, StatusBar, m, timeout, action);
}

void put_Status2(const char *msg, double timeout, status_timeout action)
{
	static char m[60];
	strncpy(m, msg, sizeof(m));
	m[sizeof(m) - 1] = '\0';

	info2msg = msg;
	
	REQ(put_status_msg, Status2, m, timeout, action);
}

void put_Status1(const char *msg, double timeout, status_timeout action)
{
	static char m[60];
	strncpy(m, msg, sizeof(m));
	m[sizeof(m) - 1] = '\0';

	info1msg = msg;
	
	REQ(put_status_msg, Status1, m, timeout, action);
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
	FL_LOCK_D();
	sldrCWxmtWPM->value(progdefaults.CWspeed);
	FL_UNLOCK_D();
}

void clear_StatusMessages()
{
	FL_LOCK_D();
	StatusBar->label("");
	Status1->label("");
	Status2->label("");
	FL_UNLOCK_D();
	FL_AWAKE_D();
}

void put_MODEstatus(const char* fmt, ...)
{
	static char s[32];
	va_list args;
	va_start(args, fmt);
	vsnprintf(s, sizeof(s), fmt, args);
	va_end(args);

	REQ(static_cast<void (Fl_Button::*)(const char *)>(&Fl_Button::label), MODEstatus, s);
}

void put_MODEstatus(trx_mode mode)
{
	put_MODEstatus("%s", mode_info[mode].sname);
}


void put_rx_data(int *data, int len)
{
 	FHdisp->data(data, len);
}

char szTestChar[] = "E|I|S|T|M|O|A|V";
int get_tx_char(void)
{
	if (arq_text_available)
		return arq_get_char();

    if (active_modem == cw_modem && progdefaults.QSKadjust)
        return szTestChar[2 * progdefaults.TestChar];
        
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
		REQ_SYNC(&FTextEdit::clear_sent, TransmitText);
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
    if (progdefaults.QSKadjust) return;
    
	static unsigned int last = 0;
	const char **asc = ascii;
	
	if (mailclient || mailserver || arqmode)
		asc = ascii2;
	if (active_modem->get_mode() == MODE_RTTY ||
		active_modem->get_mode() == MODE_CW)
		asc = ascii;

	if (data == '\r' && last == '\r') // reject multiple CRs
		return;

	last = data;

	int style = FTextBase::XMIT;
	if (asc == ascii2 && iscntrl(data))
		style = FTextBase::CTRL;
	REQ(&FTextBase::addchr, ReceiveText, data, style);

	string s = iscntrl(data) ? ascii2[data & 0x7F] : string(1, data);
	if (Maillogfile)
		Maillogfile->log_to_file(cLogfile::LOG_TX, s);

	if (progStatus.LOGenabled)
		logfile->log_to_file(cLogfile::LOG_TX, s);
}

void resetRTTY() {
	if (active_modem->get_mode() == MODE_RTTY)
		trx_start_modem(active_modem);
}

void resetOLIVIA() {
	if (active_modem->get_mode() == MODE_OLIVIA)
		trx_start_modem(active_modem);
}

void resetTHOR() {
	trx_mode md = active_modem->get_mode();
	if (md == MODE_THOR4 || md == MODE_THOR5 || md == MODE_THOR8 ||
	    md == MODE_THOR11 ||
	    md == MODE_THOR16 || md == MODE_THOR22 )
		trx_start_modem(active_modem);
}

void resetDOMEX() {
	trx_mode md = active_modem->get_mode();
	if (md == MODE_DOMINOEX4 || md == MODE_DOMINOEX5 ||
	    md == MODE_DOMINOEX8 || md == MODE_DOMINOEX11 ||
	    md == MODE_DOMINOEX16 || md == MODE_DOMINOEX22 )
		trx_start_modem(active_modem);
}

void enableMixer(bool on)
{
#if !USE_OSS
	on = false;
#endif

	FL_LOCK_D();
	if (on) {
		progdefaults.EnableMixer = true;
#if USE_OSS
		mixer = new MixerOSS;
#else
		mixer = new MixerBase;
#endif
		try {
			mixer->openMixer(progdefaults.MXdevice.c_str());
		}
		catch (const MixerException& e) {
			put_status(e.what(), 5);
			goto ret;
		}

		mixer->PCMVolume(progdefaults.PCMvolume);
		mixer->setXmtLevel(progStatus.XmtMixer); //valXmtMixer->value());
		mixer->setRcvGain(progStatus.RcvMixer); //valRcvMixer->value());
		if (progdefaults.LineIn == true)
			setMixerInput(1);
		else if (progdefaults.MicIn == true)
			setMixerInput(2);
		else
			setMixerInput(0);
	}else{
		progdefaults.EnableMixer = false;
		if (mixer)
			mixer->closeMixer();
		delete mixer;
                mixer = 0;
	}
ret:
        resetMixerControls();
	FL_UNLOCK_D();
}

void enable_vol_sliders(bool val)
{
        if (MixerFrame->visible()) {
                if (val)
                        return;
		MixerFrame->hide();
		TiledGroup->resize(TiledGroup->x() - MixerFrame->w(), TiledGroup->y(),
				   TiledGroup->w() + MixerFrame->w(), TiledGroup->h());
        }
        else {
                if (!val)
                        return;
		TiledGroup->resize(TiledGroup->x() + MixerFrame->w(), TiledGroup->y(),
				   TiledGroup->w() - MixerFrame->w(), TiledGroup->h());
		MixerFrame->show();
        }
}

void resetMixerControls()
{
    if (progdefaults.EnableMixer) {
	    menuMix->activate();
	    btnLineIn->activate();
	    btnMicIn->activate();
        btnMixer->value(1);
	    valPCMvolume->activate();
    }
    else {
	    menuMix->deactivate();
	    btnLineIn->deactivate();
	    btnMicIn->deactivate();
        btnMixer->value(0);
	    valPCMvolume->deactivate();
    }
    enable_vol_sliders(progdefaults.EnableMixer);
}

void setPCMvolume(double vol)
{
	mixer->PCMVolume(vol);
	progdefaults.PCMvolume = vol;
}

void setMixerInput(int dev)
{
	int n= -1;
	switch (dev) {
		case 0: n = mixer->InputSourceNbr("Vol");
				break;
		case 1: n = mixer->InputSourceNbr("Line");
				break;
		case 2: n = mixer->InputSourceNbr("Mic");
				break;
		default: n = mixer->InputSourceNbr("Vol");
	}
	if (n != -1)
		mixer->SetCurrentInputSource(n);
}

void resetSoundCard()
{
    bool mixer_enabled = progdefaults.EnableMixer;
	enableMixer(false);
	trx_reset();
    if (mixer_enabled)
        enableMixer(true);
}

void setReverse(int rev) {
	active_modem->set_reverse(rev);
}

void start_tx()
{
	if (progdefaults.rsid == true) return;
	trx_transmit();
	REQ(&waterfall::set_XmtRcvBtn, wf, true);
}

void abort_tx()
{
	if (trx_state == STATE_TUNE) {
		btnTune->value(0);
		btnTune->do_callback();
	}
	else if (trx_state == STATE_TX)
		trx_start_modem(active_modem);
}

// Adjust and return fg color to ensure good contrast with bg
Fl_Color adjust_color(Fl_Color fg, Fl_Color bg)
{
	Fl_Color adj;
	unsigned max = 24;
	while ((adj = fl_contrast(fg, bg)) != fg  &&  max--)
		fg = (adj == FL_WHITE) ? fl_color_average(fg, FL_WHITE, .9)
				       : fl_color_average(fg, FL_BLACK, .9);
	return fg;
}

void qsy(long long rfc, long long fmid)
{
	if (fmid < 0LL)
		fmid = (long long)active_modem->get_freq();
	if (rfc == 0LL || rfc == wf->rfcarrier()) {
		active_modem->set_freq(fmid);
		return;
	}

	if (progdefaults.chkUSERIGCATis)
		REQ(rigCAT_set_qsy, rfc, fmid);
	else if (progdefaults.chkUSEMEMMAPis)
		REQ(rigMEM_set_qsy, rfc, fmid);
#if USE_HAMLIB
	else if (progdefaults.chkUSEHAMLIBis)
		REQ(hamlib_set_qsy, rfc, fmid);
#endif
#if USE_XMLRPC
	else if (progdefaults.chkUSEXMLRPCis)
		REQ(xmlrpc_set_qsy, rfc, fmid);
#endif
	else
		active_modem->set_freq(fmid);
}

unsigned quick_choice_menu(const char* title, unsigned sel, const Fl_Menu_Item* menu)
{
	unsigned n = menu->size();
	sel = CLAMP(sel - 1, 0, n - 1);
	int t = Fl_Tooltip::enabled();
	Fl_Tooltip::disable();
	const Fl_Menu_Item* p = menu->popup(Fl::event_x(), Fl::event_y(), title, menu + sel);
	Fl_Tooltip::enable(t);
	return p ? p - menu + 1 : 0;
}

unsigned quick_choice(const char* title, unsigned sel, ...)
{
	const char* item;
	const Fl_Menu_Item* menu = NULL;
	Fl_Menu_Item* p = NULL;

	va_list ap;
	va_start(ap, sel);
	for (size_t n = 0; (item = va_arg(ap, const char*)); n++) {
		if ((p = (Fl_Menu_Item*)realloc(p, (n+2) * sizeof(Fl_Menu_Item))) == NULL) {
			free((Fl_Menu_Item*)menu);
			va_end(ap);
			return 0;
		}
		memset(p + n, 0, 2 * sizeof(Fl_Menu_Item));
		p[n].label(item);
		p[n+1].label(NULL);
		menu = p;
	}
	va_end(ap);

	sel = quick_choice_menu(title, sel, menu);
	free(p);
	return sel;
}
