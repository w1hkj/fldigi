// ----------------------------------------------------------------------------
// rigsupport.cxx - support functions file
//
// Copyright (C) 2007-2009
//		Dave Freese, W1HKJ
// Copyright (C) 2008-2009
//		Stelios Bounanos, M0GLD
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

#include <fstream>
#include <sstream>
#include <vector>
#include <list>
#include <map>
#include <algorithm>
#include <iterator>
#include <cstring>

#include "rigsupport.h"
#include "rigxml.h"
#include "rigio.h"

#include "threads.h"
#include "main.h"
#include "fl_digi.h"
#include "trx.h"

#include "configuration.h"

#include "globals.h"

#include "debug.h"

#include "gettext.h"

extern void n3fjp_set_freq(long);
extern bool n3fjp_connected;

LOG_FILE_SOURCE(debug::LOG_RIGCONTROL);

using namespace std;

string windowTitle;

vector<qrg_mode_t> freqlist;

const unsigned char nfields = 5;//4;
int fwidths[nfields];
enum { max_rfcarrier, max_rmode, max_mode, max_carrier };

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

void qso_selMode(rmode_t m)
{
	qso_opMODE->value(mode_names[m].c_str());
}

string modeString(rmode_t m)
{
	return mode_names[m].c_str();
}

void initOptionMenus()
{
	qso_opMODE->clear();

	list<MODE>::iterator MD;
	list<MODE> *pMD = 0;

	if (lmodes.empty() == false)
		pMD = &lmodes;
	else if (lmodeCMD.empty() == false)
		pMD = &lmodeCMD;
printf("initOptionMenus()\n");
	if (pMD) {
		MD = pMD->begin();
		while (MD != pMD->end()) {
printf("adding mode: %s\n", (*MD).SYMBOL.c_str());
			qso_opMODE->add( (*MD).SYMBOL.c_str());
			MD++;
		}
		qso_opMODE->activate();
		qso_opMODE->index(0);
	}
	else {
		qso_opMODE->deactivate();
	}

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
printf("adding BW: %s\n", (*bw).SYMBOL.c_str());
			qso_opBW->add( (*bw).SYMBOL.c_str());
			bw++;
		}
		qso_opBW->activate();
		qso_opBW->index(0);
	}
	else {
		qso_opBW->deactivate();
	}
}

void clearList()
{
	freqlist.clear();
	qso_opBrowser->clear();
}

void updateSelect()
{
	if (freqlist.empty())
		return;
	for (size_t i = 0; i < freqlist.size(); i++) {
		qso_opBrowser->add(freqlist[i].str().c_str());
	}
}

size_t updateList(long rf, int freq, string rmd, trx_mode md, string usage = "")
{
	qrg_mode_t m;
	m.rmode = rmd;
	m.mode = md;
	m.rfcarrier = rf;
	m.carrier = freq;
	m.usage = usage;

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

	if (strlen(qso_opMODE->value()))
		m.rmode = qso_opMODE->value();

	if (active_modem) {
		m.carrier = active_modem->get_freq();
		m.mode = active_modem->get_mode();
	}
	return updateList(val, m.carrier, m.rmode, m.mode);
}

bool readFreqList()
{
	ifstream freqfile((HomeDir + "frequencies2.txt").c_str());
	if (!freqfile) {
		LOG_ERROR("Could not open %s", (HomeDir + "frequencies2.txt").c_str());
		return false;
	}

	string line;
	qrg_mode_t m;
	while (!getline(freqfile, line).eof()) {
		LOG_INFO("%s", line.c_str());
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
	if (freqlist.empty()) return;

	ofstream freqfile((HomeDir + "frequencies2.txt").c_str());
	if (!freqfile) {
		LOG_ERROR("Could not open %s", (HomeDir + "frequencies2.txt").c_str());
		return;
	}
	freqfile << "# rfcarrier rig_mode carrier mode usage\n";

	copy(	freqlist.begin(),
			freqlist.end(),
			ostream_iterator<qrg_mode_t>(freqfile, "\n") );

	freqfile.close();
}

void build_frequencies2_list()
{
	if (!freqlist.empty()) saveFreqList();

	clearList();
	// calculate the column widths
	memset(fwidths, 0, sizeof(fwidths));
	// these need to be a little wider than fl_width thinks
	fwidths[max_rmode] = fwidths[max_mode] = 
	fwidths[max_carrier] = fwidths[max_rfcarrier]= 15;

	fwidths[max_rfcarrier] += (int)ceil(fl_width("999999.999"));

	fwidths[max_rmode] += (int)ceil(fl_width("XXXXXX"));

	fwidths[max_carrier] += (int)ceil(fl_width("8888"));

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

	if (readFreqList() == true)
		return;

	updateList (1807000L, 1000, "USB", MODE_PSK31 );
	updateList (3505000L, 800, "USB", MODE_CW);
	updateList (3580000L, 1000, "USB", MODE_PSK31 );
	updateList (1000500L, 800, "USB", MODE_CW);
	updateList (10135000L, 1000, "USB", MODE_PSK31 );
	updateList (7005000L, 800, "USB", MODE_CW);
	updateList (7030000L, 1000, "USB", MODE_PSK31 );
	updateList (7070000L, 1000, "USB", MODE_PSK31 );
	updateList (14005000L, 800, "USB", MODE_CW);
	updateList (14070000L, 1000, "USB", MODE_PSK31 );
	updateList (18100000L, 1000, "USB", MODE_PSK31 );
	updateList (21005000L, 800, "USB", MODE_CW);
	updateList (21070000L, 1000, "USB", MODE_PSK31 );
	updateList (24920000L, 1000, "USB", MODE_PSK31 );
	updateList (28005000L, 800, "USB", MODE_CW);
	updateList (28120000, 1000, "USB", MODE_PSK31 );
	updateSelect();

}

int cb_qso_opMODE()
{
	if (connected_to_flrig) {
		set_flrig_mode(qso_opMODE->value());
		return 0;
	}
#if USE_HAMLIB
	if (progdefaults.chkUSEHAMLIBis)
		hamlib_setmode(mode_nums[qso_opMODE->value()]);
	else
#endif
	if (progdefaults.chkUSERIGCATis)
		rigCAT_setmode(qso_opMODE->value());
	else
		noCAT_setmode(qso_opMODE->value());
	return 0;
}

int cb_qso_opBW()
{
	if (connected_to_flrig)
		set_flrig_bw(qso_opBW->index(), -1);
	else if (progdefaults.chkUSERIGCATis)
		rigCAT_setwidth(qso_opBW->value());
	else
		noCAT_setwidth(qso_opBW->value());
	return 0;
}

int  cb_qso_btnBW1()
{
	qso_btnBW1->hide();
	qso_opBW1->hide();
	qso_btnBW2->show();
	qso_opBW2->show();
	return 0;
}

int cb_qso_btnBW2()
{
	qso_btnBW2->hide();
	qso_opBW2->hide();
	qso_btnBW1->show();
	qso_opBW1->show();
	return 0;
}

int cb_qso_opBW1()
{
//printf("opBW1 %d:%s\n", qso_opBW1->index(), qso_opBW1->value());
	set_flrig_bw(qso_opBW2->index(), qso_opBW1->index());
	return 0;
}

int cb_qso_opBW2()
{
//printf("opBW2 %d:%s\n", qso_opBW2->index(), qso_opBW2->value());
	set_flrig_bw(qso_opBW2->index(), qso_opBW1->index());
	return 0;
}

void sendFreq(long int f)
{
	if (connected_to_flrig)
		set_flrig_freq(f);
#if USE_HAMLIB
	else if (progdefaults.chkUSEHAMLIBis)
		hamlib_setfreq(f);
#endif
	else if (progdefaults.chkUSERIGCATis)
		rigCAT_setfreq(f);
	else
		noCAT_setfreq(f);

	if (n3fjp_connected)
		n3fjp_set_freq(f);
}

void qso_movFreq(Fl_Widget* w, void *data)
{
	cFreqControl *fc = (cFreqControl *)w;
	long int f;
	long restore = reinterpret_cast<long>(data);
	f = fc->value();
	if (fc == qsoFreqDisp1) {
		qsoFreqDisp2->value(f);
		qsoFreqDisp3->value(f);
	} else if (fc == qsoFreqDisp2) {
		qsoFreqDisp1->value(f);
		qsoFreqDisp3->value(f);
	} else {
		qsoFreqDisp1->value(f);
		qsoFreqDisp2->value(f);
	}

	sendFreq(f);
	if (restore == 1)
		restoreFocus();
	return;
}

void qso_selectFreq(long int rfcarrier, int carrier)
{
	if (rfcarrier > 0) {
		qsoFreqDisp1->value(rfcarrier);
		qsoFreqDisp2->value(rfcarrier);
		qsoFreqDisp3->value(rfcarrier);
		sendFreq(rfcarrier);
	}

	if (carrier > 0) {
		active_modem->set_freq(carrier);
	}
}

void qso_selectFreq()
{
	int n = qso_opBrowser->value();
	if (!n) return;

	n -= 1;
// transceiver frequency
	if (freqlist[n].rfcarrier > 0) {
		qsoFreqDisp1->value(freqlist[n].rfcarrier);
		qsoFreqDisp2->value(freqlist[n].rfcarrier);
		qsoFreqDisp3->value(freqlist[n].rfcarrier);
		sendFreq(freqlist[n].rfcarrier);
	}
// transceiver mode
	if (freqlist[n].rmode != "NONE") {
		qso_opMODE->value(freqlist[n].rmode.c_str());
		cb_qso_opMODE();
	}
// modem type & audio sub carrier
	if (freqlist[n].mode != NUM_MODES) {
		if (freqlist[n].mode != active_modem->get_mode())
			init_modem_sync(freqlist[n].mode);
		if (freqlist[n].carrier > 0)
			active_modem->set_freq(freqlist[n].carrier);
	}
}

void qso_setFreq(long int f)
{
	// transceiver frequency
	if (f > 0) {
		qsoFreqDisp->value(f);
		sendFreq(f);
	}
}

void qso_setFreq()
{
	int n = qso_opBrowser->value();
	if (!n) return;

	n -= 1;
// transceiver frequency
 	qso_setFreq(freqlist[n].rfcarrier);
}

void qso_delFreq()
{
	int v = qso_opBrowser->value() - 1;

	if (v >= 0) {
		freqlist.erase(freqlist.begin() + v);
		qso_opBrowser->remove(v + 1);
	}
}

void qso_addFreq()
{
	long freq = qsoFreqDisp->value();
	if (freq) {
		size_t pos = addtoList(freq);
		qso_opBrowser->insert(pos+1, freqlist[pos].str().c_str());
	}
}

void qso_updateEntry(int i, std::string usage)
{
	int v = i - 1;
	int sz = (int)freqlist.size();
	if ((v >= 0) && (v < sz)) {
		freqlist[v].usage = usage;
	}
}

void setTitle()
{
	update_main_title();
}

bool init_Xml_RigDialog()
{
	LOG_DEBUG("xml rig");
	initOptionMenus();
	xcvr_title = xmlrig.rigTitle;
	setTitle();
	return true;
}

bool init_NoRig_RigDialog()
{
	LOG_DEBUG("no rig");
	qso_opBW->clear();
	qso_opBW->add("  ");
	qso_opBW->index(0);
	qso_opBW->redraw();
	qso_opBW->deactivate();
	qso_opMODE->clear();

//printf("init_NoRig_RigDialog()\n");
	for (size_t i = 0; i < sizeof(modes)/sizeof(modes[0]); i++) {
//printf("adding %s\n", modes[i].name);
		qso_opMODE->add(modes[i].name);
	}
// list of LSB type modes that various xcvrs report via flrig
	LSBmodes.clear();
	LSBmodes.push_back("LSB");
	LSBmodes.push_back("LSB-D");
	LSBmodes.push_back("LSB-D1");
	LSBmodes.push_back("LSB-D2");
	LSBmodes.push_back("LSB-D3");
	LSBmodes.push_back("CW");
	LSBmodes.push_back("LCW");
	LSBmodes.push_back("CW-N");
	LSBmodes.push_back("CWL");
	LSBmodes.push_back("RTTY");
	LSBmodes.push_back("RTTY-L");
	LSBmodes.push_back("PKTLSB");
	LSBmodes.push_back("PKT-L");
	LSBmodes.push_back("USER-L");
	LSBmodes.push_back("DATA-L");
	LSBmodes.push_back("DATA");
	LSBmodes.push_back("D-LSB");

	qso_opMODE->index(3);
	qso_opMODE->activate();

	xcvr_title.clear();
	setTitle();

	return true;
}

#if USE_HAMLIB
bool init_Hamlib_RigDialog()
{
	LOG_DEBUG("hamlib");

	qso_opBW->deactivate();
	qso_opMODE->clear();

	for (size_t i = 0; i < sizeof(modes)/sizeof(modes[0]); i++) {
		mode_nums[modes[i].name] = modes[i].mode;
		mode_names[modes[i].mode] = modes[i].name;
		qso_opMODE->add(modes[i].name);
	}

	xcvr_title = "Hamlib ";
	xcvr_title.append(xcvr->getName());

	setTitle();

	return true;
}
#endif
