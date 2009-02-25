// ----------------------------------------------------------------------------
//      FTextView.cxx
//
// Copyright (C) 2007-2008
//              Stelios Bounanos, M0GLD
//
// Copyright (C) 2008
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
#include <map>
#include <sstream>
#include <algorithm>

#include <FL/Fl_Tooltip.H>

#include "FTextView.h"
#include "main.h"

#include "macros.h"
#include "main.h"

#include "cw.h"

#include "fileselect.h"
#include "font_browser.h"

#include "ascii.h"
#include "configuration.h"
#include "qrunner.h"

#include "mfsk.h"
#include "icons.h"
#include "globals.h"
#include "re.h"
#include "dxcc.h"
#include "locator.h"
#include "logsupport.h"
#include "status.h"
#include "gettext.h"

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
	oldw = olds = -1;
	oldf = (Fl_Font)-1;
	textfont(FL_SCREEN);
	textsize(FL_NORMAL_SIZE);
	textcolor(FL_FOREGROUND_COLOR);

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

	// Fl_Text_Editor::handle() calls window()->cursor(FL_CURSOR_DONE) when
	// it receives an FL_KEYBOARD event, which crashes some buggy X drivers
	// (e.g. Intel on the Asus Eee PC).  Call handle_key directly to work
	// around this problem.
	if (event == FL_KEYBOARD)
		return Fl_Text_Editor_mod::handle_key();
	else
		return Fl_Text_Editor_mod::handle(event);
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
	set_style(attr, textfont(), textsize(), c, SET_COLOR | SET_ADJ);
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
	// do we need to recalculate the wrap column?
	if (unlikely(text_area.w != oldw || textfont() != oldf || textsize() != olds)) {
		oldw = text_area.w; oldf = textfont(); olds = textsize();
		reset_wrap_col();
	}
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
		if (set & SET_COLOR) {
			// Fl_Text_Display_mod::draw_string may mindlessly clobber our colours with
			// FL_WHITE or FL_BLACK to satisfy contrast requirements. We adjust the
			// luminosity here so that at least we get something resembling the
			// requested hue.
			if (set & SET_ADJ)
				styles[i].color = adjust_color(c, color());
			else
				styles[i].color = c;
		}
		if (i == SKIP) // clickable styles always same as SKIP for now
			for (int j = CLICK_START; j < NATTR; j++)
				memcpy(&styles[j], &styles[i], sizeof(styles[j]));
	}

	resize(x(), y(), w(), h()); // to redraw and recalculate the wrap column
}

/// Reads a file and inserts its contents.
///
/// @return 0 on success, -1 on error
int FTextBase::readFile(const char* fn)
{
	if ( !(fn || (fn = FSEL::select(_("Insert text"), "Text\t*.txt"))) )
		return -1;

	int ret = 0, pos = insert_position();

#ifdef __CYGWIN__
	FILE* tfile = fopen(fn, "rt");
	if (!tfile)
		return -1;
	char buf[BUFSIZ+1];
	if (pos == tbuf->length()) { // optimise for append
		while (fgets(buf, sizeof(buf), tfile))
			tbuf->append(buf);
		if (ferror(tfile))
			ret = -1;
		pos = tbuf->length();
	}
	else {
		while (fgets(buf, sizeof(buf), tfile)) {
			tbuf->insert(pos, buf);
			pos += strlen(buf);
		}
		if (ferror(tfile))
			ret = -1;
	}
	fclose(tfile);
#else
	if (pos == tbuf->length()) { // optimise for append
		if (tbuf->appendfile(fn) != 0)
			ret = -1;
		pos = tbuf->length();
	}
	else {
		if (tbuf->insertfile(fn, pos) == 0) {
			struct stat st;
			if (stat(fn, &st) == 0)
				pos += st.st_size;
		}
		else
			ret = -1;
	}
#endif
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
#ifdef __CYGWIN__
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
char* FTextBase::get_word(int x, int y, bool ontext)
{
	int p = xy_to_position(x + this->x(), y + this->y(), Fl_Text_Display_mod::CURSOR_POS);
	int start, end;

	if (tbuf->selected()) {
		if (ontext && tbuf->selection_position(&start, &end) && (p < start || p >= end))
			return 0;
		else
			return tbuf->selection_text();
	}

	if (!tbuf->findchars_backward(p, " \t\n", &start))
		start = 0;
	else
		start++;
	if (!tbuf->findchars_forward(p, " .,;|\t\n", &end))
		end = tbuf->length();

	if (ontext && (p < start || p >= end))
		return 0;
	else
		return tbuf->text_range(start, end);
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
	if (m) {
		for (int i = 0; i < context_menu->size(); ++i) {
			if (m == &context_menu[i]) {
				menu_cb(i);
				break;
			}
		}
	}
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
	set_style(NATTR, FL_SCREEN, FL_NORMAL_SIZE, FL_FOREGROUND_COLOR, set);
	set_style(XMIT, FL_SCREEN, FL_NORMAL_SIZE, FL_RED, set);
	set_style(CTRL, FL_SCREEN, FL_NORMAL_SIZE, FL_DARK_GREEN, set);
	set_style(SKIP, FL_SCREEN, FL_NORMAL_SIZE, FL_BLUE, set);
	set_style(ALTR, FL_SCREEN, FL_NORMAL_SIZE, FL_DARK_MAGENTA, set);
}

// ----------------------------------------------------------------------------

#if !USE_IMAGE_LABELS
#  define LOOKUP_SYMBOL "@-4>> "
#  define ENTER_SYMBOL  "@-9-> "
#else
#  define LOOKUP_SYMBOL
#  define ENTER_SYMBOL
#endif

Fl_Menu_Item FTextView::view_menu[] = {
	{ make_icon_label(LOOKUP_SYMBOL _("&Look up call"), net_icon), 0, 0, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL },
	{ make_icon_label(ENTER_SYMBOL _("&Call"), enter_key_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ make_icon_label(ENTER_SYMBOL _("&Name"), enter_key_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ make_icon_label(ENTER_SYMBOL _("QT&H"), enter_key_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ make_icon_label(ENTER_SYMBOL _("&State"), enter_key_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ make_icon_label(ENTER_SYMBOL _("&Province"), enter_key_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ make_icon_label(ENTER_SYMBOL _("Countr&y"), enter_key_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ make_icon_label(ENTER_SYMBOL _("&Locator"), enter_key_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ make_icon_label(ENTER_SYMBOL _("&RST(r)"), enter_key_icon), 0, 0, 0,  FL_MENU_DIVIDER, _FL_MULTI_LABEL },
	{ make_icon_label(ENTER_SYMBOL _("Serial number"), enter_key_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ make_icon_label(ENTER_SYMBOL _("Exchange In"), enter_key_icon), 0, 0, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL },
	{ make_icon_label(_("Insert divider"), insert_link_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ make_icon_label(_("&Copy"), edit_copy_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ make_icon_label(_("C&lear"), edit_clear_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ make_icon_label(_("Select All"), edit_select_all_icon), 0, 0, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL },
#if 0 //#ifndef NDEBUG
        { "(debug) &Append file...", 0, 0, 0, FL_MENU_DIVIDER, FL_NORMAL_LABEL },
#endif
	{ make_icon_label(_("Save &as..."), save_as_icon), 0, 0, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL },
	{ _("Quick entry"),      0, 0, 0, FL_MENU_TOGGLE, FL_NORMAL_LABEL },
	{ _("Word &wrap"),       0, 0, 0, FL_MENU_TOGGLE, FL_NORMAL_LABEL },
	{ 0 }
};
static bool view_init = false;

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

	cursor_style(Fl_Text_Display_mod::NORMAL_CURSOR);

	context_menu = view_menu;
	// disable some keybindings that are not allowed in FTextView buffers
	change_keybindings();

	if (!view_init) {
		for (int i = 0; i < view_menu->size() - 1; i++)
			if (view_menu[i].labeltype() == _FL_MULTI_LABEL)
				set_icon_label(&view_menu[i]);
		view_menu[RX_MENU_QUICK_ENTRY].clear();
	}
	view_init = true;
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
	static Fl_Cursor cursor;

	switch (event) {
	case FL_PUSH:
		if (!Fl::event_inside(this))
			break;
 		switch (Fl::event_button()) {
		case FL_LEFT_MOUSE:
			if (Fl::event_shift() || (unlikely(Fl::event_clicks()) && progdefaults.rxtext_clicks_qso_data)) {
				handle_qso_data(Fl::event_x() - x(), Fl::event_y() - y());
				return 1;
			}
			goto out;
		case FL_MIDDLE_MOUSE:
			if (cursor != FL_CURSOR_HAND)
				handle_qso_data(Fl::event_x() - x(), Fl::event_y() - y());
			return 1;
 		case FL_RIGHT_MOUSE:
			handle_context_menu();
			return 1;
 		default:
 			goto out;
 		}
		break;
	case FL_DRAG:
		if (Fl::event_button() != FL_LEFT_MOUSE)
			return 1;
		break;
	case FL_RELEASE:
		{	int eb = Fl::event_button();
			if (cursor == FL_CURSOR_HAND && eb == FL_LEFT_MOUSE &&
			    Fl::event_is_click() && !Fl::event_clicks()) {
				handle_clickable(Fl::event_x() - x(), Fl::event_y() - y());
				return 1;
			}
			break;
		}
	case FL_MOVE: {
		int p = xy_to_position(Fl::event_x(), Fl::event_y(), Fl_Text_Display_mod::CURSOR_POS);
		if (sbuf->character(p) >= CLICK_START + FTEXT_DEF) {
			if (cursor != FL_CURSOR_HAND)
				window()->cursor(cursor = FL_CURSOR_HAND);
			return 1;
		}
		else
			cursor = FL_CURSOR_INSERT;
		break;
	}
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
	case FL_ENTER:
		if (!progdefaults.rxtext_tooltips || Fl_Tooltip::delay() == 0.0f)
			break;
		tooltips.enabled = Fl_Tooltip::enabled();
		tooltips.delay = Fl_Tooltip::delay();
		Fl_Tooltip::enable(1);
		Fl_Tooltip::delay(0.0f);
		Fl::add_timeout(tooltips.delay / 2.0, dxcc_tooltip, this);
		break;
	case FL_LEAVE:
		if (!progdefaults.rxtext_tooltips || Fl_Tooltip::delay() != 0.0f)
			break;
		Fl_Tooltip::enable(tooltips.enabled);
		Fl_Tooltip::delay(tooltips.delay);
		Fl::remove_timeout(dxcc_tooltip, this);
		break;
	}

out:
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

void FTextView::clear(void)
{
	FTextBase::clear();
	extern map<string, qrg_mode_t> qsy_map;
	qsy_map.clear();
}

void FTextView::handle_clickable(int x, int y)
{
	int pos, style;

	pos = xy_to_position(x + this->x(), y + this->y(), Fl_Text_Display_mod::CURSOR_POS);
	// return unless clickable style
	if ((style = sbuf->character(pos)) < CLICK_START + FTEXT_DEF)
		return;

	int start, end;
	for (start = pos-1; start >= 0; start--)
		if (sbuf->character(start) != style)
			break;
	start++;
	int len = sbuf->length();
	for (end = pos+1; end < len; end++)
		if (sbuf->character(end) != style)
			break;

	switch (style - FTEXT_DEF) {
	case QSY:
		handle_qsy(start, end);
		break;
	// ...
	default:
		break;
	}
}

void FTextView::handle_qsy(int start, int end)
{
	char* text = tbuf->text_range(start, end);

	extern map<string, qrg_mode_t> qsy_map;
	map<string, qrg_mode_t>::const_iterator i;
	if ((i = qsy_map.find(text)) != qsy_map.end()) {
		const qrg_mode_t& m = i->second;
		if (active_modem->get_mode() != m.mode)
			init_modem_sync(m.mode);
		qsy(m.rfcarrier, m.carrier);
	}

	free(text);
}

static fre_t rst("^[1-5][1-9]{2}$", REG_EXTENDED | REG_NOSUB);
static fre_t loc("[a-r]{2}[[:digit:]]{2}([a-x]{2})?", REG_EXTENDED | REG_ICASE);
static fre_t call("([[:alnum:]]?[[:alpha:]/]+[[:digit:]]+[[:alnum:]/]+)", REG_EXTENDED);

void FTextView::handle_qso_data(int start, int end)
{
	char* s = get_word(start, end);
	if (!s)
		return;
	char* p = s;

	Fl_Input* target = 0;

	if (rst.match(s))
		target = inpRstIn;
	else if (loc.match(s))
		target = inpLoc;
	else if (call.match(s)) { // point p to substring
		const regmatch_t& offsets = call.suboff()[1];
		p = s + offsets.rm_so;
		*(s + offsets.rm_eo) = '\0';
		target = inpCall;
	}
	else if (count_if(s, s + strlen(s), static_cast<int(*)(int)>(isdigit)))
		target = inpQth;
	else
		target = *inpName->value() ? inpQth : inpName;

	target->value(p);
	target->do_callback();
	free(s);
}

void FTextView::handle_context_menu(void)
{
	// toggle visibility of quick entry items
	if (view_menu[RX_MENU_QUICK_ENTRY].value()) {
		view_menu[RX_MENU_CALL].flags &= ~FL_MENU_DIVIDER;
		for (size_t i = RX_MENU_NAME; i <= RX_MENU_RST_IN; i++)
			view_menu[i].show();
	}
	else {
		view_menu[RX_MENU_CALL].flags |= FL_MENU_DIVIDER;
		for (size_t i = RX_MENU_NAME; i <= (progStatus.contest ? RX_MENU_LOC : RX_MENU_RST_IN); i++)
			view_menu[i].hide();
	}

	set_active(&view_menu[RX_MENU_COPY], tbuf->selected());
	set_active(&view_menu[RX_MENU_CLEAR], tbuf->length());
	set_active(&view_menu[RX_MENU_SELECT_ALL], tbuf->length());
	set_active(&view_menu[RX_MENU_SAVE], tbuf->length());
	if (wrap)
		view_menu[RX_MENU_WRAP].set();
	else
		view_menu[RX_MENU_WRAP].clear();

	if (progStatus.contest) {
			view_menu[RX_MENU_SERIAL].show();
			view_menu[RX_MENU_XCHG].show();
	} else {
			view_menu[RX_MENU_SERIAL].hide();
			view_menu[RX_MENU_XCHG].hide();
	}

	if (progdefaults.QRZ != QRZNONE)
		view_menu[RX_MENU_QRZ_THIS].show();
	else
		view_menu[RX_MENU_QRZ_THIS].hide();
	show_context_menu();
}

/// The context menu handler
///
/// @param val 
///
void FTextView::menu_cb(int val)
{
	Fl_Input* input = 0;
	switch (val) {
	case RX_MENU_QRZ_THIS:
		menu_cb(RX_MENU_CALL);
		extern void CALLSIGNquery();
		CALLSIGNquery();
		break;
	case RX_MENU_CALL:
		input = inpCall;
		break;
	case RX_MENU_NAME:
		input = inpName;
		break;
	case RX_MENU_QTH:
		input = inpQth;
		break;
	case RX_MENU_STATE:
		input = inpState;
		break;
	case RX_MENU_PROVINCE:
		input = inpVEprov;
		break;
	case RX_MENU_COUNTRY:
		input = inpCountry;
		break;
	case RX_MENU_LOC:
		input = inpLoc;
		break;
	case RX_MENU_RST_IN:
		input = inpRstIn;
		break;

	case RX_MENU_SERIAL:
		input = inpSerNo;
		break;
	case RX_MENU_XCHG:
		input = inpXchgIn;
		break;

	case RX_MENU_DIV:
		note_qrg(false, '\n', '\n');
		break;
	case RX_MENU_COPY:
		kf_copy(Fl::event_key(), this);
		break;
	case RX_MENU_CLEAR:
		clear();
		break;
	case RX_MENU_SELECT_ALL:
		tbuf->select(0, tbuf->length());
		break;

#if 0 //#ifndef NDEBUG
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

	case RX_MENU_QUICK_ENTRY:
		view_menu[val].flags ^= FL_MENU_VALUE;
		quick_entry = view_menu[val].value();
		break;

	case RX_MENU_WRAP:
		view_menu[val].flags ^= FL_MENU_VALUE;
		wrap_mode((wrap = !wrap), wrap_col);
		show_insert_position();
		break;
	}

	if (!input)
		return;
	char* s = get_word(popx, popy);
	if (!s)
		return;

	if (val == RX_MENU_XCHG) { // append
		input->position(input->size());
		input->insert(" ", 1);
		input->insert(s);
	}
	else
		input->value(s);
	input->do_callback();
	free(s);
	if (input == inpCall)
		stopMacroTimer();
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

const char* FTextView::dxcc_lookup_call(int x, int y)
{
	char* s = get_word(x - this->x(), y - this->y());
	char* mem = s;
	if (!(s && *s && call.match(s))) {
		free(s);
		return 0;
	}

	const char *name = 0, *date = 0, *qth = 0, *locator = 0, *mode;
	double lon1, lat1, lon2 = 360.0, lat2 = 360.0, distance, azimuth;
	static string tip;
	ostringstream stip;
	const dxcc* e = 0;
	cQsoRec* qso = 0;
	unsigned char qsl;

	// prevent locator-only lookup if Ctrl is held
	if (!(Fl::event_state() & FL_CTRL) && loc.match(s)) {
		const vector<regmatch_t>& v = loc.suboff();
		s += v[0].rm_so;
		*(s + v[0].rm_eo) = '\0';
		if (locator2longlat(&lon2, &lat2, s) != RIG_OK)
			goto ret;
		e = 0; qsl = 0; qso = 0;
	}
	else {
		e = dxcc_lookup(s);
		qsl = qsl_lookup(s);
		qso = SearchLog(s);
	}

	if (qso) {
		locator = qso->getField(GRIDSQUARE);
		if (!(locator && *locator && locator2longlat(&lon2, &lat2, locator) == RIG_OK))
			lon2 = lat2 = 360.0;
		name = qso->getField(NAME);
		date = qso->getField(QSO_DATE);
		qth  = qso->getField(QTH);
	}
	if (e) {
		if (lon2 == 360.0)
			lon2 = -e->longitude;
		if (lat2 == 360.0)
			lat2 = e->latitude;
		stip << e->country << " (" << e->continent
		     << " GMT" << fixed << showpos << setprecision(1) << -e->gmt_offset << noshowpos
		     << ") CQ-" << e->cq_zone << " ITU-" << e->itu_zone << '\n';
	}

	if (locator2longlat(&lon1, &lat1, progdefaults.myLocator.c_str()) == RIG_OK &&
	    qrb(lon1, lat1, lon2, lat2, &distance, &azimuth) == RIG_OK) {
		stip << "QTE " << fixed << setprecision(0) << azimuth << '\260' << " ("
		     << azimuth_long_path(azimuth) << '\260' << ")  QRB " << distance << "km ("
		     << distance_long_path(distance) << "km)\n";
	}
	if (name && *name) {
		stip << "* " << name;
		if (qth && *qth)
			stip << ' ' << _("in") << ' ' << qth;
		stip << '\n';
	}
	if (date && *date) {
		stip << "* " << _("Last QSO") << ": " << date;
		mode = qso->getField(MODE);
		if (mode)
			stip << ' ' << _("in") << ' ' << mode;
		stip << '\n';
	}
	if (qsl) {
		stip << "* ";
		for (unsigned char i = 0; i < QSL_END; i++)
			if (qsl & (1 << i))
				stip << qsl_names[i] << ' ';
		stip << '\n';
	}

ret:
	free(mem);
	tip = stip.str();
	return tip.empty() ? 0 : tip.c_str();
}

void FTextView::dxcc_tooltip(void* obj)
{
	struct point {
		int x, y;
		bool operator==(const point& p) { return x == p.x && y == p.y; }
		bool operator!=(const point& p) { return !(*this == p); }
	};
	static point p[3] = { {0, 0}, {0, 0}, {0, 0} };

	memmove(p, p+1, 2 * sizeof(point));
	p[2].x = Fl::event_x(); p[2].y = Fl::event_y();

	static const char* tip = 0;
	FTextView* v = reinterpret_cast<FTextView*>(obj);
	// look up word under cursor if we have been called twice with the cursor
	// at the same position, and if the cursor was previously somewhere else
	if (p[2] == p[1] && p[2] != p[0]  &&  ((tip = v->dxcc_lookup_call(p[2].x, p[2].y))))
		Fl_Tooltip::enter_area(v, p[2].x, p[2].y, 100, 100, tip);
	else if (p[2] != p[1])
		Fl_Tooltip::exit(v);

	Fl::repeat_timeout(tip ? Fl_Tooltip::hoverdelay() : v->tooltips.delay / 2.0, dxcc_tooltip, obj);
}


// ----------------------------------------------------------------------------


Fl_Menu_Item FTextEdit::edit_menu[] = {
	{ make_icon_label(_("&Transmit"), tx_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ make_icon_label(_("&Receive"), rx_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ make_icon_label(_("&Abort"), process_stop_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ make_icon_label(_("Send &image..."), image_icon), 0, 0, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL },
	{ make_icon_label(_("Cu&t"), edit_cut_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ make_icon_label(_("&Copy"), edit_copy_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ make_icon_label(_("&Paste"), edit_paste_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ make_icon_label(_("C&lear"), edit_clear_icon), 0, 0, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL },
	{ make_icon_label(_("Insert &file..."), file_open_icon), 0, 0, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL },
	{ _("Word &wrap"), 0, 0, 0, FL_MENU_TOGGLE, FL_NORMAL_LABEL } ,
	{ 0 }
};
static bool edit_init = false;

// needed by our static kf functions, which may restrict editing depending on
// the transmit cursor position
int *FTextEdit::ptxpos;

FTextEdit::FTextEdit(int x, int y, int w, int h, const char *l)
	: FTextBase(x, y, w, h, l),
          PauseBreak(false), txpos(0), bkspaces(0)
{
	ptxpos = &txpos;
	cursor_style(Fl_Text_Display_mod::NORMAL_CURSOR);
	tbuf->remove_modify_callback(buffer_modified_cb, this);

	tbuf->add_modify_callback(changed_cb, this);

	context_menu = edit_menu;
	change_keybindings();
	ascii_cnt = 0;
	ascii_chr = 0;

	if (!edit_init) {
		for (int i = 0; i < edit_menu->size() - 1; i++)
			if (edit_menu[i].labeltype() == _FL_MULTI_LABEL)
				set_icon_label(&edit_menu[i]);
	}
	edit_init = true;
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

	static bool dnd_paste = false;

	switch (event) {
	case FL_KEYBOARD:
		return handle_key(Fl::event_key()) ? 1 : FTextBase::handle(event);
	case FL_DND_RELEASE:
		dnd_paste = true;
		// fall through
	case FL_DND_ENTER: case FL_DND_LEAVE:
		return 1;
	case FL_DND_DRAG:
		return handle_dnd_drag();
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
		else if (eb == FL_MIDDLE_MOUSE && xy_to_position(Fl::event_x(), Fl::event_y(), CHARACTER_POS) < txpos)
			return 1; // ignore mouse2 text pastes inside the transmitted text
	}
	default:
		break;
	}

	return FTextBase::handle(event);
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
		if (t[i] == time(&t[!i])) { // two presses in a second: abort transmission
			menu_cb(TX_MENU_ABORT);
			t[i = !i] = 0;
			return 1;
		}
		i = !i;
	}
		clear();
		active_modem->set_stopflag(true);
		stopMacroTimer();
		return 1;
	case 't': // transmit for C-t
		if (trx_state == STATE_RX && Fl::event_state() & FL_CTRL) {
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
			start_tx();
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

// read ctl-ddd, where d is a digit, as ascii characters (in base 10)
// and insert verbatim; e.g. ctl-001 inserts a <soh>
	if (Fl::event_state() & FL_CTRL && (isdigit(key) || isdigit(key - FL_KP)) &&
		insert_position() >= txpos)
		return handle_key_ascii(key);
	ascii_cnt = 0; // restart the numeric keypad entries.
	ascii_chr = 0;
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
/// @return 1 if a drop is permitted at the cursor position, 0 otherwise
int FTextEdit::handle_dnd_drag(void)
{
	// scroll up or down if the event occured inside the
	// upper or lower 20% of the text area
	if (Fl::event_y() > y() + .8f * h())
		scroll(mVScrollBar->value() + 1, mHorizOffset);
	else if (Fl::event_y() < y() + .2f * h())
		scroll(mVScrollBar->value() - 1, mHorizOffset);

	int pos = xy_to_position(Fl::event_x(), Fl::event_y(), CHARACTER_POS);
	if (pos >= txpos) {
		if (Fl::focus() != this)
			take_focus();
		insert_position(pos);
		return 1;
	}
	else // refuse drop inside received text
		return 0;
}

/// Handles FL_PASTE events by inserting text
///
/// @return 1 or FTextBase::handle(FL_PASTE)
int FTextEdit::handle_dnd_drop(void)
{
	string text = Fl::event_text();
#ifndef __CYGWIN__
	string::size_type cr;
	if (text.find("file://") != string::npos) {
		text.erase(0, 7);
		if ((cr = text.find('\r')) != string::npos)
			text.erase(cr);
		readFile(text.c_str());
		return 1;
	}
	else // insert verbatim
		return FTextBase::handle(FL_PASTE);
#else
	if (readFile(text.c_str()) == -1) // paste as text
		return FTextBase::handle(FL_PASTE);
	else
		return 1;
#endif
}

/// Handles mouse-3 clicks by displaying the context menu
///
/// @param val 
///
void FTextEdit::handle_context_menu(void)
{
	// adjust Abort/Transmit/Receive menu items
	switch (trx_state) {
	case STATE_TX:
		edit_menu[TX_MENU_TX].hide();
		edit_menu[TX_MENU_RX].show();
		edit_menu[TX_MENU_ABORT].show();
		break;
	case STATE_TUNE:
		edit_menu[TX_MENU_TX].hide();
		edit_menu[TX_MENU_RX].show();
		edit_menu[TX_MENU_ABORT].hide();
		break;
	default:
		edit_menu[TX_MENU_TX].show();
		edit_menu[TX_MENU_RX].hide();
		edit_menu[TX_MENU_ABORT].hide();
		break;
	}

	bool modify_text_ok = insert_position() >= txpos;
	bool selected = tbuf->selected();
	set_active(&edit_menu[TX_MENU_MFSK16_IMG], active_modem->get_cap() & modem::CAP_IMG);
	set_active(&edit_menu[TX_MENU_CLEAR], tbuf->length());
	set_active(&edit_menu[TX_MENU_CUT], selected && modify_text_ok);
	set_active(&edit_menu[TX_MENU_COPY], selected);
	set_active(&edit_menu[TX_MENU_PASTE], modify_text_ok);
	set_active(&edit_menu[TX_MENU_READ], modify_text_ok);

	if (wrap)
		edit_menu[TX_MENU_WRAP].set();
	else
		edit_menu[TX_MENU_WRAP].clear();

	show_context_menu();
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
		start_tx();
		break;
	case TX_MENU_ABORT:
#ifndef NDEBUG
		put_status("Don't panic!", 1.0);
#endif
		abort_tx();
  		break;
  	case TX_MENU_RX:
 		if (trx_state == STATE_TX) {
 			insert_position(tbuf->length());
 			add("^r", CTRL);
 		}
 		else
 			abort_tx();
  		break;
  	case TX_MENU_MFSK16_IMG:
  		showTxViewer(0, 0);
		break;
	case TX_MENU_CLEAR:
		clear();
		break;
	case TX_MENU_CUT:
		kf_cut(0, this);
		break;
	case TX_MENU_COPY:
		kf_copy(0, this);
		break;
	case TX_MENU_PASTE:
		kf_paste(0, this);
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

// ----------------------------------------------------------------------------

Fl_Menu_Item FTextLog::log_menu[] = {
	{ make_icon_label(_("C&lear"), edit_clear_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ make_icon_label(_("&Copy"), edit_copy_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ make_icon_label(_("Save to &file..."), save_as_icon), 0, 0, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL },
	{ _("Word &wrap"),	0, 0, 0, FL_MENU_TOGGLE },
	{ 0 }
};
static bool log_init = false;

FTextLog::FTextLog(int x, int y, int w, int h, const char* l)
	: FTextView(x, y, w, h, l)
{
	context_menu = log_menu;
	if (!log_init)
		for (int i = 0; i < log_menu->size() - 1; i++)
			if (log_menu[i].labeltype() == _FL_MULTI_LABEL)
				set_icon_label(&log_menu[i]);
	log_init = true;
}

FTextLog::~FTextLog() { }

int FTextLog::handle(int event)
{
	switch (event) {
	case FL_PUSH:
		if (!Fl::event_inside(this))
			break;
		switch (Fl::event_button()) {
		case FL_LEFT_MOUSE:
			goto out;
		case FL_MIDDLE_MOUSE:
			return 1;
		case FL_RIGHT_MOUSE:
			break;
		}

		// handle a right mouse click
		set_active(&log_menu[LOG_MENU_CLEAR], tbuf->length());
		set_active(&log_menu[LOG_MENU_COPY], tbuf->selected());
		if (wrap)
			log_menu[LOG_MENU_WRAP].set();
		else
			log_menu[LOG_MENU_WRAP].clear();

		show_context_menu();
		return 1;
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

out:
	return FTextBase::handle(event);
}

void FTextLog::add(unsigned char c, int attr)
{
	const char s[] = { c, '\0' };
	tbuf->append(s);
}

void FTextLog::add(const char* s, int attr)
{
	tbuf->append(s);
}

void FTextLog::menu_cb(int val)
{
	switch (val) {
	case LOG_MENU_CLEAR:
		clear();
		break;
	case LOG_MENU_COPY:
		kf_copy(Fl::event_key(), this);
		break;
	case LOG_MENU_SAVE:
		saveFile();
		break;

	case LOG_MENU_WRAP:
		log_menu[LOG_MENU_WRAP].flags ^= FL_MENU_VALUE;
		wrap_mode((wrap = !wrap), wrap_col);
		show_insert_position();
		break;
	}
}
