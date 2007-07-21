// ----------------------------------------------------------------------------
//
//	TextView.h
//
// Copyright (C) 2006
//		Dave Freese, W1HKJ
//
// Copyright (C) 2007
//		Stelios Bounanos, 2E0DLX
//
// This file is part of fldigi.
//
// fldigi is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307	 USA
// ----------------------------------------------------------------------------

#ifndef _TextView_H
#define _TextView_H

#include "threads.h"

/* fltk includes */
#include <FL/Fl.H>
#include <FL/Enumerations.H>
#include <FL/Fl_Menu_Button.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Text_Editor.H>


///
/// The text widgets base class.
/// This class implements a basic text editing widget based on Fl_Text_Editor.
///
class TextBase : public Fl_Text_Editor
{
public:

	///
	/// Text styles used for highlighting
	///
	enum text_attr_e {
		DEFAULT = 'A',	///< Default text style
		RCV,		///< Received text style
		XMT,		///< Transmitted text style
		SKIP,		///< Skipped text style
		CTRL,		///< Control character style
		NSTYLES = 5
	};
	typedef enum text_attr_e text_attr_t;

public:
	TextBase(int x, int y, int w, int h, const char *l = 0);
	virtual ~TextBase() { delete tbuf; delete sbuf; }

	virtual int	handle(int event) { return Fl_Text_Editor::handle(event); };
	virtual void	add(const char *s, text_attr_t attr = DEFAULT) { insert(s); }
	void		clear(void) { tbuf->text(""); sbuf->text(""); }

	void		setFont(Fl_Font f, text_attr_t attr = NSTYLES);
	void		setFontSize(int s, text_attr_t attr = NSTYLES);
	void		setFontColor(Fl_Color c, text_attr_t attr = NSTYLES);

	void		cursorON(void) { show_cursor(); }
	virtual void	resize(int X, int Y, int W, int H);

protected:
	void		set_style(text_attr_t attr, Fl_Font f, int s, Fl_Color c,
				  int set = SET_FONT | SET_SIZE | SET_COLOR);
	void		readFile(void);
	void		saveFile(void);
	char		*get_word(int x, int y);
	void		show_context_menu(void);
	virtual void	menu_cb(int val) { }
	int		reset_wrap_col(void);

private:
	TextBase();
	TextBase(const TextBase &t);

protected:
	enum set_style_op_e { SET_FONT = 1 << 0, SET_SIZE = 1 << 1, SET_COLOR = 1 << 2 };
	enum { RESIZING = 1 << 0 };
	Fl_Text_Buffer				*tbuf;	///< text buffer
	Fl_Text_Buffer				*sbuf;	///< style buffer
	Fl_Text_Display::Style_Table_Entry	styles[NSTYLES];
	Fl_Menu_Item				*context_menu;
	int					popx, popy;
	bool					wrap;
	int					wrap_col;
	int					max_lines;
};

///
/// A TextBase subclass to display received & transmitted text
///
class TextView : public TextBase
{
public:
	TextView(int x, int y, int w, int h, const char *l = 0);

	virtual int	handle(int event);
	virtual void	add(char c, text_attr_t attr = DEFAULT);
	virtual	void	add(const char *s, text_attr_t attr = DEFAULT);

protected:
	enum { RX_MENU_QRZ_THIS, RX_MENU_CALL, RX_MENU_NAME, RX_MENU_QTH,
	       RX_MENU_LOC, RX_MENU_RST_IN, RX_MENU_DIV, RX_MENU_CLEAR,
	       RX_MENU_COPY, RX_MENU_SAVE, RX_MENU_WRAP };

	virtual void	menu_cb(int val);
	static void	changed_cb(int pos, int nins, int ndel, int nsty,
				   const char *dtext, void *arg);
	void		change_keybindings(void);

private:
	TextView();
	TextView(const TextView &t);

protected:
	static Fl_Menu_Item view_menu[];
};


///
/// A TextBase subclass to display and edit text to be transmitted
///
class TextEdit : public TextBase
{
public:
	TextEdit(int x, int y, int w, int h, const char *l = 0);

	virtual int	handle(int event);
	virtual void	add(const char *s, text_attr_t attr = DEFAULT);
	void		clear(void);
	int		nextChar(void);

protected:
	enum { TX_MENU_TX, TX_MENU_RX, TX_MENU_MFSK16_IMG, TX_MENU_CLEAR,
	       TX_MENU_CUT, TX_MENU_COPY, TX_MENU_PASTE, TX_MENU_READ,
	       TX_MENU_WRAP
	};
	int		handle_key(int key);
	virtual void	menu_cb(int val);
	static void	changed_cb(int pos, int nins, int ndel, int nsty,
				   const char *dtext, void *arg);
	void		change_keybindings(void);
	static int	kf_default(int c, Fl_Text_Editor* e);
	static int	kf_enter(int c, Fl_Text_Editor* e);
	static int	kf_delete(int c, Fl_Text_Editor* e);
	static int	kf_cut(int c, Fl_Text_Editor* e);
	static int	kf_paste(int c, Fl_Text_Editor* e);

private:
	TextEdit();
	TextEdit(const TextEdit &t);

protected:
	static Fl_Menu_Item	edit_menu[];
	bool			PauseBreak;
	int			txpos;
	static int		*ptxpos;
	int			bkspaces;
};

///
/// A lock class meant to be instantiated on the stack to acquire a lock which
/// is released when the object goes out of scope.
/// The no-arg ctor calls Fl::lock(), and the Fl_Mutex ctor locks that mutex.
///
class autolock
{
public:
	autolock() : m(0) { Fl::lock(); }
	autolock(Fl_Mutex *m_) : m(m_) { fl_lock(m); }
	~autolock() { if (m) fl_unlock(m); else Fl::unlock(); }
private:
	autolock(const autolock &a); // no copying
	Fl_Mutex *m;
};

#endif
