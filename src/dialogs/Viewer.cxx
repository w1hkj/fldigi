// ----------------------------------------------------------------------------
//
// Viewer.cxx -- PSK browser
//
// Copyright (C) 2008-2009
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

#include <config.h>

#include <string>

#include <FL/Enumerations.H>

#include "Viewer.h"
#include "trx.h"
#include "main.h"
#include "viewpsk.h"
#include "configuration.h"
#include "status.h"
#include "waterfall.h"
#include "fl_digi.h"
#include "re.h"
#include "gettext.h"
#include "flmisc.h"
#include "flinput2.h"
#include "flslider2.h"
#include "spot.h"
#include "icons.h"

using namespace std;


static string bwsrfreq, bwsrline[MAXCHANNELS];

static int freq, brwsFreq[MAXCHANNELS];

static long long rfc;
static bool usb;

static string dkred, dkblue, dkgreen, bkselect, white, bkgnd[2];

static int cols[] = {100, 0}, cwidth, cheight, labelwidth[VIEWER_LABEL_NTYPES],
	sbarwidth = 16, border = 4;

static fre_t seek_re("", REG_EXTENDED | REG_ICASE | REG_NOSUB);

static string freqformat(int i)
{
	static char szLine[32];
	string fline;

	if (pskviewer)
		freq = pskviewer->get_freq(progdefaults.VIEWERchannels - 1 - i);
	else
		freq = progdefaults.VIEWERstart + 100 * (progdefaults.VIEWERchannels - 1 - i);
	brwsFreq[i] = freq;

	switch (progdefaults.VIEWERlabeltype) {
	case VIEWER_LABEL_AF:
		snprintf(szLine, sizeof(szLine), "% 5d", freq);
		break;
	case VIEWER_LABEL_RF:
		snprintf(szLine, sizeof(szLine), "%10.3f", (rfc + (usb ? freq : -freq)) / 1000.0f);
		break;
	case VIEWER_LABEL_CH:
		snprintf(szLine, sizeof(szLine), "%3d", progdefaults.VIEWERchannels - i);
		break;
	default:
		break;
	}

	fline = "@f";
	fline += bkselect;
	fline += white;
	fline += szLine;
  	fline += '\t';
	fline += bkgnd[i % 2];
	fline += "@f";

	return fline;
}

void pskBrowser::resize(int x, int y, int w, int h)
{
	unsigned int nuchars = (w - cols[0] - (sbarwidth + border)) / cwidth;
	string bline;
	if (nuchars < progStatus.VIEWERnchars) {
		Fl_Hold_Browser::clear();
		for (int i = 0; i < progdefaults.VIEWERchannels; i++) {
			size_t len = bwsrline[i].length();
			if (len > nuchars)
				bwsrline[i] = bwsrline[i].substr(len - nuchars);
			bline = freqformat(i);
			if (seek_re.match(bwsrline[i].c_str(), REG_NOTBOL | REG_NOTEOL))
				bline.append(dkred);
			else if (!progdefaults.myCall.empty() &&
					strcasestr(bwsrline[i].c_str(), progdefaults.myCall.c_str()))
				bline.append(dkgreen);
			bline.append("@.").append(bwsrline[i]);
			Fl_Hold_Browser::add(bline.c_str());
		}
	}
	progStatus.VIEWERnchars = nuchars;
	progStatus.VIEWERxpos = dlgViewer->x();
	progStatus.VIEWERypos = dlgViewer->y();
	Fl_Hold_Browser::resize(x,y,w,h);
}

Fl_Double_Window *dlgViewer = 0;

static Fl_Button *btnCloseViewer;
static Fl_Button *btnClearViewer;
static pskBrowser *brwsViewer;
static Fl_Input2  *inpSeek;
static Fl_Value_Slider2 *sldrViewerSquelch;

static void make_colors()
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

static void evalcwidth()
{
	fl_font(FL_COURIER, FL_NORMAL_SIZE);
	cwidth = (int)fl_width("W");
	cheight = fl_height();
	labelwidth[VIEWER_LABEL_AF] = 6 * cwidth;
	labelwidth[VIEWER_LABEL_RF] = 11 * cwidth;
	labelwidth[VIEWER_LABEL_CH] = 4 * cwidth;
}

static void cb_btnCloseViewer(Fl_Button*, void*) {
	progStatus.VIEWERxpos = dlgViewer->x();
	progStatus.VIEWERypos = dlgViewer->y();
	dlgViewer->hide();
}

static void ClearViewer()
{
  	brwsViewer->clear();
  	usb = wf->USB();
  	rfc = wf->rfcarrier();
	string bline; 
	if (pskviewer)
		pskviewer->init();
  	for (int i = 0; i < progdefaults.VIEWERchannels; i++) {
  		if (pskviewer) freq = pskviewer->get_freq(progdefaults.VIEWERchannels - 1 - i);
  		else 		   freq = progdefaults.VIEWERstart + 100 * (progdefaults.VIEWERchannels - 1 - i);

		bline = freqformat(i);
  		bwsrline[i].clear();
  		brwsViewer->add(bline.c_str());
  	}
	cols[0] = labelwidth[progdefaults.VIEWERlabeltype];
  	brwsViewer->column_widths(cols);
}

void initViewer()
{
	if (!pskviewer) return;
	if (!dlgViewer) return;
	pskviewer->init();
	ClearViewer();
	dlgViewer->size(dlgViewer->w(), cheight * progdefaults.VIEWERchannels + 50 + border);
}

// i in [1, progdefaults.VIEWERchannels]
static void set_freq(int i, int freq)
{
	string new_line;

	if (freq == 0) // reset
		freq = progdefaults.VIEWERstart + 100 * (progdefaults.VIEWERchannels - i);

	pskviewer->set_freq(progdefaults.VIEWERchannels - i, freq);
	new_line.append(freqformat(i - 1)).append("@.").append(bwsrline[i - 1]);
	brwsViewer->replace(i, new_line.c_str());
}

static void cb_btnClearViewer(Fl_Button*, void*) {
	if (Fl::event_button() == FL_LEFT_MOUSE)
		ClearViewer();
	else
		for (int i = 1; i <= progdefaults.VIEWERchannels; i++)
			set_freq(i, 0);
}

static void cb_brwsViewer(Fl_Hold_Browser*, void*) {
	int sel = brwsViewer->value();
	if (sel == 0 || sel > progdefaults.VIEWERchannels)
		return;

	switch (Fl::event_button()) {
	case FL_LEFT_MOUSE:
		ReceiveText->addchr('\n', FTextBase::ALTR);
		ReceiveText->addstr(bwsrline[sel - 1].c_str(), FTextBase::ALTR);
		active_modem->set_freq(brwsFreq[sel - 1]);
		break;
	case FL_MIDDLE_MOUSE: // copy from modem
		set_freq(sel, active_modem->get_freq());
		break;
	case FL_RIGHT_MOUSE: // reset
		set_freq(sel, 0);
		break;
	default:
		break;
	}
}

static void cb_Seek(Fl_Input *, void *)
{
	static Fl_Color seek_color[2] = { FL_FOREGROUND_COLOR,
					  adjust_color(FL_RED, FL_BACKGROUND2_COLOR) }; // invalid RE
	seek_re.recompile(*inpSeek->value() ? inpSeek->value() : "[invalid");
	if (inpSeek->textcolor() != seek_color[!seek_re]) {
		inpSeek->textcolor(seek_color[!seek_re]);
		inpSeek->redraw();
	}
	progStatus.browser_search = inpSeek->value();
}

static void cb_Squelch(Fl_Slider *, void *)
{
	progdefaults.VIEWERsquelch = sldrViewerSquelch->value();
	progdefaults.changed = true;
}

Fl_Double_Window* createViewer(void)
{
	make_colors();
	evalcwidth();
	cols[0] = labelwidth[progdefaults.VIEWERlabeltype];

	int viewerwidth = (progStatus.VIEWERnchars * cwidth) + cols[0] + sbarwidth + border;
	int viewerheight = 50 + cheight * progdefaults.VIEWERchannels + border;
	int pad = 2;

	Fl_Double_Window* w = new Fl_Double_Window(progStatus.VIEWERxpos, progStatus.VIEWERypos,
				 viewerwidth + 2 * border, viewerheight + 2 * border,
				 _("PSK Browser"));

	Fl_Group* g = new Fl_Group(border, border, 200, 20);
	// search field
	const char* label = _("Find: ");
	fl_font(FL_HELVETICA, FL_NORMAL_SIZE);
	inpSeek = new Fl_Input2(2 * border + fl_width(label), border, 200, g->h(), label);
	inpSeek->callback((Fl_Callback*)cb_Seek);
	inpSeek->when(FL_WHEN_CHANGED);
	inpSeek->textfont(FL_COURIER);
	inpSeek->value(progStatus.browser_search.c_str());
    	inpSeek->do_callback();
	g->resizable(0);
	g->end();

	// browser
    	brwsViewer = new pskBrowser(border, inpSeek->y() + inpSeek->h() + pad,
				    viewerwidth, viewerheight - 2 * g->h() - border);
    	brwsViewer->callback((Fl_Callback*)cb_brwsViewer);
    	brwsViewer->column_widths(cols);

	g = new Fl_Group(border, brwsViewer->y() + brwsViewer->h() + pad, viewerwidth, 20);
	// close button
	btnCloseViewer = new Fl_Button(g->w() + border - 65, g->y(), 65, g->h(),
				       make_icon_label(_("Close"), close_icon));
	btnCloseViewer->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	set_icon_label(btnCloseViewer);
	btnCloseViewer->callback((Fl_Callback*)cb_btnCloseViewer);

	// clear button
	btnClearViewer = new Fl_Button(btnCloseViewer->x() - btnCloseViewer->w() - pad,
				       btnCloseViewer->y(), btnCloseViewer->w(), btnCloseViewer->h(),
				       make_icon_label(_("Clear"), edit_clear_icon));
	btnClearViewer->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	set_icon_label(btnClearViewer);
	btnClearViewer->callback((Fl_Callback*)cb_btnClearViewer);
	btnClearViewer->tooltip(_("Left click to clear text\nRight click to reset frequencies"));

	// squelch
	sldrViewerSquelch = new Fl_Value_Slider2(border, g->y(),
						 btnClearViewer->x() - border - pad, g->h());
	sldrViewerSquelch->align(FL_ALIGN_RIGHT);
	sldrViewerSquelch->tooltip(_("Set Viewer Squelch"));
	sldrViewerSquelch->type(FL_HOR_NICE_SLIDER);
	sldrViewerSquelch->range(0.0, 100.0);
	sldrViewerSquelch->step(1.0);
	sldrViewerSquelch->value(progdefaults.VIEWERsquelch);
	sldrViewerSquelch->callback((Fl_Callback*)cb_Squelch);

	g->resizable(sldrViewerSquelch);
	g->end();

	w->end();
	w->callback((Fl_Callback*)cb_btnCloseViewer);
	w->resizable(brwsViewer);
	w->xclass(PACKAGE_NAME);

	return w;
}

void openViewer()
{
	if (!dlgViewer) {
		dlgViewer = createViewer();
		ClearViewer();
	}
	dlgViewer->show();
	dlgViewer->redraw();
}

void viewer_redraw()
{
	if (!dlgViewer) return;
  	usb = wf->USB();
  	rfc = wf->rfcarrier();

  	for (int i = 0; i < progdefaults.VIEWERchannels; i++)
  		brwsViewer->text(i + 1, freqformat(i).c_str() );
	cols[0] = labelwidth[progdefaults.VIEWERlabeltype];
	brwsViewer->column_widths(cols);
}

void viewaddchr(int ch, int freq, char c)
{
	if (!dlgViewer) return;

	if (progStatus.spot_recv)
		spot_recv(c, ch, freq);

	if (rfc != wf->rfcarrier() || usb != wf->USB()) viewer_redraw();
		
	static string nuline;
	string bline;

	int index = progdefaults.VIEWERchannels - 1 - ch;
	if (progdefaults.VIEWERmarquee) {
		if (bwsrline[index].length() > progStatus.VIEWERnchars ) {
			bwsrline[index].erase(0,1);
		}
		if (c >= ' ' && c <= '~') {
			bwsrline[index] += c;
		} else {
			bwsrline[index] += ' ';
		}
	} else {
		if (c >= ' ' && c <= '~') {
			bwsrline[index] += c;
		} else {
			bwsrline[index] += ' ';
		}
		if (bwsrline[index].length() > progStatus.VIEWERnchars)
			bwsrline[index].clear();
	}
	nuline = freqformat(index);
	if (seek_re.match(bwsrline[index].c_str(), REG_NOTBOL | REG_NOTEOL))
		nuline.append(dkred);
	else if (!progdefaults.myCall.empty() &&
		 strcasestr(bwsrline[index].c_str(), progdefaults.myCall.c_str()))
		nuline.append(dkgreen);
	nuline.append("@.").append(bwsrline[index]);
	brwsViewer->text(1 + index, nuline.c_str());
	brwsViewer->redraw();
}

void viewclearchannel(int ch)
{
	int index = progdefaults.VIEWERchannels - 1 - ch;
	string nuline = freqformat(index);
	bwsrline[index] = "";
	brwsViewer->text( 1 + index, nuline.c_str());
	brwsViewer->redraw();
}

void viewer_paste_freq(int freq)
{
	if (!dlgViewer)
		return;

	int sel = 1, n = brwsViewer->size();

	for (int i = 0; i < n; i++) {
		if (brwsViewer->selected(i)) {
			brwsViewer->select(i, false);
			sel = i;
			break;
		}
	}

	set_freq(sel, freq);
	brwsViewer->select(WCLAMP(sel+1, 1, n));
}
