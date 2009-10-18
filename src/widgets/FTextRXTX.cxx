// ----------------------------------------------------------------------------
//      FTextRXTX.cxx
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
#include <sys/stat.h>
#include <map>
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

#include "debug.h"


using namespace std;

static void show_font_warning(FTextBase* w);

Fl_Menu_Item FTextRX::menu[] = {
	{ make_icon_label(_("&Look up call"), net_icon), 0, 0, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL },
	{ make_icon_label(_("&Call"), enter_key_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ make_icon_label(_("&Name"), enter_key_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ make_icon_label(_("QT&H"), enter_key_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ make_icon_label(_("&State"), enter_key_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ make_icon_label(_("&Province"), enter_key_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ make_icon_label(_("Countr&y"), enter_key_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ make_icon_label(_("&Locator"), enter_key_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ make_icon_label(_("&RST(r)"), enter_key_icon), 0, 0, 0,  FL_MENU_DIVIDER, _FL_MULTI_LABEL },
	{ make_icon_label(_("Serial number"), enter_key_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ make_icon_label(_("Exchange In"), enter_key_icon), 0, 0, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL },
	{ make_icon_label(_("Insert marker"), insert_link_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },

	{ 0 }, // VIEW_MENU_COPY
	{ 0 }, // VIEW_MENU_CLEAR
	{ 0 }, // VIEW_MENU_SELECT_ALL
	{ 0 }, // VIEW_MENU_SAVE
	{ 0 }, // VIEW_MENU_WRAP

	{ _("Quick entry"),      0, 0, 0, FL_MENU_TOGGLE, FL_NORMAL_LABEL },
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
        : FTextView(x, y, w, h, l), quick_entry(false)
{
	memcpy(menu + RX_MENU_COPY, FTextView::menu, (FTextView::menu->size() - 1) * sizeof(*FTextView::menu));
	context_menu = menu;
	init_context_menu();
	menu[RX_MENU_QUICK_ENTRY].clear();
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

void FTextRX::set_quick_entry(bool b)
{
	if ((quick_entry = b))
		menu[RX_MENU_QUICK_ENTRY].set();
	else
		menu[RX_MENU_QUICK_ENTRY].clear();
}

void FTextRX::setFont(Fl_Font f, int attr)
{
	FTextBase::setFont(f, attr);
	show_font_warning(this);
}

void FTextRX::handle_clickable(int x, int y)
{
	int pos, style;

	pos = xy_to_position(x + this->x(), y + this->y(), CURSOR_POS);
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
	restoreFocus(NULL);
}

void FTextRX::handle_context_menu(void)
{
	// toggle visibility of quick entry items
	if (menu[RX_MENU_QUICK_ENTRY].value()) {
		menu[RX_MENU_CALL].flags &= ~FL_MENU_DIVIDER;
		for (size_t i = RX_MENU_NAME; i <= RX_MENU_RST_IN; i++)
			menu[i].show();
	}
	else {
		menu[RX_MENU_CALL].flags |= FL_MENU_DIVIDER;
		for (size_t i = RX_MENU_NAME; i <= (progStatus.contest ? RX_MENU_LOC : RX_MENU_RST_IN); i++)
			menu[i].hide();
	}

	set_active(&menu[RX_MENU_COPY], tbuf->selected());
	set_active(&menu[RX_MENU_CLEAR], tbuf->length());
	set_active(&menu[RX_MENU_SELECT_ALL], tbuf->length());
	set_active(&menu[RX_MENU_SAVE], tbuf->length());
	if (wrap)
		menu[RX_MENU_WRAP].set();
	else
		menu[RX_MENU_WRAP].clear();

	if (progStatus.contest) {
			menu[RX_MENU_SERIAL].show();
			menu[RX_MENU_XCHG].show();
	} else {
			menu[RX_MENU_SERIAL].hide();
			menu[RX_MENU_XCHG].hide();
	}

	if (progdefaults.QRZ != QRZNONE)
		menu[RX_MENU_QRZ_THIS].show();
	else
		menu[RX_MENU_QRZ_THIS].hide();
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
		quick_entry = menu[item].value();
		break;

	case RX_MENU_WRAP:
		set_word_wrap(!wrap);
		break;
	}

	restoreFocus(NULL);

	if (!input)
		return;
	char* s = get_word(popx, popy);
	if (!s)
		return;

	if (item == RX_MENU_XCHG) { // append
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

const char* FTextRX::dxcc_lookup_call(int x, int y)
{
	char* s = get_word(x - this->x(), y - this->y());
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
	{ make_icon_label(_("&Transmit"), tx_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ make_icon_label(_("&Receive"), rx_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ make_icon_label(_("&Abort"), process_stop_icon), 0, 0, 0, 0, _FL_MULTI_LABEL },
	{ make_icon_label(_("Send &image..."), image_icon), 0, 0, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL },

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
	bkspaces = 0;
	PauseBreak = false;
}

/// Clears the sent text.
/// Also resets the transmit position, stored backspaces and tx pause flag.
///
void FTextTX::clear_sent(void)
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
int FTextTX::nextChar(void)
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
			REQ(FTextTX::changed_cb, txpos, 0, 0, -1,
			    static_cast<const char *>(0), this);
		}
	}

	return c;
}

void FTextTX::setFont(Fl_Font f, int attr)
{
	FTextBase::setFont(f, attr);
	show_font_warning(this);
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

	key += altMacros * NUMMACKEYS;
	if (!(macros.text[key]).empty())
		macros.execute(key);

	return 1;
}

int FTextTX::handle_dnd_drag(int pos)
{
	if (pos >= txpos)
		return FTextEdit::handle_dnd_drag(pos);
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
		set_word_wrap(!wrap);
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
