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

#include "dxcc.h"
#include "debug.h"

using namespace std;


dxcc::dxcc(const char* cn, int cq, int itu, const char* ct, float lat, float lon, float tz)
	: country(cn), cq_zone(cq), itu_zone(itu), latitude(lat), longitude(lon), gmt_offset(tz)
{
	if (*ct) {
		continent[0] = ct[0];
		continent[1] = ct[1];
	}
	continent[2] = '\0';
}

typedef map<string, dxcc*> dxcc_map;
static dxcc_map* cmap = 0;
static list<string>* cnames = 0;

static void add_prefix(string& prefix, dxcc* entry);

bool dxcc_open(const char* filename)
{
	dxcc_close();

	ifstream in(filename);
	if (!in) {
		LOG_WARN("Could not read contest country file \"%s\"", filename);
		return false;
	}

	cmap = new dxcc_map;
	cnames = new list<string>;

	dxcc* entry;
	string record;

	unsigned nrec = 0;
	while (getline(in, record, ';')) {
		istringstream is(record);
		entry = new dxcc;
		nrec++;

		// read country name
		cnames->resize(cnames->size() + 1);
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

	LOG_INFO("Loaded %zu prefixes for %u countries", cmap->size(), nrec);
	return true;
}

void dxcc_close(void)
{
	if (!cmap)
		return;
	delete cnames;
	cnames = 0;
	map<dxcc*, bool> rm;
	for (dxcc_map::iterator i = cmap->begin(); i != cmap->end(); ++i)
		if (rm.insert(make_pair(i->second, true)).second)
			delete i->second;
	delete cmap;
	cmap = 0;
}

const dxcc* dxcc_lookup(const char* callsign)
{
	if (!cmap || !callsign || !*callsign)
		return NULL;

	string sstr;
	sstr.resize(strlen(callsign) + 1);
	transform(callsign, callsign + sstr.length() - 1, sstr.begin() + 1, static_cast<int (*)(int)>(toupper));

	dxcc_map::const_iterator entry;

	// first look for a full callsign (prefixed with '=')
	sstr[0] = '=';
	if ((entry = cmap->find(sstr)) != cmap->end())
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
	static dxcc_map::iterator prev_entry = cmap->begin();

	string::size_type i = prefix.find_first_of("([<{");
	if (likely(i == string::npos)) {
		prev_entry = cmap->insert(prev_entry, make_pair(prefix, entry));
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
	prev_entry = cmap->insert(prev_entry, make_pair(prefix, entry));
}

typedef map<string, unsigned char> qsl_map;
static qsl_map* qsl_calls;
const char* qsl_names[] = { "LoTW", "eQSL" };

bool qsl_open(const char* filename, qsl_t qsl_type)
{
	ifstream in(filename);
	if (!in)
		return false;
	if (!qsl_calls)
		qsl_calls = new qsl_map;

	size_t n = qsl_calls->size();
	qsl_map::iterator prev_entry = qsl_calls->begin();
	string::size_type p;
	string s;
	s.reserve(32);
	while (getline(in, s)) {
		if ((p = s.rfind('\r')) != string::npos)
			s.erase(p);
		prev_entry = qsl_calls->insert(prev_entry, make_pair(s, 0));
		prev_entry->second |= (1 << qsl_type);
	}

	LOG_INFO("Added %zu %s callsigns from \"%s\"",
		 qsl_calls->size() - n, qsl_names[qsl_type], filename);

	return true;
}

void qsl_close(void)
{
	delete qsl_calls;
	qsl_calls = 0;
}

unsigned char qsl_lookup(const char* callsign)
{
	if (qsl_calls == 0)
		return 0;

	string str;
	str.resize(strlen(callsign));
	transform(callsign, callsign + str.length(), str.begin(), static_cast<int (*)(int)>(toupper));

	qsl_map::const_iterator i = qsl_calls->find(str);
	return i == qsl_calls->end() ? 0 : i->second;
}
