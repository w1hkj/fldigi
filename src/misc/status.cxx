// ----------------------------------------------------------------------------
// status.cxx
//
// Copyright (C) 2007-2010
//		Dave Freese, W1HKJ
// Copyright (C) 2008-2010
//		Stelios Bounanos, M0GLD
//
// This file is part of fldigi.
//
// Fldigi is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#include <config.h>

#include <iostream>
#include <fstream>
#include <string>

#include <FL/Fl_Preferences.H>

#include "gettext.h"
#include "main.h"
#include "globals.h"

#include "status.h"
#include "configuration.h"
#include "confdialog.h"
#include "fl_digi.h"

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

#include "Viewer.h"

#include "lgbook.h"
#include "logsupport.h"
#include "qso_db.h"

#include "misc.h"

#define STATUS_FILENAME "status"

status progStatus = {
	MODE_PSK31,			// trx_mode	lastmode;
	mode_info[MODE_PSK31].sname,	// lastmode_name
	50,					// int mainX;
	50,					// int mainY;
	WNOM,				// int mainW;
	HNOM,				// int mainH;
	false,				// bool WF_UI;
	false,				// bool NO_RIGLOG;
	false,				// bool Rig_Log_UI;
	false,				// bool Rig_Contest_UI;
	false,				// bool DOCKEDSCOPE;
	50,					// int RxTextHeight;
	WNOM / 2,			// int tiled_group_x;
	false,				// bool show_channels;
	50,					// int rigX;
	50,					// int rigY;
	560,				// int rigW
	80,					// int rigH
	1000,				// int carrier;
	0,					// int noCATfreq;
	"USB",				// string noCATmode;
	"3000",				// string noCATwidth;
	1,					// int mag;
	0,					// int offset;
	NORMAL,				// WFdisp::WFspeed
	-20,				// reflevel
	-70,				// ampspan
	30,					// uint	VIEWERnchars
	50,					// uint	VIEWERxpos
	50,					// uint	VIEWERypos
	200,				// uint VIEWERwidth
	400,				// uint VIEDWERheight
	3.0,				// double VIEWER_psksquelch
	-6.0,				// double VIEWER_rttysquelch
	false,				// bool VIEWERvisible
	100,				// int		tile_x
	200,				// int		tile_w;
	50,					// int		tile_y;
	100,				// int		tile_h;
	false,				// bool LOGenabled
	5.0,				// double sldrSquelchValue
	true,				// bool afconoff
	true,				// bool sqlonoff
	1.0,				// double	RcvMixer;
	1.0,				// double	XmtMixer;
	50,					// int	scopeX;
	50,					// int	scopeY;
	false,				// bool	scopeVisible;
	172,				// int	scopeW;
	172,				// int	scopeH;
	-1,					// int	repeatMacro;
	0,					// float	repeatIdleTime;
	0,					// int timer
	0,					// int timerMacro
	"macros.mdf",		// string LastMacroFile;
	0,					// int n_rsids
	false,				// bool spot_recv
	false,				// bool spot_log
	false,				// bool contest

	false,				// bool quick_entry
	true,				// bool rx_scroll_hints;
	true,				// bool rx_word_wrap
	true,				// bool tx_word_wrap

	20,					// int logbook_x;
	20,					// int logbook_y;
	590,				// int logbook_w;
	490,				// int logbook_h;
	false,				// bool logbook_reverse;
	85,					// int		logbook_browser_col_0;
	47,					// int		logbook_browser_col_1;
	100,				// int		logbook_browser_col_2;
	110,				// int		logbook_browser_col_3;
	120,				// int		logbook_browser_col_4;
	103,				// int		logbook_browser_col_5;

	progdefaults.contestiatones,
	progdefaults.contestiabw,
	progdefaults.contestiasmargin,
	progdefaults.contestiasinteg,
	progdefaults.contestia8bit,

	progdefaults.oliviatones,
	progdefaults.oliviabw,
	progdefaults.oliviasmargin,
	progdefaults.oliviasinteg,
	progdefaults.olivia8bit,

	progdefaults.rtty_shift,
	progdefaults.rtty_custom_shift,
	progdefaults.rtty_baud,
	progdefaults.rtty_bits,
	progdefaults.rtty_parity,
	progdefaults.rtty_stop,
	progdefaults.rtty_reverse,
	progdefaults.rtty_crcrlf,
	progdefaults.rtty_autocrlf,
	progdefaults.rtty_autocount,
	progdefaults.rtty_afcspeed,
	false, // bool rtty_filter_changed
	progdefaults.useFSKkeyline,
	progdefaults.useFSKkeylineDTR,
	progdefaults.FSKisLSB,
	progdefaults.useUART,
	progdefaults.PreferXhairScope,
	progdefaults.PseudoFSK,
	progdefaults.UOSrx,
	progdefaults.UOStx,

	"CQ",				// string browser_search;

	false				// bool bLastStateRead;
};

void status::saveLastState()
{
    int mX = fl_digi_main->x();
    int mY = fl_digi_main->y();
    if (mX >= 0 && mX >= 0) {
    	mainX = mX;
	    mainY = mY;
    }
	mainW = fl_digi_main->w();
	mainH = fl_digi_main->h();

	carrier = wf->Carrier();
	mag = wf->Mag();
	offset = wf->Offset();
	speed = wf->Speed();
	reflevel = progdefaults.wfRefLevel;
	ampspan = progdefaults.wfAmpSpan;

	logbook_x = dlgLogbook->x();
	logbook_y = dlgLogbook->y();
	logbook_w = dlgLogbook->w();
	logbook_h = dlgLogbook->h();
	logbook_reverse = cQsoDb::reverse;
	logbook_col_0 = wBrowser->columnWidth(0);
	logbook_col_1 = wBrowser->columnWidth(1);
	logbook_col_2 = wBrowser->columnWidth(2);
	logbook_col_3 = wBrowser->columnWidth(3);
	logbook_col_4 = wBrowser->columnWidth(4);
	logbook_col_5 = wBrowser->columnWidth(5);


	if (!bWF_only) {
		RxTextHeight = (ReceiveText->h() * 100) / text_panel->h();//VTgroup->h();
		quick_entry = ReceiveText->get_quick_entry();
		rx_scroll_hints = ReceiveText->get_scroll_hints();
		rx_word_wrap = ReceiveText->get_word_wrap();
		tx_word_wrap = TransmitText->get_word_wrap();

		tile_w = text_panel->w();
		tile_y = ReceiveText->h();
		tile_h = text_panel->h();
		if (text_panel->w() != ReceiveText->w())
			tile_x = mvgroup->w();
	}

	VIEWERvisible = dlgViewer->visible();
	VIEWERnchars = brwsViewer->numchars();
	if (VIEWERvisible) {
		VIEWERxpos = dlgViewer->x();
		VIEWERypos = dlgViewer->y();
		VIEWERwidth = dlgViewer->w();
		VIEWERheight = dlgViewer->h();
	}

	scopeVisible = false;
	if (scopeview && scopeview->visible()) {
		scopeVisible = true;
		scopeX = scopeview->x();
		scopeY = scopeview->y();
		scopeW = scopeview->w();
		scopeH = scopeview->h();
	}

	contestiatones = progdefaults.contestiatones;
	contestiabw = progdefaults.contestiabw;
	contestiamargin = progdefaults.contestiasmargin;
	contestiainteg = progdefaults.contestiasinteg;
	contestia8bit = progdefaults.contestia8bit;

	oliviatones = progdefaults.oliviatones;
	oliviabw = progdefaults.oliviabw;
	oliviamargin = progdefaults.oliviasmargin;
	oliviainteg = progdefaults.oliviasinteg;
	olivia8bit = progdefaults.olivia8bit;

	rtty_shift = progdefaults.rtty_shift;
	rtty_custom_shift = progdefaults.rtty_custom_shift;
	rtty_baud = progdefaults.rtty_baud;
	rtty_bits = progdefaults.rtty_bits;
	rtty_parity = progdefaults.rtty_parity;
	rtty_stop = progdefaults.rtty_stop;
	rtty_reverse = progdefaults.rtty_reverse;
	rtty_crcrlf = progdefaults.rtty_crcrlf;
	rtty_autocrlf = progdefaults.rtty_autocrlf;
	rtty_autocount = progdefaults.rtty_autocount;
	rtty_afcspeed = progdefaults.rtty_afcspeed;
	useFSKkeyline = progdefaults.useFSKkeyline;
	useFSKkeylineDTR = progdefaults.useFSKkeylineDTR;
	FSKisLSB = progdefaults.FSKisLSB;
	useUART = progdefaults.useUART;
	PreferXhairScope = progdefaults.PreferXhairScope;
	PseudoFSK = progdefaults.PseudoFSK;
	UOSrx = progdefaults.UOSrx;
	UOStx = progdefaults.UOStx;

	Fl_Preferences spref(HomeDir.c_str(), "w1hkj.com", PACKAGE_TARNAME);

	spref.set("version", PACKAGE_VERSION);
	spref.set("dual_channels", "YES");

	spref.set("mode_name", mode_info[lastmode].sname);
	spref.set("squelch_enabled", sqlonoff);
	spref.set("squelch_level", sldrSquelchValue);
	spref.set("afc_enabled", afconoff);
	spref.set("rx_mixer_level", RcvMixer);
	spref.set("tx_mixer_level", XmtMixer);

	spref.set("log_enabled", LOGenabled);

	spref.set("wf_carrier", carrier);
	spref.set("wf_mag", mag);
	spref.set("wf_offset", offset);
	spref.set("wf_speed", speed);
	spref.set("wf_reflevel", reflevel);
	spref.set("wf_ampspan", ampspan);
	spref.set("noCATfreq", noCATfreq);
	spref.set("noCATmode", noCATmode.c_str());
	spref.set("noCATwidth", noCATwidth.c_str());

	spref.set("main_x", mainX);
	spref.set("main_y", mainY);
	spref.set("main_w", mainW);
if (!bWF_only) {
	spref.set("main_h", mainH);
	spref.set("rx_text_height", RxTextHeight);
	spref.set("tiled_group_x", tiled_group_x);
	spref.set("show_channels", show_channels);
}
	spref.set("wf_ui", WF_UI);
	spref.set("riglog_ui", Rig_Log_UI);
	spref.set("rigcontest_ui", Rig_Contest_UI);
	spref.set("noriglog", NO_RIGLOG);
	spref.set("docked_scope", DOCKEDSCOPE);

	spref.set("rigctl_x", rigX);
	spref.set("rigctl_y", rigY);
	spref.set("rigctl_w", rigW);
	spref.set("rigctl_h", rigH);

	spref.set("viewer_visible", VIEWERvisible);
	spref.set("viewer_x", static_cast<int>(VIEWERxpos));
	spref.set("viewer_y", static_cast<int>(VIEWERypos));
	spref.set("viewer_w", static_cast<int>(VIEWERwidth));
	spref.set("viewer_h", static_cast<int>(VIEWERheight));
	spref.set("viewer_psksq", VIEWER_psksquelch);
	spref.set("viewer_rttysq", VIEWER_rttysquelch);
	spref.set("viewer_nchars", static_cast<int>(VIEWERnchars));

	spref.set("tile_x", tile_x);
	spref.set("tile_y", tile_y);
	spref.set("tile_w", tile_w);
	spref.set("tile_h", tile_h);

	spref.set("scope_visible", scopeVisible);
	spref.set("scope_x", scopeX);
	spref.set("scope_y", scopeY);
	spref.set("scope_w", scopeW);
	spref.set("scope_h", scopeH);

	spref.set("last_macro_file", LastMacroFile.c_str());

	spref.set("spot_recv", spot_recv);
	spref.set("spot_log", spot_recv);

	spref.set("contest", contest);
	spref.set("quick_entry", quick_entry);
	spref.set("rx_scroll_hints", rx_scroll_hints);
	spref.set("rx_word_wrap", rx_word_wrap);
	spref.set("tx_word_wrap", tx_word_wrap);

	spref.set("logbook_x", logbook_x);
	spref.set("logbook_y", logbook_y);
	spref.set("logbook_w", logbook_w);
	spref.set("logbook_h", logbook_h);
	spref.set("logbook_reverse", logbook_reverse);
	spref.set("logbook_col_0", logbook_col_0);
	spref.set("logbook_col_1", logbook_col_1);
	spref.set("logbook_col_2", logbook_col_2);
	spref.set("logbook_col_3", logbook_col_3);
	spref.set("logbook_col_4", logbook_col_4);
	spref.set("logbook_col_5", logbook_col_5);

	spref.set("contestiatones", contestiatones);
	spref.set("contestiabw", contestiabw);
	spref.set("contestiamargin", contestiamargin);
	spref.set("contestiainteg", contestiainteg);
	spref.set("contestia8bit", contestia8bit);

	spref.set("oliviaiatones", oliviatones);
	spref.set("oliviaiabw", oliviabw);
	spref.set("oliviaiamargin", oliviamargin);
	spref.set("oliviaiainteg", oliviainteg);
	spref.set("oliviaia8bit", olivia8bit);

	spref.set("rtty_shift", rtty_shift);
	spref.set("rtty_custom_shift", rtty_custom_shift);
	spref.set("rtty_baud", rtty_baud);
	spref.set("rtty_bits", rtty_bits);
	spref.set("rtty_parity", rtty_parity);
	spref.set("rtty_stop", rtty_stop);
	spref.set("rtty_reverse", rtty_reverse);
	spref.set("rtty_crcrlf", rtty_crcrlf);
	spref.set("rtty_autocrlf", rtty_autocrlf);
	spref.set("rtty_autocount", rtty_autocount);
	spref.set("rtty_afcspeed", rtty_afcspeed);
	spref.set("rtty_useFSKkeyline", useFSKkeyline);
	spref.set("rtty_useFSK_DTR", useFSKkeylineDTR);
	spref.set("rtty_FSKisLSB", FSKisLSB);
	spref.set("rtty_useUART", useUART);
	spref.set("preferxhairscope", PreferXhairScope);
	spref.set("psaudofsk", PseudoFSK);
	spref.set("uosrx", UOSrx);
	spref.set("uostx", UOStx);

	spref.set("browser_search", browser_search.c_str());

//	spref.set("xml_logbook", xml_logbook);
}

void status::loadLastState()
{
	Fl_Preferences spref(HomeDir.c_str(), "w1hkj.com", PACKAGE_TARNAME);

	int i;
	char strbuff[1000];
	char version[64]; version[sizeof(version)-1] = '\0';

	// Skip loading the rest of the status variables if we didn't read a
	// version name/value pair; or this is not a file that supports dual
	// channel browsers.
	bLastStateRead = spref.get("version", version, "", sizeof(version)-1);
	if (!bLastStateRead)
		return;
	bLastStateRead = spref.get("dual_channels", version, "", sizeof(version) - 1);
	if (!bLastStateRead)
		return;

	memset(strbuff, 0, sizeof(strbuff));
	spref.get("mode_name", strbuff, mode_info[MODE_PSK31].sname, sizeof(strbuff) - 1);
	mode_name = strbuff;
	lastmode = MODE_PSK31;
	for (i = 0; i < NUM_MODES;i++) {
		if (mode_name == mode_info[i].sname) {
			lastmode = (trx_mode) i;
			break;
		}
	}

	spref.get("squelch_enabled", i, sqlonoff); sqlonoff = i;
	spref.get("squelch_level", sldrSquelchValue, sldrSquelchValue);
	spref.get("afc_enabled", i, afconoff); afconoff = i;
	spref.get("rx_mixer_level", RcvMixer, RcvMixer);
	spref.get("tx_mixer_level", XmtMixer, XmtMixer);

	spref.get("rx_text_height", RxTextHeight, RxTextHeight);
	spref.get("tiled_group_x", tiled_group_x, tiled_group_x);
	spref.get("show_channels", i, show_channels); show_channels = i;

	spref.get("log_enabled", i, LOGenabled); LOGenabled = i;

	spref.get("wf_carrier", carrier, carrier);
	spref.get("wf_mag", mag, mag);
	spref.get("wf_offset", offset, offset);
	spref.get("wf_speed", speed, speed);
	spref.get("wf_reflevel", reflevel, reflevel);
	progdefaults.wfRefLevel = reflevel;
	spref.get("wf_ampspan", ampspan, ampspan);
	progdefaults.wfAmpSpan = ampspan;
	
	spref.get("noCATfreq", noCATfreq, noCATfreq);

	memset(strbuff, 0, sizeof(strbuff));
	spref.get("noCATmode", strbuff, "USB", sizeof(strbuff) - 1);
	noCATmode = strbuff;

	memset(strbuff, 0, sizeof(strbuff));
	spref.get("noCATwidth", strbuff, "3000", sizeof(strbuff) - 1);
	noCATwidth = strbuff;

	spref.get("main_x", mainX, mainX);
	spref.get("main_y", mainY, mainY);
	spref.get("main_w", mainW, mainW);
	spref.get("main_h", mainH, mainH);
	spref.get("wf_ui", i, WF_UI); WF_UI = i;
	spref.get("riglog_ui", i, Rig_Log_UI); Rig_Log_UI = i;
	spref.get("rigcontest_ui", i, Rig_Contest_UI); Rig_Contest_UI = i;
	spref.get("noriglog", i, NO_RIGLOG); NO_RIGLOG = i;
	spref.get("docked_scope", i, DOCKEDSCOPE); DOCKEDSCOPE = i;

	spref.get("rigctl_x", rigX, rigX);
	spref.get("rigctl_y", rigY, rigY);
	spref.get("rigctl_w", rigW, rigW);
	spref.get("rigctl_h", rigH, rigH);

	spref.get("viewer_visible", i, VIEWERvisible); VIEWERvisible = i;
	spref.get("viewer_x", i, VIEWERxpos); VIEWERxpos = i;
	spref.get("viewer_y", i, VIEWERypos); VIEWERypos = i;
	spref.get("viewer_w", i, VIEWERwidth); VIEWERwidth = i;
	spref.get("viewer_h", i, VIEWERheight); VIEWERheight = i;
	spref.get("viewer_psksq", VIEWER_psksquelch, VIEWER_psksquelch);
	spref.get("viewer_rttysq", VIEWER_rttysquelch, VIEWER_rttysquelch);
	spref.get("viewer_nchars", i, VIEWERnchars); VIEWERnchars = i;


	spref.get("tile_x", tile_x, tile_x);
	spref.get("tile_y", tile_y, tile_y);
	spref.get("tile_w", tile_w, tile_w);
	spref.get("tile_h", tile_h, tile_h);

	spref.get("scope_visible", i, scopeVisible); scopeVisible = i;
	spref.get("scope_x", scopeX, scopeX);
	spref.get("scope_y", scopeY, scopeY);
	spref.get("scope_w", scopeW, scopeW);
	spref.get("scope_h", scopeH, scopeH);

	memset(strbuff, 0, sizeof(strbuff));
	spref.get("last_macro_file", strbuff, "macros.mdf", sizeof(strbuff) - 1);
	LastMacroFile = strbuff;

	spref.get("spot_recv", i, spot_recv); spot_recv = i;
	spref.get("spot_log", i, spot_log); spot_log = i;

	spref.get("contest", i, contest); contest = i;
	spref.get("quick_entry", i, quick_entry); quick_entry = i;
	spref.get("rx_scroll_hints", i, rx_scroll_hints); rx_scroll_hints = i;
	spref.get("rx_word_wrap", i, rx_word_wrap); rx_word_wrap = i;
	spref.get("tx_word_wrap", i, tx_word_wrap); tx_word_wrap = i;

	spref.get("logbook_x", logbook_x, logbook_x);
	spref.get("logbook_y", logbook_y, logbook_y);
	spref.get("logbook_w", logbook_w, logbook_w);
	spref.get("logbook_h", logbook_h, logbook_h);
	spref.get("logbook_reverse", i, logbook_reverse); logbook_reverse = i;
	spref.get("logbook_col_0", logbook_col_0, logbook_col_0);
	spref.get("logbook_col_1", logbook_col_1, logbook_col_1);
	spref.get("logbook_col_2", logbook_col_2, logbook_col_2);
	spref.get("logbook_col_3", logbook_col_3, logbook_col_3);
	spref.get("logbook_col_4", logbook_col_4, logbook_col_4);
	spref.get("logbook_col_5", logbook_col_5, logbook_col_5);

	spref.get("contestiatones", contestiatones, contestiatones);
	spref.get("contestiabw", contestiabw, contestiabw);
	spref.get("contestiamargin", contestiamargin, contestiamargin);
	spref.get("contestiainteg", contestiainteg, contestiainteg);
	spref.get("contestia8bit", i, contestia8bit); contestia8bit = i;

	spref.get("oliviaiatones", oliviatones, oliviatones);
	spref.get("oliviaiabw", oliviabw, oliviabw);
	spref.get("oliviaiamargin", oliviamargin, oliviamargin);
	spref.get("oliviaiainteg", oliviainteg, oliviainteg);
	spref.get("oliviaia8bit", i, olivia8bit); olivia8bit = i;

	spref.get("rtty_shift", rtty_shift, rtty_shift);
	spref.get("rtty_custom_shift", rtty_custom_shift, rtty_custom_shift);
	spref.get("rtty_baud", rtty_baud, rtty_baud);
	spref.get("rtty_bits", rtty_bits, rtty_bits);
	spref.get("rtty_parity", rtty_parity, rtty_parity);
	spref.get("rtty_stop", rtty_stop, rtty_stop);
	spref.get("rtty_reverse", i, rtty_reverse); rtty_reverse = i;
	spref.get("rtty_crcrlf", i, rtty_crcrlf); rtty_crcrlf = i;
	spref.get("rtty_autocrlf", i, rtty_autocrlf); rtty_autocrlf = i;
	spref.get("rtty_autocount", rtty_autocount, rtty_autocount);
	spref.get("rtty_afcspeed", rtty_afcspeed, rtty_afcspeed);
	spref.get("rtty_useFSKkeyline", i, useFSKkeyline); useFSKkeyline = i;
	spref.get("rtty_useFSK_DTR", i, useFSKkeylineDTR); useFSKkeylineDTR = i;
	spref.get("rtty_FSKisLSB", i, FSKisLSB); FSKisLSB = i;
	spref.get("rtty_useUART", i, useUART); useUART = i;
	spref.get("preferxhairscope", i, PreferXhairScope); PreferXhairScope = i;
	spref.get("psaudofsk", i, PseudoFSK); PseudoFSK = i;
	spref.get("uosrx", i, UOSrx); UOSrx = i;
	spref.get("uostx", i, UOStx); UOStx = i;

	memset(strbuff, 0, sizeof(strbuff));
	spref.get("browser_search", strbuff, browser_search.c_str(), sizeof(strbuff) - 1);
	browser_search = strbuff;
	seek_re.recompile(browser_search.c_str());

//	spref.get("xml_logbook", i, xml_logbook); xml_logbook = i;
}

void status::initLastState()
{
	if (!bLastStateRead)
		loadLastState();

// RTTY
	if (lastmode == MODE_RTTY ) {
		if (rtty_shift > 0) {
			progdefaults.rtty_shift = rtty_shift;
			selShift->value(rtty_shift);
			selCustomShift->deactivate();
		}
		else { // Custom shift
			selShift->value(selShift->size() - 2);
			selShift->activate();
		}
		selBaud->value(progdefaults.rtty_baud = rtty_baud);
		selBits->value(progdefaults.rtty_bits = rtty_bits);
		selParity->value(progdefaults.rtty_parity = rtty_parity);
		selStopBits->value(progdefaults.rtty_stop = rtty_stop);
		btnCRCRLF->value(progdefaults.rtty_crcrlf = rtty_crcrlf);
		btnAUTOCRLF->value(progdefaults.rtty_autocrlf = rtty_autocrlf);
		cntrAUTOCRLF->value(progdefaults.rtty_autocount = rtty_autocount);
		chkPseudoFSK->value(progdefaults.PseudoFSK = PseudoFSK);
		chkUOSrx->value(progdefaults.UOSrx = UOSrx);
		chkUOStx->value(progdefaults.UOStx = UOStx);
//		chkXagc->value(progdefaults.Xagc = Xagc);
		mnuRTTYAFCSpeed->value(progdefaults.rtty_afcspeed = rtty_afcspeed);
		btnPreferXhairScope->value(progdefaults.PreferXhairScope = PreferXhairScope);

		if (mvsquelch) {
			mvsquelch->range(-12.0, 6.0);
			mvsquelch->value(VIEWER_rttysquelch);
		}
		if (sldrViewerSquelch) {
			sldrViewerSquelch->range(-12.0, 6.0);
			sldrViewerSquelch->value(VIEWER_rttysquelch);
		}
	}

	if (lastmode >= MODE_PSK_FIRST && lastmode <= MODE_PSK_LAST) {
		if (mvsquelch) {
			mvsquelch->range(-3.0, 6.0);
			mvsquelch->value(progStatus.VIEWER_psksquelch);
		}
		if (sldrViewerSquelch) {
			sldrViewerSquelch->range(-3.0, 6.0);
			sldrViewerSquelch->value(progStatus.VIEWER_psksquelch);
		}
	}

// OLIVIA
	if (lastmode == MODE_OLIVIA) {
		mnuOlivia_Tones->value(progdefaults.oliviatones = oliviatones);
		mnuOlivia_Bandwidth->value(progdefaults.oliviabw = oliviabw);
		cntOlivia_smargin->value(progdefaults.oliviasmargin = oliviamargin);
		cntOlivia_sinteg->value(progdefaults.oliviasinteg = oliviainteg);
		btnOlivia_8bit->value(progdefaults.olivia8bit = olivia8bit);
	}

// CONTESTIA
	if (lastmode == MODE_CONTESTIA) {
		mnuContestia_Tones->value(progdefaults.contestiatones = contestiatones);
		mnuContestia_Bandwidth->value(progdefaults.contestiabw = contestiabw);
		cntContestia_smargin->value(progdefaults.contestiasmargin = contestiamargin);
		cntContestia_sinteg->value(progdefaults.contestiasinteg = contestiainteg);
		btnContestia_8bit->value(progdefaults.contestia8bit = contestia8bit);
	}

	init_modem_sync(lastmode);

	wf->opmode();
	wf->Mag(mag);
	wf->Offset(offset);
	wf->Speed(speed);
	wf->setRefLevel();
	wf->setAmpSpan();

	btnAFC->value(afconoff);
	btnSQL->value(sqlonoff);
	sldrSquelch->value(sldrSquelchValue);
	valRcvMixer->value(RcvMixer * 100.0);
	valXmtMixer->value(XmtMixer * 100.0);

	if (mainX > Fl::w())
		mainX = 20;
	if (mainY > Fl::h())
		mainY = 20;
	if (mainW < WMIN || mainW > Fl::w())
		mainW = MAX(WMIN, Fl::w() / 2);
	if (mainH < HMIN || mainH > Fl::h())
		mainH = MAX(HMIN, Fl::h() / 2);

	if (bWF_only) 
		fl_digi_main->resize(mainX, mainY, mainW, Hmenu + Hwfall + Hstatus + 4);
	else {
		fl_digi_main->resize(mainX, mainY, mainW, mainH);

		set_macroLabels();

		UI_select();
	}

	if (VIEWERvisible)
		openViewer();

	if (scopeview) {
		scopeview->resize(scopeX, scopeY, scopeW, scopeH);
		if (scopeVisible == true)
			scopeview->show();
	}

	cQsoDb::reverse = logbook_reverse;
	if (cQsoDb::reverse) {
		qsodb.SortByDate(progdefaults.sort_date_time_off);
		loadBrowser();
	}

	dlgLogbook->resize(logbook_x, logbook_y, logbook_w, logbook_h);
	wBrowser->columnWidth(0, logbook_col_0);
	wBrowser->columnWidth(1, logbook_col_1);
	wBrowser->columnWidth(2, logbook_col_2);
	wBrowser->columnWidth(3, logbook_col_3);
	wBrowser->columnWidth(4, logbook_col_4);
	wBrowser->columnWidth(5, logbook_col_5);

	ReceiveText->set_quick_entry(quick_entry);
	ReceiveText->set_scroll_hints(rx_scroll_hints);
	ReceiveText->set_word_wrap(rx_word_wrap);
	TransmitText->set_word_wrap(tx_word_wrap);

//	set_server_label(xml_logbook);

}
