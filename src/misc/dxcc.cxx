#include <config.h>

#include <cstring>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <list>
#include <map>
#include <tr1/unordered_map>
#include <algorithm>

#include "dxcc.h"
#include "debug.h"

using namespace std;
using tr1::unordered_map;


dxcc::dxcc(const char* cn, int cq, int itu, const char* ct, float lat, float lon, float tz)
	: country(cn), cq_zone(cq), itu_zone(itu), latitude(lat), longitude(lon), gmt_offset(tz)
{
	if (*ct) {
		continent[0] = ct[0];
		continent[1] = ct[1];
	}
	continent[2] = '\0';
}

typedef unordered_map<string, dxcc*> dxcc_map_t;
typedef vector<dxcc*> dxcc_list_t;
static dxcc_map_t* cmap = 0;
static dxcc_list_t* clist = 0;
static vector<string>* cnames = 0;

static void add_prefix(string& prefix, dxcc* entry);

bool dxcc_open(const char* filename)
{
	if (cmap)
		return true;

	ifstream in(filename);
	if (!in) {
		LOG_VERBOSE("Could not read contest country file \"%s\"", filename);
		return false;
	}

	cmap = new dxcc_map_t;
	cnames = new vector<string>;
	cnames->reserve(345); // approximate number of dxcc entities
	clist = new dxcc_list_t;
	clist->reserve(345);

	dxcc* entry;
	string record;

	unsigned nrec = 0;
	while (getline(in, record, ';')) {
		istringstream is(record);
		entry = new dxcc;
		nrec++;

		// read country name
		cnames->resize(cnames->size() + 1);
		clist->push_back(entry);
		getline(is, cnames->back(), ':');
		entry->country = cnames->back().c_str();
		// cq zone
		(is >> entry->cq_zone).ignore();
		// itu zone
		(is >> entry->itu_zone).ignore();
		// continent
		(is >> ws).get(entry->continent, 3).ignore();

		// latitude
		(is >> entry->latitude).ignore();
		// longitude
		(is >> entry->longitude).ignore();
		// gmt offset
		(is >> entry->gmt_offset).ignore(256, '\n');

		// prefixes and exceptions
		int c;
		string prefix;
		while ((c = is.peek()) == ' ' || c == '\r' || c == '\n') {
			is >> ws;

			while (getline(is, prefix, ',')) {
				add_prefix(prefix, entry);
				if ((c = is.peek()) == '\r' || c == '\n')
					break;
			}
		}

		in >> ws; // cr/lf after ';'
	}

	LOG_VERBOSE("Loaded %" PRIuSZ " prefixes for %u countries", cmap->size(), nrec);
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
	map<dxcc*, bool> rm;
	for (dxcc_map_t::iterator i = cmap->begin(); i != cmap->end(); ++i)
		if (rm.insert(make_pair(i->second, true)).second)
			delete i->second;
	delete cmap;
	cmap = 0;
	delete clist;
	clist = 0;
}

const vector<dxcc*>* dxcc_entity_list(void)
{
	return clist;
}

const dxcc* dxcc_lookup(const char* callsign)
{
	if (!cmap || !callsign || !*callsign)
		return NULL;

	string sstr;
	sstr.resize(strlen(callsign) + 1);
	transform(callsign, callsign + sstr.length() - 1, sstr.begin() + 1, static_cast<int (*)(int)>(toupper));

	// first look for a full callsign (prefixed with '=')
	sstr[0] = '=';
	dxcc_map_t::const_iterator entry = cmap->find(sstr);
	if (entry != cmap->end())
		return entry->second;

	// erase the '=' and do a longest prefix search
	sstr.erase(0, 1);
	size_t len = sstr.length();
	do {
		sstr.resize(len--);
		if ((entry = cmap->find(sstr)) != cmap->end())
			return entry->second;
	} while (len);

	return NULL;
}

static void add_prefix(string& prefix, dxcc* entry)
{
	string::size_type i = prefix.find_first_of("([<{");
	if (likely(i == string::npos)) {
		(*cmap)[prefix] = entry;
		return;
	}

	string::size_type j = i, first = i;
	do {
		entry = new struct dxcc(*entry);
		switch (prefix[i++]) { // increment i past opening bracket
		case '(':
			if ((j = prefix.find(')', i)) == string::npos) {
				delete entry;
				return;
			}
			prefix[j] = '\0';
			entry->cq_zone = atoi(prefix.data() + i);
			break;
		case '[':
			if ((j = prefix.find(']', i)) == string::npos) {
				delete entry;
				return;
			}
			prefix[j] = '\0';
			entry->itu_zone = atoi(prefix.data() + i);
			break;
		case '<':
			if ((j = prefix.find('/', i)) == string::npos) {
				delete entry;
				return;
			}
			prefix[j] = '\0';
			entry->latitude = atof(prefix.data() + i);
			if ((j = prefix.find('>', j)) == string::npos) {
				delete entry;
				return;
			}
			prefix[j] = '\0';
			entry->longitude = atof(prefix.data() + i);
			break;
		case '{':
			if ((j = prefix.find('}', i)) == string::npos) {
				delete entry;
				return;
			}
			memcpy(entry->continent, prefix.data() + i, 2);
			break;
		}
	} while ((i = prefix.find_first_of("([<{", j)) != string::npos);

	prefix.erase(first);
	(*cmap)[prefix] = entry;
}

typedef unordered_map<string, unsigned char> qsl_map_t;
static qsl_map_t* qsl_calls;
static unsigned char qsl_open_;
const char* qsl_names[] = { "LoTW", "eQSL" };

bool qsl_open(const char* filename, qsl_t qsl_type)
{
	ifstream in(filename);
	if (!in)
		return false;
	if (!qsl_calls)
		qsl_calls = new qsl_map_t;

	size_t n = qsl_calls->size();
	string::size_type p;
	string s;
	s.reserve(32);
	while (getline(in, s)) {
		if ((p = s.rfind('\r')) != string::npos)
			s.erase(p);
		(*qsl_calls)[s] |= (1 << qsl_type);
	}

	LOG_VERBOSE("Added %" PRIuSZ " %s callsigns from \"%s\"",
		    qsl_calls->size() - n, qsl_names[qsl_type], filename);

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

	string str;
	str.resize(strlen(callsign));
	transform(callsign, callsign + str.length(), str.begin(), static_cast<int (*)(int)>(toupper));

	qsl_map_t::const_iterator i = qsl_calls->find(str);
	return i == qsl_calls->end() ? 0 : i->second;
}
