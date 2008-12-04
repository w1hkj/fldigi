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

#include "adif_io.h"

#include "logsupport.h"

#include <FL/fl_ask.H>

static string adif;

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
        int slen = strlen(s);
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

char logdate[32], logtime[32], adifdate[32];
const char *logmode;

//---------------------------------------------------------------------

int submit_log(void)
{
	if (progStatus.spot_log)
		spot_log(inpCall->value(), inpLoc->value());

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

	logmode = mode_info[active_modem->get_mode()].adif_name;
	
	snprintf(strFreqMhz, sizeof(strFreqMhz), "%-10f", wf->dFreq()/1.0e6);

	adif.erase();
	
	log_msg = "";
	log_msg = log_msg + "program:"	+ PACKAGE_NAME + " v " + PACKAGE_VERSION + LOG_MSEPARATOR;
	log_msg = log_msg + "version:"	+ LOG_MVERSION			+ LOG_MSEPARATOR;
	log_msg = log_msg + "date:"		+ zuluLogDate				+ LOG_MSEPARATOR;
	putadif(QSO_DATE, zuluLogDate); 
//	log_msg = log_msg + "time:"		+ zuluLogTime		+ LOG_MSEPARATOR;
//	putadif(TIME_ON, zuluLogTime);
	log_msg = log_msg + "endtime:"	+ zuluLogTime			+ LOG_MSEPARATOR;
	putadif(TIME_OFF, zuluLogTime);
	log_msg = log_msg + "call:"		+ inpCall->value()		+ LOG_MSEPARATOR;
	putadif(CALL, inpCall->value());
	log_msg = log_msg + "mhz:"		+ strFreqMhz			+ LOG_MSEPARATOR;
	putadif(FREQ, strFreqMhz);
	log_msg = log_msg + "mode:"		+ logmode				+ LOG_MSEPARATOR;
	putadif(MODE, logmode);
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
		
	string fieldstr;
	fieldstr = outSerNo->value();
	if (!fieldstr.empty()) {
		log_msg = log_msg + "serialout:"		+ fieldstr.c_str()	+ LOG_MSEPARATOR;
		putadif(STX, fieldstr.c_str());
	}
	fieldstr = inpSerNo->value();
	if (!fieldstr.empty()) {	
		log_msg = log_msg + "serialin:"	+ fieldstr.c_str()		+ LOG_MSEPARATOR;
		putadif(SRX, fieldstr.c_str());
	}
	fieldstr = inpXchg1->value();
	if (!fieldstr.empty()) {	
		log_msg = log_msg + "xchg1:"	+ fieldstr.c_str()		+ LOG_MSEPARATOR;
		putadif(XCHG1, fieldstr.c_str());
	}
	fieldstr = inpXchg2->value();
	if (!fieldstr.empty()) {	
		log_msg = log_msg + "xchg2:"	+ fieldstr.c_str()		+ LOG_MSEPARATOR;
		putadif(XCHG2, fieldstr.c_str());
	}
	fieldstr = inpXchg3->value();
	if (!fieldstr.empty()) {	
		log_msg = log_msg + "xchg3:"	+ fieldstr.c_str()		+ LOG_MSEPARATOR;
		putadif(XCHG2, fieldstr.c_str());
	}
	
	log_msg = log_msg + "notes:"	+ inpNotes->value()		+ LOG_MSEPARATOR;
	putadif(NOTES, inpNotes->value());

	writeadif();

	AddRecord();

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

