// ----------------------------------------------------------------------------
//      FTextView.h
//
// Copyright (C) 2007-2009
//              Stelios Bounanos, M0GLD
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

#ifndef F_EDIT_H_
#define F_EDIT_H_

#include <stddef.h>
#include <string>

#include <FL/Fl.H>
#include <FL/Enumerations.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Tile.H>

#include "FTextView.h"

/// A FTextBase subclass to display and edit text

class F_Edit : public FTextEdit
{
public:
	F_Edit(int x, int y, int w, int h, const char *l = 0);

	virtual int	handle(int event);

	void		clear(void);
	void		add_text(std::string s);

	void		setFont(Fl_Font f, int attr = NATTR);

protected:
	enum {
		EDIT_MENU_CUT, EDIT_MENU_COPY, EDIT_MENU_PASTE,
		EDIT_MENU_CLEAR, EDIT_MENU_READ, EDIT_MENU_WRAP
	};
	int			handle_key(int key);
	int			handle_dnd_drag(int pos);
	void		handle_context_menu(void);
	void		menu_cb(size_t item);
	void		change_keybindings(void);
	static int	kf_default(int c, Fl_Text_Editor_mod* e);
	static int	kf_enter(int c, Fl_Text_Editor_mod* e);
	static int	kf_delete(int c, Fl_Text_Editor_mod* e);
	static int	kf_cut(int c, Fl_Text_Editor_mod* e);
	static int	kf_paste(int c, Fl_Text_Editor_mod* e);

private:
	F_Edit();
	F_Edit(const F_Edit &t);

protected:
	static		Fl_Menu_Item	menu[];
	int			editpos;
	int			bkspaces;
	static int	*p_editpos;
};

#endif // F_EDIT_H_

// Local Variables:
// mode: c++
// c-file-style: "linux"
// End:
