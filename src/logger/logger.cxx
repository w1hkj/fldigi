// ----------------------------------------------------------------------------
// logger.cxx Remote Log Interface for fldigi
//
// Copyright 2006-2009 W1HKJ, Dave Freese
//
// This file is part of fldigi.
//
// Fldigi is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#include <config.h>

#include <sys/types.h>
#include <sys/stat.h>
#if !defined(__WOE32__) && !defined(__APPLE__)
#  include <sys/ipc.h>
#  include <sys/msg.h>
#endif
#include <errno.h>
#include <string>

#include "logger.h"
#include "lgbook.h"
#include "main.h"
#include "modem.h"
#include "waterfall.h"
#include "fl_digi.h"
#include "trx.h"
#include "debug.h"
#include "macros.h"
#include "status.h"
#include "spot.h"
#include "adif_io.h"

#include "logsupport.h"

#include <FL/fl_ask.H>

using namespace std;

//---------------------------------------------------------------------
const char *logmode;

static string log_msg;
static string errmsg;
static string notes;

//=============================================================================

#if defined(__WOE32__) || defined(__APPLE__)

static string adif;

void writeADIF () {
// open the adif file
	FILE *adiFile;
	string sfname;

    sfname = TempDir;
    sfname.append("log.adif");
	adiFile = fopen (sfname.c_str(), "a");
	if (adiFile) {
// write the current record to the file  
		fprintf(adiFile,"%s<EOR>\n", adif.c_str());
		fclose (adiFile);
	}
}

void putadif(int num, const char *s)
{
	char tempstr[100];
	int slen = strlen(s);
	int n = snprintf(tempstr, sizeof(tempstr), "<%s:%d>", fields[num].name->c_str(), slen);
	if (n == -1) {
		LOG_PERROR("snprintf");
		return;
	}
	adif.append(tempstr).append(s);
}

void submit_ADIF(cQsoRec &rec)
{
	adif.erase();
	
	putadif(QSO_DATE, rec.getField(QSO_DATE_OFF));
	putadif(TIME_ON, rec.getField(TIME_ON));
	putadif(TIME_OFF, rec.getField(TIME_OFF));
	putadif(CALL, rec.getField(CALL));
	putadif(FREQ, rec.getField(FREQ));
	putadif(MODE, rec.getField(MODE));
	putadif(RST_SENT, rec.getField(RST_SENT));
	putadif(RST_RCVD, rec.getField(RST_RCVD));
	putadif(TX_PWR, rec.getField(TX_PWR));
	putadif(NAME, rec.getField(NAME));
	putadif(QTH, rec.getField(QTH));
	putadif(STATE, rec.getField(STATE));
	putadif(VE_PROV, rec.getField(VE_PROV));
	putadif(COUNTRY, rec.getField(COUNTRY));
	putadif(GRIDSQUARE, rec.getField(GRIDSQUARE));
	putadif(STX, rec.getField(STX));
	putadif(SRX, rec.getField(SRX));
	putadif(XCHG1, rec.getField(XCHG1));
	putadif(MYXCHG, rec.getField(MYXCHG));
	notes = rec.getField(NOTES);
	for (size_t i = 0; i < notes.length(); i++)
	    if (notes[i] == '\n') notes[i] = ';';
	putadif(NOTES, notes.c_str());
// these fields will always be blank unless they are added to the main
// QSO log area.
	putadif(IOTA, rec.getField(IOTA));
	putadif(DXCC, rec.getField(DXCC));
	putadif(QSLRDATE, rec.getField(QSLRDATE));
	putadif(QSLSDATE, rec.getField(QSLSDATE));

	writeADIF();
}

#endif

//---------------------------------------------------------------------
// the following IPC message is compatible with xlog remote data spec.
//---------------------------------------------------------------------

#if !defined(__WOE32__) && !defined(__APPLE__)

#define addtomsg(x, y)	log_msg = log_msg + (x) + (y) + LOG_MSEPARATOR

static void send_IPC_log(cQsoRec &rec)
{
	msgtype msgbuf;
	const char   LOG_MSEPARATOR[2] = {1,0};
	int msqid, len;
	
	log_msg = "";
	log_msg = log_msg + "program:"	+ PACKAGE_NAME + " v " 	+ PACKAGE_VERSION + LOG_MSEPARATOR;
	addtomsg("version:",	LOG_MVERSION);
	addtomsg("date:",		rec.getField(QSO_DATE_OFF));
	addtomsg("time:", 		rec.getField(TIME_ON));
	addtomsg("endtime:", 	rec.getField(TIME_OFF));
	addtomsg("call:",		rec.getField(CALL));
	addtomsg("mhz:",		rec.getField(FREQ));
	addtomsg("mode:",		rec.getField(MODE));
	addtomsg("tx:",			rec.getField(RST_SENT));
	addtomsg("rx:",			rec.getField(RST_RCVD));
	addtomsg("name:",		rec.getField(NAME));
	addtomsg("qth:",		rec.getField(QTH));
	addtomsg("state:",		rec.getField(STATE));
	addtomsg("province:",	rec.getField(VE_PROV));
	addtomsg("country:",	rec.getField(COUNTRY));
	addtomsg("locator:",	rec.getField(GRIDSQUARE));
	addtomsg("serialout:",	rec.getField(STX));
	addtomsg("serialin:",	rec.getField(SRX));
	addtomsg("free1:",		rec.getField(XCHG1));
	notes = rec.getField(NOTES);
	for (size_t i = 0; i < notes.length(); i++)
	    if (notes[i] == '\n') notes[i] = ';';
	addtomsg("notes:",		notes.c_str());
	addtomsg("power:",		rec.getField(TX_PWR));
	
	strncpy(msgbuf.mtext, log_msg.c_str(), sizeof(msgbuf.mtext));
	msgbuf.mtext[sizeof(msgbuf.mtext) - 1] = '\0';

	if ((msqid = msgget(LOG_MKEY, 0666 | IPC_CREAT)) == -1) {
		LOG_PERROR("msgget");
		return;
	}
	msgbuf.mtype = LOG_MTYPE;
	len = strlen(msgbuf.mtext) + 1;
	if (msgsnd(msqid, &msgbuf, len, IPC_NOWAIT) < 0)
		LOG_PERROR("msgsnd");
}

#endif

void submit_record(cQsoRec &rec)
{
#if !defined(__WOE32__) && !defined(__APPLE__)
	send_IPC_log(rec);
#else
	submit_ADIF(rec);
#endif
}

//---------------------------------------------------------------------

extern void xml_add_record();

void submit_log(void)
{
	if (progStatus.spot_log)
		spot_log(inpCall->value(), inpLoc->value());

	logmode = mode_info[active_modem->get_mode()].adif_name;

	if (progStatus.xml_logbook)
		xml_add_record();
	else
		AddRecord();

}

