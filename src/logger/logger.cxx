// ====================================================================
//  logger.cxx Remote Log Interface for fldigi
//
// Copyright W1HKJ, Dave Freese 2006
//
// This library is free software; you can RGBredistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA.
//
// Please report all bugs and problems to "w1hkj@w1hkj.com".
//
// ====================================================================

#include <config.h>

#include <sys/types.h>
#include <sys/stat.h>
#ifndef __CYGWIN__
#  include <sys/ipc.h>
#  include <sys/msg.h>
#endif
#include <errno.h>
#include <string>

#include "logger.h"
#include "main.h"
#include "modem.h"
#include "debug.h"
#include "macros.h"
#include "status.h"
#include "spot.h"

#include <FL/fl_ask.H>

enum ADIF_FIELD_POS {
    ADDRESS = 0, 
    AGE, 
    ARRL_SECT, 
    BAND, 
    CALL,
    CNTY, 
    COMMENT, 
    CONT, 
    CONTEST_ID,
    COUNTRY,
    CQZ, 
    DXCC, 
    FREQ, 
    GRIDSQUARE, 
    MODE,
    NAME, 
    NOTES, 
    QSLRDATE, 
    QSLSDATE, 
    QSL_RCVD,
    QSL_SENT, 
    QSO_DATE,
    QTH, 
    RST_RCVD, 
    RST_SENT,
    STATE, 
    STX,
    TIME_OFF, 
    TIME_ON, 
    TX_PWR,
// additional for 2.0
    IOTA,
    ITUZ,
    OPERATOR,
    PFX,
    PROP_MODE,
    QSL_MSG,
    QSL_VIA, 
    RX_PWR, 
    SAT_MODE,
    SAT_NAME,
    SRX, 
    TEN_TEN, 
    VE_PROV,
    EXPORT, // internal use in fl_logbook
    NUMFIELDS // counter for number of fields in enum
};

struct FIELD {
  int  type;
  const char *name;
  size_t  size;
};
  
FIELD fields[] = {
//  TYPE,  NAME,    SIZE
    {ADDRESS,       "ADDRESS",      40},    // 0 - contacted stations mailing address
    {AGE,           "AGE",           3},    // 1 - contacted operators age in years
    {ARRL_SECT,     "ARRL_SECT",    12},    // 2 - contacted stations ARRL section
    {BAND,          "BAND",          6},    // 3 - QSO band
    {CALL,          "CALL",         14},    // 4 - contacted stations CALLSIGN
    {CNTY,          "CNTY",         20},    // 5 - secondary political subdivision, ie: STATE
    {COMMENT,       "COMMENT",      80},    // 6 - comment field for QSO
    {CONT,          "CONT",         10},    // 7 - contacted stations continent
    {CONTEST_ID,    "CONTEST_ID",    6},    // 8 - QSO contest identifier
    {COUNTRY,       "COUNTRY",      20},    // 9 - contacted stations DXCC entity name
    {CQZ,           "CQZ",           8},    // 10 - contacted stations CQ Zone
    {DXCC,          "DXCC",          8},    // 11 - contacted stations Country Code
    {FREQ, 			"FREQ",			10},    // 12 - QSO frequency in Mhz
    {GRIDSQUARE, 	"GRIDSQUARE",	 6},    // 13 - contacted stations Maidenhead Grid Square
    {MODE,			"MODE",          8},    // 14 - QSO mode
    {NAME, 			"NAME",         18},    // 15 - contacted operators NAME
    {NOTES, 		"NOTES",        80},    // 16 - QSO notes
    {QSLRDATE, 		"QSLRDATE",      8},    // 21 - QSL received date
    {QSLSDATE, 		"QSLSDATE",      8},    // 22 - QSL sent date
    {QSL_RCVD, 		"QSL_RCVD",      1},    // 23 - QSL received status
    {QSL_SENT, 		"QSL_SENT",      1},    // 24 - QSL sent status
    {QSO_DATE, 		"QSO_DATE",      8},    // 25 - QSO data
    {QTH, 			"QTH",          30},    // 27 - contacted stations city
    {RST_RCVD, 		"RST_RCVD",      3},    // 28 - received signal report
    {RST_SENT, 		"RST_SENT",      3},    // 29 - sent signal report
    {STATE, 		"STATE",         2},    // 34 - contacted stations STATE
    {STX, 			"STX",           8},    // 35 - QSO transmitted serial number
    {TIME_OFF, 		"TIME_OFF",      4},    // 37 - HHMM or HHMMSS in UTC
    {TIME_ON, 		"TIME_ON",       4},    // 38 - HHMM or HHMMSS in UTC
    {TX_PWR, 		"TX_PWR",        4},    // 39 - power transmitted by this station
// new fields
    {IOTA, 			"IOTA",       	 6},    // 13 
    {ITUZ,			"ITUZ",       	 6},    // 14 - ITU zone
    {OPERATOR,		"OPERATOR",   	10},    // 17 - Callsign of person loggin the QSO
    {PFX,			"PFX",        	 5},    // 18 - WPA prefix
    {PROP_MODE,		"PROP_MODE",  	 5},    // 19 - propogation mode
    {QSL_MSG,		"QSL_MSG",    	80},    // 20 - personal message to appear on qsl card
    {QSL_VIA, 		"QSL_VIA",    	30},    // 26
    {RX_PWR, 		"RX_PWR",     	 4},    // 30 - power of other station in watts
    {SAT_MODE,		"SAT_MODE",   	 8},    // 31 - satellite mode
    {SAT_NAME,		"SAT_NAME",   	12},    // 32 - satellite name
    {SRX,			"SRX",        	 5},    // 33 - received serial number for a contest QSO
    {TEN_TEN, 		"TEN_TEN",     	10},    // 36 - ten ten # of other station
    {VE_PROV,		"VE_PROV",       2},    // 40 - 2 letter abbreviation for Canadian Province
    {EXPORT,		"EXPORT",        1}     // 41 - used to indicate record is to be exported
};

#define ADIF_VERS "2.1.9"
static string adif;

const char *ADIFHEADER = 
"<ADIF_VERS:%d>%s\n\
<PROGRAMID:%d>%s\n\
<PROGRAMVERSION:%d>%s\n\
<EOH>\n\n";


void writeadif () {
// open the adif file
	FILE *adiFile;

// Append to fldigi.adif on all platforms
	string sfname = HomeDir;
	sfname.append("fldigi.adif");
	adiFile = fopen (sfname.c_str(), "a");
	if (adiFile) {
// write the current record to the file  
		adif.append("<EOR>\n");
		fprintf(adiFile,"%s", adif.c_str());
		fclose (adiFile);
	}

// Append to FL_LOGBOOK adif file on Windows if and only if C:\FL_LOGBOOK exists
#ifdef __CYGWIN__
	sfname = "C:/FL_LOGBOOK/log.adif";
	adiFile = fopen (sfname.c_str(), "a");
	if (adiFile) {
// write the current record to the file  
		fprintf(adiFile,"%s", adif.c_str());
		fclose (adiFile);
	}
#endif
}

void putadif(int num, const char *s)
{
        char tempstr[100];
        size_t slen = strlen(s);
        if (slen > fields[num].size) slen = fields[num].size;
        int n = snprintf(tempstr, sizeof(tempstr), "<%s:%zu>", fields[num].name, slen);
        if (n == -1) {
		LOG_PERROR("snprintf");
                return;
        }
        memcpy(tempstr + n, s, slen);
        tempstr[n + slen] = '\0';
        adif.append(tempstr);
}


//---------------------------------------------------------------------

#ifndef __CYGWIN__
static msgtype msgbuf;
#endif
static string log_msg;
static string errmsg;
static char strFreqMhz[20];
char   LOG_MSEPARATOR[2] = {1,0};

//---------------------------------------------------------------------

int submit_log(void)
{
	if (progStatus.spot_log)
		spot_log(inpCall->value(), inpLoc->value());

	char logdate[32], logtime[32], adifdate[32];
#ifndef __CYGWIN__
	int msqid, len;
#endif
	struct tm *tm;
	time_t t;

	time(&t);
        tm = gmtime(&t);
		strftime(logdate, sizeof(logdate), "%d %b %Y", tm);
		snprintf(adifdate, sizeof(adifdate), "%04d%02d%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
		strftime(logtime, sizeof(logtime), "%H%M", tm);

	const char *mode = mode_info[active_modem->get_mode()].adif_name;
	
	snprintf(strFreqMhz, sizeof(strFreqMhz), "%-10f", wf->dFreq()/1.0e6);

	adif.erase();
	
	log_msg = "";
	log_msg = log_msg + "program:"	+ PACKAGE_NAME + " v " + PACKAGE_VERSION + LOG_MSEPARATOR;
	log_msg = log_msg + "version:"	+ LOG_MVERSION			+ LOG_MSEPARATOR;
	log_msg = log_msg + "date:"		+ logdate				+ LOG_MSEPARATOR;
	putadif(QSO_DATE, adifdate); 
	log_msg = log_msg + "time:"		+ inpTime->value()		+ LOG_MSEPARATOR;
	putadif(TIME_ON, inpTime->value());
	log_msg = log_msg + "endtime:"	+ logtime				+ LOG_MSEPARATOR;
	putadif(TIME_OFF, logtime);
	log_msg = log_msg + "call:"		+ inpCall->value()		+ LOG_MSEPARATOR;
	putadif(CALL, inpCall->value());
	log_msg = log_msg + "mhz:"		+ strFreqMhz			+ LOG_MSEPARATOR;
	putadif(FREQ, strFreqMhz);
	log_msg = log_msg + "mode:"		+ mode					+ LOG_MSEPARATOR;
	putadif(MODE, mode);
	log_msg = log_msg + "tx:"		+ inpRstOut->value()	+ LOG_MSEPARATOR;
	putadif(RST_SENT, inpRstOut->value());
	log_msg = log_msg + "rx:"		+ inpRstIn->value() 	+ LOG_MSEPARATOR;
	putadif(RST_RCVD, inpRstIn->value());
	log_msg = log_msg + "name:"		+ inpName->value()		+ LOG_MSEPARATOR;
	putadif(NAME, inpName->value());
	log_msg = log_msg + "qth:"		+ inpQth->value()		+ LOG_MSEPARATOR;
	putadif(QTH, inpQth->value());
	log_msg = log_msg + "cnty:"    + inpCnty->value()      + LOG_MSEPARATOR;
	putadif(CNTY, inpCnty->value());
	log_msg = log_msg + "province:"    + inpVEprov->value() + LOG_MSEPARATOR;
	putadif(VE_PROV, inpVEprov->value());
	log_msg = log_msg + "locator:"	+ inpLoc->value()		+ LOG_MSEPARATOR;
	putadif(GRIDSQUARE, inpLoc->value());
	char szcnt[5] = "";
	if (contest_count.count)
		snprintf(szcnt, sizeof(szcnt), "%04d", contest_count.count);
	log_msg = log_msg + "serialout:"		+ szcnt  		+ LOG_MSEPARATOR;
	putadif(STX, szcnt);
	log_msg = log_msg + "serialin:"	+ inpSerNo->value()		+ LOG_MSEPARATOR;
	putadif(SRX, inpSerNo->value());
	log_msg = log_msg + "notes:"	+ inpNotes->value()		+ LOG_MSEPARATOR;
	putadif(NOTES, inpNotes->value());

	writeadif();

#ifndef __CYGWIN__
	strncpy(msgbuf.mtext, log_msg.c_str(), sizeof(msgbuf.mtext));
	msgbuf.mtext[sizeof(msgbuf.mtext) - 1] = '\0';

	if ((msqid = msgget(LOG_MKEY, 0666 | IPC_CREAT)) == -1) {
		errmsg = "msgget: ";
		errmsg.append(strerror(errno));
		LOG_ERROR("%s", errmsg.c_str());
		fl_message(errmsg.c_str());
		return -1;
	}
	msgbuf.mtype = LOG_MTYPE;

// allow for the NUL
	len = strlen(msgbuf.mtext) + 1;
	if (msgsnd(msqid, &msgbuf, len, IPC_NOWAIT) < 0) {
		errmsg = "msgsnd: ";
		fl_message(errmsg.c_str());
		return -1;
	}
	return 0;
#else
	return -1;
#endif
}



//---------------------------------------------------------------------

