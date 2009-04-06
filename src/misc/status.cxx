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
#include "fl_digi.h"
#include "versions.h"

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

#define STATUS_FILENAME "status"

status progStatus = {
	MODE_BPSK31,		// trx_mode	lastmode;
	50,					// int mainX;
	50,					// int mainY;
	0,				// int mainW;
	0,				// int mainH;
	0,				// int RxTextHeight;
	false,				// bool rigShown;
	50,					// int rigX;
	50,					// int rigY;
	560,				// int rigW
	80,					// int rigH
	1000,				// int carrier;
	1,					// int mag;
	NORMAL,				// WFdisp::WFspeed
	-20,				// reflevel
	-70,				// ampspan
	40,					// uint	VIEWERnchars
	50,					// uint	VIEWERxpos
	50,					// uint	VIEWERypos
	false,				// bool VIEWERvisible
	false,				// bool LOGenabled
	5.0,				// double sldrSquelchValue
	true,				// bool afconoff
	true,				// bool sqlonoff
	1.0,				// double	RcvMixer;
	1.0,				// double	XmtMixer;
	0,					// int	scopeX;
	0,					// int	scopeY;
	false,				// bool	scopeVisible;
	50,					// int	scopeW;
	50,					// int	scopeH;
	"macros.mdf",		// string LastMacroFile;
	false,			// bool spot_recv
	false,			// bool spot_log
	false,			// bool contest

	false,				// bool quick_entry
	true,				// bool rx_word_wrap
	true,				// bool tx_word_wrap

	50,					// int logbook_x;
	50,					// int logbook_y;
	380,				// int logbook_w;
	290,				// int logbook_h;
	false,				// bool logbook_reverse;
	85,					// int		logbook_browser_col_0;
	50,					// int		logbook_browser_col_1;
	100,				// int		logbook_browser_col_2;
	110,				// int		logbook_browser_col_3;
	120,				// int		logbook_browser_col_4;
	90,					// int		logbook_browser_col_5;

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
	RxTextHeight = ReceiveText->h();
	carrier = wf->Carrier();
	mag = wf->Mag();
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
	
	quick_entry = ReceiveText->QuickEntry();
	rx_word_wrap = ReceiveText->WordWrap();
	tx_word_wrap = TransmitText->WordWrap();
	
	VIEWERvisible = false;
	if (dlgViewer && dlgViewer->visible()) {
		VIEWERxpos = dlgViewer->x();
		VIEWERypos = dlgViewer->y();
		VIEWERvisible = true;
	}

	rigShown = false;
	if (rigcontrol && rigcontrol->visible()) {
		rigX = rigcontrol->x();
		rigY = rigcontrol->y();
		rigW = rigcontrol->w();
		rigH = rigcontrol->h();
		rigShown = true;
	}

	scopeVisible = false;
	if (scopeview && scopeview->visible()) {
		scopeVisible = true;
		scopeX = scopeview->x();
		scopeY = scopeview->y();
		scopeW = scopeview->w();
		scopeH = scopeview->h();
	}

	Fl_Preferences spref(HomeDir.c_str(), "w1hkj.com", PACKAGE_TARNAME);

	spref.set("version", PACKAGE_VERSION);

	spref.set("mode", (int)lastmode);
	spref.set("squelch_enabled", sqlonoff);
	spref.set("squelch_level", sldrSquelchValue);
	spref.set("afc_enabled", afconoff);
	spref.set("rx_mixer_level", RcvMixer);
	spref.set("tx_mixer_level", XmtMixer);

	spref.set("rx_text_height", RxTextHeight);
	spref.set("log_enabled", LOGenabled);

	spref.set("wf_carrier", carrier);
	spref.set("wf_mag", mag);
	spref.set("wf_speed", speed);
	spref.set("wf_reflevel", reflevel);
	spref.set("wf_ampspan", ampspan);

	spref.set("main_x", mainX);
	spref.set("main_y", mainY);
	spref.set("main_w", mainW);
	spref.set("main_h", mainH);

	spref.set("rigctl_visible", rigShown);
	spref.set("rigctl_x", rigX);
	spref.set("rigctl_y", rigY);
	spref.set("rigctl_w", rigW);
	spref.set("rigctl_h", rigH);

	spref.set("viewer_visible", VIEWERvisible);
	spref.set("viewer_x", static_cast<int>(VIEWERxpos));
	spref.set("viewer_y", static_cast<int>(VIEWERypos));
	spref.set("viewer_nchars", static_cast<int>(VIEWERnchars));

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

}

void status::loadLastState()
{
	Fl_Preferences spref(HomeDir.c_str(), "w1hkj.com", PACKAGE_TARNAME);
	
	char version[64]; version[sizeof(version)-1] = '\0';
	
	bLastStateRead = spref.get("version", version, "", sizeof(version)-1);
	// Skip loading the rest of the status variables if we didn't read a
	// version name/value pair; also clear everything to avoid creating
	// entries out of existing file contents.
	if (!bLastStateRead) {
//		while (spref.entries())
//			spref.deleteEntry(spref.entry(0));
		return;
	}

	int i;

	spref.get("mode", i, i);  lastmode = (trx_mode) i;
	spref.get("squelch_enabled", i, i); sqlonoff = i;
	spref.get("squelch_level", sldrSquelchValue, sldrSquelchValue);
	spref.get("afc_enabled", i, i); afconoff = i;
	spref.get("rx_mixer_level", RcvMixer, RcvMixer);
	spref.get("tx_mixer_level", XmtMixer, XmtMixer);

	spref.get("rx_text_height", RxTextHeight, RxTextHeight);
	spref.get("log_enabled", i, i); LOGenabled = i;

	spref.get("wf_carrier", carrier, carrier);
	spref.get("wf_mag", mag, mag);
	spref.get("wf_speed", speed, speed);
	spref.get("wf_reflevel", reflevel, reflevel);
	progdefaults.wfRefLevel = reflevel;
	spref.get("wf_ampspan", ampspan, ampspan);
	progdefaults.wfAmpSpan = ampspan;

	spref.get("main_x", mainX, mainX);
	spref.get("main_y", mainY, mainY);
	spref.get("main_w", mainW, mainW);
	spref.get("main_h", mainH, mainH);

	spref.get("rigctl_visible", i, i); rigShown = i;
	spref.get("rigctl_x", rigX, rigX);
	spref.get("rigctl_y", rigY, rigY);
	spref.get("rigctl_w", rigW, rigW);
	spref.get("rigctl_h", rigH, rigH);

	spref.get("viewer_visible", i, i); VIEWERvisible = i;
	spref.get("viewer_x", i, i); VIEWERxpos = i;
	spref.get("viewer_y", i, i); VIEWERypos = i;
	spref.get("viewer_nchars", i, i); VIEWERnchars = i;

	spref.get("scope_visible", i, i); scopeVisible = i;
	spref.get("scope_x", scopeX, scopeX);
	spref.get("scope_y", scopeY, scopeY);
	spref.get("scope_w", scopeW, scopeW);
	spref.get("scope_h", scopeH, scopeH);

	char *defbuffer;
	spref.get("last_macro_file", defbuffer, "macros.mdf");
	LastMacroFile = defbuffer;
	if (defbuffer) free(defbuffer);

	spref.get("spot_recv", i, i); spot_recv = i;
	spref.get("spot_log", i, i); spot_log = i;

	spref.get("contest", i, i); contest = i;
	spref.get("quick_entry", i, i); quick_entry = i;
	spref.get("rx_word_wrap", i, i); rx_word_wrap = i;
	spref.get("tx_word_wrap", i, i); tx_word_wrap = i;

	spref.get("logbook_x", logbook_x, logbook_x);
	spref.get("logbook_y", logbook_y, logbook_y);
	spref.get("logbook_w", logbook_w, logbook_w);
	spref.get("logbook_h", logbook_h, logbook_h);
	spref.get("logbook_reverse", i, i); logbook_reverse = i;
	spref.get("logbook_col_0", logbook_col_0, logbook_col_0);
	spref.get("logbook_col_1", logbook_col_1, logbook_col_1);
	spref.get("logbook_col_2", logbook_col_2, logbook_col_2);
	spref.get("logbook_col_3", logbook_col_3, logbook_col_3);
	spref.get("logbook_col_4", logbook_col_4, logbook_col_4);
	spref.get("logbook_col_5", logbook_col_5, logbook_col_5);

}

void status::initLastState()
{
	if (!bLastStateRead)
		loadLastState();

	init_modem(lastmode);

	while (!active_modem) MilliSleep(100);

 	wf->opmode();
	wf->Mag(mag);
	wf->Speed(speed);
	wf->setRefLevel();
	wf->setAmpSpan();
	wf->movetocenter();

	btn_afconoff->value(afconoff);
	btn_sqlonoff->value(sqlonoff);
	sldrSquelch->value(sldrSquelchValue);
	valRcvMixer->value(RcvMixer * 100.0);
	valXmtMixer->value(XmtMixer * 100.0);

	if (mainW == 0)
		mainW = MAX(WMIN, Fl::w() / 2);
	if (mainH == 0)
		mainH = MAX(HMIN, Fl::h() / 2);
	fl_digi_main->resize(mainX, mainY, mainW, mainH);

	if (!(RxTextHeight > 0 && RxTextHeight < TiledGroup->h()))
		RxTextHeight = TiledGroup->h() / 3 * 2;
	TiledGroup->position(0, TransmitText->y(), 0, TiledGroup->y() + RxTextHeight);

	if (!progdefaults.docked_rig_control && rigShown == true) {
		if (!rigcontrol)
			createRigDialog();
		rigcontrol->resize(rigX, rigY, rigW, rigH);
		rigcontrol->show();
	}
	if (VIEWERvisible == true)
		openViewer();

	if (scopeview) {
		scopeview->resize(scopeX, scopeY, scopeW, scopeH);
		if (scopeVisible == true)
			scopeview->show();
	}

	cQsoDb::reverse = logbook_reverse;
	if (cQsoDb::reverse) {
		qsodb.SortByDate();
		loadBrowser();
	}
	
	dlgLogbook->resize(logbook_x, logbook_y, logbook_w, logbook_h);
	wBrowser->columnWidth(0, logbook_col_0);
	wBrowser->columnWidth(1, logbook_col_1);
	wBrowser->columnWidth(2, logbook_col_2);
	wBrowser->columnWidth(3, logbook_col_3);
	wBrowser->columnWidth(4, logbook_col_4);
	wBrowser->columnWidth(5, logbook_col_5);
	
	ReceiveText->QuickEntry(quick_entry);
	ReceiveText->WordWrap(rx_word_wrap);
	TransmitText->WordWrap(tx_word_wrap);
	
}
