#include <FL/Fl.H>
#include <FL/filename.H>
#include <FL/fl_ask.H>

#include <string>
#include <cstring>
#include <cstdlib>
#include <string>

#include "adif_io.h"
#include "config.h"
#include "lgbook.h"
#include "icons.h"
#include "gettext.h"

using namespace std;

#ifdef __WOE32__
static const char *szEOL = "\r\n";
#else
static const char *szEOL = "\n";
#endif
static const char *szEOR = "<EOR>";

// These ADIF fields are a part of the QSO database

FIELD fields[] = {
//  TYPE,  NAME,	SIZE
	{ADDRESS,    "ADDRESS",    0, 40, NULL},                // contacted stations mailing address
	{AGE,        "AGE",        0,  3, NULL},                // contacted operators age in years
	{ARRL_SECT,  "ARRL_SECT",  0, 12, NULL},                // contacted stations ARRL section
	{BAND,       "BAND",       0,  6, &btnSelectBand},      // QSO band
	{CALL,       "CALL",       0, 32, &btnSelectCall},      // contacted stations CALLSIGN
	{CNTY,       "CNTY",       0, 20, NULL},                // secondary political subdivision, ie: county
	{COMMENT,    "COMMENT",    0, 80, NULL},                // comment field for QSO
	{CONT,       "CONT",       0, 10, &btnSelectCONT},      // contacted stations continent
	{CONTEST_ID, "CONTEST_ID", 0,  6, NULL},                // QSO contest identifier
	{COUNTRY,    "COUNTRY",    0, 20, &btnSelectCountry},   // contacted stations DXCC entity name
	{CQZ,        "CQZ",        0,  8, &btnSelectCQZ},       // contacted stations CQ Zone
	{DXCC,       "DXCC",       0,  8, &btnSelectDXCC},      // contacted stations Country Code
	{EXPORT,     "EXPORT",     0,  1, NULL},                // used to indicate record is to be exported
	{FREQ,       "FREQ",       0, 10, &btnSelectFreq},      // QSO frequency in Mhz
	{GRIDSQUARE, "GRIDSQUARE", 0,  6, &btnSelectLOC},       // contacted stations Maidenhead Grid Square
	{IOTA,       "IOTA",       0,  6, &btnSelectIOTA},      // Islands on the air
	{ITUZ,       "ITUZ",       0,  6, &btnSelectITUZ},      // ITU zone
	{MODE,       "MODE",       0,  8, &btnSelectMode},      // QSO mode
	{MYXCHG,     "STX_STRING", 0, 40, &btnSelectMyXchg},    // contest exchange sent
	{NAME,       "NAME",       0, 18, &btnSelectName},      // contacted operators NAME
	{NOTES,      "NOTES",      0, 80, &btnSelectNotes},     // QSO notes
	{OPERATOR,   "OPERATOR",   0, 10, NULL},                // Callsign of person logging the QSO
	{PFX,        "PFX",        0,  5, NULL},                // WPA prefix
	{PROP_MODE,  "PROP_MODE",  0,  5, NULL},                // propogation mode
	{QSLRDATE,   "QSLRDATE",   0,  8, &btnSelectQSLrcvd},   // QSL received date
	{QSLSDATE,   "QSLSDATE",   0,  8, &btnSelectQSLsent},   // QSL sent date
	{QSL_MSG,    "QSL_MSG",    0, 80, NULL},                // personal message to appear on qsl card
	{QSL_RCVD,   "QSL_RCVD",   0,  1, NULL},                // QSL received status
	{QSL_SENT,   "QSL_SENT",   0,  1, NULL},                // QSL sent status
	{QSL_VIA,    "QSL_VIA",    0, 30, NULL},
	{QSO_DATE,   "QSO_DATE",   0,  8, &btnSelectQSOdate},   // QSO data
	{QTH,        "QTH",        0, 30, &btnSelectQth},       // contacted stations city
	{RST_RCVD,   "RST_RCVD",   0,  3, &btnSelectRSTrcvd},   // received signal report
	{RST_SENT,   "RST_SENT",   0,  3, &btnSelectRSTsent},   // sent signal report
	{RX_PWR,     "RX_PWR",     0,  4, NULL},                // power of other station in watts
	{SAT_MODE,   "SAT_MODE",   0,  8, NULL},                // satellite mode
	{SAT_NAME,   "SAT_NAME",   0, 12, NULL},                // satellite name
	{SRX,        "SRX",        0,  5, &btnSelectSerialIN},  // received serial number for a contest QSO
	{STATE,      "STATE",      0,  2, &btnSelectState},     // contacted stations STATE
	{STX,        "STX",        0,  8, &btnSelectSerialOUT}, // QSO transmitted serial number
	{TEN_TEN,    "TEN_TEN",    0, 10, NULL},                // ten ten # of other station
	{TIME_OFF,   "TIME_OFF",   0,  4, &btnSelectTimeOFF},   // HHMM or HHMMSS in UTC
	{TIME_ON,    "TIME_ON",    0,  4, &btnSelectTimeON},    // HHMM or HHMMSS in UTC
	{TX_PWR,     "TX_PWR",     0,  4, &btnSelectTX_pwr},    // power transmitted by this station
	{VE_PROV,    "VE_PROV",    0,  2, &btnSelectProvince},  // 2 letter abbreviation for Canadian Province
	{XCHG1,      "SRX_STRING", 0, 40, &btnSelectXchgIn}     // contest exchange #1 / free1 in xlog
};

int numfields = sizeof(fields) / sizeof(FIELD);

void initfields()
{
	for (int i = 0; i < numfields; i++)
		fields[i].len = strlen(fields[i].name);
}

int fieldnbr (const char *s) {
	for (int i = 0;  i < numfields; i++)
		if (strncasecmp( fields[i].name, s, fields[i].size) == 0) {
			if (fields[i].type == COMMENT) return(NOTES);
			return fields[i].type;
		}
	return -1;
}

int findfield( char *p )
{
	int m;
	int test;

	if (strncasecmp (p, "EOR>", 4) == 0)
		return -1;

// following two tests are for backward compatibility with older
// fldigi fields
// changed to comply with ADIF 2.2.3

	if (strncasecmp (p, "XCHG1>", 6) == 0)
		return XCHG1;
	if (strncasecmp (p, "MYXCHG>", 7) == 0)
		return MYXCHG;

	string tststr;
	for (m = 0; m < numfields; m++) {
		tststr = fields[m].name;
		tststr += ':';
		if ( (test = strncasecmp( p, tststr.c_str(), tststr.length() )) == 0)
			return fields[m].type;
	}
	return -2;		//search key not found
}

cAdifIO::cAdifIO ()
{
	initfields();
}

void cAdifIO::fillfield (int fieldnum, char *buff){
const char *p = buff;
int n, fldsize;
	n = 0;
	while (*p != ':' && n < 11) {p++; n++;}
	if (n == 11) return; // bad ADIF specifier ---> no ':' after field name
// found first ':'
	p++;
	n = 0;
	fldsize = 0;
	const char *p2 = strchr(buff,'>');
	if (!p2) return;
	while (p != p2) {
		if (*p >= '0' && *p <= '9' && n < 8) {
			fldsize = fldsize * 10 + *p - '0';
			n++;
		}
		p++;
	}
	adifqso.putField (fieldnum, p2+1, fldsize);
}

void cAdifIO::readFile (const char *fname, cQsoDb *db) {
	long filesize = 0;
	char *buff;
	int found;
	int retval;

// open the adif file
	adiFile = fopen (fname, "r");
	if (!adiFile)
		return;
// determine its size for buffer creation
	fseek (adiFile, 0, SEEK_END);
	filesize = ftell (adiFile);
	buff = new char[filesize + 1];
// read the entire file into the buffer
	fseek (adiFile, 0, SEEK_SET);
	retval = fread (buff, filesize, 1, adiFile);
	fclose (adiFile);

// relaxed file integrity test to all importing from non conforming log programs
	if (filesize == 0) {
	fl_alert2(_("Empty ADIF logbook file"));
	return;
	}
	if ((strcasestr(buff, "<ADIF_VER:") != 0) &&
	(strcasestr(buff, "<CALL:") == 0)) {
	fl_alert2(_("No records in ADIF logbook file"));
		return;
	}
	if (strcasestr(buff, "<CALL:") == 0) {
	fl_alert2(_("Not an ADIF file"));
		return;
	}

	char *p1 = buff, *p2;
// is there a header?
	if (*p1 != '<') { // yes find the start of the records
		p1 = strchr(buff, '<');
		while (strncasecmp (p1+1,"EOH>", 4) != 0) {
			p1 = strchr(p1+1, '<'); // find next <> field
		}
		if (!p1) return;	 // must not be an ADIF compliant file
		p1 += 1;
	}

	p2 = strchr(p1,'<'); // find first ADIF specifier
	adifqso.clearRec();

	while (p2) {
		found = findfield(p2+1); // -2 ==> not found; -1 <eor> 0 ...N field #
//		if (found == -2 ) return; // unknown field
		if (found > -1)
			fillfield (found, p2+1);
		else if (found == -1) { // <eor> reached; add this record to db
//update fields for older db
			if (adifqso.getField(TIME_OFF)[0] == 0)
				adifqso.putField(TIME_OFF, adifqso.getField(TIME_ON));
			if (adifqso.getField(TIME_ON)[0] == 0)
				adifqso.putField(TIME_ON, adifqso.getField(TIME_OFF));
			db->qsoNewRec (&adifqso);
			adifqso.clearRec();
		}
		p1 = p2 + 1;
		p2 = strchr(p1,'<');
	}

	db->SortByDate();
	delete [] buff;
}

string ADIFHEADER = "";

static void make_adif_header()
{
	ADIFHEADER = "File: %s";
	ADIFHEADER.append(szEOL);
	ADIFHEADER.append("<ADIF_VER:%d>%s");
	ADIFHEADER.append(szEOL);
	ADIFHEADER.append("<PROGRAMID:%d>%s");
	ADIFHEADER.append(szEOL);
	ADIFHEADER.append("<PROGRAMVERSION:%d>%s");
	ADIFHEADER.append(szEOL);
	ADIFHEADER.append("<EOH>");
	ADIFHEADER.append(szEOL);
}

static const char *adifmt = "<%s:%d>%s";

// write ALL or SELECTED records to the designated file


int cAdifIO::writeFile (const char *fname, cQsoDb *db)
{
	if (ADIFHEADER.empty()) make_adif_header();
// open the adif file
	cQsoRec *rec;
	char *szFld;
	adiFile = fopen (fname, "w");
	if (!adiFile)
		return 1;
	fprintf (adiFile, ADIFHEADER.c_str(),
			 fl_filename_name(fname),
			 strlen(ADIF_VERS), ADIF_VERS,
			 strlen(PACKAGE_NAME), PACKAGE_NAME,
			 strlen(PACKAGE_VERSION), PACKAGE_VERSION);
//	db->SortByDate();
	for (int i = 0; i < db->nbrRecs(); i++) {
		rec = db->getRec(i);
		if (rec->getField(EXPORT)[0] == 'E') {
			for (int j = 0; j < numfields; j++) {
				if (fields[j].btn != NULL)
					if ((*fields[j].btn)->value()) {
					szFld = rec->getField(fields[j].type);
						if (strlen(szFld))
							fprintf(adiFile, adifmt,
						fields[j].name, strlen(szFld), szFld);
				}
			}
			rec->putField(EXPORT,"");
			db->qsoUpdRec(i, rec);
			fprintf(adiFile, "%s", szEOR);
			fprintf(adiFile, "%s", szEOL);
		}
	}
	fclose (adiFile);
	return 0;
}

// write ALL records to the common log

int cAdifIO::writeLog (const char *fname, cQsoDb *db) {
	if (ADIFHEADER.empty()) make_adif_header();
// open the adif file
	char *szFld;
	cQsoRec *rec;
	adiFile = fopen (fname, "w");
	if (!adiFile)
		return 1;
	fprintf (adiFile, ADIFHEADER.c_str(),
			 fl_filename_name(fname),
			 strlen(ADIF_VERS), ADIF_VERS,
			 strlen(PACKAGE_NAME), PACKAGE_NAME,
			 strlen(PACKAGE_VERSION), PACKAGE_VERSION);
//	db->SortByDate();
	for (int i = 0; i < db->nbrRecs(); i++) {
		rec = db->getRec(i);
		for (int j = 0; j < numfields; j++) {
			szFld = rec->getField(j);
			if (strlen(szFld))
				fprintf(adiFile, adifmt,
					fields[j].name, strlen(szFld), szFld);
		}
		db->qsoUpdRec(i, rec);
		fprintf(adiFile, "%s", szEOR);
		fprintf(adiFile, "%s", szEOL);
	}
	fclose (adiFile);
	return 0;
}
