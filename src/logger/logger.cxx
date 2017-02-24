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
#include <fstream>

#include "icons.h"
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
#include "date.h"
#include "configuration.h"

#include "logsupport.h"
#include "lookupcall.h"
#include "fd_logger.h"

#include "n3fjp_logger.h"

#include "dx_cluster.h"

#include <FL/fl_ask.H>

using namespace std;

//---------------------------------------------------------------------
const char *logmode;

static string log_msg;
static string errmsg;
static string notes;

//======================================================================
// LoTW
static string lotw_fname;
static string lotw_logfname;
static string lotw_send_fname;
static string lotw_log_fname;
static int logcheck_count = 0;

string str_lotw;
//======================================================================

static string adif;

void writeADIF () {
// open the adif file
	FILE *adiFile;
	string fname;

    fname = TempDir;
    fname.append("log.adif");
	adiFile = fl_fopen (fname.c_str(), "a");
	if (adiFile) {
// write the current record to the file  
		fprintf(adiFile,"%s<EOR>\n", adif.c_str());
		fclose (adiFile);
	}
}

void putadif(int num, const char *s, string &str = adif)
{
	char tempstr[100];
	int slen = strlen(s);
	int n = snprintf(tempstr, sizeof(tempstr), "<%s:%d>", fields[num].name, slen);
	if (n == -1) {
		LOG_PERROR("snprintf");
		return;
	}
	str.append(tempstr).append(s);
}

void check_lotw_log(void *)
{
	string logtxt;
	FILE * logfile = fopen(lotw_log_fname.c_str(), "r");
	if (!logfile) {
		logcheck_count++;
		if (logcheck_count < 20) {
			Fl::repeat_timeout(1.0, check_lotw_log);
			return;
		}
		LOG_ERROR("%s", "NO tqsl log file in 20 seconds!");
		return;
	}
	char c = fgetc(logfile);
	while (!feof(logfile)) {
		logtxt += c;
		c = fgetc(logfile);
		if (c == EOF) break;
	}
	fclose(logfile);

	size_t p = logtxt.find("UploadFile returns 0");
	if (p != string::npos) {
		if (progdefaults.lotw_quiet_mode) fl_alert2("LoTW upload OK");
	} else {
		string errlog = LoTWDir;
		errlog.append("lotw_error_log.txt");
		ofstream errfile(errlog.c_str());
		if (errfile) {
			errfile << logtxt;
			errfile.close();
			logtxt.assign("LoTW upload errors\nCheck file ");
			logtxt.append(errlog);
		}
		if (progdefaults.lotw_quiet_mode) fl_alert2("LoTW upload Failed\nView LoTW error log:\n%s", errlog.c_str());
	}
	remove(lotw_log_fname.c_str());
}

void send_to_lotw(void *)
{
	if (progdefaults.lotw_pathname.empty())
		return;
	if (str_lotw.empty()) return;

	lotw_send_fname = LoTWDir;
	lotw_send_fname.append("fldigi_lotw_").append(zdate());
	lotw_send_fname.append("_").append(ztime());
	lotw_send_fname.append(".adi");

	ofstream outfile(lotw_send_fname.c_str());
	outfile << str_lotw;
	outfile.close();

	str_lotw.clear();

	lotw_log_fname = LoTWDir;
	lotw_log_fname.append("lotw_log.txt");

	string pstring;
	pstring.assign("\"").append(progdefaults.lotw_pathname).append("\"");
	pstring.append(" -d -u -a compliant");

	if (progdefaults.lotw_quiet_mode)
		pstring.append(" -q");

	if (progdefaults.submit_lotw_password)
		pstring.append(" -p \"").append(progdefaults.lotw_pwd).append("\"");

	if (!progdefaults.lotw_location.empty())
		pstring.append(" -l \"").append(progdefaults.lotw_location).append("\"");

	pstring.append(" -t \"").append(lotw_log_fname).append("\"");

	pstring.append(" \"").append(lotw_send_fname).append("\"");

	start_process(pstring);

	logcheck_count = 5;
	Fl::add_timeout(5.0, check_lotw_log);
}

string lotw_rec(cQsoRec &rec)
{
	static string valid_lotw_modes =
	"\
AM AMTORFEC ASCI ATV CHIP64 CHIP128 CLO CONTESTI CW DSTAR DOMINO DOMINOF FAX \
FM FMHELL FSK31 FSK441 GTOR HELL HELL80 HFSK JT44 JT4A JT4B JT4C JT4D JT4E \
JT4F JT4G JT65 JT65A JT65B JT65C JT6M JT9 MFSK8 MFSK16 \
MT63 OLIVIA PAC PAC2 PAC3 PAX PAX2 PCW PKT PSK10 PSK31 PSK63 PSK63F PSK125 \
PSKAM10 PSKAM31 PSKAM50 PSKFEC31 PSKHELL Q15 QPSK31 QPSK63 QPSK125 \
ROS RTTY RTTYM SSB SSTV THRB THOR THRBX TOR VOI WINMOR WSPR ";
	static string mfsk_modes = "\
MFSK4 MFSK11 MFSK22 MFSK31 MFSK32 MFSK64 MFSK128 MFSK64L MFSK128L";

	string temp;
	string strrec;
	putadif(CALL, rec.getField(CALL), strrec);

//	temp = mode_info[active_modem->get_mode()].adif_name;
	temp = rec.getField(MODE);

	if (temp.find("MFSK") != string::npos)
		if (mfsk_modes.find(temp) != string::npos)
			temp = "MFSK16";

// LoTW will probably never catch up with the active modes
// default all others to PSK31
	if (valid_lotw_modes.find(temp) == string::npos) {
		temp = "PSK31";
	}

	putadif(MODE, temp.c_str(), strrec);

	temp = rec.getField(FREQ);
	temp.resize(7);
	putadif(FREQ, temp.c_str(), strrec);

	putadif(QSO_DATE, rec.getField(QSO_DATE), strrec);

	temp = rec.getField(TIME_ON);
	if (temp.length() > 4) temp.erase(4);
	putadif(TIME_ON, temp.c_str(), strrec);

	strrec.append("<EOR>\n");

	return strrec;
}

void submit_lotw(cQsoRec &rec)
{
	string adifrec = lotw_rec(rec);

	if (adifrec.empty()) return;

	if (str_lotw.empty())
		str_lotw = "Fldigi LoTW upload file\n<ADIF_VER:5>2.2.7\n<EOH>\n";
	str_lotw.append(adifrec);

	if (progdefaults.submit_lotw) Fl::add_timeout(2.0, send_to_lotw);
}

void submit_ADIF(cQsoRec &rec)
{
	adif.erase();
	
	putadif(QSO_DATE, rec.getField(QSO_DATE));
	putadif(TIME_ON, rec.getField(TIME_ON));
	putadif(QSO_DATE_OFF, rec.getField(QSO_DATE_OFF));
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
	putadif(QSL_VIA, rec.getField(QSL_VIA));
	putadif(QSLRDATE, rec.getField(QSLRDATE));
	putadif(QSLSDATE, rec.getField(QSLSDATE));

	writeADIF();
}

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

	int mm, dd, yyyy;
	char szdate[9];
	char sztime[5];
	strncpy(szdate, rec.getField(QSO_DATE_OFF), 8);
	szdate[8] = 0;
	sscanf(&szdate[6], "%d", &dd); szdate[6] = 0;
	sscanf(&szdate[4], "%d", &mm); szdate[4] = 0;
	sscanf(szdate, "%d", &yyyy);
	Date logdate(mm, dd, yyyy);

	log_msg.clear();
	log_msg = string("program:") + PACKAGE_NAME + string(" v ") + PACKAGE_VERSION + LOG_MSEPARATOR;
	addtomsg("version:",	LOG_MVERSION);
	addtomsg("date:",		logdate.szDate(5));
	memset(sztime, 0, sizeof(sztime) / sizeof(char));
	strncpy(sztime, rec.getField(TIME_ON), (sizeof(sztime) / sizeof(char)) - 1);
	addtomsg("TIME:",		sztime);
	memset(sztime, 0, 5);
	strncpy(sztime, rec.getField(TIME_OFF), 4);
	addtomsg("endtime:",            sztime);
	addtomsg("call:",		rec.getField(CALL));
	addtomsg("mhz:",		rec.getField(FREQ));
	addtomsg("mode:",		rec.getField(MODE));
	addtomsg("tx:",			rec.getField(RST_SENT));
	addtomsg("rx:",			rec.getField(RST_RCVD));
	addtomsg("name:",		rec.getField(NAME));
	addtomsg("qth:",		rec.getField(QTH));
	addtomsg("state:",		rec.getField(STATE));
	addtomsg("province:",	        rec.getField(VE_PROV));
	addtomsg("country:",	        rec.getField(COUNTRY));
	addtomsg("locator:",	        rec.getField(GRIDSQUARE));
	addtomsg("serialout:",	        rec.getField(STX));
	addtomsg("serialin:",	        rec.getField(SRX));
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
	submit_lotw(rec);
	n3fjp_add_record(rec);
}

//---------------------------------------------------------------------

extern void xml_add_record();

void submit_log(void)
{
	if (progdefaults.spot_when_logged) {
		if (!dxcluster_viewer->visible()) dxcluster_viewer->show();
		send_DXcluster_spot();
	}

	if (progStatus.spot_log)
		spot_log(inpCall->value(), inpLoc->value());

	logmode = mode_info[active_modem->get_mode()].adif_name;

	if (progdefaults.eqsl_when_logged)
		makeEQSL("");

	if (progdefaults.xml_logbook)
		xml_add_record();
	else
		AddRecord();

	if (FD_logged_on) FD_add_record();
}

