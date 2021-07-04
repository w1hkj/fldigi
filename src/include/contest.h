// ----------------------------------------------------------------------------
// contest.h
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

#ifndef _CONTEST_H
#define _CONTEST_H

#include <string>

enum CONTEST_FIELD {
cCALL, cMODE, cNAME, cQTH,
cSTATE, cVE_PROV, cDIST,
cGRIDSQUARE, cCNTY, cCOUNTRY,
cCQZ, cDXCC, cIOTA, cITUZ, cCONT,
cSRX, cXCHG1,
cSS_SERNO, cSS_PREC, cSS_CHK, cSS_SEC,
cROOKIE, cCHECK,
cKD_XCHG, cARR_XCHG,
c1010, cRST, cNUMERIC,
cCLASS, cFD_CLASS, cWFD_CLASS, cASCR_CLASS,
cARRL_SECT, cFD_SECTION,
cITALIAN
};

struct CONTESTS {
	std::string name;
	std::string notes;
};

// Contest, ST, TEST, rRST , rST, rCY, rSERNO, rXCHG, rNAME, rCAT, rSTCTY, Notes

struct QSOP {
	const char *contest;
	const char *state;
	const char *instate;
	const char *rst;
	const char *st;
	const char *cnty;
	const char *serno;
	const char *xchg;
	const char *name;
	const char *cat;
	const char *stcty;
	const char *notes;
};

class Ccontests {
private:
public:
	static struct QSOP qso_parties[];

	Ccontests() {};
	~Ccontests() {};

	const std::string names();
	char rst(std::string party);
	char st(std::string party);
	char cnty(std::string party);
	char serno(std::string party);
	char sect(std::string party);
	char xchg(std::string party);
	char name(std::string party);
	char cat(std::string party);
	char stcty(std::string party);
	const char *notes(std::string party);
};

extern Ccontests QSOparties;

extern bool cut_numeric_test(std::string s);
extern std::string cut_to_numeric(std::string s);
extern int check_field(std::string s1, CONTEST_FIELD, std::string s2="");

extern void adjust_for_contest(void *);

extern CONTESTS contests[];
extern std::string contest_names();

extern std::string country_match;
extern bool state_test(std::string s);
extern bool province_test(std::string s);
extern bool country_test(std::string s);
extern bool county_test(std::string st, std::string cty);
extern bool section_test(std::string s);

#endif
