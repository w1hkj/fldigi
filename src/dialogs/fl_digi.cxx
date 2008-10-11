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
#ifndef __CYGWIN__
#  include <sys/ipc.h>
#  include <sys/msg.h>
#endif

#ifdef __CYGWIN__
#  include <w32api/windows.h>
#endif

#include <stdlib.h>
#include <string>

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

#include "waterfall.h"
#include "raster.h"
#include "progress.h"
#include "afcind.h"

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

#include "macros.h"
#include "macroedit.h"
#include "logger.h"
#include "qrzcall.h"

#include "combo.h"
#include "font_browser.h"
#if !defined(__APPLE__) && !defined(__CYGWIN__)
#        include "fldigi-icon-48.xpm"
#endif
#include "status.h"

#include "rigsupport.h"

#include "qrunner.h"

#include "Viewer.h"
#include "soundconf.h"

#include "htmlstrings.h"
#if USE_XMLRPC
#	include "xmlrpc.h"
#endif
#include "debug.h"
#include "re.h"
#include "network.h"

Fl_Double_Window	*fl_digi_main=(Fl_Double_Window *)0;
Fl_Help_Dialog 		*help_dialog = (Fl_Help_Dialog *)0;
Fl_Double_Window	*scopeview = (Fl_Double_Window *)0;

MixerBase* mixer = 0;

bool	useCheckButtons;


Fl_Light_Button		*btnTune = (Fl_Light_Button *)0;
Fl_Light_Button		*btnRSID = (Fl_Light_Button *)0;

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
Fl_Button			*btnMacroDummy;
Fl_Button			*btnQRZ;
Fl_Group			*MixerFrame;
Fl_Value_Slider			*valRcvMixer;
Fl_Value_Slider			*valXmtMixer;
AFCind				*AFCindicator;

int					altMacros = 0;
bool				bSaveFreqList = false;
string				strMacroName[NUMMACKEYS];


waterfall			*wf = (waterfall *)0;
Digiscope			*digiscope = (Digiscope *)0;
Digiscope			*wfscope = (Digiscope *)0;

Fl_Slider			*sldrSquelch = (Fl_Slider *)0;
Progress			*pgrsSquelch = (Progress *)0;

Fl_RGB_Image		*feld_image = 0;

#if !defined(__APPLE__) && !defined(__CYGWIN__)
Pixmap				fldigi_icon_pixmap;
#endif

int IMAGE_WIDTH = DEFAULT_IMAGE_WIDTH;
int Hwfall = DEFAULT_HWFALL;
int HNOM = DEFAULT_HNOM;
int WNOM = DEFAULT_WNOM;

bool clean_exit(void);

void cb_init_mode(Fl_Widget *, void *arg);

void cb_oliviaA(Fl_Widget *w, void *arg);
void cb_oliviaB(Fl_Widget *w, void *arg);
void cb_oliviaC(Fl_Widget *w, void *arg);
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
	{ "8/500", 0, cb_oliviaA, (void *)MODE_OLIVIA },
	{ "16/500", 0, cb_oliviaB, (void *)MODE_OLIVIA },
	{ "32/1000", 0, cb_oliviaC, (void *)MODE_OLIVIA },
	{ "Custom", 0, cb_oliviaCustom, (void *)MODE_OLIVIA },
	{ 0 }
};

Fl_Menu_Item quick_change_rtty[] = {
	{ "RTTY-45", 0, cb_rtty45, (void *)MODE_RTTY },
	{ "RTTY-50", 0, cb_rtty50, (void *)MODE_RTTY },
	{ "RTTY-75", 0, cb_rtty75, (void *)MODE_RTTY },
	{ "Custom", 0, cb_rttyCustom, (void *)MODE_RTTY },
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

void startup_modem(modem *m)
{
	trx_start_modem(m);

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
		sldrHellBW->value(m->get_bandwidth());
		progdefaults.HELL_BW = m->get_bandwidth();
	} else {
		ReceiveText->show();
		FHdisp->hide();
	}

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
	macros.openMacroFile();
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
	quick_change = 0;
	modem_config_tab = tabsModems->child(0);

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


void restoreFocus()
{
	FL_LOCK_D();
	TransmitText->take_focus();
	FL_UNLOCK_D();
	FL_AWAKE_D();
}

void macro_cb(Fl_Widget *w, void *v)
{
	stopMacroTimer();
	int b = (int)(reinterpret_cast<long> (v));
	b += altMacros * NUMMACKEYS;
	int mouse = Fl::event_button();
	if (mouse == 1 && !macros.text[b].empty())
		macros.execute(b);
	else if (mouse == 3)
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

void cb_mnuConfigVideo(Fl_Menu_*, void*) {
	progdefaults.loadDefaults();
	tabsConfigure->value(tabVideo);
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

#if USE_SNDFILE
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
#endif // USE_SNDFILE

void cb_mnuConfigFonts(Fl_Menu_*, void *) {
	selectColorsFonts();
}

void cb_mnuSaveConfig(Fl_Menu_ *, void *) {
	progdefaults.saveDefaults();
	restoreFocus();
}

void cb_mnuVisitURL(Fl_Widget*, void* arg)
{
	const char* url = reinterpret_cast<const char *>(arg);
#ifndef __CYGWIN__
#  ifdef __APPLE__
	const char* browsers[] = { "open" };
#  else
	const char* browsers[] = { "xdg-open", getenv("BROWSER"), "sensible-brower",
				   "firefox", "mozilla" };
#  endif
	switch (fork()) {
	case 0:
		for (size_t i = 0; i < sizeof(browsers)/sizeof(browsers[0]); i++)
			if (browsers[i])
				execlp(browsers[i], browsers[i], url, (char*)0);
		LOG_PERROR("Could not execute a web browser");
		exit(EXIT_FAILURE);
	case -1:
		fl_alert("Could not run a web browser:\n%s\n\n"
			 "Open this URL manually:\n%s",
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
		fl_alert("Could not open url:\n%s\n", url);
#endif

        restoreFocus();
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
	string deffname = HomeDir;
	deffname.append("beginners.html");
	ofstream f(deffname.c_str());
	if (!f)
		return;
	f << szBeginner;
	f.close();
#ifndef __CYGWIN__
	cb_mnuVisitURL(NULL, (void *)deffname.insert(0, "file://").c_str());
#else
	cb_mnuVisitURL(NULL, (void *)deffname.c_str());
#endif
}

void cb_mnuCheckUpdate(Fl_Widget* w, void*)
{
	struct {
		const char* url;
		const char* re;
		string version_str;
		long version;
	} sites[] = {
		{ PACKAGE_HOME, "fldigi-distro/fldigi-([0-9.]+).tar.gz", "", 0 },
		{ PACKAGE_PROJ, "fldigi/fldigi-([0-9.]+).tar.gz", "", 0 }
	}, *latest;
	string reply;

	w->window()->cursor(FL_CURSOR_WAIT);
	put_status("Checking for updates...");
	for (size_t i = 0; i < sizeof(sites)/sizeof(*sites); i++) { // fetch .url, grep for .re
		Fl::check();
		reply.clear();
		if (!fetch_http(sites[i].url, reply, 20.0))
			continue;
		re_t re(sites[i].re, REG_EXTENDED | REG_ICASE | REG_NEWLINE);
		if (!re.match(reply.c_str()) || re.nsub() != 2)
			continue;

		sites[i].version = ver2int((sites[i].version_str = re.submatch(1)).c_str());
	}
	w->window()->cursor(FL_CURSOR_DEFAULT);
	put_status("");

	latest = sites[1].version > sites[0].version ? &sites[1] : &sites[0];
	if (sites[0].version == 0 && sites[1].version == 0) {
		fl_message("Could not check for updates:\n%s", reply.c_str());
		return;
	}
	if (latest->version > ver2int(PACKAGE_VERSION)) {
		switch (fl_choice("Version %s is available at\n\n%s\n\nWhat would you like to do?",
				  "Close", "Visit URL", "Copy URL",
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
		fl_message("You are running the latest version");
}

void cb_mnuAboutURL(Fl_Widget*, void*)
{
	if (!help_dialog)
		help_dialog = new Fl_Help_Dialog;
	help_dialog->value(szAbout);
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
	extern string version_text;
	fldigi_help(version_text);
	restoreFocus();
}

void cb_mnuDebug(Fl_Widget*, void*)
{
	debug::show();
}

#ifndef NDEBUG
void cb_mnuFun(Fl_Widget*, void*)
{
        fl_message("Sunspot creation underway!");
}
#endif

void cb_mnuAudioInfo(Fl_Widget*, void*)
{
        if (progdefaults.btnAudioIOis != SND_IDX_PORT) {
                fl_alert("Audio device information is only available for the PortAudio backend");
                return;
        }

#if USE_PORTAUDIO
	size_t ndev;
        string devtext[2], headers[2];
	SoundPort::devices_info(devtext[0], devtext[1]);
	if (devtext[0] != devtext[1]) {
		headers[0] = "Capture device";
		headers[1] = "Playback device";
		ndev = 2;
	}
	else {
		headers[0] = "Capture and playback devices";
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

const char *zuluTime()
{
	time_t t;
	struct tm tm;
	static char logtime[5];
	if ((t = time(NULL)) != (time_t)-1 && gmtime_r(&t, &tm) &&
	    strftime(logtime, sizeof(logtime), "%H%M", &tm))
	return logtime;
	else
		return NULL;
}

bool oktoclear = true;

void qsoTime_cb(Fl_Widget *b, void *)
{
	inpTime->value(zuluTime());
	oktoclear = false;
	restoreFocus();
}

void clearQSO()
{
	Fl_Input* in[] = { inpCall, inpName, inpRstIn, inpRstOut,
			   inpQth, inpNotes, inpLoc, inpAZ };
	for (size_t i = 0; i < sizeof(in)/sizeof(*in); i++)
		in[i]->value("");
		inpTime->value(zuluTime());
}

void cb_log(Fl_Widget*, void*)
{
	oktoclear = false;
}

void qsoClear_cb(Fl_Widget *b, void *)
{
	if (oktoclear || fl_choice("Clear log fields?", "Cancel", "OK", NULL) == 1) {
		clearQSO();
	oktoclear = true;
	}
	restoreFocus();
}

void qsoSave_cb(Fl_Widget *b, void *)
{
	submit_log();
	oktoclear = true;
	restoreFocus();
}

void cb_QRZ(Fl_Widget *b, void *)
{
	CALLSIGNquery();
	oktoclear = false;
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
                const Fl_Menu_Item *m;
                m = quick_change->popup(Fl::event_x(),
                                        Fl::event_y(), 0, 0, 0);
                if (m && m->callback_)
                        m->do_callback(0);
        }
}

void cb_cboBand(Fl_Widget *w, void *d) 
{
	Fl_ComboBox *cbBox = (Fl_ComboBox *) w;
	wf->rfcarrier(atoi(cbBox->value())*1000L);
	oktoclear = false;
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

void stopMacroTimer()
{
	if (progdefaults.useTimer == false) return;
	
	progdefaults.useTimer = false;
	Fl::remove_timeout(macro_timer);
	FL_LOCK_D();
	btnMacroTimer->hide();
	btnMacroDummy->show();
	FL_UNLOCK_D();
	restoreFocus();
}

void cbMacroTimerButton(Fl_Widget *w, void *d)
{
	stopMacroTimer();
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
	return 0;
}

bool clean_exit(void) {
	arq_close();

	if (progdefaults.changed) {
		switch (fl_choice("Save changed configuration before exiting?", "Cancel", "Save", "Don't save")) {
		case 0:
			return false;
		case 1:
			progdefaults.saveDefaults();
			// fall through
		case 2:
			break;
		}
	}
	if (!oktoclear) {
		switch (fl_choice("Save log before exiting?", "Cancel", "Save", "Don't save")) {
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
		switch (fl_choice("Save changed macros before exiting?", "Cancel", "Save", "Don't save")) {
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

	if (bSaveFreqList)
		saveFreqList();

	progStatus.saveLastState();

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

	return true;
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
{"&Files", 0,  0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{"Open macros...", 0,  (Fl_Callback*)cb_mnuOpenMacro, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
{"Save macros...", 0,  (Fl_Callback*)cb_mnuSaveMacro, 0, FL_MENU_DIVIDER, FL_NORMAL_LABEL, 0, 14, 0},
{"Show config", 0, cb_ShowConfig, 0, FL_MENU_DIVIDER, FL_NORMAL_LABEL, 0, 14, 0},
//{"Log File", 0, (Fl_Callback*)cb_mnuLogFile, 0, FL_MENU_DIVIDER | FL_MENU_TOGGLE, FL_NORMAL_LABEL, 0, 14, 0},
{"Log File", 0, 0, 0, FL_MENU_DIVIDER | FL_MENU_TOGGLE, FL_NORMAL_LABEL, 0, 14, 0},
#if USE_SNDFILE
{"Audio", 0, 0, 0, FL_MENU_DIVIDER | FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{"Rx capture",  0, (Fl_Callback*)cb_mnuCapture,  0, FL_MENU_TOGGLE, FL_NORMAL_LABEL, 0, 14, 0},//70
{"Tx generate", 0, (Fl_Callback*)cb_mnuGenerate, 0, FL_MENU_TOGGLE, FL_NORMAL_LABEL, 0, 14, 0},//71
{"Playback",    0, (Fl_Callback*)cb_mnuPlayback, 0, FL_MENU_TOGGLE, FL_NORMAL_LABEL, 0, 14, 0},//72
{0,0,0,0,0,0,0,0,0},
#endif
{"E&xit", 0,  (Fl_Callback*)cb_E, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},
{"Op &Mode", 0,  0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},

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
{ "8/500", 0, cb_oliviaA, (void *)MODE_OLIVIA, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ "16/500", 0, cb_oliviaB, (void *)MODE_OLIVIA, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ "32/1000", 0, cb_oliviaC, (void *)MODE_OLIVIA, FL_MENU_DIVIDER, FL_NORMAL_LABEL, 0, 14, 0},
{ "Custom", 0, cb_oliviaCustom, (void *)MODE_OLIVIA, 0, FL_NORMAL_LABEL, 0, 14, 0},
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
{ "Custom", 0, cb_rttyCustom, (void *)MODE_RTTY, 0, FL_NORMAL_LABEL, 0, 14, 0},
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

{"Configure", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{"Defaults",  0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{"Colors-Fonts", 0, (Fl_Callback*)cb_mnuConfigFonts, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
{"Operator", 0, (Fl_Callback*)cb_mnuConfigOperator, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
{"Waterfall", 0,  (Fl_Callback*)cb_mnuConfigWaterfall, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
{"Video", 0,  (Fl_Callback*)cb_mnuConfigVideo, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
{"Rig Ctrl", 0, (Fl_Callback*)cb_mnuConfigRigCtrl, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
{"QRZ", 0,  (Fl_Callback*)cb_mnuConfigQRZ, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
{"Sound Card", 0, (Fl_Callback*)cb_mnuConfigSoundCard, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
{"Misc", 0,  (Fl_Callback*)cb_mnuConfigMisc, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},
{"Modems", 0, (Fl_Callback*)cb_mnuConfigModems, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
{"Save Config", 0, (Fl_Callback*)cb_mnuSaveConfig, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{"View", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{"Digiscope", 0, (Fl_Callback*)cb_mnuDigiscope, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
{"MFSK Image", 0, (Fl_Callback*)cb_mnuPicViewer, 0, FL_MENU_INACTIVE, FL_NORMAL_LABEL, 0, 14, 0},
{"PSK Browser", 0, (Fl_Callback*)cb_mnuViewer, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
{"Rig Control", 0, (Fl_Callback*)cb_mnuRig, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{"     ", 0, 0, 0, FL_MENU_INACTIVE, FL_NORMAL_LABEL, 0, 14, 0},

{"Help", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
#ifndef NDEBUG
// settle the gmfsk vs fldigi argument once and for all
{"@-1circle  Create sunspots", 0, cb_mnuFun, 0, FL_MENU_DIVIDER, FL_NORMAL_LABEL, 0, 14, 0},
#endif
{"Beginners' Guide", 0, cb_mnuBeginnersURL, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
{"Online documentation...", 0, cb_mnuVisitURL, (void *)PACKAGE_DOCS, 0, FL_NORMAL_LABEL, 0, 14, 0},
{"Fldigi web site...", 0, cb_mnuVisitURL, (void *)PACKAGE_HOME, FL_MENU_DIVIDER, FL_NORMAL_LABEL, 0, 14, 0},
{"Command line options", 0, cb_mnuCmdLineHelp, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
{"Audio device info", 0, cb_mnuAudioInfo, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
{"Build info", 0, cb_mnuBuildInfo, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
{"Event log", 0, cb_mnuDebug, 0, FL_MENU_DIVIDER, FL_NORMAL_LABEL, 0, 14, 0},
{"Check for updates...", 0, cb_mnuCheckUpdate, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
{"About", 0, cb_mnuAboutURL, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},
	
{"  ", 0, 0, 0, FL_MENU_INACTIVE, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},
};

Fl_Menu_Bar *mnu;

Fl_Menu_Item *getMenuItem(const char *caption)
{
	Fl_Menu_Item *item = 0;
	for (size_t i = 0; i < sizeof(menu_)/sizeof(menu_[0]); i++) {
		if (menu_[i].text && !strcmp(menu_[i].text, caption)) {
			item = menu_ + i;
			break;
		}
	}
	if (!item)
		LOG_ERROR("FIXME: could not find menu \"%s\"", caption);
	return item;
}

void activate_rig_menu_item(bool b)
{
	Fl_Menu_Item *rig = getMenuItem("Rig Control");
	if (!rig)
		return;

	if (b) {
		bSaveFreqList = true;
		rig->activate();
		
	} else {
		rig->deactivate();
		if (rigcontrol)
			rigcontrol->hide();
	}
	mnu->redraw();
}

void activate_mfsk_image_item(bool b)
{
	Fl_Menu_Item *mfsk_item = getMenuItem("MFSK Image");
	if (!mfsk_item)
		return;
	if (b)
		mfsk_item->activate();
	else
		mfsk_item->deactivate();
	mnu->redraw();
}

#if !defined(__APPLE__) && !defined(__CYGWIN__)
void make_pixmap(Pixmap *xpm, const char **data)
{
	// We need a displayed window to provide a GC for X_CreatePixmap
	Fl_Window w(0, 0, PACKAGE_NAME);
	w.xclass(PACKAGE_NAME);
	w.show();
	w.make_current();

	Fl_Pixmap icon(data);
	int maxd = MAX(icon.w(), icon.h());
	*xpm = fl_create_offscreen(maxd, maxd);

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
	main_window_title += (progdefaults.myCall.empty() ? "NO CALLSIGN SET" : progdefaults.myCall.c_str());
	if (fl_digi_main != NULL)
		fl_digi_main->label(main_window_title.c_str());
}


void create_fl_digi_main() {

	int pad = wSpace, Y = 0;

	if (progdefaults.docked_scope)
		WNOM -= 2*DEFAULT_SW;
	
    update_main_title();
    fl_digi_main = new Fl_Double_Window(WNOM, HNOM, main_window_title.c_str());
			mnu = new Fl_Menu_Bar(0, 0, WNOM - 150 - pad, Hmenu);
			// FL_NORMAL_SIZE may have changed; update the menu items
			for (size_t i = 0; i < sizeof(menu_)/sizeof(menu_[0]); i++)
				if (menu_[i].text)
					menu_[i].labelsize_ = FL_NORMAL_SIZE;
			mnu->menu(menu_);

			// reset the message dialog font
			fl_message_font(FL_HELVETICA, FL_NORMAL_SIZE);

			// reset the tooltip font
			Fl_Tooltip::font(FL_HELVETICA);
			Fl_Tooltip::size(FL_NORMAL_SIZE);

			btnRSID = new Fl_Light_Button(WNOM - 150 - pad, 0, 50, Hmenu, "RSID");
			btnRSID->selection_color(FL_GREEN);
			btnRSID->callback(cbRSID, 0);
			
			btnTune = new Fl_Light_Button(WNOM - 100 - pad, 0, 50, Hmenu, "TUNE");
			btnTune->selection_color(FL_RED);
			btnTune->callback(cbTune, 0);
			
			btnMacroTimer = new Fl_Button(WNOM - 50 - pad, 0, 50, Hmenu);
			btnMacroTimer->color(fl_rgb_color(255, 255, 100));
			btnMacroTimer->labelcolor(FL_RED);
			btnMacroTimer->callback(cbMacroTimerButton, 0);
			btnMacroTimer->hide();
			btnMacroDummy = new Fl_Button(WNOM - 50 - pad, 0, 50, Hmenu, "");
//			Fl_Box *dbx = new Fl_Box(WNOM - 50 - pad, 0, 50, Hmenu,"");
//			dbx->box(FL_BORDER_FRAME);
			
		
		Y += Hmenu;

		Fl_Group *qsoFrame = new Fl_Group(0, Y, WNOM, Hqsoframe);
			inpFreq = new Fl_Input(pad, Y + Hqsoframe/2 - pad, 85, Hqsoframe/2, "Frequency");
			inpFreq->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);

			inpTime = new Fl_Input(rightof(inpFreq) + pad, Y + Hqsoframe/2 - pad, 45, Hqsoframe/2, "Time");
			inpTime->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);

			qsoTime = new Fl_Button(rightof(inpTime) + pad, Y + Hqsoframe/2 - pad, 24, Hqsoframe/2);
			Fl_Image *pixmap = new Fl_Pixmap(cal_16);
			qsoTime->image(pixmap);
			qsoTime->callback(qsoTime_cb, 0);

			inpCall = new Fl_Input(rightof(qsoTime) + pad, Y + Hqsoframe/2 - pad, 80, Hqsoframe/2, "Call");
			inpCall->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);

			inpName = new Fl_Input(rightof(inpCall) + pad, Y + Hqsoframe/2 - pad, 100, Hqsoframe/2, "Name");
			inpName->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);

			inpRstIn = new Fl_Input(rightof(inpName) + pad, Y + Hqsoframe/2 - pad, 35, Hqsoframe/2, "RST In ");
			inpRstIn->align(FL_ALIGN_TOP | FL_ALIGN_RIGHT);

			inpRstOut = new Fl_Input(rightof(inpRstIn) + pad, Y + Hqsoframe/2 - pad, 35, Hqsoframe/2, "Out");
			inpRstOut->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);

			btnQRZ = new Fl_Button(WNOM - 40 - pad, Y + 1, 40, Hqsoframe/2 - pad, "QRZ");
			btnQRZ->callback(cb_QRZ, 0);

			inpQth = new Fl_Input(rightof(inpRstOut) + pad, Y + Hqsoframe/2 - pad,
					      leftof(btnQRZ) - rightof(inpRstOut) - 2*pad, Hqsoframe/2, "QTH");
			inpQth->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);
			qsoFrame->resizable(inpQth);

			qsoClear = new Fl_Button(WNOM - 40 - pad, Y + Hqsoframe/2 + 1, 40, Hqsoframe/2 - pad, "Clear");
			qsoClear->callback(qsoClear_cb, 0);

		qsoFrame->end();
		Y += Hqsoframe;

		Fl_Group *qsoFrame2 = new Fl_Group(0,Y, WNOM, Hnotes);
			qsoSave = new Fl_Button(WNOM - 40 - pad, Y + 1, 40, Hnotes - 2, "Save");
			qsoSave->callback(qsoSave_cb, 0);

			inpAZ = new Fl_Input(leftof(qsoSave) - 40 - pad, Y, 40, Hnotes, "Az"); // WA5ZNU
			inpAZ->align(FL_ALIGN_LEFT);

			inpLoc = new Fl_Input(leftof(inpAZ) - pad - pad - 70, Y, 70, Hnotes, "Loc");
			inpLoc->align(FL_ALIGN_LEFT);

			// align this vertically with the Call field
			inpNotes = new Fl_Input(leftof(inpLoc) - pad - (leftof(inpLoc) - leftof(inpCall)), Y, 
			                        leftof(inpLoc) - leftof(inpCall) - 2*pad, Hnotes, "Notes");
			inpNotes->align(FL_ALIGN_LEFT);
			qsoFrame2->resizable(inpNotes);

			btnSideband = new Fl_Button(leftof(inpNotes) - 2*pad - (Hnotes-2), Y+1, Hnotes-2, Hnotes-2, "U");
			btnSideband->callback(cb_btnSideband, 0);
			btnSideband->hide();
			cboBand	 = new Fl_ComboBox(pad, Y, leftof(btnSideband) - pad, Hnotes, "");
			cboBand->hide();
		qsoFrame2->end();
		Y += Hnotes;
		
		Fl_Widget* logfields[] = { inpFreq, inpTime, inpCall, inpName, inpRstIn,
					   inpRstOut, inpQth, inpAZ, inpLoc, inpNotes };
		for (size_t i = 0; i < sizeof(logfields)/sizeof(*logfields); i++)
			logfields[i]->callback(cb_log);

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
			
			if (progdefaults.DisplayMacroFilename) {
				string Macroset = "<<<===== Macro File ";
				Macroset.append(progStatus.LastMacroFile);
				Macroset.append(" Loaded =====>>>\n\n");
				ReceiveText->add(Macroset.c_str());
			}
			
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
				btnMacro[i]->tooltip("Left Click - execute\nRight Click - edit");
				colorize_macro(i);
				xpos += Wbtn;
			}
			bx = new Fl_Box(xpos, Y+2, WNOM - 32 - xpos, Hmacros - 4);
			bx->box(FL_FLAT_BOX);
			bx->color(FL_BLACK);
			btnAltMacros = new Fl_Button(WNOM-32, Y+2, 30, Hmacros - 4, "1");
			btnAltMacros->callback(altmacro_cb, 0);
			btnAltMacros->tooltip("Change macro set");
				
		Y += Hmacros;

		if (progdefaults.docked_scope) {
			Fl_Pack *wfpack = new Fl_Pack(0, Y, WNOM, Hwfall);
				wfpack->type(1);
				wf = new waterfall(0, Y, Wwfall - Hwfall + 24, Hwfall);
				wf->end();
				Fl_Pack *ypack = new Fl_Pack(Wwfall - Hwfall + 24, Y, Hwfall - 24, Hwfall);
					ypack->type(0);

					wfscope = new Digiscope (Wwfall - Hwfall, Y, Hwfall - 24, Hwfall - 24);
		
					pgrsSquelch = new Progress(
						Wwfall - Hwfall + 24, Y + Hwfall - 24,
						Hwfall - 24, 12, "");
					pgrsSquelch->color(FL_BACKGROUND2_COLOR, FL_DARK_GREEN);
					sldrSquelch = new Fl_Slider(
						FL_HOR_NICE_SLIDER, 
						Wwfall - Hwfall + 24, Y + Hwfall - 12, 
						Hwfall - 24, 12, "");
							
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
					WNOM - 2*DEFAULT_SW, Y + 4,
					DEFAULT_SW, Hwfall - 8, "");
				pgrsSquelch->color(FL_BACKGROUND2_COLOR, FL_DARK_GREEN);
				pgrsSquelch->type(Progress::VERTICAL);
				pgrsSquelch->tooltip("Detected signal level");

				sldrSquelch = new Fl_Slider(
					FL_VERT_NICE_SLIDER, 
					WNOM - DEFAULT_SW, Y + 4, 
					DEFAULT_SW, Hwfall - 8, "");
				sldrSquelch->minimum(100);
				sldrSquelch->maximum(0);
				sldrSquelch->step(1);
				sldrSquelch->value(progStatus.sldrSquelchValue);
				sldrSquelch->callback((Fl_Callback*)cb_sldrSquelch);
				sldrSquelch->color(FL_INACTIVE_COLOR);
				sldrSquelch->tooltip("Squelch level");
									
				Fl_Group::current()->resizable(wf);
			wfpack->end();
		}
		Y += (Hwfall + 2);

		Fl_Pack *hpack = new Fl_Pack(0, Y, WNOM, Hstatus);
			hpack->type(1);
			MODEstatus = new Fl_Button(0,Hmenu+Hrcvtxt+Hxmttxt+Hwfall, Wmode, Hstatus, "");
			MODEstatus->box(FL_DOWN_BOX);
			MODEstatus->color(FL_BACKGROUND2_COLOR);
			MODEstatus->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
			MODEstatus->callback(status_cb, (void *)0);
			MODEstatus->when(FL_WHEN_CHANGED);
			MODEstatus->tooltip("Left clk - change mode\nRight clk - Modem Tab");

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
                WNOM - bwSqlOnOff - bwAfcOnOff - Wwarn - rightof(Status2) - 60, 
                Hstatus, "");
			StatusBar->box(FL_DOWN_BOX);
			StatusBar->color(FL_BACKGROUND2_COLOR);
			StatusBar->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
			
			WARNstatus = new Fl_Box(
				rightof(StatusBar), Hmenu+Hrcvtxt+Hxmttxt+Hwfall, 
                Wwarn, Hstatus, "");
			WARNstatus->box(FL_DIAMOND_DOWN_BOX);
			WARNstatus->color(FL_BACKGROUND_COLOR);
			WARNstatus->labelcolor(FL_RED);
			WARNstatus->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE);
				
			AFCindicator = new AFCind(
				rightof(WARNstatus), Hmenu+Hrcvtxt+Hxmttxt+Hwfall, 
				60,
				Hstatus, "");

			int sql_width = bwSqlOnOff;
#ifdef __APPLE__
			sql_width -= 15; // leave room for resize handle
#endif
			if (useCheckButtons) {
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
			btn_afconoff->tooltip("AFC on/off");
			btn_sqlonoff->callback(sqlonoff_cb, 0);
			btn_sqlonoff->value(1);
			btn_sqlonoff->tooltip("SQL on/off");


			Fl_Group::current()->resizable(StatusBar);
		hpack->end();

	fl_digi_main->end();
	fl_digi_main->callback(cb_wMain);

#if defined (__CYGWIN__)
	fl_digi_main->icon((char*)LoadIcon(fl_display, MAKEINTRESOURCE(IDI_ICON)));
#elif defined (__linux__)
	make_pixmap(&fldigi_icon_pixmap, fldigi_icon_48_xpm);
	fl_digi_main->icon((char *)fldigi_icon_pixmap);
#endif

	fl_digi_main->xclass(PACKAGE_NAME);
	fl_digi_main->size_range(WNOM, (HNOM < 400 ? HNOM : 400));

	scopeview = new Fl_Double_Window(0,0,140,140, "Scope");
	scopeview->xclass(PACKAGE_NAME);
	digiscope = new Digiscope (0, 0, 140, 140);
	scopeview->resizable(digiscope);
	scopeview->size_range(SCOPEWIN_MIN_WIDTH, SCOPEWIN_MIN_HEIGHT);
	scopeview->end();
	scopeview->hide();	

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

Fl_Menu_Item *mnuLogging = (Fl_Menu_Item *)0;

void put_rx_char(unsigned int data)
{
	static unsigned int last = 0;
	const char **asc = ascii;

	if (mailclient || mailserver || arqmode)
		asc = ascii2;
	if (active_modem->get_mode() == MODE_RTTY ||
		active_modem->get_mode() == MODE_CW)
		asc = ascii;

	int style = FTextBase::RECV;
	if (asc == ascii2 && iscntrl(data))
		style = FTextBase::CTRL;
	if (wf->tmp_carrier())
		style = FTextBase::ALTR;

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
	
	string s = iscntrl(data) ? ascii2[data & 0x7F] : string(1, data);
	if (Maillogfile)
		Maillogfile->log_to_file(cLogfile::LOG_RX, s);

	if (!mnuLogging) mnuLogging = getMenuItem("Log File");
	if (mnuLogging)
		if (mnuLogging->value())
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

	REQ(put_status_msg, Status2, m, timeout, action);
}

void put_Status1(const char *msg, double timeout, status_timeout action)
{
	static char m[60];
	strncpy(m, msg, sizeof(m));
	m[sizeof(m) - 1] = '\0';

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
	REQ(static_cast<void (Fl_Button::*)(const char *)>(&Fl_Button::label), MODEstatus, mode_info[mode].sname);
	FL_UNLOCK_D();
	FL_AWAKE_D();
}

void put_rx_data(int *data, int len)
{
 	FHdisp->data(data, len);
}

int get_tx_char(void)
{
	if (arq_text_available)
		return arq_get_char();

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

	if (!mnuLogging) mnuLogging = getMenuItem("Log File"); // should only be called once
	if (mnuLogging)
		if (mnuLogging->value())
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

void set_AFCind(double val)
{
	REQ (&AFCind::value, AFCindicator, val );
}

void set_AFCrange(double val)
{
	REQ (&AFCind::range, AFCindicator, val);

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
}
