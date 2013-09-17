// ----------------------------------------------------------------------------
// logsupport.cxx
//
// Copyright (C) 2006-2010
//		Dave Freese, W1HKJ
// Copyright (C) 2008-2009
//		Stelios Bounanos, M0GLD
//
// This file is part of fldigi.
//
// Fldigi is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#include <config.h>

#include <string>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

#include "main.h"
#include "trx.h"
#include "debug.h"
#include "macros.h"
#include "status.h"
#include "date.h"

#include "logger.h"
#include "adif_io.h"
#include "textio.h"
#include "logbook.h"
#include "rigsupport.h"

#include "fl_digi.h"
#include "fileselect.h"
#include "configuration.h"
#include "main.h"
#include "locator.h"
#include "icons.h"
#include "gettext.h"
#include "qrunner.h"

#include <FL/filename.H>
#include <FL/fl_ask.H>

using namespace std;

cQsoDb		qsodb;
cAdifIO		adifFile;
cTextFile	txtFile;

string		logbook_filename;

enum sorttype {NONE, SORTCALL, SORTDATE, SORTFREQ, SORTMODE};
sorttype lastsort = SORTDATE;
bool callfwd = true;
bool modefwd = true;
bool freqfwd = true;

void restore_sort();

// convert to and from "00:00:00" <=> "000000"
const char *timeview(const char *s)
{
	static char ds[9];
	int len = strlen(s);
	strcpy(ds, "00:00:00");
	if (len < 4)
		return ds;
	ds[0] = s[0]; ds[1] = s[1];
	ds[3] = s[2]; ds[4] = s[3];
	if (len < 6)
		return ds;
	ds[6] = s[4];
	ds[7] = s[5];
	return ds;
}

const char *timestring(const char *s)
{
	static char ds[7];
	int len = strlen(s);
	if (len <= 4) return s;
	ds[0] = s[0]; ds[1] = s[1];
	ds[2] = s[3]; ds[3] = s[4];
	if (len < 8) {
		ds[4] = ds[5] = '0';
		ds[6] = 0;
		return ds;
	}
	ds[4] = s[6];
	ds[5] = s[7];
	ds[6] = 0;
	return ds;
}

const char *timeview4(const char *s)
{
	static char ds[6];
	int len = strlen(s);
	strcpy(ds, "00:00");
	if (len < 5)
		return ds;
	ds[0] = s[0]; ds[1] = s[1];
	ds[3] = s[2]; ds[4] = s[3];
	return ds;
}

const char *time4(const char *s)
{
	static char ds[5];
	int len = strlen(s);
	if (len <= 4)
		return ds;
	memset(ds, 0, 5);
	strncpy(ds, s, 4);
	return ds;
}

void Export_CSV()
{
	if (chkExportBrowser->nchecked() == 0) return;

	cQsoRec *rec;

	string title = _("Export to CSV file");
	string filters = "CSV\t*.csv";
#ifdef __APPLE__
	filters.append("\n");
#endif
	const char* p = FSEL::saveas( title.c_str(), filters.c_str(), "export.csv");
	if (!p)
		return;

	for (int i = 0; i < chkExportBrowser->FLTK_nitems(); i++) {
		if (chkExportBrowser->checked(i + 1)) {
			rec = qsodb.getRec(i);
			rec->putField(EXPORT, "E");
			qsodb.qsoUpdRec (i, rec);
		}
	}
	string sp = p;
	if (sp.find(".csv") == string::npos) sp.append(".csv");
	txtFile.writeCSVFile(sp.c_str(), &qsodb);
}

void Export_TXT()
{
	if (chkExportBrowser->nchecked() == 0) return;

	cQsoRec *rec;
	string title = _("Export to fixed field text file");
	string filters = "TEXT\t*.txt";
#ifdef __APPLE__
	filters.append("\n");
#endif
	const char* p = FSEL::saveas( title.c_str(), filters.c_str(), "export.txt");
	if (!p)
		return;

	for (int i = 0; i < chkExportBrowser->FLTK_nitems(); i++) {
		if (chkExportBrowser->checked(i + 1)) {
			rec = qsodb.getRec(i);
			rec->putField(EXPORT, "E");
			qsodb.qsoUpdRec (i, rec);
		}
	}
	string sp = p;
	if (sp.find(".txt") == string::npos) sp.append(".txt");
	txtFile.writeTXTFile(p, &qsodb);
}

void Export_ADIF()
{
	if (chkExportBrowser->nchecked() == 0) return;

	cQsoRec *rec;

	string title = _("Export to ADIF file");
	string filters;
	string defname;
	filters.assign("ADIF\t*.").append(ADIF_SUFFIX);
#ifdef __APPLE__
	filters.append("\n");
#endif
	defname.assign("export.").append(ADIF_SUFFIX);
	const char* p = FSEL::saveas( title.c_str(), filters.c_str(), defname.c_str());

	if (!p)
		return;

	for (int i = 0; i < chkExportBrowser->FLTK_nitems(); i++) {
		if (chkExportBrowser->checked(i + 1)) {
			rec = qsodb.getRec(i);
			rec->putField(EXPORT, "E");
			qsodb.qsoUpdRec (i, rec);
		}
	}
	string sp = p;
	if (sp.find("."ADIF_SUFFIX) == string::npos) sp.append("."ADIF_SUFFIX);
	adifFile.writeFile (sp.c_str(), &qsodb);
}

static savetype export_to = ADIF;

void Export_log()
{
	if (export_to == ADIF) Export_ADIF();
	else if (export_to == CSV) Export_CSV();
	else Export_TXT();
}

void saveLogbook()
{
	if (!qsodb.isdirty()) return;
	if (progdefaults.NagMe)
		if (!fl_choice2(_("Save changed Logbook?"), _("No"), _("Yes"), NULL))
			return;

	cQsoDb::reverse = false;
	qsodb.SortByDate(progdefaults.sort_date_time_off);

	qsodb.isdirty(0);
	restore_sort();

	adifFile.writeLog (logbook_filename.c_str(), &qsodb);

}

static void dxcc_entity_cache_clear(void);
static void dxcc_entity_cache_add(cQsoRec* r);
static void dxcc_entity_cache_rm(cQsoRec* r);
static void dxcc_entity_cache_add(cQsoDb& db);

void cb_mnuNewLogbook(Fl_Menu_* m, void* d){
	if (!fl_choice2(_("Create New Logbook?"), _("No"), _("Yes"), NULL))
		return;

	saveLogbook();

	logbook_filename = LogsDir;
	logbook_filename.append("newlog.").append(ADIF_SUFFIX);
	progdefaults.logbookfilename = logbook_filename;
	dlgLogbook->label(fl_filename_name(logbook_filename.c_str()));
	progdefaults.changed = true;
	qsodb.deleteRecs();
	dxcc_entity_cache_clear();
	wBrowser->clear();
	clearRecord();
}

void adif_read_OK()
{
	if (qsodb.nbrRecs() == 0)
		adifFile.writeFile(logbook_filename.c_str(), &qsodb);
	dxcc_entity_cache_clear();
	dxcc_entity_cache_add(qsodb);
	restore_sort();
	activateButtons();
	loadBrowser();
}

void cb_mnuOpenLogbook(Fl_Menu_* m, void* d)
{
	string title = _("Open logbook file");
	string filters;
	filters.assign("ADIF\t*.").append(ADIF_SUFFIX);
#ifdef __APPLE__
	filters.append("\n");
#endif

	const char* p = FSEL::select( title.c_str(), filters.c_str(), logbook_filename.c_str());
	if (p) {
		saveLogbook();
		qsodb.deleteRecs();

		logbook_filename = p;
		progdefaults.logbookfilename = logbook_filename;
		progdefaults.changed = true;

		adifFile.readFile (logbook_filename.c_str(), &qsodb);
		dlgLogbook->label(fl_filename_name(logbook_filename.c_str()));
		qsodb.isdirty(0);
	}
}

void cb_mnuSaveLogbook(Fl_Menu_*m, void* d) {
	string title = _("Save logbook file");
	string filter;
	filter.assign("ADIF\t*.").append(ADIF_SUFFIX);
#ifdef __APPLE__
	filter.append("\n");
#endif
	const char* p = FSEL::saveas( title.c_str(), filter.c_str(), logbook_filename.c_str());
	if (p) {
		logbook_filename = p;
		if (logbook_filename.find("."ADIF_SUFFIX) == string::npos)
			logbook_filename.append("."ADIF_SUFFIX);
		dlgLogbook->label(fl_filename_name(logbook_filename.c_str()));

		cQsoDb::reverse = false;
		qsodb.SortByDate(progdefaults.sort_date_time_off);

		qsodb.isdirty(0);
		restore_sort();
		adifFile.writeLog (logbook_filename.c_str(), &qsodb);
	}
}

int comparerecs (const void *rp1, const void *rp2) { // rp1 needle, rp2 haystack
	cQsoRec *r1 = (cQsoRec *)rp1;
	cQsoRec *r2 = (cQsoRec *)rp2;
	int cmp = 0;
// compare by call
	const char * s1 = r1->getField(CALL);
	const char * s2 = r2->getField(CALL);

	const char *p1 = strpbrk (s1+1, "0123456789");
	const char *p2 = strpbrk (s2+1, "0123456789");
	if (p1 && p2) {
		cmp = (*p1 < *p2) ? -1 : (*p1 > *p2) ? 1 : 0;
		if (cmp == 0) {
			cmp = strncmp (s1, s2, max(p1 - s1, p2 - s2));
			if (cmp == 0)
				cmp = strcmp(p1+1, p2+1);
		}
	} else // not a normal call, do a simple string comparison
		cmp = strcmp(s1, s2);

	if (cmp != 0)
		return cmp;

// compare by date
	cmp = strcmp( r1->getField(QSO_DATE), r2->getField(QSO_DATE));
	if (cmp != 0) return cmp;

// compare by time
	int t1 = atoi(r1->getField(TIME_ON));
	int t2 = atoi(r2->getField(TIME_ON));
	if (abs(t1 - t2) > 200) { // changed from 2 to accommodate seconds
		if (t1 < t2)
			return -1;
		if (t1 > t2)
			return 1;
	} // matched with +/- 2 minutes

// compare by mode
	const char *m1 = r1->getField(MODE); // needle
	const char *m2 = r2->getField(MODE); // haystack
	if (strcasestr(m1, "DOM"))
		cmp = strncasecmp("DOM", m2, 3);
	else if (strcasestr(m2, m1)) // eQSL, LoTW use sparse MODE designators
		cmp = 0;
	else
		cmp = strcasecmp(m1, m2);
	if (cmp != 0) return cmp;

// compare by band
	cmp = strcasecmp( r1->getField(BAND), r2->getField(BAND));

if (cmp == 0) printf("r1: %s, %s, %s, %s, %s\nr2: %s, %s, %s, %s, %s\n",
r1->getField(CALL), r1->getField(QSO_DATE), r1->getField(TIME_ON), r1->getField(MODE), r1->getField(BAND),
r2->getField(CALL), r2->getField(QSO_DATE), r2->getField(TIME_ON), r2->getField(MODE), r2->getField(BAND));

	return cmp;
}

static void rxtext(const char *s)
{
	ReceiveText->addstr(s);
}

void merge_recs( cQsoDb *db, cQsoDb *mrgdb ) // (haystack, needle)
{
	static char msg1[100];
	static char msg2[100];
	static char msg3[100];
	sorttype origsort = lastsort;
	cQsoDb::reverse = false;
	cQsoDb *reject = new cQsoDb;
	cQsoDb *copy = new cQsoDb(db);
	cQsoDb *merged = new cQsoDb;

	snprintf(msg1, sizeof(msg1), "Read %d records", mrgdb->nbrRecs());
	LOG_INFO("%s", msg1);
	REQ(rxtext, "\n*** ");
	REQ(rxtext, msg1);

	db->clearDatabase();

	copy->SortByCall();
	mrgdb->SortByCall();

	int n = 0; // copy
	int m = 0; // merge
	int N = copy->nbrRecs();
	int M = mrgdb->nbrRecs();

	int cmp;
	for (;;) {
		if (n == N && m == M) break;
		if (n < N && m < M) {
			if ((cmp = comparerecs(copy->getRec(n), mrgdb->getRec(m))) <= 0) {
				db->qsoNewRec(copy->getRec(n));
				n++;
				if (cmp == 0) {
					reject->qsoNewRec(mrgdb->getRec(m));
					m++;
				}
			} else {
				if (db->nbrRecs() == 0) {
					db->qsoNewRec(mrgdb->getRec(m));
					merged->qsoNewRec(mrgdb->getRec(m));
				} else if (comparerecs(db->getRec(db->nbrRecs()-1), mrgdb->getRec(m)) != 0) {
						db->qsoNewRec(mrgdb->getRec(m));
						merged->qsoNewRec(mrgdb->getRec(m));
				} else {
					reject->qsoNewRec(mrgdb->getRec(m));
}
				m++;
			}
		} else if (n == N) {
			if (db->nbrRecs() == 0) {
				db->qsoNewRec(mrgdb->getRec(m));
				merged->qsoNewRec(mrgdb->getRec(m));
			} else if (comparerecs(db->getRec(db->nbrRecs()-1), mrgdb->getRec(m)) != 0) {
				db->qsoNewRec(mrgdb->getRec(m));
				merged->qsoNewRec(mrgdb->getRec(m));
			} else {
				reject->qsoNewRec(mrgdb->getRec(m));
			}
			m++;
		} else {
			db->qsoNewRec(copy->getRec(n));
			n++;
		}
	}

	if (merged->nbrRecs()) {
		string mergedname = LogsDir;
		mergedname.append("merged_recs");
#ifdef __WIN32__
		mergedname.append(".adi");
#else
		mergedname.append(".adif");
#endif
		adifFile.writeLog (mergedname.c_str(), merged, true);
		snprintf(msg2, sizeof(msg2), "%d merged records saved in\n*** %s",
			merged->nbrRecs(), mergedname.c_str());
		REQ(rxtext, "\n*** ");
		REQ(rxtext, msg2);
		LOG_INFO("%s", msg2);
		db->isdirty(1);
	}

	if (reject->nbrRecs()) {
		string rejname = LogsDir;
		rejname.append("duplicate_recs");
#ifdef __WIN32__
		rejname.append(".adi");
#else
		rejname.append(".adif");
#endif
		adifFile.writeLog (rejname.c_str(), reject, true);
		snprintf(msg3, sizeof(msg3), "%d duplicates's saved in\n    %s", 
			reject->nbrRecs(), rejname.c_str());
		REQ(rxtext, "\n*** ");
		REQ(rxtext, msg3);
		LOG_INFO("%s", msg3);
	}

	delete reject;
	delete copy;
	delete merged;

	lastsort = origsort;
	restore_sort();
	loadBrowser();
}

void cb_mnuMergeADIF_log(Fl_Menu_* m, void* d) {
	const char* p = FSEL::select(_("Merge ADIF file"), "ADIF\t*." ADIF_SUFFIX);
	Fl::wait();
	fl_digi_main->redraw();
	Fl::awake();
	if (p) {
		cQsoDb *mrgdb = new cQsoDb;
		adifFile.do_readfile (p, mrgdb);
		merge_recs(&qsodb, mrgdb);
		delete mrgdb;
	}
}

void cb_export_date_select() {
	if (qsodb.nbrRecs() == 0) return;
	int start = atoi(inp_export_start_date->value());
	int stop = atoi(inp_export_stop_date->value());
	if (!start || !stop) return;
	int chkdate;
	chkExportBrowser->check_none();
	if (!btn_export_by_date->value()) return;

	cQsoRec *rec;
	for (int i = 0; i < qsodb.nbrRecs(); i++) {
		rec = qsodb.getRec (i);
		chkdate = atoi(rec->getField(progdefaults.sort_date_time_off ? QSO_DATE_OFF : QSO_DATE));
		if (chkdate >= start && chkdate <= stop)
			chkExportBrowser->checked(i+1, 1);
	}
	chkExportBrowser->redraw();
}

inline const char *szfreq(const char *freq)
{
	static char szf[11];
	float f = atof(freq);
	snprintf(szf, sizeof(szf), "%10.6f", f);
	return szf;
}

void cb_Export_log() {
	if (qsodb.nbrRecs() == 0) return;
	cQsoRec *rec;
	char line[80];
	chkExportBrowser->clear();
	chkExportBrowser->textfont(FL_COURIER);
	chkExportBrowser->textsize(12);
	for( int i = 0; i < qsodb.nbrRecs(); i++ ) {
		rec = qsodb.getRec (i);
		snprintf(line,sizeof(line),"%8s|%6s|%-10s|%10s|%-s",
 			rec->getField(QSO_DATE),
 			rec->getField(TIME_OFF),
 			rec->getField(CALL),
			szfreq(rec->getField(FREQ)),
			rec->getField(MODE) );
        chkExportBrowser->add(line);
	}
	cb_export_date_select();
	wExport->show();
}

void cb_mnuExportADIF_log(Fl_Menu_* m, void* d) {
	export_to = ADIF;
	cb_Export_log();
}

void cb_mnuExportCSV_log(Fl_Menu_* m, void* d) {
	export_to = CSV;
	cb_Export_log();
}

void cb_mnuExportTEXT_log(Fl_Menu_* m, void *d) {
	export_to = TEXT;
	cb_Export_log();
}


void cb_mnuShowLogbook(Fl_Menu_* m, void* d)
{
	dlgLogbook->show();
}

enum State {VIEWREC, NEWREC};
static State logState = VIEWREC;

void activateButtons() 
{
	if (logState == NEWREC) {
		bNewSave->label(_("Save"));
		bUpdateCancel->label(_("Cancel"));
		bUpdateCancel->activate();
		bDelete->deactivate ();
		bSearchNext->deactivate ();
		bSearchPrev->deactivate ();
		inpDate_log->take_focus();
		return;
	}
	bNewSave->label(_("New"));
	bUpdateCancel->label(_("Update"));
	if (qsodb.nbrRecs() > 0) {
		bDelete->activate();
		bUpdateCancel->activate();
		bSearchNext->activate ();
		bSearchPrev->activate ();
		wBrowser->take_focus();
	} else {
		bDelete->deactivate();
		bUpdateCancel->deactivate();
		bSearchNext->deactivate();
		bSearchPrev->deactivate();
	}
}

void cb_btnNewSave(Fl_Button* b, void* d) {
	if (logState == VIEWREC) {
		logState = NEWREC;
		clearRecord();
		activateButtons();
	} else {
		saveRecord();
		qsodb.SortByDate(progdefaults.sort_date_time_off);
		loadBrowser();
		logState = VIEWREC;
		activateButtons();
	}
}

void cb_btnUpdateCancel(Fl_Button* b, void* d) {
	if (logState == NEWREC) {
		logState = VIEWREC;
		activateButtons ();
	} else {
		updateRecord();
		wBrowser->take_focus();
	}
}

void cb_btnDelete(Fl_Button* b, void* d) {
	deleteRecord();
	wBrowser->take_focus();
}

void restore_sort()
{
	switch (lastsort) {
	case SORTCALL :
		cQsoDb::reverse = callfwd;
		qsodb.SortByCall();
		break;
	case SORTDATE :
		cQsoDb::reverse = progStatus.logbook_reverse;
		qsodb.SortByDate(progdefaults.sort_date_time_off);
		break;
	case SORTFREQ :
		cQsoDb::reverse = freqfwd;
		qsodb.SortByFreq();
		break;
	case SORTMODE :
		cQsoDb::reverse = modefwd;
		qsodb.SortByMode();
		break;
	default: break;
	}
}

void cb_SortByCall (void) {
	if (lastsort == SORTCALL)
		callfwd = !callfwd;
	else {
		callfwd = false;
		lastsort = SORTCALL;
	}
	cQsoDb::reverse = callfwd;
	qsodb.SortByCall();
	loadBrowser();
}

void cb_SortByDate (void) {
	if (lastsort == SORTDATE)
		progStatus.logbook_reverse = !progStatus.logbook_reverse;
	else {
		lastsort = SORTDATE;
	}
	cQsoDb::reverse = progStatus.logbook_reverse;
	qsodb.SortByDate(progdefaults.sort_date_time_off);
	loadBrowser();
}

void reload_browser()
{
	qsodb.SortByDate(progdefaults.sort_date_time_off);
	loadBrowser();
}

void cb_SortByMode (void) {
	if (lastsort == SORTMODE)
		modefwd = !modefwd;
	else {
		modefwd = false;
		lastsort = SORTMODE;
	}
	cQsoDb::reverse = modefwd;
	qsodb.SortByMode();
	loadBrowser();
}

void cb_SortByFreq (void) {
	if (lastsort == SORTFREQ)
		freqfwd = !freqfwd;
	else {
		freqfwd = false;
		lastsort = SORTFREQ;
	}
	cQsoDb::reverse = freqfwd;
	qsodb.SortByFreq();
	loadBrowser();
}

void DupCheck()
{
	Fl_Color call_clr = progdefaults.LOGGINGcolor;

	if (progdefaults.xml_logbook)
		if (xml_check_dup())
			call_clr = fl_rgb_color(
				progdefaults.dup_color.R,
				progdefaults.dup_color.G,
				progdefaults.dup_color.B);

	if (!progdefaults.xml_logbook && qsodb.duplicate(
			inpCall->value(),
			zdate(), ztime(), progdefaults.timespan, progdefaults.duptimespan,
			inpFreq->value(), progdefaults.dupband,
			inpState->value(), progdefaults.dupstate,
			mode_info[active_modem->get_mode()].adif_name, progdefaults.dupmode,
			inpXchgIn->value(), progdefaults.dupxchg1 ) ) {
		call_clr = fl_rgb_color(
			progdefaults.dup_color.R,
			progdefaults.dup_color.G,
			progdefaults.dup_color.B);
	}

	inpCall1->color(call_clr);
	inpCall2->color(call_clr);
	inpCall3->color(call_clr);
	inpCall4->color(call_clr);
	inpCall1->redraw();
	inpCall2->redraw();
	inpCall3->redraw();
	inpCall4->redraw();
}

cQsoRec* SearchLog(const char *callsign)
{
	size_t len = strlen(callsign);
	char* re = new char[len + 3];
	snprintf(re, len + 3, "^%s$", callsign);

	int row = 0, col = 2;
	return wBrowser->search(row, col, !cQsoDb::reverse, re) ? qsodb.getRec(row) : 0;
}

void SearchLastQSO(const char *callsign)
{
	size_t len = strlen(callsign);
	if (len < 3)
		return;

	if (progdefaults.xml_logbook) {
		if(xml_get_record(callsign))
			return;
	}

	Fl::focus(inpCall);

	char* re = new char[len + 3];
	snprintf(re, len + 3, "^%s$", callsign);

	int row = 0, col = 2;
	if (wBrowser->search(row, col, !cQsoDb::reverse, re)) {
		wBrowser->GotoRow(row);
		inpName->value(inpName_log->value());
		inpQth->value(inpQth_log->value());
		inpLoc->value(inpLoc_log->value());
		inpState->value(inpState_log->value());
		inpVEprov->value(inpVE_Prov_log->value ());
		inpCountry->value(inpCountry_log->value ());
		inpSearchString->value(callsign);
		if (inpLoc->value()[0]) {
			double lon1, lat1, lon2, lat2;
			double azimuth, distance;
			char szAZ[4];
			if ( locator2longlat(&lon1, &lat1, progdefaults.myLocator.c_str()) == RIG_OK &&
				 locator2longlat(&lon2, &lat2, inpLoc->value()) == RIG_OK &&
				 qrb(lon1, lat1, lon2, lat2, &distance, &azimuth) == RIG_OK ) {
				snprintf(szAZ,sizeof(szAZ),"%0.f", azimuth);
				inpAZ->value(szAZ);
			} else
				inpAZ->value("");
		} else
			inpAZ->value("");
	} else {
		inpName->value("");
		inpQth->value("");
		inpLoc->value("");
		inpState->value("");
		inpVEprov->value("");
		inpCountry->value("");
		inpAZ->value("");
		inpSearchString->value("");
	}
	delete [] re;
}

void cb_search(Fl_Widget* w, void*)
{
	const char* str = inpSearchString->value();
	if (!*str)
		return;

	bool rev = w == bSearchPrev;
	int col = 2, row = wBrowser->value() + (rev ? -1 : 1);
	row = WCLAMP(row, 0, wBrowser->rows() - 1);

	if (wBrowser->search(row, col, rev, str))
		wBrowser->GotoRow(row);
	wBrowser->take_focus();
}

int log_search_handler(int)
{
	if (!(Fl::event_state() & FL_CTRL))
		return 0;

	switch (Fl::event_key()) {
	case 's':
		bSearchNext->do_callback();
		break;
	case 'r':
		bSearchPrev->do_callback();
		break;
	default:
		return 0;
	}
	return 1;
}

static int editNbr = 0;

void cb_btnDialFreq(Fl_Button* b, void* d) 
{
	double drf  = atof(inpFreq_log->value());
	if (!drf) return;

	int rf1, rf, audio;
	rf1 = drf * 1e6;
	rf = rf1 / 10000;
	rf *= 10000;
	audio = rf1 - rf;
// try to keep within normal xcvr bw, 500 - 3000 Hz
	while (audio > 3000) {
		audio -= 3000;
		rf += 3000;
	}
	if (audio < 500) {
		audio += 500;
		rf -= 500;
	}
	qsy(rf, audio);

	std::string mode_name = inpMode_log->value();
	trx_mode m;
	for (m = 0; m < NUM_MODES; m++)
		if (mode_name == mode_info[m].adif_name)
			break;
	// do we have a valid modem?
	if (m < NUM_MODES && active_modem->get_mode() != mode_info[m].mode)
			init_modem(mode_info[m].mode);

	const cQsoRec *qsoPtr = qsodb.getRec(editNbr);
	inpCall->value(qsoPtr->getField(CALL));
	inpName->value (qsoPtr->getField(NAME));
	inpTimeOn->value (inpTimeOff->value());
	inpState->value (qsoPtr->getField(STATE));
	inpCountry->value (qsoPtr->getField(COUNTRY));
	inpXchgIn->value(qsoPtr->getField(XCHG1));
	inpQth->value (qsoPtr->getField(QTH));
	inpLoc->value (qsoPtr->getField(GRIDSQUARE));
	inpNotes->value (qsoPtr->getField(NOTES));

	wBrowser->take_focus();
}

void clearRecord() {
	Date tdy;
	inpCall_log->value ("");
	inpName_log->value ("");
	inpDate_log->value (tdy.szDate(2));
	inpDateOff_log->value (tdy.szDate(2));
	inpTimeOn_log->value ("");
	inpTimeOff_log->value ("");
	inpRstR_log->value ("");
	inpRstS_log->value ("");
	inpFreq_log->value ("");
	inpMode_log->value ("");
	inpQth_log->value ("");
	inpState_log->value ("");
	inpVE_Prov_log->value ("");
	inpCountry_log->value ("");
	inpLoc_log->value ("");
	inpQSLrcvddate_log->value ("");
	inpQSLsentdate_log->value ("");
	inpSerNoOut_log->value ("");
	inpSerNoIn_log->value ("");
	inpXchgIn_log->value("");
	inpMyXchg_log->value(progdefaults.myXchg.c_str());
	inpNotes_log->value ("");
	inpIOTA_log->value("");
	inpDXCC_log->value("");
	inpQSL_VIA_log->value("");
	inpCONT_log->value("");
	inpCNTY_log->value("");
	inpCQZ_log->value("");
	inpITUZ_log->value("");
	inpTX_pwr_log->value("");
	inpSearchString->value ("");
}

void saveRecord() {
	cQsoRec rec;
	rec.putField(CALL, inpCall_log->value());
	rec.putField(NAME, inpName_log->value());
	rec.putField(QSO_DATE, inpDate_log->value());
	rec.putField(QSO_DATE_OFF, inpDateOff_log->value());

	string tm = timestring(inpTimeOn_log->value());
	rec.putField(TIME_ON, tm.c_str());
	inpTimeOn_log->value(timeview(tm.c_str()));

	tm = timestring(inpTimeOff_log->value());
	rec.putField(TIME_OFF, tm.c_str());
	inpTimeOff_log->value(timeview(tm.c_str()));

	rec.putField(FREQ, inpFreq_log->value());
	rec.putField(MODE, inpMode_log->value());
	rec.putField(QTH, inpQth_log->value());
	rec.putField(STATE, inpState_log->value());
	rec.putField(VE_PROV, inpVE_Prov_log->value());
	rec.putField(COUNTRY, inpCountry_log->value());
	rec.putField(GRIDSQUARE, inpLoc_log->value());
	rec.putField(NOTES, inpNotes_log->value());
	rec.putField(QSLRDATE, inpQSLrcvddate_log->value());
	rec.putField(QSLSDATE, inpQSLsentdate_log->value());
	rec.putField(RST_RCVD, inpRstR_log->value ());
	rec.putField(RST_SENT, inpRstS_log->value ());
	rec.putField(SRX, inpSerNoIn_log->value());
	rec.putField(STX, inpSerNoOut_log->value());
	rec.putField(XCHG1, inpXchgIn_log->value());
	if (!qso_exchange.empty()) {
		rec.putField(MYXCHG, qso_exchange.c_str());
		qso_exchange.clear();
		qso_time.clear();
	} else if (!qso_time.empty()) {
		string myexch = inpMyXchg_log->value();
		myexch.append(" ").append(qso_time);
		rec.putField(MYXCHG, myexch.c_str());
		qso_time.clear();
	} else {
		rec.putField(MYXCHG, inpMyXchg_log->value());
	}
	rec.putField(CNTY, inpCNTY_log->value());
	rec.putField(IOTA, inpIOTA_log->value());
	rec.putField(DXCC, inpDXCC_log->value());
	rec.putField(DXCC, inpQSL_VIA_log->value());
	rec.putField(CONT, inpCONT_log->value());
	rec.putField(CQZ, inpCQZ_log->value());
	rec.putField(ITUZ, inpITUZ_log->value());
	rec.putField(TX_PWR, inpTX_pwr_log->value());

	qsodb.qsoNewRec (&rec);
	dxcc_entity_cache_add(&rec);
	submit_record(rec);

	cQsoDb::reverse = false;
	qsodb.SortByDate(progdefaults.sort_date_time_off);

	qsodb.isdirty(0);

	loadBrowser();

	adifFile.writeLog (logbook_filename.c_str(), &qsodb);
}

void updateRecord() {
cQsoRec rec;
	if (qsodb.nbrRecs() == 0) return;
	rec.putField(CALL, inpCall_log->value());
	rec.putField(NAME, inpName_log->value());
	rec.putField(QSO_DATE, inpDate_log->value());
	rec.putField(QSO_DATE_OFF, inpDateOff_log->value());

	string tm = timestring(inpTimeOn_log->value());
	rec.putField(TIME_ON, tm.c_str());
	inpTimeOn_log->value(timeview(tm.c_str()));

	tm = timestring(inpTimeOff_log->value());
	rec.putField(TIME_OFF, tm.c_str());
	inpTimeOff_log->value(timeview(tm.c_str()));

	rec.putField(FREQ, inpFreq_log->value());
	rec.putField(MODE, inpMode_log->value());
	rec.putField(QTH, inpQth_log->value());
	rec.putField(STATE, inpState_log->value());
	rec.putField(VE_PROV, inpVE_Prov_log->value());
	rec.putField(COUNTRY, inpCountry_log->value());
	rec.putField(GRIDSQUARE, inpLoc_log->value());
	rec.putField(NOTES, inpNotes_log->value());
	rec.putField(QSLRDATE, inpQSLrcvddate_log->value());
	rec.putField(QSLSDATE, inpQSLsentdate_log->value());
	rec.putField(RST_RCVD, inpRstR_log->value ());
	rec.putField(RST_SENT, inpRstS_log->value ());
	rec.putField(SRX, inpSerNoIn_log->value());
	rec.putField(STX, inpSerNoOut_log->value());
	rec.putField(XCHG1, inpXchgIn_log->value());
	rec.putField(MYXCHG, inpMyXchg_log->value());
	rec.putField(CNTY, inpCNTY_log->value());
	rec.putField(IOTA, inpIOTA_log->value());
	rec.putField(DXCC, inpDXCC_log->value());
	rec.putField(QSL_VIA, inpQSL_VIA_log->value());
	rec.putField(CONT, inpCONT_log->value());
	rec.putField(CQZ, inpCQZ_log->value());
	rec.putField(ITUZ, inpITUZ_log->value());
	rec.putField(TX_PWR, inpTX_pwr_log->value());
	dxcc_entity_cache_rm(qsodb.getRec(editNbr));
	qsodb.qsoUpdRec (editNbr, &rec);
	dxcc_entity_cache_add(&rec);

	cQsoDb::reverse = false;
	qsodb.SortByDate(progdefaults.sort_date_time_off);

	qsodb.isdirty(0);
	restore_sort();

	loadBrowser(true);

	adifFile.writeLog (logbook_filename.c_str(), &qsodb);

}

void deleteRecord () {
	if (qsodb.nbrRecs() == 0 || fl_choice2(_("Really delete record for \"%s\"?"),
					       _("Yes"), _("No"), NULL, wBrowser->valueAt(-1, 2)))
		return;

	dxcc_entity_cache_rm(qsodb.getRec(editNbr));
	qsodb.qsoDelRec(editNbr);

	cQsoDb::reverse = false;
	qsodb.SortByDate(progdefaults.sort_date_time_off);

	qsodb.isdirty(0);
	restore_sort();

	loadBrowser(true);

	adifFile.writeLog (logbook_filename.c_str(), &qsodb);

}

void EditRecord( int i )
{
	cQsoRec *editQSO = qsodb.getRec (i);
	if( !editQSO )
		return;

	inpCall_log->value (editQSO->getField(CALL));
	inpName_log->value (editQSO->getField(NAME));
	inpDate_log->value (editQSO->getField(QSO_DATE));
	inpDateOff_log->value (editQSO->getField(QSO_DATE_OFF));
	inpTimeOn_log->value (timeview(editQSO->getField(TIME_ON)));
	inpTimeOff_log->value (timeview(editQSO->getField(TIME_OFF)));
	inpRstR_log->value (editQSO->getField(RST_RCVD));
	inpRstS_log->value (editQSO->getField(RST_SENT));
	inpFreq_log->value (editQSO->getField(FREQ));
	inpMode_log->value (editQSO->getField(MODE));
	inpState_log->value (editQSO->getField(STATE));
	inpVE_Prov_log->value (editQSO->getField(VE_PROV));
	inpCountry_log->value (editQSO->getField(COUNTRY));
	inpQth_log->value (editQSO->getField(QTH));
	inpLoc_log->value (editQSO->getField(GRIDSQUARE));
	inpQSLrcvddate_log->value (editQSO->getField(QSLRDATE));
	inpQSLsentdate_log->value (editQSO->getField(QSLSDATE));
	inpNotes_log->value (editQSO->getField(NOTES));
	inpSerNoIn_log->value(editQSO->getField(SRX));
	inpSerNoOut_log->value(editQSO->getField(STX));
	inpXchgIn_log->value(editQSO->getField(XCHG1));
	inpMyXchg_log->value(editQSO->getField(MYXCHG));
	inpCNTY_log->value(editQSO->getField(CNTY));
	inpIOTA_log->value(editQSO->getField(IOTA));
	inpDXCC_log->value(editQSO->getField(DXCC));
	inpQSL_VIA_log->value(editQSO->getField(QSL_VIA));
	inpCONT_log->value(editQSO->getField(CONT));
	inpCQZ_log->value(editQSO->getField(CQZ));
	inpITUZ_log->value(editQSO->getField(ITUZ));
	inpTX_pwr_log->value(editQSO->getField(TX_PWR));
}

std::string sDate_on = "";
std::string sTime_on = "";
std::string sDate_off = "";
std::string sTime_off = "";

void AddRecord ()
{
	inpCall_log->value(inpCall->value());
	inpName_log->value (inpName->value());

	if (progdefaults.force_date_time) {
		inpDate_log->value(sDate_off.c_str());
		inpTimeOn_log->value (timeview(sTime_off.c_str()));
	} else {
		inpDate_log->value(sDate_on.c_str());
		inpTimeOn_log->value (timeview(sTime_on.c_str()));
	}
	inpDateOff_log->value(sDate_off.c_str());
	inpTimeOff_log->value (timeview(sTime_off.c_str()));

	inpRstR_log->value (inpRstIn->value());
	inpRstS_log->value (inpRstOut->value());
	{
		char Mhz[30];
		snprintf(Mhz, sizeof(Mhz), "%-f", atof(inpFreq->value()) / 1000.0);
		inpFreq_log->value(Mhz);
	}
	inpMode_log->value (logmode);
	inpState_log->value (inpState->value());
	inpVE_Prov_log->value (inpVEprov->value());
	inpCountry_log->value (inpCountry->value());

	inpSerNoIn_log->value(inpSerNo->value());
	inpSerNoOut_log->value(outSerNo->value());
	inpXchgIn_log->value(inpXchgIn->value());
	inpMyXchg_log->value(progdefaults.myXchg.c_str());

	inpQth_log->value (inpQth->value());
	inpLoc_log->value (inpLoc->value());
	inpQSLrcvddate_log->value ("");
	inpQSLsentdate_log->value ("");
	inpNotes_log->value (inpNotes->value());

	inpTX_pwr_log->value (progdefaults.mytxpower.c_str());
	inpCNTY_log->value("");
	inpIOTA_log->value("");
	inpDXCC_log->value("");
	inpQSL_VIA_log->value("");
	inpCONT_log->value("");
	inpCQZ_log->value("");
	inpITUZ_log->value("");

	saveRecord();

	restore_sort();
	loadBrowser();
	logState = VIEWREC;
	activateButtons();
}

void cb_browser (Fl_Widget *w, void *data )
{
	Table *table = (Table *)w;
	editNbr = atoi(table->valueAt(-1,6));
	EditRecord (editNbr);
}

void loadBrowser(bool keep_pos)
{
	cQsoRec *rec;
	char sNbr[6];
	int row = wBrowser->value(), pos = wBrowser->scrollPos();
	if (row >= qsodb.nbrRecs()) row = qsodb.nbrRecs() - 1;
	wBrowser->clear();
	if (qsodb.nbrRecs() == 0)
		return;
	for( int i = 0; i < qsodb.nbrRecs(); i++ ) {
		rec = qsodb.getRec (i);
		snprintf(sNbr,sizeof(sNbr),"%d",i);
		wBrowser->addRow (7,
			rec->getField(progdefaults.sort_date_time_off ? QSO_DATE_OFF : QSO_DATE),
			timeview4(rec->getField(progdefaults.sort_date_time_off ? TIME_OFF : TIME_ON)),
			rec->getField(CALL),
			rec->getField(NAME),
			rec->getField(FREQ),
			rec->getField(MODE),
			sNbr);
	}
	if (keep_pos && row >= 0) {
		wBrowser->value(row);
		wBrowser->scrollTo(pos);
	}
	else {
		if (cQsoDb::reverse == true)
			wBrowser->FirstRow ();
		else
			wBrowser->LastRow ();
	}
	char szRecs[6];
	snprintf(szRecs, sizeof(szRecs), "%5d", qsodb.nbrRecs());
	txtNbrRecs_log->value(szRecs);
}

//=============================================================================
// Cabrillo reporter
//=============================================================================

const char *contests[] =
{	"AP-SPRINT",
	"ARRL-10", "ARRL-160", "ARRL-DX-CW", "ARRL-DX-SSB", "ARRL-SS-CW",
	"ARRL-SS-SSB", "ARRL-UHF-AUG", "ARRL-VHF-JAN", "ARRL-VHF-JUN", "ARRL-VHF-SEP",
	"ARRL-RTTY",
	"BARTG-RTTY", "BARTG-SPRINT",
	"CQ-160-CW", "CQ-160-SSB", "CQ-WPX-CW", "CQ-WPX-RTTY", "CQ-WPX-SSB", "CQ-VHF",
	"CQ-WW-CW", "CQ-WW-RTTY", "CQ-WW-SSB",
	"DARC-WAEDC-CW", "DARC-WAEDC-RTTY", "DARC-WAEDC-SSB",
	"FCG-FQP", "IARU-HF", "JIDX-CW", "JIDX-SSB",
	"NAQP-CW", "NAQP-RTTY", "NAQP-SSB", "NA-SPRINT-CW", "NA-SPRINT-SSB", "NCCC-CQP",
	"NEQP", "OCEANIA-DX-CW", "OCEANIA-DX-SSB", "RDXC", "RSGB-IOTA",
	"SAC-CW", "SAC-SSB", "STEW-PERRY", "TARA-RTTY", 0 };

enum icontest {
	AP_SPRINT,
	ARRL_10, ARRL_160, ARRL_DX_CW, ARRL_DX_SSB, ARRL_SS_CW,
	ARRL_SS_SSB, ARRL_UHF_AUG, ARRL_VHF_JAN, ARRL_VHF_JUN, ARRL_VHF_SEP,
	ARRL_RTTY,
	BARTG_RTTY, BARTG_SPRINT,
	CQ_160_CW, CQ_160_SSB, CQ_WPX_CW, CQ_WPX_RTTY, CQ_WPX_SSB, CQ_VHF,
	CQ_WW_CW, CQ_WW_RTTY, CQ_WW_SSB,
	DARC_WAEDC_CW, DARC_WAEDC_RTTY, DARC_WAEDC_SSB,
	FCG_FQP, IARU_HF, JIDX_CW, JIDX_SSB,
	NAQP_CW, NAQP_RTTY, NAQP_SSB, NA_SPRINT_CW, NA_SPRINT_SSB, NCCC_CQP,
	NEQP, OCEANIA_DX_CW, OCEANIA_DX_SSB, RDXC, RSGB_IOTA,
	SAC_CW, SAC_SSB, STEW_PERRY, TARA_RTTY
};

bool bInitCombo = true;
icontest contestnbr;

void setContestType()
{
    contestnbr = (icontest)cboContest->index();

   	btnCabCall->value(true);	btnCabFreq->value(true);	btnCabMode->value(true);
   	btnCabQSOdate->value(true); btnCabTimeOFF->value(true);	btnCabRSTsent->value(true);
   	btnCabRSTrcvd->value(true);	btnCabSerialIN->value(true);btnCabSerialOUT->value(true);
   	btnCabXchgIn->value(true);	btnCabMyXchg->value(true);

    switch (contestnbr) {
    	case ARRL_SS_CW :
    	case ARRL_SS_SSB :
    		btnCabRSTrcvd->value(false);
    		break;
    	case BARTG_RTTY :
    	case BARTG_SPRINT :
    		break;
    	case ARRL_UHF_AUG :
    	case ARRL_VHF_JAN :
    	case ARRL_VHF_JUN :
    	case ARRL_VHF_SEP :
    	case CQ_VHF :
    		btnCabRSTrcvd->value(false);
			btnCabSerialIN->value(false);
			btnCabSerialOUT->value(false);
    		break;
    	case AP_SPRINT :
    	case ARRL_10 :
    	case ARRL_160 :
		case ARRL_DX_CW :
		case ARRL_DX_SSB :
    	case CQ_160_CW :
    	case CQ_160_SSB :
    	case CQ_WPX_CW :
		case CQ_WPX_RTTY :
		case CQ_WPX_SSB :
		case RDXC :
		case OCEANIA_DX_CW :
		case OCEANIA_DX_SSB :
    	    break;
    	case DARC_WAEDC_CW :
    	case DARC_WAEDC_RTTY :
    	case DARC_WAEDC_SSB :
    		break;
    	case NAQP_CW :
    	case NAQP_RTTY :
    	case NAQP_SSB :
    	case NA_SPRINT_CW :
    	case NA_SPRINT_SSB :
    		break;
    	case RSGB_IOTA :
    		break;
    	default :
    		break;
	}
}

void cb_Export_Cabrillo(Fl_Menu_* m, void* d) {
	if (qsodb.nbrRecs() == 0) return;
	cQsoRec *rec;
	char line[80];
	int indx = 0;

	if (bInitCombo) {
		bInitCombo = false;
		while (contests[indx]) 	{
			cboContest->add(contests[indx]);
			indx++;
		}
	}
	cboContest->index(0);
	chkCabBrowser->clear();
	chkCabBrowser->textfont(FL_COURIER);
	chkCabBrowser->textsize(12);
	for( int i = 0; i < qsodb.nbrRecs(); i++ ) {
		rec = qsodb.getRec (i);
		memset(line, 0, sizeof(line));
		snprintf(line,sizeof(line),"%8s|%4s|%-10s|%10s|%-s",
 			rec->getField(QSO_DATE),
 			time4(rec->getField(TIME_OFF)),
 			rec->getField(CALL),
			szfreq(rec->getField(FREQ)),
			rec->getField(MODE) );
        chkCabBrowser->add(line);
	}
	wCabrillo->show();
}

void cabrillo_append_qso (FILE *fp, cQsoRec *rec)
{
	char freq[16] = "";
	string rst_in, rst_out, exch_in, exch_out, date, time, mode, mycall, call;
	string qsoline = "QSO: ";
	int rst_len = 3;
	int ifreq = 0;
	size_t len = 0;

	exch_out.clear();

	if (btnCabFreq->value()) {
		ifreq = (int)(1000.0 * atof(rec->getField(FREQ)));
		snprintf(freq, sizeof(freq), "%d", ifreq);
		qsoline.append(freq); qsoline.append(" ");
	}

	if (btnCabMode->value()) {
		mode = rec->getField(MODE);
		if (mode.compare("USB") == 0 || mode.compare("LSB") == 0 ||
			mode.compare("SSB") == 0 || mode.compare("PH") == 0 ) mode = "PH";
		else if (mode.compare("FM") == 0 || mode.compare("CW") == 0 ) ;
		else mode = "RY";
		if (mode.compare("PH") == 0 || mode.compare("FM") == 0 ) rst_len = 2;
		qsoline.append(mode); qsoline.append(" ");
	}

	if (btnCabQSOdate->value()) {
		date = rec->getField(progdefaults.sort_date_time_off ? QSO_DATE_OFF : QSO_DATE);
		date.insert(4,"-");
		date.insert(7,"-");
		qsoline.append(date); qsoline.append(" ");
	}

	if (btnCabTimeOFF->value()) {
		time = rec->getField(progdefaults.sort_date_time_off ? TIME_OFF : TIME_ON);
		qsoline.append(time4(time.c_str())); qsoline.append(" ");
	}

	mycall = progdefaults.myCall;
	if (mycall.length() > 13) mycall = mycall.substr(0,13);
	if ((len = mycall.length()) < 13) mycall.append(13 - len, ' ');
	qsoline.append(mycall); qsoline.append(" ");

	if (btnCabRSTsent->value()) {
		rst_out = rec->getField(RST_SENT);
		rst_out = rst_out.substr(0,rst_len);
		exch_out.append(rst_out).append(" ");
	}

	if (btnCabSerialOUT->value()) {
		exch_out.append(rec->getField(STX)).append(" ");
	}

	if (btnCabMyXchg->value()) {
		exch_out.append(rec->getField(MYXCHG)).append(" ");
	}

	if (contestnbr == BARTG_RTTY && exch_out.length() < 11) {
		exch_out.append(rec->getField(TIME_OFF)).append(" ");
	}

	if (exch_out.length() > 14) exch_out = exch_out.substr(0,14);
	if ((len = exch_out.length()) < 14) exch_out.append(14 - len, ' ');

	qsoline.append(exch_out).append(" ");

	if (btnCabCall->value()) {
		call = rec->getField(CALL);
		if (call.length() > 13) call = call.substr(0,13);
		if ((len = call.length()) < 13) call.append(13 - len, ' ');
		qsoline.append(call); qsoline.append(" ");
	}

	if (btnCabRSTrcvd->value()) {
		rst_in = rec->getField(RST_RCVD);
		rst_in = rst_in.substr(0,rst_len);
		qsoline.append(rst_in); qsoline.append(" ");
	}

	if (btnCabSerialIN->value()) {
		exch_in = rec->getField(SRX);
		if (exch_in.length())
			exch_in += ' ';
	}
	if (btnCabXchgIn->value())
		exch_in.append(rec->getField(XCHG1));
	if (exch_in.length() > 14) exch_in = exch_in.substr(0,14);
	if ((len = exch_in.length()) < 14) exch_in.append(14 - len, ' ');
	qsoline.append(exch_in);

	fprintf (fp, "%s\n", qsoline.c_str());
	return;
}

void WriteCabrillo()
{
	if (chkCabBrowser->nchecked() == 0) return;

	cQsoRec *rec;

	string title = _("Create cabrillo report");
	string filters = "TEXT\t*.txt";
#ifdef __APPLE__
	filters.append("\n");
#endif
	string strContest = "";

	const char* p = FSEL::saveas( title.c_str(), filters.c_str(), "contest.txt");
	if (!p)
		return;

	for (int i = 0; i < chkCabBrowser->FLTK_nitems(); i++) {
		if (chkCabBrowser->checked(i + 1)) {
			rec = qsodb.getRec(i);
			rec->putField(EXPORT, "E");
			qsodb.qsoUpdRec (i, rec);
		}
	}

	string sp = p;
	if (sp.find(".txt") == string::npos) sp.append(".txt");
    FILE *cabFile = fopen (p, "w");
    if (!cabFile)
        return;

    strContest = cboContest->value();
    contestnbr = (icontest)cboContest->index();

	fprintf (cabFile,
"START-OF-LOG: 3.0\n\
CREATED-BY: %s %s\n\
\n\
# The callsign used during the contest.\n\
CALLSIGN: %s\n\
\n\
# ASSISTED or NON-ASSISTED\n\
CATEGORY-ASSISTED: \n\
\n\
# Band: ALL, 160M, 80M, 40M, 20M, 15M, 10M, 6M, 2M, 222, 432, 902, 1.2G\n\
CATEGORY-BAND: \n\
\n\
# Mode: SSB, CW, RTTY, MIXED \n\
CATEGORY-MODE: \n\
\n\
# Operator: SINGLE-OP, MULTI-OP, CHECKLOG \n\
CATEGORY-OPERATOR: \n\
\n\
# Power: HIGH, LOW, QRP \n\
CATEGORY-POWER: \n\
\n\
# Station: FIXED, MOBILE, PORTABLE, ROVER, EXPEDITION, HQ, SCHOOL \n\
CATEGORY-STATION: \n\
\n\
# Time: 6-HOURS, 12-HOURS, 24-HOURS \n\
CATEGORY-TIME: \n\
\n\
# Transmitter: ONE, TWO, LIMITED, UNLIMITED, SWL \n\
CATEGORY-TRANSMITTER: \n\
\n\
# Overlay: ROOKIE, TB-WIRES, NOVICE-TECH, OVER-50 \n\
CATEGORY-OVERLAY: \n\
\n\
# Integer number\n\
CLAIMED-SCORE: \n\
\n\
# Name of the radio club with which the score should be aggregated.\n\
CLUB: \n\
\n\
# Contest: AP-SPRINT, ARRL-10, ARRL-160, ARRL-DX-CW, ARRL-DX-SSB, ARRL-SS-CW,\n\
# ARRL-SS-SSB, ARRL-UHF-AUG, ARRL-VHF-JAN, ARRL-VHF-JUN, ARRL-VHF-SEP,\n\
# ARRL-RTTY, BARTG-RTTY, CQ-160-CW, CQ-160-SSB, CQ-WPX-CW, CQ-WPX-RTTY,\n\
# CQ-WPX-SSB, CQ-VHF, CQ-WW-CW, CQ-WW-RTTY, CQ-WW-SSB, DARC-WAEDC-CW,\n\
# DARC-WAEDC-RTTY, DARC-WAEDC-SSB, FCG-FQP, IARU-HF, JIDX-CW, JIDX-SSB,\n\
# NAQP-CW, NAQP-RTTY, NAQP-SSB, NA-SPRINT-CW, NA-SPRINT-SSB, NCCC-CQP,\n\
# NEQP, OCEANIA-DX-CW, OCEANIA-DX-SSB, RDXC, RSGB-IOTA, SAC-CW, SAC-SSB,\n\
# STEW-PERRY, TARA-RTTY \n\
CONTEST: %s\n\
\n\
# Optional email address\n\
EMAIL: \n\
\n\
LOCATION: \n\
\n\
# Operator name\n\
NAME: \n\
\n\
# Maximum 4 address lines.\n\
ADDRESS: \n\
ADDRESS: \n\
ADDRESS: \n\
ADDRESS: \n\
\n\
# A space-delimited list of operator callsign(s). \n\
OPERATORS: \n\
\n\
# Offtime yyyy-mm-dd nnnn yyyy-mm-dd nnnn \n\
# OFFTIME: \n\
\n\
# Soapbox comments.\n\
SOAPBOX: \n\
SOAPBOX: \n\
SOAPBOX: \n\n",
		PACKAGE_NAME, PACKAGE_VERSION,
		progdefaults.myCall.c_str(),
		strContest.c_str() );

	qsodb.SortByDate(progdefaults.sort_date_time_off);
    for (int i = 0; i < qsodb.nbrRecs(); i++) {
        rec = qsodb.getRec(i);
        if (rec->getField(EXPORT)[0] == 'E') {
        	cabrillo_append_qso(cabFile, rec);
            rec->putField(EXPORT,"");
            qsodb.qsoUpdRec(i, rec);
        }
    }
    fprintf(cabFile, "END-OF-LOG:\n");
    fclose (cabFile);
    return;
}


#include <tr1/unordered_map>
typedef tr1::unordered_map<string, unsigned> dxcc_entity_cache_t;
static dxcc_entity_cache_t dxcc_entity_cache;
static bool dxcc_entity_cache_enabled = false;

#include "dxcc.h"

static void dxcc_entity_cache_clear(void)
{
	if (dxcc_entity_cache_enabled)
		dxcc_entity_cache.clear();
}

static void dxcc_entity_cache_add(cQsoRec* r)
{
	if (!dxcc_entity_cache_enabled | !r)
		return;

	const dxcc* e = dxcc_lookup(r->getField(CALL));
	if (e)
		dxcc_entity_cache[e->country]++;
}
static void dxcc_entity_cache_add(cQsoDb& db)
{
	if (!dxcc_entity_cache_enabled)
		return;

	int n = db.nbrRecs();
	for (int i = 0; i < n; i++)
		dxcc_entity_cache_add(db.getRec(i));
	if (!dxcc_entity_cache.empty())
		LOG_INFO("Found %" PRIuSZ " countries in %d QSO records",
			 dxcc_entity_cache.size(), n);
}

static void dxcc_entity_cache_rm(cQsoRec* r)
{
	if (!dxcc_entity_cache_enabled || !r)
		return;

	const dxcc* e = dxcc_lookup(r->getField(CALL));
	if (!e)
		return;
	dxcc_entity_cache_t::iterator i = dxcc_entity_cache.find(e->country);
	if (i != dxcc_entity_cache.end()) {
		if (i->second)
			i->second--;
		else
			dxcc_entity_cache.erase(i);
	}
}

void dxcc_entity_cache_enable(bool v)
{
	if (dxcc_entity_cache_enabled == v)
		return;

	dxcc_entity_cache_clear();
	if ((dxcc_entity_cache_enabled = v))
		dxcc_entity_cache_add(qsodb);
}

bool qsodb_dxcc_entity_find(const char* country)
{
	return dxcc_entity_cache.find(country) != dxcc_entity_cache.end();
}
