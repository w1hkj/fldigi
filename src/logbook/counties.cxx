// ----------------------------------------------------------------------------
// counties.cxx
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

#include <iostream>
#include <fstream>

#include "main.h"
#include "counties.h"
#include "contest.h"
#include "configuration.h"
#include "debug.h"

#include "strutil.h"

//----------------------------------------------------------------------
Cstates states;

std::vector<STATE_COUNTY_QUAD> vec_SQSO;
std::vector<STATE_COUNTY_QUAD> vec_6QP;
std::vector<STATE_COUNTY_QUAD> vec_7QP;

void load_from_string(std::string &str, vector<STATE_COUNTY_QUAD> &vec)
{
	size_t ptr1 = 0;
	size_t ptr2 = 0;
	size_t ptr3 = 0;

	std::string line;
	STATE_COUNTY_QUAD scq;

	vec.clear();

// eat first line
	ptr1 = str.find("\n");
	line = str.substr(ptr1);

	LOG_INFO("%s data read from internal data string",
			(&vec == &vec_SQSO ? "SQSO" :
			 &vec == &vec_6QP ? "NEQP" : "7QP"));

	ptr1++;
	ptr2 = str.find("\n", ptr1);

	while (ptr2 != std::string::npos) {
		line = str.substr(ptr1, ptr2 - ptr1);
		if (line.empty()) break;

		ptr3 = line.find(",");
		scq.state = line.substr(0,ptr3);
		line.erase(0, ptr3 + 1);

		ptr3 = line.find(",");
		scq.ST = line.substr(0,ptr3);
		line.erase(0, ptr3 + 1);

		ptr3 = line.find(",");
		scq.county = line.substr(0,ptr3);
		line.erase(0, ptr3 + 1);

		scq.CTY = line;

		if (!scq.ST.empty()) vec.push_back(scq);

		ptr1 = ptr2 + 1;
		ptr2 = str.find("\n", ptr1);
	}

	LOG_INFO("Read %d records", (int)vec.size());
}

void load_from_file( std::string &fname, vector<STATE_COUNTY_QUAD> &vec) 
{
	std::ifstream csvfile(fname.c_str());
	if (!csvfile) return;

	vec.clear();

	std::string line;
	line.reserve(1024);
	char str[1024];

	STATE_COUNTY_QUAD scq;

// eat the header line
	memset(str, 0, 1024);
	csvfile.getline(str, 1024);

	LOG_INFO("%s data read from %s",
			(&vec == &vec_SQSO ? "SQSO" :
			 &vec == &vec_6QP ? "NEQP" : "7QP"),
			fname.c_str());

	size_t ptr = 0;

	while (!csvfile.eof()) {
		memset(str, 0, 1024);
		csvfile.getline(str, 1024);
		line = str;
		if (line.empty()) break;

		ptr = line.find(",");
		scq.state = line.substr(0,ptr);
		line.erase(0, ptr + 1);

		ptr = line.find(",");
		scq.ST = line.substr(0,ptr);
		line.erase(0, ptr + 1);

		ptr = line.find(",");
		scq.county = line.substr(0,ptr);
		line.erase(0, ptr + 1);

		scq.CTY = line;

		if (!scq.ST.empty()) vec.push_back(scq);
	}
	csvfile.close();

	LOG_INFO("Read %d records", (int)vec.size());

}

void load_SQSO()
{
	std::string cnty_file = DATA_dir;
	cnty_file.append("SQSO.txt");
	std::ifstream csvfile(cnty_file.c_str());

	if (!csvfile) {
		std::string str = szSQSO;
		load_from_string( str, vec_SQSO );
		return;
	}
	csvfile.close();
	load_from_file( cnty_file, vec_SQSO );
}

void load_7qp()
{
	std::string cnty_file = DATA_dir;
	cnty_file.append("7QP.txt");
	std::ifstream csvfile(cnty_file.c_str());

	if (!csvfile) {
		std::string str = sz7QP;
		load_from_string( str, vec_7QP );
		return;
	}
	csvfile.close();
	load_from_file( cnty_file, vec_7QP );
}

void load_neqp()
{
	std::string cnty_file = DATA_dir;
	cnty_file.append("NEQP.txt");
	std::ifstream csvfile(cnty_file.c_str());

	if (!csvfile) {
		std::string str = szNEQP;
		load_from_string(str, vec_6QP);
		return;
	}
	csvfile.close();
	load_from_file( cnty_file, vec_6QP );
}

void save_SQSO()
{
	std::string cnty_file = DATA_dir;
	cnty_file.append("SQSO.txt");
	std::ofstream csvfile(cnty_file.c_str());

	if (!csvfile) {
//		std::cout << "cannot create " << cnty_file << std::endl;
		return;
	}
	csvfile << "State/Province, ST/PR, County/City/Disrict, CCD" << std::endl;

	for (size_t n = 0; n < vec_SQSO.size(); n++ )
		csvfile << vec_SQSO[n].state << ","
				<< vec_SQSO[n].ST << ","
				<< vec_SQSO[n].county << ","
				<< vec_SQSO[n].CTY
				<< std::endl;

	csvfile.close();

//std::cout << vec_SQSO.size() << " records written to " << cnty_file << std::endl;
}

void save_7qp()
{
	std::string cnty_file = DATA_dir;
	cnty_file.append("7QP.txt");
	std::ofstream csvfile(cnty_file.c_str());

	if (!csvfile) {
//		std::cout << "cannot create " << cnty_file << std::endl;
		return;
	}
	csvfile << "State, ST, County/City, CC" << std::endl;

	for (size_t n = 0; n < vec_7QP.size(); n++)
		csvfile << vec_7QP[n].state << ","
				<< vec_7QP[n].ST << ","
				<< vec_7QP[n].county << ","
				<< vec_7QP[n].CTY
				<< std::endl;

	csvfile.close();
//std::cout << vec_7QP.size() << " records written to " << cnty_file << std::endl;
}

void save_neqp()
{
	std::string cnty_file = DATA_dir;
	cnty_file.append("NEQP.txt");

	std::ofstream csvfile(cnty_file.c_str());

	if (!csvfile) {
//		std::cout << "cannot create " << cnty_file << std::endl;
		return;
	}
	csvfile << "State, ST, County/City, CC" << std::endl;

	for (size_t n = 0; n < vec_6QP.size(); n++)
		csvfile << vec_6QP[n].state << ","
				<< vec_6QP[n].ST << ","
				<< vec_6QP[n].county << ","
				<< vec_6QP[n].CTY
				<< std::endl;

	csvfile.close();

//std::cout << vec_6QP.size() << " records written to " << cnty_file << std::endl;
}

void save_counties()
{
	save_SQSO();
	save_7qp();
	save_neqp();
}

void load_counties()
{
	load_SQSO();
	load_7qp();
	load_neqp();
}

//----------------------------------------------------------------------

bool Cstates::valid_county( string st, string cnty )
{
	string ST = ucasestr(st);
	string CNTY = ucasestr(cnty);
	string dST, dCNTY, dCOUNTY;


	if (std::string(QSOparties.qso_parties[progdefaults.SQSOcontest].state) == "7QP") {
		for (size_t n = 0; n < vec_7QP.size(); n++) {
			dST = ucasestr(vec_7QP[n].ST);
			dCNTY = ucasestr(vec_7QP[n].CTY);
			dCOUNTY = ucasestr(vec_7QP[n].county);
			if ( ST != dST ) continue;
			if (CNTY == dCNTY) return true;
			if (CNTY == dCOUNTY) return true;
		}
	} else if (std::string(QSOparties.qso_parties[progdefaults.SQSOcontest].state) == "6NE") {
		for (size_t n = 0; n < vec_6QP.size(); n++) {
			dST = ucasestr(vec_6QP[n].ST);
			dCNTY = ucasestr(vec_6QP[n].CTY);
			dCOUNTY = ucasestr(vec_6QP[n].county);
			if ( ST != dST ) continue;
			if (CNTY == dCNTY) return true;
			if (CNTY == dCOUNTY) return true;
		}
	} else {
		for (size_t n = 0; n < vec_SQSO.size(); n++) {
			dST = ucasestr(vec_SQSO[n].ST);
			dCNTY = ucasestr(vec_SQSO[n].CTY);
			dCOUNTY = ucasestr(vec_SQSO[n].county);
			if ( ST != dST ) continue;
			if (CNTY == dCNTY)
				return true;
			if (CNTY == dCOUNTY)
				return true;
		}
	}
	return false;
}

const string Cstates::names()
{
	string _names = vec_SQSO[0].state;
	string _st = vec_SQSO[0].ST;
	for (size_t n = 0; n < vec_SQSO.size(); n++) {
		if (_st != vec_SQSO[n].ST && !vec_SQSO[n].ST.empty()) {
			_names.append("|").append(vec_SQSO[n].state);
			_st = vec_SQSO[n].ST;
		}
	}
	return _names;
}

const string Cstates::state_short(string ST)  // ST can be either short or long form
{
	for (size_t n = 0; n < vec_SQSO.size(); n++)
		if (ST == vec_SQSO[n].ST || ST == vec_SQSO[n].state)
			return vec_SQSO[n].ST;
	return "";
}

const string Cstates::state(string ST) // ST can be either short or long form
{
	for (size_t n = 0; n < vec_SQSO.size(); n++)
		if (ST == vec_SQSO[n].ST || ST == vec_SQSO[n].state)
			return vec_SQSO[n].state;
	return "";
}

const string Cstates::counties(string ST)
{
	string _counties = "";
	if (ST == "NIL") return _counties;
	size_t n = 0;

	if (std::string(QSOparties.qso_parties[progdefaults.SQSOcontest].state) == "7QP") {
		for (n = 0; n < vec_7QP.size(); n++) {
			if (ST == vec_7QP[n].ST || ST == vec_7QP[n].state) {
				ST = vec_7QP[n].ST;
				if (!_counties.empty() ) _counties.append("|");
				_counties.append(vec_7QP[n].county);
				continue;
			}
			if (!_counties.empty() && ST != vec_7QP[n].ST && ST != vec_7QP[n].state ) break;
		}
	} else if (std::string(QSOparties.qso_parties[progdefaults.SQSOcontest].state) == "6NE") {
		for (size_t n = 0; n < vec_6QP.size(); n++) {
			if (ST == vec_6QP[n].ST || ST == vec_6QP[n].state) {
				ST = vec_6QP[n].ST;
				if (!_counties.empty() ) _counties.append("|");
				_counties.append(vec_6QP[n].county);
				continue;
			}
			if (!_counties.empty() && ST != vec_6QP[n].ST && ST != vec_6QP[n].state ) break;
		}
	} else {
		for (n = 0; n < vec_SQSO.size(); n++) {
			if (ST == vec_SQSO[n].ST || ST == vec_SQSO[n].state) {
				ST = vec_SQSO[n].ST;
				if (!_counties.empty() ) _counties.append("|");
				_counties.append(vec_SQSO[n].county);
				continue;
			}
			if (!_counties.empty() && ST != vec_SQSO[n].ST && ST != vec_SQSO[n].state ) break;
		}
	}
	return _counties;
}

const string Cstates::cnty_short( string st, string cnty) // st/cnty can be either short or long
{
	string ST = ucasestr(st);
	string CNTY = ucasestr(cnty);
	string dSTATE, dST, dCNTY, dCOUNTY;
	string answer = "";
	size_t n = 0;
	bool OK = false;
	if (std::string(QSOparties.qso_parties[progdefaults.SQSOcontest].state) == "7QP") {
		for (n = 0; n < vec_7QP.size(); n++) {
			dST = ucasestr(vec_7QP[n].ST);
			dSTATE = ucasestr(vec_7QP[n].state);
			if (ST == dST) {OK = true; break;}
			if (ST == dSTATE) { OK = true; break; }
		}
		if (!OK) return answer;

		ST = vec_7QP[n].ST;
		for (size_t k = n; ST == vec_7QP[k].ST, k < vec_7QP.size(); k++) {
			dCNTY = ucasestr(vec_7QP[k].CTY);
			dCOUNTY = ucasestr(vec_7QP[k].county);
			if (CNTY == dCNTY) {
				answer = vec_7QP[k].CTY;
				break;
			}
			if (CNTY == dCOUNTY) {
				answer = vec_7QP[k].CTY;
				break;
			}
		}
	} else if (std::string(QSOparties.qso_parties[progdefaults.SQSOcontest].state) == "6NE") {
		for (n = 0; n < vec_6QP.size(); n++) {
			dST = ucasestr(vec_6QP[n].ST);
			dSTATE = ucasestr(vec_6QP[n].state);
			if (ST == dST) {OK = true; break;}
			if (ST == dSTATE) { OK = true; break; }
		}
		if (!OK) return answer;

		ST = vec_6QP[n].ST;
		for (size_t k = n; ST == vec_6QP[k].ST, k < vec_6QP.size(); k++) {
			dCNTY = ucasestr(vec_6QP[k].CTY);
			dCOUNTY = ucasestr(vec_6QP[k].county);
			if (CNTY == dCNTY) {
				answer = vec_6QP[k].CTY;
				break;
			}
			if (CNTY == dCOUNTY) {
				answer = vec_6QP[k].CTY;
				break;
			}
		}
	} else {
		for (n = 0; n < vec_SQSO.size(); n++) {
			dST = ucasestr(vec_SQSO[n].ST);
			dSTATE = ucasestr(vec_SQSO[n].state);
			if (ST == dST) {OK = true; break;}
			if (ST == dSTATE) { OK = true; break; }
		}
		if (!OK) return answer;

		ST = vec_SQSO[n].ST;
		for (size_t k = n; ST == vec_SQSO[k].ST, k < vec_SQSO.size(); k++) {
			dCNTY = ucasestr(vec_SQSO[k].CTY);
			dCOUNTY = ucasestr(vec_SQSO[k].county);
			if (CNTY == dCNTY) {
				answer = vec_SQSO[k].CTY;
				break;
			}
			if (CNTY == dCOUNTY) {
				answer = vec_SQSO[k].CTY;
				break;
			}
		}
	}

	return answer;
}

const string Cstates::county( string st, string cnty) // st/cnty can be either short or long
{
	string ST = ucasestr(st);
	string CNTY = ucasestr(cnty);
	string dST, dCNTY, dCOUNTY;
	size_t n = 0;

	if (std::string(QSOparties.qso_parties[progdefaults.SQSOcontest].state) == "7QP") {
		for (n = 0; n < vec_7QP.size(); n++) {
			dST = ucasestr(vec_7QP[n].ST);
			if (ST == dST) break;
		}
		if (ST != dST) return "";

		ST = vec_7QP[n].ST;
		for (size_t k = n; ST == vec_7QP[k].ST, k < vec_7QP.size(); k++) {
			dCNTY = ucasestr(vec_7QP[k].CTY);
			dCOUNTY = ucasestr(vec_7QP[k].county);
			if (CNTY == dCNTY) return vec_7QP[k].county;
			if (CNTY == dCOUNTY) return vec_7QP[k].county;
		}
		return "";
	} else if (std::string(QSOparties.qso_parties[progdefaults.SQSOcontest].state) == "6NE") {
		for (n = 0; n < vec_6QP.size(); n++) {
			dST = ucasestr(vec_6QP[n].ST);
			if (ST == dST) break;
		}
		if (ST != dST) return "";

		ST = vec_6QP[n].ST;
		for (size_t k = n; ST == vec_6QP[k].ST, k < vec_6QP.size(); k++) {
			if (vec_6QP[k].ST.empty()) return "";
			dCNTY = ucasestr(vec_6QP[k].CTY);
			dCOUNTY = ucasestr(vec_6QP[k].county);
			if (CNTY == dCNTY) return vec_6QP[k].county;
			if (CNTY == dCOUNTY) return vec_6QP[k].county;
		}
		return "";
	} else {
		for (n = 0; !vec_SQSO[n].ST.empty(); n++) {
			dST = ucasestr(vec_SQSO[n].ST);
			if (ST == dST) break;
		}
		if (ST != dST) return "";

		ST = vec_SQSO[n].ST;
		for (size_t k = n; ST == vec_SQSO[k].ST, k < vec_SQSO.size(); k++) {
			if (vec_SQSO[k].ST.empty()) return "";
			dCNTY = ucasestr(vec_SQSO[k].CTY);
			dCOUNTY = ucasestr(vec_SQSO[k].county);
			if (CNTY == dCNTY) return vec_SQSO[k].county;
			if (CNTY == dCOUNTY) return vec_SQSO[k].county;
		}
		return "";
	}
	return "";
}

static string __counties;

const string counties()
{
	load_counties();
	if (vec_SQSO.empty())
		return "";
	__counties.clear();
	for (size_t n = 0; n < vec_SQSO.size(); n++ ) {
		__counties.append(vec_SQSO[n].ST).append(" ").append(vec_SQSO[n].county);
		if (n < (vec_SQSO.size() - 1)) 
			__counties.append("|");
	}
	return __counties;
}

/*
const string seven_qp_counties()
{
	load_7qp();
	if (vec_7QP.empty())
		return "";
	__counties.clear();
	for (size_t n = 0; n < vec_7QP.size(); n++ ) {
		__counties.append(vec_7QP[n].ST).append(" ").append(vec_7QP[n].county).append("|");
		if (n < (vec_7QP.size() - 1)) 
			__counties.append("|");
	}
	return __counties;
}

const string six_qp_counties()
{
	load_neqp();
	if (vec_6QP.empty())
		return "";
	__counties.clear();
	for (size_t n = 0; n < vec_6QP.size(); n++ ) {
		__counties.append(vec_6QP[n].ST).append(" ").append(vec_6QP[n].county).append("|");
		if (n < (vec_6QP.size() - 1)) 
			__counties.append("|");
	}
	return __counties;
}
*/

