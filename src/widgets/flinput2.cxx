// ----------------------------------------------------------------------------
// flinput2.cxx
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

#include <config.h>

#include <cctype>

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Tooltip.H>

#include "icons.h"
#include "flinput2.h"
#include "gettext.h"


enum { OP_UNDO, OP_CUT, OP_COPY, OP_PASTE, OP_DELETE, OP_CLEAR, OP_SELECT_ALL };

static Fl_Menu_Item cmenu[] = {
	{ make_icon_label(_("Undo"), edit_undo_icon), 0, 0, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL },
	{ make_icon_label(_("Cut"), edit_cut_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ make_icon_label(_("Copy"), edit_copy_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ make_icon_label(_("Paste"), edit_paste_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ make_icon_label(_("Delete"), trash_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ make_icon_label(_("Clear"), edit_clear_icon), 0, 0, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL },
	{ make_icon_label(_("Select All"), edit_select_all_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ 0 }
};
static bool cmenu_init = false;


Fl_Input2::Fl_Input2(int x, int y, int w, int h, const char* l)
	: Fl_Input(x, y, w, h, l)
{
	if (!cmenu_init) {
		for (size_t i = 0; i < sizeof(cmenu)/sizeof(*cmenu) - 1; i++)
			if (cmenu[i].labeltype() == _FL_MULTI_LABEL)
				set_icon_label(&cmenu[i]);
		cmenu_init = true;
	}
}

int Fl_Input2::handle(int event)
{
	switch (event) {
	case FL_KEYBOARD: {
		int b = Fl::event_key();
		int p = position();
		// stop the move-to-next-field madness, we have Tab for that!
		if (unlikely((b == FL_Left && p == 0) || (b == FL_Right && p == size()) ||
			     (b == FL_Up && line_start(p) == 0) ||
			     (b == FL_Down && line_end(p) == size())))
			return 1;
		else if (unlikely(Fl::event_state() & (FL_ALT | FL_META))) {
			switch (b) {
			case 'c': { // capitalise
				if (readonly() || p == size())
					return 1;

				while (p < size() && isspace(*(value() + p)))
					p++;
				if (p == size())
					return 1;
				char c = toupper(*(value() + p));
				replace(p, p + 1, &c, 1);
				position(word_end(p));
			}
				return 1;
			case 'u': case 'l': { // upper/lower case
				if (readonly() || p == size())
					return 1;
				while (p < size() && isspace(*(value() + p)))
					p++;
				int n = word_end(p) - p;
				if (n == 0)
					return 1;

				char* s = new char[n];
				memcpy(s, value() + p, n);
				if (b == 'u')
					for (int i = 0; i < n; i++)
						s[i] = toupper(s[i]);
				else
					for (int i = 0; i < n; i++)
						s[i] = tolower(s[i]);
				replace(p, p + n, s, n);
				position(p + n);
				delete [] s;
				return 1;
			}
			default:
				break;
			}
		}
	}
		return Fl_Input::handle(event);
	case FL_MOUSEWHEEL: {
		if (!((type() & (FL_MULTILINE_INPUT | FL_MULTILINE_OUTPUT)) && Fl::event_inside(this)))
			return Fl_Input::handle(event);
		int d;
		if (!((d = Fl::event_dy()) || (d = Fl::event_dx())))
			return Fl_Input::handle(event);
		if (Fl::focus() != this)
			take_focus();
		up_down_position(d + (d > 0 ? line_end(position()) : line_start(position())));
		return 1;
	}
	case FL_PUSH:
		if (Fl::event_button() == FL_RIGHT_MOUSE)
			break;
		// fall through
	default:
		return Fl_Input::handle(event);
	}

	bool sel = position() != mark(), ro = readonly();
	set_active(&cmenu[OP_UNDO], !ro);
	set_active(&cmenu[OP_CUT], !ro && sel);
	set_active(&cmenu[OP_COPY], sel);
	set_active(&cmenu[OP_PASTE], !ro);
	set_active(&cmenu[OP_DELETE], !ro && sel);
	set_active(&cmenu[OP_CLEAR], !ro && size());
	set_active(&cmenu[OP_SELECT_ALL], size());

	take_focus();
	window()->cursor(FL_CURSOR_DEFAULT);
	int t = Fl_Tooltip::enabled();
	Fl_Tooltip::disable();
	const Fl_Menu_Item* m = cmenu->popup(Fl::event_x(), Fl::event_y());
	Fl_Tooltip::enable(t);

	if (!m)
		return 1;
	switch (m - cmenu) {
	case OP_UNDO:
		undo();
		break;
	case OP_CUT:
		cut();
		copy_cuts();
		break;
	case OP_COPY:
		copy(1);
		break;
	case OP_PASTE:
		Fl::paste(*this, 1);
		break;
	case OP_DELETE:
		cut();
		break;
	case OP_CLEAR:
		cut(0, size());
		break;
	case OP_SELECT_ALL:
		position(0, size());
		break;
	}

	return 1;
}
