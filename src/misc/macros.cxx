#include <config.h>

#include "macros.h"

#include "main.h"

#include "fl_digi.h"
#include "configuration.h"
#include "confdialog.h"
#include "logger.h"
#include "newinstall.h"
#include "globals.h"

#include <FL/Fl.H>
#include "File_Selector.h"

#include <string>
#include <ctime>
#include <iostream>
#include <fstream>

MACROTEXT macros;
CONTESTCNTR contest_count;
static bool TransmitON = false;
int mNbr;

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
void pLDT(string &, size_t &);
void pILDT(string &, size_t &);
void pZDT(string &, size_t &);
void pIZDT(string &, size_t &);
void pID(string &, size_t &);
void pTEXT(string &, size_t &);
void pCWID(string &, size_t &);
void pRX(string &, size_t &);
void pTX(string &, size_t &);
void pVER(string &, size_t &);
void pCNTR(string &, size_t &);
void pDECR(string &, size_t &);
void pINCR(string &, size_t &);
void pLOG(string &, size_t &);
void pTIMER(string &, size_t &);
void pMODEM(string &, size_t &);

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
{"<LDT>",		pLDT},
{"<ILDT>",		pILDT},
{"<ZDT>",		pZDT},
{"<IZDT>",		pIZDT},
{"<ID>",		pID},
{"<TEXT>",		pTEXT},
{"<CWID>",		pCWID},
{"<RX>",		pRX},
{"<TX>",		pTX},
{"<VER>",		pVER},
{"<CNTR>",		pCNTR},
{"<DECR>",		pDECR},
{"<INCR>",		pINCR},
{"<LOG>",		pLOG},
{"<TIMER>",		pTIMER},
{"<MODEM>",		pMODEM},
{0, 0}
};

size_t mystrftime( char *s, size_t max, const char *fmt, const struct tm *tm) {
	return strftime(s, max, fmt, tm);
}


void pCALL(string &s, size_t &i)
{
	s.replace( i, 6, inpCall->value() );
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

void pRST(string &s, size_t &i)
{
	s.replace( i, 5, inpRstOut->value() );
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
	mystrftime(szDt, 79, "%Y-%m-%d %H:%M:%S%z", &sTime);
	s.replace( i, 6, szDt);
}

void pZDT(string &s, size_t &i)
{
	char szDt[80];
	time_t tmptr;
	tm sTime;
	time (&tmptr);
	gmtime_r(&tmptr, &sTime);
	mystrftime(szDt, 79, "%x %H:%M %Z", &sTime);
	s.replace( i, 5, szDt);
}

void pIZDT(string &s, size_t &i)
{
	char szDt[80];
	time_t tmptr;
	tm sTime;
	time (&tmptr);
	gmtime_r(&tmptr, &sTime);
	mystrftime(szDt, 79, "%Y-%m-%d %H:%M:%SZ", &sTime);
	s.replace( i, 6, szDt);
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
	contestval = contest_count.count + progdefaults.ContestStart;
	contest_count.Format(progdefaults.ContestDigits, progdefaults.UseLeadingZeros);
	snprintf(contest_count.szCount, sizeof(contest_count.szCount), contest_count.fmt.c_str(), contestval);
	s.replace (i, 6, contest_count.szCount);
}

void pDECR(string &s, size_t &i)
{
	int  contestval;
	contest_count.count--;
	contestval = contest_count.count + progdefaults.ContestStart;
	s.replace (i, 6, "");
	snprintf(contest_count.szCount, sizeof(contest_count.szCount), contest_count.fmt.c_str(), contestval);
	snprintf(contest_count.szDisp, sizeof(contest_count.szDisp), "Next Nbr: %s", contest_count.szCount);
	put_status(contest_count.szDisp);
}

void pINCR(string &s, size_t &i)
{
	int  contestval;
	contest_count.count++;
	contestval = contest_count.count + progdefaults.ContestStart;
	s.replace (i, 6, "");
	snprintf(contest_count.szCount, sizeof(contest_count.szCount), contest_count.fmt.c_str(), contestval);
	snprintf(contest_count.szDisp, sizeof(contest_count.szDisp), "Next Nbr: %s", contest_count.szCount);
	put_status(contest_count.szDisp);
}

void pLOG(string &s, size_t &i)
{
	submit_log();
	s.replace(i, 5, "");
	clearQSO();
}

void pTIMER(string &s, size_t &i)
{
	int number;
	sscanf(s.substr(i+7).c_str(), "%d", &number);
	size_t i2;
	i2 = s.find(" ",i);
	if (i2 == string::npos)
		i2 = s.find("\n", i);
	s.replace (i, i2 - i, "");
	progdefaults.timeout = number;
	progdefaults.macronumber = mNbr;
	progdefaults.useTimer = true;
}

void pMODEM(string &s, size_t &i)
{
	if ((i = s.find('>', i)) == string::npos)
		return;

	size_t len = s.length();
	while (++i < len)
	    if (!isspace(s[i]))
		break;
	size_t j = i;
	while (++j < len)
	    if (isspace(s[j]))
		break;

	string name = s.substr(i, j-i);
	for (j = 0; j < NUM_MODES; j++) {
		if (name == mode_info[j].sname) {
			if (active_modem->get_mode() != mode_info[j].mode)
				init_modem(mode_info[j].mode);
			s.clear();
			break;
		}
	}
}

int MACROTEXT::loadMacros(string filename)
{
	string mLine;
	string mName;
	string mDef;
	bool   inMacro = false;
	int    mNumber = 0;
	unsigned long int	   crlf; // 64 bit cpu's
	char   szTemp[10];
	char   szLine[4096];
	bool   convert = false;
	
	ifstream mFile(filename.c_str());
	
	if (!mFile) {
		createDotFldigi();
		mFile.open(filename.c_str());
		if (!mFile)
			return -1;
	}
	
	mFile.getline(szLine, 4095);
	mLine = szLine;
	if (mLine.find("//fldigi macro definition file") != 0) {
		mFile.close();
		return -1;
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
				btnMacro[mNumber]->label( (macros.name[mNumber]).c_str());
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
	return 0;
}

void MACROTEXT::loadDefault()
{
	string Filename = HomeDir;
	Filename.append("macros.mdf");
	loadMacros(Filename);
}

void MACROTEXT::openMacroFile()
{
	string deffilename = HomeDir;
	deffilename.append("/macros.mdf");
    char *p = File_Select("Open macro file", "*.mdf", deffilename.c_str(), 0);
    if (p)
		loadMacros(p);
}

void MACROTEXT::saveMacroFile()
{
	string deffilename = HomeDir;
	deffilename.append("/macros.mdf");
    char *p = File_Select("Save macro file", "*.mdf", deffilename.c_str(), 0);
    if (p)
		saveMacros(p);
}

string MACROTEXT::expandMacro(int n)
{
	size_t idx = 0;
	
	TransmitON = false;
	mNbr = n;
	expanded = text[n];
	MTAGS *pMtags;

	while ((idx = expanded.find('<', idx)) != string::npos) {
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
	return expanded;
}

void MACROTEXT::execute(int n) 
{
	TransmitText->add( (expandMacro(n)).c_str() );
	if ( TransmitON ) {
		active_modem->set_stopflag(false);
		fl_lock(&trx_mutex);
		trx_state = STATE_TX;
		fl_unlock(&trx_mutex);
		wf->set_XmtRcvBtn(true);
		TransmitON = false;
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

void MACROTEXT::saveMacros(string fname) {
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
