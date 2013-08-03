// ----------------------------------------------------------------------------
//      pskrep.cxx
//
// Copyright (C) 2008-2009
//              Stelios Bounanos, M0GLD
//
// This is a client for N1DQ's PSK Automatic Propagation Reporter
// (see http://pskreporter.info/).  Philip Gladstone, N1DQ, is
// thanked for his helpful explanation of the protocol.
//
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

#ifdef __MINGW32__
#  include "compat.h"
#endif

#if HAVE_SYS_UTSNAME_H
#  include <sys/utsname.h>
#endif

#include <sys/time.h>
#if HAVE_ARPA_INET_H
#  include <arpa/inet.h>
#endif

#include <stdint.h>
#include <cstring>
#include <cstdlib>

#include <string>
#include <deque>
#include <vector>
#include <algorithm>
#include <fstream>

#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 1)
#  define MAP_TYPE std::tr1::unordered_map
#define HASH_TYPE std::tr1::hash
#  include <tr1/unordered_map>
#else
// use the non-standard gnu hash_map on gcc <= 4.0.x,
// which has a broken tr1::unordered_map::operator=
#  define MAP_TYPE __gnu_cxx::hash_map
#define HASH_TYPE __gnu_cxx::hash
#  include <ext/hash_map>
namespace __gnu_cxx {
	// define the missing hash specialisation for std::string
	// using the 'const char*' hash function
	template<> struct hash<std::string> {
		size_t operator()(const std::string& s) const { return __stl_hash_string(s.c_str()); }
	};
}
#endif

#include <FL/Fl.H>

#include "socket.h"
#include "re.h"
#include "debug.h"
#include "util.h"
#include "trx.h"
#include "fl_digi.h"
#include "main.h"
#include "configuration.h"
#include "globals.h"
#include "spot.h"

#include "pskrep.h"

LOG_FILE_SOURCE(debug::LOG_SPOTTER);

// -------------------------------------------------------------------------------------------------

// Try to flush the report queue every SEND_INTERVAL seconds.
#define SEND_INTERVAL 300

// Ignore reports that are less than DUP_INTERVAL seconds older than
// a previously sent report for the same callsign and frequency band.
// Sent reports are also garbage-collected after DUP_INTERVAL seconds.
#define DUP_INTERVAL 1800

// The first TEMPLATE_THRESHOLD packets will contain the long templates;
// the next TEMPLATE_THRESHOLD packets will include the short templates
#define TEMPLATE_THRESHOLD 3
// Resend short templates every TEMPLATE_INTERVAL seconds
#define TEMPLATE_INTERVAL 1800

// Maximum send size
#define DGRAM_MAX (1500-14-24-8)

#define PSKREP_QUEUE_FILE "pskrqueue.txt"
#define PSKREP_ID_FILE "pskrkey.txt"

// -------------------------------------------------------------------------------------------------

using namespace std;

enum status_t { PSKR_STATUS_NEW, PSKR_STATUS_PENDING, PSKR_STATUS_SENT };
enum rtype_t { PSKREP_AUTO = 1, PSKREP_LOG = 2, PSKREP_MANUAL = 3 };

struct rcpt_report_t
{
	rcpt_report_t(trx_mode m = 0, long long f = 0, time_t t = 0,
		      rtype_t p = PSKREP_AUTO, string loc = "")
		: mode(m), freq(f), rtime(t), rtype(p),
		  status(PSKR_STATUS_NEW), locator(loc) { }

	trx_mode mode;
	long long freq;
	time_t rtime;
	rtype_t rtype;

	status_t status;

	string locator;
};

// A band_map_t holds a list of reception reports (for a particular callsign and band)
typedef deque<rcpt_report_t> band_map_t;
// A call_map_t holds reception reports for a particular callsign
typedef MAP_TYPE<band_t, band_map_t, HASH_TYPE<int> > call_map_t;
// A container of this type holds all reception reports, sorted by callsign and band
typedef MAP_TYPE<string, call_map_t> queue_t;

class pskrep_sender
{
public:
	pskrep_sender(const string& call, const string& loc, const string& ant,
		      const string& host_, const string& port_,
		      const string& long_id_, const string& short_id_);
	~pskrep_sender();

	bool append(const string& callsign, const band_map_t::value_type& r);
	bool send(void);

private:
	void write_station_info(void);
	void write_preamble(void);

	string recv_callsign, recv_locator, recv_antenna;
	string host, port;
	string long_id, short_id;

	static const unsigned char long_station_info_template[];
	static const unsigned char short_station_info_template[];
	static const unsigned char rcpt_record_template[];
	vector<unsigned char> long_station_info;
	vector<unsigned char> short_station_info;

	uint32_t identifier;
	uint32_t sequence_number;

	unsigned template_count;
	time_t last_template;

	Socket* send_socket;
	unsigned char* dgram;
	size_t dgram_size;
	size_t report_offset;

	void create_socket(void);
	pthread_t resolver_thread;
	static void* resolver(void* obj);

	static const char hexsym[];

	static size_t pad(size_t len, size_t mult);
};


class pskrep
{
public:
	pskrep(const string& call, const string& loc, const string& ant,
	       const string& host, const string& port,
	       const string& long_id, const string& short_id,
	       bool reg_auto, bool reg_log, bool reg_manual);
	~pskrep();

	static void recv(trx_mode mode, int afreq, const char* str, const regmatch_t* calls, size_t len, void* obj);
	static void log(const char* call, const char* loc, long long freq, trx_mode mode, time_t rtime, void* obj);
	static void manual(const char* call, const char* loc, long long freq, trx_mode mode, time_t rtime, void* obj);
	bool progress(void);
	unsigned count(void) { return new_count; }

	static fre_t locator_re;
private:

	void append(string call, const char* loc, long long freq, trx_mode mode, time_t rtime, rtype_t rtype);
	void gc(void);

	void load_queue(void);
	void save_queue(void);

	static bool not_sent(const band_map_t::value_type& r)
	{
		return r.status != PSKR_STATUS_SENT;
	}

	queue_t queue;
	pskrep_sender sender;
	unsigned new_count;
};

fre_t pskrep::locator_re("[a-r]{2}[0-9]{2}[a-x]{2}", REG_EXTENDED | REG_NOSUB | REG_ICASE);

#define SHORT_ID_SIZE 4
#define LONG_ID_SIZE 8

// -------------------------------------------------------------------------------------------------

static string error_string;

static bool pskrep_check(void)
{
	struct {
		const string* var;
		const char* msg;
	} check[] = {
		{ &progdefaults.myCall, "callsign" },
		{ &progdefaults.myLocator, "locator" },
		{ &progdefaults.myAntenna, "antenna info" },
	};
	for (size_t i = 0; i < sizeof(check)/sizeof(*check); i++) {
		if (check[i].var->empty()) {
			error_string.assign("Error: missing ").append(check[i].msg);
			return false;
		}
	}
	if (!pskrep::locator_re.match(progdefaults.myLocator.c_str())) {
		error_string = "Error: bad Maidenhead locator\ncheck Configure->Operator->Locator";
		return false;
	}

	return true;
}

const char* pskrep_error(void)
{
	return error_string.c_str();
}

static void pskrep_progress(void* obj)
{
	if (reinterpret_cast<pskrep*>(obj)->progress())
		Fl::add_timeout(SEND_INTERVAL, pskrep_progress, obj);
	else
		pskrep_stop();
}

static void pskrep_make_id(string& id, size_t len)
{
	id.resize(len);

	ifstream f("/dev/urandom");
	if (f) {
		for (size_t i = 0; i < len; i++)
			while ((id[i] = f.get()) != EOF && !isgraph(id[i]));
		f.close();
	}
	else {
		unsigned seed = time(NULL);
		if (!progdefaults.myCall.empty())
			seed ^= simple_hash_str((const unsigned char*)progdefaults.myCall.c_str());
		srand(seed);
		for (size_t i = 0; i < len; i++)
			while (!isgraph(id[i] = rand() % 0x7F));
	}
}

static pskrep* pskr = 0;

bool pskrep_start(void)
{
	if (pskr)
		return true;
	else if (!pskrep_check())
		return false;

	// get identifier
	string fname = TempDir;
	fname.append(PSKREP_ID_FILE);
	ifstream in(fname.c_str());
	string long_id, short_id;
	if (in)
		in >> long_id >> short_id;
	if (!in || in.eof()) {
		in.close();
		pskrep_make_id(long_id, LONG_ID_SIZE);
		pskrep_make_id(short_id, SHORT_ID_SIZE);

		ofstream out(fname.c_str());
		if (out)
			out << long_id << ' ' << short_id << '\n';
		else
			LOG_ERROR("Could not write identifiers (\"%s\", \"%s\") to %s",
				  long_id.c_str(), short_id.c_str(), fname.c_str());
	}

	pskr = new pskrep(progdefaults.myCall, progdefaults.myLocator, progdefaults.myAntenna,
			  progdefaults.pskrep_host, progdefaults.pskrep_port, long_id, short_id,
			  progdefaults.pskrep_auto, progdefaults.pskrep_log, true);
	Fl::add_timeout(SEND_INTERVAL, pskrep_progress, pskr);

	return true;
}

void pskrep_stop(void)
{
	Fl::remove_timeout(pskrep_progress, pskr);

	delete pskr;
	pskr = 0;
}

unsigned pskrep_count(void)
{
	return pskr ? pskr->count() : 0;
}

// -------------------------------------------------------------------------------------------------

pskrep::pskrep(const string& call, const string& loc, const string& ant,
	       const string& host, const string& port,
	       const string& long_id, const string& short_id,
	       bool reg_auto, bool reg_log, bool reg_manual)
	: sender(call, loc, ant, host, port, long_id, short_id), new_count(0)
{
	if (reg_auto)
		spot_register_recv(pskrep::recv, this, PSKREP_RE, REG_EXTENDED | REG_ICASE);
	if (reg_log)
		spot_register_log(pskrep::log, this);
	if (reg_manual)
		spot_register_manual(pskrep::manual, this);
	load_queue();
}

pskrep::~pskrep()
{
	spot_unregister_recv(pskrep::recv, this);
	spot_unregister_log(pskrep::log, this);
	spot_unregister_manual(pskrep::manual, this);
	save_queue();
}

// This function is called by spot_recv() when its buffer matches our PSKREP_RE
void pskrep::recv(trx_mode mode, int afreq, const char* str, const regmatch_t* calls, size_t len, void* obj)
{
	if (unlikely(calls[PSKREP_RE_INDEX].rm_so == -1 || calls[PSKREP_RE_INDEX].rm_eo == -1))
		return;

	string call(str + calls[PSKREP_RE_INDEX].rm_so, calls[PSKREP_RE_INDEX].rm_eo - calls[PSKREP_RE_INDEX].rm_so);
	long long freq = afreq;
	if (!wf->USB())
		freq = -freq;
	freq += wf->rfcarrier();
	LOG_DEBUG("Spotted \"%s\" in buffer \"%s\"", call.c_str(), str);

	reinterpret_cast<pskrep*>(obj)->append(call.c_str(), "", freq,
					       active_modem->get_mode(), time(NULL), PSKREP_AUTO);
}

// This function is called by spot_log()
void pskrep::log(const char* call, const char* loc, long long freq, trx_mode mode, time_t rtime, void* obj)
{
	reinterpret_cast<pskrep*>(obj)->append(call, loc, freq, mode, rtime, PSKREP_LOG);
}

// This function is called by spot_manual()
void pskrep::manual(const char* call, const char* loc, long long freq, trx_mode mode, time_t rtime, void* obj)
{
	reinterpret_cast<pskrep*>(obj)->append(call, loc, freq, mode, rtime, PSKREP_MANUAL);
}

void pskrep::append(string call, const char* loc, long long freq, trx_mode mode, time_t rtime, rtype_t rtype)
{
	if (unlikely(call.empty()))
		return;
	transform(call.begin(), call.end(), call.begin(), static_cast<int (*)(int)>(toupper));

	if (!progdefaults.pskrep_qrg)
		freq = 0LL;

	if (*loc && !locator_re.match(loc))
		loc = "";

	band_map_t& bandq = queue[call][band(freq)];
	if (bandq.empty() || rtime - bandq.back().rtime >= DUP_INTERVAL) { // add new
		bandq.push_back(rcpt_report_t(mode, freq, rtime, rtype, loc));
		LOG_VERBOSE("Added (call=\"%s\", loc=\"%s\", mode=\"%s\", freq=%d, time=%" PRIdMAX ", type=%u)",
			 call.c_str(), loc, mode_info[mode].adif_name, 
			 static_cast<int>(freq), (intmax_t)rtime, rtype);
		new_count++;
		save_queue();
	}
	else if (!bandq.empty()) {
		band_map_t::value_type& r = bandq.back();
		if (r.status != PSKR_STATUS_SENT && *loc && r.locator != loc) { // update last
			r.locator = loc;
			r.rtype = rtype;
			LOG_VERBOSE("Updated (call=\"%s\", loc=\"%s\", mode=\"%s\", freq=%d, time=%d, type=%u)",
				 call.c_str(), loc, mode_info[r.mode].adif_name, 
				 static_cast<int>(r.freq), 
				 static_cast<int>(r.rtime), rtype);
			save_queue();
		}
	}
}

// Handle queued reports
bool pskrep::progress(void)
{
	if (queue.empty())
		return true;

	unsigned nrep = 0;
	bool sender_full = false;
	for (queue_t::iterator i = queue.begin(); i != queue.end(); ++i) {
		for (call_map_t::iterator j = i->second.begin(); j != i->second.end(); ++j) {
			for (band_map_t::iterator k = j->second.begin(); k != j->second.end(); ++k) {
				switch (k->status) {
				case PSKR_STATUS_NEW:
					if ((sender_full = !sender.append(i->first, *k)))
						goto send_reports;
					k->status = PSKR_STATUS_PENDING;
					nrep++;
					break;
				case PSKR_STATUS_PENDING: // sent in last cycle
					k->status = PSKR_STATUS_SENT;
				default:
					break;
				}
			}
		}
	}

send_reports:
	LOG_VERBOSE("Found %u new report(s)", nrep);
	if (nrep) {
		if (!sender.send()) {
			LOG_ERROR("Sender failed, disabling pskreporter");
			return false;
		}
		return progress();
	}

	gc();
	save_queue();
	return true;
}

// Delete sent reports that are older than DUP_INTERVAL seconds
void pskrep::gc(void)
{
	time_t threshold = time(NULL) - DUP_INTERVAL;
	unsigned rm = 0;

	for (queue_t::iterator i = queue.begin(); i != queue.end() ; ) {
		for (call_map_t::iterator j = i->second.begin(); j != i->second.end() ; ) {
			band_map_t& b = j->second;
			band_map_t::iterator k = find_if(b.begin(), b.end(), not_sent);
			if (k != b.begin() && k == b.end())
				--k;
			rm += k - b.begin();
			k = b.erase(b.begin(), k);

			if (k != b.end() && k->status == PSKR_STATUS_SENT && k->rtime <= threshold) {
				b.erase(k);
				rm++;
			}

			if (b.empty())
				i->second.erase(j++);
			else
				++j;
		}
		if (i->second.empty())
			queue.erase(i++);
		else
			++i;
	}

	LOG_DEBUG("Removed %u sent report(s)", rm);
}

static ostream& operator<<(ostream& out, const rcpt_report_t& r);
static istream& operator>>(istream& in, rcpt_report_t& r);
static ostream& operator<<(ostream& out, const queue_t& q);
static istream& operator>>(istream& in, queue_t& q);

void pskrep::save_queue(void)
{
	string fname = TempDir;
	fname.append(PSKREP_QUEUE_FILE);
	ofstream out(fname.c_str());

	if (out)
		out << queue;
	else
		LOG_ERROR("Could not write %s", fname.c_str());
}

void pskrep::load_queue(void)
{
	string fname = TempDir;
	fname.append(PSKREP_QUEUE_FILE);
	ifstream in(fname.c_str());
	if (!in)
		return;

	in >> queue;
	// restore pending reports as new
	for (queue_t::iterator i = queue.begin(); i != queue.end(); ++i)
		for (call_map_t::iterator j = i->second.begin(); j != i->second.end(); ++j)
			for (band_map_t::iterator k = j->second.begin(); k != j->second.end(); ++k)
				if (k->status == PSKR_STATUS_PENDING)
					k->status = PSKR_STATUS_NEW;
}

// -------------------------------------------------------------------------------------------------

// Text fields must be <= 254 bytes
#define MAX_TEXT_SIZE 254
// Records must be padded to a multiple of 4
#define PAD 4

pskrep_sender::pskrep_sender(const string& call, const string& loc, const string& ant,
			     const string& host_, const string& port_,
			     const string& long_id_, const string& short_id_)
	: recv_callsign(call, 0, MAX_TEXT_SIZE), recv_locator(loc, 0, MAX_TEXT_SIZE),
	  recv_antenna(ant, 0, MAX_TEXT_SIZE),
	  host(host_, 0, MAX_TEXT_SIZE), port(port_, 0, MAX_TEXT_SIZE),
	  long_id(long_id_, 0, LONG_ID_SIZE), short_id(short_id_, 0, SHORT_ID_SIZE),
	  sequence_number(0), template_count(2 * TEMPLATE_THRESHOLD), last_template(0),
	  send_socket(0), dgram_size(0), report_offset(0)
{
	create_socket();
	dgram = new unsigned char[DGRAM_MAX];
	write_station_info();
}

pskrep_sender::~pskrep_sender()
{
	delete send_socket;
	delete [] dgram;
}

// fldigi uses 0x0219 as the long station info template id (bytes 4,5)
const unsigned char pskrep_sender::long_station_info_template[] = {
	0x00, 0x03, 0x00, 0x34, 0x02, 0x19, 0x00, 0x05, 0x00, 0x00,
	0x80, 0x02, 0xFF, 0xFF, 0x00, 0x00, 0x76, 0x8F, // receiverCallsign
	0x80, 0x04, 0xFF, 0xFF, 0x00, 0x00, 0x76, 0x8F, // receiverLocator
	0x80, 0x0C, 0x00, 0x08, 0x00, 0x00, 0x76, 0x8F, // persistentIdentifier
	0x80, 0x08, 0xFF, 0xFF, 0x00, 0x00, 0x76, 0x8F, // decoderSoftware
	0x80, 0x09, 0xFF, 0xFF, 0x00, 0x00, 0x76, 0x8F, // anntennaInformation
	0x00, 0x00
};

// fldigi uses 0x0218 as the short station info template id (bytes 4,5)
const unsigned char pskrep_sender::short_station_info_template[] = {
	0x00, 0x03, 0x00, 0x24, 0x02, 0x18, 0x00, 0x03, 0x00, 0x00,
	0x80, 0x02, 0xFF, 0xFF, 0x00, 0x00, 0x76, 0x8F, // receiverCallsign
	0x80, 0x04, 0xFF, 0xFF, 0x00, 0x00, 0x76, 0x8F, // receiverLocator
	0x80, 0x0C, 0x00, 0x08, 0x00, 0x00, 0x76, 0x8F, // persistentIdentifier
	0x00, 0x00
};

void pskrep_sender::write_station_info(void)
{
	char prog_info[MAX_TEXT_SIZE];
	size_t prog_len;

	prog_len = snprintf(prog_info, sizeof(prog_info), "%s", PACKAGE_TARNAME "-" PACKAGE_VERSION);
	prog_len = MIN(prog_len, sizeof(prog_info));
	struct utsname u;
	if (uname(&u) != -1) {
		prog_len += snprintf(prog_info+prog_len, sizeof(prog_info)-prog_len, "/%s-%s", u.sysname, u.machine);
		prog_len = MIN(prog_len, sizeof(prog_info));
	}

	size_t  call_len = recv_callsign.length(),
		loc_len = recv_locator.length(),
		ant_len = recv_antenna.length();

	size_t long_len, short_len;
	// Long station info length
	long_len =  4        + // 4-byte header
		1 + call_len + // 1-byte call length + call string length
		1 + loc_len  + // 1-byte loc length  + loc string length
		8            + // 8-byte identifier
		1 + prog_len + // 1-byte prog length + prog string length
		1 + ant_len;   // 1-byte ant length  + ant string length
	long_len = pad(long_len, PAD);
	// Short station info length
	short_len = 4        + // 4-byte header
		1 + call_len + // 1-byte call length + call string length
		1 + loc_len  + // 1-byte loc length  + loc string length
		8;             // 8-byte identifier
	short_len = pad(short_len, PAD);

	long_station_info.resize(long_len);
	short_station_info.resize(short_len);
	unsigned char* p;
	size_t npad;

	// Write the long station info
	p = &long_station_info[0];
	// header
	memcpy(p, long_station_info_template + 4, 2);               p += 2;
	*reinterpret_cast<uint16_t*>(p) = htons(long_len);          p += sizeof(uint16_t);
	// call
	*p++ = call_len; memcpy(p, recv_callsign.data(), call_len); p += call_len;
	// locator
	*p++ = loc_len;  memcpy(p, recv_locator.data(), loc_len);   p += loc_len;
	// identifier
	memcpy(p, long_id.data(), LONG_ID_SIZE);                    p += LONG_ID_SIZE;
	// program
	*p++ = prog_len; memcpy(p, prog_info, prog_len);            p += prog_len;
	// antenna
	*p++ = ant_len;  memcpy(p, recv_antenna.data(), ant_len);   p += ant_len;
	// pad
	npad = &long_station_info[0] + long_len - p;
	if (npad)
		memset(p, 0, npad);
	LOG_DEBUG("long_station_info=\"%s\"", str2hex(&long_station_info[0], long_len));

	// Write the short station info
	p = &short_station_info[0];
	// header
	memcpy(p, short_station_info_template + 4, 2);              p += 2;
	*reinterpret_cast<uint16_t*>(p) = htons(short_len);         p += sizeof(uint16_t);
	// call
	*p++ = call_len; memcpy(p, recv_callsign.data(), call_len); p += call_len;
	// locator
	*p++ = loc_len;  memcpy(p, recv_locator.data(), loc_len);   p += loc_len;
	// identifier
	memcpy(p, long_id.data(), LONG_ID_SIZE);                    p += LONG_ID_SIZE;
	// pad
	npad = &short_station_info[0] + short_len - p;
	if (npad)
		memset(p, 0, npad);
	LOG_DEBUG("short_station_info=\"%s\"", str2hex(&short_station_info[0], short_len));
}

// fldigi uses 0x022C as the reception record template id (bytes 4,5)
const unsigned char pskrep_sender::rcpt_record_template[] = {
	0x00, 0x02, 0x00, 0x34, 0x02, 0x2C, 0x00, 0x06,
	0x80, 0x01, 0xFF, 0xFF, 0x00, 0x00, 0x76, 0x8F, // senderCallsign
	0x00, 0x96, 0x00, 0x04,                         // flowStartSeconds
	0x80, 0x05, 0x00, 0x04, 0x00, 0x00, 0x76, 0x8F, // frequency
	0x80, 0x0A, 0xFF, 0xFF, 0x00, 0x00, 0x76, 0x8F, // mode (adif string)
	0x80, 0x03, 0xff, 0xff, 0x00, 0x00, 0x76, 0x8F, // senderLocator (if known)
	0x80, 0x0B, 0x00, 0x01, 0x00, 0x00, 0x76, 0x8F  // flags (informationSource)
};

void pskrep_sender::write_preamble(void)
{
	time_t now = time(NULL);
	unsigned char* p = dgram;

	// header
	*p++ = 0x00; *p++ = 0x0A;   /* length written later */    p += 2;
	/* time written later */                                  p += sizeof(uint32_t);
	*reinterpret_cast<uint32_t*>(p) = htonl(sequence_number); p += sizeof(uint32_t);
	memcpy(p, short_id.data(), SHORT_ID_SIZE);                p += SHORT_ID_SIZE;

	const unsigned char* station_info_template;
	size_t tlen;
	vector<unsigned char>* station_info;

	if (template_count == 0 && now - last_template >= TEMPLATE_INTERVAL)
		template_count = TEMPLATE_THRESHOLD;
	if (template_count > TEMPLATE_THRESHOLD) {
		station_info_template = long_station_info_template;
		tlen = sizeof(long_station_info_template);
		station_info = &long_station_info;
	}
	else if (template_count >= 0) {
		station_info_template = short_station_info_template;
		tlen = sizeof(short_station_info_template);
		station_info = &short_station_info;
	}
	if (template_count > 0) {
		memcpy(p, rcpt_record_template, sizeof(rcpt_record_template)); p += sizeof(rcpt_record_template);
		memcpy(p, station_info_template, tlen);                        p += tlen;
		template_count--;
		last_template = now;
	}

	// station info record
	memcpy(p, &(*station_info)[0], station_info->size());     p += station_info->size();

	report_offset = p - dgram;
	// write report record header
	memcpy(p, rcpt_record_template + 4, 2); p += 2; /* length written later */ p += sizeof(uint16_t);

	dgram_size = p - dgram;
}

bool pskrep_sender::append(const string& callsign, const band_map_t::value_type& r)
{
	if (dgram_size == 0)
		write_preamble();

	size_t call_len = callsign.length();
	call_len = MIN(MAX_TEXT_SIZE, call_len);

	const char* mode = mode_info[r.mode].adif_name;
	size_t mode_len = strlen(mode);
	mode_len = MIN(MAX_TEXT_SIZE, mode_len);

	size_t loc_len = MIN(MAX_TEXT_SIZE, r.locator.length());

	// call_len + call + time + freq + mode_len + mode + loc_len + loc + info
	size_t rlen = 1 + call_len + 4 + 4 + 1 + mode_len + 1 + loc_len + 1;

	if (pad(rlen, PAD) + dgram_size > DGRAM_MAX) // datagram full
		return false;


	LOG_INFO("Appending report (call=%s mode=%s freq=%d time=%d type=%u)",
		 callsign.c_str(), mode_info[r.mode].adif_name, 
		 static_cast<int>(r.freq), 
		 static_cast<int>(r.rtime), r.rtype);

	unsigned char* start = dgram + dgram_size;
	unsigned char* p = start;
	// call
	*p++ = call_len; memcpy(p, callsign.data(), call_len); p += call_len;
	// 4-byte reception time
	*reinterpret_cast<uint32_t*>(p) = htonl(r.rtime);      p += sizeof(uint32_t);
	// 4-byte freq
	*reinterpret_cast<uint32_t*>(p) = htonl(r.freq);       p += sizeof(uint32_t);
	// mode
	*p++ = mode_len; memcpy(p, mode, mode_len);            p += mode_len;
	// locator
	*p++ = loc_len; memcpy(p, r.locator.data(), loc_len);  p += loc_len;
	// info source
	*p++ = r.rtype;

	LOG_DEBUG("                 \"%s\"", str2hex(start, p - start));

	dgram_size += rlen;
	return true;
}

void* pskrep_sender::resolver(void* obj)
{
	pskrep_sender* s = reinterpret_cast<pskrep_sender*>(obj);
	try {
		s->send_socket = new Socket(Address(s->host.c_str(), s->port.c_str(), "udp"));
		s->send_socket->connect();
	}
	catch (const SocketException& e) {
		LOG_ERROR("Could not resolve %s: %s", s->host.c_str(), e.what());
	}

	return NULL;
}

void pskrep_sender::create_socket(void)
{
	if (pthread_create(&resolver_thread, NULL, resolver, this) != 0)
		LOG_PERROR("pthread_create");
}

bool pskrep_sender::send(void)
{
	if (!send_socket)
		return false;

	// empty dgram or no reports (shouldn't happen)
	if (dgram_size == 0 || dgram_size == report_offset + 4) {
		LOG_DEBUG("Not sending empty dgram: %" PRIuSZ " %" PRIuSZ "", dgram_size, report_offset);
		return false;
	}

	// Finish writing the report record:
	//   do we need padding?
	size_t npad = (dgram_size - report_offset) % PAD;
	if (npad) {
		npad = PAD - npad;
		memset(dgram + dgram_size, 0x0, npad);
		dgram_size += npad;
	}
	//   write length
	*reinterpret_cast<uint16_t*>(dgram + report_offset + 2) = htons(dgram_size - report_offset);

	// finish writing the datagram
	*reinterpret_cast<uint16_t*>(dgram + 2) = htons(dgram_size);
	*reinterpret_cast<uint32_t*>(dgram + 4) = htonl(time(NULL));

	bool ret;
	LOG_DEBUG("Sending datagram (%" PRIuSZ "): \"%s\"", dgram_size, str2hex(dgram, dgram_size));
	try {
		if ((size_t)send_socket->send(dgram, dgram_size) != dgram_size)
			throw SocketException("short write");
		ret = true;
	}
	catch (const SocketException& e) {
		LOG_ERROR("Could not send datagram to %s port %s: %s", host.c_str(), port.c_str(), e.what());
		ret = false;
	}

	// increment this regardless of any errors
	sequence_number++;
	dgram_size = 0;

	return ret;
}

// Pad len to a multiple of mult
size_t pskrep_sender::pad(size_t len, size_t mult)
{
	size_t r = len % mult;
	return r ? len + mult - r : len;
}

// -------------------------------------------------------------------------------------------------

static istream& operator>>(istream& in, rtype_t& t)
{
	int i;
	in >> i;
	t = static_cast<rtype_t>(i);
	return in;
}
static istream& operator>>(istream& in, status_t& t)
{
	int i;
	in >> i;
	t = static_cast<status_t>(i);
	return in;
}
static istream& operator>>(istream& in, rcpt_report_t& r)
{
	in >> r.mode >> r.freq  >> r.rtime >> r.rtype >> r.status >> r.locator;
	if (*r.locator.c_str() == '?') r.locator.clear();
	return in;
}

static ostream& operator<<(ostream& out, const rcpt_report_t& r)
{
	return out << r.mode << ' ' << r.freq << ' ' << r.rtime << ' '
		   << r.rtype << ' ' << r.status << ' ' << (r.locator.empty() ? "?" : r.locator);
}
static ostream& operator<<(ostream& out, const queue_t& q)
{
	for (queue_t::const_iterator i = q.begin(); i != q.end(); ++i)
		for (call_map_t::const_iterator j = i->second.begin(); j != i->second.end(); ++j)
			for (band_map_t::const_iterator k = j->second.begin(); k != j->second.end(); ++k)
				out << *k << "  " << j->first << "  " << i->first << '\n';

	return out;
}
static istream& operator>>(istream& in, queue_t& q)
{
	rcpt_report_t rep;
	int band;
	string call;

	while (in >> rep >> band >> call)
	 	q[call][static_cast<band_t>(band)].push_back(rep);

	return in;
}
