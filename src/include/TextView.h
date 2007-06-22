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
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// ----------------------------------------------------------------------------

#ifndef _TextView_H
#define _TextView_H

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>

/* fltk includes */
#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Scrollbar.H>
#include <FL/Enumerations.H>
#include <FL/Fl_Menu_Button.H>
#include <FL/Fl_Menu_Item.H>

using namespace std;

class textview : public Fl_Widget
{
public:
enum TV_ATTR {RCV, XMT};
enum CURSOR_TYPE {CARET_CURSOR, NORMAL_CURSOR, HEAVY_CURSOR, DIM_CURSOR, BLOCK_CURSOR};
protected:
    string		buff;
    string		attr;
    int			nlines;
	int			wrappos;
    int			charwidth;
	int			maxcharwidth;
    int			charheight;
	int			cursorX;
	int			cursorY;
	int			startidx;
	int			popx;
	int			popy;
	bool		cursorON;
	bool		wordwrap;
	CURSOR_TYPE	cursorStyle;
	Fl_Font		TextFont;
	Fl_Color	TextColor[16];
	int			TextSize;
public:

	textview( int x, int y, int w, int h, const char *label = 0 );
	virtual ~textview() = 0;
	virtual int handle(int event);
	virtual void resize( int x, int y, int w, int h );
	void	draw_cursor();
	virtual void draw();
	void	Show();
	void	Hide();

	virtual void add( char *text, int attr = 1 );
	virtual void add( char c, int attr = 1);
	virtual void clear();
	virtual string findtext();
	
	virtual void setFont(Fl_Font fnt);
	virtual void setFontSize(int siz);
	virtual void setFontColor(Fl_Color clr);

	inline void setTextStyle(int n, Fl_Color c )
		{ if (n < 0 || n > 15) return;      
		TextColor[n] = c; 
	}

protected:
	Fl_Scrollbar *scrollbar;
	Fl_Menu_Button *mpopup;

	void scrollbarCB();	
	inline static void _scrollbarCB( Fl_Widget* w, void* arg )
	{ 
		((textview*)arg)->scrollbarCB(); 
	}
	
	int lineCount();
	void _backspace();
	void _add(char c, int attribute);
	void setScrollbar();
};


class TextView : public textview {
public:
	TextView( int x, int y, int w, int h, const char *label = 0 );
	virtual void add( char *text, int attr = 1 ) {textview::add(text, attr);};
	virtual void add( char c, int attr = 1) {textview::add(c, attr);};
	virtual void clear() {textview::clear();};
	virtual void setFont(Fl_Font fnt) { textview::setFont(fnt); }
	virtual void setFontSize(int siz) { textview::setFontSize(siz); }
	virtual void setFontColor(Fl_Color clr) { textview::setFontColor(clr); }
protected:
	int		handle (int event);
	void	menu_cb(int val);
	void	saveFile();
};


class TextEdit : public textview {
public:
	TextEdit( int x, int y, int w, int h, const char *label = 0 );
	virtual void add( char *text, int attr = 1 ) {textview::add(text);};
	virtual void add( const char *text, int attr = 1) {textview::add((char*)text);};
	virtual void add( char c, int attr = 1) {textview::add(c);};
	virtual void clear();
	int		nextChar();
	void	readFile();	
	virtual void setFont(Fl_Font fnt) { textview::setFont(fnt); }
	virtual void setFontSize(int siz) { textview::setFontSize(siz); }
	virtual void setFontColor(Fl_Color clr) { textview::setFontColor(clr); }
	void	cursorON();
protected:
	int 	handle_fnckey(int key);
	int		handle_key();
	int		handle (int event);
	void	menu_cb(int val);
private:
	unsigned int chrptr;
	int	bkspaces;
	bool PauseBreak;
};

#endif
