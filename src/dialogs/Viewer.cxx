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
#include "spot.h"
#include "icons.h"

#include "psk_browser.h"

extern pskBrowser *mainViewer;

using namespace std;

//
// External viewer dialog
// 

Fl_Double_Window *dlgViewer = 0;
static Fl_Button *btnCloseViewer;
static Fl_Button *btnClearViewer;

Fl_Value_Slider2 *sldrViewerSquelch;

pskBrowser *brwsViewer;

static long long rfc;
static bool usb;

fre_t seek_re("CQ", REG_EXTENDED | REG_ICASE | REG_NOSUB);

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
		sldrViewerSquelch->value(progdefaults.VIEWERsquelch);
		brwsViewer->setfont(progdefaults.ViewerFontnbr, progdefaults.ViewerFontsize);
		dlgViewer->size(dlgViewer->w(), dlgViewer->h() - brwsViewer->h() +
			pskBrowser::cheight * progdefaults.VIEWERchannels + 4);
		brwsViewer->clear();
	}
}

void viewaddchr(int ch, int freq, char c, int md)
{
	if (mainViewer->rfc != wf->rfcarrier() || mainViewer->usb != wf->USB()) {
		mainViewer->rfc = wf->rfcarrier();
		mainViewer->usb = wf->USB();
		mainViewer->redraw();
	}
	mainViewer->addchr(ch, freq, c, md);

	if (dlgViewer) {
		if (brwsViewer->rfc != wf->rfcarrier() || brwsViewer->usb != wf->USB()) {
			brwsViewer->rfc = wf->rfcarrier();
			brwsViewer->usb = wf->USB();
			brwsViewer->redraw();
		}
		brwsViewer->addchr(ch, freq, c, md);
	}

	if (progStatus.spot_recv && freq != NULLFREQ)
		spot_recv(c, ch, freq, md);
}

void viewclearchannel(int ch)
{
	mainViewer->clearch(ch + 1, NULLFREQ);
	if (dlgViewer)
		brwsViewer->clearch(ch + 1, NULLFREQ);
}

void viewerswap(int i, int j)
{
	mainViewer->swap(i,j);
	if (dlgViewer)
		brwsViewer->swap(i,j);
}

void viewer_redraw()
{
	usb = wf->USB();
	rfc = wf->rfcarrier();

	mainViewer->usb = usb;
	mainViewer->rfc = rfc;
	mainViewer->resize(mainViewer->x(), mainViewer->y(), mainViewer->w(), mainViewer->h());

	if (dlgViewer) {
		brwsViewer->usb = usb;
		brwsViewer->rfc = rfc;
		brwsViewer->resize(
			brwsViewer->x(), brwsViewer->y(), brwsViewer->w(), brwsViewer->h());
		dlgViewer->redraw();
	}
}

static void cb_btnCloseViewer(Fl_Button*, void*) {
	progStatus.VIEWERxpos = dlgViewer->x();
	progStatus.VIEWERypos = dlgViewer->y();
	dlgViewer->hide();
}

static void cb_btnClearViewer(Fl_Button*, void*) {
	brwsViewer->clear();
}

static void cb_brwsViewer(Fl_Hold_Browser*, void*) {
	if (!pskviewer) return;
	int sel = brwsViewer->value();
	if (sel == 0 || sel > progdefaults.VIEWERchannels)
		return;

	switch (Fl::event_button()) {
	case FL_LEFT_MOUSE:
		if (brwsViewer->freq(sel) != NULLFREQ) {
			ReceiveText->addchr('\n', FTextBase::ALTR);
			ReceiveText->addstr(brwsViewer->line(sel).c_str(), FTextBase::ALTR);
			active_modem->set_freq(brwsViewer->freq(sel));
			active_modem->set_sigsearch(SIGSEARCH);
		}
		break;
	case FL_MIDDLE_MOUSE: // copy from modem
//		set_freq(sel, active_modem->get_freq());
		break;
	case FL_RIGHT_MOUSE: // reset
		pskviewer->clearch(sel-1);
		brwsViewer->deselect();
	default:
		break;
	}
}

static void cb_Squelch(Fl_Slider *, void *)
{
	progdefaults.VIEWERsquelch = sldrViewerSquelch->value();
	mvsquelch->value(progdefaults.VIEWERsquelch);
	progdefaults.changed = true;
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
	btnCloseViewer = new Fl_Button(g->w() + BWSR_BORDER - 65, g->y(), 65, g->h(),
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
	sldrViewerSquelch = new Fl_Value_Slider2(BWSR_BORDER, g->y(),
						 btnClearViewer->x() - BWSR_BORDER - pad, g->h());
	sldrViewerSquelch->align(FL_ALIGN_RIGHT);
	sldrViewerSquelch->tooltip(_("Set Viewer Squelch"));
	sldrViewerSquelch->type(FL_HOR_NICE_SLIDER);
	sldrViewerSquelch->range(-6.0, 20.0);
	sldrViewerSquelch->step(0.5);
	sldrViewerSquelch->value(progdefaults.VIEWERsquelch);
	sldrViewerSquelch->callback((Fl_Callback*)cb_Squelch);
	sldrViewerSquelch->color((Fl_Color)246);
	sldrViewerSquelch->selection_color((Fl_Color)4);

	g->resizable(sldrViewerSquelch);
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
	initViewer();
	dlgViewer->show();
	dlgViewer->redraw();
}

void viewer_paste_freq(int freq)
{
	int ch = (freq - progdefaults.LowFreqCutoff) / 100;

	mainViewer->select(WCLAMP(0, progdefaults.VIEWERchannels, ch));
	if (dlgViewer)
		brwsViewer->select(WCLAMP(0, progdefaults.VIEWERchannels, ch));
}


