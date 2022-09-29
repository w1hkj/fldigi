// ----------------------------------------------------------------------------
// dxcc.cxx
//
// Copyright (C) 2008-2010
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

#include <cstring>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <list>
#include <map>
#include <algorithm>

#include <FL/filename.H>
#include "fileselect.h"

#include "gettext.h"
#include "dxcc.h"
#include "debug.h"
#include "configuration.h"
#include "confdialog.h"
#include "main.h"

#if HAVE_STD_HASH
#	include <unordered_map>
	using std::unordered_map;
	typedef std::unordered_map<std::string, dxcc*> dxcc_map_t;
	typedef std::unordered_map<std::string, unsigned char> qsl_map_t;
#elif HAVE_STD_TR1_HASH
#	include <tr1/unordered_map>
	using std::tr1::unordered_map;
	typedef std::tr1::unordered_map<std::string, dxcc*> dxcc_map_t;
	typedef std::tr1::unordered_map<std::string, unsigned char> qsl_map_t;
#else
#	error "No std::hash or std::tr1::hash support"
#endif

dxcc::dxcc(const char* cn, int cq, int itu, const char* ct, float lat, float lon, float tz)
	: country(cn), cq_zone(cq), itu_zone(itu), latitude(lat), longitude(lon), gmt_offset(tz)
{
	if (*ct) {
		continent[0] = ct[0];
		continent[1] = ct[1];
	}
	continent[2] = '\0';
}

typedef std::vector<dxcc*> dxcc_list_t;
static dxcc_map_t* cmap = 0;
static dxcc_list_t* clist = 0;
static std::vector<std::string>* cnames = 0;

static std::list<std::string> lnames;
std::string cbolist;

static void add_prefix(std::string& prefix, dxcc* entry);

extern std::string s_ctydat;

// comparison, not case sensitive.
static bool compare_nocase (const std::string& first, const std::string& second)
{
	unsigned int i=0;
	while ( (i<first.length()) && (i<second.length()) )
	{
		if (tolower(first[i]) < tolower(second[i])) return true;
		else if (tolower(first[i]) > tolower(second[i])) return false;
		++i;
	}
	return ( first.length() < second.length() );
}

bool dxcc_internal_data()
{

	std::string tempfname = HomeDir;
	tempfname.append("/temp/ctydat.txt");
	std::ofstream out(tempfname.c_str());
	if (!out) {
		LOG_INFO("Could not write temp file %s", tempfname.c_str());
		return false;
	}
	out << s_ctydat;
	out.close();

	std::ifstream in(tempfname.c_str());
	if (!in) {
		LOG_INFO("Could not read temp file %s", tempfname.c_str());
		return false;
	}

	LOG_INFO("Using internal cty.dat data");

	cmap = new dxcc_map_t;
	cnames = new std::vector<std::string>;

// this MUST be greater than the actual number of dcxx entities or
// the Windows gcc string library will move all of the strings and
// destroy the integrity of the cmap c_str() pointer to the country
// string in cnames

	cnames->reserve(500); // approximate number of dxcc entities

	clist = new dxcc_list_t;
	clist->reserve(500);

	dxcc* entry;
	std::string record;

	unsigned nrec = 0;
	while (getline(in, record, ';')) {
		std::istringstream is(record);
		entry = new dxcc;
		nrec++;

		// read country name
		cnames->resize(cnames->size() + 1);
		clist->push_back(entry);
		getline(is, cnames->back(), ':');
		entry->country = cnames->back().c_str();

		lnames.push_back(entry->country);

		// cq zone
		(is >> entry->cq_zone).ignore();
		// itu zone
		(is >> entry->itu_zone).ignore();
		// continent
		(is >> std::ws).get(entry->continent, 3).ignore();

		// latitude
		(is >> entry->latitude).ignore();
		// longitude
		(is >> entry->longitude).ignore();
		// gmt offset
		(is >> entry->gmt_offset).ignore(256, '\n');

		// prefixes and exceptions
		int c;
		std::string prefix;
		while ((c = is.peek()) == ' ' || c == '\n') {
			is >> std::ws;

			while (getline(is, prefix, ',')) {
				add_prefix(prefix, entry);
				if ((c = is.peek()) == '\n')
					break;
			}
		}

		if (cnames->back() == "United States") {
			dxcc *usa_entry = new dxcc(
				"USA",
				entry->cq_zone,
				entry->itu_zone,
				entry->continent,
				entry->latitude,
				entry->longitude,
				entry->gmt_offset );
			nrec++;
			cnames->resize(cnames->size() + 1);
			clist->push_back(usa_entry);
			lnames.push_back("USA");
		}

		in >> std::ws;

	}
	lnames.sort(compare_nocase);

	cbolist.clear();
	std::list<std::string>::iterator p = lnames.begin();
	while (p != lnames.end()) {
		cbolist.append(*p);
		p++;
		if (p != lnames.end()) cbolist.append("|");
	}

	std::stringstream info;
	info << "\nLoaded " << cmap->size() << " prefixes for " << nrec
	     << " countries from internal cty.dat\n"
		 << "You should download the latest from http://www.country-files.com";
	LOG_INFO("%s", info.str().c_str());

	remove(tempfname.c_str());

	return true;
}

bool dxcc_open(const char* filename)
{
	if (cmap) {
		LOG_INFO("cty.dat already loaded");
		return true;
	}

	std::ifstream in(filename);
	if (!in) {
		LOG_INFO("Could not read contest country file \"%s\"", filename);
		return dxcc_internal_data();
	}

	cmap = new dxcc_map_t;
	cnames = new std::vector<std::string>;

// this MUST be greater than the actual number of dcxx entities or
// the Windows gcc string library will move all of the strings and
// destroy the integrity of the cmap c_str() pointer to the country
// string in cnames

	cnames->reserve(500); // approximate number of dxcc entities

	clist = new dxcc_list_t;
	clist->reserve(500);

	dxcc* entry;
	std::string record;

	unsigned nrec = 0;
	while (getline(in, record, ';')) {
		std::istringstream is(record);
		entry = new dxcc;
		nrec++;

		// read country name
		cnames->resize(cnames->size() + 1);
		clist->push_back(entry);
		getline(is, cnames->back(), ':');
		entry->country = cnames->back().c_str();

		lnames.push_back(entry->country);

		// cq zone
		(is >> entry->cq_zone).ignore();
		// itu zone
		(is >> entry->itu_zone).ignore();
		// continent
		(is >> std::ws).get(entry->continent, 3).ignore();

		// latitude
		(is >> entry->latitude).ignore();
		// longitude
		(is >> entry->longitude).ignore();
		// gmt offset
		(is >> entry->gmt_offset).ignore(256, '\n');

		// prefixes and exceptions
		int c;
		std::string prefix;
		while ((c = is.peek()) == ' ' || c == '\r' || c == '\n') {
			is >> std::ws;

			while (getline(is, prefix, ',')) {
				add_prefix(prefix, entry);
				if ((c = is.peek()) == '\r' || c == '\n')
					break;
			}
		}

		if (cnames->back() == "United States") {
			dxcc *usa_entry = new dxcc(
				"USA",
				entry->cq_zone,
				entry->itu_zone,
				entry->continent,
				entry->latitude,
				entry->longitude,
				entry->gmt_offset );
			nrec++;
			cnames->resize(cnames->size() + 1);
			clist->push_back(usa_entry);
			lnames.push_back("USA");
		}

		in >> std::ws;

	}
	lnames.sort(compare_nocase);

	cbolist.clear();
	std::list<std::string>::iterator p = lnames.begin();
	while (p != lnames.end()) {
		cbolist.append(*p);
		p++;
		if (p != lnames.end()) cbolist.append("|");
	}

	std::stringstream info;
	info << "Loaded " << cmap->size() << " prefixes for " << nrec << " countries";
	LOG_VERBOSE("%s", info.str().c_str());

	return true;
}

bool dxcc_is_open(void)
{
	return cmap;
}

void dxcc_close(void)
{
	if (!cmap)
		return;
	delete cnames;
	cnames = 0;
	std::map<dxcc*, bool> rm;
	for (dxcc_map_t::iterator i = cmap->begin(); i != cmap->end(); ++i)
		if (rm.insert(std::make_pair(i->second, true)).second)
			delete i->second;
	delete cmap;
	cmap = 0;
	delete clist;
	clist = 0;
}

const std::vector<dxcc*>* dxcc_entity_list(void)
{
	return clist;
}

const dxcc* dxcc_lookup(const char* callsign)
{
	if (!cmap || !callsign || !*callsign)
		return NULL;

	std::string sstr;
	sstr.resize(strlen(callsign) + 1);
	transform(callsign, callsign + sstr.length() - 1, sstr.begin() + 1, static_cast<int (*)(int)>(toupper));

	// first look for a full callsign (prefixed with '=')
	sstr[0] = '=';
	dxcc_map_t::const_iterator entry = cmap->find(sstr);
	if (entry != cmap->end()) {
		entry->second->print();
		return entry->second;
	}
	// erase the '=' and do a longest prefix search
	sstr.erase(0, 1);
	size_t len = sstr.length();
// accomodate special case for KG4... calls
// all two letter suffix KG4 calls are Guantanamo
// all others are US non Guantanamo
	if (sstr.find("KG4") != std::string::npos) {
		if (len == 4 || len == 6) {
			sstr = "K";
			len = 1;
		}
	}
	do {
		sstr.resize(len--);
		if ((entry = cmap->find(sstr)) != cmap->end()) {
		entry->second->print();
			return entry->second;
		}
	} while (len);

	return NULL;
}

static void add_prefix(std::string& prefix, dxcc* entry)
{
	std::string::size_type i = prefix.find_first_of("([<{");
	if (likely(i == std::string::npos)) {
		(*cmap)[prefix] = entry;
		return;
	}

	std::string::size_type j = i, first = i;
	do {
		entry = new struct dxcc(*entry);
		switch (prefix[i++]) { // increment i past opening bracket
		case '(':
			if ((j = prefix.find(')', i)) == std::string::npos) {
				delete entry;
				return;
			}
			prefix[j] = '\0';
			entry->cq_zone = atoi(prefix.data() + i);
			break;
		case '[':
			if ((j = prefix.find(']', i)) == std::string::npos) {
				delete entry;
				return;
			}
			prefix[j] = '\0';
			entry->itu_zone = atoi(prefix.data() + i);
			break;
		case '<':
			if ((j = prefix.find('/', i)) == std::string::npos) {
				delete entry;
				return;
			}
			prefix[j] = '\0';
			entry->latitude = atof(prefix.data() + i);
			if ((j = prefix.find('>', j)) == std::string::npos) {
				delete entry;
				return;
			}
			prefix[j] = '\0';
			entry->longitude = atof(prefix.data() + i);
			break;
		case '{':
			if ((j = prefix.find('}', i)) == std::string::npos) {
				delete entry;
				return;
			}
			memcpy(entry->continent, prefix.data() + i, 2);
			break;
		}
	} while ((i = prefix.find_first_of("([<{", j)) != std::string::npos);

	prefix.erase(first);
	(*cmap)[prefix] = entry;
}

static qsl_map_t* qsl_calls;
static unsigned char qsl_open_;
const char* qsl_names[] = { "LoTW", "eQSL" };

bool qsl_open(const char* filename, qsl_t qsl_type)
{
	std::ifstream in(filename);
	if (!in)
		return false;
	if (!qsl_calls)
		qsl_calls = new qsl_map_t;

	size_t n = qsl_calls->size();
	std::string::size_type p;
	std::string s;
	s.reserve(32);
	while (getline(in, s)) {
		if ((p = s.rfind('\r')) != std::string::npos)
			s.erase(p);
		(*qsl_calls)[s] |= (1 << qsl_type);
	}

	std::stringstream info;
	info << "Added " << qsl_calls->size() - n
		 << " " << qsl_names[qsl_type]
		 << " callsigns from \"" << filename << "\"";
	LOG_VERBOSE("%s", info.str().c_str());

	qsl_open_ |= (1 << qsl_type);
	return true;
}

unsigned char qsl_is_open(void)
{
	return qsl_open_;
}

void qsl_close(void)
{
	delete qsl_calls;
	qsl_calls = 0;
	qsl_open_ = 0;
}

unsigned char qsl_lookup(const char* callsign)
{
	if (qsl_calls == 0)
		return 0;

	std::string str;
	str.resize(strlen(callsign));
	transform(callsign, callsign + str.length(), str.begin(), static_cast<int (*)(int)>(toupper));

	qsl_map_t::const_iterator i = qsl_calls->find(str);
	return i == qsl_calls->end() ? 0 : i->second;
}

void reload_cty_dat()
{
	dxcc_close();
	dxcc_open(std::string(progdefaults.cty_dat_pathname).append("cty.dat").c_str());
	qsl_close();
	qsl_open(std::string(progdefaults.cty_dat_pathname).append("lotw1.txt").c_str(), QSL_LOTW);
	if (!qsl_open(std::string(progdefaults.cty_dat_pathname).append("eqsl.txt").c_str(), QSL_EQSL))
		qsl_open(std::string(progdefaults.cty_dat_pathname).append("AGMemberList.txt").c_str(), QSL_EQSL);
}

void default_cty_dat_pathname()
{
	progdefaults.cty_dat_pathname = HomeDir;
	txt_cty_dat_pathname->value(progdefaults.cty_dat_pathname.c_str());
}

void select_cty_dat_pathname()
{
	std::string deffilename = progdefaults.cty_dat_pathname;
	const char *p = FSEL::select(_("Locate cty.dat folder"), _("cty.dat\t*"), deffilename.c_str());
	if (p) {
		std::string nupath = p;
		size_t ptr;
//crappy win32 again
		ptr = nupath.find("\\");
		while (ptr != std::string::npos) {
			nupath[ptr] = '/';
			ptr = nupath.find("\\");
		}
		size_t endslash = nupath.rfind("/");
		if ((endslash != std::string::npos) && (endslash != (nupath.length()-1)))
			nupath.erase(endslash + 1);
		progdefaults.cty_dat_pathname = nupath;
		progdefaults.changed = true;
		txt_cty_dat_pathname->value(nupath.c_str());
	}
}

