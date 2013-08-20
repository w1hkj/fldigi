// ----------------------------------------------------------------------------
//      FTextRXTX.cxx
//
// Copyright (C) 2007-2010
//              Stelios Bounanos, M0GLD
//
// Copyright (C) 2008-2010
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

#include <string>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <sys/stat.h>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>

#include <FL/Fl_Tooltip.H>

#include "FTextView.h"
#include "main.h"
#include "trx.h"
#include "macros.h"
#include "main.h"
#include "fl_digi.h"

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
#include "strutil.h"
#include "dxcc.h"
#include "locator.h"
#include "logsupport.h"
#include "status.h"
#include "gettext.h"
#include "arq_io.h"

#include "debug.h"


using namespace std;


// Fl_Scrollbar wrapper to draw marks on the slider background.
// Currently only implemented for a vertical scrollbar.
class MVScrollbar : public Fl_Scrollbar
{
	struct mark_t {
		double pos;
		Fl_Color color;
		mark_t(double pos_, Fl_Color color_) : pos(pos_), color(color_) { }
	};

public:
	MVScrollbar(int X, int Y, int W, int H, const char* l = 0)
		: Fl_Scrollbar(X, Y, W, H, l), draw_marks(false) { }

	void draw(void);
	void mark(Fl_Color c) { marks.push_back(mark_t(maximum() - 1.0, c)); redraw(); }
	bool has_marks(void) { return !marks.empty(); }
	void show_marks(bool b) { draw_marks = b; redraw(); }
	void clear(void) { marks.clear(); redraw(); }

private:
	vector<mark_t> marks;
	bool draw_marks;
};

static void show_font_warning(FTextBase* w);

Fl_Menu_Item FTextRX::menu[] = {
	{ make_icon_label(_("Look up call"), net_icon), 0, 0, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL },
	{ make_icon_label(_("Call"), enter_key_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ make_icon_label(_("Name"), enter_key_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ make_icon_label(_("QTH"), enter_key_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ make_icon_label(_("State"), enter_key_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ make_icon_label(_("Province"), enter_key_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ make_icon_label(_("Country"), enter_key_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ make_icon_label(_("Locator"), enter_key_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ make_icon_label(_("RST(r)"), enter_key_icon), 0, 0, 0,  FL_MENU_DIVIDER, _FL_MULTI_LABEL },
	{ make_icon_label(_("Exchange In"), enter_key_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ make_icon_label(_("Serial number"), enter_key_icon), 0, 0, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL },
	{ make_icon_label(_("Insert marker"), insert_link_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },

	{ 0 }, // VIEW_MENU_COPY
	{ 0 }, // VIEW_MENU_CLEAR
	{ 0 }, // VIEW_MENU_SELECT_ALL
	{ 0 }, // VIEW_MENU_SAVE
	{ 0 }, // VIEW_MENU_WRAP

	{ _("Quick entry"),      0, 0, 0, FL_MENU_TOGGLE, FL_NORMAL_LABEL },
	{ _("Scroll hints"),      0, 0, 0, FL_MENU_TOGGLE, FL_NORMAL_LABEL },
	{ 0 }
};

/// FTextRX constructor.
/// We remove \c Fl_Text_Display_mod::buffer_modified_cb from the list of callbacks
/// because we want to scroll depending on the visibility of the last line; @see
/// changed_cb.
/// @param x
/// @param y
/// @param w
/// @param h
/// @param l
FTextRX::FTextRX(int x, int y, int w, int h, const char *l)
        : FTextView(x, y, w, h, l)
{
	memcpy(menu + RX_MENU_COPY, FTextView::menu, (FTextView::menu->size() - 1) * sizeof(*FTextView::menu));
	context_menu = menu;
	init_context_menu();
	menu[RX_MENU_QUICK_ENTRY].clear();
	menu[RX_MENU_SCROLL_HINTS].clear();
	menu[RX_MENU_WRAP].hide();
	// Replace the scrollbar widget
	MVScrollbar* mvsb = new MVScrollbar(mVScrollBar->x(), mVScrollBar->y(),
					    mVScrollBar->w(), mVScrollBar->h(), NULL);
	mvsb->show_marks(false);
	mvsb->callback(mVScrollBar->callback(), mVScrollBar->user_data());
	remove(mVScrollBar);
	delete mVScrollBar;
	Fl_Group::add(mVScrollBar = mvsb);
	mFastDisplay = 1;
}

FTextRX::~FTextRX()
{
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
int FTextRX::handle(int event)
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
		if ((unsigned char)sbuf->byte_at(p) >= CLICK_START + FTEXT_DEF) {
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
		break;
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
	return FTextView::handle(event);
}

/// Adds a char to the buffer
///
/// @param c The character
/// @param attr The attribute (@see enum text_attr_e); RECV if omitted.
///

void FTextRX::add(unsigned int c, int attr)
{
	if (c == '\r')
		return;

	char s[] = { '\0', '\0', char( FTEXT_DEF + attr ), '\0' };
	const char *cp = &s[0];

	// The user may have moved the cursor by selecting text or
	// scrolling. Place it at the end of the buffer.
	if (mCursorPos != tbuf->length())
		insert_position(tbuf->length());

	switch (c) {
	case '\b':
		// we don't call kf_backspace because it kills selected text
		if (s_text.length()) {
			int character_start = tbuf->utf8_align(tbuf->length() - 1);
			int character_length = fl_utf8len1(tbuf->byte_at(character_start));

			tbuf->remove(character_start, tbuf->length());
			sbuf->remove(character_start, sbuf->length());
			s_text.resize(s_text.length() - character_length);
			s_style.resize(s_style.length() - character_length);
		}
		break;
	case '\n':
		// maintain the scrollback limit, if we have one
		if (max_lines > 0 && tbuf->count_lines(0, tbuf->length()) >= max_lines) {
			int le = tbuf->line_end(0) + 1; // plus 1 for the newline
			tbuf->remove(0, le);
			sbuf->remove(0, le);
		}
		s_text.clear();
		s_style.clear();
		insert("\n");
		sbuf->append(s + 2);
		break;
	default:
		if ((c < ' ' || c == 127) && attr != CTRL) // look it up
			cp = ascii[(unsigned char)c];
		else  // insert verbatim
			s[0] = c;

		for (int i = 0; cp[i]; ++i) {
			s_text += cp[i];
			s_style += s[2];
		}

		fl_font( textfont(), textsize() );
		int lwidth = (int)fl_width( s_text.c_str(), s_text.length());
		bool wrapped = false;
		if ( lwidth >= (text_area.w - mVScrollBar->w() - LEFT_MARGIN - RIGHT_MARGIN)) {
			if (c != ' ') {
				size_t p = s_text.rfind(' ');
				if (p != string::npos) {
					s_text.erase(0, p+1);
					s_style.erase(0, p+1);
					if (s_text.length() < 10) { // wrap and delete trailing space
						tbuf->remove(tbuf->length() - s_text.length(), tbuf->length());
						sbuf->remove(sbuf->length() - s_style.length(), sbuf->length());
						insert("\n"); // always insert new line
						sbuf->append(s + 2);
						insert(s_text.c_str());
						sbuf->append(s_style.c_str());
						wrapped = true;
					}
				}
			}
			if (!wrapped) { // add a new line if not wrapped
				insert("\n");
				sbuf->append(s + 2);
				s_text.clear();
				s_style.clear();
				if (c != ' ') { // add character if not a space (no leading spaces)
					for (int i = 0; cp[i]; ++i) {
						sbuf->append(s + 2);
						s_style.append(s + 2);
					}
					s_text.append(cp);
					insert(cp);
				}
			}
		} else {
			for (int i = 0; cp[i]; ++i)
				sbuf->append(s + 2);
			insert(cp);
		}
		break;
	}

// test for bottom of text visibility
	if (// !mFastDisplay && 
		(mVScrollBar->value() >= mNBufferLines - mNVisibleLines + mVScrollBar->linesize() - 1))
		show_insert_position();
}

void FTextRX::set_quick_entry(bool b)
{
	if (b)
		menu[RX_MENU_QUICK_ENTRY].set();
	else
		menu[RX_MENU_QUICK_ENTRY].clear();
}

void FTextRX::set_scroll_hints(bool b)
{
	if (b)
		menu[RX_MENU_SCROLL_HINTS].set();
	else
		menu[RX_MENU_SCROLL_HINTS].clear();
	static_cast<MVScrollbar*>(mVScrollBar)->show_marks(b);
}

void FTextRX::mark(FTextBase::TEXT_ATTR attr)
{
	if (attr == NATTR)
		attr = CLICK_START;
	static_cast<MVScrollbar*>(mVScrollBar)->mark(styles[attr].color);
}

void FTextRX::clear(void)
{
	FTextBase::clear();
	s_text.clear();
	s_style.clear();
	static_cast<MVScrollbar*>(mVScrollBar)->clear();
}

void FTextRX::setFont(Fl_Font f, int attr)
{
	FTextBase::setFont(f, attr);
	show_font_warning(this);
}

void FTextRX::handle_clickable(int x, int y)
{
	int pos;
	unsigned int style;

	pos = xy_to_position(x + this->x(), y + this->y(), CURSOR_POS);
	// return unless clickable style
	if ((style = (unsigned char)sbuf->byte_at(pos)) < CLICK_START + FTEXT_DEF)
		return;

	int start, end;
	for (start = pos-1; start >= 0; start--)
		if ((unsigned char)sbuf->byte_at(start) != style)
			break;
	start++;
	int len = sbuf->length();
	for (end = pos+1; end < len; end++)
		if ((unsigned char)sbuf->byte_at(end) != style)
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

void FTextRX::handle_qsy(int start, int end)
{
	char* text = tbuf->text_range(start, end);

	extern map<string, qrg_mode_t> qrg_marks;
	map<string, qrg_mode_t>::const_iterator i;
	if ((i = qrg_marks.find(text)) != qrg_marks.end()) {
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

void FTextRX::handle_qso_data(int start, int end)
{
	char* s = get_word(start, end, progdefaults.nonwordchars.c_str());
	if (!s)
		return;
	char* p = s;

	Fl_Input* target = 0;

	if (QsoInfoFrame1B->visible()) {
		if (call.match(s)) { // point p to substring
			const regmatch_t& offsets = call.suboff()[1];
			p = s + offsets.rm_so;
			*(s + offsets.rm_eo) = '\0';
			inpCall->value(p);
			inpCall->do_callback();
		} else {
			inpXchgIn->position(inpXchgIn->size());
			if (inpXchgIn->size()) inpXchgIn->insert(" ", 1);
			inpXchgIn->insert(s);
		}
	} else {
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
	}
	free(s);

	restoreFocus(NULL);
}

void FTextRX::handle_context_menu(void)
{
	bool contest_ui = progStatus.Rig_Contest_UI ||
		(progStatus.contest && !progStatus.NO_RIGLOG && !progStatus.Rig_Log_UI);

	unsigned shown = 0;

#define show_item(x_) (shown |=  (1 << x_))
#define hide_item(x_) (shown &= ~(1 << x_))
#define test_item(x_) (shown &   (1 << x_))

	show_item(RX_MENU_CALL);

	if (contest_ui || (progStatus.contest && !progStatus.NO_RIGLOG && !progStatus.Rig_Log_UI)) {
		show_item(RX_MENU_SERIAL);
		show_item(RX_MENU_XCHG);
	}
// "Look up call" shown only in non-contest mode
	else if (progdefaults.QRZWEB != QRZWEBNONE || progdefaults.QRZXML != QRZXMLNONE)
		show_item(RX_MENU_QRZ_THIS);

	if (menu[RX_MENU_QUICK_ENTRY].value()) {
		for (size_t i = RX_MENU_NAME; i <= RX_MENU_RST_IN; i++)
			show_item(i);
		menu[RX_MENU_CALL].flags &= ~FL_MENU_DIVIDER;
	}
	else {
		if (!contest_ui && !progStatus.contest) {
			show_item(RX_MENU_NAME);
			show_item(RX_MENU_QTH);
			show_item(RX_MENU_RST_IN);
		}
		menu[RX_MENU_CALL].flags |= FL_MENU_DIVIDER;
	}

	if (static_cast<MVScrollbar*>(mVScrollBar)->has_marks())
		menu[RX_MENU_SCROLL_HINTS].show();
	else
		menu[RX_MENU_SCROLL_HINTS].hide();

	for (size_t i = RX_MENU_QRZ_THIS; i <= RX_MENU_XCHG; i++) {
		if (test_item(i))
			menu[i].show();
		else
			menu[i].hide();
	}

#undef show_item
#undef hide_item
#undef test_item

	// availability of editing items depend on buffer state
	set_active(&menu[RX_MENU_COPY], tbuf->selected());
	set_active(&menu[RX_MENU_CLEAR], tbuf->length());
	set_active(&menu[RX_MENU_SELECT_ALL], tbuf->length());
	set_active(&menu[RX_MENU_SAVE], tbuf->length());

	if (wrap)
		menu[RX_MENU_WRAP].set();
	else
		menu[RX_MENU_WRAP].clear();

	show_context_menu();
}

/// The context menu handler
///
/// @param val
///
void FTextRX::menu_cb(size_t item)
{
	Fl_Input* input = 0;
	switch (item) {
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
		note_qrg(false, "\n", "\n");
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

	case RX_MENU_SAVE:
		saveFile();
		break;

	case RX_MENU_QUICK_ENTRY:
		menu[item].flags ^= FL_MENU_VALUE;
		if (menu[item].value())
			handle_context_menu();
		break;

	case RX_MENU_WRAP:
		set_word_wrap(!wrap);
		restore_wrap = wrap;
		break;

	case RX_MENU_SCROLL_HINTS:
		menu[item].flags ^= FL_MENU_VALUE;
		static_cast<MVScrollbar*>(mVScrollBar)->show_marks(menu[item].value());
		break;
	}

	restoreFocus(NULL);

	if (!input)
		return;
	char* s = get_word(popx, popy, progdefaults.nonwordchars.c_str());
	if (!s)
		return;

	if (item == RX_MENU_XCHG) { // append
		input->position(input->size());
		if (input->size())
			input->insert(" ", 1);
		input->insert(s);
	}
	else
		input->value(s);
	input->do_callback();
	free(s);
}

const char* FTextRX::dxcc_lookup_call(int x, int y)
{
	char* s = get_word(x - this->x(), y - this->y(), progdefaults.nonwordchars.c_str());
	char* mem = s;
	if (!(s && *s && call.match(s))) {
		free(s);
		return 0;
	}

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

	if (qso && locator2longlat(&lon2, &lat2, qso->getField(GRIDSQUARE)) != RIG_OK)
		lon2 = lat2 = 360.0;

	if (e) {
		// use dxcc data if we didn't have a good locator string in the log file
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

	if (qso) {
		const char* info[] = {
			qso->getField(NAME), qso->getField(QTH), qso->getField(QSO_DATE),
			qso->getField(BAND), qso->getField(MODE)
		};
		// name & qth
		if (*info[0])
			join(stip << "* ", info, 2, _(" in "), true) << '\n';
		// other info
		join(stip << "* " << _("Last QSO") << ": ", info+2, 3, ", ", true) << '\n';
	}
	if (qsl) {
		stip << "* QSL: ";
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

void FTextRX::dxcc_tooltip(void* obj)
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
	FTextRX* v = reinterpret_cast<FTextRX*>(obj);
	// look up word under cursor if we have been called twice with the cursor
	// at the same position, and if the cursor was previously somewhere else
	if (p[2] == p[1] && p[2] != p[0]  &&  ((tip = v->dxcc_lookup_call(p[2].x, p[2].y))))
		Fl_Tooltip::enter_area(v, p[2].x, p[2].y, 100, 100, tip);
	else if (p[2] != p[1])
		Fl_Tooltip::exit(v);

	Fl::repeat_timeout(tip ? Fl_Tooltip::hoverdelay() : v->tooltips.delay / 2.0, dxcc_tooltip, obj);
}


// ----------------------------------------------------------------------------


Fl_Menu_Item FTextTX::menu[] = {
	{ make_icon_label(_("Transmit"), tx_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ make_icon_label(_("Receive"), rx_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ make_icon_label(_("Abort"), process_stop_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ make_icon_label(_("Send image..."), image_icon), 0, 0, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL },

	{ 0 }, // EDIT_MENU_CUT
	{ 0 }, // EDIT_MENU_COPY
	{ 0 }, // EDIT_MENU_PASTE
	{ 0 }, // EDIT_MENU_CLEAR
	{ 0 }, // EDIT_MENU_READ
	{ 0 }, // EDIT_MENU_WRAP

	{ 0 }
};

// needed by our static kf functions, which may restrict editing depending on
// the transmit cursor position
int *FTextTX::ptxpos;

FTextTX::FTextTX(int x, int y, int w, int h, const char *l)
	: FTextEdit(x, y, w, h, l),
          PauseBreak(false), txpos(0), bkspaces(0)
{
	ptxpos = &txpos;

	change_keybindings();

	memcpy(menu + TX_MENU_CUT, FTextEdit::menu, (FTextEdit::menu->size() - 1) * sizeof(*FTextEdit::menu));
	context_menu = menu;
	init_context_menu();
	utf8_txpos = txpos = 0;
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
int FTextTX::handle(int event)
{
	if ( !(Fl::event_inside(this) || (event == FL_KEYBOARD && Fl::focus() == this)) )
		return FTextEdit::handle(event);

	switch (event) {
	case FL_KEYBOARD:
		return handle_key(Fl::event_key()) ? 1 : FTextEdit::handle(event);
	case FL_PUSH:
		if (Fl::event_button() == FL_MIDDLE_MOUSE &&
		    xy_to_position(Fl::event_x(), Fl::event_y(), CHARACTER_POS) < txpos)
			return 1; // ignore mouse2 text pastes inside the transmitted text
	}

	return FTextEdit::handle(event);
}

/// Clears the buffer.
/// Also resets the transmit position, stored backspaces and tx pause flag.
///
void FTextTX::clear(void)
{
	FTextEdit::clear();
	txpos = 0;
	utf8_txpos = 0;
	bkspaces = 0;
	PauseBreak = false;
}

/// Clears the sent text.
/// Also resets the transmit position, stored backspaces and tx pause flag.
///
void FTextTX::clear_sent(void)
{
 	tbuf->remove(0, utf8_txpos);
 	sbuf->remove(0, utf8_txpos);
	txpos = 0;
	utf8_txpos = 0;
	bkspaces = 0;
	PauseBreak = false;
	set_word_wrap(restore_wrap);
}

/// Returns boolean <eot> end of text
///
/// true if empty buffer
/// false if characters remain
///
bool FTextTX::eot(void)
{
	return (insert_position() == txpos);
}

/// Returns the next character to be transmitted.
///
/// @return The next character, or ETX if the transmission has been paused, or
/// NUL if no text should be transmitted.
///
int FTextTX::nextChar(void)
{
	int c;

	if (bkspaces) {
		--bkspaces;
		c = '\b';
	}
	else if (PauseBreak) {
		PauseBreak = false;
		c = GET_TX_CHAR_ETX;//0x03;
	} else if (insert_position() <= utf8_txpos) { // empty buffer or cursor inside transmitted text
		c = -1;
	} else {
		if ((c = tbuf->char_at(utf8_txpos)) > 0) {
			int n = fl_utf8bytes(c);

			REQ(FTextTX::changed_cb, utf8_txpos, 0, 0, -1, static_cast<const char *>(0), this);
			REQ(FTextTX::changed_cb, utf8_txpos+1, 0, 0, -1, static_cast<const char *>(0), this);
			++txpos;
			utf8_txpos += n;
		} else
			c = -1;
	}
	return c;
}

// called by xmlrpc thread
// called by macro execution
void FTextTX::add_text(string s)
{
	for (size_t n = 0; n < s.length(); n++) {
		if (s[n] == '\b') {
			int ipos = insert_position();
			if (tbuf->length()) {
				if (ipos > 0 && txpos == ipos) {
					bkspaces++;
					txpos--;
					int nn;
					tbuf->get_char_at(utf8_txpos, nn);
					utf8_txpos -= nn;
				}
				tbuf->remove(tbuf->length() - 1, tbuf->length());
				sbuf->remove(sbuf->length() - 1, sbuf->length());
				redraw();
			}
		} else {
//LOG_DEBUG("%04x ", s[n] & 0x00FF);
			add(s[n] & 0xFF, RECV);
		}
	}
}

void FTextTX::setFont(Fl_Font f, int attr)
{
	FTextBase::setFont(f, attr);
	show_font_warning(this);
}

/// Handles keyboard shorcuts
///
/// @param key
//   pressed key
///
/// @return
//   1 if shortcut is handled, otherwise 0.
///
int FTextTX::handle_key_shortcuts(int key)
{
	std::string etag = "";

	switch (key) {
	case 'c': // add <CALL> for SC-c
	case 'm': // add <MYCALL> for SC-m
	case 'n': // add <NAME> for SC-n
	case 'r': // add <RST> for SC-r
	case 'l': // add <MYLOC> for SC-l
	case 'h': // add <MYQTH> for SC-h
	case 'a': // add <ANTENNA> for SC-a
		if ((Fl::event_state() & FL_CTRL) && (Fl::event_state() & FL_SHIFT))
//		if ((Fl::event_state() & (FL_CTRL | FL_SHIFT))) // investigate why this doesn't work...
		{
			switch (key)
			{
			case 'c':
				etag = inpCall->value();
				break;
			case 'm':
				etag = progdefaults.myCall;
				break;
			case 'n':
				etag = inpName->value();
				break;
			case 'r':
			        {
					std::string s;
					etag = (s = inpRstIn->value()).length() ? s : std::string("599");
				}
				break;
			case 'l':
				etag = progdefaults.myLocator;
				break;
			case 'h':
				etag = progdefaults.myQth;
				break;
			case 'a':
				etag = progdefaults.myAntenna;
			default:
				break;
			}
			
			// Add text + space if length is > 0
			if (etag.length())
				add_text(etag + std::string(" "));
			
			return 1;
		}
		break;
		
	default:
		break;
	}

	return 0;
}

/// Handles keyboard events to override Fl_Text_Editor_mod's handling of some
/// keystrokes.
///
/// @param key
///
/// @return
///
int FTextTX::handle_key(int key)
{

	if (handle_key_shortcuts(key))
	    return 1;

	switch (key) {
	case FL_Escape: // set stop flag and clear
	{
		static time_t t[2] = { 0, 0 };
		static unsigned char i = 0;
		if (t[i] == time(&t[!i])) { // two presses in a second: abort transmission
			if (trx_state == STATE_TX)
				menu_cb(TX_MENU_ABORT);
			t[i = !i] = 0;
			return 1;
		}
		i = !i;
	}

		if (trx_state == STATE_TX && active_modem->get_stopflag() == false) {
			kf_select_all(0, this);
			kf_copy(0, this);
			clear();
			if (arq_text_available)
				AbortARQ();
			active_modem->set_stopflag(true);
		}

		if (trx_state == STATE_TUNE)
			abort_tx();

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
		if (utf8_txpos != insert_position())
			insert_position(utf8_txpos);
		else
			insert_position(tbuf->length());
		if (!(Fl::event_state() & FL_CTRL) && active_modem == cw_modem) {
			int n = tbuf->length() - utf8_txpos;
			char s[n + 1];
			memset(s, FTEXT_DEF + SKIP, n);
			s[n] = 0;
			sbuf->replace(utf8_txpos, sbuf->length(), s);
			insert_position(tbuf->length());
			redisplay_range(utf8_txpos, insert_position());
			utf8_txpos = insert_position();
		}
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
		if (utf8_txpos > 0 && utf8_txpos == ipos) {
			bkspaces++;
			utf8_txpos = tbuf->prev_char(ipos);
			txpos--;
		}
		return 0;
	}
// alt - 1 / 2 changes macro sets
	case '1':
	case '2':
	case '3':
	case '4':
		if (Fl::event_state() & FL_ALT) {
			static char lbl[2] = "1";
			altMacros = key - '1';
			if (progdefaults.mbar2_pos) {
				if (!altMacros) altMacros = 1;
				for (int i = 0; i < NUMMACKEYS; i++) {
					btnMacro[NUMMACKEYS + i]->label(
						macros.name[(altMacros * NUMMACKEYS) + i].c_str());
					btnMacro[NUMMACKEYS + i]->redraw_label();
				}
				lbl[0] = key;
				btnAltMacros2->label(lbl);
				btnAltMacros2->redraw_label();
			} else {
				for (int i = 0; i < NUMMACKEYS; i++) {
					btnMacro[i]->label(
						macros.name[(altMacros * NUMMACKEYS) + i].c_str());
					btnMacro[i]->redraw_label();
				}
				lbl[0] = key;
				btnAltMacros1->label(lbl);
				btnAltMacros1->redraw_label();
			}
			return 1;
		}
		break;
	default:
		break;
	}

	if (insert_position() < txpos)
		return 1;

// insert a macro
	if (key >= FL_F && key <= FL_F_Last)
		return handle_key_macro(key);

// read ctl-ddd, where d is a digit, as ascii characters (in base 10)
// and insert verbatim; e.g. ctl-001 inserts a <soh>
	if (Fl::event_state() & FL_CTRL && (key >= FL_KP) && (key <= FL_KP + '9'))
		return handle_key_ascii(key);

// restart the numeric keypad entries.
	ascii_cnt = 0;
	ascii_chr = 0;

	return 0;
}

/// Inserts the macro for function key \c key.
///
/// @param key An integer in the range [FL_F, FL_F_Last]
///
/// @return 1
///
int FTextTX::handle_key_macro(int key)
{
	key -= FL_F + 1;
	if (key > 11)
		return 0;

	if (progdefaults.mbar2_pos) {
		if (Fl::event_state(FL_SHIFT))
			key += altMacros * NUMMACKEYS;
	} else {
		key += altMacros * NUMMACKEYS;
	}
	if (!(macros.text[key]).empty())
		macros.execute(key);

	return 1;
}

int FTextTX::handle_dnd_drag(int pos)
{
	if (pos >= txpos) {
		return FTextEdit::handle_dnd_drag(pos);
	}
	else // refuse drop inside transmitted text
		return 0;
}

/// Handles mouse-3 clicks by displaying the context menu
///
/// @param val
///
void FTextTX::handle_context_menu(void)
{
	// adjust Abort/Transmit/Receive menu items
	switch (trx_state) {
	case STATE_TX:
		menu[TX_MENU_TX].hide();
		menu[TX_MENU_RX].show();
		menu[TX_MENU_ABORT].show();
		break;
	case STATE_TUNE:
		menu[TX_MENU_TX].hide();
		menu[TX_MENU_RX].show();
		menu[TX_MENU_ABORT].hide();
		break;
	default:
		menu[TX_MENU_TX].show();
		menu[TX_MENU_RX].hide();
		menu[TX_MENU_ABORT].hide();
		break;
	}

	bool modify_text_ok = insert_position() >= txpos;
	bool selected = tbuf->selected();
 	set_active(&menu[TX_MENU_MFSK16_IMG], active_modem->get_cap() & modem::CAP_IMG);
	set_active(&menu[TX_MENU_CLEAR], tbuf->length());
	set_active(&menu[TX_MENU_CUT], selected && modify_text_ok);
	set_active(&menu[TX_MENU_COPY], selected);
	set_active(&menu[TX_MENU_PASTE], modify_text_ok);
	set_active(&menu[TX_MENU_READ], modify_text_ok);

	if (wrap)
		menu[TX_MENU_WRAP].set();
	else
		menu[TX_MENU_WRAP].clear();

	show_context_menu();
}

/// The context menu handler
///
/// @param val
///
void FTextTX::menu_cb(size_t item)
{
  	switch (item) {
  	case TX_MENU_TX:
		active_modem->set_stopflag(false);
		start_tx();
		break;
	case TX_MENU_ABORT:
		char panic[200];
		snprintf(panic, sizeof(panic), "*** Don't panic *** %s", progdefaults.myName.c_str());
		put_status(panic, 5.0);
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
	case TX_MENU_READ: {
		restore_wrap = wrap;
		set_word_wrap(false);
		readFile();
		break;
	}
	case TX_MENU_WRAP:
		set_word_wrap(!wrap);
		restore_wrap = wrap;
		break;
	}
}

/// Overrides some useful Fl_Text_Edit keybindings that we want to keep working,
/// provided that they don't try to change chunks of transmitted text.
///
void FTextTX::change_keybindings(void)
{
	struct {
		Fl_Text_Editor_mod::Key_Func function, override;
	} fbind[] = {
		{ Fl_Text_Editor_mod::kf_default, FTextTX::kf_default },
		{ Fl_Text_Editor_mod::kf_enter,   FTextTX::kf_enter   },
		{ Fl_Text_Editor_mod::kf_delete,  FTextTX::kf_delete  },
		{ Fl_Text_Editor_mod::kf_cut,     FTextTX::kf_cut     },
		{ Fl_Text_Editor_mod::kf_paste,   FTextTX::kf_paste   }
	};
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

int FTextTX::kf_default(int c, Fl_Text_Editor_mod* e)
{
	return e->insert_position() < *ptxpos ? 1 : Fl_Text_Editor_mod::kf_default(c, e);
}

int FTextTX::kf_enter(int c, Fl_Text_Editor_mod* e)
{
	return e->insert_position() < *ptxpos ? 1 : Fl_Text_Editor_mod::kf_enter(c, e);
}

int FTextTX::kf_delete(int c, Fl_Text_Editor_mod* e)
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

int FTextTX::kf_cut(int c, Fl_Text_Editor_mod* e)
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

int FTextTX::kf_paste(int c, Fl_Text_Editor_mod* e)
{
	return e->insert_position() < *ptxpos ? 1 : Fl_Text_Editor_mod::kf_paste(c, e);
}

static void show_font_warning(FTextBase* w)
{
	Fl_Font f = w->textfont();

	if (Font_Browser::fixed_width(f))
		return;

	// Check if we should generate a warning message
	bool* warn = 0;
	const char* fn = Fl::get_font_name(f);
	if (w == ReceiveText) {
		warn = &progdefaults.RxFontWarn;
		if (progdefaults.RxFontName != fn)
			*warn = true;
	}
	else if (w == TransmitText) {
		warn = &progdefaults.TxFontWarn;
		if (progdefaults.TxFontName != fn)
			*warn = true;
	}
	if (warn && *warn) {
		w->add(Fl::get_font_name(f), FTextBase::XMIT);
		w->add(" is a variable width font.\n", FTextBase::XMIT);
		w->add("Line wrapping with a variable width font may be\n"
		       "too slow. Consider using a fixed width font.\n\n", FTextBase::XMIT);
		*warn = false;
	}
}


// ----------------------------------------------------------------------------


void MVScrollbar::draw(void)
{
	Fl_Scrollbar::draw();

	if (marks.empty() || !draw_marks)
		return;

	assert((type() & FL_HOR_SLIDER) == 0);

	// Calculate the slider knob position and height.  For a vertical scrollbar,
	// the scroll buttons' height is the scrollbar width and the minimum knob
	// height is half that.
	int H = h() - Fl::box_dh(box()) - 2 * w(); // internal height (minus buttons)
	int slider_h = (int)(slider_size() * H + 0.5);
	int min_h = (w() - Fl::box_dw(box())) / 2 + 1;
	if (slider_h < min_h)
		slider_h = min_h;
	double val = (Fl_Slider::value() - minimum()) / (maximum() - minimum());
	int slider_y = (int)(val * (H - slider_h) + 0.5) + w(); // relative to y()
	// This would draw a green rectangle around the slider knob:
	// fl_color(FL_GREEN);
	// fl_rect(x(), y() + slider_y, w() - Fl::box_dw(box()), slider_h);

	int x1 = x() + Fl::box_dx(box()), x2 = x1 + w() - Fl::box_dw(box()) - 1, ypos;
	// Convert stored scrollbar values to vertical positions and draw
	// lines inside the widget if they don't overlap with the knob area.
	for (vector<mark_t>::const_iterator i = marks.begin(); i != marks.end(); ++i) {
		ypos = static_cast<int>(w() + H * i->pos / maximum());
		// Don't draw over slider knob
		if ((ypos > slider_y && ypos < slider_y + slider_h) ||
		    (ypos < slider_y + slider_h && ypos > slider_y))
			continue;
		ypos += y();
		fl_color(i->color);
		fl_line(x1, ypos, x2, ypos);
	}
}
