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

#include "main.h"
#include "trx.h"
#include "debug.h"
#include "macros.h"
#include "status.h"
#include "date.h"

#include "adif_io.h"

#include "logsupport.h"
#include "textio.h"

#include "logger.h"
#include "lgbook.h"
#include "fl_digi.h"
#include "fileselect.h"
#include "configuration.h"
#include "main.h"
#include "locator.h"
#include "icons.h"
#include "gettext.h"

#include <FL/filename.H>
#include <FL/fl_ask.H>

using namespace std;

cQsoDb		qsodb;
cAdifIO		adifFile;
cTextFile	txtFile;

string		logbook_filename;

void Export_CSV()
{
	if (chkExportBrowser->nchecked() == 0) return;

	cQsoRec *rec;

	string filters = "CSV\t*." "csv";
	const char* p = FSEL::saveas(_("Export to CSV file"), filters.c_str(),
					 "export." "csv");
	if (!p)
		return;

	for (int i = 0; i < chkExportBrowser->nitems(); i++) {
		if (chkExportBrowser->checked(i + 1)) {
			rec = qsodb.getRec(i);
			rec->putField(EXPORT, "E");
			qsodb.qsoUpdRec (i, rec);
		}
	}
	txtFile.writeCSVFile(p, &qsodb);
}

void Export_TXT()
{
	if (chkExportBrowser->nchecked() == 0) return;

	cQsoRec *rec;

	string filters = "TEXT\t*." "txt";
	const char* p = FSEL::saveas(_("Export to fixed field text file"), filters.c_str(),
					 "export." "txt");
	if (!p)
		return;

	for (int i = 0; i < chkExportBrowser->nitems(); i++) {
		if (chkExportBrowser->checked(i + 1)) {
			rec = qsodb.getRec(i);
			rec->putField(EXPORT, "E");
			qsodb.qsoUpdRec (i, rec);
		}
	}
	txtFile.writeTXTFile(p, &qsodb);
}

void Export_ADIF()
{
	if (chkExportBrowser->nchecked() == 0) return;

	cQsoRec *rec;

	string filters = "ADIF\t*." ADIF_SUFFIX;
	const char* p = FSEL::saveas(_("Export to ADIF file"), filters.c_str(),
					 "export." ADIF_SUFFIX);
	if (!p)
		return;

	for (int i = 0; i < chkExportBrowser->nitems(); i++) {
		if (chkExportBrowser->checked(i + 1)) {
			rec = qsodb.getRec(i);
			rec->putField(EXPORT, "E");
			qsodb.qsoUpdRec (i, rec);
		}
	}

	adifFile.writeFile (p, &qsodb);
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
	if (adifFile.writeLog (logbook_filename.c_str(), &qsodb))
		fl_alert2(_("Could not update file %s"), logbook_filename.c_str());
	qsodb.isdirty(0);
}

static void dxcc_entity_cache_clear(void);
static void dxcc_entity_cache_add(cQsoRec* r);
static void dxcc_entity_cache_rm(cQsoRec* r);
static void dxcc_entity_cache_add(cQsoDb& db);

void cb_mnuNewLogbook(Fl_Menu_* m, void* d){
	saveLogbook();

	logbook_filename = LogsDir;
	logbook_filename.append("newlog." ADIF_SUFFIX);
	progdefaults.logbookfilename = logbook_filename;
	dlgLogbook->label(fl_filename_name(logbook_filename.c_str()));
	progdefaults.changed = true;
	qsodb.deleteRecs();
	dxcc_entity_cache_clear();
	wBrowser->clear();
	clearRecord();
}

void cb_mnuOpenLogbook(Fl_Menu_* m, void* d)
{
	const char* p = FSEL::select(_("Open logbook file"), "ADIF\t*." ADIF_SUFFIX,
				     logbook_filename.c_str());
	if (p) {
		saveLogbook();
		qsodb.deleteRecs();

		logbook_filename = p;
		progdefaults.logbookfilename = logbook_filename;
		progdefaults.changed = true;
		adifFile.readFile (logbook_filename.c_str(), &qsodb);
		dxcc_entity_cache_clear();
		dxcc_entity_cache_add(qsodb);
		qsodb.isdirty(0);
		loadBrowser();
		dlgLogbook->label(fl_filename_name(logbook_filename.c_str()));
	}
}

void cb_mnuSaveLogbook(Fl_Menu_*m, void* d) {
	const char* p = FSEL::saveas(_("Save logbook file"), "ADIF\t*." ADIF_SUFFIX,
				     logbook_filename.c_str());
	if (p) {
		logbook_filename = p;
		dlgLogbook->label(fl_filename_name(logbook_filename.c_str()));
		if (adifFile.writeLog (p, &qsodb))
			fl_alert2(_("Could not write to %s"), p);
		qsodb.isdirty(0);
	}
}

void cb_mnuMergeADIF_log(Fl_Menu_* m, void* d) {
	const char* p = FSEL::select(_("Merge ADIF file"), "ADIF\t*." ADIF_SUFFIX);
	if (p) {
		adifFile.readFile (p, &qsodb);
		dxcc_entity_cache_clear();
		dxcc_entity_cache_add(qsodb);
		loadBrowser();
		qsodb.isdirty(1);
	}
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
		memset(line, 0, sizeof(line));
		snprintf(line,sizeof(line),"%8s  %4s  %-32s  %10s  %-s",
			rec->getField(QSO_DATE),
			rec->getField(TIME_OFF),
			rec->getField(CALL),
			rec->getField(FREQ),
			rec->getField(MODE) );
        chkExportBrowser->add(line);
	}
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

void activateButtons() {

	if (logState == NEWREC) {
		bNewSave->label(_("Save"));
		bUpdateCancel->label(_("Cancel"));
		bDelete->deactivate ();
		bSearchNext->deactivate ();
		bSearchPrev->deactivate ();
		inpDate_log->take_focus();
		return;
	}
	bNewSave->label(_("New"));
	bUpdateCancel->label(_("Update"));
	bDelete->activate();
	bSearchNext->activate ();
	bSearchPrev->activate ();
	wBrowser->take_focus();
}

void cb_btnNewSave(Fl_Button* b, void* d) {
	if (logState == VIEWREC) {
		logState = NEWREC;
		clearRecord();
		activateButtons();
	} else {
		saveRecord();
		qsodb.SortByDate();
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

enum sorttype {NONE, SORTCALL, SORTDATE, SORTFREQ, SORTMODE};
sorttype lastsort = SORTDATE;

void cb_SortByCall (void) {
	if (lastsort == SORTCALL)
		cQsoDb::reverse = !cQsoDb::reverse;
	else {
		cQsoDb::reverse = false;
		lastsort = SORTCALL;
	}
	qsodb.SortByCall();
	loadBrowser();
}

void cb_SortByDate (void) {
	if (lastsort == SORTDATE)
		cQsoDb::reverse = !cQsoDb::reverse;
	else {
		cQsoDb::reverse = false;
		lastsort = SORTDATE;
	}
	qsodb.SortByDate();
	loadBrowser();
}

void cb_SortByMode (void) {
	if (lastsort == SORTMODE)
		cQsoDb::reverse = !cQsoDb::reverse;
	else {
		cQsoDb::reverse = false;
		lastsort = SORTMODE;
	}
	qsodb.SortByMode();
	loadBrowser();
}

void cb_SortByFreq (void) {
	if (lastsort == SORTFREQ)
		cQsoDb::reverse = !cQsoDb::reverse;
	else {
		cQsoDb::reverse = false;
		lastsort = SORTFREQ;
	}
	qsodb.SortByFreq();
	loadBrowser();
}

void DupCheck()
{
	Fl_Color call_clr = FL_BACKGROUND2_COLOR;
	if (qsodb.duplicate(
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
	if (!len)
		return;
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

int editNbr = 0;

void clearRecord() {
	Date tdy;
	inpCall_log->value ("");
	inpName_log->value ("");
	inpDate_log->value (tdy.szDate(2));
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
	inpCONT_log->value("");
	inpCQZ_log->value("");
	inpITUZ_log->value("");
	inpTX_pwr_log->value("");
	inpSearchString->value ("");
	editGroup->show();
}

void saveRecord() {
cQsoRec rec;
	rec.putField(CALL, inpCall_log->value());
	rec.putField(NAME, inpName_log->value());
	rec.putField(QSO_DATE, inpDate_log->value());
	rec.putField(TIME_ON, inpTimeOn_log->value());
	rec.putField(TIME_OFF, inpTimeOff_log->value());
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
	rec.putField(IOTA, inpIOTA_log->value());
	rec.putField(DXCC, inpDXCC_log->value());
	rec.putField(CONT, inpCONT_log->value());
	rec.putField(CQZ, inpCQZ_log->value());
	rec.putField(ITUZ, inpITUZ_log->value());
	rec.putField(TX_PWR, inpTX_pwr_log->value());

	adifFile.writeLog (logbook_filename.c_str(), &qsodb);
	qsodb.isdirty(0);

	qsodb.qsoNewRec (&rec);
	dxcc_entity_cache_add(&rec);
}

void updateRecord() {
cQsoRec rec;
	if (qsodb.nbrRecs() == 0) return;
	rec.putField(CALL, inpCall_log->value());
	rec.putField(NAME, inpName_log->value());
	rec.putField(QSO_DATE, inpDate_log->value());
	rec.putField(TIME_ON, inpTimeOn_log->value());
	rec.putField(TIME_OFF, inpTimeOff_log->value());
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
	rec.putField(IOTA, inpIOTA_log->value());
	rec.putField(DXCC, inpDXCC_log->value());
	rec.putField(CONT, inpCONT_log->value());
	rec.putField(CQZ, inpCQZ_log->value());
	rec.putField(ITUZ, inpITUZ_log->value());
	rec.putField(TX_PWR, inpTX_pwr_log->value());
	dxcc_entity_cache_rm(qsodb.getRec(editNbr));
	qsodb.qsoUpdRec (editNbr, &rec);
	dxcc_entity_cache_add(&rec);
	adifFile.writeLog (logbook_filename.c_str(), &qsodb);
	qsodb.isdirty(0);
	loadBrowser(true);
}

void deleteRecord () {
	if (qsodb.nbrRecs() == 0 || fl_choice2(_("Really delete record for \"%s\"?"),
					       _("Yes"), _("No"), NULL, wBrowser->valueAt(-1, 2)))
		return;

	dxcc_entity_cache_rm(qsodb.getRec(editNbr));
	qsodb.qsoDelRec(editNbr);
	adifFile.writeLog (logbook_filename.c_str(), &qsodb);
	qsodb.isdirty(0);

	loadBrowser(true);
}

void EditRecord( int i )
{
	cQsoRec *editQSO = qsodb.getRec (i);
	if( !editQSO )
		return;

	inpCall_log->value (editQSO->getField(CALL));
	inpName_log->value (editQSO->getField(NAME));
	inpDate_log->value (editQSO->getField(QSO_DATE));
	inpTimeOn_log->value (editQSO->getField(TIME_ON));
	inpTimeOff_log->value (editQSO->getField(TIME_OFF));
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
	inpIOTA_log->value(editQSO->getField(IOTA));
	inpDXCC_log->value(editQSO->getField(DXCC));
	inpCONT_log->value(editQSO->getField(CONT));
	inpCQZ_log->value(editQSO->getField(CQZ));
	inpITUZ_log->value(editQSO->getField(ITUZ));
	inpTX_pwr_log->value(editQSO->getField(TX_PWR));
	editGroup->show();
}

std::string sDate_on = "";

void AddRecord ()
{
	inpCall_log->value(inpCall->value());
	inpName_log->value (inpName->value());
	inpTimeOn_log->value (inpTimeOn->value());
	inpTimeOff_log->value (ztime());
    inpDate_log->value(sDate_on.c_str());
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
	inpIOTA_log->value("");
	inpDXCC_log->value("");
	inpCONT_log->value("");
	inpCQZ_log->value("");
	inpITUZ_log->value("");

	saveRecord();
	qsodb.SortByDate();
	adifFile.writeLog (logbook_filename.c_str(), &qsodb);
	qsodb.isdirty(0);

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
			rec->getField(QSO_DATE),
			rec->getField(TIME_OFF),
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
		snprintf(line,sizeof(line),"%8s  %4s  %-32s  %10s  %-s",
			rec->getField(QSO_DATE),
			rec->getField(TIME_OFF),
			rec->getField(CALL),
			rec->getField(FREQ),
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

	if (btnCabFreq->value()) {
		ifreq = (int)(1000.0 * atof(rec->getField(FREQ)));
		snprintf(freq, sizeof(freq), "%d", ifreq);
		qsoline.append(freq); qsoline.append(" ");
	}

	if (btnCabMode->value()) {
		mode = rec->getField(MODE);
		if (mode.compare("USB") == 0 || mode.compare("LSB") == 0 ||
		    mode.compare("PH") == 0 ) mode = "PH";
		else if (mode.compare("FM") == 0 || mode.compare("CW") == 0 ) ;
		else mode = "RY";
		if (mode.compare("PH") == 0 || mode.compare("FM") == 0 ) rst_len = 2;
		qsoline.append(mode); qsoline.append(" ");
	}

	if (btnCabQSOdate->value()) {
		date = rec->getField(QSO_DATE);
		date.insert(4,"-");
		date.insert(7,"-");
		qsoline.append(date); qsoline.append(" ");
	}

	if (btnCabTimeOFF->value()) {
		time = rec->getField(TIME_OFF);
		qsoline.append(time); qsoline.append(" ");
	}

	mycall = progdefaults.myCall;
	if (mycall.length() > 13) mycall = mycall.substr(0,13);
	if ((len = mycall.length()) < 13) mycall.append(13 - len, ' ');
	qsoline.append(mycall); qsoline.append(" ");

	if (btnCabRSTsent->value()) {
		rst_out = rec->getField(RST_SENT);
		rst_out = rst_out.substr(0,rst_len);
		qsoline.append(rst_out); qsoline.append(" ");
	}

	if (btnCabSerialOUT->value()) {
		exch_out = rec->getField(STX);
		if (exch_out.length())
			exch_out += ' ';
	}
	if (btnCabMyXchg->value()) {
		exch_out.append(rec->getField(MYXCHG));
		exch_out.append(" ");
	}
	if (contestnbr == BARTG_RTTY) {
		exch_out.append(rec->getField(TIME_OFF));
		exch_out.append(" ");
	}

	if (exch_out.length() > 10) exch_out = exch_out.substr(0,10);
	if ((len = exch_out.length()) < 10) exch_out.append(10 - len, ' ');
	qsoline.append(exch_out); qsoline.append(" ");

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
	if (exch_in.length() > 10) exch_in = exch_in.substr(0,10);
	if ((len = exch_in.length()) < 10) exch_in.append(10 - len, ' ');
	qsoline.append(exch_in);

	fprintf (fp, "%s\n", qsoline.c_str());
	return;
}

void WriteCabrillo()
{
	if (chkCabBrowser->nchecked() == 0) return;

	cQsoRec *rec;

	string filters = "TEXT\t*.txt";
	string strContest = "";

	const char* p = FSEL::saveas(_("Create cabrillo report"), filters.c_str(),
					 "contest.txt");
	if (!p)
		return;

	for (int i = 0; i < chkCabBrowser->nitems(); i++) {
		if (chkCabBrowser->checked(i + 1)) {
			rec = qsodb.getRec(i);
			rec->putField(EXPORT, "E");
			qsodb.qsoUpdRec (i, rec);
		}
	}

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

	qsodb.SortByDate();
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
