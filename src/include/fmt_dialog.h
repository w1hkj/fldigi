// ----------------------------------------------------------------------------
// fmt_dialog.h  --  fmt modem
//
// Copyright (C) 2020
//		Dave Freese, W1HKJ
//		JC Gibbons,  N8OBJ
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

#ifndef fmt_dialog_h
#define fmt_dialog_h
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Group.H>

#include "plot_xy.h"
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Counter.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Light_Button.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Box.H>

#include "combo.h"
#include "fmt.h"

extern plot_xy *fmt_plot;

extern Fl_Group			*ref_group;
extern Fl_Light_Button	*btn_ref_enable;
extern Fl_Button		*btn_ref_up;
extern Fl_Counter		*cnt_ref_freq;
extern Fl_Button		*btn_ref_dn;
extern Fl_Button		*btn_ref_reset;
extern Fl_Button		*btn_ref_clear;
extern Fl_Output		*fmt_ref_val;
extern Fl_Output		*fmt_ref_db;
extern Fl_Box			*ref_color;

extern Fl_Group			*unk_group;
extern Fl_Light_Button	*btn_unk_enable;
extern Fl_Button		*btn_unk_up;
extern Fl_Counter		*cnt_unk_freq;
extern Fl_Button		*btn_unk_dn;
extern Fl_Button		*btn_unk_reset;
extern Fl_Button		*btn_unk_clear;
extern Fl_Output		*fmt_unk_val;
extern Fl_Output		*fmt_unk_db;
extern Fl_Box			*unk_color;

extern Fl_Group			*fmt_record_group;
extern Fl_Light_Button	*btn_fmt_record ;
extern Fl_Box			*box_fmt_recording;

extern Fl_ListBox		*fmt_rec_interval;
extern Fl_ListBox		*fmt_scale;
extern Fl_ListBox		*fmt_cntr_minutes;

extern Fl_Group* fmt_panel(int, int, int, int);

extern void set_fmt_scope ();

extern void clear_ref_scope();
extern void clear_unk_scope();

extern void show_1(bool);
extern void show_2(bool);

extern void cb_ref_reset (void *);
extern void cb_ref_clear (void *);

extern void cb_unk_reset (void *);
extern void cb_unk_clear (void *);

extern void put_unk_value (const char *msg);
extern void put_ref_value (const char *msg);
extern void put_unk_amp (const char *msg);
extern void put_ref_amp (const char *msg);

extern void set_ref_freq (void *);
extern void set_unk_freq (void *);
extern void set_unk_freq_value(double f);
extern void set_ref_freq_value(double f);

#endif
