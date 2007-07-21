// ----------------------------------------------------------------------------
//
//	TextView.cxx
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

#include <cstring>
#include <cstdlib>

#include "TextView.h"
#include "main.h"

#include "macros.h"
#include "main.h"

#include "cw.h"

#include "File_Selector.h"


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

using namespace std;


/// TextBase constructor.
/// Word wrapping is enabled by default at column 80, but see \c reset_wrap_col.
/// @param x 
/// @param y 
/// @param w 
/// @param h 
/// @param l 
TextBase::TextBase(int x, int y, int w, int h, const char *l)
	: Fl_Text_Editor(x, y, w, h, l), wrap(true), wrap_col(80), max_lines(0)
{
	tbuf = new Fl_Text_Buffer;
	sbuf = new Fl_Text_Buffer;

	cursor_style(Fl_Text_Editor::NORMAL_CURSOR);
	buffer(tbuf);
	highlight_data(sbuf, styles, NSTYLES, DEFAULT, 0, 0);

	wrap_mode(wrap, wrap_col);
// change by W1HKJ
//	scrollbar_width((int)floor(scrollbar_width() * 3.0/4.0));
	scrollbar_width(16);

	// set some defaults
	set_style(NSTYLES, FL_COURIER, 12, FL_FOREGROUND_COLOR);
	set_style(XMT, FL_COURIER, 12, FL_RED, SET_COLOR);
	set_style(SKIP, FL_COURIER, 12, FL_BLUE, SET_COLOR);
	set_style(CTRL, FL_COURIER, 12, FL_DARK_GREEN, SET_COLOR);
}


void TextBase::setFont(Fl_Font f, text_attr_t attr)
{
	set_style(attr, f, textsize(), textcolor(), SET_FONT);
}

void TextBase::setFontSize(int s, text_attr_t attr)
{
	set_style(attr, textfont(), s, textcolor(), SET_SIZE);
}

void TextBase::setFontColor(Fl_Color c, text_attr_t attr)
{
	set_style(attr, textfont(), textsize(), c, SET_COLOR);
}

/// Resizes the text widget.
/// The real work is done by \c Fl_Text_Editor::resize or, if \c HSCROLLBAR_KLUDGE
/// is defined, a version of that code modified so that no horizontal
/// scrollbars are displayed when word wrapping.
///
/// @param X 
/// @param Y 
/// @param W 
/// @param H 
///
void TextBase::resize(int X, int Y, int W, int H)
{
	reset_wrap_col();

#ifdef HSCROLLBAR_KLUDGE
#	include "TextView_resize.cxx"
#else
	Fl_Text_Editor::resize(int X, int Y, int W, int H);
#endif // HSCROLLBAR_KLUDGE
}

/// Changes text style attributes
///
/// @param attr The attribute name to change, or \c NSTYLES to change all styles.
/// @param f The new font
/// @param s The new font size
/// @param c The new font color
/// @param set One or more (OR'd together) SET operations; @see set_style_op_e
///
void TextBase::set_style(text_attr_t attr, Fl_Font f, int s, Fl_Color c, int set)
{
	int start, end;
	if (attr == NSTYLES) { // update all styles
		start = 0;
		end = NSTYLES;
		if (set & SET_FONT)
			textfont(f);
		if (set & SET_SIZE)
			textsize(s);
		if (set & SET_COLOR)
			textcolor(c);
	}
	else {
		start = attr - DEFAULT;
		end = start + 1;
	}
	for (int i = start; i < end; i++) {
		if (set & SET_FONT)
			styles[i].font = f;
		if (set & SET_SIZE)
			styles[i].size = s;
		if (set & SET_COLOR)
			styles[i].color = c;
	}
	resize(x(), y(), w(), h()); // to redraw and recalculate the wrap column
}

/// Reads a file and appends its contents to the buffer.
///
///
void TextBase::readFile(void)
{
	char *fn = File_Select("Select ASCII text file", "*.txt", "", 0);
	if (fn) {
		tbuf->appendfile(fn);
		insert_position(tbuf->length());
		show_insert_position();
	}
}

/// Writes all buffer text out to a file.
///
///
void TextBase::saveFile(void)
{
	char *fn = File_Select("Select ASCII text file", "*.txt", "", 0);
	if (fn)
		tbuf->outputfile(fn, 0, tbuf->length());
}

/// Returns a character string containing the word at (\a x, \a y) relative to
/// the widget's \c x() and \c y().
///
/// @param x 
/// @param y 
///
/// @return The word text at (x,y). <b>Must be freed by the caller</b>.
///
char *TextBase::get_word(int x, int y)
{
	int p = xy_to_position(x + this->x(), y + this->y(),
			       Fl_Text_Display::CURSOR_POS);
	tbuf->select(word_start(p), word_end(p));
	char *s = tbuf->selection_text();
	tbuf->unselect();

	return s;
}

/// Displays the menu pointed to by \c context_menu and calls the menu function;
/// @see call_cb.
///
void TextBase::show_context_menu(void)
{
	const Fl_Menu_Item *m;
	int xpos = Fl::event_x();
	int ypos = Fl::event_y();

	popx = xpos - x();
	popy = ypos - y();
	m = context_menu->popup(xpos, ypos, 0, 0, 0);
	if (!m)
		return;
	for (int i = 0; i < context_menu->size(); ++i) {
		if (m->text == context_menu[i].text) {
			menu_cb(i);
			break;
		}
	}
}

/// Recalculates the wrap margin when the font is changed or the widget resized.
/// At the moment we only handle constant width fonts. Using proportional fonts
/// will result in a small amount of unused space at the end of each line.
///
int TextBase::reset_wrap_col(void)
{
	if (!wrap || wrap_col == 0 || text_area.w == 0)
		return wrap_col;

	int old_wrap_col = wrap_col;

	fl_font(textfont(), textsize());
	wrap_col = (int)floor(text_area.w / fl_width('X'));
	// wrap_mode triggers a resize; don't call it if wrap_col hasn't changed
	if (old_wrap_col != wrap_col)
		wrap_mode(wrap, wrap_col);

	return old_wrap_col;
}


// ----------------------------------------------------------------------------


Fl_Menu_Item TextView::view_menu[] = {
	{ "@-9$returnarrow &QRZ this call",	0, 0 },
	{ "@-9-> &Call",			0, 0 },
	{ "@-9-> &Name",			0, 0 },
	{ "@-9-> QT&H",				0, 0 },
	{ "@-9-> &Locator",			0, 0 },
	{ "@-9-> &RSTin",			0, 0, 0, FL_MENU_DIVIDER },
	{ "Insert divider",			0, 0 },
	{ "C&lear",				0, 0 },
	{ "&Copy",				0, 0, 0, FL_MENU_DIVIDER },
	{ "Save to &file...",			0, 0, 0, FL_MENU_DIVIDER },
	{ "Word &wrap",				0, 0, 0, FL_MENU_TOGGLE	 },
	{ 0 }
};

/// TextView constructor.
/// We remove \c Fl_Text_Display::buffer_modified_cb from the list of callbacks
/// because we want to scroll depending on the visibility of the last line; @see
/// changed_cb.
/// @param x 
/// @param y 
/// @param w 
/// @param h 
/// @param l 
TextView::TextView(int x, int y, int w, int h, const char *l)
	: TextBase(x, y, w, h, l)
{
	tbuf->remove_modify_callback(Fl_Text_Display::buffer_modified_cb, this);
	tbuf->add_modify_callback(changed_cb, this);

	cursor_style(Fl_Text_Display::BLOCK_CURSOR);

	context_menu = view_menu;
	// disable some keybindings that are not allowed in TextView buffers
	change_keybindings();
}

/// Handles fltk events for this widget.

/// We only care about mouse presses (to display the popup menu and prevent
/// pasting) and keyboard events (to make sure no text can be inserted).
/// Everything else is passed to the base class handle().
///
/// @param event 
///
/// @return 
///
int TextView::handle(int event)
{
	switch (event) {
	case FL_PUSH:
		if (!Fl::event_inside(this))
			break;
		// stop mouse2 text paste events from reaching Fl_Text_Editor
		if (Fl::event_button() == FL_MIDDLE_MOUSE)
			return 1;
		if (Fl::event_button() != FL_RIGHT_MOUSE)
			break;

		// enable/disable menu items
		if (tbuf->length())
			view_menu[RX_MENU_CLEAR].flags &= ~FL_MENU_INACTIVE;
		else
			view_menu[RX_MENU_CLEAR].flags |= FL_MENU_INACTIVE;

		if (tbuf->selected())
			view_menu[RX_MENU_COPY].flags &= ~FL_MENU_INACTIVE;
		else
			view_menu[RX_MENU_COPY].flags |= FL_MENU_INACTIVE;
		if (wrap)
			view_menu[RX_MENU_WRAP].flags |= FL_MENU_VALUE;
		else
			view_menu[RX_MENU_WRAP].flags &= ~FL_MENU_VALUE;

		show_context_menu();
		return 1;
		break;
		// catch some text-modifying events that are not handled by kf_* functions
	case FL_KEYBOARD:
		int d;
		if (Fl::compose(d))
			return 1;
		int k = Fl::event_key();
		if (k == FL_BackSpace || k == FL_Tab)
			return 1;
	}

	return TextBase::handle(event);
}

/// Adds a char to the buffer
///
/// @param c The character
/// @param attr The attribute (@see enum text_attr_e); DEFAULT if omitted.
///
void TextView::add(char c, text_attr_t attr)
{
	if (c == '\r')
		return;

	Fl::lock();

	// The user may have moved the cursor by selecting text or
	// scrolling. Place it at the end of the buffer.
	insert_position(tbuf->length());

	switch (c) {
	case '\b':
		// we don't call kf_backspace because it kills selected text
		tbuf->remove(tbuf->length() - 1, tbuf->length());
		sbuf->remove(sbuf->length() - 1, sbuf->length());
		break;
	case '\n':
		// maintain the scrollback limit, if we have one
		if (max_lines > 0 && tbuf->count_lines(0, tbuf->length()) >= max_lines) {
			int le = tbuf->line_end(0) + 1; // plus 1 for the newline
			tbuf->remove(0, le);
			sbuf->remove(0, le);
		}
		// fall-through
	default:
		char s[] = { c, '\0' };
		insert(s);
		s[0] = attr;
		sbuf->append(s);
		break;
	}

	Fl::unlock();
	Fl::awake();
}

/// Appends a string to the buffer
///
/// @param s 
/// @param attr 
///
void TextView::add(const char *s, text_attr_t attr)
{
	while (*s)
		add(*s++, attr);
}

/// The context menu handler
///
/// @param val 
///
void TextView::menu_cb(int val)
{
	handle(FL_UNFOCUS);

	switch (val) {
		char *s;
	case RX_MENU_QRZ_THIS:
		menu_cb(RX_MENU_CALL);
		extern void QRZquery();
		QRZquery();
		break;
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
		kf_copy(Fl::event_key(), this);
		break;

	case RX_MENU_SAVE:
		saveFile();
		break;

	case RX_MENU_WRAP:
		view_menu[RX_MENU_WRAP].flags ^= FL_MENU_VALUE;
		wrap_mode((wrap = !wrap), wrap_col);
		show_insert_position();
		break;
	}
}

/// Scrolls down if the buffer has been modified and the last line is
/// visible. See Fl_Text_Buffer::add_modify_callback() for parameter details.
///
/// @param pos 
/// @param nins 
/// @param ndel 
/// @param nsty 
/// @param dtext 
/// @param arg 
///
void TextView::changed_cb(int pos, int nins, int ndel, int nsty, const char *dtext, void *arg)
{
	TextView *v = (TextView *)arg;

	// In the ctor we removed the Fl_Text_Display callback because we want
	// it to run *before* our callback, so call it now.
	v->buffer_modified_cb(pos, nins, ndel, nsty, dtext, arg);

	// Is the last line visible? To scroll when it isn't would make
	// text selection impossible while receiving.
	if (v->mTopLineNum + v->mNVisibleLines > v->mNBufferLines) {
		if (v->wrap) {
			// v->scroll(v->mNBufferLines, 0);

			// The scrolling code below is a little expensive, but
			// takes care of the only known case where the simple
			// scroll above is not enough. Specifically, the height
			// of the widget and font can be such that the last text
			// line is partially outside the text area, but
			// technically still visible. This can only happen once,
			// until the next newline displays the scrollbar, so I
			// am just being pedantic here.

			// scroll if the last character is vertically outside the text area
			int x, y;
			if (v->position_to_xy(v->tbuf->length() - 1, &x, &y) == 0 ||
			    y + fl_height() >= v->text_area.h)
				v->show_insert_position();
		}
		else
			v->show_insert_position();
	}
}

/// Removes Fl_Text_Edit keybindings that would modify text and put it out of
/// sync with the style buffer. At some point we may decide that we want
/// TextView to be editable (e.g., to insert comments about a QSO), in which
/// case we'll keep the keybindings and add some code to changed_cb to update
/// the style buffer.
///
void TextView::change_keybindings(void)
{
	Fl_Text_Editor::Key_Func fdelete[] = { Fl_Text_Editor::kf_default,
					       Fl_Text_Editor::kf_enter,
					       Fl_Text_Editor::kf_delete,
					       Fl_Text_Editor::kf_cut,
					       Fl_Text_Editor::kf_paste };
	int n = sizeof(fdelete) / sizeof(fdelete[0]);

	// walk the keybindings linked list and delete items containing elements
	// of fdelete
	for (Fl_Text_Editor::Key_Binding *k = key_bindings; k; k = k->next) {
		for (int i = 0; i < n; i++)
			if (k->function == fdelete[i])
				remove_key_binding(k->key, k->state);
	}
}


// ----------------------------------------------------------------------------


Fl_Menu_Item TextEdit::edit_menu[] = {
	{"&Transmit",		0, 0  },
	{"&Receive",		0, 0  },
	{"Send &image...",	0, 0, 0, FL_MENU_DIVIDER },
	{"C&lear",		0, 0, },
	{"Cu&t",		0, 0, },
	{"&Copy",		0, 0, },
	{"&Paste",		0, 0, 0, FL_MENU_DIVIDER },
	{"Append &file...",	0, 0, 0, FL_MENU_DIVIDER },
	{"Word &wrap",		0, 0, 0, FL_MENU_TOGGLE	 } ,
	{ 0 }
};

// needed by our static kf functions, which may restrict editing depending on
// the transmit cursor position
int *TextEdit::ptxpos;

TextEdit::TextEdit(int x, int y, int w, int h, const char *l)
	: TextBase(x, y, w, h, l), PauseBreak(false), txpos(0), bkspaces(0)
{
	ptxpos = &txpos;

	cursor_style(Fl_Text_Display::NORMAL_CURSOR);
	tbuf->add_modify_callback(changed_cb, this);

	context_menu = edit_menu;
	change_keybindings();
}

/// Handles fltk events for this widget.
/// We pass keyboard events to handle_key() and handle mouse3 presses to show
/// the popup menu. We also disallow mouse2 events in the transmitted text area.
/// Everything else is passed to the base class handle().
///
/// @param event 
///
/// @return 
///
int TextEdit::handle(int event)
{
	if (event == FL_KEYBOARD) {
		autolock txlock;
		return handle_key(Fl::event_key()) ? 1 : TextBase::handle(event);
	}
	if (!(Fl::event_inside(this) && event == FL_PUSH))
		return TextBase::handle(event);

	// do not mouse2-paste in the transmitted text
	if (Fl::event_button() == FL_MIDDLE_MOUSE &&
	    xy_to_position(Fl::event_x(), Fl::event_y(),
			   Fl_Text_Display::CHARACTER_POS) < txpos)
		return 1;

	if (Fl::event_button() != FL_RIGHT_MOUSE)
		return TextBase::handle(event);

	// handle a right click
	if (active_modem != mfsk16_modem)
		edit_menu[TX_MENU_MFSK16_IMG].flags |= FL_MENU_INACTIVE;
	else
		edit_menu[TX_MENU_MFSK16_IMG].flags &= ~FL_MENU_INACTIVE;
	if (tbuf->length())
		edit_menu[TX_MENU_CLEAR].flags &= ~FL_MENU_INACTIVE;
	else
		edit_menu[TX_MENU_CLEAR].flags |= FL_MENU_INACTIVE;
	if (tbuf->selected()) {
		edit_menu[TX_MENU_CUT].flags &= ~FL_MENU_INACTIVE;
		edit_menu[TX_MENU_COPY].flags &= ~FL_MENU_INACTIVE;
	}
	else {
		edit_menu[TX_MENU_CUT].flags |= FL_MENU_INACTIVE;
		edit_menu[TX_MENU_COPY].flags |= FL_MENU_INACTIVE;
	}
	if (wrap)
		edit_menu[TX_MENU_WRAP].flags |= FL_MENU_VALUE;
	else
		edit_menu[TX_MENU_WRAP].flags &= ~FL_MENU_VALUE;

	show_context_menu();
	return 1;
}

/// @see TextView::add
///
/// @param s 
/// @param attr 
///
void TextEdit::add(const char *s, text_attr_t attr)
{
	insert(s);

	int n = strlen(s);
	char a[n + 1];
	memset(a, attr, n);
	a[n] = '\0';
	sbuf->replace(sbuf->length() - n, sbuf->length(), a);

	insert_position(tbuf->length());
	show_insert_position();
}

/// Clears the buffer.
/// Also resets the transmit position, stored backspaces and tx pause flag.
///
void TextEdit::clear(void)
{
	autolock lock;

	TextBase::clear();
	txpos = 0;
	bkspaces = 0;
	PauseBreak = false;
}

/// Returns the next character to be transmitted.
///
/// @return The next character, or ETX if the transmission has been paused, or
/// NUL if no text should be transmitted.
///
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
	else if (insert_position() <= txpos) // empty buffer or cursor inside transmitted text
		c = '\0';
	else {
		if ((c = tbuf->character(txpos))) {
			++txpos;
			// we do not call tbuf->call_modify_callbacks() here
			// because we are only updating the style buffer
			changed_cb(0, 0, 0, 0, 0, this);
			redraw();
		}
	}

	return c;
}

/// Handles keyboard events to override Fl_Text_Editor's handling of some
/// keystrokes.
///
/// @param key 
///
/// @return 
///
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

		// In CW mode: Tab pauses, skips rest of buffer, applies the
		// SKIP style, then resumes sending when new text is entered.
		// Ctrl-tab does the same thing as for all other modes.
		insert_position(txpos != insert_position() ? txpos : tbuf->length());

		if (!(Fl::event_state() & FL_CTRL) && active_modem == cw_modem) {
			int n = tbuf->length() - txpos;
			char s[n + 1];

			memset(s, SKIP, n);
			s[n] = 0;
			sbuf->replace(txpos, sbuf->length(), s);
			insert_position(txpos = tbuf->length());
			redraw();
		}
		// show_insert_position();
		return 1;
		break;
		// Move cursor, or search up/down with the Meta/Alt modifiers
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
		// queue a BS and decr. the txpos, unless the cursor is in the tx text
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
		if (key >= FL_F && key <= FL_F_Last) { // insert a macro
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

/// The context menu handler
///
/// @param val 
///
void TextEdit::menu_cb(int val)
{
	handle(FL_UNFOCUS);
	switch (val) {
	case TX_MENU_TX:
		active_modem->set_stopflag(false);
		fl_lock(&trx_mutex);
		trx_state = STATE_TX;
		fl_unlock(&trx_mutex);
		wf->set_XmtRcvBtn(true);
		break;
	case TX_MENU_RX:
		add("^r", CTRL);
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
		edit_menu[TX_MENU_WRAP].flags ^= FL_MENU_VALUE;
		wrap_mode((wrap = !wrap), wrap_col);
		show_insert_position();
		break;
	}
}

/// This function is called by Fl_Text_Buffer when the buffer is modified, and
/// also by nextChar when a character has been passed up the transmit path. In
/// the first case either nins or ndel will be nonzero, and we change a
/// corresponding amount of text in the style buffer.
///
/// In the latter case, nins, ndel, pos and nsty are all zero and we update the
/// style buffer to mark the last character in the buffer with the XMT
/// attribute.
///
/// The select/unselect calls are there to minimize redrawing (copied from
/// Fl_Text_Display/Editor; need to verify that they work as expected).
///
/// @param pos 
/// @param nins 
/// @param ndel 
/// @param nsty 
/// @param dtext 
/// @param arg 
///
void TextEdit::changed_cb(int pos, int nins, int ndel, int nsty, const char *dtext, void *arg)
{
	TextEdit *e = (TextEdit *)arg;

	if (nins == 0 && ndel == 0) {
		if (pos == 0 && nsty == 0) { // update transmitted text style
			char s[2] = { XMT, '\0' };
			e->sbuf->replace(e->txpos - 1, e->txpos, s);
		}

		e->sbuf->unselect();
		return;
	}

	if (nins > 0) { // set the default style for newly inserted text
		char *s = new char[nins + 1];

		memset(s, DEFAULT, nins);
		s[nins] = 0;
		e->sbuf->replace(pos, pos + ndel, s);
		delete [] s;
	}
	else
		e->sbuf->remove(pos, pos + ndel);

	e->sbuf->select(pos, pos + nins - ndel);
}

/// Overrides some useful Fl_Text_Edit keybindings that we want to keep working,
/// provided that they don't try to change chunks of transmitted text.
///
void TextEdit::change_keybindings(void)
{
	struct {
		Fl_Text_Editor::Key_Func function, override;
	} fbind[] = { { Fl_Text_Editor::kf_default, TextEdit::kf_default },
		      { Fl_Text_Editor::kf_enter,   TextEdit::kf_enter	 },
		      { Fl_Text_Editor::kf_delete,  TextEdit::kf_delete	 },
		      { Fl_Text_Editor::kf_cut,	    TextEdit::kf_cut	 },
		      { Fl_Text_Editor::kf_paste,   TextEdit::kf_paste	 } };
	int n = sizeof(fbind) / sizeof(fbind[0]);

	// walk the keybindings linked list and replace items containing
	// functions for which we have an override in fbind
	for (Fl_Text_Editor::Key_Binding *k = key_bindings; k; k = k->next) {
		for (int i = 0; i < n; i++)
			if (fbind[i].function == k->function)
				k->function = fbind[i].override;
	}
}

// The kf_* functions below call the corresponding Fl_Text_Editor routines, but
// may make adjustments so that no transmitted text is modified.

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
