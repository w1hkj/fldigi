#ifndef MACROEDIT_H
#define MACROEDIT_H

#include <FL/Fl_Widget.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/Fl_Pixmap.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Box.H>

#include "flinput2.h"

extern void loadBrowser(Fl_Widget *widget);

extern Fl_Button		*btnMacroEditOK;
extern Fl_Button		*btnMacroEditCancel;
extern Fl_Hold_Browser	*macroDefs;
extern Fl_Button		*btnInsertMacro;
extern Fl_Input2		*macrotext;
extern Fl_Input2		*labeltext;

extern Fl_Double_Window *MacroEditDialog;

extern Fl_Double_Window* make_macroeditor();

enum { MACRO_EDIT_BUTTON, MACRO_EDIT_INPUT };
extern void editMacro(int b, int t = MACRO_EDIT_BUTTON, Fl_Input* in = 0);

#endif
