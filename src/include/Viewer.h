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

#ifndef Viewer_h
#define Viewer_h

#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/fl_show_colormap.H>

#include <string>

#include <config.h>
#include "viewpsk.h"
#include "psk_browser.h"
#include "flslider2.h"
#include "flinput2.h"

extern Fl_Double_Window *dlgViewer;
extern Fl_Value_Slider2 *sldrViewerSquelch;
extern Fl_Double_Window* createViewer();
extern Fl_Input2  *viewer_inp_seek;
extern pskBrowser *brwsViewer;

extern void openViewer();
extern void viewaddchr(int ch, int freq, char c, int md);
extern void viewerswap(int, int);
extern void initViewer();
extern void viewclearchannel(int ch);
extern void viewer_paste_freq(int freq);
extern void viewer_redraw();

#endif
