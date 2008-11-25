#ifndef macroedit_h
#define macroedit_h

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Hold_Browser.H>
#include "flinput2.h"

extern void loadBrowser(Fl_Widget *widget);

extern Fl_Button		*btnMacroEditOK;
extern Fl_Button		*btnMacroEditCancel;
extern Fl_Hold_Browser	*macroDefs;
extern Fl_Button		*btnInsertMacro;
extern Fl_Input2		*macrotext;
extern Fl_Input2		*labeltext;

extern Fl_Double_Window* make_macroeditor();

extern void editMacro(int);

#endif
