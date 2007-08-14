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

#include "cw.h"
#include "misc.h"

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

#define SBwidth 16

textview :: textview( int x, int y, int w, int h, const char *label )
  : Fl_Widget( x, y, w, h, label ),
    scrollbar(x+w-SBwidth, y+2, SBwidth, h-4 )
{
	scrollbar.linesize( 1 );
	scrollbar.callback( _scrollbarCB, this );

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
	cursorwidth = 4;
	startidx = string::npos;
	laststartidx = string::npos;
    highlightstart = string::npos;
    highlightend = string::npos;
		
	H = h - 4;
	W = w - 4 - SBwidth;
	X = x + 2;
	Y = y + 2;

	clear();
}

textview :: ~textview()
{
}

void textview::Show() {
	scrollbar.show();
	show();
}

void textview::Hide() {
	scrollbar.hide();
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
	damage(FL_DAMAGE_ALL);
	Fl::unlock();
	Fl::awake();
}

void textview::setFontSize(int siz)
{
	Fl::lock();
	TextSize = siz;
	damage(FL_DAMAGE_ALL);
	Fl::unlock();
	Fl::awake();
}

void textview::setFontColor(Fl_Color clr)
{
}

int textview::lineCount()
{
	int cnt = 1;
	size_t len = buff.length();
	if (len == 0)
		cnt = 0;
	else
		for (size_t n = 0; n < len; n++)
			if (buff[n] == '\n') cnt++;
	return cnt;
}

size_t textview::linePosition(int linenbr)
{
	size_t len = buff.length();
	size_t pos = 0;
	while (linenbr && (pos < len) ) {
		if (buff[pos] == '\n')
			--linenbr;
		pos++;
	}
	if (pos == len) return 0;
	return pos;
}

size_t textview::xy2bufidx()
{
    size_t idx = startidx;
	size_t len = buff.length();
	int xc = 0, yc = 0;
    int cheight, cwidth, descent;
	char c;	

    if (!len) return string::npos;

	fl_font(TextFont, TextSize);
    cheight = (int)fl_height();
    descent = fl_descent();

	while(idx < len) {
		if (yc > H) 
			break;
		while (idx < len ) {
			c = buff[idx];
            cwidth = (int)fl_width(c);
			if (c == '\n') {
				xc = 0;
				yc += cheight;
				break;
			}
            if ( (popx >= xc && (popx < (xc + cwidth))) &&
                 (popy >= yc && (popy < (yc + charheight + descent))) ) {
				return idx;
			}
            idx++;
            xc += cwidth;
		}
		idx++;
	}
    return string::npos;
}

void textview::highlight(bool b)
{
    if (highlightstart == string::npos || highlightend == string::npos)
        return;
    if (b)
        for (size_t n = highlightstart; n <= highlightend; n++)
            attr[n] |= 0x20;
    else
        for (size_t n = highlightstart; n <= highlightend; n++)
            attr[n] &= 0x0F;
    damage(FL_DAMAGE_ALL);        
}

string textview::findtext()
{
	size_t idx, wordstart, wordend;
	selword = "";

    idx = xy2bufidx();
    if (idx != string::npos) {
		wordstart = buff.find_last_of(" \n", idx);
		if (wordstart == string::npos) 
            wordstart = 0;
        else
            wordstart++;
		wordend = buff.find_first_of(" ,\n", idx);
		if (wordend == string::npos) 
            wordend = buff.length();
		selword = buff.substr(wordstart, wordend - wordstart);
		return selword;
	}
	return selword;
}


void textview::highlightword()
{
	size_t idx, wordstart, wordend;

    idx = xy2bufidx();
    highlightstart = highlightend = string::npos;
    if (idx != string::npos) {
		wordstart = buff.find_last_of(" \n", idx);
		if (wordstart == string::npos) 
            wordstart = 0;
        else
            wordstart++;
		wordend = buff.find_first_of(" ,\n", idx);
		if (wordend == string::npos) 
            wordend = buff.length();
        else
            wordend--;
        highlightstart = wordstart;
        highlightend = wordend;
        highlight(true);
	}
}


void textview::draw_cursor()
{
	if (cursorStyle == NONE) return;
  
	typedef struct { int x1, y1, x2, y2; } Segment;

 	Segment segs[ 8 ];
	int nSegs = 0;
	int left = cursorX + 1,
		right = left + cursorwidth,
		bot = cursorY + descent - 2,
		top = cursorY - charheight + descent + 2;

  /* For cursors other than the block, make them around 2/3 of a character
     width, rounded to an even number of pixels so that X will draw an
     odd number centered on the stem at x. */

	if (cursorON == false)
		return;

	fl_color(FL_WHITE);
	fl_rectf ( X + cursorX, Y + cursorY - charheight + descent, maxcharwidth, charheight);

  /* Create segments and draw cursor */
  if ( cursorStyle == CARET_CURSOR ) {
    int midY = top - charheight / 5;
    segs[ 0 ].x1 = left;		segs[ 0 ].y1 = top;		segs[ 0 ].x2 = left;		segs[ 0 ].y2 = midY;
    segs[ 1 ].x1 = left;		segs[ 1 ].y1 = midY;	segs[ 1 ].x2 = right;		segs[ 1 ].y2 = top;
    segs[ 2 ].x1 = left;		segs[ 2 ].y1 = top;		segs[ 2 ].x2 = left;		segs[ 2 ].y2 = midY - 1;
    segs[ 3 ].x1 = left;		segs[ 3 ].y1 = midY - 1;segs[ 3 ].x2 = right;		segs[ 3 ].y2 = top;
    nSegs = 4;
  } else if ( cursorStyle == NORMAL_CURSOR ) {
  	int midX = left + cursorwidth / 2;
    segs[ 0 ].x1 = left;		segs[ 0 ].y1 = bot;		segs[ 0 ].x2 = right;		segs[ 0 ].y2 = bot;
    segs[ 1 ].x1 = midX;		segs[ 1 ].y1 = bot;		segs[ 1 ].x2 = midX;		segs[ 1 ].y2 = top;
    segs[ 2 ].x1 = left; 		segs[ 2 ].y1 = top;		segs[ 2 ].x2 = right;		segs[ 2 ].y2 = top;
    nSegs = 3;
  } else if ( cursorStyle == HEAVY_CURSOR ) {
  	int topp1 = top + 1, botm1 = bot - 1,
  		mid = left + cursorwidth / 2,
  		midp1 = mid + 1;
    segs[ 0 ].x1 = mid;			segs[ 0 ].y1 = bot;		segs[ 0 ].x2 = mid;			segs[ 0 ].y2 = top;
    segs[ 1 ].x1 = midp1;		segs[ 1 ].y1 = bot;		segs[ 1 ].x2 = midp1;		segs[ 1 ].y2 = top;
    segs[ 3 ].x1 = left;		segs[ 3 ].y1 = bot;		segs[ 3 ].x2 = right;		segs[ 3 ].y2 = bot;
    segs[ 4 ].x1 = left;		segs[ 4 ].y1 = top;		segs[ 4 ].x2 = right;		segs[ 4 ].y2 = top;
    segs[ 5 ].x1 = left;		segs[ 5 ].y1 = botm1;	segs[ 5 ].x2 = right;		segs[ 5 ].y2 = botm1;
    segs[ 6 ].x1 = left;		segs[ 6 ].y1 = topp1;	segs[ 6 ].x2 = right;		segs[ 6 ].y2 = topp1;
    nSegs = 7;
  } else if ( cursorStyle == DIM_CURSOR ) {
  	int midX = left + cursorwidth / 2;
    segs[ 0 ].x1 = left;		segs[ 0 ].y1 = bot;		segs[ 0 ].x2 = right;		segs[ 0 ].y2 = bot;
    segs[ 1 ].x1 = midX;		segs[ 1 ].y1 = bot;		segs[ 1 ].x2 = midX;		segs[ 1 ].y2 = top;
    segs[ 2 ].x1 = left; 		segs[ 2 ].y1 = top;		segs[ 2 ].x2 = right;		segs[ 2 ].y2 = top;
    nSegs = 3;
  } else if ( cursorStyle == BLOCK_CURSOR ) {
    right = cursorX + maxcharwidth;
    segs[ 0 ].x1 = left;		segs[ 0 ].y1 = bot;		segs[ 0 ].x2 = right;		segs[ 0 ].y2 = bot;
    segs[ 1 ].x1 = right;		segs[ 1 ].y1 = bot;		segs[ 1 ].x2 = right;		segs[ 1 ].y2 = top;
    segs[ 2 ].x1 = right;		segs[ 2 ].y1 = top;		segs[ 2 ].x2 = left;		segs[ 2 ].y2 = top;
    segs[ 3 ].x1 = left;		segs[ 3 ].y1 = top;		segs[ 3 ].x2 = left;		segs[ 3 ].y2 = bot;
    nSegs = 4;
  }
  fl_color( FL_BLACK );

  for ( int k = 0; k < nSegs; k++ ) {
    fl_line( X + segs[ k ].x1, Y + segs[ k ].y1, X + segs[ k ].x2, Y + segs[ k ].y2 );
  }
}

void textview::drawall()
{
	int line = 0;
	size_t len = 0;
	char c = 0;

	fl_font(TextFont, TextSize);
	charheight = fl_height();
	maxcharwidth = (int)fl_width('X');
	descent = fl_descent();

	draw_box();
// resize the scrollbar to be a constant width
	scrollbar.resize( x()+w()-SBwidth, y()+2, SBwidth, h()-4 );
	scrollbar.redraw();

	cursorX = 0;
	cursorY = charheight - descent;
	endidx = 0;
	if ((len = buff.length()) == 0) {
		fl_push_clip( X, Y, W, H);
		draw_cursor();
		fl_pop_clip();
    	return;
	}

	nlines = lineCount();
	line = nlines - H / charheight - scrollbar.value();
	  
	startidx = linePosition(line);
	endidx = startidx;
	
	fl_push_clip( X, Y, W, H );

	memset(cstr, 0, 1000);
	int pos = 0;
	int a;
  
	while(endidx < len) {
		if (cursorY > H) 
			break;
		while (endidx < len ) {
			c = buff[endidx];
			a = attr[endidx];
			if (c == '\n') {
				cursorX = 0;
				cursorY += charheight;
				endidx++;
				memset(cstr, 0, 1000);
				pos = 0;
				break;
			}
			cstr[pos++] = c;
            if ((a & 0x20) == 0x20) {
                fl_color(FL_YELLOW);
	            fl_rectf ( X + cursorX, Y + cursorY - charheight + descent, maxcharwidth, charheight);
				fl_color (TextColor[(int)a & 0x0F]);
				fl_draw ( cstr, 1, X + cursorX, Y + cursorY );
				cursorX += (int)fl_width(c);
				pos = 0;
				endidx++;
            } else {
            	endidx++;
            	c = buff[endidx];
				while (endidx < len && c != '\n' && a == attr[endidx] && pos < 999) {
					cstr[pos++] = c;
					++endidx;
					c = buff[endidx];
				} 
				fl_color (TextColor[(int)a & 0x0F]);
				fl_draw ( cstr, X + cursorX, Y + cursorY );
				cursorX += (int)fl_width(cstr);
				memset(cstr, 0, 1000);
				pos = 0;
            }
		}
	}
	laststartidx = startidx;
	
	draw_cursor();
	fl_pop_clip();
}

void textview::drawchars()
{
	int line = 0;
	size_t startidx = string::npos;
	size_t len = 0;
	char c = 0;
	char cstr[] = " ";
  
	if ((len = buff.length()) == 0) {
		drawall();
		return;
	}

	fl_font(TextFont, TextSize);
	charheight = fl_height();
	descent = fl_descent();
	maxcharwidth = (int)fl_width('X');

	nlines = lineCount();
	line = nlines - H / charheight - scrollbar.value();
	  
	startidx = linePosition(line);
	if (startidx != laststartidx) {
		drawall();
		return;
	}

	fl_push_clip( X, Y, W, H );

	while (endidx < len) {
		c = buff[endidx];
		if (c == '\n') {
			cursorX = 0;
			cursorY += charheight;
		} else {
			cstr[0] = c;
            if ((attr[endidx] & 0x20) == 0x20)
                fl_color(FL_YELLOW);
            else
                fl_color(FL_WHITE);
            fl_rectf ( X + cursorX, Y + cursorY - charheight + descent, maxcharwidth, charheight);
			fl_color (TextColor[(int)attr[endidx] & 0x0F]);
			fl_draw ( cstr, 1, X + cursorX, Y + cursorY );
			cursorX += (int)fl_width(c);
		}
		endidx++;
	}
	laststartidx = startidx;
	
	draw_cursor();
	fl_pop_clip();
}

void textview::drawmodify()
{
    if (modidx < laststartidx )
        return;
    if (modidx > endidx)
        return;
    if (buff[modidx] == '\n')
        return;
// modify the character insitu
// find the screen location for the character redraw
    size_t posidx = laststartidx;
    int posX = 0, posY = charheight - descent;
    char c = 0;
    
	fl_font(TextFont, TextSize);
	while (posidx < modidx) {
		c = buff[posidx];
		if (c == '\n') {
			posX = 0;
			posY += charheight;
		} else {
			posX += (int)(fl_width(c));
		}
		posidx++;
	}
// should now be pointing to the (x,y) screen location for the character
	char cstr[] = "";
    cstr[0] = buff[modidx];
// erase existing
    if ((attr[modidx] & 0x20) == 0x20)
        fl_color(FL_YELLOW);
    else
        fl_color(FL_WHITE);
	fl_rectf ( X + posX, Y + posY - charheight + descent, (int)fl_width(c), charheight);
// draw new with attribute
	fl_color (TextColor[(int)attr[modidx] & 0x0F]);
	fl_draw ( cstr, 1, X + posX, Y + posY );
}

void textview::draw()
{
	if (damage() & FL_DAMAGE_ALL) {
		drawall();
		return;
	}
	if (damage() & (FL_DAMAGE_ALL | 1)) {
		drawchars();
		return;
	}
    if (damage() & (FL_DAMAGE_ALL | 2)) {
        draw_cursor();
        return;
    }
    if (damage() & (FL_DAMAGE_ALL | 4)) {
        drawmodify();
        return;
    }
}

void textview::scrollbarCB()
{
	damage(FL_DAMAGE_ALL);
}

void textview::_backspace()
{
	int c;
	if (buff.empty()) return;
	
	size_t lastcrlf = buff.rfind('\n');

	if (lastcrlf == string::npos) lastcrlf = 0;

Fl::lock();	
	if (attr[attr.length() - 1] == -1) { // soft linefeed skip over
		buff.erase(buff.length()-1);
		attr.erase(attr.length()-1);
		wrappos = 0;
		lastcrlf = buff.rfind('\n');
		if (lastcrlf == string::npos) lastcrlf = 0;
		fl_font(TextFont, TextSize);
		for (size_t i = lastcrlf; i < buff.length(); i++) {
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
		for (size_t i = lastcrlf; i < buff.length(); i++) {
			wrappos += (int)(fl_width(buff[i]));
		}
	} else {
		buff.erase(buff.length()-1);
		attr.erase(attr.length()-1);
		fl_font(TextFont, TextSize);
		wrappos -= (int)fl_width(c);
	}
	damage(FL_DAMAGE_ALL);
Fl::unlock();
Fl::awake();
}

void textview::add( char c, int attribute)
{
	if (c == 0x08) {
		_backspace();
		return;
	}

	Fl::lock();
	if (c >= ' ' && c <= '~') {
		buff += c;
		attr += attribute;
		fl_font(TextFont, TextSize);
		charwidth = (int)fl_width(c);
		if (charwidth > maxcharwidth)
			maxcharwidth = charwidth;
		wrappos += charwidth;
	} else if (c == '\n') {
		buff += c;
		attr += attribute;
		wrappos = 0;
	}

	if (wrappos >= (w() - 24 - maxcharwidth)) {
		size_t lastspace = buff.find_last_of(' ');
		if (!wordwrap 
			|| lastspace == string::npos 
			|| (buff.length() - lastspace) >= 10
			|| (buff.length() - lastspace) == 1) {
			buff += '\n';
			attr += -1; // soft linefeed attribute
			wrappos = 0;
			damage(1);
		} else {
			buff.insert(lastspace+1, 1, '\n');
			attr.insert(lastspace+1, 1, -1);
			wrappos = 0;
			fl_font(TextFont, TextSize);
			for (size_t i = lastspace+2; i < buff.length(); i++)
				wrappos += (int)fl_width(buff[i]);
			damage(FL_DAMAGE_ALL);
		}
	} else
		damage(1);
	Fl::unlock();
//	Fl::awake();
	
	setScrollbar();
}

void textview::add( char *text, int attr )
{
	for (unsigned int i = 0; i < strlen(text); i++)
		add(text[i], attr);
	return;
}

void textview::clear()
{
	Fl::lock();
	buff.erase();
	attr.erase();
    highlightstart = string::npos;
    highlightend = string::npos;
	wrappos = 0;
	startidx = string::npos;
	endidx = 0;
//	cursorX = 0;
//	cursorY = charheight;
	laststartidx = string::npos;
	setScrollbar();
	damage(FL_DAMAGE_ALL);
	Fl::unlock();
	Fl::awake();
}


void textview :: setScrollbar()
{
	int lines = lineCount();
	double size;

	fl_font(TextFont, TextSize);
	charheight = fl_height();
	scrollbar.range (lines, 0);
	if (lines * charheight <= h()-4)
		size = 1.0;
	else
		size = (double)(h()-4) / (double)(lines * charheight);
	if (size < 0.08) size = 0.08;
	scrollbar.slider_size( size );
}

void textview :: rebuildsoft(int W)
{
	size_t lfpos, chpos;
	if (W == w())
		return; // same width no rebuild needed
// remove all soft linefeeds
	while ((lfpos = attr.find(-1)) != string::npos) {
		buff.erase(lfpos, 1);
		attr.erase(lfpos, 1);
	}
	int endidx = W - 24 - maxcharwidth;
	wrappos = 0;
	chpos = 0;
	while (chpos < buff.length()) {
		if (buff[chpos] == '\n')
			wrappos = 0;
		else
			fl_font(TextFont, TextSize);
			wrappos += (int)fl_width(buff[chpos]);
		if (wrappos >= endidx) {
			size_t lastspace = buff.find_last_of(' ', chpos);
			if (!wordwrap 
				|| lastspace == string::npos 
				|| (chpos - lastspace) >= 10
				|| (chpos == lastspace) ) {
				buff.insert(chpos, 1, '\n');
				attr.insert(chpos, 1, -1);
				wrappos = 0;
				chpos++;
			} else {
				buff.insert(lastspace+1, 1, '\n');
				attr.insert(lastspace+1, 1, -1);
				wrappos = 0;
				chpos++;
				fl_font(TextFont, TextSize);
				for (size_t i = lastspace+2; i < chpos; i++)
					wrappos += (int)fl_width(buff[i]);
			}
		}
		chpos++;
	}
}

void textview :: resize( int x, int y, int w, int h )
{
	rebuildsoft(w);
	H = h - 4;
	W = w - 4 - SBwidth;
	X = x + 2;
	Y = y + 2;
	Fl_Widget::resize( x, y, w, h );
	setScrollbar();
}


//=====================================================================
// Class TextView
// Viewer for received text
// derived from Class textview
//
// redefines the handle() and menu_cb() functions specified in the
// base class.  All other functions are in the base class
//=====================================================================

void TextView::saveFile()
{
	char * fn = File_Select(
					"Select ASCII text file", 
					"*.txt",
					"", 0);
	if (fn) {
		ofstream out(fn);
		out << buff;
		out.close();
	}
}

Fl_Menu_Item TextView::viewmenu[] = {
	{"@-9$returnarrow &QRZ this call",	0, 0 }, 				// 0
	{"@-9-> &Call",				0, 0 },							// 1
	{"@-9-> &Name",				0, 0 },							// 2
	{"@-9-> QT&H",				0, 0 },							// 3
	{"@-9-> &Locator",			0, 0 },							// 4
	{"@-9-> &RSTin",			0, 0, 0, FL_MENU_DIVIDER },		// 5
	{"Insert divider",			0, 0 },							// 6
	{"C&lear",				0, 0 },								// 7
//	{"&Copy",				0, 0, 0, FL_MENU_DIVIDER },
	{"Save to &file...",			0, 0, 0, FL_MENU_DIVIDER },	// 8
	{"Word &wrap",				0, 0, 0, FL_MENU_TOGGLE	 },		// 9
	{ 0 }
};

int viewmenuNbr = 10;

TextView::TextView( int x, int y, int w, int h, const char *label )
	: textview ( x, y, w, h, label )
{
	cursorStyle = NONE;// BLOCK_CURSOR;
	cursorON = true;
	wordwrap = true;
	viewmenu[9].set();
}

void TextView::menu_cb(int val)
{
//	handle(FL_UNFOCUS);
	switch (val) {
	case 0: // select call & do qrz query
		menu_cb(1);
		extern void QRZquery();
		QRZquery();
		break;
	case 1: // select call
		inpCall->value(findtext().c_str());
		break;
	case 2: // select name
		inpName->value(findtext().c_str());
		break;
	case 3: // select QTH
		inpQth->value(findtext().c_str());
		break;
	case 4: // select locator
		inpLoc->value(findtext().c_str());
		break;
	case 5: // select RST rcvd
		inpRstIn->value(findtext().c_str());
		break;
	case 6:
		add("\n	    <<================>>\n", RCV);
		break;
	case 7:
		clear();
		break;
//	case 8: // clipboard copy
//		clipboard_copy();
//		break;
	case 8:
		saveFile();
		break;

	case 9: // wordwrap toggle
		wordwrap = !wordwrap;
		if (wordwrap)
			viewmenu[9].set();
		else
			viewmenu[9].clear();
		break;
	}
}

int TextView::handle(int event)
{
// handle events inside the textview and invoked by Right Mouse button or scrollbar
	if (Fl::event_inside( this )) {
		const Fl_Menu_Item * m;
		int xpos = Fl::event_x();
		int ypos = Fl::event_y();
		if (xpos > x() + w() - SBwidth) {
			scrollbar.handle(event);
			return 1;
		}
		if (event == FL_PUSH && Fl::event_button() == 3) {
			popx = xpos - x();
			popy = ypos - y();
            highlightword();
			m = viewmenu->popup(xpos, ypos, 0, 0, 0);
			if (m) {
				for (int i = 0; i < viewmenuNbr; i++)
					if (m == &viewmenu[i]) {
						menu_cb(i);
						break;
					}
			}
            highlight(false);
			return 1;
		}
//        if (event == FL_PUSH && Fl::event_button() == 1) {
//            popx = xpos - x();
//            popy = ypos - y();
//            size_t pos = xy2bufidx();
//            if (pos != string::npos)
//                std::cout << buff[pos] << " @ " << pos << " ==> " << popx << ", " << popy << std::endl;
//            else
//                std::cout << "? position\n";
//            return 1;
//        }

	}
	return 0;
}

//=====================================================================
// Class TextEdit
// derived from base class textview
// redfines the handle() and menu_cb() functions specified in the
// base class.  All other functions are in the base class
//=====================================================================

Fl_Menu_Item editmenu[] = {
	{"clear",	0, 0, 0, FL_MENU_DIVIDER },
	{"File",	0, 0, 0, FL_MENU_DIVIDER },
	{"^t",	0, 0 },
	{"^r",	0, 0, 0, FL_MENU_DIVIDER },
	{"Picture", 0, 0 },
	{0}
};
int editmenuNbr = 5;

TextEdit::TextEdit( int x, int y, int w, int h, const char *label )
	: textview ( x, y, w, h, label )
{
	xmtidx = 0;
	bkspaces = 0;
	textview::cursorStyle = NORMAL_CURSOR;
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
	Fl::focus(this);
}

void TextEdit::menu_cb(int val)
{
	if (val == 0) {
		clear();
		xmtidx = 0;
		bkspaces = 0;
	}
	if (val == 1)
		readFile();
	if (val == 2 && buff.empty()) {
		fl_lock(&trx_mutex);
		trx_state = STATE_TX;
		fl_unlock(&trx_mutex);
		wf->set_XmtRcvBtn(true);
	}
	if (val == 3)
		add("^r");
	if (val == 4)
		if (active_modem->get_mode() == MODE_MFSK16)
			active_modem->makeTxViewer(0,0);
}

void TextEdit::clear() {
	textview::clear();
	xmtidx = 0;
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
		Fl::focus(this);
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
		Fl::focus(this);
		return 1;
	}
	
	if (key == (FL_KP + '+')) {
		if (active_modem == cw_modem) active_modem->incWPM();
		Fl::focus(this);
		return 1;
	}
	if (key == (FL_KP + '-')) {
		if (active_modem == cw_modem) active_modem->decWPM();
		Fl::focus(this);
		return 1;
	}
	if (key == (FL_KP + '*')) {
		if (active_modem == cw_modem) active_modem->toggleWPM();
		Fl::focus(this);
		return 1;
	}

	if (key >= FL_F && key <= FL_F_Last)
		return handle_fnckey(key);
		
	if (key == FL_Tab && active_modem == cw_modem) {
		while (xmtidx < buff.length()) {
			attr[xmtidx] = 2;
			xmtidx++;
		}
		damage(FL_DAMAGE_ALL);
		Fl::focus(this);
		return 1;
	}

	if (key == FL_Left) {
		active_modem->searchDown();
		Fl::focus(this);
		return 1;
	}
	if (key == FL_Right) {
		active_modem->searchUp();
		Fl::focus(this);
		return 1;
	}
		
	if (key == FL_Enter) {
		add('\n');
		Fl::focus(this);
		return 1;
	}
	
	if (key == FL_BackSpace) {
		add (0x08);
		if (xmtidx > buff.length()) {
			xmtidx = buff.length();
			bkspaces++;
		}
		Fl::focus(this);
		return 1;
	}
	
	const char *ch = Fl::event_text();
	add(ch);
	Fl::focus(this);
	return 1;
}

int TextEdit::handle(int event)
{
// handle events inside the textedit widget
	if (event == FL_UNFOCUS && Fl::focus() != this) {
		textview::cursorON = false;
		damage(3);
		return 1;
	}
	if (event == FL_FOCUS && Fl::focus() == this) {
		textview::cursorON = true;
		damage(2);
		Fl::focus(this);
		return 1;
	}
	if (event == FL_KEYBOARD) {
		return handle_key();
	}
	if (Fl::event_inside( this )) {
		const Fl_Menu_Item * m;
		int xpos = Fl::event_x();
		int ypos = Fl::event_y();
		if (xpos > x() + w() - SBwidth) {
			scrollbar.handle(event);
			Fl::focus(this);
			return 1;
		}
		if (event == FL_PUSH && Fl::event_button() == 3) {
			popx = xpos - x();
			popy = ypos - y();
			m = editmenu->popup(xpos, ypos, 0, 0, 0);
			if (m) {
				for (int i = 0; i < editmenuNbr; i++)
					if (m == &editmenu[i]) {
						menu_cb(i);
						break;
					}
			}
			Fl::focus(this);
			return 1;
		}

		switch (event) {
			case FL_PUSH:
				textview::cursorON = true;
				damage(2);
				Fl::focus(this);
				return 1;
			case FL_FOCUS:
				textview::cursorON = true;
				damage(2);
				return 1;
			case FL_UNFOCUS:
				textview::cursorON = false;
				damage(2);
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
	if (xmtidx == buff.length()) return 0;
	if (attr[xmtidx] == -1) {
		xmtidx++;
		if (xmtidx == buff.length()) return 0;
	}
	Fl::lock();
	attr[xmtidx] = 4;
    modidx = xmtidx;
	damage(4);
	Fl::unlock();
	Fl::flush();
//    Fl::awake();
	return (buff[xmtidx++]);
}

void TextEdit::cursorON() 
{ 
	textview::cursorON = true; 
	damage(2);
}

