// ----------------------------------------------------------------------------
//
//	TextView.cxx
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

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>

#include "TextView.h"
#include "main.h"

#include "macros.h"
#include "main.h"

#include <FL/Enumerations.H>
#include "File_Selector.h"

using namespace std;

//=====================================================================
// class textview
// a virtual base class for building either a text viewer or editor
// you cannot instantiate this class by itself
// only as a base for child classes
//
// text is stored in <string> arrays
// the attribute of each character is mirrored in another <string> array
//=====================================================================

textview :: textview( int x, int y, int w, int h, const char *label )
  : Fl_Widget( x, y, w, h, label )
{
	scrollbar = new Fl_Scrollbar( x+w-20, y+2, 20, h-4 );
	scrollbar->linesize( 1 );
	scrollbar->callback( _scrollbarCB, this );

	mitems = 0;
	mpopup = (Fl_Menu_Button *)0;

	box( FL_DOWN_BOX );
	color( FL_WHITE );

	TextFont = FL_SCREEN;
	TextSize = FL_NORMAL_SIZE;
	
	for (int i = 0; i < 16; i++) {
		TextColor[i] = FL_BLACK;
	}
	TextColor[2] = FL_BLUE;
	TextColor[3] = FL_GREEN;
	TextColor[4] = FL_RED;
	
	wrappos = 0;
	cursorON = false;
	startidx = 0;
	
	clear();
}

textview :: ~textview()
{
	delete scrollbar;
}

void textview::Show() {
	scrollbar->show();
	show();
}

void textview::Hide() {
	scrollbar->hide();
	hide();
}

int textview::handle(int event)
{
	return 0;
}

void textview::setFont(Fl_Font fnt)
{
	Fl::lock();
	TextFont = fnt;
	redraw();
	Fl::unlock();
	Fl::awake();
}

void textview::setFontSize(int siz)
{
	Fl::lock();
	TextSize = siz;
	redraw();
	Fl::unlock();
	Fl::awake();
}

void textview::setFontColor(Fl_Color clr)
{
}

int textview::lineCount()
{
	int cnt = 1;
	unsigned long int len = buff.length(); // long int needed for 64 bit cpu's
	unsigned long int indx = 0;
	if (len == 0) return 0;
	while ((indx = buff.find('\n', indx)) != string::npos) {
		cnt++;
		indx++;
		if (indx == len) break;
	}
	return cnt;
}

void textview::draw_cursor()
{
  int X = cursorX, Y = cursorY;
  
  typedef struct {
    int x1, y1, x2, y2;
  }
  Segment;

  Segment segs[ 5 ];
  int left, right, cursorWidth, midY;
  int nSegs = 0;
  int bot = Y  - charheight + 1;

  /* For cursors other than the block, make them around 2/3 of a character
     width, rounded to an even number of pixels so that X will draw an
     odd number centered on the stem at x. */
  cursorWidth = 4;
  X += cursorWidth/2;
  left = X - cursorWidth/2;
  right = left + cursorWidth;

  if (cursorON == false) {
	fl_color(FL_WHITE);
	fl_rectf ( X, Y - charheight, cursorWidth, charheight );
	return;
  }

  /* Create segments and draw cursor */
  if ( cursorStyle == CARET_CURSOR ) {
    midY = bot - charheight / 5;
    segs[ 0 ].x1 = left; segs[ 0 ].y1 = bot; segs[ 0 ].x2 = X; segs[ 0 ].y2 = midY;
    segs[ 1 ].x1 = X; segs[ 1 ].y1 = midY; segs[ 1 ].x2 = right; segs[ 1 ].y2 = bot;
    segs[ 2 ].x1 = left; segs[ 2 ].y1 = bot; segs[ 2 ].x2 = X; segs[ 2 ].y2 = midY - 1;
    segs[ 3 ].x1 = X; segs[ 3 ].y1 = midY - 1; segs[ 3 ].x2 = right; segs[ 3 ].y2 = bot;
    nSegs = 4;
  } else if ( cursorStyle == NORMAL_CURSOR ) {
    segs[ 0 ].x1 = left; segs[ 0 ].y1 = Y; segs[ 0 ].x2 = right; segs[ 0 ].y2 = Y;
    segs[ 1 ].x1 = X; segs[ 1 ].y1 = Y; segs[ 1 ].x2 = X; segs[ 1 ].y2 = bot;
    segs[ 2 ].x1 = left; segs[ 2 ].y1 = bot; segs[ 2 ].x2 = right; segs[ 2 ].y2 = bot;
    nSegs = 3;
  } else if ( cursorStyle == HEAVY_CURSOR ) {
    segs[ 0 ].x1 = X - 1; segs[ 0 ].y1 = Y; segs[ 0 ].x2 = X - 1; segs[ 0 ].y2 = bot;
    segs[ 1 ].x1 = X; segs[ 1 ].y1 = Y; segs[ 1 ].x2 = X; segs[ 1 ].y2 = bot;
    segs[ 2 ].x1 = X + 1; segs[ 2 ].y1 = Y; segs[ 2 ].x2 = X + 1; segs[ 2 ].y2 = bot;
    segs[ 3 ].x1 = left; segs[ 3 ].y1 = Y; segs[ 3 ].x2 = right; segs[ 3 ].y2 = Y;
    segs[ 4 ].x1 = left; segs[ 4 ].y1 = bot; segs[ 4 ].x2 = right; segs[ 4 ].y2 = bot;
    nSegs = 5;
  } else if ( cursorStyle == DIM_CURSOR ) {
    midY = Y + charheight / 2;
    segs[ 0 ].x1 = X; segs[ 0 ].y1 = Y; segs[ 0 ].x2 = X; segs[ 0 ].y2 = Y;
    segs[ 1 ].x1 = X; segs[ 1 ].y1 = midY; segs[ 1 ].x2 = X; segs[ 1 ].y2 = midY;
    segs[ 2 ].x1 = X; segs[ 2 ].y1 = bot; segs[ 2 ].x2 = X; segs[ 2 ].y2 = bot;
    nSegs = 3;
  } else if ( cursorStyle == BLOCK_CURSOR ) {
    right = X + maxcharwidth;
    segs[ 0 ].x1 = X; segs[ 0 ].y1 = Y; segs[ 0 ].x2 = right; segs[ 0 ].y2 = Y;
    segs[ 1 ].x1 = right; segs[ 1 ].y1 = Y; segs[ 1 ].x2 = right; segs[ 1 ].y2 = bot;
    segs[ 2 ].x1 = right; segs[ 2 ].y1 = bot; segs[ 2 ].x2 = X; segs[ 2 ].y2 = bot;
    segs[ 3 ].x1 = X; segs[ 3 ].y1 = bot; segs[ 3 ].x2 = X; segs[ 3 ].y2 = Y;
    nSegs = 4;
  }
  fl_color( FL_BLACK );

  for ( int k = 0; k < nSegs; k++ ) {
    fl_line( segs[ k ].x1, segs[ k ].y1, segs[ k ].x2, segs[ k ].y2 );
  }
}

string textview::findtext()
{
	unsigned long idx, wordstart, wordend;
	idx = startidx;
	int xc = 0, yc = 0;
	char c;
	unsigned long len = buff.length();
	static string selword;


	fl_font(TextFont, TextSize);
	selword = "";
	if (!len) return selword;
	while(idx < len) {
		if (yc > h()-4) 
			break;
		while (idx < len ) {
			c = buff[idx];
			if (c == '\n') {
				xc = 0;
				yc += fl_height();
				break;
			}
			xc += (int)(fl_width(c) + 0.5);
			idx++;
			if (	(yc < popy && yc > popy - charheight) &&
					(xc >= popx) ) {
				wordstart = buff.find_last_of(" \n", idx);
				if (wordstart == string::npos) wordstart = 0;
				wordend = buff.find_first_of(" ,\n", idx);
				if (wordend == string::npos) wordend = len;
				if (wordstart && wordend)
					selword = buff.substr(wordstart+1, wordend - wordstart - 1);
				else if (!wordstart && wordend)
					selword = buff.substr(0, wordend);
				return selword;
			}
		}
		idx++;
	}
	return selword;
}

void textview::draw()
{
// draw the background box
	draw_box();

// resize the scrollbar to be a constant width
	scrollbar->resize( x()+w()-20, y()+2, 20, h()-4 );
	scrollbar->redraw();

	int line = 0;
	unsigned int idx = 0;
	unsigned int xpos = 0;
	unsigned int ypos = 0;
	unsigned int len = 0;
	char c = 0;
	char cstr[] = " ";
	unsigned int	H = h() - 4, 
					W = w() - 4 - 20, 
					X = x() + 2, 
					Y = y() + 2;
  
	fl_font(TextFont, TextSize);
	charheight = fl_height();
	maxcharwidth = (int)fl_width('X');

	if (buff.length() == 0) {
		fl_push_clip( X, Y, W, H);
		cursorX = X; cursorY = Y + charheight;
		draw_cursor();
		fl_pop_clip();
    	return;
	}

	nlines = lineCount();
	line = nlines - H / charheight - scrollbar->value();
	
	startidx = 0;
	if (line > 0) {
		while (line) {
			startidx = buff.find('\n', startidx) + 1;
			line--;
		}
	}

	len = buff.length();
	  
	fl_push_clip( X, Y, W, H );
	
	fl_font(TextFont, TextSize);
	xpos = 0;
	ypos = fl_height();
	idx = startidx;
	while(idx < len) {
		if (ypos > H) 
			break;
		while (idx < len ) { //&& (c = buff[idx]) != '\n') {
			c = buff[idx];
			if (c == '\n') {
				xpos = 0;
				ypos += fl_height();
				break;
			}
			cstr[0] = c;
			fl_color (TextColor[(int)attr[idx]]);
			fl_draw ( cstr, 1, X + xpos, Y + ypos );
			xpos += (int)(fl_width(c) + 0.5);
			idx++;
		}
		idx++;
	}
	cursorX = X + xpos; cursorY = Y + ypos;
	draw_cursor();
	fl_pop_clip();
}

void textview::scrollbarCB()
{
	redraw();
}

void textview::_backspace()
{
	int c;
	unsigned long int lastcrlf = buff.rfind('\n'); // long int for 64 bit cpu's

	if (lastcrlf == string::npos) lastcrlf = 0;
	
	if (attr[attr.length() - 1] == -1) { // soft linefeed skip over
		buff.erase(buff.length()-1);
		attr.erase(attr.length()-1);
		wrappos = 0;
		lastcrlf = buff.rfind('\n');
		if (lastcrlf == string::npos) lastcrlf = 0;
		fl_font(TextFont, TextSize);
		for (unsigned long int i = lastcrlf; i < buff.length(); i++) {
			wrappos += (int)(fl_width(buff[i]));
		}
	}

	c = buff[buff.length()-1];
	if (c == '\n') {
		buff.erase(buff.length()-1);
		attr.erase(attr.length()-1);
		wrappos = 0;
		lastcrlf = buff.rfind('\n');
		if (lastcrlf == string::npos) lastcrlf = 0;
		fl_font(TextFont, TextSize);
		for (unsigned long int i = lastcrlf; i < buff.length(); i++) {
			wrappos += (int)(fl_width(buff[i]));
		}
	} else {
		buff.erase(buff.length()-1);
		attr.erase(attr.length()-1);
		fl_font(TextFont, TextSize);
		wrappos -= (int)(fl_width(c) + 0.5);
	}
}

void textview::_add( char c, int attribute)
{
	if (c == 0x08) {
		if (buff.length() > 0)
			_backspace();
		return;
	} else {
		if (c >= ' ' && c <= '~') {
			buff += c;
			attr += attribute;
			fl_font(TextFont, TextSize);
			charwidth = (int)(fl_width(c) + 0.5);
			if (charwidth > maxcharwidth)
				maxcharwidth = charwidth;
			wrappos += charwidth;
		} else if (c == '\n') {
			buff += c;
			attr += attribute;
			wrappos = 0;
		} 
	}

	int endpos = w() - 24 - maxcharwidth;
	if (wrappos >= endpos) {
		unsigned long int lastspace = buff.find_last_of(' ');
		if (!wordwrap 
			|| lastspace == string::npos 
			|| (buff.length() - lastspace) >= 10
			|| (buff.length() - lastspace) == 1) {
			buff += '\n';
			attr += -1; // soft linefeed attribute
			wrappos = 0;
		} else {
			buff.insert(lastspace+1, 1, '\n');
			attr.insert(lastspace+1, 1, -1);
			wrappos = 0;
			fl_font(TextFont, TextSize);
			for (unsigned long int i = lastspace+2; i < buff.length(); i++)
				wrappos += (int)(fl_width(buff[i]) + 0.5);
		}
	}

	setScrollbar();
}

void textview::add( char *text, int attribute )
{
	unsigned int len = strlen(text);
	Fl::lock();
	for (unsigned int i = 0; i < len; i++)
		_add(text[i], attribute);
	redraw();
	Fl::unlock();
	Fl::awake();
}

void textview::add( char c, int attribute )
{
	Fl::lock();
	_add(c, attribute);
	redraw();
	Fl::unlock();
	Fl::awake();
}

void textview::clear()
{
	Fl::lock();
	buff.erase();
	attr.erase();
	wrappos = 0;
	setScrollbar();
	redraw();
	Fl::unlock();
	Fl::awake();
}


void textview :: setScrollbar()
{
	int lines = lineCount();
	double size;

	fl_font(TextFont, TextSize);
	charheight = fl_height();
	scrollbar->range (lines, 0);
	if (lines * charheight <= h()-4)
		size = 1.0;
	else
		size = (double)(h()-4) / (double)(lines * charheight);
	if (size < 0.08) size = 0.08;
	scrollbar->slider_size( size );
}


void textview :: resize( int x, int y, int w, int h )
{
  Fl_Widget::resize( x, y, w, h );
  setScrollbar();
}


//=====================================================================
// Class TextView
// Viewer for received text
// derived from Class textview
//
// redfines the handle() and menu_cb() functions specified in the
// base class.  All other functions are in the base class
//=====================================================================

TextView::TextView( int x, int y, int w, int h, const char *label )
	: textview ( x, y, w, h, label )
{
	static Fl_Menu_Item menupopup[] = {
		{"Dismiss", 0, _menu_cb, this, FL_MENU_DIVIDER },
		{"divider", 0, _menu_cb, this, FL_MENU_DIVIDER },
		{"clear",	0, _menu_cb, this, FL_MENU_DIVIDER },
		{"Call",	0, _menu_cb, this},
		{"Name",	0, _menu_cb, this},
		{"Qth",		0, _menu_cb, this},
		{"Loc",		0, _menu_cb, this},
		{"RstIn",	0, _menu_cb, this},
		{0}
	};
	mitems = menupopup;
	
	mpopup = new Fl_Menu_Button(-1, -1, -1, -1);
	mpopup->menu(mitems);
	mpopup->type(Fl_Menu_Button::POPUP1);
	cursorStyle = BLOCK_CURSOR;
	cursorON = true;
	wordwrap = true;
}

void TextView::menu_cb(int val)
{
	if (val == 1)
		add("\n     <<================>>\n", RCV);
	else if (val == 2)
		clear();
	else if (val == 3)
		inpCall->value(findtext().c_str());
	else if (val == 4)
		inpName->value(findtext().c_str());
	else if (val == 5)
		inpQth->value(findtext().c_str());
	else if (val == 6) 
		inpLoc->value(findtext().c_str());
	else if (val == 7)
		inpRstIn->value(findtext().c_str());
	restoreFocus();
}

int TextView::handle(int event)
{
// handle events inside the textview and invoked by Right Mouse button or scrollbar
	if (Fl::event_inside( this )) {
		int xpos = Fl::event_x();
		int ypos = Fl::event_y();
		if (xpos > x() + w() - 20) {
			scrollbar->handle(event);
			return 1;
		} else if (event == FL_RELEASE && Fl::event_button() == 3) {
			popx = xpos - x();
			popy = ypos - y();
			mpopup->resize (xpos, ypos, -1, -1);
			mpopup->popup();
			return 1;
		}
	}
	return 0;
}

//=====================================================================
// Class TextEdit
// derived from base class textview
// redfines the handle() and menu_cb() functions specified in the
// base class.  All other functions are in the base class
//=====================================================================

TextEdit::TextEdit( int x, int y, int w, int h, const char *label )
	: textview ( x, y, w, h, label )
{
	static Fl_Menu_Item menupopup[] = {
		{"Dismiss", 0, _menu_cb, this, FL_MENU_DIVIDER },
		{"clear",	0, _menu_cb, this, FL_MENU_DIVIDER },
		{"File",	0, _menu_cb, this, FL_MENU_DIVIDER },
		{"^t",	0, _menu_cb, this},
		{"^r",	0, _menu_cb, this, FL_MENU_DIVIDER },
		{"Picture", 0, _menu_cb, this },
		{0}
	};
	mitems = menupopup;
	
	mpopup = new Fl_Menu_Button(-1, -1, -1, -1);
	mpopup->menu(mitems);
	mpopup->type(Fl_Menu_Button::POPUP1);
	chrptr = 0;
	bkspaces = 0;
	textview::cursorStyle = HEAVY_CURSOR;
	PauseBreak = false;
	wordwrap = false;
}


void TextEdit::readFile()
{
	char * fn = File_Select(
					"Select ASCII text file", 
					"*.txt",
					"", 0);
	if (fn)  {
		string inbuff;
		ifstream in(fn);
		if (in) {
			char ch;
			while (!in.eof()) {
				ch = in.get();
				inbuff += ch;
			}
			in.close();
			add (inbuff.c_str());
		}
	}
}

void TextEdit::menu_cb(int val)
{
	if (val == 1) {
		clear();
		chrptr = 0;
		bkspaces = 0;
	}
	if (val == 2)
		readFile();
	if (val == 3 && buff.empty()) {
		fl_lock(&trx_mutex);
		trx_state = STATE_TX;
		fl_unlock(&trx_mutex);
		wf->set_XmtRcvBtn(true);
	}
	if (val == 4)
		add("^r");
	if (val == 5)
		if (active_modem->get_mode() == MODE_MFSK16)
			active_modem->makeTxViewer(0,0);
}

void TextEdit::clear() {
	textview::clear();
	chrptr = 0;
	PauseBreak = false;
}

int TextEdit::handle_fnckey(int key) {
	int b = key - FL_F - 1;
	if (b > 9)
		return 0;
	
	b += (altMacros ? 10 : 0);
	if (!(macros.text[b]).empty())
		macros.execute(b);

	return 1;
}

int TextEdit::handle_key() {
	int key = Fl::event_key();
	
	if (key == FL_Escape) {
		clear();
		active_modem->set_stopflag(true);
		return 1;
	}
	
	if (key == FL_Pause) {
		if (trx_state != STATE_TX) {
			fl_lock(&trx_mutex);
			trx_state = STATE_TX;
			fl_unlock(&trx_mutex);
			wf->set_XmtRcvBtn(true);
		} else
			PauseBreak = true;
		return 1;
	}

	if (key >= FL_F && key <= FL_F_Last)
		return handle_fnckey(key);

// substitute the FN # you want to map for each
// ie: F1 ==> 1 + Fl_F
// then remove the two slashes from each of the following lines
//	if (key == FL_Left)
//		return handle_fnckey(n + FL_F);
//	if (key == FL_Up)
//		return handle_fnckey(n + FL_F);
//	if (key == FL_Right)
//		return handle_fnckey(n + FL_F);
//	if (key == FL_Down)
//		return handle_fnckey(n + FL_F);

	if (key == FL_Left) {
		active_modem->searchDown();
		return 1;
	}
	if (key == FL_Right) {
		active_modem->searchUp();
		return 1;
	}
		
	if (key == FL_Enter) {
		add('\n');
		return 1;
	}
	
	if (key == FL_BackSpace) {
		add (0x08);
		if (chrptr > buff.length()) {
			chrptr = buff.length();
			bkspaces++;
		}
		return 1;
	}
	
	const char *ch = Fl::event_text();
	add(ch);
	return 1;
}

int TextEdit::handle(int event)
{
// handle events inside the textedit widget
	if (event == FL_UNFOCUS && Fl::focus() != this) {
		textview::cursorON = false;
		redraw();
		return 1;
	}
	if (event == FL_FOCUS && Fl::focus() == this) {
		textview::cursorON = true;
		redraw();
		return 1;
	}
	if (event == FL_KEYBOARD) {
//		textview::cursorON = true;
		return handle_key();
	}
	if (Fl::event_inside( this )) {
		int xpos = Fl::event_x();
		int ypos = Fl::event_y();
		if (xpos > x() + w() - 20) {
			scrollbar->handle(event);
			return 1;
		} else if (event == FL_RELEASE && Fl::event_button() == 3) {
			mpopup->resize (xpos, ypos, -1, -1);
			mpopup->popup();
			textview::cursorON = true;
			Fl::focus(this);
			redraw();
			return 1;
		}
		switch (event) {
			case FL_RELEASE:
				textview::cursorON = true;
				Fl::focus(this);
			case FL_FOCUS:
				textview::cursorON = true;
				redraw();
				return 1;
			case FL_UNFOCUS:
				textview::cursorON = false;
				redraw();
				return 1;
		}
	}
	return 0;
}

int TextEdit::nextChar()
{
	if (bkspaces) {
		--bkspaces;
		return 0x08;
	}
	if (PauseBreak) {
		PauseBreak = false;
		return 0x03;
	}
	if (buff.empty()) return 0;
	if (chrptr == buff.length()) return 0;
	if (attr[chrptr] == -1) {
		chrptr++;
		if (chrptr == buff.length()) return 0;
	}
	Fl::lock();
	attr[chrptr] = 4;
	redraw();
	Fl::unlock();
//	redraw();
	Fl::awake();
	return (buff[chrptr++]);
}

void TextEdit::cursorON() 
{ 
	textview::cursorON = true; 
	redraw();
}

//=====================================================================
// Class MacroEdit
// derived from base class textview
// redfines the handle() and menu_cb() functions specified in the
// base class.  All other functions are in the base class
//=====================================================================

MacroEdit::MacroEdit( int x, int y, int w, int h, const char *label )
	: textview ( x, y, w, h, label )
{
	static Fl_Menu_Item menupopup[] = {
		{"Dismiss", 0, _menu_cb, this, FL_MENU_DIVIDER },
		{"clear",	0, _menu_cb, this, FL_MENU_DIVIDER },
		{0}
	};
	mitems = menupopup;
	
	mpopup = new Fl_Menu_Button(-1, -1, -1, -1);
	mpopup->menu(mitems);
	mpopup->type(Fl_Menu_Button::POPUP1);
}

void MacroEdit::menu_cb(int val)
{
	if (val == 1) {
		clear();
	}
}

int MacroEdit::handle_key() {
	int key = Fl::event_key();
	if (key == FL_Enter) {
		add('\n');
		return 1;
	}
	if (key == FL_BackSpace) {
		add (0x08);
		return 1;
	}
	const char *ch = Fl::event_text();
	add(ch);
	return 1;
}

int MacroEdit::handle(int event)
{
// handle events inside the MacroEdit widget
	if (Fl::event_inside( this )) {
		int xpos = Fl::event_x();
		int ypos = Fl::event_y();
		if (xpos > x() + w() - 20) {
			scrollbar->handle(event);
			return 1;
		} else if (event == FL_RELEASE && Fl::event_button() == 3) {
			mpopup->resize (xpos, ypos, -1, -1);
			mpopup->popup();
			Fl::focus(this);
			return 1;
		}
	}
	switch (event) {
		case FL_RELEASE:
		case FL_FOCUS:
			Fl::focus(this);
			return 1;
		case FL_UNFOCUS:
			return 1;
		case FL_KEYBOARD : 
			return handle_key();
	}
	return 0;
}

