#include <FL/Fl.H>
#include <FL/filename.H>
#include <FL/fl_ask.H>

#include <cstring>
#include <cstdlib>
#include <string>

#include "fl_digi.h"

#include "signal.h"
#include "threads.h"
#include "adif_io.h"
#include "config.h"
#include "configuration.h"
#include "lgbook.h"
#include "icons.h"
#include "gettext.h"
#include "debug.h"
#include "util.h"
#include "date.h"
#include "logsupport.h"
#include "qrunner.h"
#include "timeops.h"

using namespace std;

#ifdef __WOE32__
static const char *szEOL = "\r\n";
#else
static const char *szEOL = "\n";
#endif
static const char *szEOR = "<EOR>";

// These ADIF fields define the ADIF database
FIELD fields[] = {
//  TYPE,            NAME,           WIDGET
	{FREQ,           "FREQ",         &btnSelectFreq},      // QSO frequency in Mhz
	{CALL,           "CALL",         &btnSelectCall},      // contacted stations CALLSIGN
	{MODE,           "MODE",         &btnSelectMode},      // QSO mode
	{NAME,           "NAME",         &btnSelectName},      // contacted operators NAME
	{QSO_DATE,       "QSO_DATE",     &btnSelectQSOdateOn}, // QSO data
	{QSO_DATE_OFF,   "QSO_DATE_OFF", &btnSelectQSOdateOff},// QSO data OFF, according to ADIF 2.2.6
	{TIME_OFF,       "TIME_OFF",     &btnSelectTimeOFF},   // HHMM or HHMMSS in UTC
	{TIME_ON,        "TIME_ON",      &btnSelectTimeON},    // HHMM or HHMMSS in UTC
	{QTH,            "QTH",          &btnSelectQth},       // contacted stations city
	{RST_RCVD,       "RST_RCVD",     &btnSelectRSTrcvd},   // received signal report
	{RST_SENT,       "RST_SENT",     &btnSelectRSTsent},   // sent signal report
	{STATE,          "STATE",        &btnSelectState},     // contacted stations STATE
	{VE_PROV,        "VE_PROV",      &btnSelectProvince},  // 2 letter abbreviation for Canadian Province
	{NOTES,          "NOTES",        &btnSelectNotes},     // QSO notes
	{QSLRDATE,       "QSLRDATE",     &btnSelectQSLrcvd},   // QSL received date
	{QSLSDATE,       "QSLSDATE",     &btnSelectQSLsent},   // QSL sent date
	{GRIDSQUARE,     "GRIDSQUARE",   &btnSelectLOC},       // contacted stations Maidenhead Grid Square
	{BAND,           "BAND",         &btnSelectBand},      // QSO band
	{CNTY,           "CNTY",         &btnSelectCNTY},      // secondary political subdivision, ie: county
	{COUNTRY,        "COUNTRY",      &btnSelectCountry},   // contacted stations DXCC entity name
	{CQZ,            "CQZ",          &btnSelectCQZ},       // contacted stations CQ Zone
	{DXCC,           "DXCC",         &btnSelectDXCC},      // contacted stations Country Code
	{QSL_VIA,        "QSL_VIA",      &btnSelectQSL_VIA},   // contacted stations path
	{IOTA,           "IOTA",         &btnSelectIOTA},      // Islands on the air
	{ITUZ,           "ITUZ",         &btnSelectITUZ},      // ITU zone
	{CONT,           "CONT",         &btnSelectCONT},      // contacted stations continent

	{MYXCHG,         "MYXCHG",       &btnSelectMyXchg},    // contest exchange sent
	{XCHG1,          "XCHG1",        &btnSelectXchgIn},    // contest exchange #1 / free1 in xlog

	{MYXCHG,         "STX_STRING",   &btnSelectMyXchg},    // contest exchange sent
	{XCHG1,          "SRX_STRING",   &btnSelectXchgIn},    // contest exchange #1 / free1 in xlog

	{SRX,            "SRX",          &btnSelectSerialIN},  // received serial number for a contest QSO
	{STX,            "STX",          &btnSelectSerialOUT}, // QSO transmitted serial number
	{TX_PWR,         "TX_PWR",       &btnSelectTX_pwr},    // power transmitted by this station
	{NUMFIELDS,      "",             NULL}
};

// This ADIF fields is in the fldigi QSO database, but not saved in the ADIF file
/*
	{EXPORT,         "EXPORT",       NULL},                // used to indicate record is to be exported
*/

// These ADIF fields are not in the fldigi QSO database
/*
	{COMMENT,        "COMMENT",      NULL},                // comment field for QSO
	{ADDRESS,        "ADDRESS",      NULL},                // contacted stations mailing address
	{AGE,            "AGE",          NULL},                // contacted operators age in years
	{ARRL_SECT,      "ARRL_SECT",    NULL},                // contacted stations ARRL section
	{CONTEST_ID,     "CONTEST_ID",   NULL},                // QSO contest identifier
	{OPERATOR,       "OPERATOR",     NULL},                // Callsign of person logging the QSO
	{PFX,            "PFX",          NULL},                // WPA prefix
	{PROP_MODE,      "PROP_MODE",    NULL},                // propogation mode
	{QSL_MSG,        "QSL_MSG",      NULL},                // personal message to appear on qsl card
	{QSL_RCVD,       "QSL_RCVD",     NULL},                // QSL received status
	{QSL_SENT,       "QSL_SENT",     NULL},                // QSL sent status
	{QSL_VIA,        "QSL_VIA",      NULL},                // QSL via this person
	{RX_PWR,         "RX_PWR",       NULL},                // power of other station in watts
	{SAT_MODE,       "SAT_MODE",     NULL},                // satellite mode
	{SAT_NAME,       "SAT_NAME",     NULL},                // satellite name
	{TEN_TEN,        "TEN_TEN",      NULL}                 // ten ten # of other station
};
*/

static char *fastlookup = 0;

static unsigned int maxlen = 0;

static void initfields()
{
	if (fastlookup) return; // may have multiple instances using common code
	int i = 0;
	while (fields[i].type != NUMFIELDS) {
		if (strlen(fields[i].name) > maxlen) maxlen = strlen(fields[i].name);
		i++;
	}
	maxlen++;
	fastlookup = new char[maxlen * i + 1];
	fastlookup[0] = 0;
	i = 0;
	while (fields[i].type != NUMFIELDS) {
		strcat(fastlookup, fields[i].name);
		unsigned int n = maxlen - strlen(fastlookup) % maxlen;
		if (n > 0 && n < maxlen) for (unsigned int j = 0; j < n; j++) strcat(fastlookup, " ");
		i++;
	}
}

static inline int findfield( char *p )
{
	if (strncasecmp (p, "EOR>", 4) == 0 || !maxlen)
		return -1;

	char *pos;
	char *p1 = strchr(p, ':');
	char *p2 = strchr(p, '>');
	if (p1 && p2) {
		if (p1 < p2) {
			pos = p;
			do { *pos = toupper(*pos); } while (++pos < p1);
			*p1 = 0;
			pos = strcasestr(fastlookup, p);
			*p1 = ':';
			if (pos) return fields[(pos - fastlookup) / maxlen].type;
		}
	}
	return -2;		//search key not found
}

int cAdifIO::instances = 0;

cAdifIO::cAdifIO ()
{
	initfields();
	instances++;
}

cAdifIO::~cAdifIO()
{
	if (--instances == 0) {
		delete [] fastlookup;
		fastlookup = 0;
	}
}

void cAdifIO::fillfield (int fieldnum, char *buff)
{
	char *p1 = strchr(buff, ':');
	char *p2 = strchr(buff, '>');
	if (!p1 || !p2 || p2 < p1) return; // bad ADIF specifier ---> no ':' after field name

	p1++;
	int fldsize = 0;
	while (p1 != p2) {
		if (*p1 >= '0' && *p1 <= '9') {
			fldsize = fldsize * 10 + *p1 - '0';
		}
		p1++;
	}
	if ((fieldnum == TIME_ON || fieldnum == TIME_OFF) && fldsize < 6) {
		string tmp = "";
		tmp.assign(p2+1, fldsize);
		while (tmp.length() < 6) tmp += '0';
		adifqso->putField(fieldnum, tmp.c_str(), 6);
	} else
		adifqso->putField (fieldnum, p2+1, fldsize);
}

static void write_rxtext(const char *s)
{
	ReceiveText->addstr(s);
}

void cAdifIO::do_readfile(const char *fname, cQsoDb *db)
{
	long filesize = 0;
	char *buff;
	int found;

LOG_INFO("Reading %s", fname);

// open the adif file
	FILE *adiFile = fopen (fname, "r");

	if (adiFile == NULL) {
LOG_INFO("Cannot open %s", fname);
		return;
	}

// determine its size for buffer creation
	fseek (adiFile, 0, SEEK_END);
	filesize = ftell (adiFile);

	if (filesize == 0) {
		LOG_INFO(_("Empty ADIF logbook file %s"), fl_filename_name(fname));
		return;
	}

	buff = new char[filesize + 1];

	static char szmsg[100];
	static char szmsg2[100];
	snprintf(szmsg, sizeof(szmsg), "Reading %ld bytes from %s",
		filesize, fl_filename_name(fname));
	REQ(write_rxtext, "\n*** ");
	REQ(write_rxtext, szmsg);
	LOG_INFO("%s", szmsg);
// read the entire file into the buffer

	fseek (adiFile, 0, SEEK_SET);
	int retval = fread (buff, filesize, 1, adiFile);
	fclose (adiFile);
	if (retval != 1) {
		LOG_ERROR(_("Error reading %s"), fl_filename_name(fname));
		return;
	}

// relaxed file integrity test to all importing from non conforming log programs
	if (strcasestr(buff, "<CALL:") == 0) {
		strcpy(szmsg2, "NO RECORDS IN FILE");
		REQ(write_rxtext, "\n*** ");
		REQ(write_rxtext, szmsg2);
		REQ(write_rxtext, "\n");
		LOG_INFO("%s", szmsg2);
		delete [] buff;
		db->clearDatabase();
		return;
	}

	struct timespec t0, t1;
#ifdef _POSIX_MONOTONIC_CLOCK
	clock_gettime(CLOCK_MONOTONIC, &t0);
#else
	clock_gettime(CLOCK_REALTIME, &t0);
#endif

	char *p1 = buff, *p2;
	if (*p1 != '<') { // yes, skip over header to start of records
		p1 = strchr(buff, '<');
		while (strncasecmp (p1+1,"EOH>", 4) != 0) {
			p1 = strchr(p1+1, '<'); // find next <> field
		}
		if (!p1) {
			delete [] buff;
			strcpy(szmsg2, "Corrupt ADIF file ***");
			REQ(write_rxtext, "\n*** ");
			REQ(write_rxtext, szmsg2);
			REQ(write_rxtext, "\n");
			LOG_ERROR("%s", szmsg2);
			return;	 // must not be an ADIF compliant file
		}
		p1 += 1;
	}

	p2 = strchr(p1,'<'); // find first ADIF specifier

	adifqso = 0;
	while (p2) {
		found = findfield(p2+1);
		if (found > -1) {
			if (!adifqso) adifqso = db->newrec(); // need new record in db
			fillfield (found, p2+1);
		} else if (found == -1) { // <eor> reached;
			adifqso = 0;
		}
		p1 = p2 + 1;
		p2 = strchr(p1,'<');
	}
	delete [] buff;

#ifdef _POSIX_MONOTONIC_CLOCK
	clock_gettime(CLOCK_MONOTONIC, &t1);
#else
	clock_gettime(CLOCK_REALTIME, &t1);
#endif

	t0 = t1 - t0;
	float t = (t0.tv_sec + t0.tv_nsec/1e9);

	snprintf(szmsg2, sizeof(szmsg2), "Read %d records in %4.2f seconds", db->nbrRecs(), t);
	REQ(write_rxtext, "\n*** ");
	REQ(write_rxtext, szmsg2);
	REQ(write_rxtext, "\n");
	LOG_INFO("%s", szmsg2);

	if (db == &qsodb)
		REQ(adif_read_OK);
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
			int j = 0;
			while (fields[j].type != NUMFIELDS) {
				if (strcmp(fields[j].name,"MYXCHG") == 0) { j++; continue; }
				if (strcmp(fields[j].name,"XCHG1") == 0) { j++; continue; }
				if (fields[j].btn != NULL)
					if ((*fields[j].btn)->value()) {
					sFld = rec->getField(fields[j].type);
						if (!sFld.empty())
							fprintf(adiFile, adifmt,
								fields[j].name,//->c_str(),
								sFld.length());
							fprintf(adiFile, "%s", sFld.c_str());
				}
				j++;
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

//======================================================================
// thread support writing database
//======================================================================

pthread_t* ADIF_RW_thread = 0;
pthread_mutex_t ADIF_RW_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t ADIF_RW_cond = PTHREAD_COND_INITIALIZER;
static void ADIF_RW_init();

static string adif_file_image;
static string adif_file_name;
static string records;
static string record;
static char recfield[200];
static int nrecs;

static bool ADIF_READ = false;
static bool ADIF_WRITE = false;

static cQsoDb *adif_db;

static cAdifIO *adifIO = 0;

void cAdifIO::readFile (const char *fname, cQsoDb *db) 
{
	ENSURE_THREAD(FLMAIN_TID);

	if (!ADIF_RW_thread) {
		ADIF_RW_init();
		MilliSleep(50);
	}

	pthread_mutex_lock(&ADIF_RW_mutex);

	adif_file_name = fname;
	adif_db = db;
	adifIO = this;
	ADIF_READ = true;

	pthread_cond_signal(&ADIF_RW_cond);
	pthread_mutex_unlock(&ADIF_RW_mutex);
}

static cQsoDb *adifdb = 0;
static cQsoDb *wrdb = 0;

static struct timespec t0, t1;

int cAdifIO::writeLog (const char *fname, cQsoDb *db, bool immediate) {
	ENSURE_THREAD(FLMAIN_TID);

	if (!ADIF_RW_thread)
		ADIF_RW_init();

#ifdef _POSIX_MONOTONIC_CLOCK
	clock_gettime(CLOCK_MONOTONIC, &t0);
#else
	clock_gettime(CLOCK_REALTIME, &t0);
#endif

	if (!immediate) {
		pthread_mutex_lock(&ADIF_RW_mutex);
		adif_file_name = fname;
		adifIO = this;
		ADIF_WRITE = true;
		if (wrdb) delete wrdb;
		wrdb = new cQsoDb(db);
		adifdb = wrdb;
		pthread_cond_signal(&ADIF_RW_cond);
		pthread_mutex_unlock(&ADIF_RW_mutex);
	} else {
		adif_file_name = fname;
		adifdb = db;
		do_writelog();
	}

	return 1;
}

void cAdifIO::do_writelog()
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
	ADIFHEADER.append("<DATA CHECKSUM:%d>%s");
	ADIFHEADER.append(szEOL);
	ADIFHEADER.append("<EOH>");
	ADIFHEADER.append(szEOL);

	Ccrc16 checksum;
	string s_checksum;

	adiFile = fopen (adif_file_name.c_str(), "w");

	if (!adiFile) {
		LOG_ERROR("Cannot write to %s", adif_file_name.c_str());
		if (wrdb) {
			delete wrdb;
			wrdb = 0;
		}
		return;
	}
	LOG_INFO("Writing %s", adif_file_name.c_str());

	string sFld;
	cQsoRec *rec;

	records.clear();
	for (int i = 0; i < adifdb->nbrRecs(); i++) {
		rec = adifdb->getRec(i);
		record.clear();
		int j = 0;
		while (fields[j].type != NUMFIELDS) {
			if (strcmp(fields[j].name,"MYXCHG") == 0) { j++; continue; }
			if (strcmp(fields[j].name,"XCHG1") == 0) { j++; continue; }
			sFld = rec->getField(fields[j].type);
			if (!sFld.empty()) {
				snprintf(recfield, sizeof(recfield), adifmt,
					fields[j].name,
					sFld.length());
				record.append(recfield).append(sFld);
			}
			j++;
		}
		record.append(szEOR);
		record.append(szEOL);
		records.append(record);
		adifdb->qsoUpdRec(i, rec);
	}
	nrecs = adifdb->nbrRecs();

	s_checksum = checksum.scrc16(records);

	fprintf (adiFile, ADIFHEADER.c_str(),
		 fl_filename_name(adif_file_name.c_str()),
		 strlen(ADIF_VERS), ADIF_VERS,
		 strlen(PACKAGE_NAME), PACKAGE_NAME,
		 strlen(PACKAGE_VERSION), PACKAGE_VERSION,
		 s_checksum.length(), s_checksum.c_str()
		);
	fprintf (adiFile, "%s", records.c_str());

	fclose (adiFile);

	if (wrdb) {
		delete wrdb;
		wrdb = 0;
	}

#ifdef _POSIX_MONOTONIC_CLOCK
	clock_gettime(CLOCK_MONOTONIC, &t1);
#else
	clock_gettime(CLOCK_REALTIME, &t1);
#endif

	t0 = t1 - t0;
	float t = (t0.tv_sec + t0.tv_nsec/1e9);

	static char szmsg[50];
	snprintf(szmsg, sizeof(szmsg), "%d records in %4.2f seconds", adifdb->nbrRecs(), t);
	LOG_INFO("%s", szmsg);

	return;
}

//======================================================================
// thread to support writing database in a separate thread
//======================================================================

static void *ADIF_RW_loop(void *args);
static bool ADIF_RW_EXIT = false;

static void *ADIF_RW_loop(void *args)
{
	SET_THREAD_ID(ADIF_RW_TID);

	for (;;) {
		pthread_mutex_lock(&ADIF_RW_mutex);
		pthread_cond_wait(&ADIF_RW_cond, &ADIF_RW_mutex);
		pthread_mutex_unlock(&ADIF_RW_mutex);

		if (ADIF_RW_EXIT)
			return NULL;
		if (ADIF_WRITE && adifIO) {
			adifIO->do_writelog();
			ADIF_WRITE = false;
		} else if (ADIF_READ && adifIO) {
			adifIO->do_readfile(adif_file_name.c_str(), adif_db);
			ADIF_READ = false;
		}
	}
	return NULL;
}

void ADIF_RW_close(void)
{
	ENSURE_THREAD(FLMAIN_TID);

	if (!ADIF_RW_thread)
		return;

	pthread_mutex_lock(&ADIF_RW_mutex);
	ADIF_RW_EXIT = true;
	pthread_cond_signal(&ADIF_RW_cond);
	pthread_mutex_unlock(&ADIF_RW_mutex);

	pthread_join(*ADIF_RW_thread, NULL);
	delete ADIF_RW_thread;
	ADIF_RW_thread = 0;
}

static void ADIF_RW_init()
{
	ENSURE_THREAD(FLMAIN_TID);

	if (ADIF_RW_thread)
		return;
LOG_INFO("%s","Starting logbook r/w thread");

	ADIF_RW_thread = new pthread_t;
	ADIF_RW_EXIT = false;
	if (pthread_create(ADIF_RW_thread, NULL, ADIF_RW_loop, NULL) != 0) {
		LOG_PERROR("pthread_create");
		return;
	}
//	MilliSleep(50); // increased from 10 for Win7 testing
}
