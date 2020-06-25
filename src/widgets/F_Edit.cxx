// ----------------------------------------------------------------------------
//      FTextView.cxx
//
// Copyright (C) 2007-2009
//              Stelios Bounanos, M0GLD
//
// Copyright (C) 2008-2009
//              Dave Freese, W1HKJ
//
// This file is part of fldigi.
//
// fldigi is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#include <config.h>

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <sys/stat.h>
#include <map>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <iomanip>

#include <string>

#include <FL/Fl_Tooltip.H>

#include "flmisc.h"
#include "fileselect.h"
#include "font_browser.h"
#include "ascii.h"
#include "icons.h"
#include "gettext.h"
#include "macros.h"

#include "F_Edit.h"

#include "debug.h"

using namespace std;

int * F_Edit::p_editpos;

Fl_Menu_Item F_Edit::menu[] = {
	{ 0 }, // EDIT_MENU_CUT
	{ 0 }, // EDIT_MENU_COPY
	{ 0 }, // EDIT_MENU_PASTE
	{ 0 }, // EDIT_MENU_CLEAR
	{ 0 }, // EDIT_MENU_READ
	{ 0 }, // EDIT_MENU_WRAP

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
	{ 0 }
};

F_Edit::F_Edit(int x, int y, int w, int h, const char *l)
	: FTextEdit(x, y, w, h, l), editpos(0), bkspaces(0)
{
	change_keybindings();

	memcpy(menu + EDIT_MENU_CUT, FTextEdit::menu, (FTextEdit::menu->size() - 1) * sizeof(*FTextEdit::menu));
	context_menu = menu;
	init_context_menu();

	p_editpos = &editpos;

}

/// Handles fltk events for this widget.
/// We pass keyboard events to handle_key() and handle mouse3 presses to show
/// the popup menu. We also disallow mouse2 events in the transmitted text area.
/// Everything else is passed to the base class handle().
///
/// @param event
///
/// @return
///
int F_Edit::handle(int event)
{
//	if ( !(Fl::event_inside(this) || (event == FL_KEYBOARD && Fl::focus() == this)) )
//		return FTextEdit::handle(event);

//	if (event == FL_KEYBOARD)
//		return handle_key(Fl::event_key()) ? 1 : FTextEdit::handle(event);

	return FTextEdit::handle(event);
}

/// Clears the buffer.
/// Also resets the transmit position, stored backspaces and tx pause flag.
///
void F_Edit::clear(void)
{
	FTextEdit::clear();
	editpos = 0;
	bkspaces = 0;
}

void F_Edit::add_text(string s)
{
	for (size_t n = 0; n < s.length(); n++) {
		if (s[n] == '\b') {
			int ipos = insert_position();
			if (tbuf->length()) {
				if (ipos > 0) {
					bkspaces++;
					int nn;
					tbuf->get_char_at(editpos, nn);
					editpos -= nn;
				}
				tbuf->remove(tbuf->length() - 1, tbuf->length());
				sbuf->remove(sbuf->length() - 1, sbuf->length());
				redraw();
			}
		} else {
			add(s[n] & 0xFF, RECV);
		}
	}
}

void F_Edit::setFont(Fl_Font f, int attr)
{
	FTextBase::setFont(f, attr);
}

/// Handles keyboard events to override Fl_Text_Editor_mod's handling of some
/// keystrokes.
///
/// @param key
///
/// @return
///
int F_Edit::handle_key(int key)
{
	switch (key) {
	case FL_Tab:
		if (editpos != insert_position())
			insert_position(editpos);
		else
			insert_position(tbuf->length());
		return 1;
	case FL_BackSpace: {
			int ipos = insert_position();
			if (editpos > 0 && editpos == ipos) {
				bkspaces++;
				editpos = tbuf->prev_char(ipos);
			}
			return 0;
		}
	default:
		break;
	}

// read ctl-ddd, where d is a digit, as ascii characters (in base 10)
// and insert verbatim; e.g. ctl-001 inserts a <soh>
	if (Fl::event_state() & FL_CTRL && (key >= FL_KP) && (key <= FL_KP + '9'))
		return handle_key_ascii(key);

// restart the numeric keypad entries.
	ascii_cnt = 0;
	ascii_chr = 0;

	return 0;
}

int F_Edit::handle_dnd_drag(int pos)
{
	return 1;
	return FTextEdit::handle_dnd_drag(pos);
}

/// Handles mouse-3 clicks by displaying the context menu
///
/// @param val
///
void F_Edit::handle_context_menu(void)
{
	bool selected = tbuf->selected();
std::cout << "F_Edit::tbuf " << (selected ? "selected" : "not selected") << std::endl;
	icons::set_active(&menu[EDIT_MENU_CLEAR], tbuf->length());
	icons::set_active(&menu[EDIT_MENU_CUT], selected);
	icons::set_active(&menu[EDIT_MENU_COPY], selected);
	icons::set_active(&menu[EDIT_MENU_PASTE], true);
	icons::set_active(&menu[EDIT_MENU_READ], true);

	if (wrap)
		menu[EDIT_MENU_WRAP].set();
	else
		menu[EDIT_MENU_WRAP].clear();

	show_context_menu();
}

/// The context menu handler
///
/// @param val
///
void F_Edit::menu_cb(size_t item)
{
  	switch (item) {
	case EDIT_MENU_CLEAR:
		clear();
		break;
	case EDIT_MENU_CUT:
		kf_cut(0, this);
		break;
	case EDIT_MENU_COPY:
		kf_copy(0, this);
		break;
	case EDIT_MENU_PASTE:
		kf_paste(0, this);
		break;
	case EDIT_MENU_READ: {
		restore_wrap = wrap;
		set_word_wrap(false);
		readFile();
		break;
	}
	case EDIT_MENU_WRAP:
		set_word_wrap(!wrap, true);
		break;
	default:
		if (F_Edit::menu[item].flags == 0) { // not an FL_SUB_MENU
			add(F_Edit::menu[item].text[0]);
			add(F_Edit::menu[item].text[1]);
		}
	}
}

/// Overrides some useful Fl_Text_Edit keybindings that we want to keep working,
/// provided that they don't try to change chunks of transmitted text.
///
void F_Edit::change_keybindings(void)
{
	struct {
		Fl_Text_Editor_mod::Key_Func function, override;
	} fbind[] = {
		{ Fl_Text_Editor_mod::kf_default, F_Edit::kf_default },
		{ Fl_Text_Editor_mod::kf_enter,   F_Edit::kf_enter   },
		{ Fl_Text_Editor_mod::kf_delete,  F_Edit::kf_delete  },
		{ Fl_Text_Editor_mod::kf_cut,     F_Edit::kf_cut     },
		{ Fl_Text_Editor_mod::kf_paste,   F_Edit::kf_paste   }
	};
	int n = sizeof(fbind) / sizeof(fbind[0]);

	// walk the keybindings linked list and replace items containing
	// functions for which we have an override in fbind
	for (Fl_Text_Editor_mod::Key_Binding *k = key_bindings; k; k = k->next) {
		for (int i = 0; i < n; i++)
			if (fbind[i].function == k->function)
				k->function = fbind[i].override;
	}
}

// The kf_* functions below call the corresponding Fl_Text_Editor_mod routines, but
// may make adjustments so that no transmitted text is modified.

int F_Edit::kf_default(int c, Fl_Text_Editor_mod* e)
{
	return e->insert_position() < *p_editpos ? 1 : Fl_Text_Editor_mod::kf_default(c, e);
}

int F_Edit::kf_enter(int c, Fl_Text_Editor_mod* e)
{
	return e->insert_position() < *p_editpos ? 1 : Fl_Text_Editor_mod::kf_enter(c, e);
}

int F_Edit::kf_delete(int c, Fl_Text_Editor_mod* e)
{
	// single character
	if (!e->buffer()->selected()) {
                if (e->insert_position() >= *p_editpos &&
                    e->insert_position() != e->buffer()->length())
                        return Fl_Text_Editor_mod::kf_delete(c, e);
                else
                        return 1;
        }

	// region: delete as much as we can
	int start, end;
	e->buffer()->selection_position(&start, &end);
	if (*p_editpos >= end)
		return 1;
	if (*p_editpos > start)
		e->buffer()->select(*p_editpos, end);

	return Fl_Text_Editor_mod::kf_delete(c, e);
}

int F_Edit::kf_cut(int c, Fl_Text_Editor_mod* e)
{
	if (e->buffer()->selected()) {
		int start, end;
		e->buffer()->selection_position(&start, &end);
		if (*p_editpos >= end)
			return 1;
		if (*p_editpos > start)
			e->buffer()->select(*p_editpos, end);
	}

	return Fl_Text_Editor_mod::kf_cut(c, e);
}

int F_Edit::kf_paste(int c, Fl_Text_Editor_mod* e)
{
	return e->insert_position() < *p_editpos ? 1 : Fl_Text_Editor_mod::kf_paste(c, e);
}
