#include <config.h>

#include "macros.h"

#include "main.h"

#include "fl_digi.h"
#include "configuration.h"
#include "Config.h"
#include "logger.h"
#include "newinstall.h"

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
// clear all of the macros
	for (int i = 0; i < 20; i++) {
		snprintf(szTemp, sizeof(szTemp), "%d", i+1);
		name[i] = "Macro ";
		name[i] = name[i] + szTemp;
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
			if (mNumber < 0 || mNumber > 19)
				break;
			name[mNumber] = mLine.substr(idx+1, 9);
			if (mNumber < 10) {
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
"//fldigi macro definition file\n\
// This file defines the macro structe(s) for the digital modem program, fldigi\n\
// It also serves as a basis for any macros that are written by the user\n\
//\n\
// The top line of this file should always be the first line in every macro definition file (.mdf)\n\
// for the fldigi program to recognize it as such.\n\
//\n\
// Macros are short text statements that contain imbedded references to text data used by the\n\
// program fldigi.  The imbedded references are always prefaced by the carat(^) symbol and consist\n\
// of single letter codes.  All references to the local system are in lower case and to the remote\n\
// system or global values are in upper case.\n\
//\n\
//\n\
// <CALL>  remote call\n\
// <LDT>  local date time Zone\n\
//     format : %x %H:%M %Z\n\
//     where %x is preferred short form date ie: MM/DD/YY or DD/MM/YYYY etc\n\
//           %H is ho with leading 0\n\
//           %M is minute with leading 0\n\
//           %Z is abbreviated time zone ie: EDT or GMT\n\
// <ILDT> local date and time in iso-8601 format: %Y-%m-%d %H:%M:%S%z\n\
// <ZDT>  UTC date time Zone\n\
//     format : %x %H:%M %Z\n\
// <IZDT> UTC date and time in iso-8601 format: %Y-%m-%d %H:%M:%SZ\n\
// <FREQ>  my frequency\n\
// <ID>  send Mode Idenfier - waterfall script\n\
// <TEXT> send video text - waterfall script\n\
// <CWID> send CW identifer at end of transmission (not for CW)\n\
// <LOC>  remote locator\n\
// <LOG>  submit QSO data to logbook program & clear the QSO data fields\n\
//     Not constrained to a particular position in the macro.\n\
//     Action takes place when macro is expanded, so effect is seen immediately\n\
//     after executing the macro which contains this macro reference.\n\
//\n\
// <MODE>  my mode\n\
// <MYCALL>  my call\n\
// <MYLOC>  my locator\n\
// <MYNAME>  my name\n\
// <MYQTH>  my qth\n\
// <MYRST>  my RST\n\
// <NAME>  remote name\n\
// <QTH>  remote qth\n\
// <RST>  remote RST\n\
// <RX>  retn to receive\n\
// <TX>  start transmit\n\
// <VERSION>  Fldigi + version\n\
//\n\
// Contest macro definitions:\n\
// <CNTR>  substitute the contest counter - no change in value\n\
// <DECR>  decrement the contest counter - no substitution\n\
// <INCR>  increment the contest counter - no substitution\n\
//\n\
// Autorepeat macro definition:\n\
// <TIMER>NNN autorepeat this macro after NNN seconds\n\
// NNN must be terminated by either a space ' ' or a linefeed\n\
// it can appear anywhere in the macro defintion.\n\
// see macro # 19 for an example of an auto-cq repeating macro\n\
//\n\
//\n\
// Local references are specified ding the program configation and can be changed during \n\
// program operation.\n\
//\n\
// Remote references are all part of the qso log field definitions and are routinely changed \n\
// from contact to contact.\n\
//\n\
// Global references are to things like Greenwich Mean Time\n\
//\n\
// Each new macro begins with a macro specifier line as: /$ nn MACRONAME\n\
//   where 'nn' specifies the macro number.  Macros numbered 0..9 refer to function key 1 to 10\n\
//   respectively.  Macros numbered 10..19 refer to \"alt\" function keys 1 to 10.  Undefined Macros\n\
//   will also contain an empty character string and thus produce no output when invoked.\n\
// MACRONAME can be as long as you want, however only the first 8 characters will be displayed on\n\
//   the associated function key button.\n\
//\n\
// You can put the macro definition on multiple lines.  These lines will be concatenated\n\
// into a single line unless you put the new-line pair \"\\n\" at the end of the line\n\
// that you want to terminate a line ding transmission.\n\
//\n\
// Macro definitions do not need to be in numerical order.\n\
// Macro definitions may be skipped or you can truncate the file to less than\n\
// the full macro set of 20.\n\
//\n\
// I recommend using a copy of this file suitably modified for all of yo macro\n\
// definition files.  Just fill in the appropriate fields and delete those you do not\n\
// need for that macro set.\n\
//\n\
//\n\
// Let the Macros begin!\n\
//\n\
";

void MACROTEXT::saveMacros(string fname) {
	string work;
	ofstream mfile(fname.c_str());
	mfile << mtext;
	for (int i = 0; i < 20; i++) {
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
