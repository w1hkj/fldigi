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

#ifndef PSK_BROWSER_H
#define PSK_BROWSER_H

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Hold_Browser.H>

#include <string>

#include <config.h>
#include "viewpsk.h"
#include "globals.h"
#include "charsetdistiller.h"

#include "re.h"

#define BWSR_BORDER 2

class pskBrowser : public Fl_Hold_Browser{
private:
	static std::string hilite_color_1;
	static std::string hilite_color_2;
	static std::string white;
	static std::string bkgnd[];

	std::string fline;
	std::string nuline;

	std::string bwsrline[MAXCHANNELS];
	int bwsrfreq[MAXCHANNELS];
	struct {bool regex_alert; bool mycall_alert;} alerted[MAXCHANNELS];

	CharsetDistiller *cdistiller;

	int labelwidth[VIEWER_LABEL_NTYPES];

	Fl_Font fnt;
	Fl_Color HiLite_1;
	Fl_Color HiLite_2;
	Fl_Color BkSelect;
	Fl_Color Backgnd1;
	Fl_Color Backgnd2;

	int siz;
	int cols[2];
	char szLine[32];

public:
	static int cwidth;
	static int cheight;
	static int sbarwidth;

	long long rfc;
	bool usb;
	fre_t *seek_re;

public:
	pskBrowser(int x, int y, int w, int h, const char *l = "");
	~pskBrowser();
	void makecolors();
	void evalcwidth();
	void setfont(Fl_Font font, int sz) { fnt = font; siz = sz; evalcwidth();}
	void HighLight_1(Fl_Color clr) { HiLite_1 = clr; }
	Fl_Color HighLight_1() { return HiLite_1; }
	void HighLight_2(Fl_Color clr) { HiLite_2 = clr; }
	Fl_Color HighLight_2() { return HiLite_2; }
	void SelectColor(Fl_Color clr) { BkSelect = clr; }
	Fl_Color SelectColor() { return BkSelect; }
	void Background1(Fl_Color clr) { Backgnd1 = clr; }
	Fl_Color Background1() { return Backgnd1; }
	void Background2(Fl_Color clr) { Backgnd2 = clr; }
	Fl_Color Background2() { return Backgnd2; }
	void columns(int a) { cols[0] = a; cols[1] = 0; column_widths(cols); }
	void resize(int x, int y, int w, int h);
	void addchr(int ch, int freq, unsigned char c, int md, bool alert = false);
	void set_input_encoding(int encoding_id);
	std::string freqformat (int i);
	void set_freq(int i, int freq);
	void clearline(int i) { bwsrline[i] = ""; }
	std::string line(int i) { return (i < 1 ? "" : i > MAXCHANNELS ? "" : bwsrline[i - 1]); }
	int  freq(int i);
	void clear();
	void clearch(int n, int freq);
	void swap(int, int);
};

#endif
