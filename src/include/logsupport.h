#ifndef SUPPORT_H
#define SUPPORT_h

#include <string>

#include "qso_db.h"
#include "adif_io.h"
#include "date.h"

#ifdef __CYGWIN__
#  define ADIF_SUFFIX "adi"
#else
#  define ADIF_SUFFIX "adif"
#endif

extern cQsoDb        qsodb;
extern cAdifIO       adifFile;
extern std::string logbook_filename;

extern void loadBrowser();

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
extern void saveLogbook();
extern void cb_mnuShowLogbook(Fl_Menu_ *m, void* d);

extern void activateButtons();
extern void saveRecord ();
extern void clearRecord ();
extern void updateRecord ();
extern void deleteRecord ();
extern void AddRecord ();
extern void SearchLastQSO (const char *);
extern void cb_search(Fl_Widget* w, void*);
extern int log_search_handler(int);

extern void cb_doExport();

#endif
