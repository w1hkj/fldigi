// ----------------------------------------------------------------------------
//
// Viewer.cxx -- PSK browser
//
// Copyright (C) 2015
//		David Freese, W1HKJ
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
#include "qrunner.h"

using namespace std;

//
// External fsq monitor dialog
//

Fl_Double_Window  *fsqMonitor = 0;
static Fl_Button *btnCloseMonitor;
FTextRX           *fsq_monitor = 0;
FTextRX           *fsq_que = 0;

static void cb_btnCloseMonitor(Fl_Button*, void*) {
	progStatus.fsqMONITORxpos = fsqMonitor->x();
	progStatus.fsqMONITORypos = fsqMonitor->y();
	progStatus.fsqMONITORwidth = fsqMonitor->w();
	progStatus.fsqMONITORheight = fsqMonitor->h();
	btn_MONITOR->value(0);
	btn_MONITOR->redraw();
	fsqMonitor->hide();
}

Fl_Double_Window* create_fsqMonitor(void)
{
	Fl_Double_Window* w = new Fl_Double_Window(
							progStatus.fsqMONITORxpos, progStatus.fsqMONITORypos,
							progStatus.fsqMONITORwidth, progStatus.fsqMONITORheight,
							_("FSQ monitor"));

	Panel *monitor_panel = new Panel(
			2, 2,
			w->w() - 4, w->h() - 28);

		fsq_monitor = new FTextRX(
				monitor_panel->x(), monitor_panel->y(),
				monitor_panel->w(), 7*monitor_panel->h()/8);
			fsq_monitor->color(
			fl_rgb_color(
				0.98*progdefaults.RxColor.R,
				0.98*progdefaults.RxColor.G,
				0.98*progdefaults.RxColor.B),
				progdefaults.RxTxSelectcolor);
			fsq_monitor->setFont(progdefaults.RxFontnbr);
			fsq_monitor->setFontSize(progdefaults.RxFontsize);
			fsq_monitor->setFontColor(progdefaults.RxFontcolor, FTextBase::RECV);

		fsq_que = new FTextRX(
				fsq_monitor->x(), fsq_monitor->y() + fsq_monitor->h(),
				fsq_monitor->w(), monitor_panel->h() - fsq_monitor->h());
			fsq_que->color(
			fl_rgb_color(
				0.98*progdefaults.RxColor.R,
				0.98*progdefaults.RxColor.G,
				0.98*progdefaults.RxColor.B),
				progdefaults.RxTxSelectcolor);
			fsq_que->setFont(progdefaults.RxFontnbr);
			fsq_que->setFontSize(progdefaults.RxFontsize);
			fsq_que->setFontColor(progdefaults.RxFontcolor, FTextBase::RECV);

		Fl_Box *minbox = new Fl_Box(
			monitor_panel->x(), monitor_panel->y() + 66,
			monitor_panel->w(), monitor_panel->h() - 132);
		minbox->hide();

		monitor_panel->resizable(minbox);
	monitor_panel->end();

	Fl_Group *g = new Fl_Group(
		fsq_monitor->x(), monitor_panel->y() + monitor_panel->h() + 2,
		fsq_monitor->w(), 22);
		Fl_Group *g1 = new Fl_Group(
					g->x(), g->y() + 2, g->w() - 80, g->h());
			g1->box(FL_FLAT_BOX);
			g1->end();
	// close button
		btnCloseMonitor = new Fl_Button(
					g->x() + g->w() - 82, g->y(), 80, g->h(),
					icons::make_icon_label(_("Close"), close_icon));
		btnCloseMonitor->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
		icons::set_icon_label(btnCloseMonitor);
		btnCloseMonitor->callback((Fl_Callback*)cb_btnCloseMonitor);
		g->resizable(g1);
	g->end();

	w->end();
	w->callback((Fl_Callback*)cb_btnCloseMonitor);
	w->resizable(monitor_panel);
	w->size_range( 300, 200 );
	w->xclass(PACKAGE_NAME);

	w->hide();
	return w;
}

void open_fsqMonitor()
{
	if (!fsqMonitor)
		fsqMonitor = create_fsqMonitor();
	fsqMonitor->show();
	fsqMonitor->redraw();
}

#if (FSQDEBUG == 1)
Fl_Double_Window  *fsqDebug = 0;
FTextRX           *fsq_debug = 0;

void close_fsqDebug()
{
	if (fsqDebug) fsqDebug->hide();
}

static void cb_CloseDebug(Fl_Button*, void*) {
	fsqDebug->hide();
}

Fl_Double_Window* create_fsqDebug(void)
{
	Fl_Double_Window* w = new Fl_Double_Window( 50, 50, 500, 300, "FSQ debug");

	fsq_debug = new FTextRX(
			2, 2,
			w->w() - 4, w->h() - 4);
		fsq_debug->color( FL_WHITE);
		fsq_debug->setFont(progdefaults.RxFontnbr);
		fsq_debug->setFontSize(progdefaults.RxFontsize);
		fsq_debug->setFontColor(progdefaults.RxFontcolor, FTextBase::RECV);
		fsq_debug->setFontColor(progdefaults.XMITcolor, FTextBase::XMIT);
		fsq_debug->setFontColor(progdefaults.CTRLcolor, FTextBase::CTRL);
		fsq_debug->setFontColor(progdefaults.SKIPcolor, FTextBase::SKIP);
		fsq_debug->setFontColor(progdefaults.ALTRcolor, FTextBase::ALTR);

	w->end();
	w->callback((Fl_Callback*)cb_CloseDebug);
	w->resizable(fsq_debug);
	w->size_range( 200, 200 );
	w->xclass(PACKAGE_NAME);
	w->hide();
	return w;
}

void open_fsqDebug()
{
	if (!fsqDebug)
		fsqDebug = create_fsqDebug();
	fsqDebug->show();
	fsqDebug->redraw();
}

void write_fsqDebug(string s, int style)
{
	if (!fsq_debug) return;
	REQ(&FTextRX::addstr, fsq_debug, s, style);
}
#endif

void fsq_que_clear()
{
	REQ(&FTextRX::clear, fsq_que);
}

void write_fsq_que(std::string s)
{
	if (!fsq_que) return;
//	REQ(&FTextRX::clear, fsq_que);
	REQ(&FTextRX::addstr, fsq_que, s, FTextBase::ALTR);
}
