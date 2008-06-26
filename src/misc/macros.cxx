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
#include "fileselect.h"

#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <string>
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
void pEXEC(string &, size_t &);
void pSTOP(string &, size_t &);
void pCONT(string &, size_t &);
void pGET(string &, size_t &);

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
{"<EXEC>",		pEXEC},
{"<STOP>",		pSTOP},
{"<CONT>",		pCONT},
{"<GET>",	pGET},
{0, 0}
};

static bool expand;
static bool GET = false;

size_t mystrftime( char *s, size_t max, const char *fmt, const struct tm *tm) {
	return strftime(s, max, fmt, tm);
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
				init_modem_sync(mode_info[m].mode);
			break;
		}
	}
	s.erase(i, k-i);
}

void set_env(void)
{
	enum { PATH, FLDIGI_RX_IPC_KEY, FLDIGI_TX_IPC_KEY, FLDIGI_VERSION,
	       FLDIGI_PID, FLDIGI_CONFIG_DIR,

	       FLDIGI_MY_CALL, FLDIGI_MY_NAME, FLDIGI_MY_LOCATOR,

	       FLDIGI_MODEM, FLDIGI_MODEM_LONG_NAME, FLDIGI_DIAL_FREQUENCY,
	       FLDIGI_AUDIO_FREQUENCY, FLDIGI_FREQUENCY,

	       FLDIGI_LOG_FREQUENCY, FLDIGI_LOG_TIME, FLDIGI_LOG_CALL, FLDIGI_LOG_NAME,
	       FLDIGI_LOG_RST_IN, FLDIGI_LOG_RST_OUT, FLDIGI_LOG_QTH, FLDIGI_LOG_LOCATOR,
	       FLDIGI_LOG_NOTES, FLDIGI_AZ, ENV_SIZE
	};

	struct {
		const char* var;
		const char* val;
	} env[] = {
		{ "PATH", "" },
		{ "FLDIGI_RX_IPC_KEY", "" },
		{ "FLDIGI_TX_IPC_KEY", "" },
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
		{ "FLDIGI_LOG_TIME", inpTime->value() },
		{ "FLDIGI_LOG_CALL", inpCall->value() },
		{ "FLDIGI_LOG_NAME", inpName->value() },
		{ "FLDIGI_LOG_RST_IN", inpRstIn->value() },
		{ "FLDIGI_LOG_RST_OUT", inpRstOut->value() },
		{ "FLDIGI_LOG_QTH", inpQth->value() },
		{ "FLDIGI_LOG_LOCATOR", inpLoc->value() },
		{ "FLDIGI_LOG_NOTES", inpNotes->value() },
		{ "FLDIGI_AZ", inpAZ->value() }
	};

	// PATH
	static string path;
	if (path.length() == 0) {
		path = HomeDir;
		if (*path.rbegin() != '/')
			path += '/';
		path.append("scripts");
		const char* p;
		if ((p = getenv("PATH")))
			path.append(":").append(p);
	}
	env[PATH].val = path.c_str();

	// IPC keys
        char key[2][8];
	snprintf(key[0], sizeof(key[0]), "%d", progdefaults.rx_msgid);
	env[FLDIGI_RX_IPC_KEY].val = key[0];
	snprintf(key[1], sizeof(key[1]), "%d", progdefaults.tx_msgid);
	env[FLDIGI_TX_IPC_KEY].val = key[1];

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
#ifndef NDEBUG
	unsetenv("FLDIGI_NO_EXEC");
	unsetenv("MALLOC_CHECK_");
	unsetenv("MALLOC_PERTURB_");
#endif

	for (size_t j = 0; j < ENV_SIZE; j++)
		setenv(env[j].var, env[j].val, 1);
}

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
		perror("pipe");
		return;
	}
	pid_t pid;
	switch (pid = fork()) {
	case -1:
		perror("fork");
		return;
	case 0: // child
		close(pfd[0]);
		if (dup2(pfd[1], STDOUT_FILENO) != STDOUT_FILENO) {
			perror("dup2");
			exit(EXIT_FAILURE);
		}
		close(pfd[1]);
		set_env();
		execl("/bin/sh", "sh", "-c", s.substr(start, end-start).c_str(), (char *)NULL);
		perror("execl");
		exit(EXIT_FAILURE);
	}
	// parent
	close(pfd[1]);
	FILE* fp = fdopen(pfd[0], "r");
	if (!fp) {
		perror("fdopen");
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

int MACROTEXT::loadMacros(string filename)
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
		createDotFldigi();
		for (int i = 0; i < 12; i++) {
			btnMacro[i]->label( name[i].c_str());
			btnMacro[i]->redraw_label();
		}
		return 0;
//		mFile.open(filename.c_str());
//		if (!mFile)
//			return -1;
	}
	
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
	return 0;
}

void MACROTEXT::loadDefault()
{
	int erc;
	string Filename = HomeDir;
	Filename.append("macros.mdf");
	if ((erc = loadMacros(Filename)) != 0)
#ifndef __CYGWIN__
		printf("Error #%d loading %s\n", erc, Filename.c_str());
#else
	;
#endif
}

void MACROTEXT::openMacroFile()
{
	string deffilename = HomeDir;
	deffilename.append("/macros.mdf");
    const char *p = FSEL::select("Open macro file", "Fldigi macro definition file\t*.mdf", deffilename.c_str());
    if (p)
		loadMacros(p);
}

void MACROTEXT::saveMacroFile()
{
	string deffilename = HomeDir;
	deffilename.append("/macros.mdf");
    const char *p = FSEL::saveas("Save macro file", "Fldigi macro definition file\t*.mdf", deffilename.c_str());
    if (p)
		saveMacros(p);
}


string MACROTEXT::expandMacro(int n)
{
	size_t idx = 0;
	expand = true;
	TransmitON = false;
	mNbr = n;
	expanded = text[n];
	MTAGS *pMtags;

	while ((idx = expanded.find('<', idx)) != string::npos) {
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
		if (pos1 != string::npos && pos2 != string::npos) {
			pos1 += 5;
			inpName->value(expanded.substr(pos1, pos2 - pos1).c_str());
		}
		if (pos2 != string::npos) {
			pos2 += 4;
			inpQth->value(expanded.substr(pos2).c_str());
		}
		GET = false;
		return "";
	}
		
	return expanded;
}

void MACROTEXT::execute(int n) 
{
	TransmitText->add( (expandMacro(n)).c_str() );
	if ( TransmitON ) {
		active_modem->set_stopflag(false);
		start_tx();
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
