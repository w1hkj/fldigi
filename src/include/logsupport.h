// ----------------------------------------------------------------------------
// Copyright (C) 2014
//              David Freese, W1HKJ
//
// This file is part of fldigi
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

#ifndef LOG_SUPPORT_H
#define LOG_SUPPORT_H

#include <string>

#include <FL/Fl_Widget.H>
#include <FL/Fl_Menu_.H>

#include "qso_db.h"
#include "adif_io.h"

#include "lgbook.h"

#ifdef __WOE32__
#  define ADIF_SUFFIX "adi"
#else
#  define ADIF_SUFFIX "adif"
#endif

enum savetype {ADIF, CSV, TEXT, LOTW, LO};

enum logtype {
  LOG_QSO, 			// No Contest
  LOG_GENERIC,		// Generic contest
  LOG_AIDX,			// Africa International DX
  LOG_FD,			// ARRL Field Day
  LOG_AICW,			// ARRL International DX (Cw)
  LOG_JOTA,			// ARRL Jamboree On The Air
  LOG_KD,			// ARRL Kids Day
  LOG_ARR,			// ARRL Rookie Roundup
  LOG_RTTY,			// ARRL RTTY
  LOG_ASCR,			// ARRL School Club Roundup
  LOG_CWSS,			// ARRL November Sweepstakes
  LOG_WFD,			// ARRL Winter Field Day
  LOG_BART,			// BARTG RTTY contest
  LOG_CQ_WPX,		// CQ WPX
  LOG_CQWW_DX,		// CQ WW Dx
  LOG_CQWW_RTTY,	// CQ WW RTTY
  LOG_IARI,			// Italian ARI International DX
  LOG_NAQP,			// North American QSO Party
  LOG_NAS,			// North American Sprint
  LOG_1010,			// Ten Ten
  LOG_VHF,			// VHF
//  LOG_WAE,			// Worked All Europe
  LOG_SQSO			// State QSO parties
};

extern cQsoDb        qsodb;
extern cAdifIO       adifFile;
extern std::string logbook_filename;
extern std::string sDate_on;
extern std::string sDate_off;
extern std::string sTime_on;
extern std::string sTime_off;

extern void loadBrowser(bool keep_pos = false);

extern void Export_log();
extern void cb_SortByCall();
extern void cb_SortByDate();
extern void cb_SortByMode();
extern void cb_SortByFreq();
extern void cb_browser(Fl_Widget *, void *);

extern void cb_mnuNewLogbook(Fl_Menu_* m, void* d);
extern void cb_mnuOpenLogbook(Fl_Menu_* m, void* d);
extern void cb_mnuSaveLogbook(Fl_Menu_*m, void* d);
extern void cb_mnuMergeADIF_log(Fl_Menu_* m, void* d);
extern void cb_mnuExportADIF_log(Fl_Menu_* m, void* d);
extern void cb_mnuExportCSV_log(Fl_Menu_* m, void* d);
extern void cb_mnuExportTEXT_log(Fl_Menu_* m, void* d);

extern string lotw_rec(cQsoRec &rec);
extern void cb_btnExportLoTW();

extern Fl_Double_Window *lotw_review_dialog;
extern void cb_review_lotw();
extern void cb_send_lotw();
extern void send_to_lotw(void *);
extern void cb_btn_verify_lotw(Fl_Button *, void *);
extern void cb_btn_verify_eqsl(Fl_Button *, void *);
extern void cb_btn_view_unmatched(Fl_Button *, void *);

extern void cb_Export_Cabrillo(Fl_Menu_* m, void* d);
extern void cb_export_date_select();

extern void saveLogbook(bool force = false);
extern void cb_mnuShowLogbook(Fl_Menu_ *m, void* d);

extern void activateButtons();
extern void saveRecord ();
extern void clearRecord ();
extern void updateRecord ();
extern void deleteRecord ();
extern void AddRecord ();
extern void DisplayRecord (int idxRec);
extern void SearchLastQSO (const char *);
extern cQsoRec* SearchLog(const char *callsign);
extern void DupCheck();
extern void show_dup(void *);
extern void cb_search(Fl_Widget* w, void*);
extern int log_search_handler(int);
extern void reload_browser();
extern void restore_sort();

extern void cb_doExport();

extern void WriteCabrillo();

extern void dxcc_entity_cache_enable(bool v);
extern bool qsodb_dxcc_entity_find(const char* country);

extern void adif_read_OK();

#endif
