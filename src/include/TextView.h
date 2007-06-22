// ----------------------------------------------------------------------------
//
//	TextView.h
//
// Copyright (C) 2006
//		Dave Freese, W1HKJ
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

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>

#include "threads.h"

/* fltk includes */
#include <FL/Fl.H>
#include <FL/Enumerations.H>
#include <FL/Fl_Menu_Button.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Text_Editor.H>

using namespace std;

class TextView : public Fl_Text_Display
{
public:
	TextView(int x, int y, int w, int h, const char *l = 0);
	~TextView();

	int	handle(int event);
	void	add(char c, int attr = 0);
	void	add(const char *s, int attr = 0);
	void	clear(void);

	void	Show(void) { show(); }
	void	Hide(void) { hide(); }
	void	setFont(Fl_Font f) { setFont(-1, f); }
	void	setFontSize(int s) { setFontSize(-1, s); }
	void	setFontColor(Fl_Color c) { setFontColor(-1, c); }
	void	setFont(int n, Fl_Font f);
	void	setFontSize(int n, int s);
	void	setFontColor(int n, Fl_Color c);

public:
	enum TV_ATTR {RCV, XMT};
	enum { NSTYLES = 16 };

protected:
	enum { RX_MENU_CALL, RX_MENU_NAME, RX_MENU_QTH, RX_MENU_LOC,
	       RX_MENU_RST_IN, RX_MENU_DIV, RX_MENU_CLEAR, RX_MENU_COPY,
	       RX_MENU_SAVE, RX_MENU_WRAP };
	void	draw(void);
	void	menu_cb(int val);
	char	*get_word(int x, int y);
	void	saveFile(void);
	void	clipboard_copy(void);
	void	wrap_mode(bool v = true);

private:
	TextView();

protected:
	Fl_Text_Buffer *tbuf, *sbuf;
	static Fl_Text_Display::Style_Table_Entry styles[NSTYLES];
	int		popx, popy;
	bool		wrap;
};

class TextEdit : public Fl_Text_Editor
{
public:
	TextEdit(int x, int y, int w, int h, const char *l = 0);
	~TextEdit();

	int	handle(int event);
	void	add(const char *s, int attr = 1);
	void	clear(void);
	int	nextChar(void);

	void	setFont(int n, Fl_Font f);
	void	setFontSize(int n, int s);
	void	setFontColor(int n, Fl_Color c);
	void	setFont(Fl_Font f) { setFont(-1, f); }
	void	setFontSize(int s) { setFontSize(-1, s); }
	void	setFontColor(Fl_Color c) { setFontColor(-1, c); }
	void	cursorON(void);

public:
	enum { NSTYLES = 16 };

protected:
	enum { TX_MENU_TX, TX_MENU_RX, TX_MENU_MFSK16_IMG, TX_MENU_CLEAR,
	       TX_MENU_CUT, TX_MENU_COPY, TX_MENU_PASTE, TX_MENU_READ,
	       TX_MENU_WRAP };
	int	handle_key(int key);
	void	readFile(void);
	static void style_cb(int pos, int nins, int ndel, int nsty,
			     const char *dtext, void *arg);
	void	menu_cb(int val);
	void	wrap_mode(bool v = true);

	void	change_keybindings(void);
	static int kf_default(int c, Fl_Text_Editor* e);
	static int kf_enter(int c, Fl_Text_Editor* e);
	static int kf_delete(int c, Fl_Text_Editor* e);
	static int kf_cut(int c, Fl_Text_Editor* e);
	static int kf_paste(int c, Fl_Text_Editor* e);

private:
	TextEdit();

protected:
	Fl_Text_Buffer *tbuf, *sbuf;
	static Fl_Text_Display::Style_Table_Entry styles[NSTYLES];
	bool		PauseBreak;
	int		txpos;
	static int	*ptxpos;
	int		bkspaces;
	int		popx, popy;
	bool		wrap;
};

class autolock
{
public:
	autolock() : m(0) { Fl::lock(); }
	autolock(Fl_Mutex *m_) : m(m_) { fl_lock(m); }
	~autolock() { if (m) fl_unlock(m); else Fl::unlock(); }
private:
	autolock(const autolock &a);
	Fl_Mutex *m;
};

#endif
