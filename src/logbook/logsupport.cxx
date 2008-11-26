#include <config.h>

#include <FL/fl_file_chooser.H>

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

// remove after testing
extern string HomeDir;

using namespace std;

cQsoDb		qsodb;
cAdifIO		adifFile;
cTextFile	txtFile;

string		logbook_filename;

enum savetype {TEXT, LO, ADIF};
savetype saveas = ADIF;

void Export_log()
{
    if (chkExportBrowser->nchecked() == 0) return;
    cQsoRec *rec;
	char *p = 0;
    switch (saveas) {
        case TEXT :
            p = fl_file_chooser("Export to text file", "*.txt", "export.txt");
            break;
        case ADIF :
        	p = fl_file_chooser("Export to ADI file", "*.adi", "export.adi");
        	break;
        default:
            break;
    }
	if (p) {
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
}

void saveLogbook()
{
	if (qsodb.isdirty()) {
		if (fl_choice("Logbook Database changed, Save?", "No", "Yes", NULL))
			if (adifFile.writeLog (logbook_filename.c_str(), &qsodb))
				fl_message ("Could not update file %s", logbook_filename.c_str());
		qsodb.isdirty(0);
	}
}

void cb_mnuNewLogbook(Fl_Menu_* m, void* d){
	saveLogbook();

	logbook_filename = HomeDir;
	logbook_filename.append("newlog.adi");
	qsodb.deleteRecs();
	wBrowser->clear();
}

void cb_mnuOpenLogbook(Fl_Menu_* m, void* d)
{
	char *p = fl_file_chooser("Open logbook file", "*.adi", "");
	if (p) {
		saveLogbook();
		qsodb.deleteRecs();
		
		logbook_filename = p;
		adifFile.readFile (logbook_filename.c_str(), &qsodb);
		loadBrowser();
		qsodb.isdirty(0);
	}
}

void cb_mnuSaveLogbook(Fl_Menu_*m, void* d) {
	if (qsodb.nbrRecs() == 0) return;
	char *p = fl_file_chooser("SaveAs to logbook file", "*.adi", logbook_filename.c_str());
	if (p) {
		if (adifFile.writeLog (p, &qsodb))
			fl_message ("Could not write to %s", p);
		qsodb.isdirty(0);
	}
}

void cb_mnuMergeADIF_log(Fl_Menu_* m, void* d) {
	char *p = fl_file_chooser("Select ADI merge file", "*.adi", "");
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
		bSearchFirst->deactivate ();
		bSearchNext->deactivate ();
		inpDate_log->take_focus();
		return;
	}
	bNewSave->label("New");
	bUpdateCancel->label("Update");
	bDelete->activate();
	bSearchFirst->activate ();
	bSearchNext->activate ();
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

static int lastfind = -1;

void cb_btnSearchFirst (Fl_Button* b, void* d) {
	char *srchstr = new char[strlen(inpSearchString->value())+ 1];
	int i;
	strcpy(srchstr, inpSearchString->value());
	char *p = srchstr + strlen(srchstr) - 1;
	while (p > srchstr && *p == ' ') *p-- = 0;
	while (srchstr[0] == ' ') strcpy(srchstr, &srchstr[1]);
	p = srchstr;
	while (*p) {*p = toupper(*p); p++;}
	lastfind = -1;
	size_t len = strlen(srchstr);
	for (i = 0; i < wBrowser->rows(); i++) {
		if (strncmp(srchstr, wBrowser->valueAt(i, 2), len ) == 0) {
			lastfind = i;
			wBrowser->GotoRow(i);
			return;
		}
	}
}

void cb_btnSearchNext (Fl_Button *b, void *d) {
	if (lastfind == -1)
		return;
	char *srchstr = new char[strlen(inpSearchString->value())+ 1];
	int i;
	strcpy(srchstr, inpSearchString->value());
	char *p = srchstr + strlen(srchstr) - 1;
	while (p > srchstr && *p == ' ') *p-- = 0;
	while (srchstr[0] == ' ') strcpy(srchstr, &srchstr[1]);
	p = srchstr;
	size_t len = strlen(srchstr);
	while (*p) {*p = toupper(*p); p++;}
	for (i = lastfind + 1; i < wBrowser->rows(); i++) {
		if (strncmp(srchstr, wBrowser->valueAt(i, 2), len) == 0) {
			lastfind = i;
			wBrowser->GotoRow(i);
			return;
		}
	}
}

void SearchLastQSO (const char *callsign) {
	char *srchstr = new char[strlen(callsign) + 1];
	int i;
	strcpy(srchstr, callsign);
	char *p = srchstr + strlen(srchstr) - 1;
	while (p > srchstr && *p == ' ') *p-- = 0;
	while (srchstr[0] == ' ') strcpy(srchstr, &srchstr[1]);
	p = srchstr;
	size_t len = strlen(srchstr);
	lastfind = -1;
	while (*p) {*p = toupper(*p); p++;}
	if (cQsoDb::reverse)
		for (i = 0; i < wBrowser->rows(); i++) {
			if (strncmp(srchstr, wBrowser->valueAt(i, 2), len ) == 0) {
				lastfind = i;
				break;
			}
		}
	else
		for (i = wBrowser->rows() - 1; i >= 0; i--) {
			if (strncmp(srchstr, wBrowser->valueAt(i, 2), len) == 0) {
				lastfind = i;
				break;
			}
		}
	if (lastfind != -1) {
    	wBrowser->GotoRow(lastfind);
    	inpName->value(inpName_log->value());
	}
	return;
}

void cb_btnSearchLast (Fl_Button *b, void *d) {
	char *srchstr = new char[strlen(inpSearchString->value())+ 1];
	int i;
	strcpy(srchstr, inpSearchString->value());
	char *p = srchstr + strlen(srchstr) - 1;
	while (p > srchstr && *p == ' ') *p-- = 0;
	while (srchstr[0] == ' ') strcpy(srchstr, &srchstr[1]);
	p = srchstr;
	size_t len = strlen(srchstr);
	lastfind = -1;
	while (*p) {*p = toupper(*p); p++;}
	for (i = wBrowser->rows() - 1; i >= 0; i--)
		if (strncmp(srchstr, wBrowser->valueAt(i, 2), len) == 0) {
			lastfind = i;
			break;
		}
	if (lastfind != -1)
    	wBrowser->GotoRow(lastfind);
	return;
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
	inpCnty_log->value ("");
	inpSerNoOut_log->value ("");
	inpSerNoIn_log->value ("");
	inpVE_Prov_log->value ("");
	inpLoc_log->value ("");
	inpQSLrcvddate_log->value ("");
	inpQSLsentdate_log->value ("");
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
	rec.putField(CNTY, inpCnty_log->value());
	rec.putField(SRX, inpSerNoIn_log->value());
	rec.putField(STX, inpSerNoOut_log->value());
	rec.putField(VE_PROV, inpVE_Prov_log->value());
	rec.putField(GRIDSQUARE, inpLoc_log->value());
	rec.putField(COMMENT, inpComment_log->value());
	rec.putField(QSLRDATE, inpQSLrcvddate_log->value());
	rec.putField(QSLSDATE, inpQSLsentdate_log->value());
	rec.putField(RST_RCVD, inpRstR_log->value ());
	rec.putField(RST_SENT, inpRstS_log->value ());
	
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
	rec.putField(CNTY, inpCnty_log->value());
	rec.putField(SRX, inpSerNoIn_log->value());
	rec.putField(STX, inpSerNoOut_log->value());
	rec.putField(VE_PROV, inpVE_Prov_log->value());
	rec.putField(GRIDSQUARE, inpLoc_log->value());
	rec.putField(COMMENT, inpComment_log->value());
	rec.putField(QSLRDATE, inpQSLrcvddate_log->value());
	rec.putField(QSLSDATE, inpQSLsentdate_log->value());
	rec.putField(RST_RCVD, inpRstR_log->value ());
	rec.putField(RST_SENT, inpRstS_log->value ());
	qsodb.qsoUpdRec (editNbr, &rec);
	qsodb.isdirty(1);
//	qsodb.qsoWriteFile(logbook_filename.c_str());
//	qsodb.isdirty(0);
}

void deleteRecord () {
	if (qsodb.nbrRecs() == 0) return;
	if (fl_choice("Confirm Delete", "No", "Yes", NULL)) {
		qsodb.qsoDelRec(editNbr);
		loadBrowser();
		qsodb.isdirty(1);
//		qsodb.qsoWriteFile(logbook_filename.c_str());
//		qsodb.isdirty(0);
	}
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
	inpCnty_log->value (editQSO->getField(CNTY));
	inpVE_Prov_log->value (editQSO->getField(VE_PROV));
	inpSerNoIn_log->value(editQSO->getField(SRX));
	inpSerNoOut_log->value(editQSO->getField(STX));
	inpQth_log->value (editQSO->getField(QTH));
	inpLoc_log->value (editQSO->getField(GRIDSQUARE));
	inpQSLrcvddate_log->value (editQSO->getField(QSLRDATE));
	inpQSLsentdate_log->value (editQSO->getField(QSLSDATE));
	inpComment_log->value (editQSO->getField(COMMENT));
	editGroup->show();
}

void AddRecord ()
{
	inpCall_log->value(inpCall->value());
	inpName_log->value (inpName->value());
	inpDate_log->value (logdate);
	inpTime_log->value (logtime);
	inpRstR_log->value (inpRstIn->value());
	inpRstS_log->value (inpRstOut->value());
	inpFreq_log->value (inpFreq->value());
	inpMode_log->value (logmode);
	inpCnty_log->value (inpCnty->value());
	inpVE_Prov_log->value (inpVEprov->value());
	inpSerNoIn_log->value(inpSerNo->value());
	char szcnt[5] = "";
	if (contest_count.count)
		snprintf(szcnt, sizeof(szcnt), "%04d", contest_count.count);
	inpSerNoOut_log->value(szcnt);
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


