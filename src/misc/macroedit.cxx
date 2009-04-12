#include <config.h>

#include <string>
#include <glob.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>

#include "macros.h"
#include "macroedit.h"
#include "globals.h"
#include "status.h"
#include "fileselect.h"
#include "fl_lock.h"
#include "fl_digi.h"
#include "main.h"

using namespace std;

Fl_Double_Window *MacroEditDialog = (Fl_Double_Window *)0;

Fl_Button	*btnMacroEditOK = (Fl_Button *)0;
Fl_Button	*btnMacroEditCancel = (Fl_Button *)0;
Fl_Button	*btnInsertMacro = (Fl_Button *)0;
Fl_Input2	*macrotext = (Fl_Input2 *)0;
Fl_Input2	*labeltext = (Fl_Input2 *)0;
static int widths[] = {90, 0};

Fl_Hold_Browser *macroDefs=(Fl_Hold_Browser *)0;

static int iMacro;

// fl_color(0) is always the foreground colour
#define LINE_SEP "@B0"

void loadBrowser(Fl_Widget *widget) {
	Fl_Browser *w = (Fl_Browser *)widget;
	w->add("<MYCALL>\tmy call");
	w->add("<MYLOC>\tmy locator");
	w->add("<MYNAME>\tmy name");
	w->add("<MYQTH>\tmy qth");
	w->add("<MYRST>\tmy RST");

	w->add(LINE_SEP);
	w->add("<CALL>\tother call");
	w->add("<LOC>\tother locator");
	w->add("<NAME>\tremote name");
	w->add("<QTH>\tother qth");
	w->add("<RST>\tother RST");
	w->add("<INFO1>\ts/n etc.");
	w->add("<INFO2>\timd etc.");
	
	w->add(LINE_SEP);
	w->add("<CLRRX>\tclear Rx pane");
	
	w->add(LINE_SEP);
	w->add("<GET>\ttext to NAME/QTH");

	w->add(LINE_SEP);
	w->add("<FREQ>\tmy frequency");
	w->add("<MODE>\tmode");

	w->add(LINE_SEP);
	w->add("<QSOTIME>\tqso time HHMM");
	w->add("<LDT>\tLocal datetime");
	w->add("<ILDT>\tLDT in iso-8601 format");
	w->add("<ZDT>\tZulu datetime");
	w->add("<IZDT>\tZDT in iso-8601 format");

	w->add(LINE_SEP);
	w->add("<CNTR>\tcontest cnt");
	w->add("<DECR>\tdecr cnt");
	w->add("<INCR>\tincr cnt");
	w->add("<XOUT>\tExchange out");

	w->add(LINE_SEP);
	w->add("<ID>\tMode ID'r");
	w->add("<TEXT>\tVideo text");
	w->add("<CWID>\tCW identifier");

	w->add(LINE_SEP);
	w->add("<RX>\treceive");
	w->add("<TX>\ttransmit");
	w->add("<LOG>\tsave QSO data");
	w->add("<VER>\tFldigi + version");
	w->add("<TIMER:NN>\trepeat every NN sec");
	w->add("<IDLE:NN>\tidle signal for NN sec");
	
	w->add(LINE_SEP);
	w->add("<WPM:NN>\tCW wpm");
	w->add("<RISE:nn.n>\tCW risetime");
	w->add("<PRE:nn.n>\tQSK pre-timing");
	w->add("<POST:+/-nn.n>\tQSK post-timing");
	
	w->add(LINE_SEP);
	w->add("<FILE:>\tinsert text file");
	w->add("<MACROS:>\tchange macro defs file");

	w->add(LINE_SEP);
	char s[256];
	for (size_t i = 0; i < NUM_MODES; i++) {
		snprintf(s, sizeof(s), "<MODEM>%s", mode_info[i].sname);
		w->add(s);
	}

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
}

void cbMacroEditOK(Fl_Widget *w, void *)
{
	macros.text[iMacro] = macrotext->value();
	macros.name[iMacro] = labeltext->value();
	FL_LOCK_D();
		int n = iMacro;
		while (n >= 12) n-= 12;
//		if (n >= 12) n -= 12;
		btnMacro[n]->label( macros.name[iMacro].c_str());
	FL_UNLOCK_D();
	MacroEditDialog->hide();
	macros.changed = true;
}

void cbMacroEditCancel(Fl_Widget *w, void *)
{
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

static string editor_label;

Fl_Double_Window* make_macroeditor(void)
{
	editor_label.append("Macro editor - ").append(progStatus.LastMacroFile);

	Fl_Double_Window* w = new Fl_Double_Window(700, 230, editor_label.c_str());
		labeltext = new Fl_Input2(114, 15, 95, 25, "Label:");
		
		btnMacroEditOK = new Fl_Button(500, 15, 75, 25, "OK");
		btnMacroEditOK->callback(cbMacroEditOK);
		
		btnMacroEditCancel = new Fl_Button(600, 15, 75, 25, "Cancel");
		btnMacroEditCancel->callback(cbMacroEditCancel);
		
		macrotext = new Fl_Input2(5, 60, 450, 165, "Text");
		macrotext->type(4);
		macrotext->align(FL_ALIGN_TOP_LEFT);
		
		btnInsertMacro = new Fl_Button(460, 114, 25, 25, "@<<");
		btnInsertMacro->callback(cbInsertMacro);
		
		macroDefs = new Fl_Hold_Browser(490, 60, 200, 165);
		macroDefs->column_widths(widths);
		loadBrowser(macroDefs);
	w->end();
	return w;
}

void editMacro(int n)
{
	if (!MacroEditDialog) MacroEditDialog = make_macroeditor();
	else {
		editor_label = "";
		editor_label.append("Macro editor - ").append(progStatus.LastMacroFile);
		MacroEditDialog->label(editor_label.c_str());
	}
	macrotext->value(macros.text[n].c_str());
	labeltext->value(macros.name[n].c_str());
	iMacro = n;
	MacroEditDialog->xclass(PACKAGE_NAME);
	MacroEditDialog->show();
}

