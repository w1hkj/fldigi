// ----------------------------------------------------------------------------
// configuration.cxx
//
// Copyright (C) 2006-2010
//		Dave Freese, W1HKJ
// Copyright (C) 2007-2008
//		Leigh L. Klotz, Jr., WA5ZNU
// Copyright (C) 2007-2010
//		Stelios Bounanos, M0GLD
// Copyright (C) 2013
//		Remi Chateauneu, F4ECW
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

#include "configuration.h"
#include "confdialog.h"
#include "xmlreader.h"
#include "soundconf.h"
#include "fl_digi.h"
#include "main.h"
#include "gettext.h"
#include "nls.h"
#include "icons.h"
#include "rigsupport.h"
#include "contest.h"

#if USE_HAMLIB
	#include "hamlib.h"
	#include "rigclass.h"
#endif

#include "rigio.h"
#include "rigxml.h"
#include "nanoIO.h"
#include "debug.h"

#include <FL/Fl_Tooltip.H>

#include <unistd.h>
#include <iostream>
#include <fstream>
#include <map>
#include <sstream>

#ifdef __linux__
#  include <dirent.h>
#  include <limits.h>
#  include <errno.h>
#  include <glob.h>
#endif
#ifdef __APPLE__
#  include <glob.h>
#endif
#ifndef __CYGWIN__
#  include <sys/stat.h>
#else
#  include <fcntl.h>
#endif

// this tests depends on a modified FL/filename.H in the Fltk-1.3.0
// change
//#  if defined(WIN32) && !defined(__CYGWIN__) && !defined(__WATCOMC__)
// to
//#  if defined(WIN32) && !defined(__CYGWIN__) && !defined(__WATCOMC__) && !defined(__WOE32__)

#include <dirent.h>

const char *szBaudRates[] = {
	"",
	"300","600","1200","2400",
	"4800","9600","19200","38400",
	"57600","115200","230400","460800"};

const char *szBands[] = {
	"",
	"1830", "3580", "7030", "7070", "10138",
	"14070", "18100", "21070", "21080", "24920", "28070", "28120", 0};

std::ostream& operator<<(std::ostream& out, const RGB& rgb)
{
	return out << (int)rgb.R << ' ' << (int)rgb.G << ' ' << (int)rgb.B;
}

std::istream& operator>>(std::istream& in, RGB& rgb)
{
	int i;
	in >> i; rgb.R = i;
	in >> i; rgb.G = i;
	in >> i; rgb.B = i;
	return 	in;

}

std::ostream& operator<<(std::ostream& out, const RGBI& rgbi)
{
	return out << (int)rgbi.R << ' ' << (int)rgbi.G << ' ' << (int)rgbi.B;
}

std::istream& operator>>(std::istream& in, RGBI& rgbi)
{
	int i;
	in >> i; rgbi.R = i;
	in >> i; rgbi.G = i;
	in >> i; rgbi.B = i;
	return 	in;
}

// This allows to put tag elements into containers
class tag_base
{
public:
	tag_base(const char* t, const char* d = "") : tag(t), doc(d) { }
	virtual void write(std::ostream& out) const = 0;
	virtual void read(const char* data) = 0;
	virtual ~tag_base() { }
	const char* tag;
	const char* doc;
};

// This will handle every type that has << and >> stream operators
template <typename T>
class tag_elem : public tag_base
{
public:
	tag_elem(const char* t, const char* d, T& v) : tag_base(t, d), var(v) { }
	void write(std::ostream& out) const
        {
		out << "<!-- " << doc << " -->\n"
		    << '<' << tag << '>' << var << "</" << tag << ">\n\n";
	}
	void read(const char* data)
	{
		std::istringstream iss(data);
		iss >> var;
	}
	T& var;
};

// Instantiate an explicit tag_elem<T> for types that require unusual handling.

// Special handling for strings
template <>
class tag_elem<std::string> : public tag_base
{
public:
	tag_elem(const char* t, const char* d, std::string& s) : tag_base(t, d), str(s) { }
	void write(std::ostream& out) const
        {
		std::string s = str;
		std::string s2 = doc;

		std::string::size_type i = s.find('&');
		while (i != std::string::npos) {
			s.replace(i, 1, "&amp;");
			i = s.find('&', i + 1);
		}
		while ((i = s.find('<')) != std::string::npos)
			s.replace(i, 1, "&lt;");
		while ((i = s.find('>')) != std::string::npos)
			s.replace(i, 1, "&gt;");
		while ((i = s.find('"')) != std::string::npos)
			s.replace(i, 1, "&quot;");
		while ((i = s.find('\'')) != std::string::npos)
			s.replace(i, 1, "&apos;");

		i = s2.find('&');
		while (i != std::string::npos) {
			s2.replace(i, 1, "&amp;");
			i = s2.find('&', i + 1);
		}
		while ((i = s2.find('<')) != std::string::npos)
			s2.replace(i, 1, "&lt;");
		while ((i = s2.find('>')) != std::string::npos)
			s2.replace(i, 1, "&gt;");
		while ((i = s2.find('"')) != std::string::npos)
			s2.replace(i, 1, "&quot;");
		while ((i = s2.find('\'')) != std::string::npos)
			s2.replace(i, 1, "&apos;");

		out << "<!-- " << s2 << " -->\n"
		    << '<' << tag << '>' << s << "</" << tag << ">\n\n";
	}
	void read(const char* data) { str = data; }
	std::string& str;
};

// Special handling for mode bitsets
template <>
class tag_elem<mode_set_t> : public tag_base
{
public:
	tag_elem(const char* t, const char* d, mode_set_t& m) : tag_base(t, d), modes(m) { }
	void write(std::ostream& out) const
        {
		out << "<!-- " << doc << " -->\n" << '<' << tag << '>';
		for (size_t i = 0; i < modes.size(); i++) {
			if (!modes.test(i))
				out << mode_info[i].name << ',';
		}
		out << ",</" << tag << ">\n\n";
	}
	void read(const char* data)
	{
		std::string sdata = data, smode, tstmode;
		modes.set();
		size_t p = sdata.find(",");
		while ((p != std::string::npos) && (p != 0)) {
			smode = sdata.substr(0, p);

			for (size_t i = 0; i < modes.size(); i++) {
				tstmode = mode_info[i].name;
				if (smode == tstmode) {
					modes.set(i,0);
					break;
				}
			}
			sdata.erase(0, p+1);
			p = sdata.find(",");
		}
	}
	mode_set_t& modes;
};

// By redefining the ELEM_ macro, we can control what the CONFIG_LIST macro
// will expand to, and accomplish several things:
// 1) Declare "struct configuration". See ELEM_DECLARE_CONFIGURATION
//    in configuration.h.
// 2) Define progdefaults, the configuration struct that is initialised with
//    fldigi's default options
#define ELEM_PROGDEFAULTS(type_, var_, tag_, doc_, ...) __VA_ARGS__,
// 3) Define an array of tag element pointers
#define ELEM_TAG_ARRAY(type_, var_, tag_, doc_, ...)                                             \
        (*tag_ ? new tag_elem<type_>(tag_, "type: " #type_ "; default: " #__VA_ARGS__ "\n" doc_, \
                                     progdefaults.var_) : 0),

// First define the default config
#undef ELEM_
#define ELEM_ ELEM_PROGDEFAULTS
configuration progdefaults = { CONFIG_LIST };


void configuration::writeDefaultsXML()
{
	std::string deffname(HomeDir);
	deffname.append("fldigi_def.xml");

	std::string deffname_backup(deffname);
	deffname_backup.append("-old");
	remove(deffname_backup.c_str());
	rename(deffname.c_str(), deffname_backup.c_str());

	std::ofstream f(deffname.c_str());
	if (!f) {
		LOG_ERROR("Could not write %s", deffname.c_str());
		return;
	}

	// create an array
#undef ELEM_
#define ELEM_ ELEM_TAG_ARRAY
	tag_base* tag_list[] = { CONFIG_LIST };

	// write all variables with non-empty tags to f
	f << "<FLDIGI_DEFS>\n\n";
	for (size_t i = 0; i < sizeof(tag_list)/sizeof(*tag_list); i++) {
		if (tag_list[i]) {
			tag_list[i]->write(f);
			delete tag_list[i];
		}
	}
	f << "</FLDIGI_DEFS>\n";
	f.close();
}

static void log_excluded_modes(void)
{
return;
	struct {
		mode_set_t* modes;
		const char* msgstr;
	} excluded[] = {
		{ &progdefaults.rsid_rx_modes, "RSID (rx)" },
		{ &progdefaults.rsid_tx_modes, "RSID (tx)" },
		{ &progdefaults.cwid_modes, "CWID" },
		{ &progdefaults.videoid_modes, "VIDEOID" }
	};
	std::string buf;
	for (size_t i = 0; i < sizeof(excluded)/sizeof(*excluded); i++) {
		size_t n = excluded[i].modes->size();
		if (excluded[i].modes->count() == n)
			continue;
		buf.erase();
		for (size_t j = 0; j < n; j++) {
			if (!excluded[i].modes->test(j)) {
				if (!buf.empty())
					buf += ' ';
				buf += mode_info[j].sname;
			}
		}
		LOG(debug::QUIET_LEVEL, debug::LOG_OTHER, "%-10s: %s", excluded[i].msgstr, buf.c_str());
	}
}

bool configuration::readDefaultsXML()
{
	// Decode all RSID modes
	rsid_rx_modes.set();
	// Don't transmit RSID or VideoID for CW, PSK31, RTTY
	rsid_tx_modes.set().reset(MODE_CW).reset(MODE_PSK31).reset(MODE_RTTY);
	videoid_modes = rsid_tx_modes;
	// Don't transmit CWID for CW
	cwid_modes.set().reset(MODE_CW);
	// Show all op modes
	visible_modes.set();

	std::string deffname = HomeDir;
	deffname.append("fldigi_def.xml");
	std::ifstream f(deffname.c_str());
	if (!f)
		return false;

	std::string xmlbuf;

	f.seekg(0, std::ios::end);
	xmlbuf.reserve(f.tellg()); // reserve some space to avoid reallocations
	f.seekg(0, std::ios::beg);

	char line[2048];
	while (f.getline(line, sizeof(line)))
		xmlbuf.append(line).append("\n");
	f.close();

	IrrXMLReader* xml = createIrrXMLReader(new IIrrXMLStringReader(xmlbuf));
	if (!xml)
		return false;

	// create a TAG_NAME -> ELEMENT map
	typedef std::map<std::string, tag_base*> tag_map_t;
	tag_map_t tag_map;

	tag_base* tag_list[] = { CONFIG_LIST };
	for (size_t i = 0; i < sizeof(tag_list)/sizeof(*tag_list); i++)
		if (tag_list[i])
			tag_map[tag_list[i]->tag] = tag_list[i];

	// parse the xml buffer
	tag_map_t::const_iterator i = tag_map.end();
	while(xml->read()) {
		switch(xml->getNodeType()) {
		case EXN_TEXT:
		case EXN_CDATA:
			if (i != tag_map.end()) // do we know about this tag?
				i->second->read(xml->getNodeData());
			break;
		case EXN_ELEMENT_END:
			i = tag_map.end(); // ignore the next EXN_CDATA
			break;
		case EXN_ELEMENT:
			i = tag_map.find(xml->getNodeName());
			break;
		case EXN_NONE: case EXN_COMMENT: case EXN_UNKNOWN:
			break;
		}
	}

	delete xml;
	// delete the tag objects
	for (size_t i = 0; i < sizeof(tag_list)/sizeof(*tag_list); i++)
		delete tag_list[i];

	log_excluded_modes();

	return true;
}

void configuration::loadDefaults()
{
// RTTY
	selShift->index(rtty_shift);
	if (progdefaults.rtty_shift == selShift->lsize() - 1) {
		selCustomShift->activate();
		selCustomShift->value(rtty_custom_shift);
	} else
		selCustomShift->deactivate();
	selBaud->index(rtty_baud);
	selBits->index(rtty_bits);
	selParity->index(rtty_parity);
//	chkMsbFirst->value(rtty_msbfirst);
	selStopBits->index(rtty_stop);
	btnCRCRLF->value(rtty_crcrlf);
	btnAUTOCRLF->value(rtty_autocrlf);
	cntrAUTOCRLF->value(rtty_autocount);
	chkPseudoFSK->value(PseudoFSK);
	chkUOSrx->value(UOSrx);
	chkUOStx->value(UOStx);

	i_listbox_rtty_afc_speed->index(rtty_afcspeed);
	btnPreferXhairScope->value(PreferXhairScope);

// OLIVIA
	i_listbox_olivia_tones->index(oliviatones);
	i_listbox_olivia_bandwidth->index(oliviabw);
	cntOlivia_smargin->value(oliviasmargin);
	cntOlivia_sinteg->value(oliviasinteg);
	btnOlivia_8bit->value(olivia8bit);

// CONTESTIA
	i_listbox_contestia_tones->index(contestiatones);
	i_listbox_contestia_bandwidth->index(contestiabw);
	cntContestia_smargin->value(contestiasmargin);
	cntContestia_sinteg->value(contestiasinteg);
	btnContestia_8bit->value(contestia8bit);

	chkDominoEX_FEC->value(DOMINOEX_FEC);

	Fl_Tooltip::enable(tooltips);

// contest settings for state qso parties
	adjust_for_contest(0);

	UI_select();
	set_log_colors();
	clear_log_fields();
	clearQSO();
}

void configuration::saveDefaults()
{
	ENSURE_THREAD(FLMAIN_TID);

	memcpy(&cfgpal0, &palette[0], sizeof(cfgpal0));
	memcpy(&cfgpal1, &palette[1], sizeof(cfgpal1));
	memcpy(&cfgpal2, &palette[2], sizeof(cfgpal2));
	memcpy(&cfgpal3, &palette[3], sizeof(cfgpal3));
	memcpy(&cfgpal4, &palette[4], sizeof(cfgpal4));
	memcpy(&cfgpal5, &palette[5], sizeof(cfgpal5));
	memcpy(&cfgpal6, &palette[6], sizeof(cfgpal6));
	memcpy(&cfgpal7, &palette[7], sizeof(cfgpal7));
	memcpy(&cfgpal8, &palette[8], sizeof(cfgpal8));

	RxFontName = Fl::get_font_name(RxFontnbr);
	TxFontName = Fl::get_font_name(TxFontnbr);

	WaterfallFontName = Fl::get_font_name(WaterfallFontnbr);

	ViewerFontName = Fl::get_font_name(ViewerFontnbr);

	FreqControlFontName = Fl::get_font_name(FreqControlFontnbr);

	MacroEditFontName = Fl::get_font_name(MacroEditFontnbr);
	MacroBtnFontName = Fl::get_font_name(MacroBtnFontnbr);

	DXC_textname = Fl::get_font_name(DXC_textfont);
	DXfontname = Fl::get_font_name(DXfontnbr);

	LOGGINGfontname = Fl::get_font_name(LOGGINGtextfont);
	LOGBOOKtextname = Fl::get_font_name(LOGBOOKtextfont);


#if ENABLE_NLS && defined(__WOE32__)
	set_ui_lang(listbox_language->index());
#endif

	writeDefaultsXML();
	changed = false;
}

#if USE_HAMLIB
static int fill_hamlib_menu(const char* rigname)
{
	cboHamlibRig->add(rigname);
	return 1;
}
#endif

int configuration::setDefaults()
{
	ENSURE_THREAD(FLMAIN_TID);

#if USE_HAMLIB
	hamlib_get_rigs();
	hamlib_get_rig_str(fill_hamlib_menu);
	if (HamRigModel == 0 && !HamRigName.empty()) { // compatibility with < 3.04
		HamRigModel = hamlib_get_rig_model_compat(HamRigName.c_str());
		LOG_VERBOSE("Found rig model %d for \"%s\"", HamRigModel, HamRigName.c_str());
	}
#endif

	inpMyCallsign->value(myCall.c_str());
	inpMyName->value(myName.c_str());
	inpMyQth->value(myQth.c_str());
	inpMyLocator->value(myLocator.c_str());
	inpMyAntenna->value(myAntenna.c_str());
	UseLeadingZeros = btnUseLeadingZeros->value();
	ContestStart = (int)nbrContestStart->value();
	ContestDigits = (int)nbrContestDigits->value();

	txtSecondary->value(secText.c_str());

	txtTHORSecondary->value(THORsecText.c_str());
	valTHOR_BW->value(THOR_BW);
	valTHOR_FILTER->value(THOR_FILTER);
	valTHOR_PATHS->value(THOR_PATHS);
	valThorCWI->value(ThorCWI);
	valTHOR_PREAMBLE->value(THOR_PREAMBLE);
	valTHOR_SOFTSYMBOLS->value(THOR_SOFTSYMBOLS);
	valTHOR_SOFTBITS->value(THOR_SOFTBITS);

	valDominoEX_BW->value(DOMINOEX_BW);
	valDominoEX_FILTER->value(DOMINOEX_FILTER);
	chkDominoEX_FEC->value(DOMINOEX_FEC);
	valDominoEX_PATHS->value(DOMINOEX_PATHS);
	valDomCWI->value(DomCWI);

	btnRigCatCMDptt->value(RigCatCMDptt);
	btnTTYptt->value(TTYptt);
	btnUsePPortPTT->value(progdefaults.UsePPortPTT);
	btnUseUHrouterPTT->value(progdefaults.UseUHrouterPTT);

	btn_use_cmedia_PTT->value(progdefaults.cmedia_ptt);

#if USE_HAMLIB
	listbox_sideband->add(_("Rig mode"));
	listbox_sideband->add(_("Always LSB"));
	listbox_sideband->add(_("Always USB"));
	listbox_sideband->index(HamlibSideband);
    btnHamlibCMDptt->value(HamlibCMDptt);
    inpRIGdev->show();
	listbox_baudrate->show();
	cboHamlibRig->show();
	cboHamlibRig->value(HamRigName.c_str());
#else
	tab_tree->remove(tab_tree->find_item(_("Rig Control/Hamlib")));
#endif
	btnRTSptt->value(RTSptt);
	btnDTRptt->value(DTRptt);
	btnRTSplusV->value(RTSplus);
	btnDTRplusV->value(DTRplus);

	inpTTYdev->value(PTTdev.c_str());

	chkUSEHAMLIB->value(0);
	chkUSERIGCAT->value(0);
	if (chkUSEHAMLIBis) chkUSEHAMLIB->value(1);
	if (chkUSERIGCATis) chkUSERIGCAT->value(1);

	if (!XmlRigFilename.empty()) readRigXML();

	inpRIGdev->value(HamRigDevice.c_str());
	listbox_baudrate->index(HamRigBaudrate);

	inpXmlRigDevice->value(XmlRigDevice.c_str());
	listbox_xml_rig_baudrate->index(XmlRigBaudrate);

	select_nanoIO_CommPort->value(nanoIO_serial_port_name.c_str());
	select_nanoCW_CommPort->value(nanoIO_serial_port_name.c_str());
	select_CW_KEYLINE_CommPort->value(CW_KEYLINE_serial_port_name.c_str());
	select_FSK_CommPort->value(fsk_port.c_str());

	select_USN_FSK_port->value(Nav_FSK_port.c_str());
	select_Nav_config_port->value(Nav_config_port.c_str());

	valCWsweetspot->value(CWsweetspot);
	valRTTYsweetspot->value(RTTYsweetspot);
	valPSKsweetspot->value(PSKsweetspot);
	btnWaterfallHistoryDefault->value(WaterfallHistoryDefault);
	btnWaterfallQSY->value(WaterfallQSY);

	inpWaterfallClickText->input_type(FL_MULTILINE_INPUT);
	inpWaterfallClickText->value(WaterfallClickText.c_str());
	if (!WaterfallClickInsert)
		inpWaterfallClickText->deactivate();

	for (size_t i = 0;
	     i < sizeof(waterfall::wf_wheel_action)/sizeof(*waterfall::wf_wheel_action); i++)
		listboxWaterfallWheelAction->add(waterfall::wf_wheel_action[i]);
	listboxWaterfallWheelAction->index(WaterfallWheelAction);

	btnStartAtSweetSpot->value(StartAtSweetSpot);
	btnPSKmailSweetSpot->value(PSKmailSweetSpot);
	cntSearchRange->value(SearchRange);
	cntServerOffset->value(ServerOffset);
	cntACQsn->value(ACQsn);

	btnCursorBWcolor->color(
		fl_rgb_color(cursorLineRGBI.R, cursorLineRGBI.G, cursorLineRGBI.B) );
	btnCursorCenterLineColor->color(
		fl_rgb_color(cursorCenterRGBI.R, cursorCenterRGBI.G, cursorCenterRGBI.B) );
	btnBwTracksColor->color(
		fl_rgb_color(bwTrackRGBI.R, bwTrackRGBI.G, bwTrackRGBI.B) );

	sldrCWxmtWPM->value(CWspeed);
	cntCWdefWPM->value(defCWspeed);
	sldrCWbandwidth->value(CWbandwidth);
	btnCWrcvTrack->value(CWtrack);
	cntCWrange->value(CWrange);
	cntCWlowerlimit->value(CWlowerlimit);
	cntCWupperlimit->value(CWupperlimit);
	cntCWlowerlimit->maximum(CWupperlimit - 20);
	cntCWupperlimit->minimum(CWlowerlimit + 20);
	cntCWrisetime->value(CWrisetime);
	cntCWdash2dot->value(CWdash2dot);
	i_listboxQSKshape->index(QSKshape);
	sldrCWxmtWPM->minimum(CWlowerlimit);
	sldrCWxmtWPM->maximum(CWupperlimit);
	btnQSK->value(QSK);
	cntPreTiming->value(CWpre);
	cntPostTiming->value(CWpost);
	btnCWID->value(CWid);

	listboxHellFont->index(feldfontnbr);
	btnFeldHellIdle->value(HellXmtIdle);

	btnTxRSID->value(TransmitRSid);
	btnRSID->value(rsid);
	chkRSidWideSearch->value(rsidWideSearch);
	chkSlowCpu->value(slowcpu);

	Fl_Button* qrzb = btnQRZXMLnotavailable;
	Fl_Button* qrzb2 = btnQRZWEBnotavailable;
	switch (QRZXML) {
	case QRZCD:
		qrzb = btnQRZcdrom;
		break;
	case QRZNET:
		qrzb = btnQRZsub;
		break;
	case HAMCALLNET:
		qrzb = btnHamcall;
		break;
	case CALLOOK:
		qrzb = btnCALLOOK;
		break;
	case HAMQTH:
		qrzb = btnHamQTH;
		break;
	case QRZXMLNONE:
	default :
		break;
	}
	switch (QRZWEB) {
	case QRZHTML:
		qrzb2 = btnQRZonline;
		break;
	case HAMCALLHTML:
		qrzb2 = btnHAMCALLonline;
		break;
	case HAMQTHHTML:
		qrzb2 = btnHamQTHonline;
		break;
	case QRZWEBNONE:
	default :
		break;
	}

	set_qrzxml_buttons(qrzb);
	set_qrzweb_buttons(qrzb2);

	txtQRZpathname->value(QRZpathname.c_str());

	btnsendid->value(sendid);
	btnsendvideotext->value(sendtextid);
	chkID_SMALL->value(ID_SMALL);

	wf->setPrefilter(wfPreFilter);
	btnWFaveraging->value(WFaveraging);

	memcpy(&palette[0], &cfgpal0, sizeof(palette[0]));
	memcpy(&palette[1], &cfgpal1, sizeof(palette[1]));
	memcpy(&palette[2], &cfgpal2, sizeof(palette[2]));
	memcpy(&palette[3], &cfgpal3, sizeof(palette[3]));
	memcpy(&palette[4], &cfgpal4, sizeof(palette[4]));
	memcpy(&palette[5], &cfgpal5, sizeof(palette[5]));
	memcpy(&palette[6], &cfgpal6, sizeof(palette[6]));
	memcpy(&palette[7], &cfgpal7, sizeof(palette[7]));
	memcpy(&palette[8], &cfgpal8, sizeof(palette[8]));

	wf->setcolors();
	setColorButtons();

#if !HAVE_UHROUTER
	btnUseUHrouterPTT->hide();
#endif

#if !HAVE_PARPORT
	btnUsePPortPTT->hide();
#endif

#if ENABLE_NLS && defined(__WOE32__)
	std::ostringstream ss;
	for (lang_def_t* p = ui_langs; p->lang; p++) {
		ss.str("");
		ss << p->native_name;
		listbox_language->add(ss.str().c_str());
	}
	listbox_language->index(get_ui_lang());
	listbox_language->show();
#else
	listbox_language->hide();
#endif

	return 1;
}

void configuration::resetDefaults(void)
{
	if (!fl_choice2(_("\
Reset all options to their default values?\n\n\
Reset options will take effect at the next start\n\
Files: fldigi_def.xml and fldigi.prefs will be deleted!\n"), _("OK"), _("Cancel"), NULL) &&
			Fl::event_key() != FL_Escape) {
		if (!fl_choice2(_("Confirm RESET"), _("Yes"), _("No"), NULL) &&
			Fl::event_key() != FL_Escape) {
			reset();
			atexit(reset);
		}
	}
}

void configuration::reset(void)
{
	remove(std::string(HomeDir).append("fldigi_def.xml").c_str());
	remove(std::string(HomeDir).append("fldigi.prefs").c_str());
}

#include "rigio.h"

void configuration::initInterface()
{
	ENSURE_THREAD(FLMAIN_TID);

// close down any possible rig interface threads
#if USE_HAMLIB
	hamlib_close();
//		MilliSleep(100);
#endif
	rigCAT_close();
//		MilliSleep(100);

	RigCatCMDptt = btnRigCatCMDptt->value();
	TTYptt = btnTTYptt->value();

	RTSptt = btnRTSptt->value();
	DTRptt = btnDTRptt->value();
	RTSplus = btnRTSplusV->value();
	DTRplus = btnDTRplusV->value();

	PTTdev = inpTTYdev->value();

#if USE_HAMLIB
	chkUSEHAMLIBis = chkUSEHAMLIB->value();
     HamlibCMDptt = btnHamlibCMDptt->value();
#endif
	chkUSERIGCATis = chkUSERIGCAT->value();

#if USE_HAMLIB
	if (*cboHamlibRig->value() == '\0') // no selection at start up
		cboHamlibRig->index(hamlib_get_index(HamRigModel));
	else
		HamRigModel = hamlib_get_rig_model(cboHamlibRig->index());
	HamRigDevice = inpRIGdev->value();
	HamRigBaudrate = listbox_baudrate->index();
#else
	cboHamlibRig->hide();
	inpRIGdev->hide();
	listbox_baudrate->hide();
#endif

	if (connected_to_flrig) {
		LOG_INFO("%s", "using flrig xcvr control");
		wf->setQSY(1);
	} else if (chkUSERIGCATis) { // start the rigCAT thread
		if (rigCAT_init()) {
			LOG_INFO("%s", "using rigCAT xcvr control");
			wf->USB(true);
			wf->setQSY(1);
			rigCAT_get_pwrlevel();
		} else {
			LOG_INFO("%s", "defaulting to no xcvr control");
			noCAT_init();
			wf->USB(true);
			wf->setQSY(0);
			chkUSERIGCATis = false;
		}
#if USE_HAMLIB
	} else if (chkUSEHAMLIBis) { // start the hamlib thread
		if (hamlib_init(HamlibCMDptt)) {
			LOG_INFO("%s", "using HAMLIB xcvr control");
			btnInitHAMLIB->deactivate();
			wf->USB(true);
			wf->setQSY(1);
		} else {
			LOG_INFO("%s", "defaulting to no xcvr control");
			noCAT_init();
			wf->USB(true);
			wf->setQSY(0);
		}
#endif
	} else {
		LOG_INFO("%s", "No xcvr control selected");
		noCAT_init();
		wf->USB(true);
		wf->setQSY(0);
	}
	build_frequencies2_list();

	if (HamlibCMDptt && chkUSEHAMLIBis)
		push2talk->reset(PTT::PTT_HAMLIB);
	else if ((RigCatCMDptt || RigCatRTSptt || RigCatDTRptt) && chkUSERIGCATis)
		push2talk->reset(PTT::PTT_RIGCAT);
	else if (TTYptt)
		push2talk->reset(PTT::PTT_TTY);
	else if (UsePPortPTT)
		push2talk->reset(PTT::PTT_PARPORT);
	else if (UseUHrouterPTT)
		push2talk->reset(PTT::PTT_UHROUTER);
	else if (cmedia_ptt)
		push2talk->reset(PTT::PTT_CMEDIA);
	else if (gpio_ptt)
		push2talk->reset(PTT::PTT_GPIO);
	else
		push2talk->reset(PTT::PTT_NONE);

	wf->setRefLevel();
	wf->setAmpSpan();
	cntLowFreqCutoff->value(LowFreqCutoff);
}

const char* configuration::strBaudRate()
{
	return (szBaudRates[HamRigBaudrate + 1]);
}

int configuration::nBaudRate(const char *szBR)
{
    for (size_t i = 1; i < sizeof(szBaudRates); i++)
        if (strcmp(szBaudRates[i], szBR) == 0)
            return i - 1;
    return 0;
}

int configuration::BaudRate(size_t n)
{
	if (n > sizeof(szBaudRates) + 1) return 1200;
	return (atoi(szBaudRates[n + 1]));
}

#ifdef __WOE32__
static bool open_serial(const char* dev)
{
	bool ret = false;
#ifdef __CYGWIN__
	int fd = fl_open(dev, O_RDWR | O_NOCTTY | O_NDELAY | O_CLOEXEC);
	if (fd != -1) {
		close(fd);
		ret = true;
	}
#elif defined(__MINGW32__)
	HANDLE fd = CreateFile(dev, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
	if (fd != INVALID_HANDLE_VALUE) {
		CloseHandle(fd);
		ret = true;
	}
#endif
	return ret;
}
#endif // __WOE32__

void configuration::testCommPorts()
{
	inpTTYdev->clear();
	inpRIGdev->clear();
	inpXmlRigDevice->clear();
#ifndef PATH_MAX
#  define PATH_MAX 1024
#endif
#ifndef __WOE32__
	struct stat st;
#endif

#ifndef __APPLE__
	char ttyname[PATH_MAX + 1];
#endif

	const char* tty_fmt[] = {
#if defined(__linux__)
		"/dev/ttyS%u",
		"/dev/ttyUSB%u",
		"/dev/usb/ttyUSB%u",
		"/dev/ttyACM%u",
		"/dev/usb/ttyACM%u",
		"/dev/rfcomm%u",
		"/opt/vttyS%u"
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
		"/dev/tty%2.2u"
#elif defined(__CYGWIN__)
		"/dev/ttyS%u"
#elif defined(__MINGW32__)
		"//./COM%u"
#elif defined(__APPLE__)
		"/dev/cu.*",
		"/dev/tty.*"
#endif
	};

#if defined(__WOE32__)
#  define TTY_MAX 255
#elif defined(__OpenBSD__) || defined(__NetBSD__)
#  define TTY_MAX 4
#else
#  define TTY_MAX 8
#endif

#ifdef __linux__
	glob_t gbuf;
	glob("/dev/serial/by-id/*", 0, NULL, &gbuf);
	for (size_t j = 0; j < gbuf.gl_pathc; j++) {
		if ( !(stat(gbuf.gl_pathv[j], &st) == 0 && S_ISCHR(st.st_mode)) ||
		     strstr(gbuf.gl_pathv[j], "modem") )
			continue;
		LOG_INFO("Found serial port %s", gbuf.gl_pathv[j]);
		inpTTYdev->add(gbuf.gl_pathv[j]);
#  if USE_HAMLIB
		inpRIGdev->add(gbuf.gl_pathv[j]);
#  endif
		inpXmlRigDevice->add(gbuf.gl_pathv[j]);

		select_nanoIO_CommPort->add(gbuf.gl_pathv[j]);
		select_nanoCW_CommPort->add(gbuf.gl_pathv[j]);
		select_CW_KEYLINE_CommPort->add(gbuf.gl_pathv[j]);
		select_FSK_CommPort->add(gbuf.gl_pathv[j]);

		select_USN_FSK_port->add(gbuf.gl_pathv[j]);
		select_Nav_config_port->add(gbuf.gl_pathv[j]);

		select_WK_CommPort->add(gbuf.gl_pathv[j]);
		select_WKFSK_CommPort->add(gbuf.gl_pathv[j]);
	}
	globfree(&gbuf);
#endif

	for (size_t i = 0; i < sizeof(tty_fmt)/sizeof(*tty_fmt); i++) {
#ifndef __APPLE__
		for (unsigned j = 0; j < TTY_MAX; j++) {
			snprintf(ttyname, sizeof(ttyname), tty_fmt[i], j);
#  ifndef __WOE32__
			if ( !(stat(ttyname, &st) == 0 && S_ISCHR(st.st_mode)) )
				continue;
#  else // __WOE32__
			LOG_DEBUG("Testing serial port %s", ttyname);
			if (!open_serial(ttyname))
				continue;
#    ifdef __CYGWIN__
			snprintf(ttyname, sizeof(ttyname), "COM%u", j+1);
#    else
			snprintf(ttyname, sizeof(ttyname), "COM%u", j);
#    endif
#  endif // __WOE32__

			LOG_VERBOSE("Found serial port %s", ttyname);
			inpTTYdev->add(ttyname);
#  if USE_HAMLIB
			inpRIGdev->add(ttyname);
#  endif
			inpXmlRigDevice->add(ttyname);

			select_nanoIO_CommPort->add(ttyname);
			select_nanoCW_CommPort->add(ttyname);
			select_CW_KEYLINE_CommPort->add(ttyname);
			select_FSK_CommPort->add(ttyname);

			select_USN_FSK_port->add(ttyname);
			select_Nav_config_port->add(ttyname);

			select_WK_CommPort->add(ttyname);
			select_WKFSK_CommPort->add(ttyname);
		}
#else // __APPLE__
		glob_t gbuf;
		glob(tty_fmt[i], 0, NULL, &gbuf);
		for (size_t j = 0; j < gbuf.gl_pathc; j++) {
			int ret1 = !stat(gbuf.gl_pathv[j], &st);
			int ret2 = S_ISCHR(st.st_mode);
			if (ret1) {
				LOG_INFO("Serial port %s", gbuf.gl_pathv[j]);
				LOG_INFO("  device mode:     %X", st.st_mode);
				LOG_INFO("  char device?     %s", ret2 ? "Y" : "N");
			} else
				LOG_INFO("%s does not return stat query", gbuf.gl_pathv[j]);

			if ( (ret1 && ret2 ) || strstr(gbuf.gl_pathv[j], "modem") )
				inpTTYdev->add(gbuf.gl_pathv[j]);
			else
				continue;
#  if USE_HAMLIB
			inpRIGdev->add(gbuf.gl_pathv[j]);
#  endif
			inpXmlRigDevice->add(gbuf.gl_pathv[j]);

			select_nanoIO_CommPort->add(gbuf.gl_pathv[j]);
			select_nanoCW_CommPort->add(gbuf.gl_pathv[j]);
			select_CW_KEYLINE_CommPort->add(gbuf.gl_pathv[j]);
			select_FSK_CommPort->add(gbuf.gl_pathv[j]);

			select_USN_FSK_port->add(gbuf.gl_pathv[j]);
			select_Nav_config_port->add(gbuf.gl_pathv[j]);

			select_WK_CommPort->add(gbuf.gl_pathv[j]);
			select_WKFSK_CommPort->add(gbuf.gl_pathv[j]);
		}
		globfree(&gbuf);
#endif // __APPLE__
	}

#if HAVE_UHROUTER
	if (stat(UHROUTER_FIFO_PREFIX "Read", &st) != -1 && S_ISFIFO(st.st_mode) &&
	    stat(UHROUTER_FIFO_PREFIX "Write", &st) != -1 && S_ISFIFO(st.st_mode))
		inpTTYdev->add(UHROUTER_FIFO_PREFIX);
#endif // HAVE_UHROUTER
}

Fl_Font font_number(const char* name)
{
    int n = (int)Fl::set_fonts(0);
    for (int i = 0; i < n; i++) {
        if (strcmp(Fl::get_font_name((Fl_Font)i), name) == 0)
            return (Fl_Font)i;
    }
	return FL_HELVETICA;
}

void configuration::initFonts(void)
{
	RxFontnbr =
	TxFontnbr =
	WaterfallFontnbr =
	ViewerFontnbr =
	FreqControlFontnbr =
	MacroBtnFontnbr =
	MacroEditFontnbr  = 

	DXC_textfont =
	DXfontnbr =

	LOGGINGtextfont =
	LOGBOOKtextfont =

	FL_HELVETICA;

	if (!RxFontName.empty())
		RxFontnbr = font_number(RxFontName.c_str());
	if (!TxFontName.empty())
		TxFontnbr = font_number(TxFontName.c_str());
	if (!WaterfallFontName.empty())
		WaterfallFontnbr = font_number(WaterfallFontName.c_str());
	if (!ViewerFontName.empty())
		ViewerFontnbr = font_number(ViewerFontName.c_str());
	if (!FreqControlFontName.empty())
		FreqControlFontnbr = font_number(FreqControlFontName.c_str());

	if (!MacroEditFontName.empty())
		MacroEditFontnbr = font_number(MacroEditFontName.c_str());
	if (!MacroBtnFontName.empty())
		MacroBtnFontnbr = font_number(MacroBtnFontName.c_str());

	if (!DXC_textname.empty())
		DXC_textfont = font_number(DXC_textname.c_str());
	if (!DXfontname.empty())
		DXfontnbr = font_number(DXfontname.c_str());

	if (!LOGGINGfontname.empty())
		LOGGINGtextfont = font_number(LOGGINGfontname.c_str());
	if (!LOGBOOKtextname.empty())
		LOGBOOKtextfont = font_number(LOGBOOKtextname.c_str());
}
