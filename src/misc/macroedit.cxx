#include "macros.h"
#include "macroedit.h"

#include <string>

using namespace std;

Fl_Double_Window *MacroEditDialog = (Fl_Double_Window *)0;

Fl_Button	*btnMacroEditOK = (Fl_Button *)0;
Fl_Button	*btnMacroEditCancel = (Fl_Button *)0;
Fl_Button	*btnInsertMacro = (Fl_Button *)0;
Fl_Input	*macrotext = (Fl_Input *)0;
Fl_Input	*labeltext = (Fl_Input *)0;
static int widths[] = {90, 0};

Fl_Hold_Browser *macroDefs=(Fl_Hold_Browser *)0;

static int iMacro;

void loadBrowser(Fl_Widget *widget) {
	Fl_Browser *w = (Fl_Browser *)widget;
	w->add("<CALL>\tother call");
	w->add("<FREQ>\tmy frequency");
	w->add("<LOC>\tother locator");
	w->add("<MODE>\tmode");
	w->add("<NAME>\tremote name");
	w->add("<QTH>\tother qth");
	w->add("<RST>\tother RST");
	w->add("<MYCALL>\tmy call");
	w->add("<MYLOC>\tmy locator");
	w->add("<MYNAME>\tmy name");
	w->add("<MYQTH>\tmy qth");
	w->add("<MYRST>\tmy RST");
	w->add("<LDT>\tLocal datetime");
	w->add("<ZDT>\tZulu datetime");
	w->add("<ID>\tMode ID'r");
	w->add("<TEXT>\tVideo text");
	w->add("<CWID>\tCW identifier");
	w->add("<RX>\treceive");
	w->add("<TX>\ttransmit");
	w->add("<VER>\tFldigi + version");
	w->add("<CNTR>\tcontest cnt");
	w->add("<DECR>\tdecr cnt");
	w->add("<INCR>\tincr cnt");
	w->add("<LOG>\tsave QSO data");
	w->add("<TIMER>\trepeat every NNN sec");
}

void cbMacroEditOK(Fl_Widget *w, void *)
{
	macros.text[iMacro] = macrotext->value();
	macros.name[iMacro] = labeltext->value();
	FL_LOCK_D();
		int n = iMacro;
		if (n >= 10) n -= 10;
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
	text.erase(tab);
	macrotext->insert(text.c_str());
	macrotext->take_focus();
}

Fl_Double_Window* make_macroeditor() {
	Fl_Double_Window* w;
	{ Fl_Double_Window* o = new Fl_Double_Window(700, 230, "Edit User Macro");
		w = o;
		labeltext = new Fl_Input(114, 15, 95, 25, "Label:");
		
		btnMacroEditOK = new Fl_Button(500, 15, 75, 25, "OK");
		btnMacroEditOK->callback(cbMacroEditOK);
		
		btnMacroEditCancel = new Fl_Button(600, 15, 75, 25, "Cancel");
		btnMacroEditCancel->callback(cbMacroEditCancel);
		
		macrotext = new Fl_Input(5, 60, 450, 165, "Text");
		macrotext->type(4);
		macrotext->align(FL_ALIGN_TOP_LEFT);
		
		btnInsertMacro = new Fl_Button(460, 114, 25, 25, "@<<");
		btnInsertMacro->callback(cbInsertMacro);
		
		macroDefs = new Fl_Hold_Browser(490, 60, 200, 165);
		macroDefs->column_widths(widths);
		loadBrowser(macroDefs);
		o->end();
	}
	return w;
}

void editMacro(int n)
{
	if (!MacroEditDialog) MacroEditDialog = make_macroeditor();
	macrotext->value(macros.text[n].c_str());
	labeltext->value(macros.name[n].c_str());
	iMacro = n;
	MacroEditDialog->xclass(FLDIGI_NAME);
	MacroEditDialog->show();
}

