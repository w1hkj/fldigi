//########################################################################
//
//  rigsupport.cxx
//
//  rig control - support functions file
//
//  copywrite David Freese, w1hkj@w1hkj.com
//
//########################################################################

#include <config.h>

#include "rigdialog.h"
#include "rigsupport.h"
#include "rigxml.h"
#include "rigio.h"
#include "threads.h"
#include "main.h"

#include "configuration.h"

using namespace std;

Fl_Double_Window *rigcontrol = (Fl_Double_Window *)0;
string windowTitle;

#define LISTSIZE 1000
long int freqlist[LISTSIZE];
int  numinlist = 0;

#if USE_HAMLIB
const char *szmodes[] = {
	"AM",
	"USB",
	"LSB",
	"CW", "CWR",
	"FM",
	"RTTY", "RTTYR", 
	0};
rmode_t modes[] = {
	RIG_MODE_AM,
	RIG_MODE_USB,
	RIG_MODE_LSB,
	RIG_MODE_CW, RIG_MODE_CWR,
	RIG_MODE_FM,
	RIG_MODE_RTTY, RIG_MODE_RTTYR
};
int nummodes = 8;

void selMode(rmode_t m)
{
	int i;
	for (i = 0; i < nummodes; i++)
		if (modes[i] == m)
			break;
	if (i == nummodes)
		return;
	if (opMODE)
		opMODE->value(szmodes[i]);
}

void selFreq(long int f)
{
	if (FreqDisp)
		FreqDisp->value(f);
}

#endif


void initOptionMenus()
{
	opMODE->clear();
	list<MODE>::iterator MD;
	list<MODE> *pMD = 0;
	if (lmodes.empty() == false)
		pMD = &lmodes;
	else if (lmodeCMD.empty() == false)
		pMD = &lmodeCMD;
	
	if (pMD) {
		MD = pMD->begin();
		while (MD != pMD->end()) {
			opMODE->add( (*MD).SYMBOL.c_str());
			MD++;
		}
		opMODE->show();
		opMODE->index(0);
	}
	else
		opMODE->hide();

	opBW->clear();
	list<BW>::iterator bw;
	list<BW> *pBW = 0;
	if (lbws.empty() == false)
		pBW = &lbws;
	else if (lbwCMD.empty() == false)
		pBW = &lbwCMD;
	
	if (pBW) {
		bw = pBW->begin();
		while (bw != pBW->end()) {
			opBW->add( (*bw).SYMBOL.c_str());
			bw++;
		}
		opBW->show();
		opBW->index(0);
	}
	else
		opBW->hide();
}

void sortList() {
	if (!numinlist) return;
	long int temp;
	for (int i = 0; i < numinlist - 1; i++)
		for (int j = i + 1; j < numinlist; j++)
			if (freqlist[i] > freqlist[j]) {
					temp = freqlist[i];
					freqlist[i] = freqlist[j];
					freqlist[j] = temp;
			}
}

void clearList() {
	if (!numinlist) return;
	for (int i = 0; i < LISTSIZE; i++)
		freqlist[i] = 0;

	FreqSelect->clear();
	numinlist = 0;
}

void updateSelect() {
	char szFREQ[20];
	if (!numinlist) return;
	sortList();
	FreqSelect->clear();
	for (int n = 0; n < numinlist; n++) {
		sprintf(szFREQ, "%9.3f", freqlist[n] / 1000.0);
		FreqSelect->add (szFREQ);
	}
}

void addtoList(long val) {
	freqlist[numinlist] = val;
	numinlist++;
}

bool readFreqList()
{
	long int freq;
	string freqfname = HomeDir;
	freqfname.append("frequencies.txt");
	ifstream freqfile(freqfname.c_str(), ios::in);
	if (freqfile) {
		while (!freqfile.eof()) {
			freq = 0;
			freqfile >> freq;
			if (freq > 0)
				addtoList(freq);
		}
		freqfile.close();
		updateSelect();
		if (numinlist) {
			FreqDisp->value(freqlist[0]);
			return true;
		}
	}
	return false;
}

void saveFreqList()
{
    if (!numinlist) 
		return;
	string freqfname = HomeDir;
	freqfname.append("frequencies.txt");
	ofstream freqfile(freqfname.c_str(), ios::out);
	if (freqfile) {
		for (int i = 0; i < numinlist; i++)
			freqfile << freqlist[i] << endl;
		freqfile.close();
	}
}

void buildlist()
{
	if (readFreqList() == true)
		return;
	Fl::lock();
	for (int n = 0; n < 100; n++) {freqlist[n] = 0;}
	addtoList (1807000L);
	addtoList (10135000L);
	addtoList (21070000L);
	addtoList (24920000);
	addtoList (28120000);
	addtoList (50290000);
	addtoList (3580000L);
	addtoList (14070000L);
	addtoList (21000000L);
	addtoList (7070000L);
	addtoList (14000000L);
	addtoList (28000000L);
	addtoList (7000000L);
	addtoList (3500000L);
	addtoList (3662000L);
	addtoList (7030000L);
	updateSelect();
	FreqDisp->value(freqlist[0]);
	Fl::unlock();
}

void setMode() {
#if USE_HAMLIB
	if (progdefaults.chkUSEHAMLIBis)
		hamlib_setmode(modes[opMODE->index()-1]);
	else
#endif		
		rigCAT_setmode (opMODE->value());
}

void setBW() {
	rigCAT_setwidth (opBW->value());
}

int movFreq() {
	long int f;
	f = FreqDisp->value();
#if USE_HAMLIB
	if (progdefaults.chkUSEHAMLIBis)
		hamlib_setfreq(f);
	else
#endif		
		rigCAT_setfreq(f);
	return 0;
}
	
void selectFreq() {
	int n = FreqSelect->value();
	if (!n) return;
	n--;
	FreqDisp->value(freqlist[n]);
	movFreq();
}

void delFreq() {
	if (FreqSelect->value()) {
		int n = FreqSelect->value() - 1;
		for (int i = n; i < numinlist; i ++)
			freqlist[i] = freqlist[i+1];
		freqlist[numinlist - 1] = 0;
		numinlist--;
		updateSelect();
	}
}

void addFreq() {
	long freq = FreqDisp->value();
	if (!freq) return;
	for (int n = 0; n < numinlist; n++) 
		if (freq == freqlist[n]) return;
	addtoList(freq);
	updateSelect();
}	


bool init_Xml_RigDialog()
{
	initOptionMenus();
	clearList();
	buildlist();
	if (windowTitle.length() > 0)
		rigcontrol->label(windowTitle.c_str());
	return true;
}

#if USE_HAMLIB
bool init_Hamlib_RigDialog()
{
	opBW->hide();
	opMODE->clear();
	const char **p;
	p = szmodes;
	while (*p) 
		opMODE->add(*p++);
	clearList();
	buildlist();
	rigcontrol->label("Hamlib Controller");
	return true;
}
#endif

Fl_Double_Window *createRigDialog()
{
	Fl_Double_Window *w;
	w = rig_dialog();
	w->xclass(PACKAGE_NAME);
	return w;
}

/*
void configColor()
{
	uchar red, green, blue;
	{
		FreqDisp->GetONCOLOR(red,green,blue);
		char *title = "LED on color";
		if (fl_color_chooser(title, red, green, blue)) {
			FreqDisp->SetONCOLOR( red, green, blue);
		}
	}
	{
		FreqDisp->GetOFFCOLOR(red,green,blue);
		char *title = "LED off color";
		if (fl_color_chooser(title, red, green, blue)) {
			FreqDisp->SetOFFCOLOR( red, green, blue);
		}
	}
}



*/
