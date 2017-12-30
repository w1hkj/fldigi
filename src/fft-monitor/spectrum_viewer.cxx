// ----------------------------------------------------------------------------
// spectrum_viewer.cxx  --  spectrum dialog
//
// Copyright (C) 2017
//		Dave Freese, W1HKJ
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

#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Counter.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Box.H>

#include <iostream>

#include "configuration.h"
#include "status.h"

#include "spectrum.h"
#include "spectrum_viewer.h"
#include "fft-monitor.h"
#include "gettext.h"
#include "modem.h"
#include "trx.h"

Fl_Double_Window	*spectrum_viewer = (Fl_Double_Window *)0;

spectrum			*fftscope = (spectrum *)0;

Fl_Group			*fftmon_mnuFrame = (Fl_Group *)0;
Fl_Group			*group1 = (Fl_Group *)0;
Fl_Group			*group2 = (Fl_Group *)0;
Fl_Counter			*fftviewer_scans = (Fl_Counter *)0;
Fl_Counter			*fftviewer_range = (Fl_Counter *)0;
Fl_Counter			*fftviewer_maxdb = (Fl_Counter *)0;
Fl_Counter			*fftviewer_fcenter = (Fl_Counter *)0;
Fl_Counter			*fftviewer_frng = (Fl_Counter *)0;
Fl_Button			*fftviewer_reset = (Fl_Button *)0;
Fl_Button			*fftviewer_goto = (Fl_Button *)0;
Fl_Group			*g1 = (Fl_Group *)0;
Fl_Group			*g3 = (Fl_Group *)0;
Fl_Group			*g2 = (Fl_Group *)0;

Fl_Menu_Bar 		*fftmon_mnu_bar = (Fl_Menu_Bar *)0;
Fl_Output			*values = (Fl_Output *)0;
Fl_Output			*db_diffs = (Fl_Output *)0;
Fl_Output			*f_diffs = (Fl_Output *)0;
Fl_Button			*pause_button = (Fl_Button *)0;
Fl_Box				*annunciator = (Fl_Box *)0;

fftmon *fft_modem = (fftmon *)0;

void cb_fftviewer_scans(Fl_Counter *w, void *d)
{
	progdefaults.fftviewer_scans = w->value();
	fft_modem->restart();
}

void cb_fftviewer_range(Fl_Counter *w, void *d)
{
	progdefaults.fftviewer_range = w->value();
}

void cb_fftviewer_maxdb(Fl_Counter *w, void *d)
{
	progdefaults.fftviewer_maxdb = w->value();
}

void check_frng(int fr)
{
	int fc = progdefaults.fftviewer_fcenter;
	if (fc - fr/2 < 0) fr = 2*fc;
	if (fc + fr/2 > 4000) fr = 2*(4000 - fc);
	progdefaults.fftviewer_frng = fr;
	fftviewer_frng->value(fr);
	fftviewer_frng->redraw();
}

void cb_fftviewer_fcenter(Fl_Counter *w, void *d)
{
	progdefaults.fftviewer_fcenter = w->value();
	int fr = progdefaults.fftviewer_frng;
	check_frng(fr);
}

void cb_fftviewer_frng(Fl_Counter *w, void *d)
{
	progdefaults.fftviewer_frng = w->value();
	int fr = progdefaults.fftviewer_frng;
	check_frng(fr);
}

void cb_fftviewer_reset(Fl_Button *b, void *d)
{
	progdefaults.fftviewer_fcenter = 2000;
	progdefaults.fftviewer_frng = 4000;
	fftviewer_fcenter->value(2000);
	fftviewer_frng->value(4000);
	fftviewer_fcenter->redraw();
	fftviewer_frng->redraw();

	if (progdefaults.wf_spectrum_dbvals) {
		progdefaults.fftviewer_range = progdefaults.wfAmpSpan;
		progdefaults.fftviewer_maxdb = progdefaults.wfRefLevel;
		fftviewer_maxdb->value(progdefaults.fftviewer_maxdb);
		fftviewer_maxdb->redraw();
		fftviewer_range->value(progdefaults.fftviewer_range);
		fftviewer_range->redraw();
	}
}

void cb_pause_button(Fl_Button *b, void *d)
{
	fftscope->paused( !fftscope->paused() );
}

void cb_fftviewer_goto(Fl_Button *b, void *d)
{
	progdefaults.fftviewer_fcenter = active_modem->get_freq();
	int fr = progdefaults.fftviewer_frng;
	if (progdefaults.wf_spectrum_modem_scale)
		fr = progdefaults.wf_spectrum_scale_factor * active_modem->get_bandwidth();
	check_frng(fr);
	fftviewer_fcenter->value(progdefaults.fftviewer_fcenter);
	fftviewer_fcenter->redraw();

	if (progdefaults.wf_spectrum_dbvals) {
		progdefaults.fftviewer_range = progdefaults.wfAmpSpan;
		progdefaults.fftviewer_maxdb = progdefaults.wfRefLevel;
		fftviewer_maxdb->value(progdefaults.fftviewer_maxdb);
		fftviewer_maxdb->redraw();
		fftviewer_range->value(progdefaults.fftviewer_range);
		fftviewer_range->redraw();
	}
}

void cb_spectrum_viewer(Fl_Double_Window *w, void *)
{
	progStatus.svX = spectrum_viewer->x();
	progStatus.svY = spectrum_viewer->y();
	progStatus.svW = spectrum_viewer->w();
	progStatus.svH = spectrum_viewer->h();
	spectrum_viewer->hide();
	return;
}

void cb_mnu_fftmon_close(Fl_Menu_*, void*)
{
	cb_spectrum_viewer(0,0);
}

extern bool b_write_fftfile;
void cb_mnu_fftmon_csv(Fl_Menu_*, void*)
{
	if (!b_write_fftfile) b_write_fftfile = true;
}

void cb_mnu_x_graticule(Fl_Menu_*, void*)
{
	progStatus.x_graticule = true;
	progStatus.y_graticule = false;
	progStatus.xy_graticule = false;
	fftscope->x_graticule(true);
	fftscope->y_graticule(false);
}

void cb_mnu_y_graticule(Fl_Menu_*, void*)
{
	progStatus.y_graticule = true;
	progStatus.x_graticule = false;
	progStatus.xy_graticule = false;
	fftscope->x_graticule(false);
	fftscope->y_graticule(true);
}

void cb_mnu_xy_graticules(Fl_Menu_*, void*)
{
	progStatus.xy_graticule = true;
	progStatus.x_graticule = false;
	progStatus.y_graticule = false;
	fftscope->x_graticule(true);
	fftscope->y_graticule(true);
}

Fl_Menu_Item fftmon_menu[] = {
{_("&Dialog"), 0,  0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
  {_("&Graticule"), 0,  0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
    { _("X graticule"), 0, (Fl_Callback*)cb_mnu_x_graticule, 0, FL_MENU_RADIO, FL_NORMAL_LABEL, 0, 14, 0},
    { _("Y graticule"), 0, (Fl_Callback*)cb_mnu_y_graticule, 0, FL_MENU_RADIO, FL_NORMAL_LABEL, 0, 14, 0},
    { _("X/Y graticules"), 0, (Fl_Callback*)cb_mnu_xy_graticules, 0, FL_MENU_RADIO, FL_NORMAL_LABEL, 0, 14, 0},
  { 0 },
  {_("&Save to CSV"),  0, (Fl_Callback*)cb_mnu_fftmon_csv,  0, FL_MENU_DIVIDER, FL_NORMAL_LABEL, 0, 14, 0},
  {_("&Close"), 0,  (Fl_Callback*)cb_mnu_fftmon_close, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ 0 },
{ 0 }
};

#define VALWIDTH 120
#define DIFFSWIDTH 80
#define PAUSEWIDTH 60
#define ANNUNWIDTH 140
#define WIDTHS (VALWIDTH + 2*DIFFSWIDTH + PAUSEWIDTH + ANNUNWIDTH)
#define STATUS_COLOR 0xfdf5e600

void create_spectrum_viewer()
{
	if (spectrum_viewer) return;

	spectrum_viewer = new Fl_Double_Window(0, 0, 550, 400, "Spectrum Scope");
	spectrum_viewer->xclass(PACKAGE_NAME);

	fftmon_mnuFrame = new Fl_Group(0,0, spectrum_viewer->w(), 20);
		fftmon_mnu_bar = new Fl_Menu_Bar(0, 0, fftmon_mnuFrame->w() - WIDTHS, 20);
		fftmon_mnu_bar->menu(fftmon_menu);

		pause_button = new Fl_Button(
				fftmon_mnu_bar->x() + fftmon_mnu_bar->w(), 0,
				PAUSEWIDTH, 20, "Running");
		pause_button->callback((Fl_Callback*)cb_pause_button);

		annunciator = new Fl_Box(
				pause_button->x() + pause_button->w(), 0,
				ANNUNWIDTH, 20, "");
		annunciator->box(FL_DOWN_BOX);
		annunciator->color(STATUS_COLOR);

		values = new Fl_Output(
				annunciator->x() + annunciator->w(), 0,
				VALWIDTH, 20, "");
		values->value("");
		values->color(STATUS_COLOR);

		db_diffs = new Fl_Output(
				values->x() + values->w(), 0,
				DIFFSWIDTH, 20, "");
		db_diffs->value("");
		db_diffs->color(STATUS_COLOR);

		f_diffs = new Fl_Output(
				db_diffs->x() + db_diffs->w(), 0,
				DIFFSWIDTH, 20, "");
		f_diffs->value("");
		f_diffs->color(STATUS_COLOR);

		fftmon_mnuFrame->resizable(fftmon_mnu_bar);
	fftmon_mnuFrame->end();

	group1 = new Fl_Group(0, 20, 550, 330);
		fftscope = new spectrum (group1->x(), group1->y(), group1->w(), group1->h());
		group1->resizable(fftscope);
	group1->end();
	group1->show();

	group2 = new Fl_Group(0, 350, 550, 50);

		g1 = new Fl_Group(0, 350, 548, 50, "");

			fftviewer_scans = new Fl_Counter(5, 360, 100, 22, "# scans");
			fftviewer_scans->minimum(1);
			fftviewer_scans->maximum(500);
			fftviewer_scans->step(1);
			fftviewer_scans->lstep(10.0);
			fftviewer_scans->value(50);
			fftviewer_scans->callback((Fl_Callback*)cb_fftviewer_scans);
			fftviewer_scans->value(progdefaults.fftviewer_scans);
			fftviewer_scans->tooltip(_("each display point an average of past N fft values"));

			fftviewer_range = new Fl_Counter(
				5 + fftviewer_scans->x() + fftviewer_scans->w(), 360,
				80, 22, "dB Range");
			fftviewer_range->type(1);
			fftviewer_range->minimum(20);
			fftviewer_range->maximum(120);
			fftviewer_range->step(10);
			fftviewer_range->value(60);
			fftviewer_range->callback((Fl_Callback*)cb_fftviewer_range);
			fftviewer_range->value(progdefaults.fftviewer_range);
			fftviewer_range->tooltip(_("range of dB scale"));

			fftviewer_maxdb = new Fl_Counter(
				5 + fftviewer_range->x() + fftviewer_range->w(), 360,
				80, 22, "upper dB");
			fftviewer_maxdb->type(1);
			fftviewer_maxdb->minimum(-60);
			fftviewer_maxdb->maximum(0);
			fftviewer_maxdb->step(10);
			fftviewer_maxdb->value(0);
			fftviewer_maxdb->callback((Fl_Callback*)cb_fftviewer_maxdb);
			fftviewer_maxdb->value(progdefaults.fftviewer_maxdb);
			fftviewer_maxdb->tooltip(_("offset Db scale"));

			int xp = 5 + fftviewer_maxdb->x() + fftviewer_maxdb->w();
			g2 = new Fl_Group(
				xp, 352,
				g1->w() - 2 - xp, 46, "");
			g2->box(FL_ENGRAVED_FRAME);

			fftviewer_fcenter = new Fl_Counter(
				5 + g2->x(), 358,
				100, 22, "F-center");
			fftviewer_fcenter->minimum(0);
			fftviewer_fcenter->maximum(3950);
			fftviewer_fcenter->step(1);
			fftviewer_fcenter->lstep(10);
			fftviewer_fcenter->value(0);
			fftviewer_fcenter->callback((Fl_Callback*)cb_fftviewer_fcenter);
			fftviewer_fcenter->value(progdefaults.fftviewer_fcenter);
			fftviewer_fcenter->tooltip(_("center frequency"));

			fftviewer_frng = new Fl_Counter(
				5 + fftviewer_fcenter->x() + fftviewer_fcenter->w(), 358,
				110, 22, "F-range");
			fftviewer_frng->minimum(100);
			fftviewer_frng->maximum(4000);
			fftviewer_frng->step(20);
			fftviewer_frng->lstep(100);
			fftviewer_frng->value(4000);
			fftviewer_frng->callback((Fl_Callback*)cb_fftviewer_frng);
			fftviewer_frng->value(progdefaults.fftviewer_frng);
			fftviewer_frng->tooltip(_("frequency range"));

			fftviewer_reset = new Fl_Button(
				5 + fftviewer_frng->x() + fftviewer_frng->w(), 354,
				38, 20, "Reset");
			fftviewer_reset->callback((Fl_Callback*)cb_fftviewer_reset);
			fftviewer_reset->tooltip(_("Center = 2000, Range = 4000"));

			fftviewer_goto = new Fl_Button(
				5 + fftviewer_frng->x() + fftviewer_frng->w(), 376,
				38, 20, "Goto");
			fftviewer_goto->callback((Fl_Callback*)cb_fftviewer_goto);
			fftviewer_goto->tooltip(_("Center - wf track\nRange = 10 * mode-bw"));

			g2->end();

		g1->end();

		g3 = new Fl_Group(spectrum_viewer->w() - 2, 350, 2, 50, "");
		g3->end();

	group2->end();
	group2->resizable(g3);

	spectrum_viewer->resizable(group1);
	spectrum_viewer->size_range(550, 400);
	spectrum_viewer->end();
	spectrum_viewer->callback((Fl_Callback *)cb_spectrum_viewer);

	if (progStatus.x_graticule) {
		fftmon_menu[2].setonly();
		fftscope->x_graticule(true);
		fftscope->y_graticule(false);
	} else if (progStatus.y_graticule) {
		 fftmon_menu[3].setonly();
		fftscope->x_graticule(false);
		fftscope->y_graticule(true);
	} else {
		fftmon_menu[4].setonly();
		fftscope->x_graticule(true);
		fftscope->y_graticule(true);
	}
}

//======================================================================

void open_spectrum_viewer()
{
	if (!spectrum_viewer) create_spectrum_viewer();
	if (!fft_modem) {
		progdefaults.fftviewer_fcenter = active_modem->get_freq();
		fft_modem = new fftmon();
		active_modem->set_freq(progdefaults.fftviewer_fcenter);
	} else
		progdefaults.fftviewer_fcenter = active_modem->get_freq();

	int fr = progdefaults.fftviewer_frng;
	if (progdefaults.wf_spectrum_modem_scale)
		fr = progdefaults.wf_spectrum_scale_factor * active_modem->get_bandwidth();
	check_frng(fr);
	fftviewer_fcenter->value(progdefaults.fftviewer_fcenter);
	fftviewer_fcenter->redraw();

	if (progdefaults.wf_spectrum_dbvals) {
		progdefaults.fftviewer_range = progdefaults.wfAmpSpan;
		progdefaults.fftviewer_maxdb = progdefaults.wfRefLevel;
		fftviewer_maxdb->value(progdefaults.fftviewer_maxdb);
		fftviewer_maxdb->redraw();
		fftviewer_range->value(progdefaults.fftviewer_range);
		fftviewer_range->redraw();
	}

	spectrum_viewer->show();

	spectrum_viewer->redraw();
	spectrum_viewer->resize(progStatus.svX, progStatus.svY, progStatus.svW, progStatus.svH);

}

void close_spectrum_viewer()
{
	if (spectrum_viewer) {
		spectrum_viewer->hide();
		delete spectrum_viewer;
		spectrum_viewer = 0;
	}
	if (fft_modem) {
		delete fft_modem;
		fft_modem = 0;
	}
}

void recenter_spectrum_viewer()
{
	if (!spectrum_viewer) return;
	if (!spectrum_viewer->visible()) return;
	if (!progdefaults.wf_spectrum_center) return;
	if (!fft_modem) return;

	progdefaults.fftviewer_fcenter = active_modem->get_freq();
	int fr = progdefaults.fftviewer_frng;
	if (progdefaults.wf_spectrum_modem_scale)
		fr = progdefaults.wf_spectrum_scale_factor * active_modem->get_bandwidth();
	check_frng(fr);

	fftviewer_fcenter->value(progdefaults.fftviewer_fcenter);
	fftviewer_fcenter->redraw();

	if (progdefaults.wf_spectrum_dbvals) {
		progdefaults.fftviewer_range = progdefaults.wfAmpSpan;
		progdefaults.fftviewer_maxdb = progdefaults.wfRefLevel;
		fftviewer_maxdb->value(progdefaults.fftviewer_maxdb);
		fftviewer_maxdb->redraw();
		fftviewer_range->value(progdefaults.fftviewer_range);
		fftviewer_range->redraw();
	}

	spectrum_viewer->redraw();
}
