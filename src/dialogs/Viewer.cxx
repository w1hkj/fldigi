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
#include "spot.h"
#include "icons.h"

#include "psk_browser.h"
#include "view_rtty.h"

extern pskBrowser *mainViewer;

using namespace std;

//
// External viewer dialog
// 

Fl_Double_Window *dlgViewer = 0;
static Fl_Button *btnCloseViewer;
static Fl_Button *btnClearViewer;
Fl_Input2  *viewer_inp_seek;

Fl_Value_Slider2 *sldrViewerSquelch;

pskBrowser *brwsViewer;

static long long rfc;
static bool usb;

void initViewer()
{
//	if (!pskviewer) return;
	usb = wf->USB();
	rfc = wf->rfcarrier();
	if (mainViewer) {
		mainViewer->usb = usb;
		mainViewer->rfc = rfc;
		mainViewer->setfont(progdefaults.ViewerFontnbr, progdefaults.ViewerFontsize);
		mainViewer->clear();
	}
	if (brwsViewer) {
		brwsViewer->usb = usb;
		brwsViewer->rfc = rfc;
		sldrViewerSquelch->value(progStatus.VIEWERsquelch);
		brwsViewer->setfont(progdefaults.ViewerFontnbr, progdefaults.ViewerFontsize);
		dlgViewer->size(dlgViewer->w(), dlgViewer->h() - brwsViewer->h() +
			pskBrowser::cheight * progdefaults.VIEWERchannels + 4);
		brwsViewer->clear();
	}
	if (pskviewer) pskviewer->clear();
	if (rttyviewer) rttyviewer->clear();
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

void viewclearchannel(int ch) // 0 < ch < channels - 1
{
	if (mainViewer)
		mainViewer->clearch(ch, NULLFREQ);
	if (dlgViewer)
		brwsViewer->clearch(ch, NULLFREQ);
}

void viewerswap(int i, int j)
{
	if (mainViewer)
		mainViewer->swap(i,j);
	if (dlgViewer)
		brwsViewer->swap(i,j);
}

void viewer_redraw()
{
	usb = wf->USB();
	rfc = wf->rfcarrier();
	if (mainViewer) {
		mainViewer->usb = usb;
		mainViewer->rfc = rfc;
		mainViewer->resize(mainViewer->x(), mainViewer->y(), mainViewer->w(), mainViewer->h());
	}
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
	progStatus.VIEWERwidth = dlgViewer->w();
	progStatus.VIEWERheight = dlgViewer->h();
	dlgViewer->hide();
}

static void cb_btnClearViewer(Fl_Button*, void*) {
	brwsViewer->clear();
	if (mainViewer)
		mainViewer->clear();
	if (pskviewer) pskviewer->clear();
	if (rttyviewer) rttyviewer->clear();
}

static void cb_brwsViewer(Fl_Hold_Browser*, void*) {
	if (!pskviewer && !rttyviewer) return;
	int sel = brwsViewer->value();
	if (sel == 0 || sel > progdefaults.VIEWERchannels)
		return;

	switch (Fl::event_button()) {
	case FL_LEFT_MOUSE:
		if (brwsViewer->freq(sel) != NULLFREQ) {
			if (progdefaults.VIEWERhistory) {
				ReceiveText->addchr('\n', FTextBase::RECV);
				bHistory = true;
			} else {
				ReceiveText->addchr('\n', FTextBase::ALTR);
				ReceiveText->addstr(brwsViewer->line(sel).c_str(), FTextBase::ALTR);
			}
			active_modem->set_freq(brwsViewer->freq(sel));
			active_modem->set_sigsearch(SIGSEARCH);
			if (mainViewer)
				mainViewer->select(sel);
		} else
			brwsViewer->deselect();
		break;
	case FL_MIDDLE_MOUSE: // copy from modem
//		set_freq(sel, active_modem->get_freq());
		break;
	case FL_RIGHT_MOUSE: // reset
		{
		int ch = progdefaults.VIEWERascend ? progdefaults.VIEWERchannels - sel : sel - 1;
		if (pskviewer) pskviewer->clearch(ch);
		if (rttyviewer) rttyviewer->clearch(ch);
		brwsViewer->deselect();
		if (mainViewer) mainViewer->deselect();
		}
	default:
		break;
	}
}

static void cb_Squelch(Fl_Slider *, void *)
{
	progStatus.VIEWERsquelch = sldrViewerSquelch->value();
	if (mainViewer)
		mvsquelch->value(progStatus.VIEWERsquelch);
}

static void cb_Seek(Fl_Input *, void *)
{
	static Fl_Color seek_color[2] = { FL_FOREGROUND_COLOR,
					  adjust_color(FL_RED, FL_BACKGROUND2_COLOR) }; // invalid RE
	seek_re.recompile(*viewer_inp_seek->value() ? viewer_inp_seek->value() : "[invalid");
	if (viewer_inp_seek->textcolor() != seek_color[!seek_re]) {
		viewer_inp_seek->textcolor(seek_color[!seek_re]);
		viewer_inp_seek->redraw();
	}
	progStatus.browser_search = viewer_inp_seek->value();
	if (mainViewer) txtInpSeek->value(progStatus.browser_search.c_str());
}

Fl_Double_Window* createViewer(void)
{
	fl_font(progdefaults.ViewerFontnbr, progdefaults.ViewerFontsize);
	pskBrowser::cwidth = (int)fl_width("W");
	pskBrowser::cheight = fl_height();

	progStatus.VIEWERnchars = progStatus.VIEWERnchars > 30 ? progStatus.VIEWERnchars : 30;

	int pad = BWSR_BORDER / 2;

	int viewerwidth = progStatus.VIEWERwidth - 2*BWSR_BORDER;
//		(progStatus.VIEWERnchars * pskBrowser::cwidth) + pskBrowser::sbarwidth;
	int viewerheight = progStatus.VIEWERheight - 2 * BWSR_BORDER - pad - 20;
//		pskBrowser::cheight * progdefaults.VIEWERchannels;

	Fl_Double_Window* w = new Fl_Double_Window(progStatus.VIEWERxpos, progStatus.VIEWERypos,
						   viewerwidth + 2 * BWSR_BORDER,
						   viewerheight + 2 * BWSR_BORDER + pad + 20 + 20,
						   _("Signal Browser"));

	Fl_Group* gseek = new Fl_Group(BWSR_BORDER, BWSR_BORDER, viewerwidth, 20);
	// search field
	const char* label = _("Find: ");
	fl_font(FL_HELVETICA, FL_NORMAL_SIZE);
	viewer_inp_seek = new Fl_Input2(static_cast<int>(BWSR_BORDER + fl_width(label) + fl_width("X")), BWSR_BORDER, 200, gseek->h(), label);
	viewer_inp_seek->labelfont(FL_HELVETICA);
	viewer_inp_seek->callback((Fl_Callback*)cb_Seek);
	viewer_inp_seek->when(FL_WHEN_CHANGED);
	viewer_inp_seek->textfont(FL_COURIER);
	viewer_inp_seek->value(progStatus.browser_search.c_str());
	viewer_inp_seek->do_callback();
	gseek->resizable(0);
	gseek->end();

//	brwsViewer = new pskBrowser(BWSR_BORDER, BWSR_BORDER, viewerwidth, viewerheight);
	brwsViewer = new pskBrowser(BWSR_BORDER, viewer_inp_seek->y() + viewer_inp_seek->h(), viewerwidth, viewerheight);

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
	sldrViewerSquelch->value(progStatus.VIEWERsquelch);
	sldrViewerSquelch->callback((Fl_Callback*)cb_Squelch);
	sldrViewerSquelch->color( fl_rgb_color(
		progdefaults.bwsrSliderColor.R, 
		progdefaults.bwsrSliderColor.G,
		progdefaults.bwsrSliderColor.B));
	sldrViewerSquelch->selection_color( fl_rgb_color(
		progdefaults.bwsrSldrSelColor.R, 
		progdefaults.bwsrSldrSelColor.G,
		progdefaults.bwsrSldrSelColor.B));

	g->resizable(sldrViewerSquelch);
	g->end();

	w->end();
	w->callback((Fl_Callback*)cb_btnCloseViewer);
	w->resizable(brwsViewer);
	w->size_range(
		(30 * pskBrowser::cwidth) + pskBrowser::sbarwidth + 2 * BWSR_BORDER, 
		5 * pskBrowser::cheight + 20 + 2 * BWSR_BORDER + pad);
	w->xclass(PACKAGE_NAME);

	w->hide();
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
	if (pskviewer) {
		for (int i = 0; i < progdefaults.VIEWERchannels; i++) {
			int ftest = pskviewer->get_freq(i);
			if (ftest == NULLFREQ) continue;
			if (fabs(ftest - freq) <= 50) {
				if (progdefaults.VIEWERascend)
					i = (progdefaults.VIEWERchannels - i);
				else i++;
				if (mainViewer)
					mainViewer->select(i);
				if (brwsViewer)
					brwsViewer->select(i);
				return;
			}
		}
	}
	if (rttyviewer) {
		for (int i = 0; i < progdefaults.VIEWERchannels; i++) {
			int ftest = rttyviewer->get_freq(i);
			if (ftest == NULLFREQ) continue;
			if (fabs(ftest - freq) <= 50) {
				if (progdefaults.VIEWERascend)
					i = (progdefaults.VIEWERchannels - i);
				else i++;
				if (mainViewer)
					mainViewer->select(i);
				if (brwsViewer)
					brwsViewer->select(i);
				return;
			}
		}
	}
}


