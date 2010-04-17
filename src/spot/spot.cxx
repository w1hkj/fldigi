// ----------------------------------------------------------------------------
//      spot.cxx
//
// Copyright (C) 2008-2009
//              Stelios Bounanos, M0GLD
//
// This file is part of fldigi.
//
// fldigi is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#include <config.h>

#include <list>
#include <tr1/unordered_map>
#include <functional>

#include "trx.h"
#include "globals.h"
#include "re.h"
#include "fl_digi.h"
#include "debug.h"
#include "spot.h"

// the number of characters that we match our REs against
#define SEARCHLEN 32
#define DECBUFSIZE 8 * SEARCHLEN


using namespace std;

struct callback_t
{
	void* data;
	spot_log_cb_t lcb;
	spot_log_cb_t mcb;
	spot_recv_cb_t rcb;
};
typedef list<callback_t> cblist_t;


struct fre_hash : std::unary_function<const fre_t*, size_t>
{
	size_t operator()(const fre_t* r) const { return r->hash(); }
};
struct fre_comp : std::unary_function<const fre_t*, bool>
{
	size_t operator()(const fre_t* l, const fre_t* r) const { return *l == *r; }
};

typedef list<callback_t*> callback_p_list_t;
typedef tr1::unordered_map<fre_t*, callback_p_list_t, fre_hash, fre_comp> rcblist_t;

static tr1::unordered_map<int, string> buffers;
static cblist_t cblist;
static rcblist_t rcblist;

void spot_recv(char c, int decoder, int afreq, int md)
{
	static trx_mode last_mode = NUM_MODES + 1;

	if (decoder == -1) { // mode without multiple decoders
		decoder = active_modem->get_mode();
		if (last_mode != active_modem->get_mode()) {
			buffers.clear();
			last_mode = active_modem->get_mode();
		}
	} else if (last_mode != md) {
		buffers.clear();
		last_mode = md;
	}
	if (afreq == 0)
		afreq = active_modem->get_freq();

	string& buf = buffers[decoder];
	if (unlikely(buf.capacity() < DECBUFSIZE))
		buf.reserve(DECBUFSIZE);

	buf += c;
	string::size_type n = buf.length();
	if (n == DECBUFSIZE)
		buf.erase(0, DECBUFSIZE - SEARCHLEN);
	const char* search = buf.c_str() + (n > SEARCHLEN ? n - SEARCHLEN : 0);

	for (rcblist_t::iterator i = rcblist.begin(); i != rcblist.end(); ++i) {
		if (unlikely(i->first->match(search))) {
			const vector<regmatch_t>& m = i->first->suboff();
			for (list<callback_t*>::iterator j = i->second.begin();
			     j != i->second.end() && (*j)->rcb; ++j) {
				if (m.empty())
					(*j)->rcb(last_mode, afreq, search, NULL, 0, (*j)->data);
				else
					(*j)->rcb(last_mode, afreq, search, &m[0], m.size(), (*j)->data);
			}
		}
	}
}

static void get_log_details(long long& freq, trx_mode& mode, time_t& rtime)
{
	if (mode == NUM_MODES)
		mode = active_modem->get_mode();
	if (mode >= MODE_WWV)
		return;

	if (freq == 0LL)
		freq = active_modem->get_freq();
	if (!wf->USB())
		freq = -freq;
	freq += wf->rfcarrier();

	if (rtime == -1L)
		rtime = time(NULL);
}

void spot_log(const char* callsign, const char* locator, long long freq, trx_mode mode, time_t rtime)
{
	get_log_details(freq, mode, rtime);
	for (cblist_t::const_iterator i = cblist.begin(); i != cblist.end(); ++i)
		if (i->lcb)
			i->lcb(callsign, locator, freq, mode, rtime, i->data);
}

void spot_manual(const char* callsign, const char* locator, long long freq, trx_mode mode, time_t rtime)
{
	get_log_details(freq, mode, rtime);
	for (cblist_t::const_iterator i = cblist.begin(); i != cblist.end(); ++i)
		if (i->mcb)
			i->mcb(callsign, locator, freq, mode, rtime, i->data);
}

//
// A callback of type spot_log_cb_t is registered with a data argument.
// The callback is invoked every time a QSO is logged.
//
void spot_register_log(spot_log_cb_t lcb, void* ldata)
{
	callback_t c = { ldata, lcb, 0, 0 };
	cblist.push_back(c);
}
//
// A callback of type spot_log_cb_t is registered with a data argument.
// The callback is invoked every time the user manually spots a callsign.
//
void spot_register_manual(spot_log_cb_t mcb, void* mdata)
{
	callback_t c = { mdata, 0, mcb, 0 };
	cblist.push_back(c);
}

//
// A callback of type spot_recv_cb_t is registered with a regular
// expression (RE, RE_flags).  If the RE matches the spotter's search
// buffer, the callback is invoked with offsets into the search buffer
// indicating substring matches, if the RE defines any, and with its
// data argument.  The offset format is described in regexec(3).  The
// buffer and offsets are only valid during that particular invocation.
// Clients should use anchoring to avoid repeated calls.
//
void spot_register_recv(spot_recv_cb_t rcb, void* rdata, const char* re, int reflags)
{
	callback_t c = { rdata, 0, 0, rcb };
	cblist.push_back(c);

	fre_t* fre = new fre_t(re, reflags);
	rcblist_t::iterator i = rcblist.find(fre);
	if (i != rcblist.end()) {
		i->second.push_back(&cblist.back());
		delete fre;
	}
	else
		rcblist[fre].push_back(&cblist.back());
	show_spot(true);
}

void spot_unregister_log(spot_log_cb_t lcb, const void* ldata)
{
	for (cblist_t::iterator i = cblist.begin(); i != cblist.end(); ++i) {
		if (lcb == i->lcb && ldata == i->data) {
			cblist.erase(i);
			break;
		}
	}
}
void spot_unregister_manual(spot_log_cb_t mcb, const void* mdata)
{
	for (cblist_t::iterator i = cblist.begin(); i != cblist.end(); ++i) {
		if (mcb == i->mcb && mdata == i->data) {
			cblist.erase(i);
			break;
		}
	}
}
void spot_unregister_recv(spot_recv_cb_t rcb, const void* rdata)
{
	cblist_t::iterator i;
	callback_t* p = 0;
	for (i = cblist.begin(); i != cblist.end(); ++i) {
		if (rcb == i->rcb && rdata == i->data) {
			p = &*i;
			cblist.erase(i);
			break;
		}
	}
	if (!p)
		return;

	// remove pointer from rcblist
	for (rcblist_t::iterator j = rcblist.begin(); j != rcblist.end(); ++j) {
		for (list<callback_t*>::iterator k = j->second.begin(); k != j->second.end(); ++k) {
			if (*k == p) {
				j->second.erase(k);
				if (j->second.empty()) {
					delete j->first;
					rcblist.erase(j);
				}
				goto out;
			}
		}
	}

out:
	for (i = cblist.begin(); i != cblist.end(); ++i)
		if (i->rcb) break;
	show_spot(i != cblist.end());
}
