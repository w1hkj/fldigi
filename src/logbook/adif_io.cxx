#include <FL/Fl.H>
#include <FL/filename.H>
#include <FL/fl_ask.H>

#include <string>
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
	{CNTY,           0,  &btnSelectCNTY},      // secondary political subdivision, ie: county
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

static void initfields()
{
	for (int i = 0; i < NUMFIELDS; i++)
		fields[i].name = new string(fieldnames[i]);
}

static int findfield( const char *p )
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

void cAdifIO::fillfield (int fieldnum, char *buff)
{
const char *p = buff;
int fldsize;
	while (*p != ':' && *p != '>') p++;
	if (*p == '>') return; // bad ADIF specifier ---> no ':' after field name
// found first ':'
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

static void write_rxtext(const char *s)
{
	ReceiveText->add(s);
}

void cAdifIO::do_readfile(const char *fname, cQsoDb *db)
{
	long filesize = 0;
	char *buff;
	int found;
	int retval;

// open the adif file
	FILE *adiFile = fopen (fname, "r");

	if (adiFile == NULL)
		return;
// determine its size for buffer creation
	fseek (adiFile, 0, SEEK_END);
	filesize = ftell (adiFile);

	if (filesize == 0) {
		LOG_INFO("%s", _("Empty ADIF logbook file"));
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
		LOG_ERROR("%s", _("Not an ADIF file"));
		delete [] buff;
		return;
	}
	if (strcasestr(buff, "<CALL:") == 0) {
		LOG_ERROR("%s", _("Not an ADIF file"));
		delete [] buff;
		return;
	}

	struct timespec t0, t1;
#ifdef _POSIX_MONOTONIC_CLOCK
	clock_gettime(CLOCK_MONOTONIC, &t0);
#else
	clock_gettime(CLOCK_REALTIME, &t0);
#endif

	static string msg;
	msg.clear();
	msg.append("\n<===== Reading ").append(fl_filename_name(fname)).append(" =====>");
	REQ(write_rxtext, msg.c_str());

	char *p1 = buff, *p2;
	if (*p1 != '<') { // yes, skip over header to start of records
		p1 = strchr(buff, '<');
		while (strncasecmp (p1+1,"EOH>", 4) != 0) {
			p1 = strchr(p1+1, '<'); // find next <> field
		}
		if (!p1) {
			delete [] buff;
			LOG_ERROR("%s", _("Not an ADIF file"));
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

			if ((strlen(adifqso.getField(QSO_DATE)) > 7) && 
				(adifqso.getField(QSO_DATE_OFF)[0] == 0)) {
				char d_str[20];
				int d, m, y, t_on, t_off;
				strcpy(d_str, adifqso.getField(QSO_DATE));
				d = atoi(&d_str[6]); d_str[6] = 0;
				m = atoi(&d_str[4]); d_str[4] = 0;
				y = atoi(d_str);
				t_on = atoi(adifqso.getField(TIME_ON));
				t_off = atoi(adifqso.getField(TIME_OFF));
				Date dt(m, d, y);
				if (t_off < t_on) dt++;
				adifqso.putField(QSO_DATE_OFF, dt.szDate(2));
			}
			db->qsoNewRec (&adifqso);
			adifqso.clearRec();
		}
		p1 = p2 + 1;
		p2 = strchr(p1,'<');
	}
	db->SortByDate(progdefaults.sort_date_time_off);
	delete [] buff;

#ifdef _POSIX_MONOTONIC_CLOCK
	clock_gettime(CLOCK_MONOTONIC, &t1);
#else
	clock_gettime(CLOCK_REALTIME, &t1);
#endif

	t0 = t1 - t0;
	float t = (t0.tv_sec + t0.tv_nsec/1e9);

	static char szmsg[50];
	snprintf(szmsg, sizeof(szmsg), "\n<=== read %d records in %4.2f seconds===>\n", db->nbrRecs(), t);
	REQ(write_rxtext, szmsg);

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

	if (!ADIF_RW_thread)
		ADIF_RW_init();

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

int cAdifIO::writeLog (const char *fname, cQsoDb *db, bool immediate) {
	ENSURE_THREAD(FLMAIN_TID);

	if (!ADIF_RW_thread)
		ADIF_RW_init();

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
		return;
	}

	string sFld;
	cQsoRec *rec;

	records.clear();
	for (int i = 0; i < adifdb->nbrRecs(); i++) {
		rec = adifdb->getRec(i);
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
	LOG_INFO("%d records written to %s", nrecs, adif_file_name.c_str());

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

	SET_THREAD_CANCEL();

	for (;;) {
		TEST_THREAD_CANCEL();
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

	CANCEL_THREAD(*ADIF_RW_thread);

	pthread_mutex_lock(&ADIF_RW_mutex);
	ADIF_RW_EXIT = true;
	pthread_cond_signal(&ADIF_RW_cond);
	pthread_mutex_unlock(&ADIF_RW_mutex);

	pthread_join(*ADIF_RW_thread, NULL);
	delete ADIF_RW_thread;
	ADIF_RW_thread = 0;
	if (wrdb) delete wrdb;
}

static void ADIF_RW_init()
{
	ENSURE_THREAD(FLMAIN_TID);

	if (ADIF_RW_thread)
		return;
	ADIF_RW_thread = new pthread_t;
	ADIF_RW_EXIT = false;
	if (pthread_create(ADIF_RW_thread, NULL, ADIF_RW_loop, NULL) != 0) {
		LOG_PERROR("pthread_create");
		return;
	}
	MilliSleep(10);
}
