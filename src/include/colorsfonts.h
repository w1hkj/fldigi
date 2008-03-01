// ----------------------------------------------------------------------------
//
//	colorsfonts.cxx
//
// Copyright (C) 2006
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.
//
// fldigi is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// ----------------------------------------------------------------------------

#ifndef colorsfonts_h
#define colorsfonts_h

#include "fl_digi.h"
#include "configuration.h"
#include "font_browser.h"

#include <FL/Fl.H>
#include <FL/Fl_Color_Chooser.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Multiline_Output.H>

void selectColorsFonts();

extern void cbRxFontBrowser(Font_Browser*, void* v);
extern void cbTxFontBrowser(Font_Browser*, void* v);
extern void cb_ColorFontOK();
extern void make_colorsfonts();

extern Fl_Check_Button *btnUseGroupColors;
extern Fl_Button *btnGroup1;
extern Fl_Button *btnGroup2;
extern Fl_Button *btnGroup3;
extern Fl_Button *btnFkeyTextColor;
extern Fl_Button *btnFkeyDefaults;
extern Fl_Multiline_Output *RxText;
extern Fl_Multiline_Output *TxText;
extern Fl_Button *btnRxColor;
extern Fl_Button *btnRxFont;
extern Fl_Button *btnTxColor;
extern Fl_Button *btnTxFont;
extern Fl_Button *btnTextDefaults;
extern Fl_Button *btnClrFntOK;

#endif
