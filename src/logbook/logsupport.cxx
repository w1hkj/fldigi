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
#include "main.h"
#include "locator.h"

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
	const char* p = FSEL::saveas("Export to CSV file", filters.c_str(),
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
	const char* p = FSEL::saveas("Export to fixed field text file", filters.c_str(),
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
	const char* p = FSEL::saveas("Export to ADIF file", filters.c_str(),
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

savetype export_to = ADIF;

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
		if (!fl_choice("Save changed Logbook?", "No", "Yes", NULL)) 
			return;
	if (adifFile.writeLog (logbook_filename.c_str(), &qsodb))
		fl_message ("Could not update file %s", logbook_filename.c_str());
	qsodb.isdirty(0);
}

void cb_mnuNewLogbook(Fl_Menu_* m, void* d){
	saveLogbook();

	logbook_filename = LogsDir;
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
	const char* p = FSEL::select("Open logbook file", "ADIF\t*." ADIF_SUFFIX,
						logbook_filename.c_str());
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
	const char* p = FSEL::saveas("Save logbook file", "ADIF\t*." ADIF_SUFFIX,
				     logbook_filename.c_str());
	if (p) {
		logbook_filename = p;
		dlgLogbook->label(fl_filename_name(logbook_filename.c_str()));
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
		snprintf(line,sizeof(line),"%8s  %4s  %-10s  %10s  %-s",
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
	if (qsodb.duplicate(
			inpCall->value(),
			zdate(), ztime(), progdefaults.timespan, progdefaults.duptimespan, 
			inpFreq->value(), progdefaults.dupband,
			inpState->value(), progdefaults.dupstate,
			mode_info[active_modem->get_mode()].adif_name, progdefaults.dupmode,
			inpXchgIn->value(), progdefaults.dupxchg1 ) ) {
		lblDup->show();
	}
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
	inpComment_log->value ("");
	inpIOTA_log->value("");
	inpDXCC_log->value("");
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
	rec.putField(COMMENT, inpComment_log->value());
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
	rec.putField(TX_PWR, inpTX_pwr_log->value());
	
	qsodb.qsoNewRec (&rec);
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
	rec.putField(COMMENT, inpComment_log->value());
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
	rec.putField(TX_PWR, inpTX_pwr_log->value());
	qsodb.qsoUpdRec (editNbr, &rec);
	qsodb.isdirty(1);
	loadBrowser(true);
}

void deleteRecord () {
	if (qsodb.nbrRecs() == 0 || fl_choice("Really delete record for \"%s\"?\n",
					      "Yes", "No", NULL, wBrowser->valueAt(-1, 2)))
		return;

	qsodb.qsoDelRec(editNbr);
	loadBrowser(true);
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
	inpComment_log->value (editQSO->getField(COMMENT));
	inpSerNoIn_log->value(editQSO->getField(SRX));
	inpSerNoOut_log->value(editQSO->getField(STX));
	inpXchgIn_log->value(editQSO->getField(XCHG1));
	inpMyXchg_log->value(editQSO->getField(MYXCHG));
	inpIOTA_log->value(editQSO->getField(IOTA));
	inpDXCC_log->value(editQSO->getField(DXCC));
	inpTX_pwr_log->value(editQSO->getField(TX_PWR));
	editGroup->show();
}

void AddRecord ()
{
	inpCall_log->value(inpCall->value());
	inpName_log->value (inpName->value());
	inpDate_log->value (zdate());
	inpTimeOn_log->value (inpTimeOn->value());
	inpTimeOff_log->value (ztime());
	inpRstR_log->value (inpRstIn->value());
	inpRstS_log->value (inpRstOut->value());
	{
		char Mhz[30];
		snprintf(Mhz, sizeof(Mhz), "%10f", atof(inpFreq->value()) / 1000.0);
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
	inpComment_log->value (inpNotes->value());
	
	inpTX_pwr_log->value (progdefaults.mytxpower.c_str());
	inpIOTA_log->value("");
	inpDXCC_log->value("");
	
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
	"BARTG-RTTY", 
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
	BARTG_RTTY, 
	CQ_160_CW, CQ_160_SSB, CQ_WPX_CW, CQ_WPX_RTTY, CQ_WPX_SSB, CQ_VHF, 
	CQ_WW_CW, CQ_WW_RTTY, CQ_WW_SSB, 
	DARC_WAEDC_CW, DARC_WAEDC_RTTY, DARC_WAEDC_SSB, 
	FCG_FQP, IARU_HF, JIDX_CW, JIDX_SSB, 
	NAQP_CW, NAQP_RTTY, NAQP_SSB, NA_SPRINT_CW, NA_SPRINT_SSB, NCCC_CQP,
	NEQP, OCEANIA_DX_CW, OCEANIA_DX_SSB, RDXC, RSGB_IOTA, 
	SAC_CW, SAC_SSB, STEW_PERRY, TARA_RTTY
};

bool bInitCombo = true;

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
		snprintf(line,sizeof(line),"%8s  %4s  %-10s  %10s  %-s",
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
	string rst_in, rst_out, exch_in, exch_out, date, time, mode, mycall, call;
	char freq[16] = "";
		   
	int rst_len = 3;
	int ifreq = 0;
	
	mycall = progdefaults.myCall;
	if (mycall.length() > 13) mycall = mycall.substr(0,13);

	if (btnCabCall->value()) {
		call = rec->getField(CALL);
		if (call.length() > 13) call = call.substr(0,13);
	}
			
	if (btnCabMode->value()) {
		mode = rec->getField(MODE);
		if (mode.compare("USB") == 0 || mode.compare("LSB") == 0 || 
		    mode.compare("PH") == 0 ) mode = "PH";
		else if (mode.compare("FM") == 0 || mode.compare("CW") == 0 ) ;
		else mode = "RY";
		if (mode.compare("PH") == 0 || mode.compare("FM") == 0 ) rst_len = 2;
	}
	
	if (btnCabRSTrcvd->value()) {
		rst_in = rec->getField(RST_RCVD);
		rst_in = rst_in.substr(0,rst_len);
		rst_out = rec->getField(RST_SENT);
		rst_out = rst_out.substr(0,rst_len);
	}

	if (btnCabSerialIN->value()) {
		exch_in = rec->getField(SRX);
		if (exch_in.length())
			exch_in += ' ';
	}
	if (btnCabXchgIn->value())
		exch_in.append(rec->getField(XCHG1));
	if (exch_in.length() > 10) exch_in = exch_in.substr(0,10);

	if (btnCabSerialOUT->value()) {
		exch_out = rec->getField(STX);
		if (exch_out.length())
			exch_out += ' ';
	}
	if (btnCabMyXchg->value())
		exch_out.append(rec->getField(MYXCHG));
	if (exch_out.length() > 10) exch_out = exch_out.substr(0,10);
	
	if (btnCabFreq->value()) {
		ifreq = (int)(1000.0 * atof(rec->getField(FREQ)));
		snprintf(freq, sizeof(freq), "%d", ifreq);
	}
	
	if (btnCabQSOdate->value()) {
		date = rec->getField(QSO_DATE);
		date.insert(4,"-");
		date.insert(7,"-");
	}
	
	if (btnCabTimeOFF->value())
		time = rec->getField(TIME_OFF);

	fprintf (fp, "QSO: %-5s %-2s %-10s %-4s %-13s %-3s %-10s %-13s %-3s %-10s\n",
		freq, mode.c_str(), date.c_str(), time.c_str(), 
		mycall.c_str(), rst_out.c_str(), exch_out.c_str(), 
		call.c_str(), rst_in.c_str(), exch_in.c_str() );
	return;
}

void WriteCabrillo()
{
	if (chkCabBrowser->nchecked() == 0) return;

	cQsoRec *rec;

	string filters = "TEXT\t*.txt";
	string strContest = "";
	
	const char* p = FSEL::saveas("Create cabrillo report", filters.c_str(),
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
