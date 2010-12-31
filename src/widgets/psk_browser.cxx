// ----------------------------------------------------------------------------
//
// PSK browser widget
//
// Copyright (C) 2008-2010
//		David Freese, W1HKJ
// Copyright (C) 2008-2010
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

#include <FL/Enumerations.H>

#include "config.h"

#include "psk_browser.h"

#include "configuration.h"
#include "confdialog.h"
#include "status.h"
#include "waterfall.h"
#include "fl_digi.h"
#include "gettext.h"
#include "flmisc.h"
#include "flinput2.h"
#include "flslider2.h"
#include "spot.h"
#include "icons.h"

using namespace std;

string pskBrowser::dkred;
string pskBrowser::dkblue;
string pskBrowser::dkgreen;
string pskBrowser::bkselect;
string pskBrowser::white;
string pskBrowser::bkgnd[2];

int pskBrowser::cwidth = 5;
int pskBrowser::cheight = 12;
int pskBrowser::sbarwidth = 16;

static fre_t def_seek_re("CQ", REG_EXTENDED | REG_ICASE | REG_NOSUB);

pskBrowser::pskBrowser(int x, int y, int w, int h, const char *l)
	:Fl_Hold_Browser(x,y,w,h,l) 
{
	fnt = FL_COURIER;
	siz = 12;
	rfc = 0LL;
	usb = true;
	seek_re = &def_seek_re;
	cols[0] = 80; cols[1] = 0;
	evalcwidth();
	makecolors();

	string bline;
	for (int i = 0; i < MAXCHANNELS; i++) {
		bwsrline[i] = "";
		bline = freqformat(i, NULLFREQ);
		if ( i < progdefaults.VIEWERchannels) add(bline.c_str());
	}
	szLine[0] = 0;
	nchars = (w - cols[0] - (sbarwidth + 2*BWSR_BORDER)) / cwidth;
	nchars = nchars < 1 ? 1 : nchars;
}

void pskBrowser::evalcwidth()
{
	fl_font(fnt, siz);
	textfont(fnt);
	textsize(siz);
	cwidth = (int)fl_width("W");
	cheight = fl_height();
	labelwidth[VIEWER_LABEL_OFF] = cwidth;
	labelwidth[VIEWER_LABEL_AF] = 6 * cwidth;
	labelwidth[VIEWER_LABEL_RF] = 10 * cwidth;
	labelwidth[VIEWER_LABEL_CH] = 4 * cwidth;
	columns(labelwidth[progdefaults.VIEWERlabeltype]);
}

string pskBrowser::freqformat(int i, int freq)
{
	if (progdefaults.VIEWERlabeltype == VIEWER_LABEL_OFF) {
		fline.clear();
		fline.append(" \t");
		fline += bkgnd[i % 2];
		return fline;
	}
	fline.clear();
	bwsrfreq[i] = freq;
	if (freq == 1e6) strcpy(szLine, " ");
	else
		switch (progdefaults.VIEWERlabeltype) {
		case VIEWER_LABEL_AF:
			snprintf(szLine, sizeof(szLine), "% 5d", freq);
			break;
		case VIEWER_LABEL_RF:
			snprintf(szLine, sizeof(szLine), "%9.2f", (rfc + (usb ? freq : -freq)) / 1000.0f);
			break;
		case VIEWER_LABEL_CH:
			snprintf(szLine, sizeof(szLine), "%3d", i + 1);//progdefaults.VIEWERchannels - i);
			break;
		default:
			break;
		}

	fline += bkselect;
	fline += white;
	fline += szLine;
  	fline += '\t';
	fline += bkgnd[i % 2];

	return fline;
}

void pskBrowser::swap(int i, int j)
{
	string tempstr = bwsrline[j];
	bwsrline[j] = bwsrline[i];
	bwsrline[i] = tempstr;
	int f = bwsrfreq[j];
	bwsrfreq[j] = bwsrfreq[i];
	bwsrfreq[i] = f;

	tempstr = freqformat(i, bwsrfreq[i]);
	tempstr.append("@.").append(bwsrline[i]);
	text(i+1, tempstr.c_str());

	tempstr = freqformat(j, bwsrfreq[j]);
	tempstr.append("@.").append(bwsrline[j]);
	text(j+1, tempstr.c_str());

	redraw();

}

void pskBrowser::resize(int x, int y, int w, int h)
{
	size_t nuchars = (w - cols[0] - (sbarwidth + 2 * BWSR_BORDER)) / cwidth;
	nuchars = nuchars < 1 ? 1 : nuchars; 
	string bline;
	if (nuchars < nchars) {
		Fl_Hold_Browser::clear();
		for (int i = 0; i < progdefaults.VIEWERchannels; i++) {
			size_t len = bwsrline[i].length();
			if (len > nuchars)
				bwsrline[i] = bwsrline[i].substr(len - nuchars);
			bline = freqformat(i, bwsrfreq[i]);
			if (seek_re) {
				if (seek_re->match(bwsrline[i].c_str(), REG_NOTBOL | REG_NOTEOL))
					bline.append(dkred);
			} else if (!progdefaults.myCall.empty() &&
					strcasestr(bwsrline[i].c_str(), progdefaults.myCall.c_str()))
				bline.append(dkgreen);
			bline.append("@.").append(bwsrline[i]);
			Fl_Hold_Browser::add(bline.c_str());
		}
	}
	nchars = nuchars;
	evalcwidth();
	Fl_Hold_Browser::resize(x,y,w,h);
}

void pskBrowser::makecolors()
{
	char tempstr[20];

	snprintf(tempstr, sizeof(tempstr), "@b@C%d",
		 adjust_color(fl_color_cube(128 * (FL_NUM_RED - 1) / 255,
					    0 * (FL_NUM_GREEN - 1) / 255,
					    0 * (FL_NUM_BLUE - 1) / 255),
			      FL_BACKGROUND2_COLOR)); // dark red
	dkred = tempstr;

	snprintf(tempstr, sizeof(tempstr), "@b@C%d",
		 adjust_color(fl_color_cube(0 * (FL_NUM_RED - 1) / 255,
					    128 * (FL_NUM_GREEN - 1) / 255,
					    0 * (FL_NUM_BLUE - 1) / 255),
			      FL_BACKGROUND2_COLOR)); // dark green
	dkgreen = tempstr;

	snprintf(tempstr, sizeof(tempstr), "@b@C%d",
		 adjust_color(fl_color_cube(0 * (FL_NUM_RED - 1) / 255,
					    0 * (FL_NUM_GREEN - 1) / 255,
					    128 * (FL_NUM_BLUE - 1) / 255),
			      FL_BACKGROUND2_COLOR)); // dark blue
	dkblue = tempstr;

	snprintf(tempstr, sizeof(tempstr), "@C%d", FL_FOREGROUND_COLOR); // foreground
	white = tempstr;

	snprintf(tempstr, sizeof(tempstr), "@B%d",
		 adjust_color(FL_BACKGROUND2_COLOR, FL_FOREGROUND_COLOR)); // default selection color bkgnd
	bkselect = tempstr;

	snprintf(tempstr, sizeof(tempstr), "@B%d", FL_BACKGROUND2_COLOR); // background for odd rows
	bkgnd[0] = tempstr;

	Fl_Color bg2 = fl_color_average(FL_BACKGROUND2_COLOR, FL_BLACK, .9);
	if (bg2 == FL_BLACK)
		bg2 = fl_color_average(FL_BACKGROUND2_COLOR, FL_WHITE, .9);
	snprintf(tempstr, sizeof(tempstr), "@B%d", adjust_color(bg2, FL_FOREGROUND_COLOR)); // even rows
	bkgnd[1] = tempstr;
}

void pskBrowser::addchr(int ch, int freq, char c, int md)
{
	static string nuline;
	string bline;
	size_t chars = (w() - cols[0] - (sbarwidth + 2 * BWSR_BORDER)) / cwidth;
	chars = chars < 1 ? 1 : chars; 

	int index = ch;//progdefaults.VIEWERchannels - 1 - ch;
	if (index < 0 || index > (MAXCHANNELS - 1))//index >= MAXCHANNELS)
		return;
	bwsrfreq[index] = freq;
	if (c >= ' ' && c <= '~') {
		if (progdefaults.VIEWERmarquee) {
			if (bwsrline[index].length() >= chars )
				bwsrline[index].erase(0,1);
		} else {
			if (bwsrline[index].length() >= chars)
				bwsrline[index].clear();
		}
		bwsrline[index] += c;
	}
	nuline = freqformat(index, bwsrfreq[index]);
	if (seek_re) {
		if (seek_re->match(bwsrline[index].c_str(), REG_NOTBOL | REG_NOTEOL))
			nuline.append(dkred);
	} else if (!progdefaults.myCall.empty() &&
		 strcasestr(bwsrline[index].c_str(), progdefaults.myCall.c_str()))
		nuline.append(dkgreen);
	nuline.append("@.").append(bwsrline[index]);
	text(1 + index, nuline.c_str());
	redraw();
}

void pskBrowser::set_freq(int i, int freq)
{
	string new_line;

	bwsrfreq[i-1] = freq;
	new_line.append(freqformat(i - 1, freq)).append("@.").append(bwsrline[i - 1]);
	replace(i, new_line.c_str());
}

void pskBrowser::clear()
{
	long freq;
	Fl_Hold_Browser::clear();
	for (int i = 0; i < progdefaults.VIEWERchannels; i++) {
		freq = 1e6;
		bwsrline[i] = "";
		bwsrfreq[i] = freq;
		fline = freqformat(i, freq);
		add(fline.c_str());
	}
	szLine[0] = 0;
	deselect();
	redraw();
}

void pskBrowser::clearch(int n, int freq)
{
	clearline(n-1);
	set_freq(n, freq);
}

