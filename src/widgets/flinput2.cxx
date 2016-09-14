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
#include "debug.h"


enum { OP_UNDO, OP_CUT, OP_COPY, OP_PASTE, OP_DELETE, OP_CLEAR, OP_SELECT_ALL };

static Fl_Menu_Item cmenu[] = {
	{ icons::make_icon_label(_("Undo"), edit_undo_icon), 0, 0, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL },
	{ icons::make_icon_label(_("Cut"), edit_cut_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ icons::make_icon_label(_("Copy"), edit_copy_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ icons::make_icon_label(_("Paste"), edit_paste_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ icons::make_icon_label(_("Delete"), trash_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ icons::make_icon_label(_("Clear"), edit_clear_icon), 0, 0, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL },
	{ icons::make_icon_label(_("Select All"), edit_select_all_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ _("Spec Char"), 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL },
		{ "¢ - cent", 0, 0, 0, 0, FL_NORMAL_LABEL },
		{ "£ - pound", 0, 0, 0, 0, FL_NORMAL_LABEL },
		{ "µ - micro", 0, 0, 0, 0, FL_NORMAL_LABEL },
		{ "° - degree", 0, 0, 0, 0, FL_NORMAL_LABEL },
		{ "¿ - iques", 0, 0, 0, 0, FL_NORMAL_LABEL },
		{ "× - times", 0, 0, 0, 0, FL_NORMAL_LABEL },
		{ "÷ - divide", 0, 0, 0, 0, FL_NORMAL_LABEL },
		{ _("A"), 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL },
			{ "À - grave", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{ "à - grave", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{ "Á - acute", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{ "á - acute", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{ "Â - circ", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{ "â - circ", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{ "Ã - tilde", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{ "ã - tilde", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{ "Ä - umlaut", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{ "ä - umlaut", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{ "Å - ring", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{ "å - ring", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{ "Æ - aelig", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{ "æ - aelig", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{0,0,0,0,0,0,0,0,0},
		{ _("E"), 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL },
			{ "È - grave", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{ "è - grave", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{ "É - acute", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{ "é - acute", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{ "Ê - circ", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{ "Ê - circ", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{ "Ë - umlaut", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{ "ë - umlaut", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{0,0,0,0,0,0,0,0,0},
		{ _("I"), 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL },
			{ "Ì - grave", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{ "ì - grave", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{ "Í - acute", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{ "í - acute", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{ "Î - circ", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{ "î - circ", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{ "Ï - umlaut", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{ "ï - umlaut", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{0,0,0,0,0,0,0,0,0},
		{ _("N"), 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL },
			{ "Ñ - tilde", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{ "ñ - tilde", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{0,0,0,0,0,0,0,0,0},
		{ _("O"), 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL },
			{ "Ò - grave", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{ "ò - grave", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{ "Ó - acute", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{ "ó - acute", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{ "Ô - circ", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{ "ô - circ", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{ "Õ - tilde", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{ "õ - tilde", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{ "Ö - umlaut", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{ "ö - umlaut", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{ "Ø - slash", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{ "ø - slash", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{0,0,0,0,0,0,0,0,0},
		{ _("U"), 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL },
			{ "Ù - grave", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{ "ù - grave", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{ "Ú - acute", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{ "ú - acute", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{ "Û - circ", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{ "û - circ", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{ "Ü - umlaut", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{ "ü - umlaut", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{0,0,0,0,0,0,0,0,0},
		{ _("Y"), 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL },
			{ "Ý - acute", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{ "ý - acute", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{ "ÿ - umlaut", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{0,0,0,0,0,0,0,0,0},
		{ _("Other"), 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL },
			{ "ß - szlig", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{ "Ç - cedil", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{ "ç - cedil", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{ "Ð - eth", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{ "ð - eth", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{ "Þ - thorn", 0, 0, 0, 0, FL_NORMAL_LABEL },
			{0,0,0,0,0,0,0,0,0},
		{0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0}
};
static bool cmenu_init = false;

Fl_Input2::Fl_Input2(int x, int y, int w, int h, const char* l)
	: Fl_Input(x, y, w, h, l)
{
	if (!cmenu_init) {
		for (size_t i = 0; i < sizeof(cmenu)/sizeof(*cmenu) - 1; i++)
			if (cmenu[i].labeltype() == _FL_MULTI_LABEL)
				icons::set_icon_label(&cmenu[i]);
		cmenu_init = true;
	}
	ascii_cnt = 0; // restart the numeric keypad entries.
	ascii_chr = 0;
	utf8text = NULL;
}

//----------------------------------------------------------------------
/// Composes ascii characters and adds them to the Fl_Input2 buffer.
/// Control characters are inserted with the CTRL style. Values larger than 127
/// (0x7f) are ignored. We cannot really add NULs for the time being.
/// 
/// @param key A digit character
///
/// @return 1
///
int Fl_Input2::handle_key_ascii(int key)
{
	if (key >= FL_KP) key -= FL_KP;
	key -= '0';
	ascii_cnt++;
	ascii_chr *= 10;
	ascii_chr += key;
	if (ascii_cnt == 3) {
		if (ascii_chr < 0x100) {
			utf8text = new char[fl_utf8bytes(ascii_chr) + 1];
			utf8cnt = fl_utf8encode(ascii_chr, utf8text);
			return 1;
		}
		ascii_cnt = ascii_chr = 0;
	}

	return 0;
}

//----------------------------------------------------------------------
int Fl_Input2::handle(int event)
{
	switch (event) {
	if (event == FL_LEAVE) {
		do_callback();
		return 1;
	}
	case FL_KEYBOARD: {
		int b = Fl::event_key();

		if (b == FL_Enter || b == FL_KP_Enter) {
			do_callback();
			return Fl_Input::handle(event);
		}
		if (b == FL_Tab) {
			do_callback();
			return Fl_Input::handle(event);
		}

		if ((Fl::event_state() & FL_CTRL) && (isdigit(b) || isdigit(b - FL_KP))) {
			if (handle_key_ascii(b)) {
				if (utf8text) {
					insert(utf8text, utf8cnt);
					delete utf8text;
				}
				ascii_cnt = 0;
				ascii_chr = 0;
			}
			return 1;
		}
		ascii_cnt = 0;
		ascii_chr = 0;

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
	icons::set_active(&cmenu[OP_UNDO], !ro);
	icons::set_active(&cmenu[OP_CUT], !ro && sel);
	icons::set_active(&cmenu[OP_COPY], sel);
	icons::set_active(&cmenu[OP_PASTE], !ro);
	icons::set_active(&cmenu[OP_DELETE], !ro && sel);
	icons::set_active(&cmenu[OP_CLEAR], !ro && size());
	icons::set_active(&cmenu[OP_SELECT_ALL], size());

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
	default:
		insert(m->text, 1);
	}

	return 1;
}
