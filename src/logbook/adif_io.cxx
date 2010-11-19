#include <FL/Fl.H>
#include <FL/filename.H>
#include <FL/fl_ask.H>

#include <string>
#include <cstring>
#include <cstdlib>
#include <string>

#include "adif_io.h"
#include "config.h"
#include "configuration.h"
#include "lgbook.h"
#include "icons.h"
#include "gettext.h"
#include "debug.h"
#include "util.h"

#ifdef __WOE32__
static const char *szEOL = "\r\n";
#else
static const char *szEOL = "\n";
#endif
static const char *szEOR = "<EOR>";

// These ADIF fields define the QSO database

const char *fieldnames[] = {
	"ADDRESS", "AGE", "ARRL_SECT", "BAND", "CALL", "CNTY", "COMMENT",
	"CONT", "CONTEST_ID", "COUNTRY", "CQZ", "DXCC", "EXPORT", "FREQ",
	"GRIDSQUARE", "IOTA", "ITUZ", "MODE", "STX_STRING",
	"NAME", "NOTES", "OPERATOR", "PFX", "PROP_MODE",
	"QSLRDATE", "QSLSDATE", "QSL_MSG", "QSL_RCVD", "QSL_SENT", "QSL_VIA", "QSO_DATE", "QSO_DATE_OFF",
	"QTH",
	"RST_RCVD", "RST_SENT", "RX_PWR",
	"SAT_MODE", "SAT_NAME", "SRX",
	"STATE", "STX", "TEN_TEN",
	"TIME_OFF", "TIME_ON", "TX_PWR", "VE_PROV", "SRX_STRING"
};

FIELD fields[] = {
//  TYPE, NAME, WIDGET
	{ADDRESS,        0,  NULL},                // contacted stations mailing address
	{AGE,            0,  NULL},                // contacted operators age in years
	{ARRL_SECT,      0,  NULL},                // contacted stations ARRL section
	{BAND,           0,  &btnSelectBand},      // QSO band
	{CALL,           0,  &btnSelectCall},      // contacted stations CALLSIGN
	{CNTY,           0,  NULL},                // secondary political subdivision, ie: county
	{COMMENT,        0,  NULL},                // comment field for QSO
	{CONT,           0,  &btnSelectCONT},      // contacted stations continent
	{CONTEST_ID,     0,  NULL},                // QSO contest identifier
	{COUNTRY,        0,  &btnSelectCountry},   // contacted stations DXCC entity name
	{CQZ,            0,  &btnSelectCQZ},       // contacted stations CQ Zone
	{DXCC,           0,  &btnSelectDXCC},      // contacted stations Country Code
	{EXPORT,         0,  NULL},                // used to indicate record is to be exported
	{FREQ,           0,  &btnSelectFreq},      // QSO frequency in Mhz
	{GRIDSQUARE,     0,  &btnSelectLOC},       // contacted stations Maidenhead Grid Square
	{IOTA,           0,  &btnSelectIOTA},      // Islands on the air
	{ITUZ,           0,  &btnSelectITUZ},      // ITU zone
	{MODE,           0,  &btnSelectMode},      // QSO mode
	{MYXCHG,         0,  &btnSelectMyXchg},    // contest exchange sent
	{NAME,           0,  &btnSelectName},      // contacted operators NAME
	{NOTES,          0,  &btnSelectNotes},     // QSO notes
	{OPERATOR,       0,  NULL},                // Callsign of person logging the QSO
	{PFX,            0,  NULL},                // WPA prefix
	{PROP_MODE,      0,  NULL},                // propogation mode
	{QSLRDATE,       0,  &btnSelectQSLrcvd},   // QSL received date
	{QSLSDATE,       0,  &btnSelectQSLsent},   // QSL sent date
	{QSL_MSG,        0,  NULL},                // personal message to appear on qsl card
	{QSL_RCVD,       0,  NULL},                // QSL received status
	{QSL_SENT,       0,  NULL},                // QSL sent status
	{QSL_VIA,        0,  NULL},
	{QSO_DATE,       0,  &btnSelectQSOdateOn}, // QSO data
	{QSO_DATE_OFF,   0,  &btnSelectQSOdateOff},// QSO data OFF, according to ADIF 2.2.6
	{QTH,            0,  &btnSelectQth},       // contacted stations city
	{RST_RCVD,       0,  &btnSelectRSTrcvd},   // received signal report
	{RST_SENT,       0,  &btnSelectRSTsent},   // sent signal report
	{RX_PWR,         0,  NULL},                // power of other station in watts
	{SAT_MODE,       0,  NULL},                // satellite mode
	{SAT_NAME,       0,  NULL},                // satellite name
	{SRX,            0,  &btnSelectSerialIN},  // received serial number for a contest QSO
	{STATE,          0,  &btnSelectState},     // contacted stations STATE
	{STX,            0,  &btnSelectSerialOUT}, // QSO transmitted serial number
	{TEN_TEN,        0,  NULL},                // ten ten # of other station
	{TIME_OFF,       0,  &btnSelectTimeOFF},   // HHMM or HHMMSS in UTC
	{TIME_ON,        0,  &btnSelectTimeON},    // HHMM or HHMMSS in UTC
	{TX_PWR,         0,  &btnSelectTX_pwr},    // power transmitted by this station
	{VE_PROV,        0,  &btnSelectProvince},  // 2 letter abbreviation for Canadian Province
	{XCHG1,          0,  &btnSelectXchgIn}     // contest exchange #1 / free1 in xlog
};

void initfields()
{
	for (int i = 0; i < NUMFIELDS; i++)
		fields[i].name = new string(fieldnames[i]);
}

/*
int fieldnbr (const char *s) {
	for (int i = 0;  i < NUMFIELDS; i++)
		if (fields[i].name == s) {
//		if (strncasecmp( fields[i].name, s, fields[i].size) == 0) {
			if (fields[i].type == COMMENT) return(NOTES);
			return fields[i].type;
		}
	return -1;
}
*/

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
	for (m = 0; m < NUMFIELDS; m++) {
		tststr = *(fields[m].name);
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
	while (*p != ':' && n <= FieldLabelMaxLen) {p++; n++;}
	if (n == FieldLabelMaxLen +1) return; // bad ADIF specifier ---> no ':' after FieldLabelMaxLen +1
                                          // chars name found first ':'
	p++;
	fldsize = 0;
	const char *p2 = strchr(buff,'>');
	if (!p2) return;
	while (p != p2) {
		if (*p >= '0' && *p <= '9') {
			fldsize = fldsize * 10 + *p - '0';
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

	if (filesize == 0) {
		fl_alert2(_("Empty ADIF logbook file"));
		return;
	}

	buff = new char[filesize + 1];
// read the entire file into the buffer
	fseek (adiFile, 0, SEEK_SET);
	retval = fread (buff, filesize, 1, adiFile);
	fclose (adiFile);

// relaxed file integrity test to all importing from non conforming log programs
	if ((strcasestr(buff, "<ADIF_VER:") != 0) &&
		(strcasestr(buff, "<CALL:") == 0)) {
//		fl_alert2(_("No records in ADIF logbook file"));
		delete [] buff;
		return;
	}
	if (strcasestr(buff, "<CALL:") == 0) {
		fl_alert2(_("Not an ADIF file"));
		delete [] buff;
		return;
	}
	char *p = strcasestr(buff, "<DATA CHECKSUM:");
	if (p) {
		p = strchr(p + 1, '>');
		if (p) {
			p++;
			file_checksum.clear();
			for (int i = 0; i < 4; i++, p++) file_checksum += *p;
		}
	}

	char *p1 = buff, *p2;
	if (*p1 != '<') { // yes, skip over header to start of records
		p1 = strchr(buff, '<');
		while (strncasecmp (p1+1,"EOH>", 4) != 0) {
			p1 = strchr(p1+1, '<'); // find next <> field
		}
		if (!p1) {
			delete [] buff;
			return;	 // must not be an ADIF compliant file
		}
		p1 += 1;
	}

	p2 = strchr(p1,'<'); // find first ADIF specifier
	adifqso.clearRec();

	while (p2) {
		found = findfield(p2+1);
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

	log_checksum = file_checksum;
	db->SortByDate(progdefaults.sort_date_time_off);
	delete [] buff;
}

static const char *adifmt = "<%s:%d>";

// write ALL or SELECTED records to the designated file

int cAdifIO::writeFile (const char *fname, cQsoDb *db)
{
	string ADIFHEADER;
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
// open the adif file
	cQsoRec *rec;
	string sFld;
	adiFile = fopen (fname, "w");
	if (!adiFile)
		return 1;
	fprintf (adiFile, ADIFHEADER.c_str(),
			 fl_filename_name(fname),
			 strlen(ADIF_VERS), ADIF_VERS,
			 strlen(PACKAGE_NAME), PACKAGE_NAME,
			 strlen(PACKAGE_VERSION), PACKAGE_VERSION);

	for (int i = 0; i < db->nbrRecs(); i++) {
		rec = db->getRec(i);
		if (rec->getField(EXPORT)[0] == 'E') {
			for (int j = 0; j < NUMFIELDS; j++) {
				if (fields[j].btn != NULL)
					if ((*fields[j].btn)->value()) {
					sFld = rec->getField(fields[j].type);
						if (!sFld.empty())
							fprintf(adiFile, adifmt,
								fields[j].name->c_str(),
								sFld.length());
							fprintf(adiFile, "%s", sFld.c_str());
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

	string ADIFHEADER;
	ADIFHEADER = "File: %s";
	ADIFHEADER.append(szEOL);
	ADIFHEADER.append("<ADIF_VER:%d>%s");
	ADIFHEADER.append(szEOL);
	ADIFHEADER.append("<PROGRAMID:%d>%s");
	ADIFHEADER.append(szEOL);
	ADIFHEADER.append("<PROGRAMVERSION:%d>%s");
	ADIFHEADER.append(szEOL);
	ADIFHEADER.append("<DATA CHECKSUM:%d>%s");
	ADIFHEADER.append(szEOL);
	ADIFHEADER.append("<EOH>");
	ADIFHEADER.append(szEOL);

// open the adif file
	string sFld;
	cQsoRec *rec;
	Ccrc16 checksum;
	string s_checksum;

	adiFile = fopen (fname, "w");
	if (!adiFile) {
		LOG_ERROR("Cannot write to %s", fname);
		return 1;
	}

	string records;
	string record;
	char recfield[200];

	records.clear();
	for (int i = 0; i < db->nbrRecs(); i++) {
		rec = db->getRec(i);
		record.clear();
		for (int j = 0; j < NUMFIELDS; j++) {
			sFld = rec->getField(j);
			if (!sFld.empty()) {
				snprintf(recfield, sizeof(recfield), adifmt,
					fields[j].name->c_str(), sFld.length());
				record.append(recfield).append(sFld);
			}
		}
		record.append(szEOR);
		record.append(szEOL);
		records.append(record);
		db->qsoUpdRec(i, rec);
	}

	s_checksum = checksum.scrc16(records);

	fprintf (adiFile, ADIFHEADER.c_str(),
		 fl_filename_name(fname),
		 strlen(ADIF_VERS), ADIF_VERS,
		 strlen(PACKAGE_NAME), PACKAGE_NAME,
		 strlen(PACKAGE_VERSION), PACKAGE_VERSION,
		 s_checksum.length(), s_checksum.c_str()
		);
	fprintf (adiFile, "%s", records.c_str());

	fclose (adiFile);
	log_checksum = s_checksum;

	return 0;
}

void cAdifIO::do_checksum(cQsoDb &db)
{
	Ccrc16 checksum;
	string sFld;
	cQsoRec *rec;
	string records;
	string record;
	char recfield[200];

	records.clear();
	for (int i = 0; i < db.nbrRecs(); i++) {
		rec = db.getRec(i);
		record.clear();
		for (int j = 0; j < NUMFIELDS; j++) {
			sFld = rec->getField(j);
			if (!sFld.empty()) {
				snprintf(recfield, sizeof(recfield), adifmt,
					fields[j].name->c_str(), sFld.length());
				record.append(recfield).append(sFld);
			}
		}
		record.append(szEOR);
		record.append(szEOL);
		records.append(record);
	}
	log_checksum = checksum.scrc16(records);
}

bool cAdifIO::log_changed (const char *fname)
{
	int retval;
// open the adif file
	FILE *adiFile = fopen (fname, "r");
	if (!adiFile) {
		LOG_ERROR("Cannot open %s", fname);
		return false;
	}

// read first 2048 chars
	char buff[2048];
	retval = fread (buff, 2048, 1, adiFile);
	fclose (adiFile);

	if (retval) {
		string sbuff = buff;
		size_t p = sbuff.find("<DATA CHECKSUM:");
		if (p == string::npos) return false;
		p = sbuff.find(">", p);
		if (p == string::npos) return false;
		p++;
		if (log_checksum != sbuff.substr(p, 4))
			return true;
	}
	return  false;
}
