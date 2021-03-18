// ----------------------------------------------------------------------------
// fmt_dialog.cxx  --  fmt modem
//
// Copyright (C) 2020
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

#include <iostream>

#include <FL/Fl_Box.H>

#include "configuration.h"
#include "confdialog.h"
#include "gettext.h"
#include "fmt_dialog.h"
#include "fmt.h"
#include "fl_digi.h"
#include "modem.h"
#include "status.h"

plot_xy *fmt_plot = (plot_xy *)0;

Fl_Group *ref_group						= (Fl_Group *)0;
Fl_Light_Button *btn_ref_enable			= (Fl_Light_Button *)0;
Fl_Button *btn_ref_up					= (Fl_Button *)0;
Fl_Counter *cnt_ref_freq				= (Fl_Counter *)0;
Fl_Button *btn_ref_dn					= (Fl_Button *)0;
Fl_Button *btn_ref_reset				= (Fl_Button *)0;
Fl_Button *btn_ref_clear				= (Fl_Button *)0;
Fl_Output *fmt_ref_val					= (Fl_Output *)0;
Fl_Output *fmt_ref_db					= (Fl_Output *)0;
Fl_Box *ref_color						= (Fl_Box *)0;

Fl_Group *unk_group						= (Fl_Group *)0;
Fl_Light_Button *btn_unk_enable			= (Fl_Light_Button *)0;
Fl_Button  *btn_unk_up					= (Fl_Button *)0;
Fl_Counter *cnt_unk_freq				= (Fl_Counter *)0;
Fl_Button *btn_unk_dn					= (Fl_Button *)0;
Fl_Button *btn_unk_reset				= (Fl_Button *)0;
Fl_Button *btn_unk_clear				= (Fl_Button *)0;
Fl_Output *fmt_unk_val					= (Fl_Output *)0;
Fl_Output *fmt_unk_db					= (Fl_Output *)0;
Fl_Box *unk_color						= (Fl_Box *)0;

Fl_Light_Button *btn_fmt_record			= (Fl_Light_Button *)0;
Fl_Box *box_fmt_recording				= (Fl_Box *)0;

Fl_ListBox *fmt_rec_interval			= (Fl_ListBox *)0;
Fl_ListBox *fmt_scale					= (Fl_ListBox *)0;
Fl_ListBox *fmt_cntr_minutes			= (Fl_ListBox *)0;

static const char *legend_p100 =
	"-.010|-.008|-.006|-.004|-.002|0|.002|.004|.006|.008|.010";
static const char *legend_p50 =
	"-0.05|-0.04|-0.03|-0.02|-0.01|0|0.01|0.02|0.03|0.04|0.05";
static const char *legend_p10 =
	"-.10|-.08|-.06|-.04|-.02|0|.02|.04|.06|.08|.10";
static const char *legend_p25 =
	"-0.25|-0.2|-0.15|-0.10|-0.05|0|0.05|0.10|0.15|0.20|0.25";
static const char *legend_p5 =
	"-0.5|-0.4|-0.3|-0.2|-0.1|0|0.1|0.2|0.3|0.4|0.5";
static const char *legend_1p =
	"-1.0|-0.8|-0.6|-0.4|-0.2|0|0.2|0.4|0.6|0.8|1.0";
static const char *legend_2p =
	"-2.0|-1.5|-1.0|-0.5|0|0.5|1.0|1.5|2.0";
static const char *legend_4p =
	"-4.0|-3.5|-3.0|-2.5|-2.0|-1.5|-1.0|-0.5|0|0.5|1.0|1.5|2.0|2.5|3.0|3.5|4.0";
static const char *legend_5p =
	"5.0|-4.0|-3.0|-2.0|-1.0|0|1.0|2.0|3.0|4.0|5.0";
static const char *legend_10p =
	"-10.0|-8.0|-6.0|-4.0|-2.0|0|2.0|4.0|6.0|8.0|10.0";

static const char *legend_5   = " |4|3|2|1| |";
static const char *legend_15  = " |14|13|12|11|10|9|8|7|6|5|4|3|2|1| |";
static const char *legend_30  = " |28|26|24|22|20|18|16|14|12|10|8|6|4|2| |";
static const char *legend_60  = " |55|50|45|40|35|30|25|20|15|10|5| |";
static const char *legend_120 = " |110|100|90|80|70|60|50|40|30|20|10| |";

static int seconds[5] = {300, 900, 1800, 3600, 7200};

void cb_unk_up(void *)
{
	double f = cnt_unk_freq->value () + 10;
	if (f > 3000) f = 3000;
	cnt_unk_freq->value (f);
	cnt_unk_freq->callback ();
}

void cb_unk_dn(void *)
{
	double f = cnt_unk_freq->value () - 10;
	if (f < 100) f = 100;
	cnt_unk_freq->value (f);
	cnt_unk_freq->callback ();
}

void cb_ref_up(void *)
{
	double f = cnt_ref_freq->value () + 10;
	if (f > 3000) f = 3000;
	cnt_ref_freq->value (f);
	cnt_ref_freq->callback ();
}

void cb_ref_dn(void *)
{
	double f = cnt_ref_freq->value () - 10;
	if (f < 100) f = 100;
	cnt_ref_freq->value (f);
	cnt_ref_freq->callback ();
}

void cb_btn_fmt_record(void *)
{
	write_recs = btn_fmt_record->value();
}

void cb_btn_unk_enable(void *)
{
	set_unk_freq(NULL);
	record_unk = btn_unk_enable->value();
}

void cb_btn_ref_enable(void *)
{
	set_ref_freq(NULL);
	record_ref = btn_ref_enable->value();
}

void fmt_rec_interval_cb(void *)
{
	progStatus.FMT_rec_interval = fmt_rec_interval->index();
}

void fmt_set_x_scale()
{
	int minutes = 0;
	int markers = 0;
	const char *legend = NULL;

	switch (progStatus.FMT_minutes) {
		case 4: minutes = 120; markers = 12; legend = legend_120; break;
		case 3: minutes = 60; markers = 12; legend = legend_60;  break;
		case 2: minutes = 30; markers = 15; legend = legend_30;  break;
		case 1: minutes = 15; markers = 15; legend = legend_15;  break;
		case 0:
		default:
			minutes = 5; markers = 5; legend = legend_5;
	}
	fmt_plot->x_scale (MAX_DATA_PTS - 60 * minutes, MAX_DATA_PTS, markers);
	fmt_plot->set_x_legend (legend);
	fmt_plot->thick_lines(progdefaults.FMT_thick_lines);
	fmt_plot->plot_over_axis(progdefaults.FMT_plot_over_axis);
	fmt_plot->redraw();
}

void fmt_set_y_scale()
{
	switch (progStatus.FMT_trk_scale) {

		case 0 :
			fmt_plot->y_scale(-0.01, 0.01, 10);
			fmt_plot->set_y_legend(legend_p100);
			break;
		case 1 :
			fmt_plot->y_scale(-0.05, .05, 10);
			fmt_plot->set_y_legend(legend_p50);
			break;
		case 2 :
			fmt_plot->y_scale(-0.10, 0.10, 10);
			fmt_plot->set_y_legend(legend_p10);
			break;
		case 3 :
			fmt_plot->y_scale(-0.25, .25, 10);
			fmt_plot->set_y_legend(legend_p25);
			break;
		case 4 :
			fmt_plot->y_scale(-0.5, .5, 10);
			fmt_plot->set_y_legend(legend_p5);
			break;
		case 5 :
			fmt_plot->y_scale(-1.0, 1.0, 10);
			fmt_plot->set_y_legend(legend_1p);
			break;
		case 6 :
			fmt_plot->y_scale(-2.0, 2.0, 8);
			fmt_plot->set_y_legend(legend_2p);
			break;
		case 7 :
			fmt_plot->y_scale(-4.0, 4.0, 16);
			fmt_plot->set_y_legend(legend_4p);
			break;
		case 8 :
			fmt_plot->y_scale(-5.0, 5.0, 10);
			fmt_plot->set_y_legend(legend_5p);
			break;
		case 9 :
			fmt_plot->y_scale(-10.0, 10.0, 10);
			fmt_plot->set_y_legend(legend_10p);
			break;
		default:
			fmt_plot->y_scale(-1.0, 1.0, 10);
			fmt_plot->set_y_legend(legend_1p);
			break;
	}
	fmt_plot->redraw();
}

void fmt_scale_cb(void *)
{
	progStatus.FMT_trk_scale = fmt_scale->index();
	fmt_set_y_scale();
}

void fmt_cntr_minutes_cb(void *)
{
	progStatus.FMT_minutes = fmt_cntr_minutes->index();
	fmt_set_x_scale();
}

Fl_Group* fmt_panel(int X, int Y, int W, int H) {
	Fl_Group* grp = new Fl_Group(X, Y, W, H);

	int grp_height = 24;

	fmt_plot = new plot_xy (
		grp->x() + 2, grp->y() + 2,
		grp->w() - 4, grp->h() - 4- 2 * grp_height, "");

		fmt_plot->reverse_x(progdefaults.FMT_reverse);
		fmt_plot->bk_color (progdefaults.FMT_background);
		fmt_plot->line_color_1 (progdefaults.FMT_unk_color);
		fmt_plot->line_color_2 (progdefaults.FMT_ref_color);
		fmt_plot->axis_color (progdefaults.FMT_axis_color);
		fmt_plot->legend_color (progdefaults.FMT_legend_color);
		fmt_set_x_scale ();
		fmt_set_y_scale ();
		fmt_plot->show_1(false);
		fmt_plot->show_2(false);

	ref_group = new Fl_Group(
		fmt_plot->x(), fmt_plot->y() + fmt_plot->h(),
		fmt_plot->w(), 24);
		ref_group->box(FL_ENGRAVED_BOX);

		btn_unk_enable = new Fl_Light_Button(
			ref_group->x() + 2, ref_group->y() + 2,
				50, 20, "Unk'");
			btn_unk_enable->selection_color(progdefaults.default_btn_color);
			btn_unk_enable->callback((Fl_Callback *)cb_btn_unk_enable);

		unk_color = new Fl_Box(
			btn_unk_enable->x() + btn_unk_enable->w() + 2, btn_unk_enable->y() + 4,
			12, 12, "");
			unk_color->box(FL_DOWN_BOX);
			unk_color->color(progdefaults.FMT_unk_color);

		btn_unk_dn = new Fl_Button(
			unk_color->x() + unk_color->w() + 2, btn_unk_enable->y(),
				20, 20, "@|<");
			btn_unk_dn->callback((Fl_Callback*)cb_unk_dn);

		cnt_unk_freq = new Fl_Counter(
			btn_unk_dn->x() + btn_unk_dn->w(), btn_unk_dn->y(),
				120, 20, "");
		cnt_unk_freq->minimum(100);
		cnt_unk_freq->maximum(4000);
		cnt_unk_freq->step(0.1);
		cnt_unk_freq->lstep(1.0);
		cnt_unk_freq->value(progStatus.FMT_unk_freq);
		cnt_unk_freq->callback((Fl_Callback*)set_unk_freq);

		btn_unk_up = new Fl_Button(
			cnt_unk_freq->x() + cnt_unk_freq->w(), cnt_unk_freq->y(),
				20, 20, "@>|");
			btn_unk_up->callback((Fl_Callback*)cb_unk_up);

		btn_unk_reset = new Fl_Button(
			btn_unk_up->x() + btn_unk_up->w() + 2, btn_unk_up->y(),
			50, 20, "Reset");
			btn_unk_reset->callback((Fl_Callback*)cb_unk_reset);
			btn_unk_reset->tooltip("Reset unknown frequency");

		fmt_unk_val = new Fl_Output(
			btn_unk_reset->x() + btn_unk_reset->w() + 4, btn_unk_enable->y(),
			110, 20, "");
			fmt_unk_val->value(0);

		fmt_unk_db = new Fl_Output(
			fmt_unk_val->x() + fmt_unk_val->w() + 6, btn_unk_enable->y(),
			50, 20, "");
			fmt_unk_db->value("");
			fmt_unk_db->tooltip("amplitude in dBvp");

		btn_unk_clear = new Fl_Button(
			fmt_unk_db->x() + fmt_unk_db->w() + 6, fmt_unk_val->y(),
			60, 20, "Clear");
		btn_unk_clear->callback((Fl_Callback*)cb_unk_clear);
		btn_unk_clear->tooltip("Clear unknown plot");

		Fl_Group *dmy1 = new Fl_Group(
			btn_unk_clear->x() + btn_unk_clear->w(), btn_unk_clear->y(),
			1, btn_unk_clear->h());
			dmy1->box(FL_FLAT_BOX);
		dmy1->end();

		btn_fmt_record = new Fl_Light_Button(
			ref_group->x() + ref_group->w() - 84, dmy1->y(),
			80, 20, "Record");
			btn_fmt_record->callback((Fl_Callback *)cb_btn_fmt_record);

		box_fmt_recording = new Fl_Box(
			btn_fmt_record->x() - 20, btn_fmt_record->y() + 4,
			12, 12, "");
		box_fmt_recording->box(FL_DOWN_BOX);
		box_fmt_recording->color(FL_WHITE);

		fmt_rec_interval = new Fl_ListBox(
			box_fmt_recording->x() - 110, btn_fmt_record->y(),
			85, 20, "Interval");
			fmt_rec_interval->align(FL_ALIGN_LEFT);
			fmt_rec_interval->add("0.10 sec");
			fmt_rec_interval->add("0.25 sec");
			fmt_rec_interval->add("0.50 sec");
			fmt_rec_interval->add("1.0  sec");
			fmt_rec_interval->add("2.0  sec");
			fmt_rec_interval->add("5.0  sec");
			fmt_rec_interval->add("10.0 sec");
			fmt_rec_interval->color(FL_WHITE);
			fmt_rec_interval->index(progStatus.FMT_rec_interval);
			fmt_rec_interval->callback((Fl_Callback *)fmt_rec_interval_cb);
			fmt_rec_interval->tooltip(_("Record update every NN seconds"));

	ref_group->end();
	ref_group->resizable(dmy1);


	unk_group = new Fl_Group(
		ref_group->x(), ref_group->y() + ref_group->h(),
		ref_group->w(), grp_height);
		unk_group->box(FL_ENGRAVED_BOX);

		btn_ref_enable = new Fl_Light_Button(
			unk_group->x() + 2, unk_group->y() + 2,
				50, 20, "Ref'");
			btn_ref_enable->selection_color(progdefaults.default_btn_color);
			btn_ref_enable->callback((Fl_Callback *)cb_btn_ref_enable);

		ref_color = new Fl_Box(
			btn_ref_enable->x() + btn_ref_enable->w() + 2, btn_ref_enable->y() + 4,
			12, 12, "");
			ref_color->box(FL_DOWN_BOX);
			ref_color->color(progdefaults.FMT_ref_color);

		btn_ref_dn = new Fl_Button(
			unk_color->x() + unk_color->w() + 2, btn_ref_enable->y(),
				20, 20, "@|<");
			btn_ref_dn->callback((Fl_Callback*)cb_ref_dn);

		cnt_ref_freq = new Fl_Counter(
			cnt_unk_freq->x(), btn_ref_dn->y(),
			120, 20, "");
		cnt_ref_freq->minimum(100);
		cnt_ref_freq->maximum(4000);
		cnt_ref_freq->step(0.1);
		cnt_ref_freq->lstep(1.0);
		cnt_ref_freq->value(progStatus.FMT_ref_freq);
		cnt_ref_freq->callback((Fl_Callback*)set_ref_freq);

		btn_ref_up = new Fl_Button(
			btn_unk_up->x(), cnt_ref_freq->y(),
				20, 20, "@>|");
			btn_ref_up->callback((Fl_Callback*)cb_ref_up);

		btn_ref_reset = new Fl_Button(
			btn_ref_up->x() + btn_ref_up->w() + 2, btn_ref_up->y(),
			50, 20, "Reset");
			btn_ref_reset->callback((Fl_Callback*)cb_ref_reset);
			btn_ref_reset->tooltip("Reset unknown tracking frequency");

		fmt_ref_val = new Fl_Output(
			btn_ref_reset->x() + btn_ref_reset->w() + 2, btn_ref_reset->y(),
			110, 20, "");
			fmt_ref_val->value("");

		fmt_ref_db = new Fl_Output(
			fmt_unk_db->x(), fmt_ref_val->y(),
			50, 20, "");
			fmt_ref_db->value("");
			fmt_ref_db->tooltip("amplitude in dBvp");

		btn_ref_clear = new Fl_Button(
			btn_unk_clear->x(), fmt_ref_val->y(),
			60, 20, "Clear");
			btn_ref_clear->callback((Fl_Callback*)cb_ref_clear);
			btn_ref_clear->tooltip("Clear reference plot");

		Fl_Group *dmy2 = new Fl_Group(
			btn_ref_clear->x() + btn_ref_clear->w(), btn_ref_clear->y(),
			1, btn_ref_clear->h());
			dmy2->box(FL_FLAT_BOX);
		dmy2->end();

		fmt_scale = new Fl_ListBox(
			unk_group->x() + unk_group->w() - 84, dmy2->y(),
			80, 20, "Scale");
			fmt_scale->align(FL_ALIGN_LEFT);
			fmt_scale->add("+/- .01");
			fmt_scale->add("+/- .05");
			fmt_scale->add("+/- .10");
			fmt_scale->add("+/- .25");
			fmt_scale->add("+/- .5");
			fmt_scale->add("+/- 1");
			fmt_scale->add("+/- 2");
			fmt_scale->add("+/- 4");
			fmt_scale->add("+/- 5");
			fmt_scale->add("-/- 10");
			fmt_scale->color(FL_WHITE);
			fmt_scale->index(progStatus.FMT_trk_scale);
			fmt_scale->callback((Fl_Callback *)fmt_scale_cb);
			fmt_scale->tooltip(_("Vertical scale of tracks"));

		fmt_cntr_minutes = new Fl_ListBox(
			fmt_scale->x() - 130, fmt_scale->y(),
			85, 20, "T-scale");
			fmt_cntr_minutes->align(FL_ALIGN_LEFT);
			fmt_cntr_minutes->add("5   min");
			fmt_cntr_minutes->add("15  min");
			fmt_cntr_minutes->add("30  min");
			fmt_cntr_minutes->add("60  min");
			fmt_cntr_minutes->add("120 min");
			fmt_cntr_minutes->color(FL_WHITE);
			fmt_cntr_minutes->index(progStatus.FMT_minutes);
			fmt_cntr_minutes->callback((Fl_Callback *)fmt_cntr_minutes_cb);
			fmt_cntr_minutes->tooltip(_("Time scale span in minutes"));

	unk_group->end();
	unk_group->resizable(dmy2);

	grp->end();
	grp->resizable (fmt_plot);

	return grp;
}

void put_unk_value (const char *msg)
{
	fmt_unk_val->value (msg);
}

void put_ref_value (const char *msg)
{
	fmt_ref_val->value (msg);
}

void put_unk_amp (const char *msg)
{
	fmt_unk_db->value (msg);
}

void put_ref_amp (const char *msg)
{
	fmt_ref_db->value (msg);
}

void set_ref_freq (void *) {
	progStatus.FMT_ref_freq = cnt_ref_freq->value();
	wf->draw_fmt_marker();
	active_modem->reset_reference();
}

void set_unk_freq (void *) {
	progStatus.FMT_unk_freq = cnt_unk_freq->value();
	wf->draw_fmt_marker();
	active_modem->reset_unknown();
}

void set_unk_freq_value(double f)
{
	cnt_unk_freq->value(f);
	progStatus.FMT_unk_freq = f;
	wf->draw_fmt_marker();
	active_modem->reset_unknown();
}

void set_ref_freq_value(double f)
{
	cnt_ref_freq->value(f);
	progStatus.FMT_ref_freq = f;
	wf->draw_fmt_marker();
	active_modem->reset_reference();
}

void cb_unk_reset (void *) {
	active_modem->reset_unknown();
}

void cb_unk_clear (void *) {
	fmt_modem->clear_unk_pipe();
	fmt_plot->data_1(NULL, 0);
	fmt_plot->redraw();
}

void cb_ref_reset (void *) {
	active_modem->reset_reference();
}

void cb_ref_clear (void *) {
	fmt_modem->clear_ref_pipe();
	fmt_plot->data_2(NULL, 0);
	fmt_plot->redraw();
}

void set_fmt_scope()
{
	guard_lock datalock (&scope_mutex);

	int num_pts = seconds[progStatus.FMT_minutes];

	fmt_plot->data_1 (&fmt_modem->unk_pipe[MAX_DATA_PTS - num_pts], num_pts);
	fmt_plot->data_2 (&fmt_modem->ref_pipe[MAX_DATA_PTS - num_pts], num_pts);

	fmt_plot->redraw();

}

void clear_ref_scope()
{
	fmt_plot->data_2 (&fmt_modem->ref_pipe[MAX_DATA_PTS], MAX_DATA_PTS);
	fmt_plot->redraw();
	Fl::flush();
}

void clear_unk_scope()
{
	fmt_plot->data_1 (&fmt_modem->ref_pipe[MAX_DATA_PTS], MAX_DATA_PTS);
	fmt_plot->redraw();
	Fl::flush();
}

