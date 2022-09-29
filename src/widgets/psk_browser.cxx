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

#include "audio_alert.h"

#include <string>

std::string pskBrowser::hilite_color_1;
std::string pskBrowser::hilite_color_2;
std::string pskBrowser::white;
std::string pskBrowser::bkgnd[2];

int pskBrowser::cwidth = 5;
int pskBrowser::cheight = 12;
int pskBrowser::sbarwidth = 16;

pskBrowser::pskBrowser(int x, int y, int w, int h, const char *l)
	:Fl_Hold_Browser(x,y,w,h,l)
{
	fnt = FL_HELVETICA;
	siz = 12;
	rfc = 0LL;
	usb = true;
	seek_re = NULL;
	cols[0] = 80; cols[1] = 0;
	evalcwidth();

	HiLite_1 = FL_RED;
	HiLite_2 = FL_GREEN;
	BkSelect = FL_BLUE;
	Backgnd1 = (Fl_Color)55;
	Backgnd2 = (Fl_Color)53;
	makecolors();
	cdistiller = reinterpret_cast<CharsetDistiller*>(operator new(MAXCHANNELS*sizeof(CharsetDistiller)));

	std::string bline;
	for (int i = 0; i < MAXCHANNELS; i++) {
		alerted[i].regex_alert = alerted[i].mycall_alert = false;
		bwsrline[i] = " ";
		bwsrfreq[i] = NULLFREQ;
		bline = freqformat(i);
		if ( i < progdefaults.VIEWERchannels) add(bline.c_str());
		new(&cdistiller[i]) CharsetDistiller;
	}
}

pskBrowser::~pskBrowser()
{
	for (int i = MAXCHANNELS-1; i >= 0; i--)
		cdistiller[i].~CharsetDistiller();
	
	operator delete(cdistiller);
}

void pskBrowser::evalcwidth()
{
	fl_font(fnt, siz);
	textfont(fnt);
	textsize(siz);
	cwidth = (int)fl_width("8");
	if (cwidth <= 0) cwidth = 5;
	cheight = fl_height();
	labelwidth[VIEWER_LABEL_OFF] = 1;
	labelwidth[VIEWER_LABEL_AF] = 5*cwidth;
	labelwidth[VIEWER_LABEL_RF] = 10*cwidth;
	labelwidth[VIEWER_LABEL_CH] = 3*cwidth;
	columns(labelwidth[progdefaults.VIEWERlabeltype]);
}

std::string pskBrowser::freqformat(int i) // 0 < i < channels
{
	szLine[0] = 0;
	int freq = bwsrfreq[i];
	switch (progdefaults.VIEWERlabeltype) {
		case VIEWER_LABEL_AF:
			if (freq != NULLFREQ)
				snprintf(szLine, sizeof(szLine), "%4d", freq);
			else
				snprintf(szLine, sizeof(szLine), "    ");
			break;
		case VIEWER_LABEL_RF:
			if (freq != NULLFREQ)
				snprintf(szLine, sizeof(szLine), "%8.2f", (rfc + (usb ? freq : -freq)) / 1000.0f);
			else
				snprintf(szLine, sizeof(szLine), "    ");
			break;
		case VIEWER_LABEL_CH:
			snprintf(szLine, sizeof(szLine), "%2d", i + 1);
			break;
		default:
			snprintf(szLine, sizeof(szLine), "    ");
			break;
	}
	fline = white;
	fline.append("@r").append(szLine).append("\t").append(bkgnd[i%2]);

	return fline;
}

void pskBrowser::swap(int i, int j)
{
	std::string tempstr = bwsrline[j];
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

static size_t case_find(std::string &haystack, std::string &needle)
{
	std::string Uhaystack = haystack;
	std::string Uneedle = needle;
	for (size_t i = 0; i < Uhaystack.length(); i++ ) Uhaystack[i] = toupper(Uhaystack[i]);
	for (size_t i = 0; i < Uneedle.length(); i++ ) Uneedle[i] = toupper(Uneedle[i]);
	return Uhaystack.find(Uneedle);
}

void pskBrowser::resize(int x, int y, int w, int h)
{
	if (w) {
		Fl_Hold_Browser::resize(x,y,w,h);
		evalcwidth();
		std::string bline;
		Fl_Hold_Browser::clear();
		for (int i = 0, j = 0; i < progdefaults.VIEWERchannels; i++) {
			if (progdefaults.VIEWERascend) j = progdefaults.VIEWERchannels - 1 - i;
			else j = i;
			bwsrline[j].clear();
			bline = freqformat(j);
			if (seek_re  && seek_re->match(bwsrline[j].c_str(), REG_NOTBOL | REG_NOTEOL))
				bline.append(hilite_color_1);
			else if (	!progdefaults.myCall.empty() && 
						case_find (bwsrline[j], progdefaults.myCall ) != std::string::npos)
				bline.append(hilite_color_2);
			Fl_Hold_Browser::add(bline.c_str());
		}
	}
}

void pskBrowser::makecolors()
{
	char tempstr[20];
       
	snprintf(tempstr, sizeof(tempstr), "@C%u", HiLite_1);
	hilite_color_1 = tempstr;

	snprintf(tempstr, sizeof(tempstr), "@C%u", HiLite_2);
	hilite_color_2 = tempstr;

	snprintf(tempstr, sizeof(tempstr), "@C%u", FL_FOREGROUND_COLOR); // foreground
	white = tempstr;

	selection_color(BkSelect);

	snprintf(tempstr, sizeof(tempstr), "@B%u", Backgnd1); // background for odd rows
	bkgnd[0] = tempstr;

	snprintf(tempstr, sizeof(tempstr), "@B%u", Backgnd2); // background for even rows
	bkgnd[1] = tempstr;
}

void pskBrowser::addchr(int ch, int freq, unsigned char c, int md, bool signal_alert)
{
	if (ch < 0 || ch >= MAXCHANNELS)
		return;

	if (c == '\n') c = ' ';
	if (c < ' ') return;

	bwsrfreq[ch] = freq;

	if (bwsrline[ch].length() == 1 && bwsrline[ch][0] == ' ') {
		bwsrline[ch].clear();
	}
	
	cdistiller[ch].rx(c);

	if (cdistiller[ch].data_length() > 0) {
		bwsrline[ch] += cdistiller[ch].data();
		cdistiller[ch].clear();
	}

	fl_font(fnt, siz);
	size_t available = (w() - cols[0] - (sbarwidth + 2*BWSR_BORDER));
	size_t linewidth = fl_width(bwsrline[ch].c_str());
	if (linewidth > available) {
		if (progdefaults.VIEWERmarquee) {
			bwsrline[ch].erase(0, fl_utf8len1(bwsrline[ch][0]));
		} else {
			bwsrline[ch].clear();
		}
	}

	nuline = freqformat(ch);

	if (!bwsrline[ch].empty()) {
		if (seek_re  && seek_re->match(bwsrline[ch].c_str(), REG_NOTBOL | REG_NOTEOL)) {
			if ((trx_state == STATE_RX) && 
				(alerted[ch].regex_alert == false) &&
				signal_alert &&
				progdefaults.ENABLE_BWSR_REGEX_MATCH) {
				if (audio_alert) audio_alert->alert(progdefaults.BWSR_REGEX_MATCH);
				alerted[ch].regex_alert = true;
			}
			nuline.append(hilite_color_1);
		} else {
			alerted[ch].regex_alert = false;
		}
	} else {
		alerted[ch].regex_alert = false;
	}
	if (!progdefaults.myCall.empty() && 
			case_find (bwsrline[ch], progdefaults.myCall ) != std::string::npos) {
		nuline.append(hilite_color_2);
		if ((trx_state == STATE_RX) && 
			(alerted[ch].mycall_alert == false) &&
			signal_alert &&
			progdefaults.ENABLE_BWSR_MYCALL_MATCH) {
			if (audio_alert) audio_alert->alert(progdefaults.BWSR_MYCALL_MATCH);
			alerted[ch].mycall_alert = true;
		} 
	} else
		alerted[ch].mycall_alert = false;

	nuline.append("@.").append(bwsrline[ch]);

	if (progdefaults.VIEWERascend)
		text(progdefaults.VIEWERchannels - ch, nuline.c_str());
	else
		text(ch + 1, nuline.c_str());

	redraw();
}

void pskBrowser::set_freq(int i, int freq) // 0 < i < channels
{
	std::string new_line = "";

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

void pskBrowser::set_input_encoding(int encoding_id)
{
	for (int i = 0; i < MAXCHANNELS; i++)
		cdistiller[i].set_input_encoding(encoding_id);
}
