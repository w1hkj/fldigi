// ----------------------------------------------------------------------------
//      spot.cxx
//
// Copyright (C) 2008
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

#include <vector>
#include <map>

#include "trx.h"
#include "globals.h"
#include "re.h"
#include "fl_digi.h"
#include "debug.h"
#include "spot.h"

#include <iostream>
using namespace std;

#define SEARCHLEN 32
//#define DECBUFSIZE 8 * SEARCHLEN
#define DECBUFSIZE 4 * SEARCHLEN


using namespace std;

struct callback_t {
	void* data;

	spot_log_cb_t lcb;

	spot_log_cb_t mcb;

	spot_recv_cb_t rcb;
	fre_t* re;
};

static map<int, string> buffers;
typedef vector<callback_t> cblist_t;


static cblist_t cblist;

#define CALLSIGN_RE "("\
"[[:alnum:]]+/[[:alnum:]]?[[:alpha:]]+[[:digit:]]+[[:alnum:]/]+"\
")|("\
"[[:alnum:]]?[[:alpha:]]+[[:digit:]]+[[:alnum:]/]+"\
")"
#define CALLSIGN_REP CALLSIGN_RE "[^[:alnum:]]+$"

static fre_t re_call(CALLSIGN_REP, REG_EXTENDED);

void spot_recv(char c, int decoder, int afreq)
{
	static trx_mode last_mode = NUM_MODES + 1;

	if (c == '\n' ) c = ' ';
	if (c < ' ') return;

	switch (decoder) {
	case -1: // mode without multiple decoders
		decoder = active_modem->get_mode();
		if (last_mode > NUM_MODES)
			last_mode = decoder;
		else if (last_mode != decoder) {
			buffers.clear();
			last_mode = decoder;
		}
		break;
	default:
		if (last_mode > NUM_MODES)
			last_mode = active_modem->get_mode();
		else if (last_mode != active_modem->get_mode()) {
			buffers.clear();
			last_mode = active_modem->get_mode();
		}
		break;
	}
	if (afreq == 0)
		afreq = active_modem->get_freq();

	string& buf = buffers[decoder];
	buf.reserve(DECBUFSIZE);

	size_t n = buf.length();
	if (n == DECBUFSIZE)
		buf.erase(0, DECBUFSIZE - SEARCHLEN);

	buf += toupper(c);
	n = buf.length();

//#ifdef __CYGWIN__
// This code segment resolves regexec problems on Windows
// Windows regex compile does not recognize the \2 tag in the reg expression
// (see pskrep.cxx)

	string search = buf.substr(n> SEARCHLEN ? n - SEARCHLEN: 0);

	for (cblist_t::iterator cbl = cblist.begin(); cbl != cblist.end(); ++cbl) {
		if (cbl->rcb) {
			if (re_call.match(search.c_str())) {
				const vector<regmatch_t>& offset = re_call.suboff();
				if (!offset.empty()) {
					size_t pos = offset[0].rm_so;
					string call = search.substr(pos);
					size_t firstcall = search.find(call);
					if (firstcall != pos)
						if ( ( (pos = search.find("DE ")) != string::npos && pos < firstcall) ||
							 ( (pos = search.find("CQ ")) != string::npos && pos < firstcall) ||
							 ( (pos = search.find("QRZ ")) != string::npos && pos < firstcall) )
							cbl->rcb(afreq, search.c_str(), &offset[0], offset.size(), cbl->data);					
				}
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
	callback_t c = { ldata, lcb, 0, 0, 0 };
	cblist.push_back(c);
}
//
// A callback of type spot_log_cb_t is registered with a data argument.
// The callback is invoked every time the user manually spots a callsign.
//
void spot_register_manual(spot_log_cb_t mcb, void* mdata)
{
	callback_t c = { mdata, 0, mcb, 0, 0 };
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
	callback_t c = { rdata, 0, 0, rcb, new fre_t(re, reflags) };
	cblist.push_back(c);
	show_spot(true);
}

void spot_unregister_log(spot_log_cb_t lcb, void* ldata)
{
	for (cblist_t::reverse_iterator ri = cblist.rbegin(); ri != cblist.rend(); ++ri) {
		if (lcb == ri->lcb && ldata == ri->data) {
			cblist.erase((++ri).base());
			break;
		}
	}
}
void spot_unregister_manual(spot_log_cb_t mcb, void* mdata)
{
	for (cblist_t::reverse_iterator ri = cblist.rbegin(); ri != cblist.rend(); ++ri) {
		if (mcb == ri->mcb && mdata == ri->data) {
			cblist.erase((++ri).base());
			break;
		}
	}
}
void spot_unregister_recv(spot_recv_cb_t rcb, void* rdata)
{
	cblist_t::reverse_iterator ri;
	for (ri = cblist.rbegin(); ri != cblist.rend(); ++ri) {
		if (rcb == ri->rcb && rdata == ri->data) {
			cblist.erase((++ri).base());
			break;
		}
	}

	for (ri = cblist.rbegin(); ri != cblist.rend(); ++ri)
		if (ri->rcb) break;
	show_spot(ri != cblist.rend());
}
