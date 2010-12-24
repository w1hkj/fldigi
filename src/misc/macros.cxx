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

#include <FL/Fl.H>
#include <FL/filename.H>
#include "fileselect.h"

#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <string>
#include <fstream>

#ifdef __WIN32__
#include "speak.h"
#endif

using namespace std;

MACROTEXT macros;
CONTESTCNTR contest_count;
static bool TransmitON = false;
static bool ToggleTXRX = false;
int mNbr;

std::string qso_time = "";
std::string qso_exchange = "";
bool save_xchg;
size_t  xbeg = 0, xend = 0;

string text2send = "";
string text2repeat = "";
string text2save = "";
size_t repeatchar = 0;

struct MTAGS { const char *mTAG; void (*fp)(string &, size_t &);};

void pCALL(string &, size_t &);
void pFREQ(string &, size_t &);
void pLOC(string &, size_t &);
void pMODE(string &, size_t &);
void pNAME(string &, size_t &);
void pQTH(string &, size_t &);
void pRST(string &, size_t &);
void pMYCALL(string &, size_t &);
void pMYLOC(string &, size_t &);
void pMYNAME(string &, size_t &);
void pMYQTH(string &, size_t &);
void pMYRST(string &, size_t &);
void pQSOTIME(string &, size_t &);
void pLDT(string &, size_t &);
void pILDT(string &, size_t &);
void pZDT(string &, size_t &);
void pIZDT(string &, size_t &);
void pZT(string &, size_t &);
void pLT(string &, size_t &);
void pLD(string &, size_t &);
void pZD(string &, size_t &);
void pID(string &, size_t &);
void pTEXT(string &, size_t &);
void pCWID(string &, size_t &);
void pRX(string &, size_t &);
void pTX(string &, size_t &);
void pTXRX(string &, size_t &);
void pVER(string &, size_t &);
void pCNTR(string &, size_t &);
void pDECR(string &, size_t &);
void pINCR(string &, size_t &);
void pXOUT(string &, size_t &);
void pSAVEXCHG(string &, size_t &);
void pXBEG(string &, size_t &);
void pXEND(string &, size_t &);
void pLOG(string &, size_t &);
void pLNW(string &, size_t &);
void pCLRLOG(string &, size_t &);
void pTIMER(string &, size_t &);
void pIDLE(string &, size_t &);
void pTUNE(string &, size_t &);
void pMODEM_compat(string &, size_t &);
void pMODEM(string &, size_t &);
void pEXEC(string &, size_t &);
void pSTOP(string &, size_t &);
void pCONT(string &, size_t &);
void pGET(string &, size_t &);
void pINFO1(string &, size_t &);
void pINFO2(string &, size_t &);
void pCLRRX(string &, size_t &);
void pFILE(string &, size_t &);
void pWPM(string &, size_t &);
void pRISETIME(string &, size_t &);
void pPRE(string &, size_t &);
void pPOST(string &, size_t &);
void pAFC(string &, size_t &);
void pLOCK(string &, size_t &);
void pRX_RSID(string &, size_t &);
void pTX_RSID(string &, size_t &);

#ifdef __WIN32__
void pTALK(string &, size_t &);
#endif

void pWAIT(string&, size_t &);
void pSRCHUP(string&, size_t&);
void pSRCHDN(string&, size_t&);
void pGOHOME(string&, size_t&);
void pGOFREQ(string&, size_t&);
void pMAPIT(string&, size_t&);
void pREPEAT(string&, size_t&);

//void pMACROS(string &, size_t &);

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
{"<MODEM>",		pMODEM_compat},
{"<MODEM:",		pMODEM},
{"<EXEC>",		pEXEC},
{"<STOP>",		pSTOP},
{"<CONT>",		pCONT},
{"<GET>",		pGET},
{"<CLRRX>",		pCLRRX},
{"<FILE:",		pFILE},
{"<WPM:",		pWPM},
{"<RISE:",		pRISETIME},
{"<PRE:",		pPRE},
{"<POST:",		pPOST},
{"<AFC:",		pAFC},
{"<LOCK:",		pLOCK},
{"<RXRSID:",	pRX_RSID},
{"<TXRSID:",	pTX_RSID},
{"<SRCHUP>",	pSRCHUP},
{"<SRCHDN>",	pSRCHDN},
{"<GOHOME>",	pGOHOME},
{"<GOFREQ:",	pGOFREQ},
{"<MAPIT:",		pMAPIT},
{"<MAPIT>",		pMAPIT},
{"<REPEAT>",	pREPEAT},
#ifdef __WIN32__
{"<TALK:",		pTALK},
#endif
{0, 0}
};

static bool expand;
static bool GET = false;

string info1msg = "";
string info2msg = "";

static char cutnumbers[] = "T12345678N";
static string cutstr;

string cutstring(const char *s)
{
	cutstr = s;
	if (!progdefaults.cutnbrs || active_modem != cw_modem)
		return cutstr;

	for (size_t i = 0; i < cutstr.length(); i++)
		if (cutstr[i] >= '0' && cutstr[i] <= '9')
			cutstr[i] = cutnumbers[cutstr[i] - '0'];
	return cutstr;

}

size_t mystrftime( char *s, size_t max, const char *fmt, const struct tm *tm) {
	return strftime(s, max, fmt, tm);
}

void pFILE(string &s, size_t &i)
{
	size_t endbracket = s.find('>',i);
	string fname = s.substr(i+6, endbracket - i - 6);
	if (fname.length() > 0) {
		FILE *toadd = fopen(fname.c_str(), "r");
		string buffer;
		char c = getc(toadd);
		while (c && !feof(toadd)) {
			if (c != '\r') buffer += c; // damn MSDOS txt files
			c = getc(toadd);
			}
		s.replace(i, endbracket - i + 1, buffer);
		fclose(toadd);
	} else
		s.replace(i, endbracket - i + 1, "");
}

void pTIMER(string &s, size_t &i)
{
	size_t endbracket = s.find('>',i);
	int number;
	string sTime = s.substr(i+7, endbracket - i - 7);
	if (sTime.length() > 0) {
		sscanf(sTime.c_str(), "%d", &number);
		progStatus.timer = number;
		progStatus.timerMacro = mNbr;
	}
	s.replace(i, endbracket - i + 1, "");
}

void pREPEAT(string &s, size_t &i)
{
	size_t endbracket = s.find('>',i);
	progStatus.repeatMacro = mNbr;
	s.replace(i, endbracket - i + 1, "");
	text2repeat = s;
	repeatchar = 0;
	s.insert(i, "[REPEAT]");
}

void pWPM(string &s, size_t &i)
{
	size_t endbracket = s.find('>',i);
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

void pRISETIME(string &s, size_t &i)
{
	size_t endbracket = s.find('>',i);
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

void pPRE(string &s, size_t &i)
{
	size_t endbracket = s.find('>',i);
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

void pPOST(string &s, size_t &i)
{
	size_t endbracket = s.find('>',i);
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

bool macro_idle_on = false;
float  idleTime = 0;

void pIDLE(string &s, size_t &i)
{
	size_t endbracket = s.find('>',i);
	float number;
	string sTime = s.substr(i+6, endbracket - i - 6);
	if (sTime.length() > 0) {
		sscanf(sTime.c_str(), "%f", &number);
		macro_idle_on = true;
		idleTime = number;
	}
	s.replace(i, endbracket - i + 1, "");
}

bool useTune = false;
int  tuneTime = 0;

void pTUNE(string &s, size_t &i)
{
	size_t endbracket = s.find('>',i);
	int number;
	string sTime = s.substr(i+6, endbracket - i - 6);
	if (sTime.length() > 0) {
		sscanf(sTime.c_str(), "%d", &number);
		useTune = true;
		tuneTime = number;
	}
	s.replace(i, endbracket - i + 1, "");
}

bool useWait = false;
int  waitTime = 0;

void pWAIT(string &s, size_t &i)
{
	size_t endbracket = s.find('>',i);
	int number;
	string sTime = s.substr(i+6, endbracket - i - 6);
	if (sTime.length() > 0) {
		sscanf(sTime.c_str(), "%d", &number);
		useWait = true;
		waitTime = number;
	}
	s.replace(i, endbracket - i + 1, "");
}

void pINFO1(string &s, size_t &i)
{
	s.replace( i, 7, info1msg );
}

void pINFO2(string &s, size_t &i)
{
	s.replace( i, 7, info2msg );
}

void pCLRRX(string &s, size_t &i)
{
	s.replace( i, 7, "" );
	ReceiveText->clear();
}

void pCALL(string &s, size_t &i)
{
	s.replace( i, 6, inpCall->value() );
}

void pGET(string &s, size_t &i)
{
	s.erase( i, 9 );
	GET = true;
}

void pFREQ(string &s, size_t &i)
{
	s.replace( i, 6, inpFreq->value() );
}

void pLOC(string &s, size_t &i)
{
	s.replace( i, 5, inpLoc->value() );
}

void pMODE(string &s, size_t &i)
{
	s.replace( i, 6, active_modem->get_mode_name());
}

void pNAME(string &s, size_t &i)
{
	s.replace( i, 6, inpName->value() );
}

void pQTH(string &s, size_t &i)
{
	s.replace( i,5, inpQth->value() );
}

void pQSOTIME(string &s, size_t &i)
{
	qso_time = inpTimeOff->value();
	s.replace( i, 9, qso_time.c_str() );
}

void pRST(string &s, size_t &i)
{
	s.replace( i, 5, cutstring(inpRstOut->value()));
}


void pMYCALL(string &s, size_t &i)
{
	s.replace( i, 8, inpMyCallsign->value() );
}

void pMYLOC(string &s, size_t &i)
{
	s.replace( i, 7, inpMyLocator->value() );
}

void pMYNAME(string &s, size_t &i)
{
	s.replace( i, 8, inpMyName->value() );
}

void pMYQTH(string &s, size_t &i)
{
	s.replace( i, 7, inpMyQth->value() );
}

void pMYRST(string &s, size_t &i)
{
	s.replace( i, 7, inpRstIn->value() );
}

void pLDT(string &s, size_t &i)
{
	char szDt[80];
	time_t tmptr;
	tm sTime;
	time (&tmptr);
	localtime_r(&tmptr, &sTime);
	mystrftime(szDt, 79, "%x %H:%M %Z", &sTime);
	s.replace( i, 5, szDt);
}

void pILDT(string &s, size_t &i)
{
	char szDt[80];
	time_t tmptr;
	tm sTime;
	time (&tmptr);
	localtime_r(&tmptr, &sTime);
	mystrftime(szDt, 79, "%Y-%m-%d %H:%M%z", &sTime);
	s.replace( i, 6, szDt);
}

void pZDT(string &s, size_t &i)
{
	char szDt[80];
	time_t tmptr;
	tm sTime;
	time (&tmptr);
	gmtime_r(&tmptr, &sTime);
	mystrftime(szDt, 79, "%x %H:%MZ", &sTime);
	s.replace( i, 5, szDt);
}

void pIZDT(string &s, size_t &i)
{
	char szDt[80];
	time_t tmptr;
	tm sTime;
	time (&tmptr);
	gmtime_r(&tmptr, &sTime);
	mystrftime(szDt, 79, "%Y-%m-%d %H:%MZ", &sTime);
	s.replace( i, 6, szDt);
}

void pLT(string &s, size_t &i)
{
	char szDt[80];
	time_t tmptr;
	tm sTime;
	time (&tmptr);
	localtime_r(&tmptr, &sTime);
	mystrftime(szDt, 79, "%H%M", &sTime);
	s.replace( i, 4, szDt);
}

void pZT(string &s, size_t &i)
{
	char szDt[80];
	time_t tmptr;
	tm sTime;
	time (&tmptr);
	gmtime_r(&tmptr, &sTime);
	mystrftime(szDt, 79, "%H%MZ", &sTime);
	s.replace( i, 4, szDt);
}

void pLD(string &s, size_t &i)
{
	char szDt[80];
	time_t tmptr;
	tm sTime;
	time (&tmptr);
	localtime_r(&tmptr, &sTime);
	mystrftime(szDt, 79, "%Y-%m-%d", &sTime);
	s.replace( i, 4, szDt);
}

void pZD(string &s, size_t &i)
{
	char szDt[80];
	time_t tmptr;
	tm sTime;
	time (&tmptr);
	gmtime_r(&tmptr, &sTime);
	mystrftime(szDt, 79, "%Y-%m-%d", &sTime);
	s.replace( i, 4, szDt);
}

void pID(string &s, size_t &i)
{
	progdefaults.macroid = true;
	s.replace( i, 4, "");
}

void pTEXT(string &s, size_t &i)
{
	progdefaults.macrotextid = true;

	s.replace( i, 6, "");
}

void pCWID(string &s, size_t &i)
{
	progdefaults.macroCWid = true;
	s.replace( i, 6, "");
}

void pRX(string &s, size_t &i)
{
	s.replace (i, 4, "^r");
}

void pTX(string &s, size_t &i)
{
	s.erase(i, 4);
	TransmitON = true;
}

void pTXRX(string &s, size_t &i)
{
	s.erase(i, 6);
	ToggleTXRX = true;
}


void pVER(string &s, size_t &i)
{
	string progname;
	progname = "Fldigi ";
	progname.append(PACKAGE_VERSION);
	s.replace( i, 5, progname );
}

void pCNTR(string &s, size_t &i)
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

void pDECR(string &s, size_t &i)
{
	int  contestval;
	contest_count.count--;
	if (contest_count.count < 0) contest_count.count = 0;
	contestval = contest_count.count;
	s.replace (i, 6, "");
	updateOutSerNo();
}

void pINCR(string &s, size_t &i)
{
	int  contestval;
	contest_count.count++;
	contestval = contest_count.count;
	s.replace (i, 6, "");
	updateOutSerNo();
}

void pXOUT(string &s, size_t &i)
{
	s.replace( i, 6, cutstring(progdefaults.myXchg.c_str()));
}

void pXBEG(string &s, size_t &i)
{
	s.replace( i, 6, "");
	xbeg = i;
}

void pXEND(string &s, size_t &i)
{
	s.replace( i, 6, "");
	xend = i;
}

void pSAVEXCHG(string &s, size_t &i)
{
	save_xchg = true;
	s.replace( i, 10, "");
}

void pLOG(string &s, size_t &i)
{
	qsoSave_cb(0, 0);
	s.replace(i, 5, "");
}

void pLNW(string &s, size_t &i)
{
	s.replace(i, 5, "^L");
}

void pCLRLOG(string &s, size_t &i)
{
	s.replace(i, 10, "^C");
}

void pMODEM_compat(string &s, size_t &i)
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

void pMODEM(string &s, size_t &i)
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
				set_rtty_baud((int)args[1]);
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
		init_modem(mode_info[m].mode);

	s.erase(i, o[0].rm_eo - i);
}

void pAFC(string &s, size_t &i)
{
  size_t endbracket = s.find('>',i);
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
  s.replace(i, endbracket - i + 1, "");
}

void pLOCK(string &s, size_t &i)
{
  size_t endbracket = s.find('>',i);
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
  s.replace(i, endbracket - i + 1, "");
}

void pTX_RSID(string &s, size_t &i)
{
  size_t endbracket = s.find('>',i);
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
  s.replace(i, endbracket - i + 1, "");
}

void pRX_RSID(string &s, size_t &i)
{
  size_t endbracket = s.find('>',i);
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
void pTALK(string &s, size_t &i)
{
  size_t endbracket = s.find('>',i);
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

void pSRCHUP(string &s, size_t &i)
{
	s.replace( i, 8, "");
	active_modem->searchUp();
}

void pSRCHDN(string &s, size_t &i)
{
	s.replace( i, 8, "");
	active_modem->searchDown();
}

void pGOHOME(string &s, size_t &i)
{
	s.replace( i, 8, "");
	if (active_modem == cw_modem)
		active_modem->set_freq(progdefaults.CWsweetspot);
	else if (active_modem == rtty_modem)
		active_modem->set_freq(progdefaults.RTTYsweetspot);
	else
		active_modem->set_freq(progdefaults.PSKsweetspot);
}

void pGOFREQ(string &s, size_t &i)
{
	size_t endbracket = s.find('>',i);
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


void set_macro_env(void)
{
	enum {
#ifndef __WOE32__
	       PATH, FLDIGI_RX_IPC_KEY, FLDIGI_TX_IPC_KEY,
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
		{ "PATH", "" },
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
	// PATH
	static string path = ScriptsDir;
	path.erase(path.length()-1,1);
	const char* p;
	if ((p = getenv("PATH")))
		path.append(":").append(p);
	env[PATH].val = path.c_str();

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
void pEXEC(string &s, size_t &i)
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
void pEXEC(string& s, size_t& i)
{
	size_t end = s.find("</EXEC>", i);
	if (end != string::npos)
		s.erase(i, end + strlen("</EXEC>") - i);
	LOG_WARN("Ignoring unimplemented EXEC macro");
}
#endif // !__MINGW32__

void MAPIT(int how)
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

void pMAPIT(string &s, size_t &i)
{
	size_t endbracket = s.find('>',i);
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

void pSTOP(string &s, size_t &i)
{
	s.erase(i, s.find('>', i) + 1 - i);
	expand = false;
}

void pCONT(string &s, size_t &i)
{
	s.erase(i, s.find('>', i) + 1 - i);
	expand = true;
}

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
//		for (int i = 0; i < 12; i++) {
//			btnMacro[i]->label( name[i].c_str());
//			btnMacro[i]->redraw_label();
//		}
//		return 0;
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
			if (mNumber < 12) {
				FL_LOCK_D();
				btnMacro[mNumber]->label( (name[mNumber]).c_str());
				FL_UNLOCK_D();
			}
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

	for (int row = 0; row < NUMKEYROWS; row++) {
		for (int i = 0; i < NUMMACKEYS; i++) {
			btnMacro[row * NUMMACKEYS + i]->label(macros.name[(NUMKEYROWS - row - 1)*NUMMACKEYS + i].c_str());
			btnMacro[row * NUMMACKEYS + i]->redraw_label();
		}
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

void MACROTEXT::loadnewMACROS(string &s, size_t &i)
{
	size_t endbracket = s.find('>',i);
	string fname = s.substr(i+8, endbracket - i - 8);
	if (fname.length() > 0) {
		loadMacros(fname);
		progStatus.LastMacroFile = fl_filename_name(fname.c_str());
	}
	s.replace(i, endbracket - i + 1, "");
	showMacroSet();
}

string MACROTEXT::expandMacro(int n)
{
	size_t idx = 0;
	expand = true;
	TransmitON = false;
	ToggleTXRX = false;
	mNbr = n;
	expanded = text[n];
	MTAGS *pMtags;

	xbeg = xend = -1;
	save_xchg = false;
	progStatus.repeatMacro = -1;
	text2repeat.clear();
	idleTime = 0;
	waitTime = 0;
	tuneTime = 0;

	while ((idx = expanded.find('<', idx)) != string::npos) {
		if (expanded.find("<MACROS:",idx) == idx) {
			loadnewMACROS(expanded, idx);
			idx++;
			continue;
		}
		// we must handle this specially
		if (expanded.find("<CONT>", idx) == idx)
			pCONT(expanded, idx);
		if (!expand) {
			idx++;
			continue;
		}

		pMtags = mtags;
		while (pMtags->mTAG != 0) {
			if (expanded.find(pMtags->mTAG,idx) == idx) {
				pMtags->fp(expanded,idx);
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

void MACROTEXT::execute(int n)
{
	text2save = text2send = expandMacro(n);

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
	expandMacro(n);
printf("%s\n",text2repeat.c_str());
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


string mtext =
"//fldigi macro definition file extended\n\
// This file defines the macro structe(s) for the digital modem program, fldigi\n\
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
