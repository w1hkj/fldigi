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
	w->add("<MYCALL>\tmy call");
	w->add("<MYLOC>\tmy locator");
	w->add("<MYNAME>\tmy name");
	w->add("<MYQTH>\tmy QTH");
	w->add("<MYRST>\tmy RST");

	w->add(LINE_SEP);
	w->add("<CALL>\tother call");
	w->add("<LOC>\tother locator");
	w->add("<NAME>\tother name");
	w->add("<QTH>\tother QTH");
	w->add("<RST>\tother RST");
	w->add("<INFO1>\tS/N etc.");
	w->add("<INFO2>\tIMD etc.");
	
	w->add(LINE_SEP);
	w->add("<CLRRX>\tclear RX pane");
	
	w->add(LINE_SEP);
	w->add("<GET>\ttext to NAME/QTH");

	w->add(LINE_SEP);
	w->add("<FREQ>\tmy frequency");
	w->add("<MODE>\tmode");

	w->add(LINE_SEP);
	w->add("<QSOTIME>\tQSO time (HHMM)");
	w->add("<LDT>\tLocal datetime");
	w->add("<ILDT>\tLDT in iso-8601 format");
	w->add("<ZDT>\tUTC datetime");
	w->add("<IZDT>\tZDT in iso-8601 format");

	w->add(LINE_SEP);
	w->add("<CNTR>\tcontest counter");
	w->add("<DECR>\tdecrement counter");
	w->add("<INCR>\tincrement counter");
	w->add("<XOUT>\texchange out");

	w->add(LINE_SEP);
	w->add("<ID>\tmode ID");
	w->add("<TEXT>\tvideo text");
	w->add("<CWID>\tCW identifier");

	w->add(LINE_SEP);
	w->add("<RX>\treceive");
	w->add("<TX>\ttransmit");
	w->add("<LOG>\tsave QSO data");
	w->add("<VER>\tFldigi version");
	w->add("<TIMER:NN>\trepeat every NN sec");
	w->add("<IDLE:NN>\tidle signal for NN sec");
	
	w->add(LINE_SEP);
	w->add("<WPM:NN>\tCW WPM");
	w->add("<RISE:nn.n>\tCW rise time");
	w->add("<PRE:nn.n>\tQSK pre-timing");
	w->add("<POST:+/-nn.n>\tQSK post-timing");
	
	w->add(LINE_SEP);
	w->add("<AFC:on|off|t>\tAFC  on,off,toggle");
	w->add("<LOCK:on|off|t>\tLOCK on,off,toggle");
	w->add("<RSID:on|off|t>\tRSID on,off,toggle");

	w->add(LINE_SEP);
	w->add("<FILE:>\tinsert text file");
	w->add("<MACROS:>\tchange macro defs file");

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
		const char* p = FSEL::select("Text file to insert", filters.c_str(),
					 "text." "txt");
		if (p) {
			text.insert(6, p);
		} else
			text = "";
	} else if (text == "<MACROS:>") {
		string filters = "Macrost\t*." "mdf";
		const char* p = FSEL::select("Change to Macro file", filters.c_str(),
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
	labeltext = new Fl_Input2(45, 15, 115, 25, "Label:");
	labeltext->textfont(FL_COURIER);

	btnMacroEditOK = new Fl_Button(500, 15, 75, 25, "OK");
	btnMacroEditOK->callback(cbMacroEditOK);

	btnMacroEditCancel = new Fl_Button(600, 15, 75, 25, "Cancel");
	btnMacroEditCancel->callback(cbMacroEditOK);

	macrotext = new Fl_Input2(5, 60, 450, 165, "Text:");
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
		editor_label.append("Macro editor - ").append(progStatus.LastMacroFile);
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
