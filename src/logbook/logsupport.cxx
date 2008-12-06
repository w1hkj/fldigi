#include <config.h>

#include <stdlib.h>
#include <string>

#include "main.h"
#include "modem.h"
#include "debug.h"
#include "macros.h"
#include "status.h"
#include "spot.h"

#include "adif_io.h"

#include "logsupport.h"
#include "textio.h"

#include "logger.h"
#include "lgbook.h"
#include "fl_digi.h"
#include "fileselect.h"
#include "configuration.h"

#include <FL/fl_ask.H>

extern string HomeDir;

using namespace std;

cQsoDb		qsodb;
cAdifIO		adifFile;
cTextFile	txtFile;

string		logbook_filename;

bool EnableDupCheck = false;

enum savetype {ADIF, TEXT, LO};

void Export_log()
{
	if (chkExportBrowser->nchecked() == 0) return;

	cQsoRec *rec;

	string filters = "ADIF\t*." ADIF_SUFFIX "\n" "Text\t*.txt";
	int saveas;
	const char* p = FSEL::saveas("Export to file", filters.c_str(),
					 "export." ADIF_SUFFIX, &saveas);
	if (!p)
		return;

	for (int i = 0; i < chkExportBrowser->nitems(); i++) {
		if (chkExportBrowser->checked(i + 1)) {
			rec = qsodb.getRec(i);
			rec->putField(EXPORT, "E");
			qsodb.qsoUpdRec (i, rec);
		}
	}

	switch (saveas) {
	case TEXT :
		txtFile.writeFile(p, &qsodb);
		break;
	case ADIF :
		adifFile.writeFile (p, &qsodb);
		break;
	default:
		break;
	}
}

void saveLogbook()
{
	if (!qsodb.isdirty()) return;
	if (progdefaults.NagMe)
		if (!fl_choice("Save changed Logbook?", "No", "Yes", NULL)) 
			return;
	if (adifFile.writeLog (logbook_filename.c_str(), &qsodb))
		fl_message ("Could not update file %s", logbook_filename.c_str());
	qsodb.isdirty(0);
}

void cb_mnuNewLogbook(Fl_Menu_* m, void* d){
	saveLogbook();

	logbook_filename = HomeDir;
	logbook_filename.append("newlog." ADIF_SUFFIX);
	progdefaults.logbookfilename = logbook_filename;
	dlgLogbook->label(fl_filename_name(logbook_filename.c_str()));
	progdefaults.changed = true;
	qsodb.deleteRecs();
	wBrowser->clear();
	clearRecord();
}

void cb_mnuOpenLogbook(Fl_Menu_* m, void* d)
{
	const char* p = FSEL::select("Open logbook file", "ADIF\t*." ADIF_SUFFIX);
	if (p) {
		saveLogbook();
		qsodb.deleteRecs();
		
		logbook_filename = p;
		progdefaults.logbookfilename = logbook_filename;
		progdefaults.changed = true;
		adifFile.readFile (logbook_filename.c_str(), &qsodb);
		loadBrowser();
		qsodb.isdirty(0);
		dlgLogbook->label(fl_filename_name(logbook_filename.c_str()));
	}
}

void cb_mnuSaveLogbook(Fl_Menu_*m, void* d) {
	if (qsodb.nbrRecs() == 0) return;
	const char* p = FSEL::saveas("Save logbook file", "ADIF\t*." ADIF_SUFFIX,
				     logbook_filename.c_str());
	if (p) {
		if (adifFile.writeLog (p, &qsodb))
			fl_message ("Could not write to %s", p);
		qsodb.isdirty(0);
	}
}

void cb_mnuMergeADIF_log(Fl_Menu_* m, void* d) {
	const char* p = FSEL::select("Merge ADIF file", "ADIF\t*." ADIF_SUFFIX);
	if (p) {
		adifFile.readFile (p, &qsodb);
		loadBrowser();
		qsodb.isdirty(1);
	}
}

void cb_mnuExportADIF_log(Fl_Menu_* m, void* d) {
	if (qsodb.nbrRecs() == 0) return;
	cQsoRec *rec;
	char line[80];
	chkExportBrowser->clear();
	chkExportBrowser->textfont(FL_COURIER);
	chkExportBrowser->textsize(12);
	for( int i = 0; i < qsodb.nbrRecs(); i++ ) {
		rec = qsodb.getRec (i);
		memset(line, sizeof(line), 0);
		snprintf(line,sizeof(line),"%-10s%-6s%-12s%-12s%-s",
			rec->getField(QSO_DATE),
			rec->getField(TIME_ON),
			rec->getField(CALL),
			rec->getField(FREQ),
			rec->getField(MODE) );
        chkExportBrowser->add(line);
	}
	wExport->show();
}

void cb_mnuShowLogbook(Fl_Menu_* m, void* d)
{
	dlgLogbook->show();
}

enum State {VIEWREC, NEWREC};
static State logState = VIEWREC;

void activateButtons() {
	if (logState == NEWREC) {
		bNewSave->label ("Save");
		bUpdateCancel->label ("Cancel");
		bDelete->deactivate ();
		bSearchNext->deactivate ();
		bSearchPrev->deactivate ();
		inpDate_log->take_focus();
		return;
	}
	bNewSave->label("New");
	bUpdateCancel->label("Update");
	bDelete->activate();
	bSearchNext->activate ();
	bSearchPrev->activate ();
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
	} else
		updateRecord();
}

void cb_btnDelete(Fl_Button* b, void* d) {
	deleteRecord();
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

void DupCheck(const char *callsign)
{
	zuluDateTime();
	if (qsodb.duplicate(
			callsign,
			zuluLogDate, zuluLogTime, progdefaults.timespan, progdefaults.duptimespan, 
			inpFreq->value(), progdefaults.dupband,
			inpState->value(), progdefaults.dupstate,
			mode_info[active_modem->get_mode()].adif_name, progdefaults.dupmode,
			inpXchg1->value(), progdefaults.dupxchg1,
			inpXchg2->value(), progdefaults.dupxchg2,
			inpXchg3->value(), progdefaults.dupxchg3 ) )
		lblDup->show();
}


void SearchLastQSO(const char *callsign)
{
	size_t len = strlen(callsign);
	char* re = new char[len + 3];
	snprintf(re, len + 3, "^%s$", callsign);

	int row = 0, col = 2;
	if (wBrowser->search(row, col, !cQsoDb::reverse, re)) {
		wBrowser->GotoRow(row);
		inpName->value(inpName_log->value());
		inpSearchString->value(callsign);
		if (EnableDupCheck)
			DupCheck(callsign);
	} else
		inpSearchString->value("");

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

	inpSearchString->take_focus();
	return 1;
}

int editNbr = 0;

void clearRecord() {
	Date tdy;
	inpCall_log->value ("");
	inpName_log->value ("");
	inpDate_log->value (tdy.szDate(2));
	inpTime_log->value ("0000");
	inpRstR_log->value ("599");
	inpRstS_log->value ("599");
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
	inpXchg1_log->value("");
	inpXchg2_log->value("");
	inpXchg3_log->value("");
	inpComment_log->value ("");
	editGroup->show();
}

void saveRecord() {
cQsoRec rec;
	rec.putField(CALL, inpCall_log->value());
	rec.putField(NAME, inpName_log->value());
	rec.putField(QSO_DATE, inpDate_log->value());
	rec.putField(TIME_ON, inpTime_log->value());
	rec.putField(FREQ, inpFreq_log->value());
	rec.putField(MODE, inpMode_log->value());
	rec.putField(QTH, inpQth_log->value());
	rec.putField(STATE, inpState_log->value());
	rec.putField(VE_PROV, inpVE_Prov_log->value());
	rec.putField(COUNTRY, inpCountry_log->value());
	rec.putField(GRIDSQUARE, inpLoc_log->value());
	rec.putField(COMMENT, inpComment_log->value());
	rec.putField(QSLRDATE, inpQSLrcvddate_log->value());
	rec.putField(QSLSDATE, inpQSLsentdate_log->value());
	rec.putField(RST_RCVD, inpRstR_log->value ());
	rec.putField(RST_SENT, inpRstS_log->value ());
	rec.putField(SRX, inpSerNoIn_log->value());
	rec.putField(STX, inpSerNoOut_log->value());
	rec.putField(XCHG1, inpXchg1_log->value());
	rec.putField(XCHG2, inpXchg2_log->value());
	rec.putField(XCHG3, inpXchg3_log->value());	
	
	qsodb.qsoNewRec (&rec);
}

void updateRecord() {
cQsoRec rec;
	if (qsodb.nbrRecs() == 0) return;
	rec.putField(CALL, inpCall_log->value());
	rec.putField(NAME, inpName_log->value());
	rec.putField(QSO_DATE, inpDate_log->value());
	rec.putField(TIME_ON, inpTime_log->value());
	rec.putField(FREQ, inpFreq_log->value());
	rec.putField(MODE, inpMode_log->value());
	rec.putField(QTH, inpQth_log->value());
	rec.putField(STATE, inpState_log->value());
	rec.putField(VE_PROV, inpVE_Prov_log->value());
	rec.putField(COUNTRY, inpCountry_log->value());
	rec.putField(GRIDSQUARE, inpLoc_log->value());
	rec.putField(COMMENT, inpComment_log->value());
	rec.putField(QSLRDATE, inpQSLrcvddate_log->value());
	rec.putField(QSLSDATE, inpQSLsentdate_log->value());
	rec.putField(RST_RCVD, inpRstR_log->value ());
	rec.putField(RST_SENT, inpRstS_log->value ());
	rec.putField(SRX, inpSerNoIn_log->value());
	rec.putField(STX, inpSerNoOut_log->value());
	rec.putField(XCHG1, inpXchg1_log->value());
	rec.putField(XCHG2, inpXchg2_log->value());
	rec.putField(XCHG3, inpXchg3_log->value());	
	qsodb.qsoUpdRec (editNbr, &rec);
	qsodb.isdirty(1);
}

void deleteRecord () {
	if (qsodb.nbrRecs() == 0 || fl_choice("Really delete record for \"%s\"?\n",
					      "Yes", "No", NULL, wBrowser->valueAt(-1, 2)))
		return;

	qsodb.qsoDelRec(editNbr);
	loadBrowser();
	qsodb.isdirty(1);
}

void EditRecord( int i )
{
	cQsoRec *editQSO = qsodb.getRec (i);
	if( !editQSO ) 
		return;

	inpCall_log->value (editQSO->getField(CALL));
	inpName_log->value (editQSO->getField(NAME));
	inpDate_log->value (editQSO->getField(QSO_DATE));
	inpTime_log->value (editQSO->getField(TIME_ON));
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
	inpComment_log->value (editQSO->getField(COMMENT));
	inpSerNoIn_log->value(editQSO->getField(SRX));
	inpSerNoOut_log->value(editQSO->getField(STX));
	inpXchg1_log->value(editQSO->getField(XCHG1));
	inpXchg2_log->value(editQSO->getField(XCHG2));
	inpXchg3_log->value(editQSO->getField(XCHG3));
	editGroup->show();
}

void AddRecord ()
{
//	zuluDateTime();
	inpCall_log->value(inpCall->value());
	inpName_log->value (inpName->value());
	inpDate_log->value (zuluLogDate);
	inpTime_log->value (zuluLogTime);
	inpRstR_log->value (inpRstIn->value());
	inpRstS_log->value (inpRstOut->value());
	inpFreq_log->value (inpFreq->value());
	inpMode_log->value (logmode);
	inpState_log->value (inpState->value());
	inpVE_Prov_log->value (inpVEprov->value());
	inpCountry_log->value (inpCountry->value());

	inpSerNoIn_log->value(inpSerNo->value());
	inpSerNoOut_log->value(outSerNo->value());
	inpXchg1_log->value(inpXchg1->value());
	inpXchg2_log->value(inpXchg2->value());
	inpXchg3_log->value(inpXchg3->value());

	inpQth_log->value (inpQth->value());
	inpLoc_log->value (inpLoc->value());
	inpQSLrcvddate_log->value ("");
	inpQSLsentdate_log->value ("");
	inpComment_log->value (inpNotes->value());
	
	saveRecord();
	qsodb.SortByDate();
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

void loadBrowser()
{
	cQsoRec *rec;
	char sNbr[6];
	wBrowser->clear();
	if (qsodb.nbrRecs() == 0)
		return;
	for( int i = 0; i < qsodb.nbrRecs(); i++ ) {
		rec = qsodb.getRec (i);
		snprintf(sNbr,sizeof(sNbr),"%d",i);
		wBrowser->addRow (7,
			rec->getField(QSO_DATE),
			rec->getField(TIME_ON),
			rec->getField(CALL),
			rec->getField(NAME),
			rec->getField(FREQ),
			rec->getField(MODE),
			sNbr);
	}
	if (cQsoDb::reverse == true)
		wBrowser->FirstRow ();
	else
		wBrowser->LastRow ();
	char szRecs[6];
	snprintf(szRecs, sizeof(szRecs), "%5d", qsodb.nbrRecs());
	txtNbrRecs_log->value(szRecs);
}


