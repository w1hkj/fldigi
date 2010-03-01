// ----------------------------------------------------------------------------
// icons.h
//
// Copyright (C) 2008-2009
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

#ifndef ICONS_H_
#define ICONS_H_

#define USE_IMAGE_LABELS 1

#include <FL/Fl_Widget.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/fl_ask.H>
#include "pixmaps.h"

const char* make_icon_label(const char* text, const char** pixmap = 0);

void set_icon_label(Fl_Menu_Item* item);
void set_icon_label(Fl_Widget* w);

void toggle_icon_labels(void);

const char* get_icon_label_text(Fl_Menu_Item* item);
const char* get_icon_label_text(Fl_Widget* w);

void free_icon_label(Fl_Menu_Item* item);
void free_icon_label(Fl_Widget* w);

void set_active(Fl_Menu_Item* item, bool v);
void set_active(Fl_Widget* w, bool v);

// fltk message dialogs with nicer icons
void set_message_icon(const char** pixmap);
#define fl_input2(...) ({ set_message_icon(dialog_question_48_icon); fl_input(__VA_ARGS__); })
#define fl_choice2(...) ({ set_message_icon(dialog_question_48_icon); fl_choice(__VA_ARGS__); })
#define fl_message2(...) ({ set_message_icon(dialog_information_48_icon); fl_message(__VA_ARGS__); })
#define fl_alert2(...) ({ set_message_icon(dialog_warning_48_icon); fl_alert(__VA_ARGS__); })
#define fl_warn_choice2(...) ({ set_message_icon(dialog_warning_48_icon); fl_choice(__VA_ARGS__); })

#endif // ICONS_H_
