// ----------------------------------------------------------------------------
//      FTextView.cxx
//
// Copyright (C) 2007-2009
//              Stelios Bounanos, M0GLD
//
// Copyright (C) 2008-2009
//              Dave Freese, W1HKJ
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
#include <cstdio>
#include <cmath>
#include <sys/stat.h>
#include <map>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>

#include <string>

#include <FL/Fl_Tooltip.H>

#include "flmisc.h"
#include "fileselect.h"
#include "font_browser.h"
#include "ascii.h"
#include "icons.h"
#include "gettext.h"

#include "FTextView.h"

#include "debug.h"

using namespace std;


/// FTextBase constructor.
/// Word wrapping is enabled by default at column 80, but see \c reset_wrap_col.
/// @param x 
/// @param y 
/// @param w 
/// @param h 
/// @param l 
FTextBase::FTextBase(int x, int y, int w, int h, const char *l)
	: Fl_Text_Editor_mod(x, y, w, h, l),
          wrap(true), wrap_col(80), max_lines(0), scroll_hint(false)
{
	oldw = oldh = olds = -1;
	oldf = (Fl_Font)-1;
	textfont(FL_COURIER);
	textsize(FL_NORMAL_SIZE);
	textcolor(FL_FOREGROUND_COLOR);

	tbuf = new Fl_Text_Buffer_mod;
	sbuf = new Fl_Text_Buffer_mod;

	buffer(tbuf);
	highlight_data(sbuf, styles, NATTR, FTEXT_DEF, 0, 0);
	cursor_style(Fl_Text_Editor_mod::NORMAL_CURSOR);

	wrap_mode(wrap, wrap_col);
	restore_wrap = wrap;
//	wrap_restore = true;

	// Do we want narrower scrollbars? The default width is 16.
	// scrollbar_width((int)floor(scrollbar_width() * 3.0/4.0));

	reset_styles(SET_FONT | SET_SIZE | SET_COLOR);
}

void FTextBase::clear()
{
	tbuf->text("");
	sbuf->text("");
	set_word_wrap(restore_wrap);
}

int FTextBase::handle(int event)
{
        if (event == FL_MOUSEWHEEL && !Fl::event_inside(this))
                return 1;

	// Fl_Text_Editor::handle() calls window()->cursor(FL_CURSOR_DONE) when
	// it receives an FL_KEYBOARD event, which crashes some buggy X drivers
	// (e.g. Intel on the Asus Eee PC).  Call handle_key directly to work
	// around this problem.
	if (event == FL_KEYBOARD)
		return Fl_Text_Editor_mod::handle_key();
	else
		return Fl_Text_Editor_mod::handle(event);
}

/// @see FTextRX::add
///
/// @param s 
/// @param attr 
///
void FTextBase::add(const char *s, int attr)
{
	// handle the text attribute first
	int n = strlen(s);
	char a[n + 1];
	memset(a, FTEXT_DEF + attr, n);
	a[n] = '\0';
	sbuf->replace(insert_position(), insert_position() + n, a);
	insert(s);
}

/// @see FTextBase::add
///
/// @param s 
/// @param attr 
///
void FTextBase::add(unsigned char c, int attr)
{
	char s[] = { (char)(FTEXT_DEF + attr), '\0' };
	sbuf->replace(insert_position(), insert_position() + 1, s);

	s[0] = c;
	insert(s);
}

void FTextBase::set_word_wrap(bool b)
{
	wrap_mode((wrap = b), wrap_col);
	show_insert_position();
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
	bool need_wrap_reset = false;
	bool need_margin_reset = false;

	if (unlikely(text_area.w != oldw)) {
		oldw = text_area.w;
		need_wrap_reset = true;
	}
	if (unlikely(text_area.h != oldh)) {
		oldh = text_area.h;
		need_margin_reset = true;
	}
	if (unlikely(textfont() != oldf || textsize() != olds)) {
		oldf = textfont();
		olds = textsize();
		need_wrap_reset = need_margin_reset = true;
	}

	if (need_wrap_reset)
		reset_wrap_col();

	TOP_MARGIN = DEFAULT_TOP_MARGIN;
	int r = H - Fl::box_dh(box()) - TOP_MARGIN - BOTTOM_MARGIN;
	if (mHScrollBar->visible())
		r -= scrollbar_width();
	int msize = mMaxsize ? mMaxsize : textsize();
	if (!msize) msize = 1;
//printf("H %d, textsize %d, lines %d, extra %d\n", r, msize, r / msize, r % msize);
	if (r %= msize)
		TOP_MARGIN += r;
	if (scroll_hint) {
		mTopLineNumHint = mNBufferLines;
		mHorizOffsetHint = 0;
//		display_insert_position_hint = 1;
		scroll_hint = false;
	}

	bool hscroll_visible = mHScrollBar->visible();
	Fl_Text_Editor_mod::resize(X, Y, W, H);
	if (hscroll_visible != mHScrollBar->visible())
		oldh = 0; // reset margins next time
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
			Fl_Text_Display_mod::textfont(f);
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
		if (i == SKIP) // clickable styles always same as SKIP for now
			for (int j = CLICK_START; j < NATTR; j++)
				memcpy(&styles[j], &styles[i], sizeof(styles[j]));
	}
	if (set & SET_COLOR)
		mCursor_color = styles[0].color;

	resize(x(), y(), w(), h()); // to redraw and recalculate the wrap column
}

/// Reads a file and inserts its contents.
/// change all occurrences of ^ to ^^ to prevent get_tx_char from
/// treating the carat as a control sequence, ie: ^r ^R ^t ^T ^L ^C
/// get_tx_char passes ^^ as a single ^
///
/// @return 0 on success, -1 on error
int FTextBase::readFile(const char* fn)
{
	set_word_wrap(restore_wrap);

	if ( !(fn || (fn = FSEL::select(_("Insert text"), "Text\t*.txt"))) )
		return -1;

	int ret = 0, pos = insert_position();

#ifdef __WOE32__
	FILE* tfile = fopen(fn, "rt");
#else
	FILE* tfile = fopen(fn, "r");
#endif
	if (!tfile)
		return -1;
	char buf[BUFSIZ+1];
	std::string newbuf;
	size_t p;
	memset(buf, 0, BUFSIZ+1);
	if (pos == tbuf->length()) { // optimise for append
		while (fgets(buf, sizeof(buf), tfile)) {
			newbuf = buf;
			p = 0;
			while ((p = newbuf.find('^',p)) != string::npos) {
				newbuf.insert(p, "^");
				p += 2;
			}
			tbuf->append(newbuf.c_str());
			memset(buf, 0, BUFSIZ+1);
		}
		if (ferror(tfile))
			ret = -1;
		pos = tbuf->length();
	}
	else {
		while (fgets(buf, sizeof(buf), tfile)) {
			newbuf = buf;
			p = 0;
			while ((p = newbuf.find('^',p)) != string::npos) {
				newbuf.insert(p, "^");
				p += 2;
			}
			tbuf->insert(pos, newbuf.c_str());
			pos += strlen(buf);
			memset(buf, 0, BUFSIZ+1);
		}
		if (ferror(tfile))
			ret = -1;
	}
	fclose(tfile);

	insert_position(pos);
	show_insert_position();

	return ret;
}

/// Writes all buffer text out to a file.
///
///
void FTextBase::saveFile(void)
{
 	const char *fn = FSEL::saveas(_("Save text as"), "Text\t*.txt");
	if (fn) {
#ifdef __WOE32__
		ofstream tfile(fn);
		if (!tfile)
			return;

		char *p1, *p2, *text = tbuf->text();
		for (p1 = p2 = text; *p1; p1 = p2) {
			while (*p2 != '\0' && *p2 != '\n')
				p2++;
			if (*p2 == '\n') {
				*p2 = '\0';
				tfile << p1 << "\r\n";
				p2++;

			}
			else
				tfile << p1;
		}
		free(text);
#else
		tbuf->outputfile(fn, 0, tbuf->length());
#endif
	}
}

/// Returns a character string containing the selected word, if any,
/// or the word at (\a x, \a y) relative to the widget's \c x() and \c y().
/// If \a ontext is true, this function will return text only if the
/// mouse cursor position is inside the text range.
///
/// @param x 
/// @param y 
///
/// @return The selection, or the word text at (x,y). <b>Must be freed by the caller</b>.
///
char* FTextBase::get_word(int x, int y, const char* nwchars, bool ontext)
{
	int p = xy_to_position(x + this->x(), y + this->y(), Fl_Text_Display_mod::CURSOR_POS);
	int start, end;

	if (tbuf->selected()) {
		if (ontext && tbuf->selection_position(&start, &end) && (p < start || p >= end))
			return 0;
		else
			return tbuf->selection_text();
	}

	string nonword = nwchars;
	nonword.append(" \t\n");
	if (!tbuf->findchars_backward(p, nonword.c_str(), &start))
		start = 0;
	else
		start++;
	if (!tbuf->findchars_forward(p, nonword.c_str(), &end))
		end = tbuf->length();

	if (ontext && (p < start || p >= end))
		return 0;
	else
		return tbuf->text_range(start, end);
}

/// Initialised the menu pointed to by \c context_menu.  The menu items' user_data
/// field is used to store the initialisation flag.
void FTextBase::init_context_menu(void)
{
	for (int i = 0; i < context_menu->size() - 1; i++) {
		if (context_menu[i].user_data() == 0 &&
		    context_menu[i].labeltype() == _FL_MULTI_LABEL) {
			set_icon_label(&context_menu[i]);
			context_menu[i].user_data(this);
		}
	}
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
	if (m)
		menu_cb(m - context_menu);
}

/// Recalculates the wrap margin when the font is changed or the widget resized.
/// Line wrapping works with proportional fonts but may be very slow.
///
int FTextBase::reset_wrap_col(void)
{
	if (!wrap || text_area.w == 0)
		return wrap_col;

	int old_wrap_col = wrap_col;
	if (Font_Browser::fixed_width(textfont())) {
		fl_font(textfont(), textsize());
		wrap_col = (int)floorf(text_area.w / fl_width('X'));
	}
	else // use slower (but accurate) wrapping for variable width fonts
		wrap_col = 0;
	// wrap_mode triggers a resize; don't call it if wrap_col hasn't changed
	if (old_wrap_col != wrap_col)
		wrap_mode(wrap, wrap_col);

	return old_wrap_col;
}

void FTextBase::reset_styles(int set)
{
	set_style(NATTR, FL_COURIER, FL_NORMAL_SIZE, FL_FOREGROUND_COLOR, set);
	set_style(XMIT, FL_COURIER, FL_NORMAL_SIZE, FL_RED, set);
	set_style(CTRL, FL_COURIER, FL_NORMAL_SIZE, FL_DARK_GREEN, set);
	set_style(SKIP, FL_COURIER, FL_NORMAL_SIZE, FL_BLUE, set);
	set_style(ALTR, FL_COURIER, FL_NORMAL_SIZE, FL_DARK_MAGENTA, set);
}

// ----------------------------------------------------------------------------

Fl_Menu_Item FTextView::menu[] = {
	{ make_icon_label(_("Copy"), edit_copy_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ make_icon_label(_("Clear"), edit_clear_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ make_icon_label(_("Select All"), edit_select_all_icon), 0, 0, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL },
	{ make_icon_label(_("Save as..."), save_as_icon), 0, 0, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL },
	{ _("Word wrap"),       0, 0, 0, FL_MENU_TOGGLE, FL_NORMAL_LABEL },
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
        : FTextBase(x, y, w, h, l), quick_entry(false)
{
	tbuf->remove_modify_callback(buffer_modified_cb, this);
	tbuf->add_modify_callback(changed_cb, this);
	tbuf->canUndo(0);

	// disable some keybindings that are not allowed in FTextView buffers
	change_keybindings();

	context_menu = menu;
	init_context_menu();
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
 		if (Fl::event_button() == FL_RIGHT_MOUSE) {
			handle_context_menu();
			return 1;
 		}
		break;
	case FL_DRAG:
		if (Fl::event_button() != FL_LEFT_MOUSE)
			return 1;
		break;
	// catch some text-modifying events that are not handled by kf_* functions
	case FL_KEYBOARD:
		int k;
		if (Fl::compose(k))
			return 1;
		k = Fl::event_key();
		if (k == FL_BackSpace)
			return 1;
		else if (k == FL_Tab)
			return Fl_Widget::handle(event);
	}

	return FTextBase::handle(event);
}

void FTextView::handle_context_menu(void)
{
	set_active(&menu[VIEW_MENU_COPY], tbuf->selected());
	set_active(&menu[VIEW_MENU_CLEAR], tbuf->length());
	set_active(&menu[VIEW_MENU_SELECT_ALL], tbuf->length());
	set_active(&menu[VIEW_MENU_SAVE], tbuf->length());
	if (wrap)
		menu[VIEW_MENU_WRAP].set();
	else
		menu[VIEW_MENU_WRAP].clear();

	show_context_menu();
}

/// The context menu handler
///
/// @param val 
///
void FTextView::menu_cb(size_t item)
{
	switch (item) {
	case VIEW_MENU_COPY:
		kf_copy(Fl::event_key(), this);
		break;
	case VIEW_MENU_CLEAR:
		clear();
		break;
	case VIEW_MENU_SELECT_ALL:
		tbuf->select(0, tbuf->length());
		break;
	case VIEW_MENU_SAVE:
		saveFile();
		break;
	case VIEW_MENU_WRAP:
		set_word_wrap(!wrap);
		restore_wrap = wrap;
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

	v->buffer_modified_cb(pos, nins, ndel, nsty, dtext, v);
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


Fl_Menu_Item FTextEdit::menu[] = {
	{ make_icon_label(_("Cut"), edit_cut_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ make_icon_label(_("Copy"), edit_copy_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ make_icon_label(_("Paste"), edit_paste_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ make_icon_label(_("Clear"), edit_clear_icon), 0, 0, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL },
	{ make_icon_label(_("Insert file..."), file_open_icon), 0, 0, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL },
	{ _("Word wrap"), 0, 0, 0, FL_MENU_TOGGLE, FL_NORMAL_LABEL } ,
	{ 0 }
};

FTextEdit::FTextEdit(int x, int y, int w, int h, const char *l)
	: FTextBase(x, y, w, h, l)
{
	tbuf->remove_modify_callback(buffer_modified_cb, this);
	tbuf->add_modify_callback(changed_cb, this);

	ascii_cnt = 0;
	ascii_chr = 0;

	context_menu = menu;
	init_context_menu();

	dnd_paste = false;
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
	if ( !(Fl::event_inside(this) || (event == FL_KEYBOARD && Fl::focus() == this)) )
		return FTextBase::handle(event);

	switch (event) {
	case FL_KEYBOARD:
		return handle_key(Fl::event_key()) ? 1 : FTextBase::handle(event);
	case FL_DND_RELEASE:
		dnd_paste = true;
		// fall through
	case FL_DND_ENTER: case FL_DND_LEAVE:
		return 1;
	case FL_DND_DRAG:
		return handle_dnd_drag(xy_to_position(Fl::event_x(), Fl::event_y(), CHARACTER_POS));
	case FL_PASTE:
	{
		int r = dnd_paste ? handle_dnd_drop() : FTextBase::handle(event);
		dnd_paste = false;
		return r;
	}
	case FL_PUSH:
	{
		int eb = Fl::event_button();
		if (eb == FL_RIGHT_MOUSE) {
			handle_context_menu();
			return 1;
		}
	}
	default:
		break;
	}

	return FTextBase::handle(event);
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
// read ctl-ddd, where d is a digit, as ascii characters (in base 10)
// and insert verbatim; e.g. ctl-001 inserts a <soh>
	if (key == FL_Control_L || key == FL_Control_R) return 0;
	bool t1 = isdigit(key);
	bool t2 = false;
	if (key >= FL_KP) t2 = isdigit(key - FL_KP + '0');
	bool t3 = (Fl::event_state() & FL_CTRL) == FL_CTRL;
	if (t3 && (t1 || t2))
		return handle_key_ascii(key);
	ascii_cnt = 0; // restart the numeric keypad entries.
	ascii_chr = 0;
	return 0;
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
	if (key  >= FL_KP)
		key -= FL_KP;
	key -= '0';
	ascii_cnt++;
	for (int i = 0; i < 3 - ascii_cnt; i++)
		key *= 10;
	ascii_chr += key;
	if (ascii_cnt == 3) {
		if (ascii_chr < 0x100) //0x7F)
			add(ascii_chr, (iscntrl(ascii_chr) ? CTRL : RECV));
		ascii_cnt = ascii_chr = 0;
	}

	return 1;
}

/// Handles FL_DND_DRAG events by scrolling and moving the cursor
///
/// @return 1
int FTextEdit::handle_dnd_drag(int pos)
{
	// Scroll if the pointer is being dragged inside the scrollbars,
	// otherwise obtain keyboard focus and set the insert position.
	if (mVScrollBar->visible() && Fl::event_inside(mVScrollBar))
		mVScrollBar->handle(FL_DRAG);
	else if (mHScrollBar->visible() && Fl::event_inside(mHScrollBar))
		mHScrollBar->handle(FL_DRAG);
	else {
		if (Fl::focus() != this)
			take_focus();
		insert_position(pos);
	}

	return 1;
}

/// Handles FL_PASTE events by inserting text
///
/// @return 1 or FTextBase::handle(FL_PASTE)
int FTextEdit::handle_dnd_drop(void)
{
// paste verbatim if the shift key was held down during dnd
	if (Fl::event_shift())
		return FTextBase::handle(FL_PASTE);

	string text;
	string::size_type p, len;

	text = Fl::event_text();

	const char sep[] = "\n";
#if defined(__APPLE__) || defined(__WOE32__)
	text += sep;
#endif

	len = text.length();
	while ((p = text.find(sep)) != string::npos) {
		text[p] = '\0';
#if !defined(__APPLE__) && !defined(__WOE32__)
		if (text.find("file://") == 0) {
			text.erase(0, 7);
			p -= 7;
			len -= 7;
		}
#endif
// paste everything verbatim if we cannot read the first file
LOG_INFO("DnD file %s", text.c_str());
		if (readFile(text.c_str()) == -1 && len == text.length())
			return FTextBase::handle(FL_PASTE);
		text.erase(0, p + sizeof(sep) - 1);
	}

	return 1;
}

/// Handles mouse-3 clicks by displaying the context menu
///
/// @param val 
///
void FTextEdit::handle_context_menu(void)
{
	bool selected = tbuf->selected();
	set_active(&menu[EDIT_MENU_CUT], selected);
	set_active(&menu[EDIT_MENU_COPY], selected);
	set_active(&menu[EDIT_MENU_CLEAR], tbuf->length());

	if (wrap)
		menu[EDIT_MENU_WRAP].set();
	else
		menu[EDIT_MENU_WRAP].clear();

	show_context_menu();
}

/// The context menu handler
///
/// @param val 
///
void FTextEdit::menu_cb(size_t item)
{
  	switch (item) {
	case EDIT_MENU_CLEAR:
		clear();
		break;
	case EDIT_MENU_CUT:
		kf_cut(0, this);
		break;
	case EDIT_MENU_COPY:
		kf_copy(0, this);
		break;
	case EDIT_MENU_PASTE:
		kf_paste(0, this);
		break;
	case EDIT_MENU_READ:
		readFile();
		break;
	case EDIT_MENU_WRAP:
		set_word_wrap(!wrap);
		restore_wrap = wrap;
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
			return e->buffer_modified_cb(pos, nins, ndel, nsty, dtext, e);

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

	e->buffer_modified_cb(pos, nins, ndel, nsty, dtext, e);
	// We may need to scroll if the text was inserted by the
	// add() methods, e.g. by a macro
	if (e->mTopLineNum + e->mNVisibleLines - 1 <= e->mNBufferLines)
		e->show_insert_position();
}
