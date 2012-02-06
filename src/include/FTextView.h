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

#ifndef FTextView_H_
#define FTextView_H_

#include <stddef.h>
#include <string>

#include <FL/Fl.H>
#include <FL/Enumerations.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Tile.H>

#include "Fl_Text_Editor_mod.H"

///
/// The text widgets base class.
/// This class implements a basic text editing widget based on Fl_Text_Editor_mod.
///
class FTextBase : public Fl_Text_Editor_mod
{
public:
	// CLICK_START: same as first clickable style
	// NATTR: number of styles (last style + 1)
	enum TEXT_ATTR { RECV, XMIT, CTRL, SKIP, ALTR,
			 CLICK_START, QSY = CLICK_START, /* FOO, BAR, ..., */
			 NATTR };

	FTextBase(int x, int y, int w, int h, const char *l = 0);
	virtual ~FTextBase() { delete tbuf; delete sbuf; }

	virtual void	add(unsigned char c, int attr = RECV);
	virtual void	add(const char *text, int attr = RECV);
	void	     	addstr(std::string text, int attr = RECV) { add(text.c_str(), attr); }
	void	     	addchr(unsigned char c, int attr = RECV) { add(c, attr); }

	virtual int	handle(int event);
	virtual void	handle_context_menu(void) { }
	virtual void	clear(void);//{ tbuf->text(""); sbuf->text(""); }

	void		set_word_wrap(bool b);
	bool		get_word_wrap(void) { return wrap; }

	virtual void	setFont(Fl_Font f, int attr = NATTR);
	void		setFontSize(int s, int attr = NATTR);
	void		setFontColor(Fl_Color c, int attr = NATTR);
	// Override Fl_Text_Display, which stores the font number in an unsigned
	// character and therefore cannot represent all fonts
	Fl_Font		textfont(void) { return styles[0].font; }
	void		textfont(Fl_Font f) { setFont(f); }
	void		textfont(uchar s) { textfont((Fl_Font)s); }

	void		cursorON(void) { show_cursor(); }
	virtual void	resize(int X, int Y, int W, int H);

	static bool	wheight_mult_tsize(void *arg, int xd, int yd);

protected:
	void		set_style(int attr, Fl_Font f, int s, Fl_Color c,
				  int set = SET_FONT | SET_SIZE | SET_COLOR);
	int		readFile(const char* fn = 0);
	void		saveFile(void);
	char*		get_word(int x, int y, const char* nwchars = "", bool ontext = true);
	void		init_context_menu(void);
	void		show_context_menu(void);
	virtual void	menu_cb(size_t item) { }
	int		reset_wrap_col(void);
        void		reset_styles(int set);
private:
	FTextBase();
	FTextBase(const FTextBase &t);

protected:
	enum { FTEXT_DEF = 'A' };
	enum set_style_op_e { SET_FONT = 1 << 0, SET_SIZE = 1 << 1, SET_COLOR = 1 << 2 };
	Fl_Text_Buffer_mod			*tbuf;	///< text buffer
	Fl_Text_Buffer_mod			*sbuf;	///< style buffer
	Fl_Text_Display_mod::Style_Table_Entry	styles[NATTR];
	Fl_Menu_Item				*context_menu;
	int					popx, popy;
	bool					wrap;
	int					wrap_col;
	int					max_lines;
	bool					scroll_hint;
	bool	restore_wrap;
//	bool	wrap_restore;

private:
	int					oldw, oldh, olds;
	Fl_Font					oldf;
};

///
/// A TextBase subclass to display received & transmitted text
///
class FTextView : public FTextBase
{
public:
	FTextView(int x, int y, int w, int h, const char *l = 0);
        ~FTextView() { }

	virtual int	handle(int event);

protected:
	enum {
		VIEW_MENU_COPY, VIEW_MENU_CLEAR, VIEW_MENU_SELECT_ALL,
		VIEW_MENU_SAVE, VIEW_MENU_WRAP
	};

	virtual void	handle_context_menu(void);
	virtual void	menu_cb(size_t item);
	static void	changed_cb(int pos, int nins, int ndel, int nsty,
				   const char *dtext, void *arg);
	void		change_keybindings(void);

private:
	FTextView();
	FTextView(const FTextView &t);

protected:
	static Fl_Menu_Item menu[];
	bool quick_entry;
};


///
/// A FTextBase subclass to display and edit text to be transmitted
///
class FTextEdit : public FTextBase
{
public:
	FTextEdit(int x, int y, int w, int h, const char *l = 0);

	virtual int	handle(int event);

protected:
	enum {
		EDIT_MENU_CUT, EDIT_MENU_COPY, EDIT_MENU_PASTE, EDIT_MENU_CLEAR,
		EDIT_MENU_READ, EDIT_MENU_WRAP
	};
	virtual int	handle_key(int key);
	int		handle_key_ascii(int key);
	virtual int	handle_dnd_drag(int pos);
	int		handle_dnd_drop(void);
	virtual void	handle_context_menu(void);
	virtual void	menu_cb(size_t item);
	static void	changed_cb(int pos, int nins, int ndel, int nsty,
				   const char *dtext, void *arg);

private:
	FTextEdit();
	FTextEdit(const FTextEdit &t);

protected:
	static Fl_Menu_Item	menu[];
	char			ascii_cnt;
	unsigned		ascii_chr;
	bool			dnd_paste;
};

#endif // FTextView_H_

// Local Variables:
// mode: c++
// c-file-style: "linux"
// End:
