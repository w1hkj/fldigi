// ----------------------------------------------------------------------------
//      FTextView.cxx
//
// Copyright (C) 2007
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

#include <config.h>

#include <cstring>
#include <cstdlib>

#include "FTextView.h"
#include "main.h"

#include "macros.h"
#include "main.h"

#include "cw.h"

#include "fileselect.h"

#include "ascii.h"
#include "configuration.h"
#include "qrunner.h"


using namespace std;


/// FTextBase constructor.
/// Word wrapping is enabled by default at column 80, but see \c reset_wrap_col.
/// @param x 
/// @param y 
/// @param w 
/// @param h 
/// @param l 
FTextBase::FTextBase(int x, int y, int w, int h, const char *l)
	: ReceiveWidget(x, y, w, h, l),
          wrap(true), wrap_col(80), max_lines(0), scroll_hint(false)
{
	tbuf = new Fl_Text_Buffer;
	sbuf = new Fl_Text_Buffer;

	cursor_style(Fl_Text_Editor_mod::NORMAL_CURSOR);
	buffer(tbuf);
	highlight_data(sbuf, styles, NATTR, FTEXT_DEF, 0, 0);

	wrap_mode(wrap, wrap_col);

	// Do we want narrower scrollbars? The default width is 16.
	// scrollbar_width((int)floor(scrollbar_width() * 3.0/4.0));

	reset_styles(SET_FONT | SET_SIZE | SET_COLOR);
}

int FTextBase::handle(int event)
{
        if (event == FL_MOUSEWHEEL && !Fl::event_inside(this))
                return 1;

        if (event == FL_SHOW) {
                reset_styles(SET_COLOR);
                adjust_colours();
        }

        return ReceiveWidget::handle(event);
}

void FTextBase::setFont(Fl_Font f, int attr)
{
	set_style(attr, f, textsize(), textcolor(), SET_FONT);
}

void FTextBase::setFontSize(int s, int attr)
{
	set_style(attr, textfont(), s, textcolor(), SET_SIZE);
}

void FTextBase::setFontColor(Fl_Color c, int attr)
{
	set_style(attr, textfont(), textsize(), c, SET_COLOR);
}

/// Resizes the text widget.
/// The real work is done by \c Fl_Text_Editor_mod::resize or, if \c HSCROLLBAR_KLUDGE
/// is defined, a version of that code modified so that no horizontal
/// scrollbars are displayed when word wrapping.
///
/// @param X 
/// @param Y 
/// @param W 
/// @param H 
///
void FTextBase::resize(int X, int Y, int W, int H)
{
	reset_wrap_col();

        if (scroll_hint) {
                mTopLineNumHint = mNBufferLines;
                mHorizOffsetHint = 0;
                scroll_hint = false;
        }

	Fl_Text_Editor_mod::resize(X, Y, W, H);
}

/// Checks the new widget height.
/// This is registered with Fl_Tile_check and then called with horizontal
/// and vertical size increments every time the Fl_Tile boundary is moved.
///
/// @param arg The callback argument; should be a pointer to a FTextBase object
/// @param xd The horizontal increment (ignored)
/// @param yd The vertical increment
///
/// @return True if the widget is visible, and the new text area height would be
///         a multiple of the font height.
///
bool FTextBase::wheight_mult_tsize(void *arg, int, int yd)
{
	FTextBase *v = reinterpret_cast<FTextBase *>(arg);
	if (!v->visible())
		return true;
	return v->mMaxsize > 0 && (v->text_area.h + yd) % v->mMaxsize == 0;
}

/// Changes text style attributes
///
/// @param attr The attribute name to change, or \c NATTR to change all styles.
/// @param f The new font
/// @param s The new font size
/// @param c The new font color
/// @param set One or more (OR'd together) SET operations; @see set_style_op_e
///
void FTextBase::set_style(int attr, Fl_Font f, int s, Fl_Color c, int set)
{
	int start, end;

	if (attr == NATTR) { // update all styles
		start = 0;
		end = NATTR;
		if (set & SET_FONT)
			textfont(f);
		if (set & SET_SIZE)
			textsize(s);
		if (set & SET_COLOR)
			textcolor(c);
	}
	else {
		start = attr;
		end = start + 1;
	}
	for (int i = start; i < end; i++) {
		styles[i].attr = 0;
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
void FTextBase::readFile(void)
{
	const char *fn = file_select("Append text", "Text\t*.txt");
	if (fn) {
		tbuf->appendfile(fn);
		insert_position(tbuf->length());
		show_insert_position();
	}
}

/// Writes all buffer text out to a file.
///
///
void FTextBase::saveFile(void)
{
	const char *fn = file_saveas("Save text as", "Text\t*.txt");
	if (fn)
		tbuf->outputfile(fn, 0, tbuf->length());
}

/// Returns a character string containing the selected text, if any,
/// or the word at (\a x, \a y) relative to the widget's \c x() and \c y().
///
/// @param x 
/// @param y 
///
/// @return The selection, or the word text at (x,y). <b>Must be freed by the caller</b>.
///
char *FTextBase::get_word(int x, int y)
{
	if (!tbuf->selected()) {
		int p = xy_to_position(x + this->x(), y + this->y(),
				       Fl_Text_Display_mod::CURSOR_POS);
		tbuf->select(word_start(p), word_end(p));
	}
	char *s = tbuf->selection_text();
	tbuf->unselect();

	return s;
}

/// Displays the menu pointed to by \c context_menu and calls the menu function;
/// @see call_cb.
///
void FTextBase::show_context_menu(void)
{
	const Fl_Menu_Item *m;
	int xpos = Fl::event_x();
	int ypos = Fl::event_y();

	popx = xpos - x();
	popy = ypos - y();
	window()->cursor(FL_CURSOR_DEFAULT);
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
int FTextBase::reset_wrap_col(void)
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

void FTextBase::adjust_colours(void)
{
        // Fl_Text_Display_mod::draw_string may mindlessly clobber our colours with
        // FL_WHITE or FL_BLACK to satisfy contrast requirements. We adjust the
        // luminosity here so that at least we get something resembling the
        // requested hue.
        for (int i = 0; i < NATTR; i++) {
                Fl_Color adj;

                while ((adj = fl_contrast(styles[i].color, color())) != styles[i].color) {
                        styles[i].color = (adj == FL_WHITE) ?
                                          fl_lighter(styles[i].color) :
                                          fl_darker(styles[i].color);
                }
        }
}

void FTextBase::reset_styles(int set)
{
	set_style(NATTR, FL_SCREEN, 12, FL_FOREGROUND_COLOR, set);
	set_style(XMIT, FL_SCREEN, 12, FL_RED, set);
	set_style(CTRL, FL_SCREEN, 12, FL_DARK_GREEN, set);
	set_style(SKIP, FL_SCREEN, 12, FL_BLUE, set);
	set_style(ALTR, FL_SCREEN, 12, FL_DARK_MAGENTA, set);
}

// ----------------------------------------------------------------------------


Fl_Menu_Item FTextView::view_menu[] = {
	{ "@-4>> &Look up call",		0, 0 },
	{ "@-9-> &Call",			0, 0 },
	{ "@-9-> &Name",			0, 0 },
	{ "@-9-> QT&H",				0, 0 },
	{ "@-9-> &Locator",			0, 0 },
	{ "@-9-> &RST(r)",			0, 0, 0, FL_MENU_DIVIDER },
	{ "Insert divider",			0, 0 },
	{ "C&lear",				0, 0 },
	{ "&Copy",				0, 0, 0, FL_MENU_DIVIDER },
#if 0 //#ifndef NDEBUG
        { "(debug) &Append file...",		0, 0, 0, FL_MENU_DIVIDER },
#endif
	{ "Save to &file...",			0, 0, 0, FL_MENU_DIVIDER },
	{ "Word &wrap",				0, 0, 0, FL_MENU_TOGGLE	 },
	{ 0 }
};

/// FTextView constructor.
/// We remove \c Fl_Text_Display_mod::buffer_modified_cb from the list of callbacks
/// because we want to scroll depending on the visibility of the last line; @see
/// changed_cb.
/// @param x 
/// @param y 
/// @param w 
/// @param h 
/// @param l 
FTextView::FTextView(int x, int y, int w, int h, const char *l)
        : ReceiveWidget(x, y, w, h, l), FTextBase(x, y, w, h, l)
{
	tbuf->remove_modify_callback(buffer_modified_cb,
                                     dynamic_cast<Fl_Text_Editor_mod *>(this));
	tbuf->add_modify_callback(changed_cb, this);

	cursor_style(Fl_Text_Display_mod::NORMAL_CURSOR);

	context_menu = view_menu;
	// disable some keybindings that are not allowed in FTextView buffers
	change_keybindings();

	TiledGroup->add_resize_check(FTextView::wheight_mult_tsize, this);
}

FTextView::~FTextView()
{
	TiledGroup->remove_resize_check(FTextView::wheight_mult_tsize, this);
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
int FTextView::handle(int event)
{
	switch (event) {
	case FL_PUSH:
		if (!Fl::event_inside(this))
			break;
		// stop mouse2 text paste events from reaching Fl_Text_Editor_mod
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
		context_menu = progdefaults.QRZ ? view_menu : view_menu + 1;

		show_context_menu();
		return 1;
		break;
		// catch some text-modifying events that are not handled by kf_* functions
	case FL_KEYBOARD:
		int d;
		if (Fl::compose(d))
			return 1;
		int k = Fl::event_key();
		if (k == FL_BackSpace)
			return 1;
		else if (k == FL_Tab)
		    return Fl_Widget::handle(event);
	}

	return FTextBase::handle(event);
}

/// Adds a char to the buffer
///
/// @param c The character
/// @param attr The attribute (@see enum text_attr_e); RECV if omitted.
///
void FTextView::add(unsigned char c, int attr)
{
	if (c == '\r')
		return;

	FL_LOCK_D();

	// The user may have moved the cursor by selecting text or
	// scrolling. Place it at the end of the buffer.
	if (mCursorPos != tbuf->length())
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
		char s[] = { '\0', '\0', FTEXT_DEF + attr, '\0' };
		const char *cp;

		if ((c < ' ' || c == 127) && attr != CTRL) // look it up
			cp = ascii[(unsigned char)c];
		else { // insert verbatim
			s[0] = c;
			cp = &s[0];
		}

		for (int i = 0; cp[i]; ++i)
			sbuf->append(s + 2);
		insert(cp);
		break;
	}

	FL_UNLOCK_D();
	FL_AWAKE_D();
}

/// The context menu handler
///
/// @param val 
///
void FTextView::menu_cb(int val)
{
	if (progdefaults.QRZ == 0)
		++val;

	switch (val) {
		char *s;
	case RX_MENU_QRZ_THIS:
		menu_cb(RX_MENU_CALL);
		extern void CALLSIGNquery();
		CALLSIGNquery();
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
		add("\n	    <<================>>\n", RECV);
		break;
	case RX_MENU_CLEAR:
		clear();
		break;
	case RX_MENU_COPY:
		kf_copy(Fl::event_key(), this);
		break;

#ifndef NDEBUG
        case RX_MENU_READ:
        {
                readFile();
                char *t = tbuf->text();
                int n = tbuf->length();
                memset(t, FTEXT_DEF, n);
                sbuf->text(t);
                free(t);
        }
                break;
#endif

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
inline
void FTextView::changed_cb(int pos, int nins, int ndel, int nsty, const char *dtext, void *arg)
{
	FTextView *v = reinterpret_cast<FTextView *>(arg);

	if (v->mTopLineNum + v->mNVisibleLines - 1 == v->mNBufferLines)
		v->scroll_hint = true;

	v->buffer_modified_cb(pos, nins, ndel, nsty, dtext, dynamic_cast<Fl_Text_Editor_mod *>(v));
}

/// Removes Fl_Text_Edit keybindings that would modify text and put it out of
/// sync with the style buffer. At some point we may decide that we want
/// FTextView to be editable (e.g., to insert comments about a QSO), in which
/// case we'll keep the keybindings and add some code to changed_cb to update
/// the style buffer.
///
void FTextView::change_keybindings(void)
{
	Fl_Text_Editor_mod::Key_Func fdelete[] = { Fl_Text_Editor_mod::kf_default,
					       Fl_Text_Editor_mod::kf_enter,
					       Fl_Text_Editor_mod::kf_delete,
					       Fl_Text_Editor_mod::kf_cut,
					       Fl_Text_Editor_mod::kf_paste };
	int n = sizeof(fdelete) / sizeof(fdelete[0]);

	// walk the keybindings linked list and delete items containing elements
	// of fdelete
loop:
	for (Fl_Text_Editor_mod::Key_Binding *k = key_bindings; k; k = k->next) {
		for (int i = 0; i < n; i++) {
			if (k->function == fdelete[i]) {
				remove_key_binding(k->key, k->state);
				goto loop;
			}
		}
	}
}

// ----------------------------------------------------------------------------


Fl_Menu_Item FTextEdit::edit_menu[] = {
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
int *FTextEdit::ptxpos;

FTextEdit::FTextEdit(int x, int y, int w, int h, const char *l)
	: ReceiveWidget(x, y, w, h, l), FTextBase(x, y, w, h, l),
          TransmitWidget(x, y, w, h, l),
          PauseBreak(false), txpos(0), bkspaces(0)
{
	ptxpos = &txpos;
	cursor_style(Fl_Text_Display_mod::NORMAL_CURSOR);
	tbuf->remove_modify_callback(buffer_modified_cb,
				     dynamic_cast<Fl_Text_Editor_mod *>(this));
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
int FTextEdit::handle(int event)
{
	if (event == FL_KEYBOARD)
		return handle_key(Fl::event_key()) ? 1 : FTextBase::handle(event);

	if (!(Fl::event_inside(this) && event == FL_PUSH))
		return FTextBase::handle(event);

	// ignore mouse2 presses (text pastes) inside the transmitted text
	if (Fl::event_button() == FL_MIDDLE_MOUSE &&
	    xy_to_position(Fl::event_x(), Fl::event_y(),
			   Fl_Text_Display_mod::CHARACTER_POS) < txpos)
		return 1;

	if (Fl::event_button() != FL_RIGHT_MOUSE)
		return FTextBase::handle(event);

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

/// @see FTextView::add
///
/// @param s 
/// @param attr 
///
void FTextEdit::add(const char *s, int attr)
{
	// handle the text attribute first
	int n = strlen(s);
	char a[n + 1];
	memset(a, FTEXT_DEF + attr, n);
	a[n] = '\0';
	sbuf->replace(insert_position(), insert_position() + n, a);

	insert(s);
}

/// @see FTextEdit::add
///
/// @param s 
/// @param attr 
///
void FTextEdit::add(unsigned char c, int attr)
{
	char s[] = { FTEXT_DEF + attr, '\0' };
	sbuf->replace(insert_position(), insert_position() + 1, s);

	s[0] = c;
	insert(s);
}

/// Clears the buffer.
/// Also resets the transmit position, stored backspaces and tx pause flag.
///
void FTextEdit::clear(void)
{
	FTextBase::clear();
	txpos = 0;
	bkspaces = 0;
	PauseBreak = false;
}

/// Clears the sent text.
/// Also resets the transmit position, stored backspaces and tx pause flag.
///
void FTextEdit::clear_sent(void)
{
	tbuf->remove(0, txpos);
	txpos = 0;
	bkspaces = 0;
	PauseBreak = false;
}

/// Returns the next character to be transmitted.
///
/// @return The next character, or ETX if the transmission has been paused, or
/// NUL if no text should be transmitted.
///
int FTextEdit::nextChar(void)
{
	int c;

	if (bkspaces) {
		--bkspaces;
		c = '\b';
	}
	else if (PauseBreak) {
		PauseBreak = false;
		c = 0x03;
	}
	else if (insert_position() <= txpos) // empty buffer or cursor inside transmitted text
		c = -1;
	else {
		if ((c = static_cast<unsigned char>(tbuf->character(txpos)))) {
			++txpos;
			REQ(FTextEdit::changed_cb, txpos, 0, 0, -1,
			    static_cast<const char *>(0), this);
		}
	}

	return c;
}

/// Handles keyboard events to override Fl_Text_Editor_mod's handling of some
/// keystrokes.
///
/// @param key 
///
/// @return 
///
int FTextEdit::handle_key(int key)
{
	switch (key) {
	case FL_Escape: // set stop flag and clear
	{
		static time_t t[2] = { 0, 0 };
		static unsigned char i = 0;
		if (t[i] == time(&t[!i])) { // two presses in a second: reset modem
			trx_start_modem(active_modem);
			t[i = !i] = 0;
			return 1;
		}
		i = !i;
	}
		clear();
		active_modem->set_stopflag(true);
		return 1;
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
	case (FL_KP + '+'):
		if (active_modem == cw_modem)
			active_modem->incWPM();
		return 1;
	case (FL_KP + '-'):
		if (active_modem == cw_modem)
			active_modem->decWPM();
		return 1;
	case (FL_KP + '*'):
		if (active_modem == cw_modem)
			active_modem->toggleWPM();
		return 1;
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
			memset(s, FTEXT_DEF + SKIP, n);
			s[n] = 0;

			sbuf->replace(txpos, sbuf->length(), s);
			insert_position(tbuf->length());
			redisplay_range(txpos, insert_position());
			txpos = insert_position();
		}
		// show_insert_position();
		return 1;
		// Move cursor, or search up/down with the Meta/Alt modifiers
	case FL_Left:
		if (Fl::event_state() & (FL_META | FL_ALT)) {
			active_modem->searchDown();
			return 1;
		}
		return 0;
	case FL_Right:
		if (Fl::event_state() & (FL_META | FL_ALT)) {
			active_modem->searchUp();
			return 1;
		}
		return 0;
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
// alt - 1 through alt - 4 changes macro sets
	case '1':
	case '2':
	case '3':
	case '4':
		if (Fl::event_state() & FL_ALT) {
			altMacros = key - '1';
			for (int i = 0; i < 12; i++)
				btnMacro[i]->label(macros.name[i + (altMacros * NUMMACKEYS)].c_str());
			static char alt_text[4];
			snprintf(alt_text, sizeof(alt_text), "%d", altMacros + 1);
			btnAltMacros->label(alt_text);
			btnAltMacros->redraw_label();
			return 1;
		}
		break;
	default:
		break;
	}

        // insert a macro
        if (key >= FL_F && key <= FL_F_Last && insert_position() >= txpos)
            return handle_key_macro(key);

        // read A/M-ddd, where d is a digit, as ascii characters (in base 10)
        // and insert verbatim; e.g. M-001 inserts a <soh>
        if (Fl::event_state() & FL_CTRL && isdigit(key) &&
            insert_position() >= txpos)
            return handle_key_ascii(key);

        // do not insert printable characters in the transmitted text
        if (insert_position() < txpos) {
            int d;
            if (Fl::compose(d))
                return 1;
        }

	return 0;
}

/// Inserts the macro for function key \c key.
///
/// @param key An integer in the range [FL_F, FL_F_Last]
///
/// @return 1
///
int FTextEdit::handle_key_macro(int key)
{
	key -= FL_F + 1;
	if (key > 11)
		return 0;

	key += altMacros * NUMMACKEYS;
	if (!(macros.text[key]).empty())
		macros.execute(key);

	return 1;
}

/// Composes ascii characters and adds them to the FTextEdit buffer.
/// Control characters are inserted with the CTRL style. Values larger than 127
/// (0x7f) are ignored. We cannot really add NULs for the time being.
/// 
/// @param key A digit character
///
/// @return 1
///
int FTextEdit::handle_key_ascii(int key)
{
	static char ascii_cnt = 0;
	static unsigned ascii_chr = 0;

	key -= '0';
	ascii_cnt++;
	for (int i = 0; i < 3 - ascii_cnt; i++)
		key *= 10;
	ascii_chr += key;
	if (ascii_cnt == 3) {
		if (ascii_chr <= 0x7F)
			add(ascii_chr, (iscntrl(ascii_chr) ? CTRL : RECV));
		ascii_cnt = ascii_chr = 0;
	}

	return 1;
}


/// The context menu handler
///
/// @param val 
///
void FTextEdit::menu_cb(int val)
{
	switch (val) {
	case TX_MENU_TX:
		active_modem->set_stopflag(false);
		fl_lock(&trx_mutex);
		trx_state = STATE_TX;
		fl_unlock(&trx_mutex);
		wf->set_XmtRcvBtn(true);
		break;
	case TX_MENU_RX:
		insert_position(tbuf->length());
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
/// style buffer to mark the last character in the buffer with the XMIT
/// attribute.
///
/// @param pos 
/// @param nins 
/// @param ndel 
/// @param nsty 
/// @param dtext 
/// @param arg 
///
void FTextEdit::changed_cb(int pos, int nins, int ndel, int nsty, const char *dtext, void *arg)
{
	FTextEdit *e = reinterpret_cast<FTextEdit *>(arg);

	if (nins == 0 && ndel == 0) {
		if (nsty == -1) { // called by nextChar to update transmitted text style
			char s[] = { FTEXT_DEF + XMIT, '\0' };
			e->sbuf->replace(pos - 1, pos, s);
			e->redisplay_range(pos - 1, pos);
		}
		else if (nsty > 0) // restyled, e.g. selected, text
			return e->buffer_modified_cb(pos, nins, ndel, nsty, dtext,
						     dynamic_cast<Fl_Text_Editor_mod *>(e));
                // No changes, e.g., a paste with an empty clipboard.
		return;
	}
	else if (nins > 0 && e->sbuf->length() < e->tbuf->length()) {
		// New text not inserted by our add() methods, i.e., via a file
		// read, mouse-2 paste or, most likely, direct keyboard entry.
		int n = e->tbuf->length() - e->sbuf->length();
		if (n == 1) {
			char s[] = { FTEXT_DEF, '\0' };
			e->sbuf->append(s);
		}
		else {
			char *s = new char [n + 1];
			memset(s, FTEXT_DEF, n);
			s[n] = '\0';
			e->sbuf->append(s);
			delete [] s;
		}
	}
	else if (ndel > 0)
		e->sbuf->remove(pos, pos + ndel);

	e->sbuf->select(pos, pos + nins - ndel);

	e->buffer_modified_cb(pos, nins, ndel, nsty, dtext, dynamic_cast<Fl_Text_Editor_mod *>(e));
	// We may need to scroll if the text was inserted by the
	// add() methods, e.g. by a macro
	if (e->mTopLineNum + e->mNVisibleLines - 1 <= e->mNBufferLines)
		e->show_insert_position();
}

/// Overrides some useful Fl_Text_Edit keybindings that we want to keep working,
/// provided that they don't try to change chunks of transmitted text.
///
void FTextEdit::change_keybindings(void)
{
	struct {
		Fl_Text_Editor_mod::Key_Func function, override;
	} fbind[] = { { Fl_Text_Editor_mod::kf_default, FTextEdit::kf_default },
		      { Fl_Text_Editor_mod::kf_enter,   FTextEdit::kf_enter	  },
		      { Fl_Text_Editor_mod::kf_delete,  FTextEdit::kf_delete  },
		      { Fl_Text_Editor_mod::kf_cut,	    FTextEdit::kf_cut	  },
		      { Fl_Text_Editor_mod::kf_paste,   FTextEdit::kf_paste	  } };
	int n = sizeof(fbind) / sizeof(fbind[0]);

	// walk the keybindings linked list and replace items containing
	// functions for which we have an override in fbind
	for (Fl_Text_Editor_mod::Key_Binding *k = key_bindings; k; k = k->next) {
		for (int i = 0; i < n; i++)
			if (fbind[i].function == k->function)
				k->function = fbind[i].override;
	}
}

// The kf_* functions below call the corresponding Fl_Text_Editor_mod routines, but
// may make adjustments so that no transmitted text is modified.

int FTextEdit::kf_default(int c, Fl_Text_Editor_mod* e)
{
	return e->insert_position() < *ptxpos ? 1 : Fl_Text_Editor_mod::kf_default(c, e);
}

int FTextEdit::kf_enter(int c, Fl_Text_Editor_mod* e)
{
	return e->insert_position() < *ptxpos ? 1 : Fl_Text_Editor_mod::kf_enter(c, e);
}

int FTextEdit::kf_delete(int c, Fl_Text_Editor_mod* e)
{
	// single character
	if (!e->buffer()->selected()) {
                if (e->insert_position() >= *ptxpos &&
                    e->insert_position() != e->buffer()->length())
                        return Fl_Text_Editor_mod::kf_delete(c, e);
                else
                        return 1;
        }

	// region: delete as much as we can
	int start, end;
	e->buffer()->selection_position(&start, &end);
	if (*ptxpos >= end)
		return 1;
	if (*ptxpos > start)
		e->buffer()->select(*ptxpos, end);

	return Fl_Text_Editor_mod::kf_delete(c, e);
}

int FTextEdit::kf_cut(int c, Fl_Text_Editor_mod* e)
{
	if (e->buffer()->selected()) {
		int start, end;
		e->buffer()->selection_position(&start, &end);
		if (*ptxpos >= end)
			return 1;
		if (*ptxpos > start)
			e->buffer()->select(*ptxpos, end);
	}

	return Fl_Text_Editor_mod::kf_cut(c, e);
}

int FTextEdit::kf_paste(int c, Fl_Text_Editor_mod* e)
{
	return e->insert_position() < *ptxpos ? 1 : Fl_Text_Editor_mod::kf_paste(c, e);
}
