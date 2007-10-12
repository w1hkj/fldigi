// ----------------------------------------------------------------------------
//      FTextView.h
//
// Copyright (C) 2007
//              Stelios Bounanos, M0GLD
//
// Interface based on the text widgets by Dave Freese, W1HKJ.
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

#include "threads.h"
#include "TextView.h"

/* fltk includes */
#include <FL/Fl.H>
#include <FL/Enumerations.H>
#include <FL/Fl_Menu_Button.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/Fl_Tile.H>

#if (FL_MAJOR_VERSION == 1 && FL_MINOR_VERSION == 1 &&		\
     (FL_PATCH_VERSION == 7 || FL_PATCH_VERSION == 8)) &&	\
	!defined(NO_HSCROLLBAR_KLUDGE)
#	define HSCROLLBAR_KLUDGE
#else
#	ifndef NO_HSCROLLBAR_KLUDGE
#		warning "Not suppressing horizontal scrollbars with this version of fltk"
#	endif
#	undef HSCROLLBAR_KLUDGE
#endif

///
/// The text widgets base class.
/// This class implements a basic text editing widget based on Fl_Text_Editor.
///
class FTextBase : virtual public ReceiveWidget
{
public:
	FTextBase(int x, int y, int w, int h, const char *l = 0);
	virtual ~FTextBase() { delete tbuf; delete sbuf; }

	virtual int	handle(int event);
	void		clear(void) { tbuf->text(""); sbuf->text(""); }

	void		setFont(Fl_Font f, int attr = NATTR);
	void		setFontSize(int s, int attr = NATTR);
	void		setFontColor(Fl_Color c, int attr = NATTR);
	virtual void	setFont(Fl_Font f)       { setFont(f, NATTR); }
	virtual void	setFontSize(int s)       { setFontSize(s, NATTR); }
	virtual void	setFontColor(Fl_Color c) { setFontColor(c, NATTR); }

	void		cursorON(void) { show_cursor(); }
	virtual void	resize(int X, int Y, int W, int H);

        virtual void	Show(void) { Fl_Text_Editor::show(); }
        virtual void	Hide(void) { Fl_Text_Editor::hide(); }
	static bool	wheight_mult_tsize(void *arg, int xd, int yd);

protected:
	void		set_style(int attr, Fl_Font f, int s, Fl_Color c,
				  int set = SET_FONT | SET_SIZE | SET_COLOR);
	void		readFile(void);
	void		saveFile(void);
	char		*get_word(int x, int y);
	void		show_context_menu(void);
	virtual void	menu_cb(int val) { }
	int		reset_wrap_col(void);
#ifdef HSCROLLBAR_KLUDGE
        void		scroll_(int topLineNum, int horizOffset);
#endif
        void		adjust_colours(void);
private:
	FTextBase();
	FTextBase(const FTextBase &t);

protected:
	enum { FTEXT_DEF = 'A' };
	enum set_style_op_e { SET_FONT = 1 << 0, SET_SIZE = 1 << 1, SET_COLOR = 1 << 2 };
	Fl_Text_Buffer				*tbuf;	///< text buffer
	Fl_Text_Buffer				*sbuf;	///< style buffer
	Fl_Text_Display::Style_Table_Entry	styles[ReceiveWidget::NATTR];
	Fl_Menu_Item				*context_menu;
	int					popx, popy;
	bool					wrap;
	int					wrap_col;
	int					max_lines;
	bool					scroll_hint;
	int					scroll_tweak;
	bool					adjusted_colours;
};

///
/// A TextBase subclass to display received & transmitted text
///
class FTextView : public FTextBase
{
public:
	FTextView(int x, int y, int w, int h, const char *l = 0);
        ~FTextView();

	virtual int	handle(int event);
	virtual void	add(char c, int attr = RECV);
	virtual	void	add(const char *s, int attr = RECV)
        {
                while (*s)
                        add(*s++, attr);
        }

protected:
	enum { RX_MENU_QRZ_THIS, RX_MENU_CALL, RX_MENU_NAME, RX_MENU_QTH,
	       RX_MENU_LOC, RX_MENU_RST_IN, RX_MENU_DIV, RX_MENU_CLEAR,
	       RX_MENU_COPY,
#ifndef NDEBUG
               RX_MENU_READ,
#endif
               RX_MENU_SAVE, RX_MENU_WRAP };

	virtual void	menu_cb(int val);
	static void	changed_cb(int pos, int nins, int ndel, int nsty,
				   const char *dtext, void *arg);
	void		change_keybindings(void);

private:
	FTextView();
	FTextView(const FTextView &t);

protected:
	static Fl_Menu_Item view_menu[];
};


///
/// A FTextBase subclass to display and edit text to be transmitted
///
class FTextEdit : public FTextBase, public TransmitWidget
{
public:
	FTextEdit(int x, int y, int w, int h, const char *l = 0);

	virtual int	handle(int event);

	virtual void	add(const char *s, int attr = RECV);
	virtual void	add(char c, int attr = RECV);
	void		clear(void);
	int		nextChar(void);

protected:
	enum { TX_MENU_TX, TX_MENU_RX, TX_MENU_MFSK16_IMG, TX_MENU_CLEAR,
	       TX_MENU_CUT, TX_MENU_COPY, TX_MENU_PASTE, TX_MENU_READ,
	       TX_MENU_WRAP
	};
	int		handle_key(int key);
	int		handle_key_macro(int key);
	int		handle_key_ascii(int key);
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
	FTextEdit();
	FTextEdit(const FTextEdit &t);

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
/// The no-arg ctor calls Fl::lock(), and the Fl_Mutex* ctor locks that mutex.
///
class autolock
{
public:
	autolock() : m(0) { FL_LOCK(); }
	autolock(Fl_Mutex *m_) : m(m_) { fl_lock(m); }
	~autolock() { if (m) fl_unlock(m); else FL_UNLOCK(); }
private:
	autolock(const autolock &a); // no copying
        autolock& operator=(const autolock&); // no copying
	Fl_Mutex *m;
};

/// A version of Fl_Tile that runs check callbacks and moves the boundary
/// between its child widgets only all resize checks return true.
class Fl_Tile_check : public Fl_Tile
{
public:
	typedef bool (*resize_check_func)(void *, int, int);
	Fl_Tile_check(int x, int y, int w, int h, const char *l = 0)
		: Fl_Tile(x, y, w, h, l) { remove_checks(); }

	int handle(int event)
	{
		switch (event) {
		case FL_DRAG: case FL_RELEASE:
			if (!do_checks(Fl::event_x() - xstart, Fl::event_y() - ystart))
				return 1;
			// fall through to reset [xy]start
		case FL_PUSH:
			xstart = Fl::event_x();
			ystart = Fl::event_y();
		}

		return Fl_Tile::handle(event);
	}

	void add_resize_check(resize_check_func f, void *a)
	{
		for (size_t i = 0; i < sizeof(resize_checks) / sizeof(resize_checks[0]); i++) {
			if (resize_checks[i] == 0) {
				resize_checks[i] = f;
				resize_args[i] = a;
				break;
			}
		}
	}
	void remove_resize_check(resize_check_func f, void *a)
	{
		for (size_t i = 0; i < sizeof(resize_checks) / sizeof(resize_checks[0]); i++)
			if (resize_checks[i] == f && resize_args[i] == a)
				resize_checks[i] = 0;
	}
	void remove_checks(void)
	{
		for (size_t i = 0; i < sizeof(resize_checks) / sizeof(resize_checks[0]); i++) {
			resize_checks[i] = 0;
			resize_args[i] = 0;
		}
	}
	bool do_checks(int xd, int yd)
	{
		for (size_t i = 0; i < sizeof(resize_checks) / sizeof(resize_checks[0]); i++)
			if (resize_checks[i] && !resize_checks[i](resize_args[i], xd, yd))
				return false;
		return true;
	}
protected:
	int xstart, ystart;
	resize_check_func resize_checks[8];
	void *resize_args[8];
};

#endif // FTextView_H_

// Local Variables:
// mode: c++
// c-file-style: "linux"
// End:
