// ----------------------------------------------------------------------------
//      notify.cxx
//
// Copyright (C) 2009-2010
//              Stelios Bounanos, M0GLD
//
// Generic notifier
//
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
#include <vector>
#include <list>
#include <algorithm>
#include <cstring>
#include <cctype>
#include <cstdlib>

#include "timeops.h"

#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 1)
#  define MAP_TYPE std::tr1::unordered_map
#  include <tr1/unordered_map>
#else
// use the non-standard gnu hash_map on gcc <= 4.0.x,
// which has a broken tr1::unordered_map::operator=
#  define MAP_TYPE __gnu_cxx::hash_map
#  include <ext/hash_map>
namespace __gnu_cxx {
	// define the missing hash specialisation for std::string
	// using the 'const char*' hash function
	template<> struct hash<std::string> {
		size_t operator()(const std::string& s) const { return __stl_hash_string(s.c_str()); }
	};
}
#endif

#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#ifdef __MINGW32__
#  include <windows.h>
#endif

#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Preferences.H>
#include <FL/Fl_Pixmap.H>
#include <FL/Fl.H>
#include <FL/Enumerations.H>
#include <FL/Fl_Button.H>

#include "flinput2.h"
#include "flmisc.h"
#include "macros.h"
#include "debug.h"
#include "dxcc.h"
#include "spot.h"
#include "pskrep.h"
#include "logsupport.h"
#include "re.h"
#include "fileselect.h"
#include "icons.h"
#include "configuration.h"
#include "macroedit.h"
#include "main.h"
#include "fl_digi.h"
#include "waterfall.h"
#include "globals.h"
#include "trx.h"
#include "rsid.h"
#include "gettext.h"
#include "notifydialog.h"
#include "notify.h"

using namespace std;

struct notify_action_t
{
	string alert;
	string rx_marker;
	string macro;
	string program;
	time_t alert_timeout;
	time_t trigger_limit;
};

enum notify_filter_match_t { NOTIFY_FILTER_CALLSIGN, NOTIFY_FILTER_DXCC };
typedef MAP_TYPE<string, bool> notify_filter_dxcc_t;
struct notify_filter_t
{
	notify_filter_match_t match;
	string callsign;
	notify_filter_dxcc_t dxcc;
	string dxcc_last;
	bool nwb, lotw, eqsl;
};

struct notify_dup_t
{
	time_t when;
	band_t band;
	trx_mode mode;
	long long freq;
};

typedef MAP_TYPE<string, notify_dup_t> notify_seen_t;
enum notify_event_t { NOTIFY_EVENT_MYCALL, NOTIFY_EVENT_STATION, NOTIFY_EVENT_CUSTOM, NOTIFY_EVENT_RSID };
struct notify_t
{
	notify_event_t event;
	time_t last_trigger;
	string re;
	bool enabled;
	int afreq;
	long long rfreq;
	trx_mode mode;

	const char* match_string;
	const regmatch_t* submatch_offsets;
	size_t submatch_length;

	notify_dup_t dup;
	bool dup_ignore;
	size_t dup_ref;
	notify_seen_t last_seen;

	notify_action_t action;
	notify_filter_t filter;
};
typedef list<notify_t> notify_list_t;


static void notify_init_window(void);
static void notify_save(void);
static void notify_load(void);
static void notify_register(notify_t& n);
static void notify_unregister(const notify_t& n);
static void notify_set_qsodb_cache(void);

static void notify_event_cb(Fl_Widget* w, void* arg);
static void notify_select_cb(Fl_Widget* w, void* arg);
static void notify_dxcc_browse_cb(Fl_Widget* w, void* arg);
static void notify_add_cb(Fl_Widget* w, void* arg);
static void notify_remove_cb(Fl_Widget* w, void* arg);
static void notify_update_cb(Fl_Widget* w, void* arg);
static void notify_dialog_default_cb(Fl_Widget* w, void* arg);
static void notify_rx_default_cb(Fl_Widget* w, void* arg);
static void notify_macro_edit_cb(Fl_Widget* w, void* arg);
static void notify_program_select_cb(Fl_Widget* w, void* arg);
static void notify_dxcc_check_cb(Fl_Widget* w, void* arg);
static void notify_test_cb(Fl_Widget* w, void* arg);
static void notify_filter_dxcc_select_cb(Fl_Widget* w, void* arg);
static void notify_filter_dxcc_search(Fl_Widget* w, void* arg);
static void notify_dup_ignore_cb(Fl_Widget* w, void* arg);
static void notify_re_cb(Fl_Widget* w, void* arg);

static void notify_recv(trx_mode mode, int afreq, const char* str, const regmatch_t* sub, size_t len, void* data);
static void notify_table_append(const notify_t& n);

////////////////////////////////////////////////////////////////////////////////

struct event_regex_t {
	const char* regex;
	size_t index;
};

static Fl_Menu_Item notify_event_menu[] = {
	{ _("My callsign de CALL") },
	{ _("Station heard twice") },
	{ _("Custom text search") },
	{ _("RSID reception") },
	{ 0 }
};

#define NOTIFY_SET_DUP_MENU (void*)1

enum { NOTIFY_LIST_MENU_TOGGLE, NOTIFY_LIST_MENU_UPDATE, NOTIFY_LIST_MENU_REMOVE };
static Fl_Menu_Item notify_list_context_menu[] = {
	{ make_icon_label(_("Toggle"), shutdown_icon), 0, notify_update_cb, (void*)NOTIFY_LIST_MENU_TOGGLE },
	{ make_icon_label(_("Update"), refresh_icon), 0, notify_update_cb, (void*)NOTIFY_LIST_MENU_UPDATE },
	{ make_icon_label(_("Remove"), minus_icon), 0, notify_remove_cb, (void*)NOTIFY_LIST_MENU_REMOVE },
	{ 0 }
};

enum {
	NOTIFY_DXCC_SELECT_CONT, NOTIFY_DXCC_SELECT_ITU,
	NOTIFY_DXCC_SELECT_CQ, NOTIFY_DXCC_SELECT_ALL,
	NOTIFY_DXCC_DESELECT_CONT, NOTIFY_DXCC_DESELECT_ITU,
	NOTIFY_DXCC_DESELECT_CQ, NOTIFY_DXCC_DESELECT_ALL
};
static Fl_Menu_Item notify_dxcc_context_menu[] = {
	{ _("Select"), 0, 0, 0, FL_SUBMENU },
		{ _("Continent"), 0, notify_filter_dxcc_select_cb, (void*)NOTIFY_DXCC_SELECT_CONT },
		{ _("ITU zone"), 0, notify_filter_dxcc_select_cb, (void*)NOTIFY_DXCC_SELECT_ITU },
		{ _("CQ zone"), 0, notify_filter_dxcc_select_cb, (void*)NOTIFY_DXCC_SELECT_CQ, FL_MENU_DIVIDER },
		{ _("All"), 0, notify_filter_dxcc_select_cb, (void*)NOTIFY_DXCC_SELECT_ALL },
		{ 0 },
	{ _("Deselect"), 0, 0, 0, FL_SUBMENU },
		{ _("Continent"), 0, notify_filter_dxcc_select_cb, (void*)NOTIFY_DXCC_DESELECT_CONT },
		{ _("ITU zone"), 0, notify_filter_dxcc_select_cb, (void*)NOTIFY_DXCC_DESELECT_ITU },
		{ _("CQ zone"), 0, notify_filter_dxcc_select_cb, (void*)NOTIFY_DXCC_DESELECT_CQ, FL_MENU_DIVIDER },
		{ _("All"), 0, notify_filter_dxcc_select_cb, (void*)NOTIFY_DXCC_DESELECT_ALL },
		{ 0 },
	{ 0 }
};

static event_regex_t event_regex[] = {
	{ "<MYCALL>.+de[[:space:]]+(<CALLSIGN_RE>)", 1 },
	{ PSKREP_RE, PSKREP_RE_INDEX },
	{ "", 0 },
	{ "", 1 }
};

static const char* default_alert_text[] = {
	"$CALLSIGN is calling you\n    $TEXT\nTime: %X %Z (%z)\nMode: $MODEM @ $RF_KHZ KHz",
	"Heard $CALLSIGN ($COUNTRY)\n    $TEXT\nTime: %X %Z (%z)\nMode: $MODEM @ $RF_KHZ KHz",
	"",
	"RSID received\nMode: $MODEM @ $RF_KHZ KHz\nTime: %X %Z (%z)",
};

static Fl_Menu_Item notify_dup_callsign_menu[] = {
	{ "" }, { "" }, { "" }, { "" }, { "" }, { "" }, { "" }, { "" }, { "" }, { "" }, { 0 }
};

static Fl_Menu_Item notify_dup_refs_menu[] = {
	{ "Substring \\0" }, { "Substring \\1" }, { "Substring \\2" }, { "Substring \\3" },
	{ "Substring \\4" }, { "Substring \\5" }, { "Substring \\6" }, { "Substring \\7" },
	{ "Substring \\8" }, { "Substring \\9" }, { 0 }
};

enum {
	NOTIFY_DXCC_COL_SEL, NOTIFY_DXCC_COL_CN, NOTIFY_DXCC_COL_CT,
	NOTIFY_DXCC_COL_ITU, NOTIFY_DXCC_COL_CQ, NOTIFY_DXCC_NUMCOL
};


template <typename T, typename U> static T& advli(T& i, int n) { advance(i, n); return i; }
template <typename T, typename U> static T advli(T i, U n) { advance(i, n); return i; }


static notify_list_t notify_list;
static notify_t notify_tmp;
static const vector<dxcc*>* dxcc_list;

Fl_Double_Window* notify_window;
Fl_Double_Window* dxcc_window;


////////////////////////////////////////////////////////////////////////////////
// public interface
////////////////////////////////////////////////////////////////////////////////

void notify_start(void)
{
	if (!notify_window)
		notify_init_window();

	notify_load();
	if (!notify_list.empty()) {
		for (notify_list_t::iterator i = notify_list.begin(); i != notify_list.end(); ++i)
			if (i->enabled)
				notify_register(*i);
		tblNotifyList->value(0);
		tblNotifyList->do_callback();
		notify_set_qsodb_cache();
	}
}

void notify_stop(void)
{
	for (notify_list_t::iterator i = notify_list.begin(); i != notify_list.end(); ++i)
		notify_unregister(*i);
	notify_list.clear();
	notify_set_qsodb_cache();
	tblNotifyList->clear();
}

void notify_show(void)
{
	if (!notify_window)
		notify_window = make_notify_window();
	notify_window->show();
}

// display the dxcc window
void notify_dxcc_show(bool readonly)
{
	if (!dxcc_list)
		return;
	if (readonly) {
		btnNotifyDXCCSelect->hide();
		btnNotifyDXCCDeselect->hide();
		tblNotifyFilterDXCC->callback(Fl_Widget::default_callback);
		tblNotifyFilterDXCC->menu(0);
		btnNotifyDXCCDeselect->do_callback(); // deselect all
		if (dxcc_window->shown())
			dxcc_window->hide();
		if (dxcc_window->modal())
			dxcc_window->set_non_modal();
	}
	else {
		btnNotifyDXCCSelect->show();
		btnNotifyDXCCDeselect->show();
		tblNotifyFilterDXCC->callback(notify_dxcc_check_cb);
		tblNotifyFilterDXCC->menu(notify_dxcc_context_menu);
		dxcc_window->set_modal();
	}
	dxcc_window->show();
}

// called by the myCall callback when the operator callsign is changed
void notify_change_callsign(void)
{
	for (notify_list_t::iterator i = notify_list.begin(); i != notify_list.end(); ++i) {
		if (i->event == NOTIFY_EVENT_MYCALL) { // re-register
			notify_unregister(*i);
			notify_register(*i);
		}
	}
}

// called by the RSID decoder
void notify_rsid(trx_mode mode, int afreq)
{
	const char* mode_name = mode_info[mode].name;
	regmatch_t sub[2] = { { 0, (regoff_t)strlen(mode_name) } };
	sub[1] = sub[0];
	for (notify_list_t::iterator i = notify_list.begin(); i != notify_list.end(); ++i)
		if (i->event == NOTIFY_EVENT_RSID)
			notify_recv(mode, afreq, mode_name, sub, 2, &*i);
}

// called by the config dialog when the "notifications only"
// rsid option is selected
void notify_create_rsid_event(bool val)
{
	if (!val)
		return;
	for (notify_list_t::iterator i = notify_list.begin(); i != notify_list.end(); ++i)
		if (i->event == NOTIFY_EVENT_RSID)
			return;

	notify_t rsid_event = {
		NOTIFY_EVENT_RSID, 0, "", true, 0, 0LL, NUM_MODES, NULL, NULL, 0,
		{ 0, NUM_BANDS, NUM_MODES, 0LL }, false, 0
	};
	notify_action_t rsid_action = { default_alert_text[NOTIFY_EVENT_RSID], "", "", "", 30, 1 };
	rsid_event.action = rsid_action;

	notify_list.push_back(rsid_event);
	notify_table_append(notify_list.back());
	tblNotifyList->do_callback();
	notify_save();
}


////////////////////////////////////////////////////////////////////////////////
// misc utility functions
////////////////////////////////////////////////////////////////////////////////

static void notify_set_event_dup(const notify_t& n);
static void notify_set_event_dup_menu(const char* re);
static bool notify_dxcc_row_checked(int i);

// return actual regular expression for event n
static string notify_get_re(const notify_t& n)
{
	string::size_type pos;
	string s = n.re;

	struct { const char* str; const char* rep; } subst[] = {
		{ "<MYCALL>", progdefaults.myCall.c_str() },
		{ "<CALLSIGN_RE>", CALLSIGN_RE }
	};
	for (size_t i = 0; i < sizeof(subst)/sizeof(*subst); i++)
		if ((pos = s.find(subst[i].str)) != string::npos)
			s.replace(pos, strlen(subst[i].str), subst[i].rep);

	return s;
}

// set the widget values using event n
static void notify_event_to_gui(const notify_t& n)
{
	// event
	mnuNotifyEvent->value(n.event);
	notify_event_cb(mnuNotifyEvent, 0);
	if (!n.re.empty() && inpNotifyRE->visible())
		inpNotifyRE->value(n.re.c_str());
	btnNotifyEnabled->value(n.enabled);

	// action
	inpNotifyActionDialog->value(n.action.alert.c_str());
	inpNotifyActionRXMarker->value(n.action.rx_marker.c_str());
	inpNotifyActionMacro->value(n.action.macro.c_str());
	inpNotifyActionProgram->value(n.action.program.c_str());
	cntNotifyActionLimit->value(n.action.trigger_limit);
	cntNotifyActionDialogTimeout->value(n.action.alert_timeout);

	// dup
	chkNotifyDupIgnore->value(n.dup_ignore);
	chkNotifyDupIgnore->do_callback();
	notify_set_event_dup(n);
	cntNotifyDupTime->value(n.dup.when);
	chkNotifyDupBand->value(n.dup.band);
	chkNotifyDupMode->value(n.dup.mode);

	// filter
	btnNotifyDXCCDeselect->do_callback(); // deselect all
	if (!grpNotifyFilter->active())
		return;
	chkNotifyFilterCall->value(0);
	inpNotifyFilterCall->hide();
	chkNotifyFilterDXCC->value(0);
	btnNotifyFilterDXCC->hide();
	inpNotifyFilterCall->value(0);

	if (n.filter.match == NOTIFY_FILTER_CALLSIGN) {
		chkNotifyFilterCall->value(1);
		inpNotifyFilterCall->show();
		inpNotifyFilterCall->value(n.filter.callsign.c_str());
	}
	else if (n.filter.match == NOTIFY_FILTER_DXCC) {
		chkNotifyFilterDXCC->value(1);
		btnNotifyFilterDXCC->show();
	}
	chkNotifyFilterNWB->value(n.filter.nwb);
	chkNotifyFilterLOTW->value(n.filter.lotw);
	chkNotifyFilterEQSL->value(n.filter.eqsl);
}

// copy widget values to event n
static void notify_gui_to_event(notify_t& n)
{
	// event
	n.event = static_cast<notify_event_t>(mnuNotifyEvent->value());
	n.last_trigger = 0;
	if (n.event == NOTIFY_EVENT_CUSTOM)
		n.re = inpNotifyRE->value();
	else
		n.re = event_regex[n.event].regex;
	n.enabled = btnNotifyEnabled->value();
	n.afreq = 0;
	n.rfreq = 0;
	n.match_string = 0;
	n.submatch_offsets = 0;
	n.submatch_length = 0;

	// action
	n.action.alert = inpNotifyActionDialog->value();
	n.action.rx_marker = inpNotifyActionRXMarker->value();
	n.action.macro = inpNotifyActionMacro->value();
	n.action.program = inpNotifyActionProgram->value();
	n.action.trigger_limit = static_cast<time_t>(cntNotifyActionLimit->value());
	n.action.alert_timeout = static_cast<time_t>(cntNotifyActionDialogTimeout->value());

	// filter
	if (chkNotifyFilterCall->value()) {
		n.filter.callsign = inpNotifyFilterCall->value();
		n.filter.match = NOTIFY_FILTER_CALLSIGN;
	}
	else if (chkNotifyFilterDXCC->value()) {
		n.filter.match = NOTIFY_FILTER_DXCC;

		n.filter.dxcc.clear();
		for (int i = 0; i < tblNotifyFilterDXCC->rows(); i++) {
			if (notify_dxcc_row_checked(i))
				n.filter.dxcc[tblNotifyFilterDXCC->valueAt(i, NOTIFY_DXCC_COL_CN)] = true;
		}
	}
	n.filter.nwb = chkNotifyFilterNWB->value();
	n.filter.lotw = chkNotifyFilterLOTW->value();
	n.filter.eqsl = chkNotifyFilterEQSL->value();

	// dup
	n.dup_ignore = chkNotifyDupIgnore->value();
	n.dup_ref = mnuNotifyDupWhich->value();
	n.dup.when = static_cast<time_t>(cntNotifyDupTime->value());
	n.dup.band = chkNotifyDupBand->value() ? NUM_BANDS : static_cast<band_t>(0);
	n.dup.mode = chkNotifyDupMode->value() ? NUM_MODES : static_cast<trx_mode>(0);
}

// initialise the notifications window
static void notify_init_window(void)
{
	notify_window = make_notify_window();
	notify_window->xclass(PACKAGE_TARNAME);
	dxcc_window = make_dxcc_window();
	dxcc_window->xclass(PACKAGE_TARNAME);

	struct { Fl_Button* button; const char* label; } buttons[] = {
		{ btnNotifyAdd, make_icon_label(_("Add"), plus_icon) },
		{ btnNotifyRemove, make_icon_label(_("Remove"), minus_icon) },
		{ btnNotifyUpdate, make_icon_label(_("Update"), refresh_icon) },
		{ btnNotifyTest, make_icon_label(_("Test..."), applications_system_icon) },
		{ btnNotifyClose, make_icon_label(_("Close"), close_icon) },

		{ btnNotifyDXCCSelect, make_icon_label(_("Select All"), edit_select_all_icon) },
		{ btnNotifyDXCCDeselect, make_icon_label(_("Clear All"), edit_clear_icon) },
		{ btnNotifyDXCCClose, make_icon_label(_("Close"), close_icon) },
	};
	for (size_t i = 0; i < sizeof(buttons)/sizeof(*buttons); i++) {
		buttons[i].button->label(buttons[i].label);
		set_icon_label(buttons[i].button);
		buttons[i].button->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	}
	struct { Fl_Button* button; const char** icon; } buttons2[] = {
		{ btnNotifyFilterDXCC, text_editor_icon },
		{ btnNotifyActionDialogDefault, text_icon },
		{ btnNotifyActionMarkerDefault, text_icon },
		{ btnNotifyActionMacro, text_editor_icon },
		{ btnNotifyActionProgram, folder_open_icon }
	};
	for (size_t i = 0; i < sizeof(buttons2)/sizeof(*buttons2); i++)
		buttons2[i].button->image(new Fl_Pixmap(buttons2[i].icon));
	inpNotifyRE->hide();
	grpNotifyFilter->deactivate();
	mnuNotifyEvent->menu(notify_event_menu);
	btnNotifyEnabled->value(1);
	inpNotifyRE->callback(notify_re_cb);

	int w = (tblNotifyList->w() - Fl::box_dw(tblNotifyList->box())) / 4;
	struct col_info_t {
		const char* label;
		int width;
	};
	col_info_t ncols[] = {
		{ "Event", w + 30 }, { "Filter", w + 30 },
		{ "Action", w - 30 }, { "Enabled", w - 30 },
	};
	for (size_t i = 0; i < sizeof(ncols)/sizeof(*ncols); i++) {
		tblNotifyList->addColumn(ncols[i].label, ncols[i].width,
					 static_cast<Fl_Align>(FL_ALIGN_CENTER | FL_ALIGN_CLIP));
	}
	tblNotifyList->rowSize(FL_NORMAL_SIZE);
	tblNotifyList->headerSize(FL_NORMAL_SIZE);
	tblNotifyList->allowSort(false);
	tblNotifyList->when(FL_WHEN_CHANGED | FL_WHEN_NOT_CHANGED);
	tblNotifyList->menu(notify_list_context_menu);
	for (int i = 0; i < notify_list_context_menu->size(); i++)
		set_icon_label(&notify_list_context_menu[i]);

	w = (tblNotifyFilterDXCC->w() - Fl::box_dw(tblNotifyFilterDXCC->box()) -
	     tblNotifyFilterDXCC->scrollbSize()) / NOTIFY_DXCC_NUMCOL;
	col_info_t dcols[NOTIFY_DXCC_NUMCOL] = {
		{ "", 25 }, { _("Country"), w + w - 25 + 2*(w - 45) },
		{ _("Continent"), w }, { "ITU", 45 }, { "CQ", 45 }
	};
	for (size_t i = 0; i < NOTIFY_DXCC_NUMCOL; i++) {
		tblNotifyFilterDXCC->addColumn(dcols[i].label, dcols[i].width,
					       static_cast<Fl_Align>(FL_ALIGN_CENTER | FL_ALIGN_CLIP), strcmp);
	}
	tblNotifyFilterDXCC->columnAlign(NOTIFY_DXCC_COL_CN, static_cast<Fl_Align>(FL_ALIGN_LEFT | FL_ALIGN_CLIP));
	tblNotifyFilterDXCC->rowSize(FL_NORMAL_SIZE);
	tblNotifyFilterDXCC->headerSize(FL_NORMAL_SIZE);
	tblNotifyFilterDXCC->when(FL_WHEN_CHANGED | FL_WHEN_NOT_CHANGED);
	tblNotifyFilterDXCC->menu(notify_dxcc_context_menu);
	btnNotifyDXCCSelect->callback(notify_filter_dxcc_select_cb, (void*)NOTIFY_DXCC_SELECT_ALL);
	btnNotifyDXCCDeselect->callback(notify_filter_dxcc_select_cb, (void*)NOTIFY_DXCC_DESELECT_ALL);
	inpNotifyDXCCSearchCountry->callback(notify_filter_dxcc_search);
	inpNotifyDXCCSearchCountry->when(FL_WHEN_CHANGED | FL_WHEN_NOT_CHANGED | FL_WHEN_ENTER_KEY);
	inpNotifyDXCCSearchCallsign->callback(notify_filter_dxcc_search);
	inpNotifyDXCCSearchCallsign->when(FL_WHEN_CHANGED);
	inpNotifyDXCCSearchCallsign->textfont(FL_COURIER);

	inpNotifyActionDialog->textfont(FL_COURIER);
	inpNotifyActionRXMarker->textfont(FL_COURIER);
	inpNotifyActionMacro->textfont(FL_COURIER);
	inpNotifyActionProgram->textfont(FL_COURIER);
	inpNotifyRE->textfont(FL_COURIER);
	inpNotifyFilterCall->textfont(FL_COURIER);

	tblNotifyList->callback(notify_select_cb);
	mnuNotifyEvent->callback(notify_event_cb, NOTIFY_SET_DUP_MENU);
	btnNotifyFilterDXCC->callback(notify_dxcc_browse_cb);
	btnNotifyAdd->callback(notify_add_cb);
	btnNotifyRemove->callback(notify_remove_cb);
	btnNotifyUpdate->callback(notify_update_cb, (void*)NOTIFY_LIST_MENU_UPDATE);
	btnNotifyActionDialogDefault->callback(notify_dialog_default_cb);
	btnNotifyActionMarkerDefault->callback(notify_rx_default_cb);
	btnNotifyActionProgram->callback(notify_program_select_cb);
	btnNotifyActionMacro->callback(notify_macro_edit_cb);
	btnNotifyTest->callback(notify_test_cb);
	tblNotifyFilterDXCC->callback(notify_dxcc_check_cb);
	chkNotifyDupIgnore->callback(notify_dup_ignore_cb);
	mnuNotifyDupWhich->menu(notify_dup_refs_menu);

	chkNotifyFilterCall->value(0);
	chkNotifyFilterDXCC->value(0);
	inpNotifyFilterCall->hide();
	btnNotifyFilterDXCC->hide();

	chkNotifyDupIgnore->value(1);
	chkNotifyDupIgnore->do_callback();
	chkNotifyDupBand->value(1);
	chkNotifyDupMode->value(1);
	cntNotifyDupTime->value(3600);
	mnuNotifyEvent->do_callback(); // for the dup menu

	dxcc_list = dxcc_entity_list();
	if (dxcc_list) {
		char cq[5], itu[5];
		for (vector<dxcc*>::const_iterator i = dxcc_list->begin(); i != dxcc_list->end(); ++i) {
			snprintf(itu, sizeof(itu), "%02d", (*i)->itu_zone);
			snprintf(cq, sizeof(cq), "%02d", (*i)->cq_zone);
			tblNotifyFilterDXCC->addRow(NOTIFY_DXCC_NUMCOL, "[x]", (*i)->country,
						    (*i)->continent, itu, cq);
		}
	}
	else {
		chkNotifyFilterDXCC->deactivate();
		btnNotifyFilterDXCC->deactivate();
	}

	unsigned char q = qsl_is_open();
	if (!(q & (1 << QSL_LOTW)))
		chkNotifyFilterLOTW->deactivate();
	if (!(q & (1 << QSL_EQSL)))
		chkNotifyFilterEQSL->deactivate();

}

// append event n to the table widget
static void notify_table_append(const notify_t& n)
{
	// add to table
	string fcol, acol;
	if (n.event == NOTIFY_EVENT_MYCALL)
		fcol = "My callsign";
	else if (n.event == NOTIFY_EVENT_STATION) {
		if (n.filter.match == NOTIFY_FILTER_CALLSIGN)
			fcol += "Callsign";
		else if (n.filter.match == NOTIFY_FILTER_DXCC)
			fcol += "DXCC";
	}
	else if (n.event == NOTIFY_EVENT_CUSTOM)
		fcol = n.re;
	if (n.filter.nwb)
		fcol += ", N";
	if (n.filter.lotw)
		fcol += ", L";
	if (n.filter.eqsl)
		fcol += ", E";

	if (!n.action.alert.empty())
		acol = "A";
	if (!n.action.rx_marker.empty()) {
		if (!acol.empty())
			acol += ", ";
		acol += "RX";
	}
	if (!n.action.macro.empty()) {
		if (!acol.empty())
			acol += ", ";
		acol += "TX";
	}
	if (!n.action.program.empty()) {
		if (!acol.empty())
			acol += ", ";
		acol += "P";
	}
	tblNotifyList->addRow(4, notify_event_menu[n.event].label(),
			      fcol.c_str(), acol.c_str(), n.enabled ? "Y" : "N");
	tblNotifyList->value(tblNotifyList->rows() - 1);
	tblNotifyList->redraw();
}

// clear and reload the event table
static void notify_table_reload(void)
{
	tblNotifyList->clear();
	for (notify_list_t::const_iterator i = notify_list.begin(); i != notify_list.end(); ++i)
		notify_table_append(*i);
}

////////////////////////////////////////////////////////////////////////////////
// spotter and notification functions
////////////////////////////////////////////////////////////////////////////////

static void notify_goto_freq_cb(Fl_Widget* w, void* arg)
{
	const notify_t* n = static_cast<notify_t*>(arg);
	if (active_modem->get_mode() != n->mode)
		init_modem_sync(n->mode);
	qsy(n->rfreq, n->afreq);
	if (n->event == NOTIFY_EVENT_RSID && btnRSID->value() &&
	    progdefaults.rsid_auto_disable && progdefaults.rsid_notify_only)
		toggleRSID();
}

static void notify_alert_window_cb(Fl_Widget* w, void* arg)
{
	delete static_cast<notify_t*>(arg);
	w->hide();
}

static void notify_show_alert(const notify_t& n, const char* msg)
{
	notify_dialog* alert_window = new notify_dialog;
	Fl_Button* goto_freq = alert_window->make_button(120);
	if (goto_freq) {
		char label[32];
		snprintf(label, sizeof(label), "Go to %d Hz", n.afreq);
		goto_freq->copy_label(label);
		goto_freq->callback(notify_goto_freq_cb, new notify_t(n));
		alert_window->callback(notify_alert_window_cb, goto_freq->user_data());
	}

	alert_window->notify(msg, n.action.alert_timeout, true);
}

struct replace_refs
{
	enum {
		REF_MODEM, REF_DF_HZ, REF_RF_HZ, REF_RF_KHZ, REF_AF_HZ,
		REF_LF_KHZ, REF_CALLSIGN, REF_COUNTRY, REF_MATCHED_TEXT, REF_TEXT
	};
	static void replace_var(const notify_t& n, int t, char* str, size_t len)
	{
		switch (t) {
		case REF_MODEM:
			strncpy(str, mode_info[n.mode].sname, len);
			str[len - 1] = '\0';
			break;
		case REF_DF_HZ:
			snprintf(str, len, "%lld", wf->rfcarrier());
			break;
		case REF_RF_HZ: case REF_RF_KHZ: {
			long long hz = wf->rfcarrier() + (wf->USB() ? n.afreq : -n.afreq);
			if (t == REF_RF_HZ)
				snprintf(str, len, "%lld", hz);
			else // REF_RF_KHZ
				snprintf(str, len, "%.3f", (double)hz / 1000.0);
			break;
		}
		case REF_AF_HZ:
			snprintf(str, len, "%d", n.afreq);
			break;
		case REF_LF_KHZ:
			strncpy(str, inpFreq->value(), len);
			str[len - 1] = '\0';
			break;
		case REF_CALLSIGN:
			if (n.event == NOTIFY_EVENT_MYCALL || n.event == NOTIFY_EVENT_STATION)
				snprintf(str, len, "\\%" PRIuSZ, event_regex[n.event].index);
			break;
		case REF_COUNTRY:
			if (n.event == NOTIFY_EVENT_STATION) {
				strncpy(str, n.filter.dxcc_last.c_str(), len);
				str[len - 1] = '\0';
			}
			break;
		case REF_MATCHED_TEXT:
			snprintf(str, len, "\\0");
			break;
		case REF_TEXT:
			strncpy(str, n.match_string, len);
			str[len - 1] = '\0';
			break;
		}
	}

	void operator()(const notify_t& n, string& edit)
	{
		char buf[128];
		string::size_type p;
		// replace $VARIABLES
		const char* vars[] = {
			"$MODEM", "$DF_HZ", "$RF_HZ", "$RF_KHZ", "$AF_HZ",
			"$LF_KHZ", "$CALLSIGN", "$COUNTRY", "$MATCHED_TEXT", "$TEXT"
		};
		for (size_t i = 0; i < sizeof(vars)/sizeof(*vars); i++) {
			if ((p = edit.find(vars[i])) != string::npos) {
				replace_var(n, i, buf, sizeof(buf));
				edit.replace(p, strlen(vars[i]), buf);
			}
		}
		// replace \X refs with regex substrings
		strcpy(buf, "\\0");
		for (size_t i = 0; i < n.submatch_length; i++, buf[1]++)
			for (p = 0; (p = edit.find(buf, p)) != string::npos; p = 0)
				edit.replace(p, 2, n.match_string + n.submatch_offsets[i].rm_so,
					     n.submatch_offsets[i].rm_eo - n.submatch_offsets[i].rm_so);
	}
};

// perform the actions for event n
static void notify_notify(const notify_t& n)
{
	// show alert window with timeout
	if (!n.action.alert.empty()) {
		string alert = n.action.alert;
		replace_refs()(n, alert);
		if (alert.find('%') == string::npos)
			notify_show_alert(n, alert.c_str());
		else { // treat alert text as strftime format string
			size_t len = alert.length() + 256;
			char* buf = new char[len];
			time_t t = time(NULL);
			struct tm ts;
			if (localtime_r(&t, &ts) && strftime(buf, len, alert.c_str(), &ts))
				notify_show_alert(n, buf);
			delete [] buf;
		}
	}

	// append to receive text
	if (!n.action.rx_marker.empty()) {
		string text = n.action.rx_marker;
		replace_refs()(n, text);
		string::size_type p;
		if ((p = text.find("$RX_MARKER")) != string::npos) {
			text[p] = '\0';
			note_qrg(false, text.c_str(), text.c_str() + p + strlen("$RX_MARKER"), n.mode, 0LL, n.afreq);
		}
		else
			ReceiveText->addstr(text);
	}

	// expand macros and append to transmit text
	if (!n.action.macro.empty()) {
		MACROTEXT m;
		m.text[0] = n.action.macro;
		replace_refs()(n, m.text[0]);
		m.execute(0);
	}

	// define substring & macro variables and run program
	if (!n.action.program.empty()) {
#ifndef __MINGW32__
		switch (fork()) {
		case -1:
			LOG_PERROR("fork");
			// fall through
		default:
			break;
		case 0:
#endif
			char var[] = "FLDIGI_NOTIFY_STR_0";
			string val;
			for (size_t i = 0; i < n.submatch_length; i++, var[sizeof(var) - 2]++) {
				val.assign(n.match_string + n.submatch_offsets[i].rm_so,
					   n.submatch_offsets[i].rm_eo - n.submatch_offsets[i].rm_so);
				setenv(var, val.c_str(), 1);
				if (i == event_regex[n.event].index)
					setenv("FLDIGI_NOTIFY_CALLSIGN", val.c_str(), 1);
			}
			setenv("FLDIGI_NOTIFY_TEXT", n.match_string, 1);
			snprintf(var, sizeof(var), "%" PRIuSZ, n.submatch_length);
			setenv("FLDIGI_NOTIFY_STR_NUM", var, 1);
			snprintf(var, sizeof(var), "%d", n.afreq);
			setenv("FLDIGI_NOTIFY_AUDIO_FREQUENCY", var, 1);
			snprintf(var, sizeof(var), "%u", (unsigned)n.event);
			setenv("FLDIGI_NOTIFY_EVENT", var, 1);
			if (n.event == NOTIFY_EVENT_STATION)
				setenv("FLDIGI_NOTIFY_COUNTRY", n.filter.dxcc_last.c_str(), 1);

			set_macro_env(); // also set macro variables
#ifdef __MINGW32__
			char* cmd = strdup(n.action.program.c_str());
			STARTUPINFO si;
			PROCESS_INFORMATION pi;
			memset(&si, 0, sizeof(si));
			si.cb = sizeof(si);
			memset(&pi, 0, sizeof(pi));
			if (!CreateProcess(NULL, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
				LOG_ERROR("CreateProcess failed with error code %ld", GetLastError());
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
			free(cmd);
#else
			execl("/bin/sh", "sh", "-c", n.action.program.c_str(), (char *)NULL);
			perror("execl");
			exit(EXIT_FAILURE);
		}
#endif
	}
}

// return true if the event n is a dup
static bool notify_is_dup(notify_t& n, const char* str, const regmatch_t* sub, size_t len,
			  time_t now, int afreq, trx_mode mode)
{
	if (!n.dup_ignore)
		return false;

	if (n.dup_ref == 0 || n.dup_ref >= len) {
		LOG_ERROR("Bad dup_ref: %" PRIuSZ " >= %" PRIuSZ, n.dup_ref, len);
		return false;
	}
	const regmatch_t& subidx = sub[n.dup_ref];
	string dupstr(subidx.rm_eo - subidx.rm_so, '\0');
	transform(str + subidx.rm_so, str + subidx.rm_eo, dupstr.begin(), static_cast<int (*)(int)>(toupper));

	notify_dup_t cur = { now, band(wf->rfcarrier()), mode };
	if (n.event == NOTIFY_EVENT_RSID)
		cur.freq = wf->rfcarrier() + (wf->USB() ? afreq : -afreq);

	notify_seen_t::iterator i;
	bool is_dup = false;
	if ((i = n.last_seen.find(dupstr)) != n.last_seen.end()) {
		const notify_dup_t& prev = i->second;
		is_dup = (cur.when - prev.when < n.dup.when);
		if (n.event == NOTIFY_EVENT_RSID)
			is_dup = is_dup && ::llabs(cur.freq - prev.freq) <= ceil(RSID_PRECISION);
		if (n.dup.band)
			is_dup = is_dup && cur.band == prev.band;
		if (n.dup.mode)
			is_dup = is_dup && cur.mode == prev.mode;
	}
	if (is_dup)
		return true;

	n.last_seen[dupstr] = cur;

	if (n.last_seen.size() > 1) { // remove old data
		for (i = n.last_seen.begin(); i != n.last_seen.end();) {
			if (now - i->second.when > n.dup.when)
				n.last_seen.erase(i++);
			else
				++i;
		}
	}

	return false;
}

static fre_t notify_filter_call_re("", REG_EXTENDED | REG_NOSUB | REG_ICASE);

// Called by the spotter when an event's regular expression matches the decoded text.
// Also called by notify_rsid for RSID recepction with: the frequency in afreq,
// the mode name in str, the str bounds in the sub array, and len = 1.
static void notify_recv(trx_mode mode, int afreq, const char* str, const regmatch_t* sub, size_t len, void* data)
{
	notify_t* n = reinterpret_cast<notify_t*>(data);
	time_t now = time(0);

	// check if we may trigger this event
	if (!n->enabled || now - n->last_trigger < n->action.trigger_limit)
		return;

	switch (n->event) {
	case NOTIFY_EVENT_MYCALL: case NOTIFY_EVENT_CUSTOM: case NOTIFY_EVENT_RSID:
		break;
	case NOTIFY_EVENT_STATION:
		size_t re_idx = event_regex[n->event].index;
		string call(str + sub[re_idx].rm_so, sub[re_idx].rm_eo - sub[re_idx].rm_so);
		const dxcc* e = dxcc_lookup(call.c_str());

		if (n->filter.match == NOTIFY_FILTER_CALLSIGN) {
			if (e) // remember the country name we found
				n->filter.dxcc_last = e->country;
			if (!n->filter.callsign.empty()) { // the callsign must match
				if (notify_filter_call_re.re() != n->filter.callsign) // compile new re
					notify_filter_call_re.recompile(n->filter.callsign.c_str());
				if (!notify_filter_call_re.match(call.c_str()))
					return;
			}
			else { // check for nwb, lotw, eqsl
				if (n->filter.nwb && SearchLog(call.c_str()))
					return;
				if ((n->filter.lotw || n->filter.eqsl) && qsl_is_open() &&
				    !(qsl_lookup(call.c_str()) & (1 << QSL_LOTW | 1 << QSL_EQSL)))
					return;
			}
		}
		else if (n->filter.match == NOTIFY_FILTER_DXCC) {
			if (e) {
				n->filter.dxcc_last = e->country;

				// if the dxcc filter is not empty, it must contain the country for call
				if (!n->filter.dxcc.empty() && n->filter.dxcc.find(e->country) == n->filter.dxcc.end())
					return;
				if (n->filter.nwb && qsodb_dxcc_entity_find(e->country))
					return;
			}

			if ((n->filter.lotw || n->filter.eqsl) && qsl_is_open() &&
			    !(qsl_lookup(call.c_str()) & (1 << QSL_LOTW | 1 << QSL_EQSL)))
				return;
		}
		break;
	}

	if (!notify_is_dup(*n, str, sub, len, now, afreq, mode)) {
		n->last_trigger = now;
		n->afreq = afreq;
		n->rfreq = wf->rfcarrier();
		n->mode = mode;
		n->match_string = str;
		n->submatch_offsets = sub;
		n->submatch_length = min(len, (size_t)10); // whole string + up to 9 user-specified backrefs
		notify_notify(*n);
	}
}

static void notify_set_qsodb_cache(void)
{
	bool v = false;
	for (notify_list_t::iterator i = notify_list.begin(); i != notify_list.end(); ++i) {
		if (i->event == NOTIFY_EVENT_STATION && i->filter.match == NOTIFY_FILTER_DXCC && i->enabled) {
			v = true;
			break;
		}
	}
	dxcc_entity_cache_enable(v);
}

// register event n with the spotter
static void notify_register(notify_t& n)
{
	if (n.event != NOTIFY_EVENT_RSID)
		spot_register_recv(notify_recv, &n, notify_get_re(n).c_str(), REG_EXTENDED | REG_ICASE);
}

// unregister event n
static void notify_unregister(const notify_t& n)
{
	if (n.event != NOTIFY_EVENT_RSID)
		spot_unregister_recv(notify_recv, &n);
}

////////////////////////////////////////////////////////////////////////////////
// callbacks
////////////////////////////////////////////////////////////////////////////////

// the event table callback
static void notify_select_cb(Fl_Widget* w, void* arg)
{
	int v = tblNotifyList->value();
	if (v >= 0)
		notify_event_to_gui(notify_tmp = *advli(notify_list.begin(), v));
}

// the remove button/menu item callback
static void notify_remove_cb(Fl_Widget* w, void*)
{
	int v = tblNotifyList->value();
	if (v < 0)
		return;

	// unregister
	notify_unregister(*advli(notify_list.begin(), v));
	// remove from list
	notify_list.erase(advli(notify_list.begin(), v));
	// remove from table
	tblNotifyList->removeRow(tblNotifyList->value());
	// select next row
	if (w == btnNotifyRemove) {
		if (v >= tblNotifyList->rows())
			v = tblNotifyList->rows() - 1;
		if (v >= 0) {
			tblNotifyList->value(v);
			notify_select_cb(tblNotifyList, 0);
		}
	}

	notify_save();
}

enum {
	NOTIFY_CHECK_CUSTOM_RE_EMPTY = 1 << 0,
	NOTIFY_CHECK_CUSTOM_RE_VALID = 1 << 1,
	NOTIFY_CHECK_MYCALL_NOT_EMPTY = 1 << 2
};
// do some sanity checks on the widget values before adding/updating an event
static bool notify_check(unsigned check)
{
	if (mnuNotifyEvent->value() == NOTIFY_EVENT_CUSTOM) {
		if (check & NOTIFY_CHECK_CUSTOM_RE_EMPTY) {
			if (!inpNotifyRE->size()) {
				fl_alert2(_("The regular expression field must not be empty."));
				return false;
			}
		}
		if (check & NOTIFY_CHECK_CUSTOM_RE_VALID) {
			if (!fre_t(inpNotifyRE->value(), REG_EXTENDED | REG_ICASE)) {
				fl_alert2(_("The regular expression must be valid."));
				return false;
			}
		}
	}
	if ((check & NOTIFY_CHECK_MYCALL_NOT_EMPTY) && mnuNotifyEvent->value() == NOTIFY_EVENT_MYCALL) {
		if (progdefaults.myCall.empty()) {
			fl_alert2(_("Please set your callsign first."));
			return false;
		}
	}
	// ...

	return true;
}

// the add button callback
static void notify_add_cb(Fl_Widget* w, void* arg)
{
	if (!notify_check(~0))
		return;

	notify_gui_to_event(notify_tmp);
	// add to list
	notify_list.push_back(notify_tmp);
	// add to table
	notify_table_append(notify_list.back());
	// register with spotter
	notify_register(notify_list.back());
	// save file
	notify_save();
}

// the update button/menu item callback
static void notify_update_cb(Fl_Widget* w, void* arg)
{
	int v = tblNotifyList->value();
	if (v < 0)
		return;
	if (!notify_check(~0))
		return;
	notify_t& nv = *advli(notify_list.begin(), v);

	if ((intptr_t)arg != NOTIFY_LIST_MENU_TOGGLE) {
		notify_gui_to_event(notify_tmp);
		nv = notify_tmp;
		if (nv.dup_ignore)
			nv.last_seen.clear();
	}
	else { // only toggle the enabled status
		nv.enabled = !nv.enabled;
		btnNotifyEnabled->value(nv.enabled);
	}
	if (!nv.enabled)
		notify_unregister(nv);
	else {
		notify_unregister(nv);
		notify_register(nv);
	}

	notify_table_reload();
	tblNotifyList->value(v);
	notify_save();
}

// the event selection menu callback
static void notify_event_cb(Fl_Widget* w, void* arg)
{
	notify_event_t e = static_cast<notify_event_t>(reinterpret_cast<Fl_Choice*>(w)->value());
	switch (e) {
	case NOTIFY_EVENT_MYCALL:
		mnuNotifyDupWhich->activate();
		chkNotifyDupBand->activate();
		chkNotifyDupMode->activate();
		grpNotifyFilter->deactivate();
		chkNotifyFilterCall->value(0);
		inpNotifyFilterCall->hide();
		chkNotifyFilterDXCC->value(0);
		btnNotifyFilterDXCC->hide();
		chkNotifyFilterNWB->value(0);
		chkNotifyFilterLOTW->value(0);
		chkNotifyFilterEQSL->value(0);
		inpNotifyRE->value(0);
		inpNotifyRE->hide();
		btnNotifyActionDialogDefault->show();
		break;
	case NOTIFY_EVENT_STATION:
		mnuNotifyDupWhich->activate();
		chkNotifyDupBand->activate();
		chkNotifyDupMode->activate();
		grpNotifyFilter->activate();
		if (!chkNotifyFilterCall->value() && !chkNotifyFilterDXCC->value()) {
			chkNotifyFilterCall->value(1);
			inpNotifyFilterCall->show();
		}
		inpNotifyRE->value(0);
		inpNotifyRE->hide();
		btnNotifyActionDialogDefault->show();
		break;
	case NOTIFY_EVENT_CUSTOM:
		mnuNotifyDupWhich->activate();
		chkNotifyDupBand->activate();
		chkNotifyDupMode->activate();
		grpNotifyFilter->deactivate();
		chkNotifyFilterCall->value(0);
		inpNotifyFilterCall->hide();
		chkNotifyFilterDXCC->value(0);
		btnNotifyFilterDXCC->hide();
		chkNotifyFilterNWB->value(0);
		chkNotifyFilterLOTW->value(0);
		chkNotifyFilterEQSL->value(0);
		inpNotifyRE->show();
		break;
	case NOTIFY_EVENT_RSID:
		grpNotifyFilter->deactivate();
		chkNotifyFilterCall->value(0);
		inpNotifyFilterCall->hide();
		chkNotifyFilterDXCC->value(0);
		btnNotifyFilterDXCC->hide();
		chkNotifyFilterNWB->value(0);
		chkNotifyFilterLOTW->value(0);
		chkNotifyFilterEQSL->value(0);
		inpNotifyRE->value(0);
		inpNotifyRE->hide();
		btnNotifyActionDialogDefault->show();
		// limited dup handling
		mnuNotifyDupWhich->deactivate();
		chkNotifyDupBand->value(0);
		chkNotifyDupBand->deactivate();
		chkNotifyDupMode->value(1);
		chkNotifyDupMode->deactivate();
		break;
	}

	if (arg == NOTIFY_SET_DUP_MENU) {
		notify_gui_to_event(notify_tmp);
		notify_set_event_dup(notify_tmp);
	}
	w->parent()->redraw();
}

// the program filesystem browse button callback
static void notify_program_select_cb(Fl_Widget* w, void* arg)
{
	const char* fn = FSEL::select(_("Run program"), "", 0, 0);
	if (fn) {
		inpNotifyActionProgram->value(fn);
		// quote program path
		inpNotifyActionProgram->position(0);
		inpNotifyActionProgram->insert("\"", 1);
		inpNotifyActionProgram->position(inpNotifyActionProgram->size());
		inpNotifyActionProgram->insert("\"", 1);
	}
}

// the test button callback
static void notify_test_cb(Fl_Widget* w, void* arg)
{
	notify_gui_to_event(notify_tmp);

	if (notify_tmp.event == NOTIFY_EVENT_RSID) {
		notify_tmp.mode = active_modem->get_mode();
		regmatch_t sub[2] = { { 0, (regoff_t)strlen(mode_info[notify_tmp.mode].name) } };
		sub[1] = sub[0];
		notify_recv(notify_tmp.mode, active_modem->get_freq(),
			    mode_info[notify_tmp.mode].name, sub, 2, &notify_tmp);
		return;
	}

	string test_strings[3];
	test_strings[NOTIFY_EVENT_MYCALL].assign(progdefaults.myCall).append(" de n0call");
	test_strings[NOTIFY_EVENT_STATION] = "cq de n0call n0call ";
	static string test;
	if (test.empty())
		test = test_strings[notify_tmp.event];

	string msg;
	msg.assign(_("Default test string is:\n  \"")).append(test_strings[notify_tmp.event]).append("\"\n")
		.append(_("Enter test string or leave blank for default:"));
        const char* s = fl_input2("%s", msg.c_str(), test.c_str());
	if (s) {
		if (test.assign(s).empty()) // empty input
			test = test_strings[notify_tmp.event];
	}
	else // cancelled
		return;

	fre_t re(notify_get_re(notify_tmp).c_str(), REG_EXTENDED | REG_ICASE);
	if (!re)
		fl_alert2(_("This event's regular expression is invalid."));
	else if (re.match(test.c_str())) {
		const vector<regmatch_t>& o = re.suboff();
		notify_recv(active_modem->get_mode(), active_modem->get_freq(),
			    test.c_str(), &o[0], o.size(), &notify_tmp);
	}
	else
		fl_message2(_("The test string did not match this event's search pattern."));
}

// the macro editor button callback
static void notify_macro_edit_cb(Fl_Widget* w, void* arg)
{
	editMacro(0, MACRO_EDIT_INPUT, inpNotifyActionMacro);
}

// the insert default alert text button callback
static void notify_dialog_default_cb(Fl_Widget* w, void* arg)
{
	size_t i = CLAMP((size_t)mnuNotifyEvent->value(), 0,
			 sizeof(default_alert_text)/sizeof(*default_alert_text) - 1);
	string s = default_alert_text[i];
	if (s.empty()) { // custom search; count and list refs
		size_t nsub = min(fre_t(inpNotifyRE->value(), REG_EXTENDED | REG_ICASE).nsub(), (size_t)9);
		if (nsub) {
			s.assign(_("Available substrings")).append(":\n\\0\n");
			char ref[] = "\\1";
			for (size_t i = 1; i < nsub; i++, ref[1]++)
				s.append(ref).append("\n");
		}
	}
	inpNotifyActionDialog->value(s.c_str());
}

// the insert default RX text button callback
static void notify_rx_default_cb(Fl_Widget* w, void* arg)
{
	if (mnuNotifyEvent->value() == NOTIFY_EVENT_RSID)
		inpNotifyActionRXMarker->value("\nRSID: $RX_MARKER\n");
	else
		inpNotifyActionRXMarker->value("\n$RX_MARKER\n");
}

// the ignore duplicates check button callback
void notify_dup_ignore_cb(Fl_Widget* w, void* arg)
{
	if (static_cast<Fl_Check_Button*>(w)->value()) {
		mnuNotifyDupWhich->show();
		cntNotifyDupTime->show();
		chkNotifyDupBand->show();
		chkNotifyDupMode->show();
	}
	else {
		mnuNotifyDupWhich->hide();
		cntNotifyDupTime->hide();
		chkNotifyDupBand->hide();
		chkNotifyDupMode->hide();
	}
	w->parent()->redraw();
}

// the custom re field callback
static void notify_re_cb(Fl_Widget* w, void* arg)
{
	notify_set_event_dup_menu(static_cast<Fl_Input*>(w)->value());
}

////////////////////////////////////////////////////////////////////////////////
// dup widget handling
////////////////////////////////////////////////////////////////////////////////

// set the dup menu substrings for regular epxression string re
static void notify_set_event_dup_menu(const char* re)
{
	int v = mnuNotifyDupWhich->value();
	size_t nref = fre_t(re, REG_EXTENDED).nsub();
	for (size_t i = 0; i < sizeof(notify_dup_refs_menu)/sizeof(*notify_dup_refs_menu); i++)
		notify_dup_refs_menu[i].hide();
	if (nref) {
		size_t i;
		for (i = 1; i < nref; i++)
			notify_dup_refs_menu[i].show();
		if ((size_t)v == nref)
			v = mnuNotifyDupWhich->size() - 1;
		else
			v = 1;
	}
	else
		v = 0;
	mnuNotifyDupWhich->value(v);
	mnuNotifyDupWhich->redraw();
}

// set the dup group widgets for event n
static void notify_set_event_dup(const notify_t& n)
{
	size_t i;
	for (i = 0; i < sizeof(notify_dup_refs_menu)/sizeof(*notify_dup_refs_menu); i++)
		notify_dup_refs_menu[i].hide();

	switch (n.event) {
	case NOTIFY_EVENT_MYCALL: case NOTIFY_EVENT_STATION: case NOTIFY_EVENT_RSID:
		i = event_regex[n.event].index;
		mnuNotifyDupWhich->menu(notify_dup_callsign_menu);
		for (size_t j = 0; j < sizeof(notify_dup_callsign_menu)/sizeof(*notify_dup_callsign_menu) - 1; j++)
			notify_dup_callsign_menu[j].hide();
		notify_dup_callsign_menu[i].show();
		if (n.event == NOTIFY_EVENT_RSID)
			notify_dup_callsign_menu[i].label(_("Frequency"));
		else
			notify_dup_callsign_menu[i].label(_("Callsign"));
		mnuNotifyDupWhich->value(i);
		break;
	case NOTIFY_EVENT_CUSTOM:
		mnuNotifyDupWhich->menu(notify_dup_refs_menu);
		notify_set_event_dup_menu(notify_get_re(n).c_str());
		break;
	}
}

////////////////////////////////////////////////////////////////////////////////
// DXCC list
////////////////////////////////////////////////////////////////////////////////

// set the toggle status of row i to cond
static void notify_dxcc_row_check(int i, bool cond = true)
{
	*tblNotifyFilterDXCC->valueAt(i, NOTIFY_DXCC_COL_SEL) = "["[!cond];
}

// toggle the checked status of row i, return new status
static bool notify_dxcc_row_toggle(int i)
{
	char* p = tblNotifyFilterDXCC->valueAt(i, NOTIFY_DXCC_COL_SEL);
	return (*p = "["[!!*p]);
}

// return checked status of row i
static bool notify_dxcc_row_checked(int i)
{
	return *tblNotifyFilterDXCC->valueAt(i, NOTIFY_DXCC_COL_SEL);
}

// the dxcc list select/deselect callback
static void notify_filter_dxcc_select_cb(Fl_Widget* w, void* arg)
{

	bool val;
	const char* str;
	int row = tblNotifyFilterDXCC->value();
	int col;

	intptr_t iarg = (intptr_t)arg;
	switch (iarg) {
	case NOTIFY_DXCC_SELECT_CONT: case NOTIFY_DXCC_DESELECT_CONT:
		str = tblNotifyFilterDXCC->valueAt(row, col = NOTIFY_DXCC_COL_CT);
		val = (iarg == NOTIFY_DXCC_SELECT_CONT);
		break;
	case NOTIFY_DXCC_SELECT_ITU: case NOTIFY_DXCC_DESELECT_ITU:
		str = tblNotifyFilterDXCC->valueAt(row, col = NOTIFY_DXCC_COL_ITU);
		val = (iarg == NOTIFY_DXCC_SELECT_ITU);
		break;
	case NOTIFY_DXCC_SELECT_CQ: case NOTIFY_DXCC_DESELECT_CQ:
		str = tblNotifyFilterDXCC->valueAt(row, col = NOTIFY_DXCC_COL_CQ);
		val = (iarg == NOTIFY_DXCC_SELECT_CQ);
		break;

	case NOTIFY_DXCC_SELECT_ALL:
		for (int i = 0; i < tblNotifyFilterDXCC->rows(); i++)
			notify_dxcc_row_check(i);
		goto redraw;
	case NOTIFY_DXCC_DESELECT_ALL:
		for (int i = 0; i < tblNotifyFilterDXCC->rows(); i++)
			notify_dxcc_row_check(i, false);
		goto redraw;

	default:
		return;
	}

	for (int i = 0; i < tblNotifyFilterDXCC->rows(); i++) {
		if (!strcmp(tblNotifyFilterDXCC->valueAt(i, col), str))
			notify_dxcc_row_check(i, val);
	}

redraw:
	tblNotifyFilterDXCC->redraw();
}

// the dxcc search field callback
static void notify_filter_dxcc_search(Fl_Widget* w, void* arg)
{
	if (w == inpNotifyDXCCSearchCallsign) {
		const dxcc* e = dxcc_lookup(inpNotifyDXCCSearchCallsign->value());
		if (e) {
			inpNotifyDXCCSearchCountry->value(e->country);
			inpNotifyDXCCSearchCountry->do_callback();
			inpNotifyDXCCSearchCountry->position(0);
		}
		return;
	}

	if (unlikely(!inpNotifyDXCCSearchCountry->size()))
		return;
	int col = 1, row = tblNotifyFilterDXCC->value() + 1;
	row = WCLAMP(row, 0, tblNotifyFilterDXCC->rows() - 1);
	if (tblNotifyFilterDXCC->search(row, col, false, inpNotifyDXCCSearchCountry->value())) {
		int when = tblNotifyFilterDXCC->when();
		tblNotifyFilterDXCC->when(FL_WHEN_NEVER);
		tblNotifyFilterDXCC->GotoRow(row);
		tblNotifyFilterDXCC->when(when);
		inpNotifyDXCCSearchCountry->textcolor(FL_FOREGROUND_COLOR);
	}
	else
		inpNotifyDXCCSearchCountry->textcolor(FL_RED);
	inpNotifyDXCCSearchCountry->redraw();
}

// the dxcc filter selection button callback
static void notify_dxcc_browse_cb(Fl_Widget* w, void* arg)
{
	int v = tblNotifyList->value();
	if (v < 0) // no selection, uncheck all rows
		btnNotifyDXCCDeselect->do_callback();
	else {
		const notify_t& n = *advli(notify_list.begin(), v);
		for (int i = 0; i < tblNotifyFilterDXCC->rows(); i++)
			notify_dxcc_row_check(i, n.filter.dxcc.find(tblNotifyFilterDXCC->valueAt(i, NOTIFY_DXCC_COL_CN))
					      != n.filter.dxcc.end());
	}
	notify_dxcc_show(false);
}

// the dxcc table callback
static void notify_dxcc_check_cb(Fl_Widget* w, void* arg)
{
	if (Fl::event_button() != FL_LEFT_MOUSE || (Fl::event() == FL_KEYDOWN && !Fl::event_shift()))
		return;
	int sel = tblNotifyFilterDXCC->value();
	const char* country = tblNotifyFilterDXCC->valueAt(sel, NOTIFY_DXCC_COL_CN);
	if (notify_dxcc_row_toggle(sel))
		notify_tmp.filter.dxcc[country] = true;
	else
		notify_tmp.filter.dxcc.erase(country);
}

////////////////////////////////////////////////////////////////////////////////
// storage functions
////////////////////////////////////////////////////////////////////////////////

// save the event list
static void notify_save(void)
{
	notify_set_qsodb_cache();

	remove(string(HomeDir).append("/").append("notify.prefs").c_str());
	Fl_Preferences ndata(HomeDir.c_str(), PACKAGE_TARNAME, "notify");
	ndata.set("items", static_cast<int>(notify_list.size()));

	char group[8];
	size_t num = 0;
	for (notify_list_t::iterator i = notify_list.begin(); i != notify_list.end(); ++i) {
		snprintf(group, sizeof(group), "item%" PRIuSZ, num++);

		ndata.set(Fl_Preferences::Name("%s/event", group), i->event);
		if (i->event >= NOTIFY_EVENT_CUSTOM)
			ndata.set(Fl_Preferences::Name("%s/re", group), i->re.c_str());
		ndata.set(Fl_Preferences::Name("%s/enabled", group), i->enabled);

		ndata.set(Fl_Preferences::Name("%s/action/alert", group), i->action.alert.c_str());
		ndata.set(Fl_Preferences::Name("%s/action/rx_marker", group), i->action.rx_marker.c_str());
		ndata.set(Fl_Preferences::Name("%s/action/macro", group), i->action.macro.c_str());
		ndata.set(Fl_Preferences::Name("%s/action/program", group), i->action.program.c_str());
		ndata.set(Fl_Preferences::Name("%s/action/trigger_limit", group),
			  static_cast<int>(i->action.trigger_limit));
		ndata.set(Fl_Preferences::Name("%s/action/alert_timeout", group),
			  static_cast<int>(i->action.alert_timeout));

		ndata.set(Fl_Preferences::Name("%s/filter/match", group), i->filter.match);
		ndata.set(Fl_Preferences::Name("%s/filter/callsign", group), i->filter.callsign.c_str());
		ndata.set(Fl_Preferences::Name("%s/filter/nwb", group), i->filter.nwb);
		ndata.set(Fl_Preferences::Name("%s/filter/lotw", group), i->filter.lotw);
		ndata.set(Fl_Preferences::Name("%s/filter/eqsl", group), i->filter.eqsl);
		int k = 0;
		for (notify_filter_dxcc_t::const_iterator j = i->filter.dxcc.begin();
		     j != i->filter.dxcc.end() && j->second; ++j) {
			ndata.set(Fl_Preferences::Name("%s/filter/dxcc/%d", group, k++), j->first.c_str());
		}

		ndata.set(Fl_Preferences::Name("%s/dup/ignore", group), i->dup_ignore);
		ndata.set(Fl_Preferences::Name("%s/dup/ref", group), static_cast<int>(i->dup_ref));
		ndata.set(Fl_Preferences::Name("%s/dup/when", group), static_cast<int>(i->dup.when));
		ndata.set(Fl_Preferences::Name("%s/dup/band", group), i->dup.band);
		ndata.set(Fl_Preferences::Name("%s/dup/mode", group), static_cast<int>(i->dup.mode));
	}
}

// load the event list
static void notify_load(void)
{
	int x;
	char s[512];
	s[sizeof(s)-1] = '\0';

	Fl_Preferences ndata(HomeDir.c_str(), PACKAGE_TARNAME, "notify");
	if (!ndata.get("items", x, 0) || x <= 0)
		return;
	size_t n = static_cast<size_t>(x);

	char group[8];
	for (size_t i = 0; i < n; i++) {
		notify_t nitem;

		snprintf(group, sizeof(group), "item%" PRIuSZ, i);

		ndata.get(Fl_Preferences::Name("%s/event", group), x, 0);
		nitem.event = static_cast<notify_event_t>(x);
		nitem.last_trigger = 0;
		if (nitem.event >= NOTIFY_EVENT_CUSTOM) {
			ndata.get(Fl_Preferences::Name("%s/re", group), s, "", sizeof(s)-1);
			nitem.re = s;
		}
		else
			nitem.re = event_regex[nitem.event].regex;
		ndata.get(Fl_Preferences::Name("%s/enabled", group), x, 1);
		nitem.enabled = x;

		ndata.get(Fl_Preferences::Name("%s/action/alert", group), s, "", sizeof(s)-1);
		nitem.action.alert = s;
		ndata.get(Fl_Preferences::Name("%s/action/macro", group), s, "", sizeof(s)-1);
		nitem.action.macro = s;
		ndata.get(Fl_Preferences::Name("%s/action/rx_marker", group), s, "", sizeof(s)-1);
		nitem.action.rx_marker = s;
		ndata.get(Fl_Preferences::Name("%s/action/program", group), s, "", sizeof(s)-1);
		nitem.action.program = s;
		ndata.get(Fl_Preferences::Name("%s/action/trigger_limit", group), x, 0);
		nitem.action.trigger_limit = x;
		ndata.get(Fl_Preferences::Name("%s/action/alert_timeout", group), x, 5);
		nitem.action.alert_timeout = x;

		ndata.get(Fl_Preferences::Name("%s/filter/match", group), x, 0);
		nitem.filter.match = static_cast<notify_filter_match_t>(x);
		ndata.get(Fl_Preferences::Name("%s/filter/callsign", group), s, "", sizeof(s)-1);
		nitem.filter.callsign = s;
		ndata.get(Fl_Preferences::Name("%s/filter/nwb", group), x, 0);
		nitem.filter.nwb = x;
		ndata.get(Fl_Preferences::Name("%s/filter/lotw", group), x, 0);
		nitem.filter.lotw = x;
		ndata.get(Fl_Preferences::Name("%s/filter/eqsl", group), x, 0);
		nitem.filter.eqsl = x;
		for (int k = 0; ndata.get(Fl_Preferences::Name("%s/filter/dxcc/%d", group, k), s, "", sizeof(s)-1); k++)
			nitem.filter.dxcc[s] = true;

		ndata.get(Fl_Preferences::Name("%s/dup/ignore", group), x, 0);
		nitem.dup_ignore = x;
		ndata.get(Fl_Preferences::Name("%s/dup/ref", group), x, 0);
		nitem.dup_ref = x;
		ndata.get(Fl_Preferences::Name("%s/dup/when", group), x, 600);
		nitem.dup.when = x;
		ndata.get(Fl_Preferences::Name("%s/dup/band", group), x, 0);
		nitem.dup.band = static_cast<band_t>(x);
		ndata.get(Fl_Preferences::Name("%s/dup/mode", group), x, 0);
		nitem.dup.mode = static_cast<trx_mode>(x);

		notify_list.push_back(nitem);
	}

	notify_table_reload();
}
