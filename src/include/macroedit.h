// ----------------------------------------------------------------------------
// Copyright (C) 2014
//              David Freese, W1HKJ
//
// This file is part of fldigi
//
// fldigi is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

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
extern void update_macro_button(int iMacro, const char *text, const char *name);
extern void update_macroedit_font();

#endif
