// ----------------------------------------------------------------------------
// states.h
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

// Extracted from FIPS 2010 census data

#ifndef _COUNTIES_H
#define _COUNTIES_H

#include <string>
#include <vector>

struct STATE_COUNTY_QUAD {
	std::string state;		// state long name
	std::string ST;			// state abbreviated
	std::string county;		// county long name
	std::string CTY;		// county abbreviated
};

class Cstates {
private:
	size_t next;
public:
	Cstates() {}
	~Cstates(){}

//	static struct STATE_COUNTY_QUAD vec_SQSO[];

	bool valid_county( std::string st, std::string cty );
	const std::string names();
	const std::string counties(std::string st);
	const std::string county(std::string st, std::string cnty);
	const std::string cnty_short(std::string st, std::string cnty);
	const std::string state(std::string ST);
	const std::string state_short(std::string ST);
};

extern Cstates states;

extern const std::string counties();
//extern const std::string six_qp_counties();
//extern const std::string seven_qp_counties();

extern void load_counties();
extern void save_counties();

extern std::vector<STATE_COUNTY_QUAD> vec_SQSO;
extern std::vector<STATE_COUNTY_QUAD> vec_6QP;
extern std::vector<STATE_COUNTY_QUAD> vec_7QP;

extern const char *szSQSO;
extern const char *sz7QP;
extern const char *szNEQP;

#endif
