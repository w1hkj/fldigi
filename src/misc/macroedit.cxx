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

#include <FL/Fl_Pixmap.H>

#include "macros.h"
#include "macroedit.h"
#include "globals.h"
#include "status.h"
#include "fileselect.h"
#include "fl_lock.h"
#include "fl_digi.h"
#include "main.h"
#include "gettext.h"
#include "pixmaps.h"

using namespace std;

Fl_Double_Window *MacroEditDialog = (Fl_Double_Window *)0;

Fl_Button	*btnMacroEditOK = (Fl_Button *)0;
Fl_Button	*btnMacroEditCancel = (Fl_Button *)0;
Fl_Button	*btnInsertMacro = (Fl_Button *)0;
Fl_Input2	*macrotext = (Fl_Input2 *)0;
Fl_Input2	*labeltext = (Fl_Input2 *)0;
static int widths[] = {110, 0};

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
	w->add(_("<VER>\tFldigi version"));

	w->add(LINE_SEP);
	w->add(_("<CALL>\tother call"));
	w->add(_("<INFO1>\tS/N etc."));
	w->add(_("<INFO2>\tIMD etc."));
	w->add(_("<LOC>\tother locator"));
	w->add(_("<NAME>\tother name"));
	w->add(_("<QTH>\tother QTH"));
	w->add(_("<RST>\tother RST"));

	w->add(LINE_SEP);
	w->add(_("<CLRRX>\tclear RX pane"));

	w->add(LINE_SEP);
	w->add(_("<GET>\ttext to NAME/QTH"));

	w->add(LINE_SEP);
	w->add(_("<LOG>\tsave QSO data"));

	w->add(LINE_SEP);
	w->add(_("<QSOTIME>\tQSO time (HHMM))"));
	w->add(_("<ILDT>\tLDT in iso-8601 format"));
	w->add(_("<LDT>\tLocal datetime"));
	w->add(_("<IZDT>\tZDT in iso-8601 format"));
	w->add(_("<ZDT>\tUTC datetime"));

	w->add(LINE_SEP);
	w->add(_("<CNTR>\tcontest counter"));
	w->add(_("<DECR>\tdecrement counter"));
	w->add(_("<INCR>\tincrement counter"));
	w->add(_("<XOUT>\texchange out"));

	w->add(LINE_SEP);
	w->add(_("<RX>\treceive"));
	w->add(_("<TX>\ttransmit"));
	w->add(_("<SRCHUP>\tsearch UP for signal"));
	w->add(_("<SRCHDN>\tsearch DOWN for signal"));
	w->add(_("<GOHOME>\treturn to sweet spot"));
	w->add(_("<GOFREQ:NNNN>\tmove to freq NNNN"));

	w->add(LINE_SEP);
	w->add(_("<FILE:>\tinsert text file"));
	w->add(_("<IDLE:NN>\tidle signal for NN sec"));
	w->add(_("<TIMER:NN>\trepeat every NN sec"));
	w->add(_("<TUNE:NN>\ttune signal for NN sec"));
	w->add(_("<WAIT:NN>\tdelay xmt for NN sec"));

	w->add(LINE_SEP);
	w->add(_("<CWID>\tCW identifier"));
	w->add(_("<ID>\tmode ID"));
	w->add(_("<TEXT>\tvideo text"));
	w->add(_("<TXRSID:on|off|t>\tTx RSID on,off,toggle"));
	w->add(_("<RXRSID:on|off|t>\tRx RSID on,off,toggle"));

	w->add(LINE_SEP);
	w->add(_("<POST:+/-nn.n>\tCW QSK post-timing"));
	w->add(_("<PRE:nn.n>\tCW QSK pre-timing"));
	w->add(_("<RISE:nn.n>\tCW rise time"));
	w->add(_("<WPM:NN>\tCW WPM"));

	w->add(LINE_SEP);
	w->add(_("<AFC:on|off|t>\tAFC  on,off,toggle"));
	w->add(_("<LOCK:on|off|t>\tLOCK on,off,toggle"));

	w->add(LINE_SEP);
	w->add(_("<MACROS:>\tchange macro defs file"));

	w->add(LINE_SEP);
	char s[256];
	for (size_t i = 0; i < NUM_MODES; i++) {
		snprintf(s, sizeof(s), "<MODEM>%s", mode_info[i].sname);
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
	for (size_t i = 0; i < gbuf.gl_pathc; i++) {
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
#endif
}

void cbMacroEditOK(Fl_Widget *w, void *)
{
	if (w == btnMacroEditCancel)
		goto ret;

	if (iType == MACRO_EDIT_BUTTON) {
		macros.text[iMacro] = macrotext->value();
		macros.name[iMacro] = labeltext->value();
		int n = iMacro;
		while (n >= 12)
			n-= 12;
		btnMacro[n]->label(macros.name[iMacro].c_str());
		macros.changed = true;
	}
	else if (iType == MACRO_EDIT_INPUT)
		iInput->value(macrotext->value());
ret:
	MacroEditDialog->hide();
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
		string filters = "Text\t*." "txt";
		const char* p = FSEL::select(_("Text file to insert"), filters.c_str(),
					 "text." "txt");
		if (p) {
			text.insert(6, p);
		} else
			text = "";
	} else if (text == "<MACROS:>") {
		string filters = "Macrost\t*." "mdf";
		const char* p = FSEL::select(_("Change to Macro file"), filters.c_str(),
					 "macros." "mdf");
		if (p) {
			text.insert(8, p);
		} else
			text = "";
	}
	macrotext->insert(text.c_str());
	macrotext->take_focus();
}

Fl_Double_Window* make_macroeditor(void)
{
	Fl_Double_Window* w = new Fl_Double_Window(730, 230, "");
	labeltext = new Fl_Input2(45, 15, 115, 25, _("Label:"));
	labeltext->textfont(FL_COURIER);

	btnMacroEditOK = new Fl_Button(500, 15, 75, 25, _("OK"));
	btnMacroEditOK->callback(cbMacroEditOK);

	btnMacroEditCancel = new Fl_Button(600, 15, 75, 25, _("Cancel"));
	btnMacroEditCancel->callback(cbMacroEditOK);

	macrotext = new Fl_Input2(5, 60, 450, 165, _("Text:"));
	macrotext->type(FL_MULTILINE_INPUT);
	macrotext->textfont(FL_COURIER);
	macrotext->align(FL_ALIGN_TOP_LEFT);

	btnInsertMacro = new Fl_Button(460, 125, 25, 25);
	btnInsertMacro->image(new Fl_Pixmap(left_arrow_icon));
	btnInsertMacro->callback(cbInsertMacro);

	macroDefs = new Fl_Hold_Browser(490, 60, 235, 165);
	macroDefs->column_widths(widths);
	loadBrowser(macroDefs);
	w->end();
	w->xclass(PACKAGE_NAME);
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
	iMacro = n;
	iType = t;
	iInput = in;
	MacroEditDialog->show();
}
