#include <FL/Fl.H>
#include <FL/filename.H>
#include <FL/fl_ask.H>

#include <string>
#include <iostream>

#include "adif_io.h"
#include "config.h"
#include "lgbook.h"

using namespace std;

#ifdef __CYGWIN__
static const char *szEOL = "\r\n";
#else
static const char *szEOL = "\n";
#endif

// These ADIF fields are a part of the QSO database
    
FIELD fields[] = {
//  TYPE,  NAME,    SIZE
    {ADDRESS,       "ADDRESS",      40, NULL},				// contacted stations mailing address
    {AGE,           "AGE",           3, NULL},    			// contacted operators age in years
    {ARRL_SECT,     "ARRL_SECT",    12, NULL},    			// contacted stations ARRL section
    {BAND,          "BAND",          6, &btnSelectBand},		// QSO band
    {CALL,          "CALL",         10, &btnSelectCall},		// contacted stations CALLSIGN
    {CNTY,          "CNTY",         20, NULL},		    	// secondary political subdivision, ie: county
    {COMMENT,       "COMMENT",      80, &btnSelectComment},  // comment field for QSO
    {CONT,          "CONT",         10, NULL}, 		   		// contacted stations continent
    {CONTEST_ID,    "CONTEST_ID",    6, NULL},    			// QSO contest identifier
    {COUNTRY,       "COUNTRY",      20, &btnSelectCountry},	// contacted stations DXCC entity name
    {CQZ,           "CQZ",           8, NULL},    			// contacted stations CQ Zone
    {DXCC,          "DXCC",          8, &btnSelectDXCC},    	// contacted stations Country Code
    {FREQ, 			"FREQ",			10, &btnSelectFreq},     // QSO frequency in Mhz
    {GRIDSQUARE, 	"GRIDSQUARE",	 6, &btnSelectLOC},    	// contacted stations Maidenhead Grid Square
    {MODE,			"MODE",          8, &btnSelectMode},     // QSO mode
    {NAME, 			"NAME",         18, &btnSelectName},     // contacted operators NAME
    {NOTES, 		"NOTES",        80, NULL},    			// QSO notes
    {QSLRDATE, 		"QSLRDATE",      8, &btnSelectQSLrcvd},  // QSL received date
    {QSLSDATE, 		"QSLSDATE",      8, &btnSelectQSLsent},  // QSL sent date
    {QSL_RCVD, 		"QSL_RCVD",      1, NULL},    			// QSL received status
    {QSL_SENT, 		"QSL_SENT",      1, NULL},    			// QSL sent status
    {QSO_DATE, 		"QSO_DATE",      8, &btnSelectQSOdate},  // QSO data
    {QTH, 			"QTH",          30, &btnSelectQth},    	// contacted stations city
    {RST_RCVD, 		"RST_RCVD",      3, &btnSelectRSTrcvd},  // received signal report
    {RST_SENT, 		"RST_SENT",      3, &btnSelectRSTsent},  // sent signal report
    {STATE, 		"STATE",         2, &btnSelectState},    // contacted stations STATE
    {STX, 			"STX",           8, &btnSelectSerialOUT},// QSO transmitted serial number
    {TIME_OFF, 		"TIME_OFF",      4, &btnSelectTimeOFF},  // HHMM or HHMMSS in UTC
    {TIME_ON, 		"TIME_ON",       4, &btnSelectTimeON},   // HHMM or HHMMSS in UTC
    {TX_PWR, 		"TX_PWR",        4, &btnSelectTX_pwr},   // power transmitted by this station
// new fields
    {IOTA, 			"IOTA",       	 6, &btnSelectIOTA},     // Islands on the air 
    {ITUZ,			"ITUZ",       	 6, NULL},    			// ITU zone
    {OPERATOR,		"OPERATOR",   	10, NULL},    			// Callsign of person loggin the QSO
    {PFX,			"PFX",        	 5, NULL},    			// WPA prefix
    {PROP_MODE,		"PROP_MODE",  	 5, NULL},    			// propogation mode
    {QSL_MSG,		"QSL_MSG",    	80, NULL},    			// personal message to appear on qsl card
    {QSL_VIA, 		"QSL_VIA",    	30, NULL},
    {RX_PWR, 		"RX_PWR",     	 4, NULL},    			// power of other station in watts
    {SAT_MODE,		"SAT_MODE",   	 8, NULL},    			// satellite mode
    {SAT_NAME,		"SAT_NAME",   	12, NULL},    			// satellite name
    {SRX,			"SRX",        	 5, &btnSelectSerialIN}, // received serial number for a contest QSO
    {TEN_TEN, 		"TEN_TEN",     	10, NULL},    			// ten ten # of other station
    {VE_PROV,		"VE_PROV",       2, &btnSelectProvince}, // 2 letter abbreviation for Canadian Province
// fldigi specific fields
	{XCHG1,			"XCHG1",        20, &btnSelectXchg1},    // contest exchange #1 / free1 in xlog
	{XCHG2,			"XCHG2",        20, &btnSelectXchg2},    // contest exchange #2 / free2 in xlog
	{XCHG3,			"XCHG3",        20, &btnSelectXchg3},    // contest exchange #3
    {EXPORT,		"EXPORT",        1, NULL}     			// used to indicate record is to be exported
};

int fieldnbr (const char *s) {
    for (int i = 0; i < EXPORT; i++)
        if (strncasecmp( fields[i].name, s, fields[i].size) == 0)
            return i;
    return -1;
}

int findfield (char *p) {
    for (int i=0; i < EXPORT; i++)
        if (strncasecmp (p, fields[i].name, strlen(fields[i].name)) == 0)
            return i;
    if (strncasecmp (p, "EOR>", 4) == 0)
        return -1;
    return -2;
}

void cAdifIO::fillfield (int fieldnum, char *buff){
char *p = buff;
char numeral[8];
int n, fldsize;
    memset (numeral, 0, 8);
    n = 0;
    while (*p != ':' && n < 11) {p++; n++;}
    if (n == 11) return; // bad ADIF specifier ---> no ':' after field name
// found first ':'
    p++;
    n = 0;
    while (*p >= '0' && *p <= '9' && n < 8) {
        numeral[n++] = *p;
        p++;
    }
    fldsize = atoi(numeral);
    p = strchr(buff,'>'); // end of specifier +1 == > start of data
    if (!p) return;
    p++;
    char *flddata = new char[fldsize+1];
    memset (flddata, 0, fldsize + 1);
    strncpy (flddata, p, fldsize);
    adifqso.putField (fieldnum, (const char *)flddata); 
    delete [] flddata;
}

void cAdifIO::readFile (const char *fname, cQsoDb *db) {
    long filesize = 0;
    char *buff;
    int found;
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
    fread (buff, filesize, 1, adiFile);
    fclose (adiFile);
    
// relaxed file integrity test to all importing from non conforming log programs
    if (filesize == 0 || (strcasestr( buff, "<CALL:")) == 0) { //"<ADIF_VER")) == 0) {
    	fl_message("Not an ADIF log file");
    	return;
	}

	char *p1 = buff, *p2;
// is there a header?
    if (*p1 != '<') { // yes find the start of the records
        p1 = strchr(buff, '<');
        while (strncasecmp (p1+1,"EOH>", 4) != 0) {
            p1 = strchr(p1+1, '<'); // find next <> field
        }
        if (!p1) return;     // must not be an ADIF compliant file
        p1 += 1;
    }
    
    p2 = strchr(p1,'<'); // find first ADIF specifier
    adifqso.clearRec();
    while (p2) {
        found = findfield(p2+1); // -2 ==> not found; -1 <eor> 0 ...N field #
//        if (found == -2 ) return; // unknown field
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
//    db->SortByDate();
    for (int i = 0; i < db->nbrRecs(); i++) {
        rec = db->getRec(i);
        if (rec->getField(EXPORT)[0] == 'E') {
        	for (int j = 0; j < EXPORT; j++) {
        		if (fields[j].btn != NULL)
        			if ((*fields[j].btn)->value()) {
       			    	szFld = rec->getField(fields[j].type);
        				fprintf(adiFile, adifmt,
   	    						fields[j].name, strlen(szFld), szFld);
				}
			}
            rec->putField(EXPORT,"");
            db->qsoUpdRec(i, rec);
            fprintf(adiFile, "<EOR>");
            fprintf(adiFile, szEOL);
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
//    db->SortByDate();
    for (int i = 0; i < db->nbrRecs(); i++) {
        rec = db->getRec(i);
        for (int j = 0; j < EXPORT; j++) {
            szFld = rec->getField(j);
            if (strlen(szFld))
                fprintf(adiFile, adifmt,
                    fields[j].name, strlen(szFld), szFld);
        }
        db->qsoUpdRec(i, rec);
        fprintf(adiFile, "<EOR>");
        fprintf(adiFile, szEOL);
    }
    fclose (adiFile);
    return 0;
}
