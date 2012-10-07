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
#include "Viewer.h"

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

pskBrowser::pskBrowser(int x, int y, int w, int h, const char *l)
	:Fl_Hold_Browser(x,y,w,h,l) 
{
	fnt = FL_COURIER;
	siz = 12;
	rfc = 0LL;
	usb = true;
	seek_re = NULL;
	cols[0] = 80; cols[1] = 0;
	evalcwidth();
	makecolors();

	string bline;
	for (int i = 0; i < MAXCHANNELS; i++) {
		bwsrline[i] = " ";
		bwsrfreq[i] = NULLFREQ;
		bline = freqformat(i);
		if ( i < progdefaults.VIEWERchannels) add(bline.c_str());
		linechars[i] = 0;
		firstUTF8[i] = 0;
	}
	nchars = (w - cols[0] - (sbarwidth + 2*BWSR_BORDER)) / cwidth;
	nchars = nchars < 1 ? 1 : nchars;

}

void pskBrowser::evalcwidth()
{
	fl_font(fnt, siz);
	textfont(fnt);
	textsize(siz);
	const char *szAF = " 9999";
	const char *szRF = " 999999.99";
	const char *szCH = " 99";
	cwidth = (int)fl_width("W");
	cheight = fl_height();
	labelwidth[VIEWER_LABEL_OFF] = 1;//cwidth;
	labelwidth[VIEWER_LABEL_AF] = (int)fl_width(szAF);
	labelwidth[VIEWER_LABEL_RF] = (int)fl_width(szRF);
	labelwidth[VIEWER_LABEL_CH] = (int)fl_width(szCH);
	columns(labelwidth[progdefaults.VIEWERlabeltype]);
}

string pskBrowser::freqformat(int i) // 0 < i < channels
{
	szLine[0] = 0;
	int freq = bwsrfreq[i];
	switch (progdefaults.VIEWERlabeltype) {
		case VIEWER_LABEL_AF:
			if (freq != NULLFREQ)
				snprintf(szLine, sizeof(szLine), "%4d", freq);
			else
				sprintf(szLine, "    ");
			break;
		case VIEWER_LABEL_RF:
			if (freq != NULLFREQ)
				snprintf(szLine, sizeof(szLine), "%8.2f", (rfc + (usb ? freq : -freq)) / 1000.0f);
			else
				sprintf(szLine, "    ");
			break;
		case VIEWER_LABEL_CH:
			snprintf(szLine, sizeof(szLine), "%2d", i + 1);
			break;
		default:
			sprintf(szLine, "    ");
			break;
	}
	fline = bkselect;
	fline.append(white).append("@r").append(szLine).append("\t").append(bkgnd[i%2]);

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

	tempstr = freqformat(i);
	tempstr.append(bwsrline[i]);
	text(i+1, tempstr.c_str());

	tempstr = freqformat(j);
	tempstr.append(bwsrline[j]);
	text(j+1, tempstr.c_str());

	redraw();

}

void pskBrowser::resize(int x, int y, int w, int h)
{
	if (w) {
		Fl_Hold_Browser::resize(x,y,w,h);
		evalcwidth();
		nchars = (w - cols[0] - (sbarwidth + 2 * BWSR_BORDER)) / cwidth;
		nchars = nchars < 1 ? 1 : nchars; 
		string bline;
		Fl_Hold_Browser::clear();
		for (int i = 0, j = 0; i < progdefaults.VIEWERchannels; i++) {
			if (progdefaults.VIEWERascend) j = progdefaults.VIEWERchannels - 1 - i;
			else j = i;
			bwsrline[j].clear();
			firstUTF8[j] = 0;
			linechars[j] = 0;
//		size_t len = bwsrline[j].length();
//		if (len > nchars)
//			bwsrline[j] = bwsrline[j].substr(len - nchars);
			bline = freqformat(j);
			if (seek_re) {
				if (seek_re->match(bwsrline[j].c_str(), REG_NOTBOL | REG_NOTEOL))
					bline.append(dkred);
			} else if (!progdefaults.myCall.empty() &&
					strcasestr(bwsrline[j].c_str(), progdefaults.myCall.c_str()))
				bline.append(dkgreen);
			Fl_Hold_Browser::add(bline.c_str());
		}
	}
}

void pskBrowser::makecolors()
{
	char tempstr[20];

	snprintf(tempstr, sizeof(tempstr), "@C%d",
		 adjust_color(fl_color_cube(128 * (FL_NUM_RED - 1) / 255,
					    0 * (FL_NUM_GREEN - 1) / 255,
					    0 * (FL_NUM_BLUE - 1) / 255),
			      FL_BACKGROUND2_COLOR)); // dark red
	dkred = tempstr;

	snprintf(tempstr, sizeof(tempstr), "@C%d",
		 adjust_color(fl_color_cube(0 * (FL_NUM_RED - 1) / 255,
					    128 * (FL_NUM_GREEN - 1) / 255,
					    0 * (FL_NUM_BLUE - 1) / 255),
			      FL_BACKGROUND2_COLOR)); // dark green
	dkgreen = tempstr;

	snprintf(tempstr, sizeof(tempstr), "@C%d",
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

void pskBrowser::addchr(int ch, int freq, unsigned char c, int md) // 0 < ch < channels
{
	if (ch < 0 || ch >= MAXCHANNELS)
		return;

	if (c == '\n') c = ' ';
	if (c < ' ') return;

	nchars = (w() - cols[0] - (sbarwidth + 2 * BWSR_BORDER)) / cwidth;
	nchars = nchars < 1 ? 1 : nchars; 

	bwsrfreq[ch] = freq;

	if (bwsrline[ch].length() == 1 && bwsrline[ch][0] == ' ') {
		bwsrline[ch].clear();
		linechars[ch] = 0;
		firstUTF8[ch] = 0;
	}

	if ((linechars[ch] > nchars) && (firstUTF8[ch] == 0)) {
		if (progdefaults.VIEWERmarquee) {
			while (linechars[ch] > nchars) {
				if ((bwsrline[ch][0] & 0x80) == 0x80) // UTF-8 character
					bwsrline[ch].erase(0,2);
				else
					bwsrline[ch].erase(0,1);
				linechars[ch]--;
				bwsrline[ch].erase(0,1);
				linechars[ch] -= 1;
			}
		} else {
			bwsrline[ch].clear();
			linechars[ch] = 0;
			firstUTF8[ch] = 0;
		}
	}

	if (firstUTF8[ch] != 0) {
		bwsrline[ch] += firstUTF8[ch];
		bwsrline[ch] += c;
		linechars[ch]++;
		firstUTF8[ch] = 0;
	} else if ((c & 0x80) == 0x80) {
			firstUTF8[ch] = c;
			return;
	} else {
		bwsrline[ch] += c;
		linechars[ch]++;
	}

	nuline = freqformat(ch);

	if (seek_re) {
		if (seek_re->match(bwsrline[ch].c_str(), REG_NOTBOL | REG_NOTEOL))
			nuline.append(dkred);
	} else if (!progdefaults.myCall.empty() &&
		 strcasestr(bwsrline[ch].c_str(), progdefaults.myCall.c_str()))
		nuline.append(dkgreen);

	nuline.append("@.").append(bwsrline[ch]);

	if (progdefaults.VIEWERascend)
		text(progdefaults.VIEWERchannels - ch, nuline.c_str());
	else
		text(ch + 1, nuline.c_str());

	redraw();
}

void pskBrowser::set_freq(int i, int freq) // 0 < i < channels
{
	string new_line = "";

	bwsrfreq[i] = freq;
	new_line.append(freqformat(i)).append(bwsrline[i]);
	if (progdefaults.VIEWERascend)
		replace(progdefaults.VIEWERchannels - i, new_line.c_str());
	else
		replace(i + 1, new_line.c_str());
}

void pskBrowser::clear()
{
	long freq;
	Fl_Hold_Browser::clear();
	for (int i = 0, j = 0; i < progdefaults.VIEWERchannels; i++) {
		if (progdefaults.VIEWERascend) j = progdefaults.VIEWERchannels - 1 - i;
		else j = i;
		freq = NULLFREQ;
		bwsrline[j] = " ";
		bwsrfreq[j] = freq;
		fline = freqformat(j);
		add((fline.append(bwsrline[j])).c_str());
	}
	deselect();
	redraw();
}

void pskBrowser::clearch(int n, int freq) // 0 < n < channels
{
	bwsrline[n] = " ";
	set_freq(n, freq);
	redraw();
}

int pskBrowser::freq(int i) { // 1 < i < progdefaults.VIEWERchannels
	if (progdefaults.VIEWERascend)
		return (
			i < 1 ? 0 : 
			i > progdefaults.VIEWERchannels ? 0 : bwsrfreq[progdefaults.VIEWERchannels - i]); 
	else
		return (i < 1 ? 0 : i > MAXCHANNELS ? 0 : bwsrfreq[i - 1]); 
}

