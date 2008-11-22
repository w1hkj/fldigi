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

#include <fstream>
#include <vector>
#include <map>
#include <algorithm>
#include <iterator>
#include <cstring>

#include "rigdialog.h"
#include "rigsupport.h"
#include "rigxml.h"
#include "rigMEM.h"
#include "rigio.h"

#include "threads.h"
#include "main.h"

#include "configuration.h"

#include "globals.h"

#include "debug.h"

using namespace std;

Fl_Double_Window *rigcontrol = (Fl_Double_Window *)0;
string windowTitle;

vector<qrg_mode_t> freqlist;

const unsigned char nfields = 4;
int fwidths[nfields];
enum { max_rfcarrier, max_rmode, max_mode };

#if !USE_HAMLIB

typedef enum {
	RIG_MODE_NONE =  	0,	/*!< '' -- None */
	RIG_MODE_AM =    	(1<<0),	/*!< \c AM -- Amplitude Modulation */
	RIG_MODE_CW =    	(1<<1),	/*!< \c CW -- CW "normal" sideband */
	RIG_MODE_USB =		(1<<2),	/*!< \c USB -- Upper Side Band */
	RIG_MODE_LSB =		(1<<3),	/*!< \c LSB -- Lower Side Band */
	RIG_MODE_RTTY =		(1<<4),	/*!< \c RTTY -- Radio Teletype */
	RIG_MODE_FM =    	(1<<5),	/*!< \c FM -- "narrow" band FM */
	RIG_MODE_WFM =   	(1<<6),	/*!< \c WFM -- broadcast wide FM */
	RIG_MODE_CWR =   	(1<<7),	/*!< \c CWR -- CW "reverse" sideband */
	RIG_MODE_RTTYR =	(1<<8),	/*!< \c RTTYR -- RTTY "reverse" sideband */
	RIG_MODE_AMS =    	(1<<9),	/*!< \c AMS -- Amplitude Modulation Synchronous */
	RIG_MODE_PKTLSB =       (1<<10),/*!< \c PKTLSB -- Packet/Digital LSB mode (dedicated port) */
	RIG_MODE_PKTUSB =       (1<<11),/*!< \c PKTUSB -- Packet/Digital USB mode (dedicated port) */
	RIG_MODE_PKTFM =        (1<<12),/*!< \c PKTFM -- Packet/Digital FM mode (dedicated port) */
	RIG_MODE_ECSSUSB =      (1<<13),/*!< \c ECSSUSB -- Exalted Carrier Single Sideband USB */
	RIG_MODE_ECSSLSB =      (1<<14),/*!< \c ECSSLSB -- Exalted Carrier Single Sideband LSB */
	RIG_MODE_FAX =          (1<<15),/*!< \c FAX -- Facsimile Mode */
	RIG_MODE_SAM =          (1<<16),/*!< \c SAM -- Synchronous AM double sideband */
	RIG_MODE_SAL =          (1<<17),/*!< \c SAL -- Synchronous AM lower sideband */
	RIG_MODE_SAH =          (1<<18),/*!< \c SAH -- Synchronous AM upper (higher) sideband */
	RIG_MODE_DSB =			(1<<19), /*!< \c DSB -- Double sideband suppressed carrier */
} rmode_t;
    
#endif

struct rmode_name_t {
	rmode_t mode;
	const char *name;
} modes[] = {
	{ RIG_MODE_NONE, "NONE" },
	{ RIG_MODE_AM, "AM" },
	{ RIG_MODE_CW, "CW" },
	{ RIG_MODE_USB, "USB" },
	{ RIG_MODE_LSB, "LSB" },
	{ RIG_MODE_RTTY, "RTTY" },
	{ RIG_MODE_FM, "FM" },
	{ RIG_MODE_WFM, "WFM" },
	{ RIG_MODE_CWR, "CWR" },
	{ RIG_MODE_RTTYR, "RTTYR" },
	{ RIG_MODE_AMS, "AMS" },
	{ RIG_MODE_PKTLSB, "PKTLSB" },
	{ RIG_MODE_PKTUSB, "PKTUSB" },
	{ RIG_MODE_PKTFM, "PKTFM" }
//, // C99 trailing commas in enumerations not yet in the C++ standard
//	{ RIG_MODE_ECSSUSB, "ECSSUSB" },
//	{ RIG_MODE_ECSSLSB, "ECSSLSB" },
//	{ RIG_MODE_FAX, "FAX" }
// the above are covered by our requirement that hamlib be >= 1.2.4
#if (defined(RIG_MODE_SAM) && defined(RIG_MODE_SAL) && defined(RIG_MODE_SAH))
	, // C99 trailing commas in enumerations not yet in the C++ standard
	{ RIG_MODE_SAM, "SAM" },
	{ RIG_MODE_SAL, "SAL" },
	{ RIG_MODE_SAH, "SAH" }
#endif
};

map<string, rmode_t> mode_nums;
map<rmode_t, string> mode_names;

void selMode(rmode_t m)
{
	opMODE->value(mode_names[m].c_str());
}

void qso_selMode(rmode_t m)
{
	qso_opMODE->value(mode_names[m].c_str());
}

string modeString(rmode_t m)
{
	return mode_names[m].c_str();
}

void selFreq(long int f)
{
	if (FreqDisp)
		FreqDisp->value(f);
}

void initOptionMenus()
{
	opMODE->clear();
	if (progdefaults.docked_rig_control)
		qso_opMODE->clear();
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
			if (progdefaults.docked_rig_control)
				qso_opMODE->add( (*MD).SYMBOL.c_str());
			MD++;
		}
		opMODE->activate();
		opMODE->index(0);
		if (progdefaults.docked_rig_control) {
			qso_opMODE->activate();
			qso_opMODE->index(0);
		}
	}
	else {
		opMODE->deactivate();
		if (progdefaults.docked_rig_control)
			qso_opMODE->deactivate();
	}
	
	opBW->clear();
	if (progdefaults.docked_rig_control)
		qso_opBW->clear();
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
			if (progdefaults.docked_rig_control)
				qso_opBW->add( (*bw).SYMBOL.c_str());
			bw++;
		}
		opBW->activate();
		opBW->index(0);
		if (progdefaults.docked_rig_control) {
			qso_opBW->activate();
			qso_opBW->index(0);
		}
	}
	else {
		opBW->deactivate();
		if (progdefaults.docked_rig_control)
			qso_opBW->deactivate();
	}
}

void clearList()
{
	freqlist.clear();
	FreqSelect->clear();
	if (progdefaults.docked_rig_control)
		qso_opBrowser->clear();
}

void qso_clearList()
{
	clearList();
}

void updateSelect()
{
	FreqSelect->clear();
	if (freqlist.empty())
		return;
	for (size_t i = 0; i < freqlist.size(); i++) {
		FreqSelect->add(freqlist[i].str().c_str());
		if (progdefaults.docked_rig_control)
			qso_opBrowser->add(freqlist[i].str().c_str());
	}
}

size_t updateList(long rf, int freq, string rmd, trx_mode md)
{
	qrg_mode_t m;
	m.rmode = rmd;
	m.mode = md;
	m.rfcarrier = rf;
	m.carrier = freq;

	freqlist.push_back(m);
	sort(freqlist.begin(), freqlist.end());

	vector<qrg_mode_t>::const_iterator pos = find(freqlist.begin(), freqlist.end(), m);
	if (pos != freqlist.end())
		return pos - freqlist.begin();
	else
		return 0;
	
}

size_t addtoList(long val)
{
	qrg_mode_t m;

	m.rfcarrier = val;

	if (progdefaults.docked_rig_control) {
	if (strlen(qso_opMODE->value()))
		m.rmode = qso_opMODE->value();
	} else {
	if (strlen(opMODE->value()))
		m.rmode = opMODE->value();
	}
	
	if (active_modem) {
		m.carrier = active_modem->get_freq();
		m.mode = active_modem->get_mode();
	}
	return updateList(val, m.carrier, m.rmode, m.mode);	 
}

bool readFreqList()
{
	ifstream freqfile((HomeDir + "frequencies2.txt").c_str());
	if (!freqfile)
		return false;

	string line;
	qrg_mode_t m;
	while (!getline(freqfile, line).eof()) {
		if (line[0] == '#')
			continue;
		istringstream is(line);
		is >> m;
		freqlist.push_back(m);
	}
	sort(freqlist.begin(), freqlist.end());
	updateSelect();
	
	freqfile.close();

	return freqlist.size();
}

void saveFreqList()
{

	ofstream freqfile((HomeDir + "frequencies2.txt").c_str());
	if (!freqfile)
		return;
	freqfile << "# rfcarrier rig_mode carrier mode\n";
	freqfile << "# Post 2.09\n";

	if (freqlist.empty()) {
		freqfile.close();
		return;
	}

	copy(freqlist.begin(), freqlist.end(),
	     ostream_iterator<qrg_mode_t>(freqfile, "\n"));
	freqfile.close();
}

void buildlist()
{
	// calculate the column widths
	memset(fwidths, 0, sizeof(fwidths));
	// these need to be a little wider than fl_width thinks
	fwidths[max_rmode] = fwidths[max_mode] = 10;
	fl_font(FreqSelect->textfont(), FreqSelect->textsize());

	fwidths[max_rfcarrier] += (int)ceil(fl_width("999999.999"));

    fwidths[max_rmode] += (int)ceil(fl_width("XXXX"));

	// find mode with longest shortname
	size_t s, smax = 0, mmax = 0;
	for (size_t i = 0; i < NUM_MODES; i++) {
		s = strlen(mode_info[i].sname);
		if (smax < s) {
			smax = s;
			mmax = i;
		}
	}
	fwidths[max_mode] += (int)ceil(fl_width(mode_info[mmax].sname));

	FreqSelect->column_widths(fwidths);


	if (readFreqList() == true)
		return;
	Fl::lock();
	updateList (1807000L, 1000, "USB", MODE_BPSK31 );
	updateList (3505000L, 800, "USB", MODE_CW);
	updateList (3580000L, 1000, "USB", MODE_BPSK31 );
	updateList (1000500L, 800, "USB", MODE_CW);
	updateList (10135000L, 1000, "USB", MODE_BPSK31 );
	updateList (7005000L, 800, "USB", MODE_CW);
	updateList (7030000L, 1000, "USB", MODE_BPSK31 );
	updateList (7070000L, 1000, "USB", MODE_BPSK31 );
	updateList (14005000L, 800, "USB", MODE_CW);
	updateList (14070000L, 1000, "USB", MODE_BPSK31 );
	updateList (18100000L, 1000, "USB", MODE_BPSK31 );
	updateList (21005000L, 800, "USB", MODE_CW);
	updateList (21070000L, 1000, "USB", MODE_BPSK31 );
	updateList (24920000L, 1000, "USB", MODE_BPSK31 );
	updateList (28005000L, 800, "USB", MODE_CW);
	updateList (28120000, 1000, "USB", MODE_BPSK31 );
	updateSelect();
	FreqDisp->value(freqlist[0].rfcarrier);
	Fl::unlock();
}

void setMode()
{
#if USE_HAMLIB
	if (progdefaults.chkUSEHAMLIBis)
		hamlib_setmode(mode_nums[opMODE->value()]);
	else
#endif
	if (progdefaults.chkUSERIGCATis)
		rigCAT_setmode(opMODE->value());
	else if (progdefaults.chkUSEMEMMAPis)
		rigMEM_setmode(opMODE->value());
	else
		rigCAT_setmode(opMODE->value());
}

int cb_qso_opMODE()
{
	if (!progdefaults.docked_rig_control) return 0;
	
#if USE_HAMLIB
	if (progdefaults.chkUSEHAMLIBis)
		hamlib_setmode(mode_nums[qso_opMODE->value()]);
	else
#endif
	if (progdefaults.chkUSERIGCATis)
		rigCAT_setmode(qso_opMODE->value());
	else if (progdefaults.chkUSEMEMMAPis)
		rigMEM_setmode(qso_opMODE->value());
	else
		rigCAT_setmode(qso_opMODE->value());
	return 0;
}

void setBW()
{
	rigCAT_setwidth (opBW->value());
}

int cb_qso_opBW()
{
	if (!progdefaults.docked_rig_control) return 0;
	rigCAT_setwidth (qso_opBW->value());
	return 0;
}

void sendFreq(long int f)
{
#if USE_HAMLIB
	if (progdefaults.chkUSEHAMLIBis)
		hamlib_setfreq(f);
	else
#endif
	if (progdefaults.chkUSEMEMMAPis)
		rigMEM_set_freq(f);
	else if (progdefaults.chkUSERIGCATis)
		rigCAT_setfreq(f);
	else
		rigCAT_setfreq(f);
}

int movFreq()
{
	long int f;
	f = FreqDisp->value();
	sendFreq(f);
	return 0;
}

int qso_movFreq()
{
	if (!progdefaults.docked_rig_control) return 0;
	long int f;
	f = qsoFreqDisp->value();
	sendFreq(f);
	return 0;
}

void selectFreq()
{
	int n = FreqSelect->value() - 1;

	if (freqlist[n].rmode != "NONE") {
		opMODE->value(freqlist[n].rmode.c_str());
		setMode();
	}

	if (freqlist[n].rfcarrier > 0) {
		FreqDisp->value(freqlist[n].rfcarrier);
		sendFreq(freqlist[n].rfcarrier);
	}

	if (freqlist[n].mode != NUM_MODES) {
		if (freqlist[n].mode != active_modem->get_mode())
			init_modem_sync(freqlist[n].mode);
		if (freqlist[n].carrier > 0)
			active_modem->set_freq(freqlist[n].carrier);
	}
}

void qso_selectFreq()
{
	if (!progdefaults.docked_rig_control) return;
	
	int n = qso_opBrowser->value();
	if (!n) return;
	
	n -= 1;
// transceiver mode
	if (freqlist[n].rmode != "NONE") {
		qso_opMODE->value(freqlist[n].rmode.c_str());
		cb_qso_opMODE();
	}
// transceiver frequency
	if (freqlist[n].rfcarrier > 0) {
		qsoFreqDisp->value(freqlist[n].rfcarrier);
		sendFreq(freqlist[n].rfcarrier);
	}
// modem type & audio sub carrier
	if (freqlist[n].mode != NUM_MODES) {
		if (freqlist[n].mode != active_modem->get_mode())
			init_modem_sync(freqlist[n].mode);
		if (freqlist[n].carrier > 0)
			active_modem->set_freq(freqlist[n].carrier);
	}
}

void qso_setFreq()
{
	if (!progdefaults.docked_rig_control) return;

	int n = qso_opBrowser->value();
	if (!n) return;
	
	n -= 1;
// transceiver frequency
	if (freqlist[n].rfcarrier > 0) {
		qsoFreqDisp->value(freqlist[n].rfcarrier);
		sendFreq(freqlist[n].rfcarrier);
	}
}

void delFreq()
{
	int v = FreqSelect->value() - 1;

	if (v >= 0) {
		freqlist.erase(freqlist.begin() + v);
		FreqSelect->remove(v + 1);
		if (qso_opBrowser)
			qso_opBrowser->remove(v + 1);
	}
}

void qso_delFreq()
{
	if (!progdefaults.docked_rig_control) return;
	
	int v = qso_opBrowser->value() - 1;

	if (v >= 0) {
		freqlist.erase(freqlist.begin() + v);
		FreqSelect->remove(v + 1);
		qso_opBrowser->remove(v + 1);
	}
}

void addFreq()
{
	long freq = FreqDisp->value();
	if (freq) {
		size_t pos = addtoList(freq);
		FreqSelect->insert(pos+1, freqlist[pos].str().c_str());
		if (qso_opBrowser)
				qso_opBrowser->insert(pos+1, freqlist[pos].str().c_str());
	}
}

void qso_addFreq()
{
	if (!progdefaults.docked_rig_control) return;

	long freq = qsoFreqDisp->value();
	if (freq) {
		size_t pos = addtoList(freq);
		FreqSelect->insert(pos+1, freqlist[pos].str().c_str());
		qso_opBrowser->insert(pos+1, freqlist[pos].str().c_str());
	}
}

void setTitle()
{
	if (windowTitle.length() > 0) {
//std::cout << windowTitle.c_str() << std::endl;
		if (rigcontrol) {
			rigcontrol->label(windowTitle.c_str());
		}
		if (progdefaults.docked_rig_control) {
			txtRigName->label(windowTitle.c_str());
			txtRigName->redraw_label();
		}
	} else {
		if (rigcontrol) {
			rigcontrol->label("");
		}
		if (progdefaults.docked_rig_control) {
			txtRigName->label("Non CAT mode");
			txtRigName->redraw_label();
		}
	}
}

bool init_Xml_RigDialog()
{
LOG_DEBUG("xml rig");
	initOptionMenus();
	clearList();
	buildlist();
	setTitle();
	return true;
}

bool init_NoRig_RigDialog()
{
LOG_DEBUG("no rig");
	opBW->deactivate();
	opMODE->clear();

	if (progdefaults.docked_rig_control) {
		qso_opBW->deactivate();
		qso_opMODE->clear();
	}		

	for (size_t i = 0; i < sizeof(modes)/sizeof(modes[0]); i++) {
		opMODE->add(modes[i].name);
		if (progdefaults.docked_rig_control)
			qso_opMODE->add(modes[i].name);
	}
	LSBmodes.clear();
	LSBmodes.push_back("LSB");
	LSBmodes.push_back("CWR");
	LSBmodes.push_back("RTTY");
	LSBmodes.push_back("PKTLSB");

	opMODE->index(0);
	opMODE->activate();

	if (progdefaults.docked_rig_control) {
		qso_opMODE->index(0);
		qso_opMODE->activate();
	}

	clearList();
	buildlist();

	windowTitle = "Rig Not Specified";
	setTitle();
		
	return true;
}

bool init_rigMEM_RigDialog()
{
LOG_DEBUG("Mem Mapped rig");
	opBW->deactivate();
	opMODE->clear();

	if (progdefaults.docked_rig_control) {
		qso_opBW->deactivate();
		qso_opMODE->clear();
	}		

	opMODE->add("LSB");
	opMODE->add("USB");
	opMODE->index(0);
	opMODE->activate();

	if (progdefaults.docked_rig_control) {
		qso_opBW->deactivate();
		qso_opMODE->clear();
		qso_opMODE->add("LSB");
		qso_opMODE->add("USB");
		qso_opMODE->index(0);
		qso_opMODE->activate();
	}

	clearList();
	buildlist();

	windowTitle = "Memory Mapped Rig";
	setTitle();
		
	return true;
}

#if USE_HAMLIB
bool init_Hamlib_RigDialog()
{
LOG_DEBUG("hamlib");
	opBW->deactivate();
	opMODE->clear();

	if (progdefaults.docked_rig_control) {
		qso_opBW->deactivate();
		qso_opMODE->clear();
	}		
	
	for (size_t i = 0; i < sizeof(modes)/sizeof(modes[0]); i++) {
		mode_nums[modes[i].name] = modes[i].mode;
		mode_names[modes[i].mode] = modes[i].name;
		opMODE->add(modes[i].name);
		if (progdefaults.docked_rig_control)
			qso_opMODE->add(modes[i].name);
	}
	clearList();
	buildlist();

	windowTitle = "Hamlib ";
	windowTitle.append(xcvr->getName());
	
	setTitle();

	return true;
}
#endif

Fl_Double_Window *createRigDialog()
{
	Fl_Double_Window *w;
	w = rig_dialog();
	if (!w) return NULL;
	FreqDisp->SetONOFFCOLOR(
		fl_rgb_color(	progdefaults.FDforeground.R, 
						progdefaults.FDforeground.G, 
						progdefaults.FDforeground.B),
		fl_rgb_color(	progdefaults.FDbackground.R, 
						progdefaults.FDbackground.G, 
						progdefaults.FDbackground.B));	
	w->xclass(PACKAGE_NAME);
	return w;
}
