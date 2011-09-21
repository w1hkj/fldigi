// ----------------------------------------------------------------------------
// macros.cxx
//
// Copyright (C) 2007-2010
//		Dave Freese, W1HKJ
// Copyright (C) 2008-2010
//		Stelios Bounanos, M0GLD
// Copyright (C) 2009
//		Chris Sylvain, KB3CS
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

#include "macros.h"

#include "gettext.h"
#include "main.h"

#include "fl_digi.h"
#include "configuration.h"
#include "confdialog.h"
#include "logger.h"
#include "newinstall.h"
#include "globals.h"
#include "debug.h"
#include "status.h"
#include "trx.h"
#include "modem.h"
#include "qrunner.h"
#include "waterfall.h"
#include "rigsupport.h"

#include <FL/Fl.H>
#include <FL/filename.H>
#include "fileselect.h"

#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <string>
#include <fstream>
#include <queue>

#ifdef __WIN32__
#include "speak.h"
#endif

using namespace std;

struct CMDS { string cmd; void (*fp)(string); };
queue<CMDS> cmds;

// following used for debugging and development
//void pushcmd(CMDS cmd)
//{
//	LOG_INFO("%s, # = %d", cmd.cmd.c_str(), (int)cmds.size());
//	cmds.push(cmd);
//}
#define pushcmd(a) cmds.push((a))

MACROTEXT macros;
CONTESTCNTR contest_count;
static bool TransmitON = false;
static bool ToggleTXRX = false;
static int mNbr;

std::string qso_time = "";
std::string qso_exchange = "";

std::string exec_date = "";
std::string exec_time = "";
std::string exec_string = "";

std::string text2send = "";
std::string text2repeat = "";
//std::string text2save = "";
std::string info1msg = "";
std::string info2msg = "";

size_t repeatchar = 0;
static size_t  xbeg = 0, xend = 0;

static bool save_xchg;
static bool expand;
static bool GET = false;
static bool timed_exec = false;

static char cutnumbers[] = "T12345678N";
static string cutstr;

static string cutstring(const char *s)
{
	cutstr = s;
	if (!progdefaults.cutnbrs || active_modem != cw_modem)
		return cutstr;

	for (size_t i = 0; i < cutstr.length(); i++)
		if (cutstr[i] >= '0' && cutstr[i] <= '9')
			cutstr[i] = cutnumbers[cutstr[i] - '0'];
	return cutstr;

}

static size_t mystrftime( char *s, size_t max, const char *fmt, const struct tm *tm) {
	return strftime(s, max, fmt, tm);
}

static void pFILE(string &s, size_t &i, size_t endbracket)
{
	string fname = s.substr(i+6, endbracket - i - 6);
	if (fname.length() > 0) {
		FILE *toadd = fopen(fname.c_str(), "r");
		if (toadd) {
			string buffer;
			char c = getc(toadd);
			while (c && !feof(toadd)) {
				if (c != '\r') buffer += c; // damn MSDOS txt files
				c = getc(toadd);
				}
			s.replace(i, endbracket - i + 1, buffer);
			fclose(toadd);
		} else {
			LOG_WARN("%s not found", fname.c_str());
			s.replace(i, endbracket - i + 1, "");
		}
	} else
		s.replace(i, endbracket - i + 1, "");
}

static void pTIMER(string &s, size_t &i, size_t endbracket)
{
	int number;
	string sTime = s.substr(i+7, endbracket - i - 7);
	if (sTime.length() > 0) {
		sscanf(sTime.c_str(), "%d", &number);
		progStatus.timer = number;
		progStatus.timerMacro = mNbr;
	}
	s.replace(i, endbracket - i + 1, "");
}

static void pREPEAT(string &s, size_t &i, size_t endbracket)
{
	progStatus.repeatMacro = mNbr;
	s.replace(i, endbracket - i + 1, "");
	text2repeat = s;
	repeatchar = 0;
	s.insert(i, "[REPEAT]");
}

static void pWPM(string &s, size_t &i, size_t endbracket)
{
	int number;
	string sTime = s.substr(i+5, endbracket - i - 5);
	if (sTime.length() > 0) {
		sscanf(sTime.c_str(), "%d", &number);
		if (number < 5) number = 5;
		if (number > 200) number = 200;
		progdefaults.CWspeed = number;
		sldrCWxmtWPM->value(number);
	}
	s.replace(i, endbracket - i + 1, "");
}

static void pRISETIME(string &s, size_t &i, size_t endbracket)
{
	float number;
	string sVal = s.substr(i+6, endbracket - i - 6);
	if (sVal.length() > 0) {
		sscanf(sVal.c_str(), "%f", &number);
		if (number < 0) number = 0;
		if (number > 20) number = 20;
		progdefaults.CWrisetime = number;
		cntCWrisetime->value(number);
	}
	s.replace(i, endbracket - i + 1, "");
}

static void pPRE(string &s, size_t &i, size_t endbracket)
{
	float number;
	string sVal = s.substr(i+5, endbracket - i - 5);
	if (sVal.length() > 0) {
		sscanf(sVal.c_str(), "%f", &number);
		if (number < 0) number = 0;
		if (number > 20) number = 20;
		progdefaults.CWpre = number;
		cntPreTiming->value(number);
	}
	s.replace(i, endbracket - i + 1, "");
}

static void pPOST(string &s, size_t &i, size_t endbracket)
{
	float number;
	string sVal = s.substr(i+6, endbracket - i - 6);
	if (sVal.length() > 0) {
		sscanf(sVal.c_str(), "%f", &number);
		if (number < -20) number = -20;
		if (number > 20) number = 20;
		progdefaults.CWpost = number;
		cntPostTiming->value(number);
	}
	s.replace(i, endbracket - i + 1, "");
}

static void setwpm(int d)
{
	sldrCWxmtWPM->value(d);
	cntCW_WPM->value(d);
}

static void doWPM(string s)
{
	int number;
	string sTime = s.substr(6);
	if (sTime.length() > 0) {
		sscanf(sTime.c_str(), "%d", &number);
		if (number < 5) number = 5;
		if (number > 200) number = 200;
		progdefaults.CWspeed = number;
		REQ(setwpm, number);
	}
}

static void pQueWPM(string &s, size_t &i, size_t endbracket)
{
	struct CMDS cmd = { s.substr(i, endbracket - i + 1), doWPM };
	pushcmd(cmd);
	s.replace(i, endbracket - i + 1, "^!");
}

static void setRISETIME(int d)
{
	cntCWrisetime->value(d);
}

static void doRISETIME(string s)
{
	float number;
	string sVal = s.substr(7, s.length() - 8);
	if (sVal.length() > 0) {
		sscanf(sVal.c_str(), "%f", &number);
		if (number < 0) number = 0;
		if (number > 20) number = 20;
		progdefaults.CWrisetime = number;
		REQ(setRISETIME, number);
	}
}

static void pQueRISETIME(string &s, size_t &i, size_t endbracket)
{
	struct CMDS cmd = { s.substr(i, endbracket - i + 1), doRISETIME };
	pushcmd(cmd);
	s.replace(i, endbracket - i + 1, "^!");
}

static void setPRE(int d)
{
	cntPreTiming->value(d);
}

static void doPRE(string s)
{
	float number;
	string sVal = s.substr(6, s.length() - 7);
	if (sVal.length() > 0) {
		sscanf(sVal.c_str(), "%f", &number);
		if (number < 0) number = 0;
		if (number > 20) number = 20;
		progdefaults.CWpre = number;
		REQ(setPRE, number);
	}
}

static void pQuePRE(string &s, size_t &i, size_t endbracket)
{
	struct CMDS cmd = { s.substr(i, endbracket - i + 1), doPRE };
	pushcmd(cmd);
	s.replace(i, endbracket - i + 1, "^!");
}

static void setPOST(int d)
{
	cntPostTiming->value(d);
}

static void doPOST(string s)
{
	float number;
	string sVal = s.substr(7, s.length() - 8);
	if (sVal.length() > 0) {
		sscanf(sVal.c_str(), "%f", &number);
		if (number < -20) number = -20;
		if (number > 20) number = 20;
		progdefaults.CWpost = number;
		REQ(setPOST, number);
	}
}

static void pQuePOST(string &s, size_t &i, size_t endbracket)
{
	struct CMDS cmd = { s.substr(i, endbracket - i + 1), doPOST };
	pushcmd(cmd);
	s.replace(i, endbracket - i + 1, "^!");
}

bool macro_idle_on = false;
static float  idleTime = 0;

static void pIDLE(string &s, size_t &i, size_t endbracket)
{
	float number;
	string sTime = s.substr(i+6, endbracket - i - 6);
	if (sTime.length() > 0) {
		sscanf(sTime.c_str(), "%f", &number);
		macro_idle_on = true;
		idleTime = number;
	}
	s.replace(i, endbracket - i + 1, "");
}

static void doneIDLE(void *)
{
	Qidle_time = 0;
}

static void doIDLE(string s)
{
	float number;
	string sTime = s.substr(7, s.length() - 8);
	if (sTime.length() > 0) {
		sscanf(sTime.c_str(), "%f", &number);
		Qidle_time = 1;
		Fl::add_timeout(number, doneIDLE);
	} else {
		Qidle_time = 0;
	}
}

static void pQueIDLE(string &s, size_t &i, size_t endbracket)
{
	struct CMDS cmd = { s.substr(i, endbracket - i + 1), doIDLE };
	pushcmd(cmd);
	s.replace(i, endbracket - i + 1, "^!");
}

static bool useTune = false;
static int  tuneTime = 0;

static void pTUNE(string &s, size_t &i, size_t endbracket)
{
	int number;
	string sTime = s.substr(i+6, endbracket - i - 6);
	if (sTime.length() > 0) {
		sscanf(sTime.c_str(), "%d", &number);
		useTune = true;
		tuneTime = number;
	}
	s.replace(i, endbracket - i + 1, "");
}

static bool useWait = false;
static int  waitTime = 0;

static void pWAIT(string &s, size_t &i, size_t endbracket)
{
	int number;
	string sTime = s.substr(i+6, endbracket - i - 6);
	if (sTime.length() > 0) {
		sscanf(sTime.c_str(), "%d", &number);
		useWait = true;
		waitTime = number;
	}
	s.replace(i, endbracket - i + 1, "");
}

static void doneWAIT(void *)
{
	Qwait_time = 0;
	start_tx();
}

static void doWAIT(string s)
{
	int number;
	string sTime = s.substr(7, s.length() - 8);
	if (sTime.length() > 0) {
		sscanf(sTime.c_str(), "%d", &number);
		Qwait_time = number;
		Fl::add_timeout (number * 1.0, doneWAIT);
	} else
		Qwait_time = 0;
	que_ok = true;
}

static void pQueWAIT(string &s, size_t &i, size_t endbracket)
{
	struct CMDS cmd = { s.substr(i, endbracket - i + 1), doWAIT };
	pushcmd(cmd);
	s.replace(i, endbracket - i + 1, "^!");
}


static void pINFO1(string &s, size_t &i, size_t endbracket)
{
	s.replace( i, 7, info1msg );
}

static void pINFO2(string &s, size_t &i, size_t endbracket)
{
	s.replace( i, 7, info2msg );
}

static void pCLRRX(string &s, size_t &i, size_t endbracket)
{
	s.replace( i, 7, "" );
	ReceiveText->clear();
}

static void pCLRTX(string &s, size_t &i, size_t endbracket)
{
	s.replace( i, 7, "" );
	TransmitText->clear();
}

static void pCALL(string &s, size_t &i, size_t endbracket)
{
	s.replace( i, 6, inpCall->value() );
}

static void pGET(string &s, size_t &i, size_t endbracket)
{
	s.erase( i, 9 );
	GET = true;
}

static void pFREQ(string &s, size_t &i, size_t endbracket)
{
	s.replace( i, 6, inpFreq->value() );
}

static void pLOC(string &s, size_t &i, size_t endbracket)
{
	s.replace( i, 5, inpLoc->value() );
}

static void pMODE(string &s, size_t &i, size_t endbracket)
{
	s.replace( i, 6, active_modem->get_mode_name());
}

static void pNAME(string &s, size_t &i, size_t endbracket)
{
	s.replace( i, 6, inpName->value() );
}

static void pQTH(string &s, size_t &i, size_t endbracket)
{
	s.replace( i,5, inpQth->value() );
}

static void pQSOTIME(string &s, size_t &i, size_t endbracket)
{
	qso_time = inpTimeOff->value();
	s.replace( i, 9, qso_time.c_str() );
}

static void pRST(string &s, size_t &i, size_t endbracket)
{
	s.replace( i, 5, cutstring(inpRstOut->value()));
}

static void pMYCALL(string &s, size_t &i, size_t endbracket)
{
	s.replace( i, 8, inpMyCallsign->value() );
}

static void pMYLOC(string &s, size_t &i, size_t endbracket)
{
	s.replace( i, 7, inpMyLocator->value() );
}

static void pMYNAME(string &s, size_t &i, size_t endbracket)
{
	s.replace( i, 8, inpMyName->value() );
}

static void pMYQTH(string &s, size_t &i, size_t endbracket)
{
	s.replace( i, 7, inpMyQth->value() );
}

static void pMYRST(string &s, size_t &i, size_t endbracket)
{
	s.replace( i, 7, inpRstIn->value() );
}

static void pLDT(string &s, size_t &i, size_t endbracket)
{
	char szDt[80];
	time_t tmptr;
	tm sTime;
	time (&tmptr);
	localtime_r(&tmptr, &sTime);
	mystrftime(szDt, 79, "%x %H:%M %Z", &sTime);
	s.replace( i, 5, szDt);
}

static void pILDT(string &s, size_t &i, size_t endbracket)
{
	char szDt[80];
	time_t tmptr;
	tm sTime;
	time (&tmptr);
	localtime_r(&tmptr, &sTime);
	mystrftime(szDt, 79, "%Y-%m-%d %H:%M%z", &sTime);
	s.replace( i, 6, szDt);
}

static void pZDT(string &s, size_t &i, size_t endbracket)
{
	char szDt[80];
	time_t tmptr;
	tm sTime;
	time (&tmptr);
	gmtime_r(&tmptr, &sTime);
	mystrftime(szDt, 79, "%x %H:%MZ", &sTime);
	s.replace( i, 5, szDt);
}

static void pIZDT(string &s, size_t &i, size_t endbracket)
{
	char szDt[80];
	time_t tmptr;
	tm sTime;
	time (&tmptr);
	gmtime_r(&tmptr, &sTime);
	mystrftime(szDt, 79, "%Y-%m-%d %H:%MZ", &sTime);
	s.replace( i, 6, szDt);
}

static void pLT(string &s, size_t &i, size_t endbracket)
{
	char szDt[80];
	time_t tmptr;
	tm sTime;
	time (&tmptr);
	localtime_r(&tmptr, &sTime);
	mystrftime(szDt, 79, "%H%M", &sTime);
	s.replace( i, 4, szDt);
}

static void pZT(string &s, size_t &i, size_t endbracket)
{
	char szDt[80];
	time_t tmptr;
	tm sTime;
	time (&tmptr);
	gmtime_r(&tmptr, &sTime);
	mystrftime(szDt, 79, "%H%MZ", &sTime);
	s.replace( i, 4, szDt);
}

static void pLD(string &s, size_t &i, size_t endbracket)
{
	char szDt[80];
	time_t tmptr;
	tm sTime;
	time (&tmptr);
	localtime_r(&tmptr, &sTime);
	mystrftime(szDt, 79, "%Y-%m-%d", &sTime);
	s.replace( i, 4, szDt);
}

static void pZD(string &s, size_t &i, size_t endbracket)
{
	char szDt[80];
	time_t tmptr;
	tm sTime;
	time (&tmptr);
	gmtime_r(&tmptr, &sTime);
	mystrftime(szDt, 79, "%Y-%m-%d", &sTime);
	s.replace( i, 4, szDt);
}

static void pID(string &s, size_t &i, size_t endbracket)
{
	progdefaults.macroid = true;
	s.replace( i, 4, "");
}

static void pTEXT(string &s, size_t &i, size_t endbracket)
{
	progdefaults.macrotextid = true;
	s.replace( i, 6, "");
}

static void pCWID(string &s, size_t &i, size_t endbracket)
{
	progdefaults.macroCWid = true;
	s.replace( i, 6, "");
}

static void doDTMF(string s)
{
	progdefaults.DTMFstr = s.substr(6, s.length() - 7);
}

static void pDTMF(string &s, size_t &i, size_t endbracket)
{
	CMDS cmd = {s.substr(i, endbracket - i + 1), doDTMF};
	pushcmd(cmd);
	s.replace(i, endbracket - i + 1, "^!");
}

static void pRX(string &s, size_t &i, size_t endbracket)
{
	s.replace (i, 4, "^r");
}

static void pTX(string &s, size_t &i, size_t endbracket)
{
	s.erase(i, 4);
	TransmitON = true;
}

static void pTXRX(string &s, size_t &i, size_t endbracket)
{
	s.erase(i, 7);
	ToggleTXRX = true;
}


static void pVER(string &s, size_t &i, size_t endbracket)
{
	string progname;
	progname = "Fldigi ";
	progname.append(PACKAGE_VERSION);
	s.replace( i, 5, progname );
}

static void pCNTR(string &s, size_t &i, size_t endbracket)
{
	int  contestval;
	contestval = contest_count.count;
	if (contestval) {
		contest_count.Format(progdefaults.ContestDigits, progdefaults.UseLeadingZeros);
		snprintf(contest_count.szCount, sizeof(contest_count.szCount), contest_count.fmt.c_str(), contestval);
		s.replace (i, 6, cutstring(contest_count.szCount));
	} else
		s.replace (i, 6, "");
}

static void pDECR(string &s, size_t &i, size_t endbracket)
{
	int  contestval;
	contest_count.count--;
	if (contest_count.count < 0) contest_count.count = 0;
	contestval = contest_count.count;
	s.replace (i, 6, "");
	updateOutSerNo();
}

static void pINCR(string &s, size_t &i, size_t endbracket)
{
	int  contestval;
	contest_count.count++;
	contestval = contest_count.count;
	s.replace (i, 6, "");
	updateOutSerNo();
}

static void pXIN(string &s, size_t &i, size_t endbracket)
{
	s.replace( i, 5, inpXchgIn->value() );
}

static void pXOUT(string &s, size_t &i, size_t endbracket)
{
	s.replace( i, 6, cutstring(progdefaults.myXchg.c_str()));
}

static void pXBEG(string &s, size_t &i, size_t endbracket)
{
	s.replace( i, 6, "");
	xbeg = i;
}

static void pXEND(string &s, size_t &i, size_t endbracket)
{
	s.replace( i, 6, "");
	xend = i;
}

static void pSAVEXCHG(string &s, size_t &i, size_t endbracket)
{
	save_xchg = true;
	s.replace( i, 10, "");
}

static void pLOG(string &s, size_t &i, size_t endbracket)
{
	qsoSave_cb(0, 0);
	s.replace(i, 5, "");
}

static void pLNW(string &s, size_t &i, size_t endbracket)
{
	s.replace(i, 5, "^L");
}

static void pCLRLOG(string &s, size_t &i, size_t endbracket)
{
	s.replace(i, 10, "^C");
}

static void pMODEM_compSKED(string &s, size_t &i, size_t endbracket)
{
	size_t	j, k,
			len = s.length();
	string name;

	if ((j = s.find('>', i)) == string::npos)
		return;
	while (++j < len)
	    if (!isspace(s[j])) break;
	k = j;
	while (++k < len)
	    if (isspace(s[k])  || s[k] == '<') break;
	name = s.substr(j, k - j);
	for (int m = 0; m < NUM_MODES; m++) {
		if (name == mode_info[m].sname) {
			if (active_modem->get_mode() != mode_info[m].mode)
				init_modem(mode_info[m].mode);
			break;
		}
	}
	s.erase(i, k-i);
}

#include <float.h>
#include "re.h"

static void doMODEM(string s)
{
	static fre_t re("<!MODEM:([[:alnum:]-]+)((:[[:digit:].+-]*)*)>", REG_EXTENDED);
	string tomatch = s;

	if (!re.match(tomatch.c_str())) {
		que_ok = true;
		return;
	}

	const std::vector<regmatch_t>& o = re.suboff();
	string name = tomatch.substr(o[1].rm_so, o[1].rm_eo - o[1].rm_so);
	trx_mode m;
	for (m = 0; m < NUM_MODES; m++)
		if (name == mode_info[m].sname)
			break;
	// do we have arguments and a valid modem?
	if (o.size() == 2 || m == NUM_MODES) {
		que_ok = true;
		return;
	}

	// parse arguments
	vector<double> args;
	args.reserve(8);
	char* end;
	double d;
	for (const char* p = s.c_str() + o[2].rm_so + 1; *p; p++) {
		errno = 0;
		d = strtod(p, &end);
		if (!errno && p != end) {
			args.push_back(d);
			p = end;
		}
		else // push an invalid value
			args.push_back(DBL_MIN);
	}

	try {
		switch (m) {
		case MODE_RTTY: // carrier shift, baud rate, bits per char
			if (args.at(0) != DBL_MIN)
				set_rtty_shift((int)args[0]);
			if (args.at(1) != DBL_MIN)
				set_rtty_baud((float)args[1]);
			if (args.at(2) != DBL_MIN)
				set_rtty_bits((int)args[2]);
			break;
		case MODE_CONTESTIA: // bandwidth, tones
			if (args.at(0) != DBL_MIN)
				set_contestia_bw((int)args[0]);
			if (args.at(1) != DBL_MIN)
				set_contestia_tones((int)args[1]);
			break;
		case MODE_OLIVIA: // bandwidth, tones
			if (args.at(0) != DBL_MIN)
				set_olivia_bw((int)args[0]);
			if (args.at(1) != DBL_MIN)
				set_olivia_tones((int)args[1]);
			break;
		default:
			break;
		}
	}
	catch (const exception& e) { }

	if (active_modem->get_mode() != mode_info[m].mode)
		init_modem_sync(mode_info[m].mode);
	que_ok = true;
}

static void pQueMODEM(string &s, size_t &i, size_t endbracket)
{
	struct CMDS cmd = { s.substr(i, endbracket - i + 1), doMODEM };
	pushcmd(cmd);
	s.replace(i, endbracket - i + 1, "^!");
}

static void pMODEM(string &s, size_t &i, size_t endbracket)
{
	static fre_t re("<MODEM:([[:alnum:]-]+)((:[[:digit:].+-]*)*)>", REG_EXTENDED);

	if (!re.match(s.c_str() + i)) {
		size_t end = s.find('>', i);
		if (end != string::npos)
			s.erase(i, end - i);
		return;
	}

	const std::vector<regmatch_t>& o = re.suboff();
	string name = s.substr(o[1].rm_so, o[1].rm_eo - o[1].rm_so);
	trx_mode m;
	for (m = 0; m < NUM_MODES; m++)
		if (name == mode_info[m].sname)
			break;
	// do we have arguments and a valid modem?
	if (o.size() == 2 || m == NUM_MODES) {
		if (m < NUM_MODES && active_modem->get_mode() != mode_info[m].mode)
			init_modem(mode_info[m].mode);
		s.erase(i, o[0].rm_eo - i);
		return;
	}

	// parse arguments
	vector<double> args;
	args.reserve(8);
	char* end;
	double d;
	for (const char* p = s.c_str() + o[2].rm_so + 1; *p; p++) {
		errno = 0;
		d = strtod(p, &end);
		if (!errno && p != end) {
			args.push_back(d);
			p = end;
		}
		else // push an invalid value
			args.push_back(DBL_MIN);
	}

	try {
		switch (m) {
		case MODE_RTTY: // carrier shift, baud rate, bits per char
			if (args.at(0) != DBL_MIN)
				set_rtty_shift((int)args[0]);
			if (args.at(1) != DBL_MIN)
				set_rtty_baud((float)args[1]);
			if (args.at(2) != DBL_MIN)
				set_rtty_bits((int)args[2]);
			break;
		case MODE_CONTESTIA: // bandwidth, tones
			if (args.at(0) != DBL_MIN)
				set_contestia_bw((int)args[0]);
			if (args.at(1) != DBL_MIN)
				set_contestia_tones((int)args[1]);
			break;
		case MODE_OLIVIA: // bandwidth, tones
			if (args.at(0) != DBL_MIN)
				set_olivia_bw((int)args[0]);
			if (args.at(1) != DBL_MIN)
				set_olivia_tones((int)args[1]);
			break;
		default:
			break;
		}
	}
	catch (const exception& e) { }

	if (active_modem->get_mode() != mode_info[m].mode) {
		init_modem(mode_info[m].mode);
		int count = 100;
		while ((active_modem->get_mode() != mode_info[m].mode) && --count)
			MilliSleep(10);
	}

	s.erase(i, o[0].rm_eo - i);
}

static void pAFC(string &s, size_t &i, size_t endbracket)
{
  string sVal = s.substr(i+5, endbracket - i - 5);
  if (sVal.length() > 0) {
    // sVal = on|off|t   [ON, OFF or Toggle]
    if (sVal.compare(0,2,"on") == 0)
      btnAFC->value(1);
    else if (sVal.compare(0,3,"off") == 0)
      btnAFC->value(0);
    else if (sVal.compare(0,1,"t") == 0)
      btnAFC->value(!btnAFC->value());

    btnAFC->do_callback();
  }
//pushcmd(s.substr(i, endbracket - i + 1));
//s.replace(i, endbracket - i + 1, "^!");
  s.replace(i, endbracket - i + 1, "");
}

static void pLOCK(string &s, size_t &i, size_t endbracket)
{
  string sVal = s.substr(i+6, endbracket - i - 6);
  if (sVal.length() > 0) {
    // sVal = on|off|t   [ON, OFF or Toggle]
    if (sVal.compare(0,2,"on") == 0)
      wf->xmtlock->value(1);
    else if (sVal.compare(0,3,"off") == 0)
      wf->xmtlock->value(0);
    else if (sVal.compare(0,1,"t") == 0)
      wf->xmtlock->value(!wf->xmtlock->value());

    wf->xmtlock->damage();
    wf->xmtlock->do_callback();
  }
//pushcmd(s.substr(i, endbracket - i + 1));
//s.replace(i, endbracket - i + 1, "^!");
  s.replace(i, endbracket - i + 1, "");
}

static void pTX_RSID(string &s, size_t &i, size_t endbracket)
{
  string sVal = s.substr(i+8, endbracket - i - 8);
  if (sVal.length() > 0) {
    // sVal = on|off|t   [ON, OFF or Toggle]
    if (sVal.compare(0,2,"on") == 0)
      btnTxRSID->value(1);
    else if (sVal.compare(0,3,"off") == 0)
      btnTxRSID->value(0);
    else if (sVal.compare(0,1,"t") == 0)
      btnTxRSID->value(!btnTxRSID->value());

    btnTxRSID->do_callback();
  }
//pushcmd(s.substr(i, endbracket - i + 1));
//s.replace(i, endbracket - i + 1, "^!");
  s.replace(i, endbracket - i + 1, "");
}

static void pRX_RSID(string &s, size_t &i, size_t endbracket)
{
  string sVal = s.substr(i+8, endbracket - i - 8);
  if (sVal.length() > 0) {
    // sVal = on|off|t   [ON, OFF or Toggle]
    if (sVal.compare(0,2,"on") == 0)
      btnRSID->value(1);
    else if (sVal.compare(0,3,"off") == 0)
      btnRSID->value(0);
    else if (sVal.compare(0,1,"t") == 0)
      btnRSID->value(!btnRSID->value());

    btnRSID->do_callback();
  }
  s.replace(i, endbracket - i + 1, "");
}

#ifdef __WIN32__
static void pTALK(string &s, size_t &i, size_t endbracket)
{
  string sVal = s.substr(i+6, endbracket - i - 6);
  if (sVal.length() > 0) {
    // sVal = on|off   [ON, OFF]
    if (sVal.compare(0,2,"on") == 0)
		open_talker();
    else if (sVal.compare(0,3,"off") == 0)
		close_talker();
    else if (sVal.compare(0,1,"t") == 0)
		toggle_talker();
  }
  s.replace(i, endbracket - i + 1, "");
}
#endif

static void pSRCHUP(string &s, size_t &i, size_t endbracket)
{
	s.replace( i, 8, "");
	active_modem->searchUp();
	if (progdefaults.WaterfallClickInsert)
	        wf->insert_text(true);
}

static void pSRCHDN(string &s, size_t &i, size_t endbracket)
{
	s.replace( i, 8, "");
	active_modem->searchDown();
	if (progdefaults.WaterfallClickInsert)
	         wf->insert_text(true);
}

static void pGOHOME(string &s, size_t &i, size_t endbracket)
{
	s.replace( i, 8, "");
	if (active_modem == cw_modem)
		active_modem->set_freq(progdefaults.CWsweetspot);
	else if (active_modem == rtty_modem)
		active_modem->set_freq(progdefaults.RTTYsweetspot);
	else
		active_modem->set_freq(progdefaults.PSKsweetspot);
}

static void doGOHOME(string s)
{
	if (active_modem == cw_modem)
		active_modem->set_freq(progdefaults.CWsweetspot);
	else if (active_modem == rtty_modem)
		active_modem->set_freq(progdefaults.RTTYsweetspot);
	else
		active_modem->set_freq(progdefaults.PSKsweetspot);
	que_ok = true;
}

static void pQueGOHOME(string &s, size_t &i, size_t endbracket)
{
	struct CMDS cmd = { s.substr(i, endbracket - i + 1), doGOHOME };
	pushcmd(cmd);
	s.replace(i, endbracket - i + 1, "^!");
}

static void pGOFREQ(string &s, size_t &i, size_t endbracket)
{
	int number;
	string sGoFreq = s.substr(i+8, endbracket - i - 8);
	if (sGoFreq.length() > 0) {
		sscanf(sGoFreq.c_str(), "%d", &number);
		if (number < progdefaults.LowFreqCutoff)
			number = progdefaults.LowFreqCutoff;
		if (number > progdefaults.HighFreqCutoff)
			number = progdefaults.HighFreqCutoff;
		active_modem->set_freq(number);
	}
	s.replace(i, endbracket - i + 1, "");
}

static void doGOFREQ(string s)
{
	int number;
	string sGoFreq = s.substr(9, s.length() - 10);
	if (sGoFreq.length() > 0) {
		sscanf(sGoFreq.c_str(), "%d", &number);
		if (number < progdefaults.LowFreqCutoff)
			number = progdefaults.LowFreqCutoff;
		if (number > progdefaults.HighFreqCutoff)
			number = progdefaults.HighFreqCutoff;
		active_modem->set_freq(number);
	}
	que_ok = true;
}

static void pQueGOFREQ(string &s, size_t &i, size_t endbracket)
{
	struct CMDS cmd = { s.substr(i, endbracket - i + 1), doGOFREQ };
	pushcmd(cmd);
	s.replace(i, endbracket - i + 1, "^!");
}

static void pQSYTO(string &s, size_t &i, size_t endbracket)
{
	s.replace( i, 7, "");
	do_qsy(true);
}

static void pQSYFM(string &s, size_t &i, size_t endbracket)
{
	s.replace( i, 7, "");
	do_qsy(false);
}

static void pQSY(string &s, size_t &i, size_t endbracket)
{
	int rf = 0;
	int audio = 0;
	float rfd = 0;
	string sGoFreq = s.substr(i+5, endbracket - i - 5);
	// no frequency(s) specified
	if (sGoFreq.length() == 0) {
		s.replace(i, endbracket-i+1, "");
		return;
	}
	// rf first value
	sscanf(sGoFreq.c_str(), "%f", &rfd);
	if (rfd > 0)
		rf = (int)(1000*rfd);
	size_t pos;
	if ((pos = sGoFreq.find(":")) != string::npos) {
		// af second value
		sGoFreq.erase(0, pos+1);
		if (sGoFreq.length())
			sscanf(sGoFreq.c_str(), "%d", &audio);
		if (audio < 0) audio = 0;
		if (audio < progdefaults.LowFreqCutoff)
			audio = progdefaults.LowFreqCutoff;
		if (audio > progdefaults.HighFreqCutoff)
			audio = progdefaults.HighFreqCutoff;
	}
	if (rf && rf != wf->rfcarrier())
		qsy(rf, audio);
	else
		active_modem->set_freq(audio);

	s.replace(i, endbracket - i + 1, "");
}

static void doQSY(string s)
{
	int rf = 0;
	int audio = 0;
	float rfd = 0;
	string sGoFreq;
	sGoFreq = s.substr(6, s.length() - 7);
	// no frequency(s) specified
	if (sGoFreq.length() == 0) {
		que_ok = true;
		return;
	}
	// rf first value
	sscanf(sGoFreq.c_str(), "%f", &rfd);
	if (rfd > 0)
		rf = (int)(1000*rfd);
	size_t pos;
	if ((pos = sGoFreq.find(":")) != string::npos) {
		// af second value
		sGoFreq.erase(0, pos+1);
		if (sGoFreq.length())
			sscanf(sGoFreq.c_str(), "%d", &audio);
		if (audio < 0) audio = 0;
		if (audio < progdefaults.LowFreqCutoff)
			audio = progdefaults.LowFreqCutoff;
		if (audio > progdefaults.HighFreqCutoff)
			audio = progdefaults.HighFreqCutoff;
	}
	if (rf && rf != wf->rfcarrier())
		qsy(rf, audio);
	else
		active_modem->set_freq(audio);
	que_ok = true;
}

static void pQueQSY(string &s, size_t &i, size_t endbracket)
{
	struct CMDS cmd = { s.substr(i, endbracket - i + 1), doQSY };
	pushcmd(cmd);
	s.replace(i, endbracket - i + 1, "^!");
}

static void pRIGMODE(string& s, size_t& i, size_t endbracket)
{
	string sMode = s.substr(i+9, endbracket - i - 9);
	qso_opMODE->value(sMode.c_str());
	cb_qso_opMODE();
	s.replace(i, endbracket - i + 1, "");
}

static void pFILWID(string& s, size_t& i, size_t endbracket)
{
	string sWidth = s.substr(i+8, endbracket - i - 8);
	qso_opBW->value(sWidth.c_str());
	cb_qso_opBW();
	s.replace(i, endbracket - i + 1, "");
}

void set_macro_env(void)
{
	enum {
#ifndef __WOE32__
	       pSKEDH, FLDIGI_RX_IPC_KEY, FLDIGI_TX_IPC_KEY,
#endif
	       FLDIGI_XMLRPC_ADDRESS, FLDIGI_XMLRPC_PORT,
	       FLDIGI_ARQ_ADDRESS, FLDIGI_ARQ_PORT,

	       FLDIGI_VERSION_ENVVAR, FLDIGI_PID, FLDIGI_CONFIG_DIR,

	       FLDIGI_MY_CALL, FLDIGI_MY_NAME, FLDIGI_MY_LOCATOR,

	       FLDIGI_MODEM, FLDIGI_MODEM_LONG_NAME, FLDIGI_DIAL_FREQUENCY,
	       FLDIGI_AUDIO_FREQUENCY, FLDIGI_FREQUENCY,

	       FLDIGI_LOG_FREQUENCY, FLDIGI_LOG_TIME_ON, FLDIGI_LOG_TIME_OFF, FLDIGI_LOG_CALL, FLDIGI_LOG_NAME,
	       FLDIGI_LOG_RST_IN, FLDIGI_LOG_RST_OUT, FLDIGI_LOG_QTH, FLDIGI_LOG_LOCATOR,
	       FLDIGI_LOG_NOTES, FLDIGI_AZ, ENV_SIZE
	};

	struct {
		const char* var;
		const char* val;
	} env[] = {
#ifndef __WOE32__
		{ "pSKEDH", "" },
		{ "FLDIGI_RX_IPC_KEY", "" },
		{ "FLDIGI_TX_IPC_KEY", "" },
#endif
		{ "FLDIGI_XMLRPC_ADDRESS", progdefaults.xmlrpc_address.c_str() },
		{ "FLDIGI_XMLRPC_PORT", progdefaults.xmlrpc_port.c_str() },
		{ "FLDIGI_ARQ_ADDRESS", progdefaults.arq_address.c_str() },
		{ "FLDIGI_ARQ_PORT", progdefaults.arq_port.c_str() },

		{ "FLDIGI_VERSION", PACKAGE_VERSION },
		{ "FLDIGI_PID", "" },
		{ "FLDIGI_CONFIG_DIR", HomeDir.c_str() },

		{ "FLDIGI_MY_CALL", progdefaults.myCall.c_str() },
		{ "FLDIGI_MY_NAME", progdefaults.myName.c_str() },
		{ "FLDIGI_MY_LOCATOR", progdefaults.myLocator.c_str() },

		{ "FLDIGI_MODEM", mode_info[active_modem->get_mode()].sname },
		{ "FLDIGI_MODEM_LONG_NAME", mode_info[active_modem->get_mode()].name },
		{ "FLDIGI_DIAL_FREQUENCY", "" },
		{ "FLDIGI_AUDIO_FREQUENCY", "" },
		{ "FLDIGI_FREQUENCY", "" },

		// logging frame
		{ "FLDIGI_LOG_FREQUENCY", inpFreq->value() },
		{ "FLDIGI_LOG_TIME_ON", inpTimeOn->value() },
		{ "FLDIGI_LOG_TIME_OFF", inpTimeOff->value() },
		{ "FLDIGI_LOG_CALL", inpCall->value() },
		{ "FLDIGI_LOG_NAME", inpName->value() },
		{ "FLDIGI_LOG_RST_IN", inpRstIn->value() },
		{ "FLDIGI_LOG_RST_OUT", inpRstOut->value() },
		{ "FLDIGI_LOG_QTH", inpQth->value() },
		{ "FLDIGI_LOG_LOCATOR", inpLoc->value() },
		{ "FLDIGI_LOG_NOTES", inpNotes->value() },
		{ "FLDIGI_AZ", inpAZ->value() }
	};

#ifndef __WOE32__
	// pSKEDH
	static string pSKEDh = ScriptsDir;
	pSKEDh.erase(pSKEDh.length()-1,1);
	const char* p;
	if ((p = getenv("pSKEDH")))
		pSKEDh.append(":").append(p);
	env[pSKEDH].val = pSKEDh.c_str();

	// IPC keys
        char key[2][8];
	snprintf(key[0], sizeof(key[0]), "%d", progdefaults.rx_msgid);
	env[FLDIGI_RX_IPC_KEY].val = key[0];
	snprintf(key[1], sizeof(key[1]), "%d", progdefaults.tx_msgid);
	env[FLDIGI_TX_IPC_KEY].val = key[1];
#endif

	// pid
	char pid[6];
	snprintf(pid, sizeof(pid), "%d", getpid());
	env[FLDIGI_PID].val = pid;

	// frequencies
	char dial_freq[20];
	snprintf(dial_freq, sizeof(dial_freq), "%lld", wf->rfcarrier());
	env[FLDIGI_DIAL_FREQUENCY].val = dial_freq;
	char audio_freq[6];
	snprintf(audio_freq, sizeof(audio_freq), "%d", active_modem->get_freq());
	env[FLDIGI_AUDIO_FREQUENCY].val = audio_freq;
	char freq[20];
	snprintf(freq, sizeof(freq), "%lld", wf->rfcarrier() + (wf->USB()
								? active_modem->get_freq()
								: -active_modem->get_freq()));
	env[FLDIGI_FREQUENCY].val = freq;

	// debugging vars
#if !defined(NDEBUG) && !defined(__WOE32__)
	unsetenv("FLDIGI_NO_EXEC");
	unsetenv("MALLOC_CHECK_");
	unsetenv("MALLOC_PERTURB_");
#endif

	for (size_t j = 0; j < ENV_SIZE; j++)
		setenv(env[j].var, env[j].val, 1);
}

#ifndef __MINGW32__
static void pEXEC(string &s, size_t &i, size_t endbracket)
{
	size_t start, end;
	if ((start = s.find('>', i)) == string::npos ||
	    (end = s.find("</EXEC>", start)) == string::npos) {
		i++;
		return;
	}
	start++;
	i++;

	int pfd[2];
	if (pipe(pfd) == -1) {
		LOG_PERROR("pipe");
		return;
	}
	pid_t pid;
	switch (pid = fork()) {
	case -1:
		LOG_PERROR("fork");
		return;
	case 0: // child
		close(pfd[0]);
		if (dup2(pfd[1], STDOUT_FILENO) != STDOUT_FILENO) {
			LOG_PERROR("dup2");
			exit(EXIT_FAILURE);
		}
		close(pfd[1]);
		set_macro_env();
		execl("/bin/sh", "sh", "-c", s.substr(start, end-start).c_str(), (char *)NULL);
		perror("execl");
		exit(EXIT_FAILURE);
	}
	// parent
	close(pfd[1]);
	FILE* fp = fdopen(pfd[0], "r");
	if (!fp) {
		LOG_PERROR("fdopen");
		close(pfd[0]);
		return;
	}

	start = --i;
	end = s.find('>', end) + 1;
	s.erase(start, end-start);
	char ln[BUFSIZ];
	while (fgets(ln, sizeof(ln), fp)) {
		end = strlen(ln);
		s.insert(start, ln, end);
		start += end;
	}
	// delete the trailing newline of what we read
	if (start > i && s[start - 1] == '\n')
		s.erase(start - 1, 1);

	fclose(fp);
	close(pfd[0]);

	// what should we do with the shell-generated text?
	// option 1: uncomment this line to skip & ignore it
	// i = start;
	// option 2: do nothing and allow it to be parsed for more macros
}
#else // !__MINGW32__

static void pEXEC(string& s, size_t& i, size_t endbracket)
{
	size_t start, end;
	if ((start = s.find('>', i)) == string::npos ||
	    (end = s.find("</EXEC>", start)) == string::npos) {
		i++;
		return;
	}
	start++;

	char* cmd = strdup(s.substr(start, end-start).c_str());
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	memset(&si, 0, sizeof(si));
	si.cb = sizeof(si);
	memset(&pi, 0, sizeof(pi));
	if (!CreateProcess(NULL, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
		LOG_ERROR("CreateProcess failed with error code %ld", GetLastError());
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	free(cmd);

	s.erase(i, end + strlen("</EXEC>") - i);
}
#endif // !__MINGW32__

static void MAPIT(int how)
{
	float lat = 0, lon = 0;
	string sCALL = inpCall->value();
	string sLOC = inpLoc->value();

	string url = "http://maps.google.com/maps?q=";

//	if (lookup_addr1.empty() && lookup_addr2.empty() &&
//		lookup_state.empty() && lookup_country.empty()) {
	if (how > 1 && !lookup_country.empty()) {
		url.append(lookup_addr1).append(",").append(lookup_addr2).append(",");
		url.append(lookup_state).append(",").append(lookup_country);
	} else {
		if (how > 0 && (!lookup_latd.empty() && !lookup_lond.empty())) {
			url.append(lookup_latd).append(",");
			url.append(lookup_lond);
		} else {
			if (sLOC.empty()) return;
			if (sLOC.length() < 4) return;
			if (sLOC.length() < 6) sLOC.append("aa");
			for (size_t i = 0; i < 6; i++) sLOC[i] = toupper(sLOC[i]);
			if (sLOC[0] -'A' > 17 || sLOC[4] - 'A' > 23 ||
				sLOC[1] -'A' > 17 || sLOC[5] - 'A' > 23 ||
				!isdigit(sLOC[2]) || !isdigit(sLOC[3])) return;
			lon =	-180.0 + 
					(sLOC[0] - 'A') * 20 +
					(sLOC[2] - '0') * 2 +
					(sLOC[4] - 'A' + 0.5) / 12;
			lat = -90.0 + 
					(sLOC[1] - 'A') * 10 +
					(sLOC[3] - '0') +
					(sLOC[5] - 'A' + 0.5) / 24;
			char sdata[20];
			snprintf(sdata, sizeof(sdata),"%10.6f", lat);
			url.append(sdata).append(",");
			snprintf(sdata, sizeof(sdata),"%10.6f", lon);
			url.append(sdata);
		}
	}
	if (!sCALL.empty()) url.append("(").append(sCALL).append(")");
	else url.append("(nocall)");
	url.append("&t=p&z=10");
	cb_mnuVisitURL(NULL, (void*)url.c_str());
}

static void pMAPIT(string &s, size_t &i, size_t endbracket)
{
	string sVal = s.substr(i + 7, endbracket - i - 7);
	if (sVal.length() > 0) {
		if (sVal.compare(0,3,"adr") == 0)
			REQ(MAPIT,2);
		else if (sVal.compare(0,6,"latlon") == 0)
			REQ(MAPIT,1);
		else if (sVal.compare(0,3,"loc") == 0)
			REQ(MAPIT,0);
		else
			REQ(MAPIT,2);
	} else
		REQ(MAPIT,2);
	s.erase(i, s.find('>', i) + 1 - i);
	expand = false;
}

static void pSTOP(string &s, size_t &i, size_t endbracket)
{
	s.erase(i, s.find('>', i) + 1 - i);
	expand = false;
}

static void pCONT(string &s, size_t &i, size_t endbracket)
{
	s.erase(i, s.find('>', i) + 1 - i);
	expand = true;
}

static void pSKED(string &s, size_t &i, size_t endbracket)
{
	string data = s.substr(i+6, endbracket - i - 6);
	size_t p = data.find(":");
	if (p == std::string::npos) {
		exec_date = zdate();
		exec_time = data;
		if (exec_time.empty()) exec_time = ztime();
	} else {
		exec_time = data.substr(0, p);
		exec_date = data.substr(p+1);
	}
	timed_exec = true;
	s.replace(i, endbracket - i + 1, "");
}

void queue_reset()
{
	if (!cmds.empty()) {
		Fl::remove_timeout(post_queue_execute);
		Fl::remove_timeout(queue_execute_after_rx);
		Fl::remove_timeout(doneIDLE);
		Fl::remove_timeout(doneWAIT);
		while (!cmds.empty()) cmds.pop();
	}
	Qwait_time = 0;
	Qidle_time = 0;
	que_ok = true;
}

void postQueue(string s)
{
	ReceiveText->add(s.c_str(), FTextBase::CTRL);
}

void queue_execute()
{
	if (cmds.empty()) {
		Qwait_time = 0;
		Qidle_time = 0;
		que_ok = true;
		return;
	}
	CMDS cmd = cmds.front();
	cmds.pop();
	cmd.fp(cmd.cmd);
	LOG_INFO("%s", cmd.cmd.c_str());
	REQ(postQueue, cmd.cmd.append("\n"));
	return;
}

bool queue_must_rx()
{
static string rxcmds = "<!MOD<!WAI<!GOH<!QSY<!GOF";
	if (cmds.empty()) return false;
	CMDS cmd = cmds.front();
	return (rxcmds.find(cmd.cmd.substr(0,5)) != string::npos);
}

struct MTAGS { const char *mTAG; void (*fp)(string &, size_t&, size_t );};

MTAGS mtags[] = {
{"<CALL>",		pCALL},
{"<FREQ>",		pFREQ},
{"<LOC>",		pLOC},
{"<MODE>",		pMODE},
{"<NAME>",		pNAME},
{"<QTH>",		pQTH},
{"<RST>",		pRST},
{"<MYCALL>",	pMYCALL},
{"<MYLOC>",		pMYLOC},
{"<MYNAME>",	pMYNAME},
{"<MYQTH>",		pMYQTH},
{"<MYRST>",		pMYRST},
{"<QSOTIME>",	pQSOTIME},
{"<INFO1>",		pINFO1},
{"<INFO2>",		pINFO2},
{"<LDT>",		pLDT},
{"<ILDT>",		pILDT},
{"<ZDT>",		pZDT},
{"<IZDT>",		pIZDT},
{"<LT>",		pLT},
{"<ZT>",		pZT},
{"<LD>",		pLD},
{"<ZD>",		pZD},
{"<ID>",		pID},
{"<TEXT>",		pTEXT},
{"<CWID>",		pCWID},
{"<RX>",		pRX},
{"<TX>",		pTX},
{"<TX/RX>",		pTXRX},
{"<VER>",		pVER},
{"<CNTR>",		pCNTR},
{"<DECR>",		pDECR},
{"<INCR>",		pINCR},
{"<X1>",		pXOUT},
{"<XIN>",		pXIN},
{"<XOUT>",		pXOUT},
{"<XBEG>",		pXBEG},
{"<XEND>",		pXEND},
{"<SAVEXCHG>",	pSAVEXCHG},
{"<LOG>",		pLOG},
{"<LNW>",		pLNW},
{"<CLRLOG>",	pCLRLOG},
{"<TIMER:",		pTIMER},
{"<IDLE:",		pIDLE},
{"<TUNE:",		pTUNE},
{"<WAIT:",		pWAIT},
{"<MODEM>",		pMODEM_compSKED},
{"<MODEM:",		pMODEM},
{"<EXEC>",		pEXEC},
{"<STOP>",		pSTOP},
{"<CONT>",		pCONT},
{"<GET>",		pGET},
{"<CLRRX>",		pCLRRX},
{"<CLRTX>",		pCLRTX},
{"<FILE:",		pFILE},
{"<WPM:",		pWPM},
{"<RISE:",		pRISETIME},
{"<PRE:",		pPRE},
{"<POST:",		pPOST},
{"<AFC:",		pAFC},
{"<LOCK:",		pLOCK},
{"<RXRSID:",	pRX_RSID},
{"<TXRSID:",	pTX_RSID},
{"<DTMF:",		pDTMF},
{"<SRCHUP>",	pSRCHUP},
{"<SRCHDN>",	pSRCHDN},
{"<GOHOME>",	pGOHOME},
{"<GOFREQ:",	pGOFREQ},
{"<QSY:",		pQSY},
{"<QSYTO>",		pQSYTO},
{"<QSYFM>",		pQSYFM},
{"<RIGMODE:",	pRIGMODE},
{"<FILWID:",	pFILWID},
{"<MAPIT:",		pMAPIT},
{"<MAPIT>",		pMAPIT},
{"<REPEAT>",	pREPEAT},
{"<SKED:",		pSKED},
#ifdef __WIN32__
{"<TALK:",		pTALK},
#endif
{"<!WPM:",		pQueWPM},
{"<!RISE:",		pQueRISETIME},
{"<!PRE:",		pQuePRE},
{"<!POST:",		pQuePOST},
{"<!GOHOME>",	pQueGOHOME},
{"<!GOFREQ:",	pQueGOFREQ},
{"<!QSY:",		pQueQSY},
{"<!IDLE:",		pQueIDLE},
{"<!WAIT:",		pQueWAIT},
{"<!MODEM:",	pQueMODEM},
{0, 0}
};

int MACROTEXT::loadMacros(const string& filename)
{
	string mLine;
	string mName;
	string mDef;
	bool   inMacro = false;
	int    mNumber = 0;
	unsigned long int	   crlf; // 64 bit cpu's
	char   szLine[4096];
	bool   convert = false;

	ifstream mFile(filename.c_str());

	if (!mFile) {
		create_new_macros();
	} else {

	mFile.getline(szLine, 4095);
	mLine = szLine;
	if (mLine.find("//fldigi macro definition file") != 0) {
		mFile.close();
		return -2;
	}
	if (mLine.find("extended") == string::npos) {
		convert = true;
		changed = true;
	}
// clear all of the macros
	for (int i = 0; i < MAXMACROS; i++) {
		name[i] = "";
		text[i] = "";
	}
	inMacro = false;
	while (!mFile.eof()) {
		mFile.getline(szLine,4095);
		mLine = szLine;
		if (mLine.find("//") == 0) // skip over all comment lines
			continue;
		if (mLine.find("/$") == 0) {
			int idx = mLine.find_first_not_of("0123456789", 3);
			sscanf((mLine.substr(3, idx - 3)).c_str(), "%d", &mNumber);
			if (mNumber < 0 || mNumber > (MAXMACROS - 1))
				break;
			if (convert && mNumber > 9) mNumber += 2;
            name[mNumber] = mLine.substr(idx+1);
			continue;
		}
		while ((crlf = mLine.find("\\n")) != string::npos) {
			mLine.erase(crlf, 2);
			mLine.append("\n");
		}
		text[mNumber] = text[mNumber] + mLine;
	}
	mFile.close();
}

	return 0;
}

void MACROTEXT::loadDefault()
{
	int erc;
	string Filename = MacrosDir;
	if (progdefaults.UseLastMacro == true)
		Filename.append(progStatus.LastMacroFile);
	else {
		Filename.append("macros.mdf");
		progStatus.LastMacroFile = "macros.mdf";
	}
	if ((erc = loadMacros(Filename)) != 0)
#ifndef __WOE32__
		LOG_ERROR("Error #%d loading %s\n", erc, Filename.c_str());
#else
	;
#endif
}

void MACROTEXT::openMacroFile()
{
	string deffilename = MacrosDir;
	deffilename.append(progStatus.LastMacroFile);
    const char *p = FSEL::select(_("Open macro file"), _("Fldigi macro definition file\t*.mdf"), deffilename.c_str());
    if (p) {
		loadMacros(p);
		progStatus.LastMacroFile = fl_filename_name(p);
	}
	showMacroSet();
}

void MACROTEXT::saveMacroFile()
{
	string deffilename = MacrosDir;
	deffilename.append(progStatus.LastMacroFile);
    const char *p = FSEL::saveas(_("Save macro file"), _("Fldigi macro definition file\t*.mdf"), deffilename.c_str());
    if (p) {
		saveMacros(p);
		progStatus.LastMacroFile = fl_filename_name(p);
	}
}

void MACROTEXT::loadnewMACROS(string &s, size_t &i, size_t endbracket)
{
	string fname = s.substr(i+8, endbracket - i - 8);
	if (fname.length() > 0) {
		loadMacros(fname);
		progStatus.LastMacroFile = fl_filename_name(fname.c_str());
	}
	s.replace(i, endbracket - i + 1, "");
	showMacroSet();
}

string MACROTEXT::expandMacro(std::string &s)
{
	size_t idx = 0;
	expand = true;
	TransmitON = false;
	ToggleTXRX = false;
//	mNbr = n;
	expanded = s;//text[n];
	MTAGS *pMtags;

	xbeg = xend = -1;
	save_xchg = false;
	progStatus.repeatMacro = -1;
	text2repeat.clear();
	idleTime = 0;
	waitTime = 0;
	tuneTime = 0;

	while ((idx = expanded.find('<', idx)) != string::npos) {
		size_t endbracket = expanded.find('>',idx);
 		if (expanded.find("<MACROS:",idx) == idx) {
			loadnewMACROS(expanded, idx, endbracket);
			idx++;
			continue;
		}
		// we must handle this specially
		if (expanded.find("<CONT>", idx) == idx)
			pCONT(expanded, idx, endbracket);
		if (!expand) {
			idx++;
			continue;
		}

		pMtags = mtags;
		while (pMtags->mTAG != 0) {
			if (expanded.find(pMtags->mTAG,idx) == idx) {
				pMtags->fp(expanded,idx, endbracket);
				break;
			}
			pMtags++;
		}
		if (pMtags->mTAG == 0)
			idx++;
	}
	if (GET) {
		size_t pos1 = expanded.find("$NAME");
		size_t pos2 = expanded.find("$QTH");
		size_t pos3 = expanded.find("$LOC");
		if (pos1 != string::npos && pos2 != string::npos) {
			pos1 += 5;
			inpName->value(expanded.substr(pos1, pos2 - pos1).c_str());
		}
		if (pos2 != string::npos) {
			pos2 += 4;
			inpQth->value(expanded.substr(pos2, pos3 - pos2).c_str());
		}
		if (pos3 != string::npos) {
			pos3 += 4;
			inpLoc->value(expanded.substr(pos3).c_str());
		}
		GET = false;
		return "";
	}

	if (xbeg != string::npos && xend != string::npos && xend > xbeg) {
		qso_exchange = expanded.substr(xbeg, xend - xbeg);
	} else if (save_xchg) {
		qso_exchange = expanded;
		save_xchg = false;
	}

// force "^r" to be last tag in the expanded string
	if ((idx = expanded.find("^r")) != string::npos) {
		expanded.erase(idx, 2);
		expanded.append("^r");
	}

	return expanded;
}

void idleTimer(void *)
{
	macro_idle_on = false;
}

void continueMacro(void *)
{
	if ( TransmitON ) {
		active_modem->set_stopflag(false);
		if (macro_idle_on && idleTime > 0)
			Fl::add_timeout(idleTime, idleTimer);
		start_tx();
		TransmitON = false;
	}
	text2send.clear();
}

void finishTune(void *)
{
	trx_receive();
// delay to allow tx/rx loop to change state
	Fl::add_timeout(0.5, continueMacro);
}

void finishWait(void *)
{
	if (useTune && tuneTime > 0) {
		trx_tune();
		Fl::add_timeout(tuneTime, finishTune);
		useTune = false;
		return;
	}
	if ( TransmitON ) {
		active_modem->set_stopflag(false);
		if (macro_idle_on && idleTime > 0)
			Fl::add_timeout(idleTime, idleTimer);
		start_tx();
		TransmitON = false;
	}
}

static void set_button(Fl_Button* button, bool value)
{
	button->value(value);
	button->do_callback();
}

void MACROTEXT::timed_execute()
{
	queue_reset();
	TransmitText->clear();
	text2send = expandMacro(exec_string);
	TransmitText->add(text2send.c_str());
	exec_string.clear();
	active_modem->set_stopflag(false);
	start_tx();
}

void MACROTEXT::execute(int n)
{
	mNbr = n;
//	text2save = 
	text2send = expandMacro(text[n]);

	if (timed_exec) {
		progStatus.repeatMacro = -1;
		exec_string = text[n];
		timed_exec = false;
		startTimedExecute(name[n]);
		return;
	}

	if (progStatus.repeatMacro == -1)
		TransmitText->add( text2send.c_str() );
	else {
		size_t p = string::npos;
		text2send = text[n];
		while ((p = text2send.find('<')) != string::npos)
			text2send[p] = '[';
		while ((p = text2send.find('>')) != string::npos)
			text2send[p] = ']';
		TransmitText->add( text2send.c_str() );
	}
	text2send.clear();

	if (ToggleTXRX) {
		text2send.clear();
		if (!wf->xmtrcv->value()) {
			REQ(set_button, wf->xmtrcv, true);
			if (macro_idle_on && idleTime > 0)
				Fl::add_timeout(idleTime, idleTimer);
		} else
			REQ(set_button, wf->xmtrcv, false);
		return;
	}
	if (useWait && waitTime > 0) {
		Fl::add_timeout(waitTime, finishWait);
		useWait = false;
		return;
	}
	if (useTune && tuneTime > 0) {
		trx_tune();
		Fl::add_timeout(tuneTime, finishTune);
		useTune = false;
		return;
	}
	if ( TransmitON ) {
		if (macro_idle_on && idleTime > 0)
			Fl::add_timeout(idleTime, idleTimer);

		active_modem->set_stopflag(false);
		start_tx();
		TransmitON = false;
	}
}

void MACROTEXT::repeat(int n)
{
	expandMacro(text[n]);
	LOG_WARN("%s",text2repeat.c_str());
	macro_idle_on = false;
	if (idleTime) progStatus.repeatIdleTime = idleTime;
}

MACROTEXT::MACROTEXT()
{
	changed = false;
	char szname[5];
	for (int i = 0; i < MAXMACROS; i++) {
		snprintf(szname, sizeof(szname), "F-%d", i+1);
		name[i] = szname;//"";
		text[i] = "";
	}
}


static string mtext =
"//fldigi macro definition file extended\n\
// This file defines the macro structure(s) for the digital modem program, fldigi\n\
// It also serves as a basis for any macros that are written by the user\n\
//\n\
// The top line of this file should always be the first line in every macro \n\
// definition file (.mdf) for the fldigi program to recognize it as such.\n\
//\n\
";

void MACROTEXT::saveMacros(const string& fname) {
	string work;
	ofstream mfile(fname.c_str());
	mfile << mtext;
	for (int i = 0; i < MAXMACROS; i++) {
		mfile << "\n//\n// Macro # " << i+1 << "\n";
		mfile << "/$ " << i << " " << macros.name[i].c_str() << "\n";
		work = macros.text[i];
		size_t pos;
		pos = work.find('\n');
		while (pos != string::npos) {
			work.insert(pos, "\\n");
			pos = work.find('\n', pos + 3);
		}
		mfile << work.c_str();
	}
	mfile << "\n";
	mfile.close();
	changed = false;
}

