#ifndef SUPPORT_H
#define SUPPORT_h

#include <string>

#include <FL/Fl_Widget.H>
#include <FL/Fl_Menu_.H>

#include "qso_db.h"
#include "adif_io.h"

// Avoid 'nitems' macro collision in FreeBSD's sys/params.h
// with fltk's FL/Fl_Check_Browser.H
// ->nitems() is called by functions in this file

#undef nitems
#include "lgbook.h"
#define FLTK_nitems nitems
#undef nitems

#ifdef __WOE32__
#  define ADIF_SUFFIX "adi"
#else
#  define ADIF_SUFFIX "adif"
#endif

enum savetype {ADIF, CSV, TEXT, LO};

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
extern void cb_Export_Cabrillo(Fl_Menu_* m, void* d);
extern void cb_export_date_select();

extern void saveLogbook();
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
extern void cb_search(Fl_Widget* w, void*);
extern int log_search_handler(int);
extern void reload_browser();

extern void cb_doExport();

extern void WriteCabrillo();

extern void dxcc_entity_cache_enable(bool v);
extern bool qsodb_dxcc_entity_find(const char* country);

extern void adif_read_OK();

#endif
