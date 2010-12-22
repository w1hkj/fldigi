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

#include <FL/Enumerations.H>

#include "config.h"

#include "Viewer.h"
#include "trx.h"
#include "main.h"
#include "configuration.h"
#include "confdialog.h"
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

#include "psk_browser.h"

extern pskBrowser *mainViewer;

using namespace std;

//static int freq; 

static long long rfc;
static bool usb;

fre_t seek_re("CQ", REG_EXTENDED | REG_ICASE | REG_NOSUB);

Fl_Double_Window *dlgViewer = 0;

static Fl_Button *btnCloseViewer;
static Fl_Button *btnClearViewer;

pskBrowser *brwsViewer;

static string brwsViewer_freqformat(int i)
{
	long freq;
	if (pskviewer)
		freq = pskviewer->get_freq(progdefaults.VIEWERchannels - 1 - i);
	else
		freq = progdefaults.VIEWERstart + 100 * (progdefaults.VIEWERchannels - 1 - i);
	return brwsViewer->freqformat(i, freq);
}

static string mainViewer_freqformat(int i)
{
	if (!mainViewer) return "";
	long freq;
	if (pskviewer)
		freq = pskviewer->get_freq(progdefaults.VIEWERchannels - 1 - i);
	else
		freq = progdefaults.VIEWERstart + 100 * (progdefaults.VIEWERchannels - 1 - i);
	return mainViewer->freqformat(i, freq);
}

static void cb_btnCloseViewer(Fl_Button*, void*) {
	progStatus.VIEWERxpos = dlgViewer->x();
	progStatus.VIEWERypos = dlgViewer->y();
	dlgViewer->hide();
}

static void ClearViewer()
{
	usb = wf->USB();
	rfc = wf->rfcarrier();
	if (pskviewer)
		pskviewer->init();
	if (brwsViewer) {
		brwsViewer->usb = usb;
		brwsViewer->rfc = rfc;
		brwsViewer->clear();
	}
	if (mainViewer) {
		mainViewer->usb = usb;
		mainViewer->rfc = rfc;
		mainViewer->clear();
	}
}

void initViewer()
{
	if (!pskviewer) return;
	usb = wf->USB();
	rfc = wf->rfcarrier();
	if (mainViewer) {
		mainViewer->usb = usb;
		mainViewer->rfc = rfc;
		mainViewer->setfont(progdefaults.ViewerFontnbr, progdefaults.ViewerFontsize);
		mainViewer->clear();
	}
	if (dlgViewer) {
		brwsViewer->usb = usb;
		brwsViewer->rfc = rfc;
		brwsViewer->setfont(progdefaults.ViewerFontnbr, progdefaults.ViewerFontsize);
		dlgViewer->size(dlgViewer->w(), dlgViewer->h() - brwsViewer->h() +
			pskBrowser::cheight * progdefaults.VIEWERchannels + 4);
		brwsViewer->clear();
	}
}

// i in [1, progdefaults.VIEWERchannels]
static void set_freq(int i, int freq)
{
	if (freq == 0) // reset
		freq = progdefaults.VIEWERstart + 100 * (progdefaults.VIEWERchannels - i);

	pskviewer->set_freq(progdefaults.VIEWERchannels - i, freq);
	if (brwsViewer) brwsViewer->set_freq(i, freq);
	if (mainViewer) mainViewer->set_freq(i, freq);
}

static void cb_btnClearViewer(Fl_Button*, void*) {
	if (Fl::event_button() == FL_LEFT_MOUSE)
		brwsViewer->clear();
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
		ReceiveText->addstr(brwsViewer->line(sel).c_str(), FTextBase::ALTR);
		active_modem->set_freq(brwsViewer->freq(sel));
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

Fl_Double_Window* createViewer(void)
{
	fl_font(progdefaults.ViewerFontnbr, progdefaults.ViewerFontsize);
	pskBrowser::cwidth = (int)fl_width("W");
	pskBrowser::cheight = fl_height();

	progStatus.VIEWERnchars = progStatus.VIEWERnchars > 30 ? progStatus.VIEWERnchars : 30;
	int viewerwidth = 
		(progStatus.VIEWERnchars * pskBrowser::cwidth) + pskBrowser::sbarwidth;
	int viewerheight = pskBrowser::cheight * progdefaults.VIEWERchannels;
	int pad = BWSR_BORDER / 2;
	Fl_Double_Window* w = new Fl_Double_Window(progStatus.VIEWERxpos, progStatus.VIEWERypos,
						   viewerwidth + 2 * BWSR_BORDER,
						   viewerheight + 2 * BWSR_BORDER + pad + 20,
						   _("PSK Browser"));
	brwsViewer = new pskBrowser(BWSR_BORDER, BWSR_BORDER, viewerwidth, viewerheight);
	brwsViewer->callback((Fl_Callback*)cb_brwsViewer);
	brwsViewer->setfont(progdefaults.ViewerFontnbr, progdefaults.ViewerFontsize);
	brwsViewer->seek_re = &seek_re;

	Fl_Group *g = new Fl_Group(BWSR_BORDER, brwsViewer->y() + brwsViewer->h() + pad, viewerwidth, 20);
	// close button
	btnCloseViewer = new Fl_Button(BWSR_BORDER, g->y(), 65, g->h(),
				       make_icon_label(_("Close"), close_icon));
	btnCloseViewer->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	set_icon_label(btnCloseViewer);
	btnCloseViewer->callback((Fl_Callback*)cb_btnCloseViewer);

	// clear button
	btnClearViewer = new Fl_Button(btnCloseViewer->x() + 65 + pad,
				       btnCloseViewer->y(), 65, btnCloseViewer->h(),
				       make_icon_label(_("Clear"), edit_clear_icon));
	btnClearViewer->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	set_icon_label(btnClearViewer);
	btnClearViewer->callback((Fl_Callback*)cb_btnClearViewer);
	btnClearViewer->tooltip(_("Left click to clear text\nRight click to reset frequencies"));

	g->resizable(0); // do not resize the buttons
	g->end();

	w->end();
	w->callback((Fl_Callback*)cb_btnCloseViewer);
	w->resizable(brwsViewer);
	w->size_range(
		(30 * pskBrowser::cwidth) + pskBrowser::sbarwidth + 2 * BWSR_BORDER, 
		5 * pskBrowser::cheight + 20 + 2 * BWSR_BORDER + pad);
	w->xclass(PACKAGE_NAME);

	return w;
}

void openViewer()
{
	if (!dlgViewer) {
		dlgViewer = createViewer();
		initViewer();
	}
	ClearViewer();
	dlgViewer->show();
	dlgViewer->redraw();
}

void viewer_redraw()
{
	usb = wf->USB();
	rfc = wf->rfcarrier();

	if (mainViewer) {
		mainViewer->usb = usb;
		mainViewer->rfc = rfc;
		for (int i = 0; i < progdefaults.VIEWERchannels; i++)
			mainViewer->text(i + 1, mainViewer_freqformat(i).c_str() );
	}
	if (dlgViewer) {
		brwsViewer->usb = usb;
		brwsViewer->rfc = rfc;
		for (int i = 0; i < progdefaults.VIEWERchannels; i++)
			brwsViewer->text(i + 1, brwsViewer_freqformat(i).c_str() );
	}
}

void viewaddchr(int ch, int freq, char c, int md)
{
	if (mainViewer) {
		if (mainViewer->rfc != wf->rfcarrier() || mainViewer->usb != wf->USB()) {
			mainViewer->rfc = wf->rfcarrier();
			mainViewer->usb = wf->USB();
			mainViewer->redraw();
		}
		mainViewer->addchr(ch, freq, c, md);
	}

	if (progStatus.spot_recv)
		spot_recv(c, ch, freq, md);

	if (!dlgViewer) return;
	if (rfc != wf->rfcarrier() || usb != wf->USB()) viewer_redraw();
	brwsViewer->addchr(ch, freq, c, md);
}

void viewclearchannel(int ch)
{
	int index = progdefaults.VIEWERchannels - 1 - ch;
	string nuline;
	if (mainViewer) {
		nuline = mainViewer_freqformat(index);
		mainViewer->clearline(index);
		mainViewer->text( 1 + index, nuline.c_str());
		mainViewer->redraw();
	}
	if (dlgViewer) {
		nuline = brwsViewer_freqformat(index);
		brwsViewer->clearline(index);
		brwsViewer->text( 1 + index, nuline.c_str());
		brwsViewer->redraw();
	}
}

void viewer_paste_freq(int freq)
{
	if (dlgViewer) {
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
	if (mainViewer) {
		int sel = 1, n = mainViewer->size();
		for (int i = 0; i < n; i++) {
			if (mainViewer->selected(i)) {
				mainViewer->select(i, false);
				sel = i;
				break;
			}
		}
		set_freq(sel, freq);
		mainViewer->select(WCLAMP(sel+1, 1, n));
	}

}
