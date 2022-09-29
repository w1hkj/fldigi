// ----------------------------------------------------------------------------
// macroedit.cxx
//
// Copyright (C) 2007-2009
//		Dave Freese, W1HKJ
// Copyright (C) 2009
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

#include <string>
#ifndef __MINGW32__
#  include <glob.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <cassert>

#include "macros.h"
#include "macroedit.h"
#include "globals.h"
#include "status.h"
#include "fileselect.h"
#include "fl_digi.h"
#include "main.h"
#include "gettext.h"
#include "pixmaps.h"
#include "configuration.h"

using namespace std;

Fl_Double_Window *MacroEditDialog = (Fl_Double_Window *)0;

Fl_Button	*btnMacroEditApply = (Fl_Button *)0;
Fl_Button	*btnMacroEditClose = (Fl_Button *)0;
Fl_Button	*btnInsertMacro = (Fl_Button *)0;
Fl_Input2	*macrotext = (Fl_Input2 *)0;
Fl_Input2	*labeltext = (Fl_Input2 *)0;
static int widths[] = {150, 0};

Fl_Hold_Browser *macroDefs=(Fl_Hold_Browser *)0;

static int iMacro, iType;
static Fl_Input* iInput;

// fl_color(0) is always the foreground colour
#define LINE_SEP "@B0"

void loadBrowser(Fl_Widget *widget) {
	Fl_Browser *w = (Fl_Browser *)widget;
	/* Do not translate the tags lefthand-side */
	w->add(_("<FREQ>\tmy frequency"));
	w->add(_("<MODE>\tmode"));
	w->add(_("<MYCALL>\tmy call"));
	w->add(_("<MYLOC>\tmy locator"));
	w->add(_("<MYNAME>\tmy name"));
	w->add(_("<MYQTH>\tmy QTH"));
	w->add(_("<MYRST>\tmy RST"));
	w->add(_("<MYCLASS>\tmy FD class"));
	w->add(_("<MYSECTION>\tmy FD section"));
	w->add(_("<MYSTATE>\tmy state"));
	w->add(_("<MYST>\tmy ST"));
	w->add(_("<MYCOUNTY>\tmy county"));
	w->add(_("<MYCNTY>\tmy CNTY"));
	w->add(_("<ANTENNA>\tmy antenna"));
	w->add(_("<BAND>\toperating band"));
	w->add(_("<VER>\tFldigi version"));
	w->add(_("<DIGI>\tdigital mode (adif)"));

	w->add(LINE_SEP);
	w->add(_("<CALL>\tother call"));
	w->add(_("<NAME>\tother name"));
	w->add(_("<QTH>\tother QTH"));
	w->add(_("<ST>\tother State"));
	w->add(_("<PR>\tother Province"));
	w->add(_("<LOC>\tother locator"));
	w->add(_("<RST>\tother RST"));

	w->add(LINE_SEP);
	w->add(_("<INFO1>\tS/N etc."));
	w->add(_("<INFO2>\tIMD etc."));

	w->add(LINE_SEP);
	w->add(_("<QSONBR>\t# QSO recs"));
	w->add(_("<NXTNBR>\tnext QSO rec #"));

	w->add(LINE_SEP);
	w->add(_("<MAPIT>\tmap on google"));
	w->add(_("<MAPIT:adr/lat/loc>\tmap by value"));

	w->add(LINE_SEP);
	w->add(_("<CLRRX>\tclear RX pane"));
	w->add(_("<CLRTX>\tclear TX pane"));
	w->add(_("<CLRQSO>\tclear QSO fields"));

	w->add(LINE_SEP);
	w->add(_("<GET>\ttext to NAME/QTH"));

#ifdef __WIN32__
	w->add(LINE_SEP);
	w->add(_("<TALK:on|off|t>\tDigitalk On, Off, Toggle"));
#endif

	w->add(LINE_SEP);
	w->add(_("<CLRLOG>\tclear log fields"));
	w->add(_("<LOG>\tsave QSO data"));
	w->add(_("<LOG:msg>\tsaveQSO data, append msg to notes"));
	w->add(_("<LNW>\tlog at xmt time"));
	w->add(_("<LNW:msg>\tsaveQSO data, append msg to notes"));
	w->add(_("<EQSL>\tlog eQSL"));
	w->add(_("<EQSL:[msg]>\tlog eQSL optional msg"));

	w->add(LINE_SEP);
	w->add(_("<QSOTIME>\tQSO time (HHMM))"));
	w->add(_("<ILDT[:fmt]>\tLDT default '%Y-%m-%d %H:%M%z'"));
	w->add(_("<LDT[[fmt]>\tLocal datetime, default '%x %H:%M %Z'"));
	w->add(_("<IZDT[:fmt]>\tZDT default '%Y-%m-%d %H:%MZ'"));
	w->add(_("<ZDT[:fmt]>\tUTC datetime, default '%x %H:%MZ'"));
	w->add(_("<LT[:fmt]>\tlocal time, default %H%M"));
	w->add(_("<ZT[:fmt]>\tzulu time default %H%MZ"));
	w->add(_("<LD[:fmt]>\tlocal date, default '%Y-%M-%D'"));
	w->add(_("<ZD[:fmt]>\tzulu date, default '%Y-%M-%D Z'"));
	w->add(_("<WX>\tget weather data"));
	w->add(_("<WX:xxxx>\tget weather data for station"));

	w->add(LINE_SEP);
	w->add(_("<CNTR>\tcontest counter"));
	w->add(_("<DECR>\tdecrement counter"));
	w->add(_("<INCR>\tincrement counter"));
	w->add(_("<XIN>\texchange in"));
	w->add(_("<XOUT>\texchange out"));
	w->add(_("<XBEG>\texchange begin"));
	w->add(_("<XEND>\texchange end"));
	w->add(_("<SAVEXCHG>\tsave contest out"));
	w->add(_("<SERNO>\tcurrent contest serno"));
	w->add(_("<LASTNO>\tlast serno sent"));
	w->add(_("<FDCLASS>\tFD class"));
	w->add(_("<FDSECT>\tFD section"));
	w->add(_("<CLASS>\tcontest class"));
	w->add(_("<SECTION>\tARRL section"));

	w->add(LINE_SEP);
	w->add(_("<RX>\treceive"));
	w->add(_("<TX>\ttransmit"));
	w->add(_("<TX/RX>\ttoggle T/R"));
	w->add(_("<SRCHUP>\tsearch UP for signal"));
	w->add(_("<SRCHDN>\tsearch DOWN for signal"));
	w->add(_("<GOHOME>\treturn to sweet spot"));
	w->add(_("<GOFREQ:NNNN>\tmove to freq NNNN Hz"));
	w->add(_("<QSYTO>\tleft-clk QSY button"));
	w->add(_("<QSYFM>\tright-clk QSY button"));
	w->add(_("<QSY:FFF.F[:NNNN]>\tqsy to kHz, Hz"));
	w->add(_("<QSY+:+/-n.nnn>\tincr/decr xcvr freq"));
	w->add(_("<RIGMODE:mode>\tvalid xcvr mode"));
	w->add(_("<FILWID:width>\tvalid xcvr filter width"));
	w->add(_("<RIGLO:lowcut>\tvalid xcvr low cutoff filter"));
	w->add(_("<RIGHI:hicut>\tvalid xcvr hi cutoff filter"));
	w->add(_("<FOCUS>\trig freq has kbd focus"));

	w->add(LINE_SEP);
	w->add(_("<QRG:text>\tinsert QRG into Rx text"));

	w->add(LINE_SEP);
	w->add(_("<FILE:>\tinsert text file"));
	w->add(_("<IMAGE:>\tinsert image file"));
	w->add(_("<AVATAR>\tsend avatar"));
	w->add(LINE_SEP);

	w->add(_("<PAUSE>\tpause transmit"));
	w->add(_("<IDLE:NN.nn>\tidle signal for NN.nn sec"));
	w->add(_("<TIMER:NN>\trepeat every NN sec"));
	w->add(_("<AFTER:NN>\trepeat after waiting NN sec"));
	w->add(_("<TUNE:NN>\ttune signal for NN sec"));
	w->add(_("<WAIT:NN.n>\tdelay xmt for NN.n sec"));
	w->add(_("<REPEAT>\trepeat macro continuously"));
	w->add(_("<SKED:hhmm[ss][:YYYYMMDD]>\tschedule execution for"));
	w->add(_("<UNTIL:hhmm[ss][:YYYYMMDD]>\tend execution at"));
	w->add(_("<LOCAL>\tuse local date/time"));

	w->add(LINE_SEP);
	w->add(_("<TXATTEN:nn.n>\t set xmt attenuator"));

	w->add(LINE_SEP);
	w->add(_("<CWID>\tCW identifier"));
	w->add(_("<ID>\tsend mode ID; TX start only"));
	w->add(_("<TEXT>\ttext at start of TX"));
	w->add(_("<VIDEO:\tvideo text in TX stream"));
	w->add(_("<TXRSID:on|off|t>\tTx RSID on,off,toggle"));
	w->add(_("<RXRSID:on|off|t>\tRx RSID on,off,toggle"));
	w->add(_("<NRSID:NN>\tTransmit |NN| successive RsID bursts"));
	w->add(_("<DTMF:[Wn:][Ln:]chrs>\t[Wait][Len](ms)"));

	w->add(LINE_SEP);
	w->add(_("<AUDIO:>\tXmt audio wav file"));

	w->add(LINE_SEP);
	w->add(_("<ALERT:[bark][checkout][doesnot][phone][beeboo][diesel][steam_train][dinner_bell][standard_tone]>"));
	w->add(_("<ALERT:>\talert using external wav file"));

	w->add(LINE_SEP);
	w->add(_("<POST:+/-nn.n>\tCW QSK post-timing"));
	w->add(_("<PRE:nn.n>\tCW QSK pre-timing"));
	w->add(_("<RISE:nn.n>\tCW rise time"));
	w->add(_("<WPM:NN.nn:FF.nn>\tChar WPM:Text WPM (15.0:5.0)"));

	w->add(LINE_SEP);
	w->add(_("<RIGCAT:[\"text\"][hex ...]:ret>\tsend CAT cmd"));
	w->add(_("<FLRIG:[\"text\"][hex ...]>\tsend CAT cmd"));

	w->add(LINE_SEP);
	w->add(_("<AFC:on|off|t>\tAFC  on,off,toggle"));
	w->add(_("<LOCK:on|off|t>\tLOCK on,off,toggle"));
	w->add(_("<REV:on|off|t>\tRev on,off,toggle"));

	w->add(LINE_SEP);
	w->add(_("<MACROS:>\tchange macro defs file"));
	w->add(_("<SAVE>\tsave current macro file"));
	w->add(_("<BUFFERED>\trun macro from buffered teext"));

	w->add(LINE_SEP);
	w->add(_("<COMMENT:comment text>\tignore comment text"));
	w->add(_("<#comments>\t ignore comments"));

	w->add(LINE_SEP);
	w->add(_("<CPS_TEST:nn>\tmodem char/sec test on nn chars"));
	w->add(_("<CPS_N:n>\tmodem timing test, 'n' random 5 char groups"));
	w->add(_("<CPS_FILE:>\tmodem timing test, spec' file"));
	w->add(_("<CPS_STRING:s>\tmodem timing test, string 's'"));

	w->add(LINE_SEP);
	w->add(_("<WAV_TEST>\tWAV file; internal string"));
	w->add(_("<WAV_N:n>\tWAV file; 'n' random 5 char groups"));
	w->add(_("<WAV_FILE:>\tWAV file; spec' file"));
	w->add(_("<WAV_STRING:s>\tWAV file; string 's'"));

	w->add(LINE_SEP);
	w->add(_("<CSV:on|off|t>\tAnalysis CSV on,off,toggle"));

	w->add(LINE_SEP);
	w->add(_("<PUSH>\tpush current mode to stack"));
	w->add(_("<PUSH:m|f\tpush current mode / audio freq to stack"));
	w->add(_("<POP>\tpop current mode/freq from stack"));
	w->add(LINE_SEP);
	assert(MODE_CONTESTIA < MODE_OLIVIA);
	char s[256];
	for (trx_mode i = 0; i <= MODE_CONTESTIA; i++) {
		snprintf(s, sizeof(s), "<MODEM:%s>", mode_info[i].sname);
		w->add(s);
	}
	// add some Contestia macros
	const char* contestia[] = { "250:8", "500:8", "500:16", "1000:8", "1000:16" };
	for (size_t i = 0; i < sizeof(contestia)/sizeof(*contestia); i++) {
		snprintf(s, sizeof(s), "<MODEM:%s:%s>", mode_info[MODE_CONTESTIA].sname, contestia[i]);
		w->add(s);
	}
	for (trx_mode i = MODE_CONTESTIA + 1; i <= MODE_OLIVIA; i++) {
		snprintf(s, sizeof(s), "<MODEM:%s>", mode_info[i].sname);
		w->add(s);
	}
	assert(MODE_OLIVIA < MODE_RTTY);
	// add some Olivia macros
	const char* olivia[] = { "250:8", "500:8", "500:16", "1000:8", "1000:32" };
	for (size_t i = 0; i < sizeof(olivia)/sizeof(*olivia); i++) {
		snprintf(s, sizeof(s), "<MODEM:%s:%s>", mode_info[MODE_OLIVIA].sname, olivia[i]);
		w->add(s);
	}
	for (trx_mode i = MODE_OLIVIA + 1; i <= MODE_RTTY; i++) {
		snprintf(s, sizeof(s), "<MODEM:%s>", mode_info[i].sname);
		w->add(s);
	}
	// add some RTTY macros
	const char* rtty[] = { "170:45.45:5", "170:50:5", "850:75:5" };
	for (size_t i = 0; i < sizeof(rtty)/sizeof(*rtty); i++) {
		snprintf(s, sizeof(s), "<MODEM:%s:%s>", mode_info[MODE_RTTY].sname, rtty[i]);
		w->add(s);
	}
	for (trx_mode i = MODE_RTTY + 1; i < NUM_MODES; i++) {
		snprintf(s, sizeof(s), "<MODEM:%s>", mode_info[i].sname);
		w->add(s);
	}

#ifndef __MINGW32__
	glob_t gbuf;
	glob(string(ScriptsDir).append("*").c_str(), 0, NULL, &gbuf);
	if (gbuf.gl_pathc == 0) {
		globfree(&gbuf);
		return;
	}
	w->add(LINE_SEP);
	struct stat st;

#  if defined(__OpenBSD__)
	for (int i = 0; i < gbuf.gl_pathc; i++) {
#  else
	for (size_t i = 0; i < gbuf.gl_pathc; i++) {
#  endif
		if (!(stat(gbuf.gl_pathv[i], &st) == 0
		      && S_ISREG(st.st_mode) && (st.st_mode & S_IXUSR)))
			continue;

		const char* p;
		if ((p = strrchr(gbuf.gl_pathv[i], '/'))) {
			snprintf(s, sizeof(s), "<EXEC>%s</EXEC>", p+1);
			w->add(s);
		}
	}
	globfree(&gbuf);
#else
	w->add("<EXEC>\tlaunch a program");
#endif
}

void cbMacroEditOK(Fl_Widget *w, void *)
{
	if (w == btnMacroEditClose) {
		MacroEditDialog->hide();
		return;
	}

	if (iType == MACRO_EDIT_BUTTON) {
		update_macro_button(iMacro, macrotext->value(), labeltext->value());
	}
	else if (iType == MACRO_EDIT_INPUT)
		iInput->value(macrotext->value());
}

void update_macro_button(int iMacro, const char *text, const char *name)
{
	macros.text[iMacro].assign(text);
	macros.name[iMacro].assign(name);

	if (progdefaults.mbar_scheme > MACRO_SINGLE_BAR_MAX) {
		if (iMacro < NUMMACKEYS) {
			btnMacro[iMacro]->label( macros.name[iMacro].c_str() );
			btnMacro[iMacro]->redraw_label();
		} else if ((iMacro / NUMMACKEYS) == altMacros) {
			btnMacro[(iMacro % NUMMACKEYS) + NUMMACKEYS]->label( macros.name[iMacro].c_str() );
			btnMacro[(iMacro % NUMMACKEYS) + NUMMACKEYS]->redraw_label();
		}
	} else {
		btnMacro[iMacro % NUMMACKEYS]->label( macros.name[iMacro].c_str() );
		btnMacro[iMacro % NUMMACKEYS]->redraw_label();
	}
	btnDockMacro[iMacro]->label(macros.name[iMacro].c_str());
	btnDockMacro[iMacro]->redraw_label();

	macros.changed = true;
}

void cbInsertMacro(Fl_Widget *, void *)
{
	int nbr = macroDefs->value();
	if (!nbr) return;
	string edittext = macrotext->value();
	string text = macroDefs->text(nbr);
	size_t tab = text.find('\t');
	if (tab != string::npos)
		text.erase(tab);
	if (text == LINE_SEP)
		return;
	if (text == "<FILE:>") {
		string filters = "Text\t*.txt";
		const char* p = FSEL::select(
			_("Text file to insert"),
			filters.c_str(),
			HomeDir.c_str());
		if (p && *p) {
			text.insert(6, p);
		} else
			text = "";
	} else if ((text == "<CPS_FILE:>") || (text == "<WAV_FILE:>")) {
		string filters = "Text\t*.txt";
		const char* p = FSEL::select(
			_("Test text file"),
			filters.c_str(),
			HomeDir.c_str());
		if (p && *p) {
			text.insert(10, p);
		} else
			text = "";
	} else if (text == "<IMAGE:>") {
		string filters = "*.{png,jpg,bmp}\t*.png";
		const char *p = FSEL::select(
			_("MFSK image file"),
			filters.c_str(),
			PicsDir.c_str());
		if (p && *p) {
			text.insert(7, p);
		} else
			text = "";
	} else if (text == "<MACROS:>") {
		string filters = "Macrost\t*.mdf";
		const char* p = FSEL::select(
			_("Change to Macro file"),
			filters.c_str(),
			MacrosDir.c_str());
		if (p && *p) {
			text.insert(8, p);
		} else
			text = "";
	} else if (text == "<ALERT:>") {
		string filters = "Audio file\t*.{mp3,wav}";
		const char* p = FSEL::select(
			_("Select audio file"),
			filters.c_str(),
			HomeDir.c_str());
		if (p && *p) {
			text.insert(7, p);
		} else
			text = "";
	} else if (text == "<AUDIO:>") {
		string filters = "Audio file\t*.{mp3,wav}";
		const char* p = FSEL::select(
			_("Select audio file"),
			filters.c_str(),
			HomeDir.c_str());
		if (p && *p) {
			text.insert(7, p);
		} else
			text = "";
	}
#ifdef __MINGW32__
	else if (text == "<EXEC>") {
		string filters = "Exe\t*.exe";
		const char* p = FSEL::select(
			_("Executable file to insert"),
			filters.c_str(),
			HomeDir.c_str());
		if (p && *p) {
			string exefile = p;
			exefile.append("</EXEC>");
			text.insert(6, exefile);
		} else
			text = "";
	}
#endif
	macrotext->insert(text.c_str());
	macrotext->take_focus();
}

#include <FL/Fl_Tile.H>

Fl_Double_Window* make_macroeditor(void)
{
	Fl_Double_Window* w = new Fl_Double_Window(800, 190, "");

	Fl_Group *grpA = new Fl_Group(0, 0, 800, 22);
	Fl_Group *grpB = new Fl_Group(450, 0, 350, 22);
	btnInsertMacro = new Fl_Button(450, 2, 40, 20);
	btnInsertMacro->image(new Fl_Pixmap(left_arrow_icon));
	btnInsertMacro->callback(cbInsertMacro);
	grpB->end();
	grpA->end();

	Fl_Group *grpC = new Fl_Group(0, 22, 800, 140);
	Fl_Tile *tile = new Fl_Tile(0,22,800,140);
	macrotext = new Fl_Input2(0, 22, 450, 140, _("Macro Text"));
	macrotext->type(FL_MULTILINE_INPUT);
	macrotext->textfont(FL_HELVETICA);
	macrotext->align(FL_ALIGN_TOP);

	macroDefs = new Fl_Hold_Browser(450, 22, 350, 140, _("Select Tag"));
	macroDefs->column_widths(widths);
	macroDefs->align(FL_ALIGN_TOP);

	Fl_Box *minbox = new Fl_Box(200, 22, 400, 140);
	minbox->hide();
	tile->end();
	tile->resizable(minbox);
	grpC->end();

	Fl_Group *grpD = new Fl_Group(0, 164, 452, 24);
	Fl_Box *box3a = new Fl_Box(0, 164, 327, 24, "");
	labeltext = new Fl_Input2(337, 164, 115, 24, _("Macro Button Label"));
	labeltext->textfont(FL_HELVETICA);
	grpD->end();
	grpD->resizable(box3a);

	Fl_Group *grpE = new Fl_Group(452, 164, 348, 24);
	Fl_Box *box4a = new Fl_Box(452, 164, 92, 24, "");

	btnMacroEditApply = new Fl_Button(544, 164, 80, 24, _("Apply"));
	btnMacroEditApply->callback(cbMacroEditOK);

	btnMacroEditClose = new Fl_Button(626 , 164, 80, 24, _("Close"));
	btnMacroEditClose->callback(cbMacroEditOK);
	grpE->end();
	grpE->resizable(box4a);

	w->end();

	w->resizable(grpC);

	w->size_range( 600, 120);

	w->xclass(PACKAGE_NAME);

	loadBrowser(macroDefs);

	return w;
}

void editMacro(int n, int t, Fl_Input* in)
{
	if (!MacroEditDialog)
		MacroEditDialog = make_macroeditor();
	if (t == MACRO_EDIT_BUTTON) {
		string editor_label;
		editor_label.append(_("Macro editor - ")).append(progStatus.LastMacroFile);
		if (editor_label != MacroEditDialog->label())
			MacroEditDialog->copy_label(editor_label.c_str());
		macrotext->value(macros.text[n].c_str());
		labeltext->value(macros.name[n].c_str());
		labeltext->show();
	}
	else if (t == MACRO_EDIT_INPUT) {
		MacroEditDialog->label(in->label());
		macrotext->value(in->value());
		labeltext->hide();
	}
	macrotext->textfont(progdefaults.MacroEditFontnbr);
	macrotext->textsize(progdefaults.MacroEditFontsize);
	iMacro = n;
	iType = t;
	iInput = in;
	MacroEditDialog->show();
}

void update_macroedit_font()
{
	if (!MacroEditDialog) return;
	if (!MacroEditDialog->visible()) return;
	macrotext->textfont(progdefaults.MacroEditFontnbr);
	macrotext->textsize(progdefaults.MacroEditFontsize);
	MacroEditDialog->redraw();
}
