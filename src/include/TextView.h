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
#include <FL/Fl_Text_Editor.H>

using namespace std;

// this interface is implemented by both rx,tx widgets
class ReceiveWidget : public Fl_Text_Editor
{
public:
	enum TEXT_ATTR { RECV, XMIT, CTRL, SKIP, ALTR, NATTR };

	ReceiveWidget(int x, int y, int w, int h, const char *label)
		: Fl_Text_Editor(x, y, w, h, label) { }
	virtual ~ReceiveWidget() { }

	virtual void add(const char *text, int attr = RECV) = 0;
	virtual void add(char c, int attr = RECV) = 0;
	void	     addstr(const char *text, int attr = RECV) { add(text, attr); }
	void	     addchr(char c, int attr = RECV) { add(c, attr); }

	virtual void clear(void) = 0;

	virtual void setFont(Fl_Font fnt) = 0;
	virtual void setFontSize(int siz) = 0;
	virtual void setFontColor(Fl_Color clr) = 0;

	virtual void Show(void) = 0;
	virtual void Hide(void) = 0;
};

// this is only implemented by the tx widget
class TransmitWidget : virtual public ReceiveWidget
{
public:
	TransmitWidget(int x, int y, int w, int h, const char *label)
		: ReceiveWidget(x, y, w, h, label) { }
	virtual ~TransmitWidget() { }

	virtual int nextChar(void) = 0;
};


class textview : virtual public ReceiveWidget
{
friend void processinput(void *);

public:
enum CURSOR_TYPE {CARET_CURSOR, NORMAL_CURSOR, HEAVY_CURSOR, DIM_CURSOR, BLOCK_CURSOR, NONE};
protected:
    string		buff;
    string		attr;
    string		inbuff;
    string		inattr;
    string      selword;
    int			nlines;
	int			wrappos;
    int			charwidth;
	int			maxcharwidth;
    int			charheight;
    int			descent;
	int			cursorX;
	int			cursorY;
	int			X;
	int			Y;
	int			W;
	int			H;
	int			xpos;
	int			ypos;
	size_t		laststartidx;
	size_t		endidx;
	size_t      startidx;
	size_t      xmtidx;
    size_t      highlightstart;
    size_t      highlightend;
	int			popx;
	int			popy;
	int			cursorwidth;
	bool		cursorON;
	bool		wordwrap;
	bool		inprocess;
	bool		timerstarted;
	
	CURSOR_TYPE	cursorStyle;
	Fl_Font		TextFont;
	Fl_Color	TextColor[16];
	int			TextSize;
	char		cstr[1000];
public:

	textview( int x, int y, int w, int h, const char *label = 0 );
	virtual ~textview() = 0;
	virtual int handle(int event);
	virtual void resize( int x, int y, int w, int h );
    void    draw_cursor();
    void    draw();
    void    drawall();
    void    drawchars();
    void    drawmodify(size_t modidx);
    void    drawbs();
    void    Show();
	void	Hide();

	virtual void add( const char *text, int attr = RECV );
	virtual void add( char c, int attr = RECV);
	virtual void clear();
	
	virtual void setFont(Fl_Font fnt);
	virtual void setFontSize(int siz);
	virtual void setFontColor(Fl_Color clr);

	inline void setTextStyle(int n, Fl_Color c )
		{ if (n < 0 || n > 15) return;      
		TextColor[n] = c; 
	}

protected:
	Fl_Scrollbar scrollbar;
	Fl_Menu_Button *mpopup;

	void add_( char c, int attr );
	void scrollbarCB();	
	inline static void _scrollbarCB( Fl_Widget* w, void* arg )
	{ 
		((textview*)arg)->scrollbarCB(); 
	}
	
	int lineCount();
	size_t linePosition(int linenbr);

	void _backspace();
	void setScrollbar();
	void rebuildsoft(int w);
    size_t xy2bufidx();
	string findtext();
    void    highlightword();
    void    highlight(bool b);

        struct modify_range {
                size_t start, end;
                modify_range() : start(0), end(0) { }
        } draw_mod_range;
};


class TextView : public textview {
public:
	TextView( int x, int y, int w, int h, const char *label = 0 );

protected:
	static Fl_Menu_Item viewmenu[];
	int		handle (int event);
	void	menu_cb(int val);
	void	saveFile();
};


class TextEdit : public textview, public TransmitWidget {
public:
	TextEdit( int x, int y, int w, int h, const char *label = 0 );
	virtual void clear();
	int		nextChar();
	void	readFile();	
	virtual void setFont(Fl_Font fnt) { 
		textview::setFont(fnt); 
	}
	virtual void setFontSize(int siz) { 
		textview::setFontSize(siz); 
	}
	virtual void setFontColor(Fl_Color clr) { 
		textview::setFontColor(clr); 
	}
	void	cursorON();
protected:
	int 	handle_fnckey(int key);
	int		handle_key();
	int		handle (int event);
	void	menu_cb(int val);
	void	update_xmit_text(size_t i);
private:
	int	bkspaces;
	bool PauseBreak;
};

#endif
