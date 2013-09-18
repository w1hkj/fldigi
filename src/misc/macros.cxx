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
#include "misc.h"

#include "fl_digi.h"
#include "timeops.h"
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
#include "network.h"
#include "logsupport.h"
#include "icons.h"
#include "weather.h"
#include "utf8file_io.h"

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

//using namespace std;

struct CMDS { std::string cmd; void (*fp)(std::string); };
static queue<CMDS> cmds;

// following used for debugging and development
void pushcmd(CMDS cmd)
{
	LOG_INFO("%s, # = %d", cmd.cmd.c_str(), (int)cmds.size());
	cmds.push(cmd);
}

// these variables are referenced outside of this file
MACROTEXT macros;
CONTESTCNTR contest_count;

std::string qso_time = "";
std::string qso_exchange = "";
std::string exec_date = "";
std::string exec_time = "";
std::string exec_string = "";
std::string info1msg = "";
std::string info2msg = "";
std::string text2repeat = "";

size_t repeatchar = 0;

bool macro_idle_on = false;

static float  idleTime = 0;
static bool TransmitON = false;
static bool ToggleTXRX = false;
static int mNbr;

static std::string text2send = "";

static size_t  xbeg = 0, xend = 0;

static bool save_xchg;
static bool expand;
static bool GET = false;
static bool timed_exec = false;
static bool within_exec = false;

static const char cutnumbers[] = "T12345678N";
static std::string cutstr;

static std::string cut_string(const char *s)
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

static void pCOMMENT(std::string &s, size_t &i, size_t endbracket)
{
	s.replace(i, endbracket - i + 1, "");
	if (s[i] == '\n') i++;
}

static void pFILE(std::string &s, size_t &i, size_t endbracket)
{
	std::string fname = s.substr(i+6, endbracket - i - 6);
	if (fname.length() > 0 && !within_exec) {
		FILE *toadd = fopen(fname.c_str(), "r");
		if (toadd) {
			std::string buffer;
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

static void pTIMER(std::string &s, size_t &i, size_t endbracket)
{
	int number;
	std::string sTime = s.substr(i+7, endbracket - i - 7);
	if (sTime.length() > 0 && !within_exec) {
		sscanf(sTime.c_str(), "%d", &number);
		progStatus.timer = number;
		progStatus.timerMacro = mNbr;
	}
	s.replace(i, endbracket - i + 1, "");
}

static void pREPEAT(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	progStatus.repeatMacro = mNbr;
	s.replace(i, endbracket - i + 1, "");
	text2repeat = s;
	repeatchar = 0;
	s.insert(i, "[REPEAT]");
}

static void pWPM(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	int number;
	std::string snumber = s.substr(i+5, endbracket - i - 5);

	if (snumber.length() > 0) {

// first value = WPM
		sscanf(snumber.c_str(), "%d", &number);
		if (number < 5) number = 5;
		if (number > 200) number = 200;
		progdefaults.CWspeed = number;
		sldrCWxmtWPM->value(number);

// second value = Farnsworth WPM
		size_t pos;
		if ((pos = snumber.find(":")) != std::string::npos) {
			snumber.erase(0, pos+1);
			if (snumber.length())
				sscanf(snumber.c_str(), "%d", &number);
			if (number < 15) number = 15;
			if (number > 200) number = 200;
			progdefaults.CWfarnsworth = number;
			sldrCWfarnsworth->value(number);
		}
	}

	s.replace(i, endbracket - i + 1, "");
}

static void pRISETIME(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	float number;
	std::string sVal = s.substr(i+6, endbracket - i - 6);
	if (sVal.length() > 0) {
		sscanf(sVal.c_str(), "%f", &number);
		if (number < 0) number = 0;
		if (number > 20) number = 20;
		progdefaults.CWrisetime = number;
		cntCWrisetime->value(number);
	}
	s.replace(i, endbracket - i + 1, "");
}

static void pPRE(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	float number;
	std::string sVal = s.substr(i+5, endbracket - i - 5);
	if (sVal.length() > 0) {
		sscanf(sVal.c_str(), "%f", &number);
		if (number < 0) number = 0;
		if (number > 20) number = 20;
		progdefaults.CWpre = number;
		cntPreTiming->value(number);
	}
	s.replace(i, endbracket - i + 1, "");
}

static void pPOST(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	float number;
	std::string sVal = s.substr(i+6, endbracket - i - 6);
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

static void setfwpm(int d)
{
	sldrCWfarnsworth->value(d);
	progdefaults.CWusefarnsworth = true;
	btnCWusefarnsworth->value(1);
}

static void doWPM(std::string s)
{
	int number;
	std::string snumber = s.substr(6);

	if (snumber.length() > 0) {

// first value = WPM
		sscanf(snumber.c_str(), "%d", &number);
		if (number < 5) number = 5;
		if (number > 200) number = 200;
		progdefaults.CWspeed = number;
		REQ(setwpm, number);

// second value = Farnsworth WPM
		size_t pos;
		if ((pos = snumber.find(":")) != std::string::npos) {
			snumber.erase(0, pos+1);
			if (snumber.length())
				sscanf(snumber.c_str(), "%d", &number);
			if (number < 15) number = 15;
			if (number > 200) number = 200;
			progdefaults.CWfarnsworth = number;
			REQ(setfwpm, number);
		}
	}

}

static void pQueWPM(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	struct CMDS cmd = { s.substr(i, endbracket - i + 1), doWPM };
	pushcmd(cmd);
	s.replace(i, endbracket - i + 1, "^!");
}

static void setRISETIME(int d)
{
	cntCWrisetime->value(d);
}

static void doRISETIME(std::string s)
{
	float number;
	std::string sVal = s.substr(7, s.length() - 8);
	if (sVal.length() > 0) {
		sscanf(sVal.c_str(), "%f", &number);
		if (number < 0) number = 0;
		if (number > 20) number = 20;
		progdefaults.CWrisetime = number;
		REQ(setRISETIME, number);
	}
}

static void pQueRISETIME(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	struct CMDS cmd = { s.substr(i, endbracket - i + 1), doRISETIME };
	pushcmd(cmd);
	s.replace(i, endbracket - i + 1, "^!");
}

static void setPRE(int d)
{
	cntPreTiming->value(d);
}

static void doPRE(std::string s)
{
	float number;
	std::string sVal = s.substr(6, s.length() - 7);
	if (sVal.length() > 0) {
		sscanf(sVal.c_str(), "%f", &number);
		if (number < 0) number = 0;
		if (number > 20) number = 20;
		progdefaults.CWpre = number;
		REQ(setPRE, number);
	}
}

static void pQuePRE(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	struct CMDS cmd = { s.substr(i, endbracket - i + 1), doPRE };
	pushcmd(cmd);
	s.replace(i, endbracket - i + 1, "^!");
}

static void setPOST(int d)
{
	cntPostTiming->value(d);
}

static void doPOST(std::string s)
{
	float number;
	std::string sVal = s.substr(7, s.length() - 8);
	if (sVal.length() > 0) {
		sscanf(sVal.c_str(), "%f", &number);
		if (number < -20) number = -20;
		if (number > 20) number = 20;
		progdefaults.CWpost = number;
		REQ(setPOST, number);
	}
}

static void pQuePOST(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	struct CMDS cmd = { s.substr(i, endbracket - i + 1), doPOST };
	pushcmd(cmd);
	s.replace(i, endbracket - i + 1, "^!");
}

static void setTXATTEN(float v)
{
	int d = (int)(v * 10);
	v = d / 10.0;
	v = clamp(v, -30.0, 0.0);
	progdefaults.txlevel = v;
	cntTxLevel->value(progdefaults.txlevel);;
}

static void pTXATTEN(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	float number;
	std::string sVal = s.substr(i+9, endbracket - i - 9);
	if (sVal.length() > 0) {
		sscanf(sVal.c_str(), "%f", &number);
		setTXATTEN(number);
	}
	s.replace(i, endbracket - i + 1, "");
}

static void doTXATTEN(std::string s)
{
	float number;
	std::string sVal = s.substr(10);
	if (sVal.length() > 0) {
		sscanf(sVal.c_str(), "%f", &number);
		REQ(setTXATTEN, number);
	}
}

static void pQueTXATTEN(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	struct CMDS cmd = { s.substr(i, endbracket - i + 1), doTXATTEN };
	pushcmd(cmd);
	s.replace(i, endbracket - i + 1, "^!");
}

static void pIDLE(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	float number;
	std::string sTime = s.substr(i+6, endbracket - i - 6);
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

static void doIDLE(std::string s)
{
	float number;
	std::string sTime = s.substr(7, s.length() - 8);
	if (sTime.length() > 0) {
		sscanf(sTime.c_str(), "%f", &number);
		Qidle_time = 1;
		Fl::add_timeout(number, doneIDLE);
	} else {
		Qidle_time = 0;
	}
}

static void pQueIDLE(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	struct CMDS cmd = { s.substr(i, endbracket - i + 1), doIDLE };
	pushcmd(cmd);
	s.replace(i, endbracket - i + 1, "^!");
}

static bool useTune = false;
static int  tuneTime = 0;

static void pTUNE(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	int number;
	std::string sTime = s.substr(i+6, endbracket - i - 6);
	if (sTime.length() > 0) {
		sscanf(sTime.c_str(), "%d", &number);
		useTune = true;
		tuneTime = number;
	}
	s.replace(i, endbracket - i + 1, "");
}

static void pQSONBR(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	char szqsonbr[10];
	snprintf(szqsonbr, sizeof(szqsonbr), "%d", qsodb.nbrRecs());
	s.replace(i, endbracket - i + 1, szqsonbr);
}

static void pNXTNBR(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	char szqsonbr[10];
	snprintf(szqsonbr, sizeof(szqsonbr), "%d", qsodb.nbrRecs() + 1);
	s.replace(i, endbracket - i + 1, szqsonbr);
}

static void pNRSID(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	int number = 0;
	std::string sNumber = s.substr(i+7, endbracket - i - 7);
	if (sNumber.length() > 0) {
		sscanf(sNumber.c_str(), "%d", &number);
		progStatus.n_rsids = number;
	}
	s.replace(i, endbracket - i + 1, "");
}

static bool useWait = false;
static int  waitTime = 0;

static void pWAIT(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	int number;
	std::string sTime = s.substr(i+6, endbracket - i - 6);
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

static void doWAIT(std::string s)
{
	int number;
	std::string sTime = s.substr(7, s.length() - 8);
	if (sTime.length() > 0) {
		sscanf(sTime.c_str(), "%d", &number);
		Qwait_time = number;
		Fl::add_timeout (number * 1.0, doneWAIT);
	} else
		Qwait_time = 0;
	que_ok = true;
}

static void pQueWAIT(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	struct CMDS cmd = { s.substr(i, endbracket - i + 1), doWAIT };
	pushcmd(cmd);
	s.replace(i, endbracket - i + 1, "^!");
}


static void pINFO1(std::string &s, size_t &i, size_t endbracket)
{
	s.replace( i, 7, info1msg );
}

static void pINFO2(std::string &s, size_t &i, size_t endbracket)
{
	s.replace( i, 7, info2msg );
}

static void pCLRRX(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	s.replace( i, 7, "" );
	ReceiveText->clear();
}

static void pCLRTX(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	s.replace( i, 7, "" );
	queue_reset();
	TransmitText->clear();
}

static void pCALL(std::string &s, size_t &i, size_t endbracket)
{
	s.replace( i, 6, inpCall->value() );
}

static void pGET(std::string &s, size_t &i, size_t endbracket)
{
	s.erase( i, 9 );
	GET = true;
}

static void pFREQ(std::string &s, size_t &i, size_t endbracket)
{
	s.replace( i, 6, inpFreq->value() );
}

static void pBAND(std::string &s, size_t &i, size_t endbracket)
{
	s.replace( i, 6, band_name( band( wf->rfcarrier() ) ) );
}

static void pLOC(std::string &s, size_t &i, size_t endbracket)
{
	s.replace( i, 5, inpLoc->value() );
}

static void pMODE(std::string &s, size_t &i, size_t endbracket)
{
	s.replace( i, 6, active_modem->get_mode_name());
}

static void pNAME(std::string &s, size_t &i, size_t endbracket)
{
	s.replace( i, 6, inpName->value() );
}

static void pQTH(std::string &s, size_t &i, size_t endbracket)
{
	s.replace( i,5, inpQth->value() );
}

static void pQSOTIME(std::string &s, size_t &i, size_t endbracket)
{
	qso_time = inpTimeOff->value();
	s.replace( i, 9, qso_time.c_str() );
}

static void pRST(std::string &s, size_t &i, size_t endbracket)
{
	s.replace( i, 5, cut_string(inpRstOut->value()));
}

static void pMYCALL(std::string &s, size_t &i, size_t endbracket)
{
	s.replace( i, 8, progdefaults.myCall.c_str() );
}

static void pMYLOC(std::string &s, size_t &i, size_t endbracket)
{
	s.replace( i, 7, progdefaults.myLocator.c_str() );
}

static void pMYNAME(std::string &s, size_t &i, size_t endbracket)
{
	s.replace( i, 8, progdefaults.myName.c_str() );
}

static void pMYQTH(std::string &s, size_t &i, size_t endbracket)
{
	s.replace( i, 7, progdefaults.myQth.c_str() );
}

static void pMYRST(std::string &s, size_t &i, size_t endbracket)
{
	s.replace( i, 7, inpRstIn->value() );
}

static void pANTENNA(std::string &s, size_t &i, size_t endbracket)
{
	s.replace( i, 9, progdefaults.myAntenna.c_str() );
}

static void pLDT(std::string &s, size_t &i, size_t endbracket)
{
	char szDt[80];
	time_t tmptr;
	tm sTime;
	time (&tmptr);
	localtime_r(&tmptr, &sTime);
	mystrftime(szDt, 79, "%x %H:%M %Z", &sTime);
	s.replace( i, 5, szDt);
}

static void pILDT(std::string &s, size_t &i, size_t endbracket)
{
	char szDt[80];
	time_t tmptr;
	tm sTime;
	time (&tmptr);
	localtime_r(&tmptr, &sTime);
	mystrftime(szDt, 79, "%Y-%m-%d %H:%M%z", &sTime);
	s.replace( i, 6, szDt);
}

static void pZDT(std::string &s, size_t &i, size_t endbracket)
{
	char szDt[80];
	time_t tmptr;
	tm sTime;
	time (&tmptr);
	gmtime_r(&tmptr, &sTime);
	mystrftime(szDt, 79, "%x %H:%MZ", &sTime);
	s.replace( i, 5, szDt);
}

static void pIZDT(std::string &s, size_t &i, size_t endbracket)
{
	char szDt[80];
	time_t tmptr;
	tm sTime;
	time (&tmptr);
	gmtime_r(&tmptr, &sTime);
	mystrftime(szDt, 79, "%Y-%m-%d %H:%MZ", &sTime);
	s.replace( i, 6, szDt);
}

static void pLT(std::string &s, size_t &i, size_t endbracket)
{
	char szDt[80];
	time_t tmptr;
	tm sTime;
	time (&tmptr);
	localtime_r(&tmptr, &sTime);
	mystrftime(szDt, 79, "%H%M", &sTime);
	s.replace( i, 4, szDt);
}

static void pZT(std::string &s, size_t &i, size_t endbracket)
{
	char szDt[80];
	time_t tmptr;
	tm sTime;
	time (&tmptr);
	gmtime_r(&tmptr, &sTime);
	mystrftime(szDt, 79, "%H%MZ", &sTime);
	s.replace( i, 4, szDt);
}

static void pLD(std::string &s, size_t &i, size_t endbracket)
{
	char szDt[80];
	time_t tmptr;
	tm sTime;
	time (&tmptr);
	localtime_r(&tmptr, &sTime);
	mystrftime(szDt, 79, "%Y-%m-%d", &sTime);
	s.replace( i, 4, szDt);
}

static void pZD(std::string &s, size_t &i, size_t endbracket)
{
	char szDt[80];
	time_t tmptr;
	tm sTime;
	time (&tmptr);
	gmtime_r(&tmptr, &sTime);
	mystrftime(szDt, 79, "%Y-%m-%d", &sTime);
	s.replace( i, 4, szDt);
}

static void p_ID(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	progdefaults.macroid = true;
	s.replace( i, 4, "");
}

static void pTEXT(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	progdefaults.macrotextid = true;
	s.replace( i, 6, "");
}

static void pCWID(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	progdefaults.macroCWid = true;
	s.replace( i, 6, "");
}

static void doDTMF(std::string s)
{
	progdefaults.DTMFstr = s.substr(6, s.length() - 7);
}

static void pDTMF(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	CMDS cmd = {s.substr(i, endbracket - i + 1), doDTMF};
	pushcmd(cmd);
	s.replace(i, endbracket - i + 1, "^!");
}

static void pPAUSE(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	s.replace (i, 7, "^p");
}

static void pRX(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	s.replace (i, 4, "^r");
}

static void pTX(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	s.erase(i, 4);
	TransmitON = true;
}

static void pTXRX(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	s.erase(i, 7);
	ToggleTXRX = true;
}

static std::string hexstr(std::string &s)
{
	static std::string hex;
	static char val[3];
	hex.clear();
	for (size_t i = 0; i < s.length(); i++) {
		snprintf(val, sizeof(val), "%02x", s[i] & 0xFF);
		hex.append("<").append(val).append(">");
	}
	return hex;
}

static void pRIGCAT(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}

	LOG_INFO("cat cmd:retnbr %s", s.c_str());

	size_t start = s.find(':', i);
	std::basic_string<char> buff;

	size_t val = 0;
	int retnbr = 0;
	char c, ch;
	bool asciisw = false;
	bool valsw = false;

	for (size_t j = start+1 ; j <= endbracket ; j++) {
		ch = s[j];
		if (ch == '\"') {
			asciisw = !asciisw;
			continue;
		}
// accumulate ascii string
		if (asciisw) {
			if (isprint(ch)) buff += ch;
			continue;
		}
// following digits is expected size of CAT response from xcvr
		if (ch == ':' && s[j+1] != '>') {
			sscanf(&s[j+1], "%d", &retnbr);
		}
// accumulate hex string values
		if ((ch == ' ' || ch == '>' || ch == ':') && valsw) {
			c = char(val);
//			LOG_INFO("c=%02x, val=%d", c, val);
			buff += c;
			val = 0;
			valsw = false;
		} else {
			val *= 16;
			ch = toupper(ch);
			if (isdigit(ch)) val += ch - '0';
			else if (ch >= 'A' && ch <= 'F') val += ch - 'A' + 10;
			valsw = true;
		}
		if (ch == ':') break;
	}

	LOG_INFO("cat %s", hexstr(buff).c_str());

	sendCommand(buff, retnbr);

	s.replace(i, endbracket - i + 1, "");
}


static void pVER(std::string &s, size_t &i, size_t endbracket)
{
	std::string progname;
	progname = "Fldigi ";
	progname.append(PACKAGE_VERSION);
	s.replace( i, 5, progname );
}

static void pCNTR(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	int  contestval;
	contestval = contest_count.count;
	if (contestval) {
		contest_count.Format(progdefaults.ContestDigits, progdefaults.UseLeadingZeros);
		snprintf(contest_count.szCount, sizeof(contest_count.szCount), contest_count.fmt.c_str(), contestval);
		s.replace (i, 6, cut_string(contest_count.szCount));
	} else
		s.replace (i, 6, "");
}

static void pDECR(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	contest_count.count--;
	if (contest_count.count < 0) contest_count.count = 0;
	s.replace (i, 6, "");
	updateOutSerNo();
}

static void pINCR(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	contest_count.count++;
	s.replace (i, 6, "");
	updateOutSerNo();
}

static void pXIN(std::string &s, size_t &i, size_t endbracket)
{
	s.replace( i, 5, inpXchgIn->value() );
}

static void pXOUT(std::string &s, size_t &i, size_t endbracket)
{
	s.replace( i, 6, cut_string(progdefaults.myXchg.c_str()));
}

static void pXBEG(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	s.replace( i, 6, "");
	xbeg = i;
}

static void pXEND(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	s.replace( i, 6, "");
	xend = i;
}

static void pSAVEXCHG(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	save_xchg = true;
	s.replace( i, 10, "");
}

static void pLOG(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	size_t start = s.find(':', i);
	if (start != std::string::npos) {
		string msg = inpNotes->value();
		if (!msg.empty()) msg.append("\n");
		msg.append(s.substr(start + 1, endbracket-start-1));
		inpNotes->value(msg.c_str());
	}
	s.replace(i, endbracket - i + 1, "");
	qsoSave_cb(0, 0);
}

static void pLNW(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	size_t start = s.find(':', i);
	if (start != std::string::npos) {
		string msg = inpNotes->value();
		if (!msg.empty()) msg.append("\n");
		msg.append(s.substr(start + 1, endbracket-start-1));
		inpNotes->value(msg.c_str());
	}
	s.replace(i, endbracket - i + 1, "^L");
}

static void pCLRLOG(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	s.replace(i, 10, "^C");
}

static void pMODEM_compSKED(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	size_t	j, k,
			len = s.length();
	std::string name;

	if ((j = s.find('>', i)) == std::string::npos)
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

static void doIMAGE(std::string s)
{
	if (s.length() > 0) {
		string fname = s.substr(7);
		fname.erase(fname.length() - 1);
		trx_mode active_mode = active_modem->get_mode();
		if ((active_mode == MODE_MFSK16 ||
			 active_mode == MODE_MFSK32 ||
			 active_mode == MODE_MFSK64 ||
			 active_mode == MODE_MFSK128) &&
			 active_modem->get_cap() & modem::CAP_IMG) {
			active_modem->send_image(fname);
		}
	}
	que_ok = true;
}

static void pQueIMAGE(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	string cmdstr = s.substr(i, endbracket - i + 1);
	struct CMDS cmd = { cmdstr, doIMAGE };
	pushcmd(cmd);
	s.replace(i, endbracket - i + 1, "^!");
}

#include <float.h>
#include "re.h"

static void doMODEM(std::string s)
{
	static fre_t re("<!MODEM:([[:alnum:]-]+)((:[[:digit:].+-]*)*)>", REG_EXTENDED);
	std::string tomatch = s;

	if (!re.match(tomatch.c_str())) {
		que_ok = true;
		return;
	}

	const std::vector<regmatch_t>& o = re.suboff();
	std::string name = tomatch.substr(o[1].rm_so, o[1].rm_eo - o[1].rm_so);
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
			if (args.at(0) != DBL_MIN && args.at(1) != DBL_MIN) {
				int bw = (int)args[0];
				int tones = (int)args[1];
				if (bw == 250 && tones == 4) m = MODE_OLIVIA_4_250;
				else if (bw == 250 && tones == 8) m = MODE_OLIVIA_8_250;
				else if (bw == 500 && tones == 4) m = MODE_OLIVIA_4_500;
				else if (bw == 500 && tones == 8) m = MODE_OLIVIA_8_500;
				else if (bw == 500 && tones == 16) m = MODE_OLIVIA_16_500;
				else if (bw == 1000 && tones == 8) m = MODE_OLIVIA_8_1000;
				else if (bw == 1000 && tones == 16) m = MODE_OLIVIA_16_1000;
				else if (bw == 1000 && tones == 32) m = MODE_OLIVIA_32_1000;
				else if (bw == 2000 && tones == 64) m = MODE_OLIVIA_64_2000;
				else {
					set_olivia_bw(bw);
					set_olivia_tones(tones);
				}
			}
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

static void pQueMODEM(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	string cmdstr = s.substr(i, endbracket - i + 1);
	struct CMDS cmd = { cmdstr, doMODEM };
	if (cmdstr.find("SSB") != string::npos || cmdstr.find("ANALYSIS") != string::npos) {
		LOG_ERROR("Disallowed: %s", cmdstr.c_str());
		size_t nextbracket = s.find('<', endbracket);
		if (nextbracket != string::npos)
			s.erase(i, nextbracket - i - 1);
		else
			s.clear(); 
	} else {
		pushcmd(cmd);
		s.replace(i, endbracket - i + 1, "^!");
	}
}

static void pMODEM(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	static fre_t re("<MODEM:([[:alnum:]-]+)((:[[:digit:].+-]*)*)>", REG_EXTENDED);

	if (!re.match(s.c_str() + i)) {
		size_t end = s.find('>', i);
		if (end != std::string::npos)
			s.erase(i, end - i);
		return;
	}

	const std::vector<regmatch_t>& o = re.suboff();
	std::string name = s.substr(o[1].rm_so, o[1].rm_eo - o[1].rm_so);
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
			if (args.at(0) != DBL_MIN && args.at(1) != DBL_MIN) {
				int bw = (int)args[0];
				int tones = (int)args[1];
				if (bw == 250 && tones == 4) m = MODE_OLIVIA_4_250;
				else if (bw == 250 && tones == 8) m = MODE_OLIVIA_8_250;
				else if (bw == 500 && tones == 4) m = MODE_OLIVIA_4_500;
				else if (bw == 500 && tones == 8) m = MODE_OLIVIA_8_500;
				else if (bw == 500 && tones == 16) m = MODE_OLIVIA_16_500;
				else if (bw == 1000 && tones == 8) m = MODE_OLIVIA_8_1000;
				else if (bw == 1000 && tones == 16) m = MODE_OLIVIA_16_1000;
				else if (bw == 1000 && tones == 32) m = MODE_OLIVIA_32_1000;
				else if (bw == 2000 && tones == 64) m = MODE_OLIVIA_64_2000;
				else {
					set_olivia_bw(bw);
					set_olivia_tones(tones);
				}
			}
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

	s.replace(i, endbracket - i + 1, "");
}

static void pAFC(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
  std::string sVal = s.substr(i+5, endbracket - i - 5);
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
  s.replace(i, endbracket - i + 1, "");
}

static void pREV(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
  std::string sVal = s.substr(i+5, endbracket - i - 5);
  if (sVal.length() > 0) {
// sVal = on|off|t   [ON, OFF or Toggle]
    if (sVal.compare(0,2,"on") == 0)
      wf->btnRev->value(1);
    else if (sVal.compare(0,3,"off") == 0)
      wf->btnRev->value(0);
    else if (sVal.compare(0,1,"t") == 0)
      wf->btnRev->value(!wf->btnRev->value());

    wf->btnRev->do_callback();
  }
  s.replace(i, endbracket - i + 1, "");
}

// <HS:on|off|t>
static void pHS(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
  std::string sVal = s.substr(i+4, endbracket - i - 4);
  if (sVal.length() > 0) {
// sVal = on|off|t   [ON, OFF or Toggle]
    if (sVal.compare(0,2,"on") == 0)
      bHighSpeed = 1;
    else if (sVal.compare(0,3,"off") == 0)
      bHighSpeed = 0;
    else if (sVal.compare(0,1,"t") == 0)
      bHighSpeed = !bHighSpeed;
  }
  s.replace(i, endbracket - i + 1, "");
}


static void pLOCK(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
  std::string sVal = s.substr(i+6, endbracket - i - 6);
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
  s.replace(i, endbracket - i + 1, "");
}

static void pTX_RSID(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
  std::string sVal = s.substr(i+8, endbracket - i - 8);
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
  s.replace(i, endbracket - i + 1, "");
}

static void pRX_RSID(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
  std::string sVal = s.substr(i+8, endbracket - i - 8);
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
static void pTALK(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
  std::string sVal = s.substr(i+6, endbracket - i - 6);
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

static void pSRCHUP(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	s.replace( i, 8, "");
	active_modem->searchUp();
	if (progdefaults.WaterfallClickInsert)
	        wf->insert_text(true);
}

static void pSRCHDN(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	s.replace( i, 8, "");
	active_modem->searchDown();
	if (progdefaults.WaterfallClickInsert)
	         wf->insert_text(true);
}

static void pGOHOME(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	s.replace( i, 8, "");
	if (active_modem == cw_modem)
		active_modem->set_freq(progdefaults.CWsweetspot);
	else if (active_modem == rtty_modem)
		active_modem->set_freq(progdefaults.RTTYsweetspot);
	else
		active_modem->set_freq(progdefaults.PSKsweetspot);
}

static void doGOHOME(std::string s)
{
	if (active_modem == cw_modem)
		active_modem->set_freq(progdefaults.CWsweetspot);
	else if (active_modem == rtty_modem)
		active_modem->set_freq(progdefaults.RTTYsweetspot);
	else
		active_modem->set_freq(progdefaults.PSKsweetspot);
	que_ok = true;
}

static void pQueGOHOME(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	struct CMDS cmd = { s.substr(i, endbracket - i + 1), doGOHOME };
	pushcmd(cmd);
	s.replace(i, endbracket - i + 1, "^!");
}

static void pGOFREQ(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	int number;
	std::string sGoFreq = s.substr(i+8, endbracket - i - 8);
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

static void doGOFREQ(std::string s)
{
	int number;
	std::string sGoFreq = s.substr(9, s.length() - 10);
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

static void pQueGOFREQ(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	struct CMDS cmd = { s.substr(i, endbracket - i + 1), doGOFREQ };
	pushcmd(cmd);
	s.replace(i, endbracket - i + 1, "^!");
}

static void pQRG(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	std::string prefix = "\n";
	prefix.append(s.substr(i+5, endbracket - i - 5));
	if (prefix.length()) note_qrg ( false, prefix.c_str(), "\n" );
	s.replace(i, endbracket - i + 1, "");
}

static void pQSYTO(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	s.replace( i, 7, "");
	do_qsy(true);
}

static void pQSYFM(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	s.replace( i, 7, "");
	do_qsy(false);
}

static void pQSY(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	int rf = 0;
	int audio = 0;
	float rfd = 0;
	std::string sGoFreq = s.substr(i+5, endbracket - i - 5);
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
	if ((pos = sGoFreq.find(":")) != std::string::npos) {
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

static void doQSY(std::string s)
{
	int rf = 0;
	int audio = 0;
	float rfd = 0;
	std::string sGoFreq;
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
	if ((pos = sGoFreq.find(":")) != std::string::npos) {
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

static void pQueQSY(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	struct CMDS cmd = { s.substr(i, endbracket - i + 1), doQSY };
	pushcmd(cmd);
	s.replace(i, endbracket - i + 1, "^!");
}

static void pRIGMODE(std::string& s, size_t& i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	std::string sMode = s.substr(i+9, endbracket - i - 9);
	qso_opMODE->value(sMode.c_str());
	cb_qso_opMODE();
	s.replace(i, endbracket - i + 1, "");
}

static void doRIGMODE(std::string s)
{
	std::string sMode = s.substr(10, s.length() - 11);
	qso_opMODE->value(sMode.c_str());
	cb_qso_opMODE();
	que_ok = true;
}

static void pQueRIGMODE(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	struct CMDS cmd = { s.substr(i, endbracket - i + 1), doRIGMODE };
	pushcmd(cmd);
	s.replace(i, endbracket - i + 1, "^!");
}

static void pFILWID(std::string& s, size_t& i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	std::string sWidth = s.substr(i+8, endbracket - i - 8);
	qso_opBW->value(sWidth.c_str());
	cb_qso_opBW();
	s.replace(i, endbracket - i + 1, "");
}

static void doFILWID(std::string s)
{
	std::string sWID = s.substr(9, s.length() - 10);
	qso_opBW->value(sWID.c_str());
	cb_qso_opBW();
	que_ok = true;
}

static void pQueFILWID(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	struct CMDS cmd = { s.substr(i, endbracket - i + 1), doFILWID };
	pushcmd(cmd);
	s.replace(i, endbracket - i + 1, "^!");
}

static void pWX(std::string &s, size_t &i, size_t endbracket)
{
	string wx;
	getwx(wx);
	s.replace(i, 4, wx);
}

// <WX:metar>
static void pWX2(std::string &s, size_t &i, size_t endbracket)
{
	string wx;
	getwx(wx, s.substr(i+4, endbracket - i - 4).c_str());
	s.replace(i, endbracket - i + 1, wx);
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

	       FLDIGI_MACRO_FILE,
	       FLDIGI_LOG_FILE,
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
		{ "FLDIGI_MACRO_FILE", progStatus.LastMacroFile.c_str() },

		{ "FLDIGI_LOG_FILE", progdefaults.logbookfilename.c_str() },
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
	static std::string pSKEDh = ScriptsDir;
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
	snprintf(dial_freq, sizeof(dial_freq), "%ld", (long)wf->rfcarrier());
	env[FLDIGI_DIAL_FREQUENCY].val = dial_freq;
	char audio_freq[6];
	snprintf(audio_freq, sizeof(audio_freq), "%d", active_modem->get_freq());
	env[FLDIGI_AUDIO_FREQUENCY].val = audio_freq;
	char freq[20];
	snprintf(freq, sizeof(freq), "%ld", (long)(wf->rfcarrier() + (wf->USB()
								? active_modem->get_freq()
								: -active_modem->get_freq())));
	env[FLDIGI_FREQUENCY].val = freq;

	// debugging vars
#if !defined(NDEBUG) && !defined(__WOE32__)
	unsetenv("FLDIGI_NO_EXEC");
	unsetenv("MALLOC_CHECK_");
	unsetenv("MALLOC_PERTURB_");
#endif

	for (size_t j = 0; j < ENV_SIZE; j++)
		setenv(env[j].var, env[j].val, 1);

	string path = getenv("PATH");
	string mypath = ScriptsDir;
	if (mypath[mypath.length()-1] == '/')
		mypath.erase(mypath.length()-1, 1);
	mypath.append(":");
	path.insert(0,mypath);
	setenv("PATH", path.c_str(), 1);
}

// this is only for the case where the user tries to nest <EXEC>...
// as in
// <EXEC> ... <EXEC> ... </EXEC></EXEC>
// which is not permitted
static void pEND_EXEC(std::string &s, size_t &i, size_t endbracket)
{
	s.replace(i, endbracket - i + 1, "");
	return;
}

#ifndef __MINGW32__
static void pEXEC(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}

	size_t start = s.find(">", i);
	size_t end = s.find("</EXEC>", start);

	if (start == std::string::npos ||
		end == std::string::npos) {
		i++;
		return;
	}

	std::string execstr = s.substr(start+1, end-start-1);
	within_exec = true;
	MACROTEXT m;
	execstr = m.expandMacro(execstr, true);
	within_exec = false;

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
		execl("/bin/sh", "sh", "-c", execstr.c_str(), (char *)NULL);
		perror("execl");
		exit(EXIT_FAILURE);
	}

// parent
	close(pfd[1]);

// give child process time to complete
	MilliSleep(50);
	FILE* fp = fdopen(pfd[0], "r");
	if (!fp) {
		LOG_PERROR("fdopen");
		close(pfd[0]);
		return;
	}

	s.erase(i, end - i + strlen("</EXEC>"));

	char ln[BUFSIZ];
	string lnbuff = "";
	while (fgets(ln, sizeof(ln), fp)) {
		lnbuff.append(ln);
	}
// remove all trailing end-of-lines
	while (lnbuff[lnbuff.length()-1] == '\n')
		lnbuff.erase(lnbuff.length()-1,1);

	if (!lnbuff.empty()) {
		lnbuff = m.expandMacro(lnbuff, false);
		s.insert(i, lnbuff);
		i += lnbuff.length();
	} else
		i++;

	fclose(fp);
	close(pfd[0]);

}
#else // !__MINGW32__

static void pEXEC(std::string& s, size_t& i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	size_t start, end;
	if ((start = s.find('>', i)) == std::string::npos ||
		(end = s.rfind("</EXEC>")) == std::string::npos) {
		i++;
		return;
	}
	start++;

	std::string execstr = s.substr(start, end-start);
	within_exec = true;
	MACROTEXT m;
	execstr = m.expandMacro(execstr, true);
	within_exec = false;

	char* cmd = strdup(execstr.c_str());

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

static void pEQSL(std::string& s, size_t& i, size_t endbracket)
{
	if (within_exec || progdefaults.eqsl_when_logged) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	size_t start = s.find(':', i);

	std::string msg = "";
	if (start != std::string::npos)
		msg = s.substr(start + 1, endbracket-start-1);

	makeEQSL(msg.c_str());

	s.replace(i, endbracket - i + 1, "");
	return;
}

static void MAPIT(int how)
{
	float lat = 0, lon = 0;
	std::string sCALL = inpCall->value();
	std::string sLOC = inpLoc->value();

	std::string url = "http://maps.google.com/maps?q=";

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

static void pMAPIT(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	std::string sVal = s.substr(i + 7, endbracket - i - 7);
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

static void pSTOP(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	s.erase(i, s.find('>', i) + 1 - i);
	expand = false;
}

static void pCONT(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	s.erase(i, s.find('>', i) + 1 - i);
	expand = true;
}

static void pSKED(std::string &s, size_t &i, size_t endbracket)
{
	if (within_exec) {
		s.replace(i, endbracket - i + 1, "");
		return;
	}
	std::string data = s.substr(i+6, endbracket - i - 6);
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

static void postQueue(std::string s)
{
	ReceiveText->addstr(s, FTextBase::CTRL);
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
static std::string rxcmds = "<!MOD<!WAI<!GOH<!QSY<!GOF<!RIG<!FIL";
	if (cmds.empty()) return false;
	CMDS cmd = cmds.front();
	bool ret = (rxcmds.find(cmd.cmd.substr(0,5)) != std::string::npos);
	return ret;
}

struct MTAGS { const char *mTAG; void (*fp)(std::string &, size_t&, size_t );};

static const MTAGS mtags[] = {
{"<COMMENT:",	pCOMMENT},
{"<CALL>",		pCALL},
{"<FREQ>",		pFREQ},
{"<BAND>",		pBAND},
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
{"<ANTENNA>",	pANTENNA},
{"<QSOTIME>",	pQSOTIME},
{"<QSONBR>",	pQSONBR},
{"<NXTNBR>",	pNXTNBR},
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
{"<ID>",		p_ID},
{"<TEXT>",		pTEXT},
{"<CWID>",		pCWID},
{"<RX>",		pRX},
{"<TX>",		pTX},
{"<TX/RX>",		pTXRX},
{"<VER>",		pVER},
{"<RIGCAT:",	pRIGCAT},
{"<CNTR>",		pCNTR},
{"<DECR>",		pDECR},
{"<INCR>",		pINCR},
{"<X1>",		pXOUT},
{"<XIN>",		pXIN},
{"<XOUT>",		pXOUT},
{"<XBEG>",		pXBEG},
{"<XEND>",		pXEND},
{"<SAVEXCHG>",	pSAVEXCHG},
{"<LOG",		pLOG},
{"<LNW",		pLNW},
{"<CLRLOG>",	pCLRLOG},
{"<EQSL",		pEQSL},
{"<TIMER:",		pTIMER},
{"<IDLE:",		pIDLE},
{"<TUNE:",		pTUNE},
{"<WAIT:",		pWAIT},
{"<NRSID:",		pNRSID},
{"<MODEM>",		pMODEM_compSKED},
{"<MODEM:",		pMODEM},
{"<EXEC>",		pEXEC},
{"</EXEC>",		pEND_EXEC},
{"<STOP>",		pSTOP},
{"<CONT>",		pCONT},
{"<PAUSE>",		pPAUSE},
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
{"<REV:",		pREV},
{"<HS:",		pHS},
{"<RXRSID:",	pRX_RSID},
{"<TXRSID:",	pTX_RSID},
{"<DTMF:",		pDTMF},
{"<SRCHUP>",	pSRCHUP},
{"<SRCHDN>",	pSRCHDN},
{"<GOHOME>",	pGOHOME},
{"<GOFREQ:",	pGOFREQ},
{"<QRG:",		pQRG},
{"<QSY:",		pQSY},
{"<QSYTO>",		pQSYTO},
{"<QSYFM>",		pQSYFM},
{"<RIGMODE:",	pRIGMODE},
{"<FILWID:",	pFILWID},
{"<MAPIT:",		pMAPIT},
{"<MAPIT>",		pMAPIT},
{"<REPEAT>",	pREPEAT},
{"<SKED:",		pSKED},
{"<TXATTEN:",	pTXATTEN},
#ifdef __WIN32__
{"<TALK:",		pTALK},
#endif
{"<WX>",		pWX},
{"<WX:",		pWX2},
{"<IMAGE:",		pQueIMAGE},
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
{"<!RIGMODE:",	pQueRIGMODE},
{"<!FILWID:",	pQueFILWID},
{"<!TXATTEN:",	pQueTXATTEN},
{0, 0}
};

int MACROTEXT::loadMacros(const std::string& filename)
{
	std::string mLine;
	std::string mName;
	std::string mDef;
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
	if (mLine.find("extended") == std::string::npos) {
		convert = true;
		changed = true;
	}
// clear all of the macros
	for (int i = 0; i < MAXMACROS; i++) {
		name[i] = "";
		text[i] = "";
	}
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
		while ((crlf = mLine.find("\\n")) != std::string::npos) {
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
	std::string Filename = MacrosDir;
	Filename.append("macros.mdf");
LOG_INFO("macro file name: %s", progStatus.LastMacroFile.c_str());
	if (progdefaults.UseLastMacro == true) {
		if (progStatus.LastMacroFile.find("/") != string::npos ||
			progStatus.LastMacroFile.find("\\") != string::npos)
			Filename.assign(progStatus.LastMacroFile);
		else
			Filename.assign(MacrosDir).append(progStatus.LastMacroFile);
	}
LOG_INFO("loading: %s", Filename.c_str());
	progStatus.LastMacroFile = Filename;

	if ((erc = loadMacros(Filename)) != 0)
#ifndef __WOE32__
		LOG_ERROR("Error #%d loading %s\n", erc, Filename.c_str());
#else
	;
#endif
	if (progdefaults.DisplayMacroFilename) {
		string Macroset;
		Macroset.assign("\nMacros: ").append(progStatus.LastMacroFile).append("\n");
		ReceiveText->addstr(Macroset);
	}
}

void MACROTEXT::openMacroFile()
{
	std::string deffilename = MacrosDir;

	if (progStatus.LastMacroFile.find("/") != string::npos ||
		progStatus.LastMacroFile.find("\\") != string::npos)
		deffilename.assign(progStatus.LastMacroFile);
	else
		deffilename.append(progStatus.LastMacroFile);

	const char *p = FSEL::select(
			_("Open macro file"),
			_("Fldigi macro definition file\t*.{mdf}"),
			deffilename.c_str());
    if (p) {
		loadMacros(p);
		progStatus.LastMacroFile = p;
	}
	showMacroSet();
	if (progdefaults.DisplayMacroFilename) {
		string Macroset;
		Macroset.assign("\nMacros: ").append(progStatus.LastMacroFile).append("\n");
		ReceiveText->addstr(Macroset);
	}
}

void MACROTEXT::writeMacroFile()
{
	std::string deffilename = MacrosDir;
	if (progStatus.LastMacroFile.find("/") != string::npos ||
		progStatus.LastMacroFile.find("\\") != string::npos)
		deffilename.assign(progStatus.LastMacroFile);
	else
		deffilename.append(progStatus.LastMacroFile);

	saveMacros(deffilename.c_str());
}

void MACROTEXT::saveMacroFile()
{
	std::string deffilename = MacrosDir;

	if (progStatus.LastMacroFile.find("/") != string::npos ||
		progStatus.LastMacroFile.find("\\") != string::npos)
		deffilename.assign(progStatus.LastMacroFile);
	else
		deffilename.append(progStatus.LastMacroFile);

	const char *p = FSEL::saveas(
			_("Save macro file"),
			_("Fldigi macro definition file\t*.{mdf}"),
			deffilename.c_str());
    if (p) {
		string sp = p;
		if (sp.rfind(".mdf") == string::npos) sp.append(".mdf");
		saveMacros(sp.c_str());
		progStatus.LastMacroFile = sp;
	}
}

void MACROTEXT::savecurrentMACROS(std::string &s, size_t &i, size_t endbracket)
{
	saveMacros(progStatus.LastMacroFile.c_str());
	s.replace(i, endbracket - i + 1, "");
}

void MACROTEXT::loadnewMACROS(std::string &s, size_t &i, size_t endbracket)
{
	std::string fname = s.substr(i+8, endbracket - i - 8);
	if (fname.length() > 0) {
		loadMacros(fname);
		progStatus.LastMacroFile = fl_filename_name(fname.c_str());
	}
	s.replace(i, endbracket - i + 1, "");
	showMacroSet();
}

std::string MACROTEXT::expandMacro(std::string &s, bool recurse = false)
{
	size_t idx = 0;
	expand = true;
	if (!recurse) {
		TransmitON = false;
		ToggleTXRX = false;
	}
//	mNbr = n;
	expanded = s;
	const MTAGS *pMtags;

	xbeg = xend = -1;
	save_xchg = false;
	progStatus.repeatMacro = -1;
	text2repeat.clear();
	idleTime = 0;
	waitTime = 0;
	tuneTime = 0;

	while ((idx = expanded.find('<', idx)) != std::string::npos) {
		size_t endbracket = expanded.find('>',idx);
		if (expanded.find("<SAVE", idx) == idx) {
			savecurrentMACROS(expanded, idx, endbracket);
			idx++;
			continue;
		}
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
		if (pos1 != std::string::npos && pos2 != std::string::npos) {
			pos1 += 5;
			inpName->value(expanded.substr(pos1, pos2 - pos1).c_str());
		}
		if (pos2 != std::string::npos) {
			pos2 += 4;
			inpQth->value(expanded.substr(pos2, pos3 - pos2).c_str());
		}
		if (pos3 != std::string::npos) {
			pos3 += 4;
			inpLoc->value(expanded.substr(pos3).c_str());
		}
		GET = false;
		return "";
	}

	if (xbeg != std::string::npos && xend != std::string::npos && xend > xbeg) {
		qso_exchange = expanded.substr(xbeg, xend - xbeg);
	} else if (save_xchg) {
		qso_exchange = expanded;
		save_xchg = false;
	}

// force "^r" to be last tag in the expanded std::string
	if ((idx = expanded.find("^r")) != std::string::npos) {
		expanded.erase(idx, 2);
		expanded.append("^r");
	}

	return expanded;
}

void idleTimer(void *)
{
	macro_idle_on = false;
}

static void continueMacro(void *)
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

static void finishTune(void *)
{
	trx_receive();
// delay to allow tx/rx loop to change state
	Fl::add_timeout(0.5, continueMacro);
}

static void finishWait(void *)
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
	TransmitText->add_text(text2send);
	exec_string.clear();
	active_modem->set_stopflag(false);
	start_tx();
}

void MACROTEXT::execute(int n)
{
	mNbr = n;
	text2send = expandMacro(text[n]);

	if (timed_exec) {
		progStatus.repeatMacro = -1;
		exec_string = text[n];
		timed_exec = false;
		startTimedExecute(name[n]);
		return;
	}

	if (progStatus.repeatMacro == -1)
		TransmitText->add_text( text2send );
	else {
		size_t p = std::string::npos;
		text2send = text[n];
		while ((p = text2send.find('<')) != std::string::npos)
			text2send[p] = '[';
		while ((p = text2send.find('>')) != std::string::npos)
			text2send[p] = ']';
		TransmitText->add_text( text2send );
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
	LOG_INFO("%s",text2repeat.c_str());
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


static std::string mtext =
"//fldigi macro definition file extended\n\
// This file defines the macro structure(s) for the digital modem program, fldigi\n\
// It also serves as a basis for any macros that are written by the user\n\
//\n\
// The top line of this file should always be the first line in every macro \n\
// definition file (.mdf) for the fldigi program to recognize it as such.\n\
//\n\
";

void MACROTEXT::saveMacros(const std::string& fname) {

	std::string work;
	std::string output;
	char temp[200];
	output.assign(mtext);
	for (int i = 0; i < MAXMACROS; i++) {
		snprintf(temp, sizeof(temp), "\n//\n// Macro # %d\n/$ %d %s\n",
			i+1, i, macros.name[i].c_str());
		output.append(temp);
		work = macros.text[i];
		size_t pos;
		pos = work.find('\n');
		while (pos != std::string::npos) {
			work.insert(pos, "\\n");
			pos = work.find('\n', pos + 3);
		}
		output.append(work).append("\n");
	}
	UTF8_writefile(fname.c_str(), output);

	changed = false;
}

