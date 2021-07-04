// ----------------------------------------------------------------------------
// contest.cxx
//
// Copyright (C) 2006-2010
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

#include <iostream>

#include <FL/Fl.H>

#include "fl_digi.h"
#include "confdialog.h"

#include "contest.h"
#include "icons.h"
#include "dxcc.h"
#include "strutil.h"

static std::string SECTIONS = "\
DX  \
CT  RI  EMA VT  ME  WMA NH  \
ENY NNY NLI SNJ NNJ WHY \
DE  MDC EPA WPA \
AL  GA  KY  NC  NFL PR  SC  SFL TN  VA  \
VI  WCF AR  LA  MS  NM  NTX OK  STX WTX \
EB  LAX ORG PAC SB  SCV SDG SF  SJV SV  \
AK  AZ  EWA ID  MT  NV  OR  UT  WWA WY  \
MI  OH  WV  \
IL  IN  WI  \
CO  IA  KS  MN  MO  ND  NE  SD  \
AB  BC  GTA MAR MB  NL  NT  ONE ONN ONS QC  SK  ";

static std::string STATES = "\
DX  CT  MA  ME  NH  RI  VT  \
NY  NJ  \
DE  PA  MD  DC \
AL  FL  GA  KY  NC  SC  TN  VA  \
AR  LA  MS  NM  OK  TX  \
CA  HI  \
AK  AZ  ID  MT  NV  OR  UT  WA  WY  \
MI  OH  WV  \
IL  WI  IN  \
CO  IA  KS  MN  MO  ND  NE  SD  ";

static std::string PROVINCES = "\
AB  BC  LB  MB  NB  NF  NS  NU  NWT ON  PEI QC  SK  YT  ";

static std::string MEXICO = "\
XE1 XE2 XE3 XF1 XF4 ";

static std::string IT1_ = "AL AT BI CN GE IM NO SP SV TO VB VC ";
static std::string IX1_ = "AO ";
static std::string IT2_ = "BG BS CO CR LC LO MB MI MN PV SO VA ";
static std::string IT3_ = "BL PD RO TV VE VI VR ";
static std::string IN3_ = "BZ TN ";
static std::string IV3_ = "GO PN TS UD ";
static std::string IT4_ = "BO FC FE MO PC PR RA RE RN ";
static std::string IT5_ = "AR FI GR LI LU MS PI PO PT SI ";
static std::string IT6_ = "AN AP AQ CH FM MC PE PS PU TE ";
static std::string IT7_ = "BA BR BT FG LE MT TA ";
static std::string IT8_ = "AV BN CB CE CS CZ IS KR NA PZ RC SA VV ";
static std::string IT0_ = "FR LT PG RI RM TR VT ";
static std::string IT9_ = "AG CL CT EN ME PA RG SR TP ";
static std::string IS0_ = "CA NU OR SS SU ";

static const std::vector<dxcc*>* dxcc_list;
std::string country_match = "";

bool class_test(std::string s)
{
	if (s.length() < 2) return false;
	static std::string clss = "ABCDEF";
	if (clss.find(toupper(s[s.length()-1])) == std::string::npos)
		return false;
	for (size_t n = 0; n < s.length() - 1; n++)
		if (s[n] < '0' || s[n] > '9') return false;
	return true;
}

bool state_test(std::string s)
{
	if (s.empty()) return false;
	while (s.length() < 4) s.append(" ");
	bool isState = (STATES.find(ucasestr(s)) != std::string::npos);
	return isState;
}

bool county_test(std::string st, std::string cty)
{
	if (st.empty() && !progdefaults.SQSOinstate)
		st = QSOparties.qso_parties[progdefaults.SQSOcontest].state;
	if (!state_test(st))
		return false;
	return states.valid_county( st, cty );
}

bool province_test(std::string s)
{
	while (s.length() < 4) s.append(" ");
	return (PROVINCES.find(ucasestr(s)) != std::string::npos);
}

bool district_test(std::string pr, std::string dist)
{
	if (pr.empty() && !progdefaults.SQSOinstate)
		pr = QSOparties.qso_parties[progdefaults.SQSOcontest].state;
	if (!province_test(pr))
		return false;
	return states.valid_county( pr, dist );
}

bool check_test(std::string s)
{
	if (s.length() < 4) {
		while (s.length() < 4) s.append(" ");
		std::string CHECK = std::string(STATES).append(PROVINCES).append(MEXICO);
		return (CHECK.find(s) != std::string::npos);
	} else
		return false;
}

bool country_test(std::string s)
{
	std::string str = ucasestr(s);
	dxcc_list = dxcc_entity_list();
	if (!dxcc_list) return true;
	for (std::vector<dxcc*>::const_iterator i = dxcc_list->begin(); i != dxcc_list->end(); ++i) {
		if (ucasestr((*i)->country).find(str) != std::string::npos) {
			country_match = (*i)->country;
			return true;
		}
	}
	country_match.clear();
	return false;
}

bool wfd_class_test(std::string s)
{
	if (s.length() < 2) return false;
	static std::string clss = "IOH";
	if (clss.find(toupper(s[s.length()-1])) == std::string::npos)
		return false;
	for (size_t n = 0; n < s.length() - 1; n++)
		if (s[n] < '0' || s[n] > '9') return false;
	return true;
}

bool ascr_class_test(std::string s)
{
	if (s.length() != 1) return false;
	if (toupper(s[0]) == 'I' ||
		toupper(s[0]) == 'C' ||
		toupper(s[0]) == 'S') return true;
	return false;
}

bool section_test(std::string s)
{
	while (s.length() < 4) s.append(" ");
	return (SECTIONS.find(s) != std::string::npos);
}

bool rookie_test(std::string s)
{
	int year_licensed = 0;
	if (!sscanf(s.c_str(), "%d", &year_licensed))
		return false;
	if (year_licensed < 100) year_licensed += 2000;
	int year_worked = 0;
	std::string s_year = std::string(zdate()).substr(0,4);
	sscanf(s_year.c_str(), "%d", &year_worked);
	if (year_worked < year_licensed) year_licensed -= 100;
	if (year_worked - year_licensed < 3) return true;
	return false;
}

bool c1010_test(std::string s)
{
	for (size_t n = 0; n < s.length(); n++)
		if (s[n] < '0' || s[n] > '9') return false;
	return true;
}

static std::string nbrs0 = "1234567890";
static std::string nbrs1 = "12345";
static std::string nbrs2 = "123456789Nn";
static std::string nbrs3 = "1234567890NnTt";

std::string cut_to_numeric(std::string s)
{
	for (size_t n = 0; n < s.length(); n++) {
		if (s[n] == 'N' || s[n] == 'n' ) s[n] = '9';
		if (s[n] == 'T' || s[n] == 't' ) s[n] = '0';
	}
	return s;
}

bool cut_numeric_test(std::string s)
{
	if (s.empty()) return false;
	for (size_t n = 0; n < s.length(); n++)
		if (nbrs3.find(s[n]) == std::string::npos) return false;
	return true;
}

bool numeric_test(std::string s)
{
	if (s.empty()) return false;
	for (size_t n = 0; n < s.length(); n++)
		if (nbrs0.find(s[n]) == std::string::npos) return false;
	return true;
}

bool rst_test(std::string s)
{
	if (s.length() < 3 && active_modem->get_mode() < MODE_SSB)
		return false;
	if (s.length() < 2 || s.length() > 3) return false;
	if (s[0] == '0') return false;
	if (s.length() == 2) {
		if (nbrs1.find(s[0]) == std::string::npos) return false;
		if (nbrs2.find(s[1]) == std::string::npos) return false;
	} else {
		if (nbrs1.find(s[0]) == std::string::npos) return false;
		if (nbrs2.find(s[1]) == std::string::npos) return false;
		if (nbrs2.find(s[2]) == std::string::npos) return false;
	}
	return true;
}

bool italian_test(std::string s)
{
	if (s.length() != 2) return false;
	s.append(" ");
	if (IT1_.find(s) != std::string::npos ||
		IX1_.find(s) != std::string::npos ||
		IT2_.find(s) != std::string::npos ||
		IT3_.find(s) != std::string::npos ||
		IN3_.find(s) != std::string::npos ||
		IV3_.find(s) != std::string::npos ||
		IT4_.find(s) != std::string::npos ||
		IT5_.find(s) != std::string::npos ||
		IT6_.find(s) != std::string::npos ||
		IT7_.find(s) != std::string::npos ||
		IT8_.find(s) != std::string::npos ||
		IT0_.find(s) != std::string::npos ||
		IT9_.find(s) != std::string::npos ||
		IS0_.find(s) != std::string::npos )
		return true;
	return false;
}

bool ss_chk_test(std::string s)
{
	std::string nums = "0123456789";
	if (nums.find(s[0]) != std::string::npos &&
		nums.find(s[0]) != std::string::npos)
		return true;
	return false;
}

bool ss_prec_test(std::string s)
{
	std::string prec = "QABUMS";
	if (prec.find(ucasestr(s)[0]) != std::string::npos)
		return true;
	return false;
}

int check_field(std::string s, CONTEST_FIELD field, std::string s2)
{
	switch (field) {
		case cSTATE: 
			return state_test(s);
			break;
		case cVE_PROV: 
			return province_test(s);
			break;
		case cCHECK: 
			return check_test(s);
			break;
		case cCOUNTRY:
			return country_test(s);
			break;
		case cCLASS:
		case cFD_CLASS:
			return class_test(s);
			break;
		case cWFD_CLASS:
			return wfd_class_test(s);
			break;
		case cASCR_CLASS:
			return ascr_class_test(s);
			break;
		case cARRL_SECT:
		case cFD_SECTION: 
			return section_test(s);
			break;
		case cROOKIE:
			return rookie_test(s);
			break;
		case c1010:
			return c1010_test(s);
			break;
		case cRST:
			return rst_test(s);
			break;
		case cSRX:
		case cNUMERIC:
			return numeric_test(s);
			break;
		case cITALIAN:
			return italian_test(s);
			break;
		case cCNTY:
			return county_test(s2, s);
			break;
		case cDIST:
			return district_test(s2, s);
			break;
		case cSS_CHK:
			return ss_chk_test(s);
			break;
		case cSS_PREC:
			return ss_prec_test(s);
			break;
		case cSS_SERNO:
			return numeric_test(s);
			break;
		case cSS_SEC:
			return section_test(s);
			break;
		case cNAME: break;
		case cQTH: break;
		case cGRIDSQUARE: break;
		case cXCHG1: break;
		case cKD_XCHG: break;
		case cARR_XCHG: break;
		case cCQZ: break;
		default: break;
	}
	return true;
}

CONTESTS contests[] = {
{ "No Contest", "CALL if (RSTr), if (LOCATOR), NAME, QTH" },
{ "Generic contest", "CALL EXCHANGE" },
{ "Africa All-Mode International", "CALL SERNO, COUNTRY, RSTr, RSTs" },
{ "ARRL Field Day", "CALL SECTION, CLASS, RSTr, RSTs" },
{ "ARRL International DX (cw)", "CALL COUNTRY, POWER, RSTr, RSTs" },
{ "ARRL Jamboree on the Air", "CALL TROOP_NO, STATE / VE_PROV / COUNTRY, RSTr, RSTs, SCOUT_NAME" },
{ "ARRL Kids Day", "CALL, NAME, AGE, QTH, COMMENT, RSTr, RSTs" },
{ "ARRL Rookie Roundup", "CALL, NAME, CHECK, STATE / VE_PROV, RSTr, RSTs" },
{ "ARRL RTTY Roundup", "CALL STATE, SERNO, COUNTRY, RSTr, RSTs" },
{ "ARRL School Club Roundup", "CALL CLASS, STATE / VE_PROV, NAME, RSTr, RSTs" },
{ "ARRL November Sweepstakes", "CALL SECTION, SERNO, PREC, CHECK, RSTr, RSTs" },
{ "ARRL Winter FD", "CALL SECTION, CLASS, RSTr, RSTs" },
{ "BARTG RTTY contest", "CALL NAME, SERIAL, EXCHANGE" },
{ "CQ WPX", "CALL SERNO, COUNTRY, RSTr, RSTs" },
{ "CQ WW DX", "CALL COUNTRY, ZONE, RSTr, RSTs" },
{ "CQ WW DX RTTY", "CALL STATE, COUNTRY, ZONE, RSTr" },
{ "Italian A.R.I. International DX", "CALL PR(ovince), COUNTRY, SERNO, RSTr, RSTs" },
{ "NAQP", "CALL NAME, STATE / VE_PROV / COUNTRY" },
{ "NA Sprint", "CALL SERNO, STATE / VE_PROV / COUNTRY, NAME, RSTr, RSTs" },
{ "Ten Ten", "CALL 1010NR, STATE, NAME, RSTr, RSTs" },
{ "VHF", "CALL RSTr, RSTs" },
//{ "Worked All Europe", "CALL SERNO, COUNTRY, RSTr, RSTs" },
{ "State QSO parties", "" },
{ "", "" }
};

struct QSOP Ccontests::qso_parties[] = {
/*
{"QSO Party Contest",              "ST", "in",  "rRST","rST","rCY","rSER","rXCHG","rNAM","rCAT", "STCTY", "Notes"},
*/
{"None selected",                  "",   "",    "",    "",   "",   "",    "",     "",     "",    "",      "CALL if (RSTr), if (LOCATOR), NAME, QTH" },
{"Alabama QSO Party",              "AL", "T",   "B",   "B",  "B",  "",    "I",    "",     "",    "",      "CALL RST CNTY ST/PR/CNTRY"},
{"ALQP (Out of State)",            "AL", "",    "B",   "B",  "B",  "",    "I",    "",     "",    "",      "CALL RST CNTY"},
{"Arizona QSO Party",              "AZ", "T",   "B",   "B",  "B",  "" ,   "",     "",     "",    "",      "CALL RST CNTY ST/PR/CNTRY"},
{"AZQP (Out of State)",            "AZ", "",    "B",   "B",  "B",  "" ,   "",     "",     "",    "",      "CALL RST CNTY"},
{"Arkansas QSO Party",             "AR", "T",   "B",   "B",  "B",  "",    "I",    "",     "",    "",      "CALL RST CNTY ST/PR/CNTRY"},
{"ARQP (Out of State)",            "AR", "",    "B",   "B",  "B",  "",    "I",    "",     "",    "",      "CALL RST CNTY"},
{"British Columbia QSO Party",     "BC", "T",   "B",   "B",  "B",  "",    "I",    "",     "",    "",      "CALL RST CNTY ST/PR/CNTRY"},
{"BCQP (Out of Province)",         "BC", "",    "B",   "B",  "B",  "",    "",     "",     "",    "",      "CALL RST CNTY"},
{"California QSO Party",           "CA", "T",   "",    "B",  "B",  "B",   "I",    "",     "",    "",      "CALL SERNO CNTY ST/PR/CNTRY"},
{"CAQP (Out of State)",            "CA", "",    "",    "B",  "B",  "B",   "I",    "",     "",    "",      "CALL SERNO CNTY"},
{"Colorado QSO Party",             "CO", "T",   "",    "B",  "B",  "",    "I",    "B",    "",    "",      "CALL NAME CNTY ST/PR/CNTRY"},
{"COQP (Out of State)",            "CO", "",    "",    "B",  "B",  "",    "I",    "B",    "",    "",      "CALL NAME CNTY"},
{"Delaware QSO Party",             "DE", "T",   "B",   "B",  "B",  "",    "",     "",     "",    "",      "CALL RST CNTY ST/PR/CNTRY"},
{"DEQP (Out of State)",            "DE", "",    "B",   "B",  "B",  "",    "",     "",     "",    "",      "CALL RST CNTY"},
{"Florida QSO Party",              "FL", "T",   "B",   "B",  "B",  "",    "I",    "",     "",    "",      "CALL RST CNTY ST/PR/CNTRY"},
{"FLQP (Out of State)",            "FL", "",    "B",   "B",  "B",  "",    "I",    "",     "",    "",      "CALL RST CNTY"},
{"Georgia QSO Party",              "GA", "T",   "B",   "B",  "B",  "",    "" ,    "",     "",    "",      "CALL RST CNTY ST/PR/CNTRY"},
{"GAQP (Out of State)",            "GA", "",    "B",   "B",  "B",  "",    "" ,    "",     "",    "",      "CALL RST CNTY"},
{"Hawaii QSO Party",               "HI", "T",   "B",   "B",  "B",  "",    "I",    "",     "",    "",      "CALL RST CNTY ST/PR/CNTRY"},
{"HIQP (Out of State)",            "HI", "",    "B",   "B",  "B",  "",    "I",    "",     "",    "",      "CALL RST CNTY"},
{"Idaho QSO Party",                "ID", "T",   "B",   "B",  "B",  "",    "I",    "",     "",    "",      "CALL RST CNTY ST/PR/CNTRY"},
{"IDQP (Out of State)",            "ID", "",    "B",   "B",  "B",  "",    "I",    "",     "",    "",      "CALL RST CNTY"},
{"Illinois QSO Party",             "IL", "T",   "B",   "B",  "B",  "",    "I",    "",     "",    "",      "CALL RST CNTY ST/PR/CNTRY"},
{"ILQP (Out of State)",            "IL", "",    "B",   "B",  "B",  "",    "I",    "",     "",    "",      "CALL RST CNTY"},
{"Indiana QSO Party",              "IN", "T",   "B",   "B",  "B",  "",    "I",    "",     "",    "",      "CALL RST CNTY ST/PR/CNTRY"},
{"INQP (Out of State)",            "IN", "",    "B",   "B",  "B",  "",    "I",    "",     "",    "",      "CALL RST CNTY"},
{"Iowa QSO Party",                 "IA", "T",   "B",   "B",  "B",  "",    "I",    "",     "",    "",      "CALL RST CNTY ST/PR/CNTRY"},
{"IAQP (Out of State)",            "IA", "",    "B",   "B",  "B",  "",    "I",    "",     "",    "",      "CALL RST CNTY"},
{"Kansas QSO Party",               "KS", "T",   "B",   "B",  "B",  "",    "I",    "",     "",    "",      "CALL RST CNTY ST/PR/CNTRY"},
{"KSQP (Out of State)",            "KS", "",    "B",   "B",  "B",  "",    "I",    "",     "",    "",      "CALL RST CNTY"},
{"Kentucky QSO Party",             "KY", "T",   "B",   "B",  "B",  "",    "I",    "",     "",    "",      "CALL RST CNTY ST/PR/CNTRY"},
{"KYQP (Out of State)",            "KY", "",    "B",   "B",  "B",  "",    "I",    "",     "",    "",      "CALL RST CNTY"},
{"Louisiana QSO Party",            "LA", "T",   "B",   "B",  "B",  "",    "I",    "",     "",    "",      "CALL RST CNTY ST/PR/CNTRY"},
{"LAQP (Out of State)",            "LA", "",    "B",   "B",  "B",  "",    "I",    "",     "",    "",      "CALL RST CNTY"},
{"Maine QSO Party",                "ME", "T",   "B",   "B",  "B",  "",    "I",    "",     "",    "",      "CALL RST CNTY ST/PR/CNTRY"},
{"MEQP (Out of State)",            "ME", "",    "B",   "B",  "B",  "",    "I",    "",     "",    "",      "CALL RST CNTY"},
{"Maryland QSO Party",             "MD", "T",   "",    "B",  "B",  "",    "",     "",     "B",   "",      "CALL CAT CNTY ST/PR/CNTRY"},
{"MDQP (Out of State)",            "MD", "",    "",    "B",  "B",  "",    "",     "",     "B",   "",      "CALL CAT CNTY"},
{"Michigan QSO Party",             "MI", "T",   "",    "B",  "B",  "B",   "I",    "",     "",    "",      "CALL SERNO CNTY ST/PR/CNTRY"},
{"MIQP (Out of State)",            "MI", "",    "",    "B",  "B",  "B",   "I",    "",     "",    "",      "CALL SERNO CNTY"},
{"Minnesota QSO Party",            "MN", "T",   "",    "B",  "B",  "",    "I",    "B",    "",    "",      "CALL NAME CNTY ST/PR/CNTRY"},
{"MNQP (Out of State)",            "MN", "",    "",    "B",  "B",  "",    "I",    "B",    "",    "",      "CALL NAME CNTY"},
{"Missouri QSO Party",             "MO", "T",   "B",   "B",  "B",  "",    "I",    "",     "",    "",      "CALL RST CNTY ST/PR/CNTRY"},
{"MOQP (Out of State)",            "MO", "",    "B",   "B",  "B",  "",    "I",    "",     "",    "",      "CALL RST CNTY"},
{"Mississippi QSO Party",          "MS", "T",   "B",   "B",  "B",  "",    "I",    "",     "",    "",      "CALL RST CNTY ST/PR/CNTRY"},
{"MSQP (Out of State)",            "MS", "",    "B",   "B",  "B",  "",    "I",    "",     "",    "",      "CALL RST CNTY"},
{"Montana QSO Party",              "MT", "T",   "B",   "B",  "B",  "",    "B",    "",     "",    "",      "CALL RST CNTY ST/PR/CNTRY"},
{"MTQP (Out of State)",            "MT", "",    "B",   "B",  "B",  "",    "B",    "",     "",    "",      "CALL RST CNTY"},
{"North Carolina QSO Party",       "NC", "T",   "B",   "B",  "B",  "",    "I",    "",     "",    "",      "CALL CNTY ST/PR/CNTRY"},
{"NCQP (Out of State)",            "NC", "",    "B",   "B",  "B",  "",    "I",    "",     "",    "",      "CALL CNTY"},
{"Nebraska QSO Party",             "NE", "T",   "B",   "B",  "B",  "",    "B",    "",     "",    "",      "CALL RST CNTY ST/PR/CNTRY"},
{"NEQP (Out of State)",            "NE", "",    "B",   "B",  "B",  "",    "B",    "",     "",    "",      "CALL RST CNTY"},
{"New Jersey QSO Party",           "NJ", "T",   "B",   "B",  "B",  "",    "I",    "",     "",    "",      "CALL RST CNTY ST/PR/CNTRY"},
{"NJQP (Out of State)",            "NJ", "",    "B",   "B",  "B",  "",    "I",    "",     "",    "",      "CALL RST CNTY"},
{"New Mexico QSO Party",           "NM", "T",   "",    "B",  "B",  "",    "I",    "B",    "",    "",      "CALL NAME CNTY ST/PR/CNTRY"},
{"NMQP (Out of State)",            "NM", "",    "",    "B",  "B",  "",    "I",    "B",    "",    "",      "CALL NAME CNTY"},
{"New York QSO Party",             "NY", "T",   "B",   "B",  "B",  "",    "B",    "",     "",    "",      "CALL RST CNTY ST/PR/CNTRY"},
{"NYQP (Out of State)",            "NY", "",    "B",   "B",  "B",  "",    "B",    "",     "",    "",      "CALL RST CNTY"},
{"North Dakota QSO Party",         "ND", "T",   "B",   "B",  "B",  "",    "I",    "",     "",    "",      "CALL RST CNTY ST/PR/CNTRY"},
{"NDQP (Out of State)",            "ND", " ",   "B",   "B",  "B",  "",    "I",    "",     "",    "",      "CALL RST CNTY"},
{"Ohio QSO Party",                 "OH", "T",   "B",   "B",  "B",  "",    "B",    "",     "",    "",      "CALL RST CNTY ST/PR/CNTRY"},
{"OHQP (Out of State)",            "OH", "",    "B",   "B",  "B",  "",    "B",    "",     "",    "",      "CALL RST CNTY"},
{"Oklahoma QSO Party",             "OK", "T",   "B",   "B",  "B",  "",    "B",    "",     "",    "",      "CALL RST CNTY ST/PR/CNTRY"},
{"OKQP (Out of State)",            "OK", "",    "B",   "B",  "B",  "",    "B",    "",     "",    "",      "CALL RST CNTY"},
{"Ontario QSO Party",              "ON", "T",   "B",   "B",  "B",  "",    "B",    "",     "",    "",      "CALL RST CNTY ST/PR/CNTRY"},
{"ONQP (Out of Province)",         "ON", "",    "B",   "B",  "B",  "",    "B",    "",     "",    "",      "CALL RST CNTY"},
{"Pennsylvania QSO Party",         "PA", "T",   "",    "B",  "B",  "B",   "I",    "",     "",    "",      "CALL SERNO CNTY ST/PR/CNTRY"},
{"PAQP (Out of State)",            "PA", "",    "",    "B",  "B",  "B",   "I",    "",     "",    "",      "CALL SERNO CNTY"},
{"South Carolina QSO Party"  ,     "SC", "T",   "B",   "B",  "B",  "",    "",     "",     "",    "",      "CALL RST CNTY ST/PR/CNTRY"},
{"SCQP (Out of State)",            "SC", "",    "B",   "B",  "B",  "",    "",     "",     "",    "",      "CALL CNTY"},
{"South Dakota QSO Party",         "SD", "T",   "B",   "B",  "B",  "",    "B",    "",     "",    "",      "CALL RST CNTY ST/PR/CNTRY"},
{"SDQP (Out of State)",            "SD", "",    "B",   "B",  "B",  "",    "B",    "",     "",    "",      "CALL CNTY"},
{"Tennessee QSO Party",            "TN", "T",   "B",   "B",  "B",  "",    "B",    "",     "",    "",      "CALL RST CNTY ST/PR/CNTRY"},
{"TNQP (Out of State)",            "TN", "",    "B",   "B",  "B",  "",    "B",    "",     "",    "",      "CALL CNTY"},
{"Texas QSO Party",                "TX", "T",   "B",   "B",  "B",  "",    "I",    "",     "",    "",      "CALL RST CNTY ST/PR/CNTRY"},
{"TXQP (Out of State)",            "TX", "",    "B",   "B",  "B",  "",    "I",    "",     "",    "",      "CALL CNTY"},
{"Vermont QSO Party",              "VT", "T",   "B",   "B",  "B",  "",    "B",    "",     "",    "",      "CALL RST CNTY ST/PR/CNTRY"},
{"VTQP (Out of State)",            "VT", "",    "B",   "B",  "B",  "",    "B",    "",     "",    "",      "CALL CNTY"},
{"Virginia QSO Party",             "VA", "T",   "",    "B",  "B",  "B",   "I",    "",     "",    "",      "CALL RST CNTY ST/PR/CNTRY"},
{"VAQP (Out of State)",            "VA", "",    "",    "B",  "B",  "B",   "I",    "",     "",    "",      "CALL CNTY"},
{"Washington Salmon Run QSO Party","WA", "T",   "B",   "B",  "B",  "",    "B",    "",     "",    "",      "CALL RST CNTY ST/PR/CNTRY"},
{"WAQP (Out of State)",            "WA", "",    "B",   "B",  "B",  "",    "B",    "",     "",    "",      "CALL CNTY"},
{"Wisconsin QSO Party",            "WI", "T",   "B",   "B",  "B",  "",    "I",    "",     "",    "",      "CALL RST CNTY ST/PR/CNTRY"},
{"WIQP (Out of State)",            "WI", "",    "B",   "B",  "B",  "",    "I",    "",     "",    "",      "CALL CNTY"},
{"West Virginia QSO Party",        "WV", "T",   "B",   "B",  "B",  "",    "",     "",     "",    "",      "CALL RST CNTY ST/PR/CNTRY"},
{"WVQP (Out of State)",            "WV", "",    "B",   "B",  "B",  "",    "",     "",     "",    "",      "CALL CNTY"},
{"7QP QSO Party",                  "7QP","T",   "B",   "B",  "B",  "",    "I",    "",     "",    "S",     "CALL RST [(ST COUNTY) or (STCNTY) or (DX)]"},
{"7QP (Out of Region)",            "7QP","",    "B",   "B",  "B",  "",    "I",    "",     "",    "S",     "CALL RST [(ST COUNTY) or (STCNTY)]"},
{"New England QSO Party",          "6NE","T",   "B",   "B",  "B",  "",    "I",    "",     "",    "C",     "CALL RST [(ST COUNTY) or (CNTYST) or (DX)]"},
{"NEQP (Out of Region)",           "6NE","",    "B",   "B",  "B",  "",    "I",    "",     "",    "C",     "CALL RST [(ST COUNTY) or (CNTYST)]"},
{"",  "",  "",    "",  "",  "",  "",  "",  "",  "",  "",  ""}
};
//{"Alaska QSO Party",           "AK", "T",   "",    "",   "",   "",    "",     "",     "",    "",      ""},
//{"Connecticut QSO Party",      "CT", "T",   "",    "",   "",   "",    "",     "",     "",    "",      ""},
//{"Massachusetts QSO Party",    "MA", "T",   "",    "",   "",   "",    "",     "",     "",    "",      ""},
//{"New Hampshire QSO Party",    "NH", "T",   "",    "",   "",   "",    "",     "",     "",    "",      ""},
//{"Nevada QSO Party",           "NV", "T",   "",    "",   "",   "",    "",     "",     "",    "",      ""},
//{"Oregon QSO Party",           "OR", "T",   "",    "",   "",   "",    "",     "",     "",    "",      ""},
//{"Rhode Island QSO Party",     "RI", "T",   "",    "",   "",   "",    "",     "",     "",    "",      ""},
//{"Utah QSO Party",             "UT", "T",   "",    "",   "",   "",    "",     "",     "",    "",      ""},
//{"Wyoming QSO Party",          "WY", "T",   "",    "",   "",   "",    "",     "",     "",    "",      ""},

std::string contest_names()
{
	std::string _names;
	for (size_t n = 0; n < sizeof(contests) / sizeof(*contests) - 1; n++)
		_names.append(contests[n].name).append("|");
	return _names;
}

Ccontests QSOparties;

const std::string Ccontests::names()
{
	std::string _names;
	for (size_t n = 0; n < sizeof(qso_parties) / sizeof(*qso_parties) - 1; n++ )
		_names.append(qso_parties[n].contest).append("|");
	return _names;
}

const char *Ccontests::notes(std::string party)
{
	for (size_t n = 0; n < sizeof(qso_parties) / sizeof(*qso_parties); n++)
		if (party == qso_parties[n].contest)
			return qso_parties[n].notes;
	return "";
}


char Ccontests::rst(std::string party)
{
	for (size_t n = 0; n < sizeof(qso_parties) / sizeof(*qso_parties); n++)
		if (party == qso_parties[n].contest)
			return qso_parties[n].rst[0];
	return ' ';
}

char Ccontests::st(std::string party)
{
	for (size_t n = 0; n < sizeof(qso_parties) / sizeof(*qso_parties); n++)
		if (party == qso_parties[n].contest)
			return qso_parties[n].st[0];
	return  ' ';
}

char Ccontests::cnty(std::string party)
{
	for (size_t n = 0; n < sizeof(qso_parties) / sizeof(*qso_parties); n++)
		if (party == qso_parties[n].contest)
			return qso_parties[n].cnty[0];
	return ' ';
}

char Ccontests::serno(std::string party)
{
	for (size_t n = 0; n < sizeof(qso_parties) / sizeof(*qso_parties); n++)
		if (party == qso_parties[n].contest)
			return qso_parties[n].serno[0];
	return ' ';
}


char Ccontests::xchg(std::string party)
{
	for (size_t n = 0; n < sizeof(qso_parties) / sizeof(*qso_parties); n++)
		if (party == qso_parties[n].contest)
			return qso_parties[n].xchg[0];
	return ' ';
}

char Ccontests::name(std::string party)
{
	for (size_t n = 0; n < sizeof(qso_parties) / sizeof(*qso_parties); n++)
		if (party == qso_parties[n].contest)
			return qso_parties[n].name[0];
	return ' ';
}

char Ccontests::cat(std::string party)
{
	for (size_t n = 0; n < sizeof(qso_parties) / sizeof(*qso_parties); n++)
		if (party == qso_parties[n].contest)
			return qso_parties[n].cat[0];
	return ' ';
}

char Ccontests::stcty(std::string party)
{
	for (size_t n = 0; n < sizeof(qso_parties) / sizeof(*qso_parties); n++)
		if (party == qso_parties[n].contest)
			return qso_parties[n].stcty[0];
	return ' ';
}

void adjust_for_contest(void *)
{
	int n = progdefaults.SQSOcontest;

	progdefaults.SQSOinstate = false;
	progdefaults.SQSOlogcounty = false;
	progdefaults.SQSOlogstate = false;
	progdefaults.SQSOlogxchg = false;
	progdefaults.SQSOlogrst = false;
	progdefaults.SQSOlogname = false;
	progdefaults.SQSOlogserno = false;
	progdefaults.SQSOlogstcty = false;
	progdefaults.SQSOlogctyst = false;
	progdefaults.SQSOlogcat = false;

	if (QSOparties.qso_parties[n].instate[0] == 'T')
		progdefaults.SQSOinstate = true;

	if (progdefaults.SQSOinstate) {
		if (QSOparties.qso_parties[n].st[0] == 'I' || QSOparties.qso_parties[n].st[0] == 'B')
			progdefaults.SQSOlogstate = true;
		if (QSOparties.qso_parties[n].cnty[0] == 'I' || QSOparties.qso_parties[n].cnty[0] == 'B')
			progdefaults.SQSOlogcounty = true;
		if (QSOparties.qso_parties[n].xchg[0] == 'I' || QSOparties.qso_parties[n].xchg[0] == 'B')
			progdefaults.SQSOlogxchg = true;
		if (QSOparties.qso_parties[n].rst[0] == 'I' ||  QSOparties.qso_parties[n].rst[0] == 'B')
			progdefaults.SQSOlogrst = true;
		if (QSOparties.qso_parties[n].name[0] == 'I' || QSOparties.qso_parties[n].name[0] == 'B')
			progdefaults.SQSOlogname = true;
		if (QSOparties.qso_parties[n].serno[0] == 'I' || QSOparties.qso_parties[n].serno[0] == 'B')
			progdefaults.SQSOlogserno = true;
		if (QSOparties.qso_parties[n].cat[0] == 'I' || QSOparties.qso_parties[n].cat[0] == 'B')
			progdefaults.SQSOlogcat = true;
	} else {
		if (QSOparties.qso_parties[n].st[0] == 'O' || QSOparties.qso_parties[n].st[0] == 'B')
			progdefaults.SQSOlogstate = true;
		if (QSOparties.qso_parties[n].cnty[0] == 'O' || QSOparties.qso_parties[n].cnty[0] == 'B')
			progdefaults.SQSOlogcounty = true;
		if (QSOparties.qso_parties[n].xchg[0] == 'O' || QSOparties.qso_parties[n].xchg[0] == 'B')
			progdefaults.SQSOlogxchg = true;
		if (QSOparties.qso_parties[n].rst[0] == 'O' || QSOparties.qso_parties[n].rst[0] == 'B')
			progdefaults.SQSOlogrst = true;
		if (QSOparties.qso_parties[n].name[0] == 'O' || QSOparties.qso_parties[n].name[0] == 'B')
			progdefaults.SQSOlogname = true;
		if (QSOparties.qso_parties[n].serno[0] == 'O' || QSOparties.qso_parties[n].serno[0] == 'B')
			progdefaults.SQSOlogserno = true;
		if (QSOparties.qso_parties[n].cat[0] == 'O' || QSOparties.qso_parties[n].cat[0] == 'B')
			progdefaults.SQSOlogcat = true;
	}
	if (QSOparties.qso_parties[n].stcty[0] == 'S')
		progdefaults.SQSOlogstcty = true;
	if (QSOparties.qso_parties[n].stcty[0] == 'C')
		progdefaults.SQSOlogctyst = true;

	update_main_title();

//std::cout << "QSOparties.qso_parties[" << n << "]\n"
//		  << QSOparties.qso_parties[n].contest << std::endl
//		  << "instate: " << progdefaults.SQSOinstate << std::endl
//		  << "ST:      " << QSOparties.qso_parties[n].state << std::endl
//		  << "rRST:    " << QSOparties.qso_parties[n].rst << std::endl
//		  << "rST:     " << QSOparties.qso_parties[n].st << std::endl
//		  << "rCY:     " << QSOparties.qso_parties[n].cnty << std::endl
//		  << "rSER:    " << QSOparties.qso_parties[n].serno << std::endl
//		  << "rXCHG:   " << QSOparties.qso_parties[n].xchg << std::endl
//		  << "rNAM:    " << QSOparties.qso_parties[n].name << std::endl
//		  << "rCAT:    " << QSOparties.qso_parties[n].cat << std::endl
//		  << "STCTY:   " << QSOparties.qso_parties[n].stcty << std::endl
//		  << "Notes:   " << QSOparties.qso_parties[n].notes << std::endl;

}

