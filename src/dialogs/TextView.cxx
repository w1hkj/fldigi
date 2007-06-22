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
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307	 USA
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

#include <FL/Enumerations.H>
#include "File_Selector.h"

using namespace std;


Fl_Menu_Item viewmenu[] = {
	{"@-9-> Call",		0, 0 },
	{"@-9-> Name",		0, 0 },
	{"@-9-> QTH",		0, 0 },
	{"@-9-> Locator",	0, 0 },
	{"@-9-> RSTin",		0, 0, 0, FL_MENU_DIVIDER },
	{"Insert divider",	0, 0 },
	{"Clear",		0, 0 },
	{"Copy",		0, 0, 0, FL_MENU_DIVIDER },
	{"Save to file...",	0, 0, 0, FL_MENU_DIVIDER },
	{"Word wrap",		0, 0, 0, FL_MENU_TOGGLE|FL_MENU_VALUE } ,
	{ 0 }
};

Fl_Text_Display::Style_Table_Entry TextView::styles[NSTYLES];

TextView::TextView(int x, int y, int w, int h, const char *l)
	: Fl_Text_Display(x, y, w, h, l)
{
	tbuf = new Fl_Text_Buffer;
	sbuf = new Fl_Text_Buffer;

	buffer(tbuf);
	highlight_data(sbuf, styles, NSTYLES, 'A', 0, 0);
	cursor_style(Fl_Text_Display::BLOCK_CURSOR);

	wrap_mode();

	// set some defaults
	setFont(FL_COURIER);
	setFontSize(12);
	setFontColor(FL_BLACK);
	setFontColor(2, FL_BLUE);
	setFontColor(3, FL_GREEN);
	setFontColor(4, FL_RED);
}
TextView::~TextView()
{
	delete tbuf;
	delete sbuf;
}

int TextView::handle(int event)
{
	switch (event) {
	case FL_FOCUS:
		show_cursor(1);
		if (tbuf->selected())
			redraw();
		Fl::focus(this);
		return 1;
		break;
	case FL_UNFOCUS:
		show_cursor(1);
		if (tbuf->selected())
			redraw();
	case FL_ENTER:
		show_cursor(1);
		return 1;
		break;
	case FL_PUSH:
		if (Fl::event_button() == FL_RIGHT_MOUSE) {
			if (!(Fl::event_inside(this) && Fl::focus() == this))
				break;

			const Fl_Menu_Item * m;
			int xpos = Fl::event_x();
			int ypos = Fl::event_y();

			popx = xpos - x();
			popy = ypos - y();
			m = viewmenu->popup(xpos, ypos, 0, 0, 0);
			if (m) {
				int msize = sizeof(viewmenu) / sizeof(viewmenu[0]);
				for (int i = 0; i < msize; i++)
					if (m == &viewmenu[i]) {
						menu_cb(i);
						break;
					}
			}
			return 1;
		}
		break;
	}

	return Fl_Text_Display::handle(event);
}

void TextView::add(char c, int attr)
{
	if (c == '\r')
		return;
	Fl::lock();
	switch (c) {
	case '\b':
	{
		int tl = tbuf->length(), sl = sbuf->length();
		tbuf->remove(tl - 1, tl);
		sbuf->remove(sl - 1, sl);
	}
	break;
	default:
		attr = (attr >= 0 && attr < NSTYLES) ? attr : 0;
		char s[] = { c, '\0' };
		tbuf->append(s);
		s[0] = 'A' + attr;
		sbuf->append(s);
		break;
	}

	// if we are displaying the last line we should scroll down to keep it visible
	if (mTopLineNum + mNVisibleLines > mNBufferLines)
		scroll(mNBufferLines - 1, 0);

	redraw();
	Fl::unlock();
	Fl::awake();
}
void TextView::add(const char *s, int attr)
{
	while (*s)
		add(*s++, attr);
}

void TextView::clear(void)
{
	tbuf->text("");
	sbuf->text("");
}


void TextView::setFont(int n, Fl_Font f)
{
	if (n >= 0 && n < NSTYLES)
		styles[n].font = f;
	else if (n == -1)
		for (int i = 0; i < NSTYLES; i++)
			styles[i].font = f;
	redraw();
}
void TextView::setFontSize(int n, int s)
{
	if (n >= 0 && n < NSTYLES)
		styles[n].size = s;
	else if (n == -1)
		for (int i = 0; i < NSTYLES; i++)
			styles[i].size = s;
	redraw();
}
void TextView::setFontColor(int n, Fl_Color c)
{
	if (n >= 0 && n < NSTYLES)
		styles[n].color = c;
	else if (n == -1)
		for (int i = 0; i < NSTYLES; i++)
			styles[i].color = c;
	redraw();
}


void TextView::draw(void)
{
	Fl_Text_Display::draw();
	redraw();
}

void TextView::menu_cb(int val)
{
	handle(FL_UNFOCUS);
	switch (val) {
		char *s;
	case RX_MENU_CALL:
		s = get_word(popx, popy);
		inpCall->value(s);
		free(s);
		break;
	case RX_MENU_NAME:
		s = get_word(popx, popy);
		inpName->value(s);
		free(s);
		break;
	case RX_MENU_QTH:
		s = get_word(popx, popy);
		inpQth->value(s);
		free(s);
		break;
	case RX_MENU_LOC:
		s = get_word(popx, popy);
		inpLoc->value(s);
		free(s);
		break;
	case RX_MENU_RST_IN:
		s = get_word(popx, popy);
		inpRstIn->value(s);
		free(s);
		break;

	case RX_MENU_DIV:
		add("\n	    <<================>>\n", RCV);
		break;
	case RX_MENU_CLEAR:
		clear();
		break;
	case RX_MENU_COPY:
		clipboard_copy();
		break;

	case RX_MENU_SAVE:
		saveFile();
		break;

	case RX_MENU_WRAP:
		viewmenu[RX_MENU_WRAP].flags ^= FL_MENU_VALUE;
		wrap_mode(!wrap);
		break;
	}

//	restoreFocus();
}

// caller must free() returned string
char *TextView::get_word(int x, int y)
{
	int p = xy_to_position(x + this->x(), y + this->y(), Fl_Text_Display::CURSOR_POS);
	tbuf->select(word_start(p), word_end(p));
	char *s = tbuf->selection_text();
	tbuf->unselect();

	return s;
}

void TextView::saveFile(void)
{
	char *fn = File_Select("Select ASCII text file", "*.txt", "", 0);
	if (fn)
		tbuf->outputfile(fn, 0, tbuf->length());
}

void TextView::clipboard_copy(void)
{
	if (tbuf->selected()) { // copy selection to primary clipboard
		char *s = tbuf->selection_text();

		Fl::copy(s, strlen(s), 1);
		free(s);
	}
}

void TextView::wrap_mode(bool v)
{
	int row, col;

	xy_to_rowcol(this->x(), this->y(), &row, &col, Fl_Text_Display::CHARACTER_POS);
	Fl_Text_Display::wrap_mode(wrap = v, col);
}


//==============================================================================

Fl_Menu_Item editmenu[] = {
	{"Transmit",		0, 0 },
	{"Receive",		0, 0 },
	{"MFSK16 image...",	0, 0, 0, FL_MENU_DIVIDER},
	{"Clear",		0, 0, },
	{"Cut",			0, 0, },
	{"Copy",		0, 0, },
	{"Paste",		0, 0, 0, FL_MENU_DIVIDER },
	{"Insert file...",	0, 0, 0, FL_MENU_DIVIDER },
	{"Word wrap",		0, 0, 0, FL_MENU_TOGGLE|FL_MENU_VALUE } ,
	{ 0 }
};

Fl_Text_Display::Style_Table_Entry TextEdit::styles[NSTYLES];
int *TextEdit::ptxpos = 0; // needed by our static kf functions

TextEdit::TextEdit(int x, int y, int w, int h, const char *l)
	: Fl_Text_Editor(x, y, w, h, l), PauseBreak(false), txpos(0), bkspaces(0)
{
	ptxpos = &txpos;

	tbuf = new Fl_Text_Buffer;
	sbuf = new Fl_Text_Buffer;

	cursor_style(Fl_Text_Display::NORMAL_CURSOR);
	buffer(tbuf);
	highlight_data(sbuf, styles, NSTYLES, 'A', 0, 0);
	tbuf->add_modify_callback(style_cb, this);

	wrap_mode();

	// set some defaults
	setFont(-1, FL_COURIER);
	setFontSize(-1, 12);
	setFontColor(-1, FL_BLACK);
	setFontColor(2, FL_BLUE);
	setFontColor(3, FL_GREEN);
	setFontColor(4, FL_RED);

	change_keybindings();
}
TextEdit::~TextEdit()
{
	delete tbuf;
	delete sbuf;
}

int TextEdit::handle(int event)
{
	if (event == FL_KEYBOARD) {
		autolock txlock;
		if (handle_key(Fl::event_key()))
			return 1;
		else
			return Fl_Text_Editor::handle(event);
	}
	if (Fl::event_inside( this )) {
		const Fl_Menu_Item * m;
		int xpos = Fl::event_x();
		int ypos = Fl::event_y();

		if (event == FL_PUSH && Fl::event_button() == FL_RIGHT_MOUSE) {
			popx = xpos - x();
			popy = ypos - y();
			m = editmenu->popup(xpos, ypos, 0, 0, 0);
			if (m) {
				int msize = sizeof(editmenu) / sizeof(editmenu[0]);
				for (int i = 0; i < msize; i++)
					if (m == &editmenu[i]) {
						menu_cb(i);
						break;
					}
			}
		}
	}

	return Fl_Text_Editor::handle(event);
}

void TextEdit::add(const char *s, int attr)
{
	tbuf->append(s);
	insert_position(tbuf->length());
}
void TextEdit::clear(void)
{
	autolock lock;

	tbuf->text("");
	sbuf->text("");
	txpos = 0;
	bkspaces = 0;
	PauseBreak = false;
}

int TextEdit::nextChar(void)
{
	autolock txlock;
	char c;

	if (bkspaces) {
		--bkspaces;
		c = '\b';
	}
	else if (PauseBreak) {
		PauseBreak = false;
		c = 0x03;
	}
	else if (insert_position() <= txpos)
		c = '\0';
	else {
		if ((c = tbuf->character(txpos)))
			++txpos;
		tbuf->call_modify_callbacks();
		redraw();
	}

	return c;
}


void TextEdit::setFont(int n, Fl_Font f)
{
	if (n >= 0 && n < NSTYLES)
		styles[n].font = f;
	else if (n == -1)
		for (int i = 0; i < NSTYLES; i++)
			styles[i].font = f;

	redraw();
}
void TextEdit::setFontSize(int n, int s)
{
	if (n >= 0 && n < NSTYLES)
		styles[n].size = s;
	else if (n == -1)
		for (int i = 0; i < NSTYLES; i++)
			styles[i].size = s;
	redraw();
}
void TextEdit::setFontColor(int n, Fl_Color c)
{
	if (n >= 0 && n < NSTYLES)
		styles[n].color = c;
	else if (n == -1)
		for (int i = 0; i < NSTYLES; i++)
			styles[i].color = c;
	redraw();
}
void TextEdit::cursorON(void)
{
	show_cursor();
}


int TextEdit::handle_key(int key)
{
	switch (key) {
	case FL_Escape:
		clear();
		active_modem->set_stopflag(true);
		return 1;
		break;
	case 't': // transmit for C-t
		if (Fl::event_state() & FL_CTRL) {
			menu_cb(TX_MENU_TX);
			return 1;
		}
		break;
	case 'r':// receive for C-r
		if (Fl::event_state() & FL_CTRL) {
			menu_cb(TX_MENU_RX);
			return 1;
		}
		else if (!(Fl::event_state() & (FL_META | FL_ALT)))
			break;
		// fall through to (un)pause for M-r or A-r
	case FL_Pause:
		if (trx_state != STATE_TX) {
			fl_lock(&trx_mutex);
			trx_state = STATE_TX;
			fl_unlock(&trx_mutex);
			wf->set_XmtRcvBtn(true);
		}
		else
			PauseBreak = true;
		return 1;
		break;
	case (FL_KP + '+'):
		if (active_modem == cw_modem)
			active_modem->incWPM();
		return 1;
		break;
	case (FL_KP + '-'):
		if (active_modem == cw_modem)
			active_modem->decWPM();
		return 1;
		break;
	case (FL_KP + '*'):
		if (active_modem == cw_modem)
			active_modem->toggleWPM();
		return 1;
		break;
	case FL_Tab:
		// In non-CW modes: Tab and Ctrl-tab both pause until user moves the
		// cursor to let some more text through. Another (ctrl-)tab goes back to
		// the end of the buffer and resumes sending.

		// In CW mode: Tab pauses, skips rest of buffer, paints it blue, then
		// resumes sending when new text is entered.
		// Ctrl-tab does the same thing as for all other modes.
		insert_position(txpos != insert_position() ? txpos : tbuf->length());

		if (!(Fl::event_state() & FL_CTRL) && active_modem == cw_modem) {
			int n = tbuf->length() - txpos;
			char s[n + 1];

			memset(s, 'C', n);
			s[n] = 0;
			sbuf->replace(txpos, sbuf->length(), s);
			txpos = tbuf->length();
			insert_position(txpos);
		}
		// show_insert_position();
		return 1;
		break;
	case FL_Left:
		if (Fl::event_state() & (FL_META | FL_ALT)) {
			active_modem->searchDown();
			return 1;
		}
		break;
	case FL_Right:
		if (Fl::event_state() & (FL_META | FL_ALT)) {
			active_modem->searchUp();
			return 1;
		}
		break;
	case FL_BackSpace:
	{
		int ipos = insert_position();
		if (txpos > 0 && txpos >= ipos) {
			if (tbuf->length() >= txpos && txpos > ipos)
				return 1;
			bkspaces++;
			txpos--;
		}
		return 0;
	}
	// case '`': // debug
	//	cerr << "bkspaces=" << bkspaces << " tbuf_length=" << tbuf->length()
	//	     << " tx_pos=" << txpos << " cursor_pos=" << insert_position() << endl;
	//	return 1;
	//	break;
	default:
		if (key >= FL_F && key <= FL_F_Last) {
			int b = key - FL_F - 1;
			if (b > 9)
				return 0;

			b += (altMacros ? 10 : 0);
			if (!(macros.text[b]).empty())
				macros.execute(b);

			return 1;
		}

		// do not insert printable characters in the transmitted text
		if (insert_position() < txpos) {
			int d;
			if (Fl::compose(d))
				return 1;
		}
		break;
	}

	return 0;
}

void TextEdit::readFile(void)
{
	char *fn = File_Select("Select ASCII text file", "*.txt", "", 0);
	if (fn) {
		tbuf->appendfile(fn);
		insert_position(tbuf->length());
	}
}

void TextEdit::style_cb(int pos, int nins, int ndel, int nsty, const char *dtext, void *arg)
{
	TextEdit *e = (TextEdit *)arg;

	if (nins == 0 && ndel == 0) {
		if (pos == 0 && nsty == 0) { // update transmitted text style
			char s[2] = { 'E', '\0' };
			e->sbuf->replace(e->txpos - 1, e->txpos, s);
		}

		e->sbuf->unselect();
		return;
	}

	if (nins > 0) {
		char *s = new char[nins + 1];

		memset(s, 'A', nins);
		s[nins] = 0;
		e->sbuf->replace(pos, pos + ndel, s);
		delete [] s;
	}
	else
		e->sbuf->remove(pos, pos + ndel);

	e->sbuf->select(pos, pos + nins - ndel);
}

void TextEdit::menu_cb(int val)
{
	handle(FL_UNFOCUS);
	switch (val) {
	case TX_MENU_TX:
		fl_lock(&trx_mutex);
		trx_state = STATE_TX;
		fl_unlock(&trx_mutex);
		wf->set_XmtRcvBtn(true);
		break;
	case TX_MENU_RX:
		tbuf->append("^r");
		insert_position(tbuf->length());
		break;
	case TX_MENU_MFSK16_IMG:
		if (active_modem->get_mode() == MODE_MFSK16)
			active_modem->makeTxViewer(0, 0);
		break;

	case TX_MENU_CLEAR:
		clear();
		break;
	case TX_MENU_CUT:
		kf_cut(Fl::event_text()[0], this);
		break;
	case TX_MENU_COPY:
		kf_copy(Fl::event_text()[0], this);
		break;
	case TX_MENU_PASTE:
		kf_paste(Fl::event_text()[0], this);
		break;

	case TX_MENU_READ:
		readFile();
		break;

	case TX_MENU_WRAP:
		editmenu[TX_MENU_WRAP].flags ^= FL_MENU_VALUE;
		wrap_mode(!wrap);
		break;
	}
	restoreFocus();
}

void TextEdit::wrap_mode(bool v)
{
	int row, col;

	xy_to_rowcol(this->x(), this->y(), &row, &col, Fl_Text_Display::CHARACTER_POS);
	Fl_Text_Display::wrap_mode(wrap = v, col);
}


void TextEdit::change_keybindings(void)
{
	struct {
		Fl_Text_Editor::Key_Func function;
		Fl_Text_Editor::Key_Func override;
	} fbind[] = { { Fl_Text_Editor::kf_default, TextEdit::kf_default },
		      { Fl_Text_Editor::kf_enter,   TextEdit::kf_enter	 },
		      { Fl_Text_Editor::kf_delete,  TextEdit::kf_delete	 },
		      { Fl_Text_Editor::kf_cut,	    TextEdit::kf_cut	 },
		      { Fl_Text_Editor::kf_paste,   TextEdit::kf_paste	 } };

	int n = sizeof(fbind) / sizeof(fbind[0]);
	for (Fl_Text_Editor::Key_Binding *k = key_bindings; k; k = k->next) {
		for (int i = 0; i < n; i++)
			if (fbind[i].function == k->function)
				k->function = fbind[i].override;
	}
}

int TextEdit::kf_default(int c, Fl_Text_Editor* e)
{
	autolock txlock;
	return e->insert_position() < *ptxpos ? 1 : Fl_Text_Editor::kf_default(c, e);
}

int TextEdit::kf_enter(int c, Fl_Text_Editor* e)
{
	autolock txlock;
	return e->insert_position() < *ptxpos ? 1 : Fl_Text_Editor::kf_enter(c, e);
}

int TextEdit::kf_delete(int c, Fl_Text_Editor* e)
{
	autolock txlock;

	// single character
	if (!e->buffer()->selected())
		return e->insert_position() < *ptxpos ? 1 : Fl_Text_Editor::kf_delete(c, e);

	// region: delete as much as we can
	int start, end;
	e->buffer()->selection_position(&start, &end);
	if (*ptxpos >= end)
		return 1;
	if (*ptxpos > start)
		e->buffer()->select(*ptxpos, end);

	return Fl_Text_Editor::kf_delete(c, e);
}

int TextEdit::kf_cut(int c, Fl_Text_Editor* e)
{
	autolock txlock;

	if (e->buffer()->selected()) {
		int start, end;
		e->buffer()->selection_position(&start, &end);
		if (*ptxpos >= end)
			return 1;
		if (*ptxpos > start)
			e->buffer()->select(*ptxpos, end);
	}

	return Fl_Text_Editor::kf_cut(c, e);
}

int TextEdit::kf_paste(int c, Fl_Text_Editor* e)
{
	autolock txlock;
	return e->insert_position() < *ptxpos ? 1 : Fl_Text_Editor::kf_paste(c, e);
}
