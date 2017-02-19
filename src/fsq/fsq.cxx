// ----------------------------------------------------------------------------
// fsq.cxx  --  fsq modem
//
// Copyright (C) 2015
//		Dave Freese, W1HKJ
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

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <libgen.h>

#include <FL/filename.H>
#include "progress.h"
#include "fsq.h"
#include "complex.h"
#include "fl_digi.h"
#include "misc.h"
#include "fileselect.h"
#include "threads.h"
#include "debug.h"
#include "re.h"

#include "configuration.h"
#include "qrunner.h"
#include "fl_digi.h"
#include "status.h"
#include "main.h"
#include "icons.h"
#include "ascii.h"
#include "timeops.h"

using namespace std;

#include "fsq_varicode.cxx"

void  clear_xmt_arrays();

#define SQLFILT_SIZE 16

#define NIT std::string::npos
#define txcenterfreq  1500.0

int fsq::symlen = 4096; // nominal symbol length; 3 baud

static const char *FSQBOL = " \n";
static const char *FSQEOL = "\n ";
static const char *FSQEOT = "  \b  ";
static const char *fsq_lf = "\n";
static const char *fsq_bot = "<bot>";
static const char *fsq_eol = "<eol>";
static const char *fsq_eot = "<eot>";

static std::string fsq_string;
static std::string fsq_delayed_string;

#include "fsq-pic.cxx"

// nibbles table used for fast conversion from tone difference to symbol

static std::string sz2utf8(std::string s)
{
	char dest[4*s.length() + 1]; // if every char were utf8
	int numbytes = fl_utf8froma(dest, sizeof(dest) - 1, s.c_str(), s.length());
	if (numbytes > 0) return dest;
	return s;
}

//static int nibbles[199];
void fsq::init_nibbles()
{
	int nibble = 0;
	for (int i = 0; i < 199; i++) {
		nibble = floor(0.5 + (i - 99)/3.0);
		// allow for wrap-around (33 tones for 32 tone differences)
		if (nibble < 0) nibble += 33;
		if (nibble > 32) nibble -= 33;
		// adjust for +1 symbol at the transmitter
		nibble--;
		nibbles[i] = nibble;
	}
}

// note:
// display_fsq_rx_text and
// display_fsq_mon_text
// use a REQ(...) access to the UI
// it is not necessary to indirectly call either write_rx_mon_char or
// write_mon_tx_char using the REQ access mechanism

void write_rx_mon_char(int ch)
{
	int ach = ch & 0xFF;
	if (!progdefaults.fsq_directed) {
		display_fsq_rx_text(fsq_ascii[ach], FTextBase::FSQ_UND) ;
		if (ach == '\n')
			display_fsq_rx_text(fsq_lf, FTextBase::FSQ_UND);
	}
	display_fsq_mon_text(fsq_ascii[ach], FTextBase::RECV);
	if (ach == '\n')
		display_fsq_mon_text(fsq_lf, FTextBase::RECV);
}

void write_mon_tx_char(int ch)
{
	int ach = ch & 0xFF;

	display_fsq_mon_text(fsq_ascii[ach], FTextBase::FSQ_TX);
	if (ach == '\n')
		display_fsq_mon_text(fsq_lf, FTextBase::FSQ_TX);
}

void printit(double speed, int bandwidth, int symlen, int bksize, int peak_hits, int tone)
{
	std::ostringstream it;
	it << "\nSpeed.......... " << speed     << "\nBandwidth...... " << bandwidth;
	it << "\nSymbol length.. " << symlen    << "\nBlock size..... " << bksize;
	it << "\nMinimum Hits... " << peak_hits << "\nBasetone....... " << tone << "\n";
	display_fsq_mon_text(it.str(), FTextBase::ALTR);
}

fsq::fsq(trx_mode md) : modem()
{
	modem::set_freq(1500); // default Rx/Tx center frequency

	mode = md;
	samplerate = SR;
	fft = new g_fft<double>(FFTSIZE);
	snfilt = new Cmovavg(SQLFILT_SIZE);

	noisefilt = new Cmovavg(32);
	sigfilt = new Cmovavg(8);

//	baudfilt = new Cmovavg(3);
	movavg_size = progdefaults.fsq_movavg;
	if (movavg_size < 1) movavg_size = progdefaults.fsq_movavg = 1;
	if (movavg_size > 4) movavg_size = progdefaults.fsq_movavg = 4;
	for (int i = 0; i < NUMBINS; i++) binfilt[i] = new Cmovavg(movavg_size);
	spacing = 3;
	txphase = 0;
	basetone = 333;

	picfilter = new C_FIR_filter();
	picfilter->init_lowpass(257, 1, 500.0 / samplerate);
	phase = 0;
	phidiff = 2.0 * M_PI * frequency / samplerate;
	prevz = cmplx(0,0);

	bkptr = 0;
	peak_counter = 0;
	peak = last_peak = 0;
	max = 0;
	curr_nibble = prev_nibble = 0;
	s2n = 0;
	ch_sqlch_open = false;
	memset(rx_stream, 0, sizeof(rx_stream));
	rx_text.clear();

	for (int i = 0; i < BLOCK_SIZE; i++)
		a_blackman[i] = blackman(1.0 * i / BLOCK_SIZE);

	heard_log_fname = progdefaults.fsq_heard_log;
	std::string sheard = TempDir;
	sheard.append(heard_log_fname);
	heard_log.open(sheard.c_str(), ios::app);

	audit_log_fname = progdefaults.fsq_audit_log;
	std::string saudit = TempDir;
	saudit.append(audit_log_fname);
	audit_log.open(saudit.c_str(), ios::app);

	audit_log << "\n==================================================\n";
	audit_log << "Audit log: " << zdate() << ", " << ztime() << "\n";
	audit_log << "==================================================\n";

	fsq_tx_image = false;

	init_nibbles();

	sounder_interval = progdefaults.fsq_sounder;
	start_sounder(sounder_interval);

	start_aging();

	show_mode();
}

fsq::~fsq()
{
	delete fft;
	delete snfilt;

	delete sigfilt;
	delete noisefilt;

	for (int i = 0; i < NUMBINS; i++)
		delete binfilt[i];
//	delete baudfilt;
	delete picfilter;
	REQ(close_fsqMonitor);
	stop_sounder();
	stop_aging();
	heard_log.close();
	audit_log.close();
};

void  fsq::tx_init(SoundBase *sc)
{
	scard = sc;
	tone = prevtone = 0;
	txphase = 0;
	send_bot = true;
	mycall = progdefaults.myCall;
	if (progdefaults.fsq_lowercase)
		for (size_t n = 0; n < mycall.length(); n++) mycall[n] = tolower(mycall[n]);
	videoText();
}

void  fsq::rx_init()
{
	set_freq(frequency);
	bandwidth = 33 * spacing * samplerate / FSQ_SYMLEN;
	bkptr = 0;
	peak_counter = 0;
	peak = last_peak = 0;
	max = 0;
	curr_nibble = prev_nibble = 0;
	s2n = 0;
	ch_sqlch_open = false;
	memset(rx_stream, 0, sizeof(rx_stream));

	rx_text.clear();
	for (int i = 0; i < NUMBINS; i++) {
		tones[i] = 0.0;
		binfilt[i]->reset();
	}

	pixel = 0;
	amplitude = 0;
	phase = 0;
	prevz = cmplx(0,0);
	image_counter = 0;
	RXspp = 10; // 10 samples per pixel
	state = TEXT;
}

void fsq::init()
{
	modem::init();
	rx_init();
}

void fsq::set_freq(double f)
{
	frequency = f;
	modem::set_freq(frequency);
	basetone = ceil(1.0*(frequency - bandwidth / 2) * FSQ_SYMLEN / samplerate);
	tx_basetone = ceil((get_txfreq() - bandwidth / 2) * FSQ_SYMLEN / samplerate );
	int incr = basetone % spacing;
	basetone -= incr;
	tx_basetone -= incr;
}

void fsq::show_mode()
{
	if (speed == 2.0)
		put_MODEstatus("FSQ-2");
	else if (speed == 3.0)
		put_MODEstatus("FSQ-3");
	else if (speed == 4.5)
		put_MODEstatus("FSQ-4.5");
	else
		put_MODEstatus("FSQ-6");
}

void fsq::adjust_for_speed()
{
	speed = progdefaults.fsqbaud;

	if (speed == 2.0) {
		symlen = 6144;
	} else if (speed == 3.0) {
		symlen = 4096;
	} else if (speed == 4.5) {
		symlen = 3072;
	} else { // speed == 6
		symlen = 2048;
	}
	show_mode();
}

void fsq::restart()
{
	set_freq(frequency);

	peak_hits = progdefaults.fsqhits;
	adjust_for_speed();

	mycall = progdefaults.myCall;
	if (progdefaults.fsq_lowercase)
		for (size_t n = 0; n < mycall.length(); n++) mycall[n] = tolower(mycall[n]);

	movavg_size = progdefaults.fsq_movavg;
	if (movavg_size < 1) movavg_size = progdefaults.fsq_movavg = 1;
	if (movavg_size > 4) movavg_size = progdefaults.fsq_movavg = 4;

	for (int i = 0; i < NUMBINS; i++) binfilt[i]->setLength(movavg_size);

	printit(speed, bandwidth, symlen, SHIFT_SIZE, peak_hits, basetone);

}

bool fsq::valid_char(int ch)
{
	if ( ch ==  10 || ch == 163 || ch == 176 ||
		ch == 177 || ch == 215 || ch == 247 ||
		(ch > 31 && ch < 128))
		return true;
	return false;
}

//=====================================================================
// receive processing
//=====================================================================

bool fsq::fsq_squelch_open()
{
	return ch_sqlch_open || metric >= progStatus.sldrSquelchValue;
}

static string triggers = " !#$%&'()*+,-.;<=>?@[\\]^_{|}~";
static string allcall = "allcall";
static string cqcqcq = "cqcqcq";

static fre_t call("([[:alnum:]]?[[:alpha:]/]+[[:digit:]]+[[:alnum:]/]+)", REG_EXTENDED);

// test for valid callsign
// returns:
// 0 - not a callsign
// 1 - mycall
// 2 - allcall
// 4 - cqcqcq
// 8 - any other valid call

int fsq::valid_callsign(std::string s)
{
	if (s.length() < 3) return 0;
	if (s.length() > 20) return 0;

	if (s == allcall) return 2;
	if (s == cqcqcq) return 4;
	if (s == mycall) return 1;
	if (s.find("Heard") != string::npos) return 0;

	static char sz[21];
	memset(sz, 0, 21);
	strcpy(sz, s.c_str());
	bool matches = call.match(sz);
	return (matches ? 8 : 0);
}

void fsq::parse_rx_text()
{
	char ztbuf[20];
	struct timeval tv;
	gettimeofday(&tv, NULL);
	struct tm tm;
	time_t t_temp;
	t_temp=(time_t)tv.tv_sec;
	gmtime_r(&t_temp, &tm);
	strftime(ztbuf, sizeof(ztbuf), "%Y%m%d,%H%M%S", &tm);

	toprint.clear();

	if (rx_text.empty()) return;
	if (rx_text.length() > 65536) {
		rx_text.clear();
		return;
	}

	state = TEXT;
	size_t p = rx_text.find(':');
	if (p == 0) {
		rx_text.erase(0,1);
		return;
	}
	if (p == std::string::npos ||
		rx_text.length() < p + 2) {
		return;
	}

	std::string rxcrc = rx_text.substr(p+1,2);

	station_calling.clear();

	int max = p+1;
	if (max > 20) max = 20;
	std::string substr;
	for (int i = 1; i < max; i++) {
		if (rx_text[p-i] <= ' ' || rx_text[p-i] > 'z') {
			rx_text.erase(0, p+1);
			return;
		}
		substr = rx_text.substr(p-i, i);
		if ((crc.sval(substr) == rxcrc) && valid_callsign(substr)) {
			station_calling = substr;
			break;
		}
	}

	if (station_calling == mycall) { // do not display any of own rx stream
		LOG_ERROR("Station calling is mycall: %s", station_calling.c_str());
		rx_text.erase(0, p+3);
		return;
	}
	if (!station_calling.empty()) {
		REQ(add_to_heard_list, station_calling, szestimate);
		std::string sheard = ztbuf;
		sheard.append(",").append(station_calling);
		sheard.append(",").append(szestimate).append("\n");
		heard_log << sheard;
		heard_log.flush();
	}

// remove station_calling, colon and checksum
	rx_text.erase(0, p+3);

// extract all directed callsigns
// look for 'allcall', 'cqcqcq' or mycall

	bool all = false;
	bool directed = false;

// test next word in string
	size_t tr_pos = 0;
	char tr = rx_text[tr_pos];
	size_t trigger = triggers.find(tr);

// strip any leading spaces before either text or first directed callsign

	while (rx_text.length() > 1 &&
		triggers.find(rx_text[0]) != std::string::npos)
		rx_text.erase(0,1);

// find first word
	while ( tr_pos < rx_text.length()
			&& ((trigger = triggers.find(rx_text[tr_pos])) == std::string::npos) ) {
		tr_pos++;
	}

	while (trigger != std::string::npos && tr_pos < rx_text.length()) {
		int word_is = valid_callsign(rx_text.substr(0, tr_pos));

		if (word_is == 0) {
			rx_text.insert(0," ");
			break; // not a callsign
		}
		if (word_is == 1) {
			directed = true; // mycall
		}
		// test for cqcqcq and allcall
		else if (word_is != 8)
			all = true;

		rx_text.erase(0, tr_pos);

		while (rx_text.length() > 1 &&
			(rx_text[0] == ' ' && rx_text[1] == ' '))
			rx_text.erase(0,1);

		if (rx_text[0] != ' ') break;
		rx_text.erase(0, 1);

		tr_pos = 0;
		tr = rx_text[tr_pos];
		trigger = triggers.find(tr);
		while ( tr_pos < rx_text.length() && (trigger == std::string::npos) ) {
			tr_pos++;
			tr = rx_text[tr_pos];
			trigger = triggers.find(tr);
		}
	}

	if ( (all == false) && (directed == false)) {
		rx_text.clear();
		return;
	}

// remove eot if present
	if (rx_text.length() > 3) rx_text.erase(rx_text.length() - 3);

	toprint.assign(station_calling).append(":");

// test for trigger
	tr = rx_text[0];
	trigger = triggers.find(tr);

	if (trigger == NIT) {
		tr = ' '; // force to be text line
		rx_text.insert(0, " ");
	}

// if asleep suppress all but the * trigger

	if (btn_SELCAL->value() == 0) {
		if (tr == '*') parse_star();
		rx_text.clear();
		return;
	}

// now process own call triggers
	if (directed) {
		switch (tr) {
			case ' ': parse_space(false);   break;
			case '?': parse_qmark();   break;
			case '*': parse_star();    break;
			case '+': parse_plus();    break;
			case '-': break;//parse_minus();   break;
			case ';': parse_relay();    break;
			case '!': parse_repeat();    break;
			case '~': parse_delayed_repeat();   break;
			case '#': parse_pound();   break;
			case '$': parse_dollar();  break;
			case '@': parse_at();      break;
			case '&': parse_amp();     break;
			case '^': parse_carat();   break;
			case '%': parse_pcnt();    break;
			case '|': parse_vline();   break;
			case '>': parse_greater(); break;
			case '<': parse_less();    break;
			case '[': parse_relayed(); break;
		}
	}

// if allcall; only respond to the ' ', '*', '#', '%', and '[' triggers
	else {
		switch (tr) {
			case ' ': parse_space(true);   break;
			case '*': parse_star();    break;
			case '#': parse_pound();   break;
			case '%': parse_pcnt();    break;
			case '[': parse_relayed(); break;
		}
	}

	rx_text.clear();
}

void fsq::parse_space(bool all)
{
	display_fsq_rx_text(toprint.append(rx_text).append("\n"), FTextBase::FSQ_DIR);
}

void fsq::parse_qmark(std::string relay)
{
	std::string response = station_calling;
	if (!relay.empty()) response.append(";").append(relay);
	response.append(" snr=").append(szestimate);
	reply(response);
	display_fsq_rx_text(toprint.append(rx_text).append("\n"), FTextBase::FSQ_DIR);
}

void fsq::parse_dollar(std::string relay)
{
	std::string response = station_calling;
	if (!relay.empty()) response.append(";").append(relay);
	response.append(" Heard:\n");
	response.append(heard_list());
	reply(response);
	display_fsq_rx_text(toprint.append(rx_text).append("\n"), FTextBase::FSQ_DIR);
}

void fsq::parse_star()
{
	REQ(enableSELCAL);
	reply(std::string(station_calling).append(" ack"));
}

// immediate repeat of msg
void fsq::parse_repeat()
{
	std::string response;
	rx_text.erase(0, 1);
	if (rx_text[0] != ' ') rx_text.insert(0, " ");
	response.assign(" ");
	response.append(rx_text);
	reply(response);
	display_fsq_rx_text(toprint.append(rx_text).append("\n"), FTextBase::FSQ_DIR);
}

// delayed repeat of msg
void fsq::parse_delayed_repeat()
{
	std::string response;
	rx_text.erase(0, 1);
	if (rx_text[0] != ' ') rx_text.insert(0, " ");
	response.assign(" ");
	response.append(rx_text);
	delayed_reply(response, 15);
	display_fsq_rx_text(toprint.append(rx_text).append("\n"), FTextBase::FSQ_DIR);
}

// extended relay of msg
// k2a sees : k1a:; k3a hi
// k2a sends: k2a:22k3a[k1a] hi
void fsq::parse_relay()
{
	std::string send_txt = rx_text;
	send_txt.erase(0,1); // remove ';'
	if (send_txt.empty()) return;
	while (send_txt[0] == ' ' && !send_txt.empty())
		send_txt.erase(0,1); // remove leading spaces
	// find trigger
	size_t p = 0;
	while ((triggers.find(send_txt[p]) == NIT) && p < send_txt.length()) p++;
	std::string response = string("[").append(station_calling).append("]");
	send_txt.insert(p, response);
	if ((p = send_txt.find('^')) != NIT) send_txt.insert(p, "^");
	display_fsq_rx_text(toprint.append(rx_text).append("\n"), FTextBase::FSQ_DIR);
	reply(send_txt);
}

// k3a sees : k2a: [k1a]@
// or       : k2a:[k1a]@
void fsq::parse_relayed()
{
	std::string relayed = "";

	size_t p1 = rx_text.find('[');
	if (p1 == NIT) {
		LOG_ERROR("%s", "missing open bracket");
		return;
	}
	rx_text.erase(0,p1 + 1);
	if (rx_text.empty()) return;

	size_t p2 = rx_text.find(']');
	if (p2 == NIT) {
		LOG_ERROR("%s", "missing end bracket");
		return;
	}
	relayed = rx_text.substr(0, p2);
	rx_text.erase(0, p2 + 1);
	if (rx_text.empty()) return;

	if (triggers.find(rx_text[0]) == NIT) {
		LOG_ERROR("%s", "invalid relay trigger");
		return;
	}
// execute trigger
	switch (rx_text[0]) {
		case ' ' : {
			std::string response = station_calling;
			response.append(";").append(relayed).append(rx_text);
			display_fsq_rx_text(toprint.append(response).append("\n"), FTextBase::FSQ_DIR);
		} break;
		case '$' : parse_dollar(relayed); break;
		case '&' : parse_amp(relayed); break;
		case '?' : parse_qmark(relayed); break;
		case '@' : parse_at(relayed); break;
		case '^' : parse_carat(relayed); break;
		case '|' : parse_vline(relayed); break;
		case '#' : parse_pound(relayed); break;
		case '<' : parse_less(relayed); break;
		case '>' : parse_greater(relayed); break;
		default : break;
	}
}

// rx_text[0] will be '#'

void fsq::parse_pound(std::string relay)
{
	size_t p1 = NIT, p2 = NIT;
	std::string fname = "";
	p1 = rx_text.find('[');
	if (p1 != NIT) {
		p2 = rx_text.find(']', p1);
		if (p2 != NIT) {
			fname = rx_text.substr(p1 + 1, p2 - p1 - 1);
			fname = fl_filename_name(fname.c_str());
		} else p2 = 0;
	} else p2 = 0;
	if (fname.empty()) {
		if (!relay.empty()) fname = relay;
		else fname = station_calling;
		fname.append(".txt");
	}
	if (fname.find(".txt") == std::string::npos) fname.append(".txt");
	if (rx_text[rx_text.length() -1] != '\n') rx_text.append("\n");

	std::ofstream rxfile;
	fname.insert(0, TempDir);
	if (progdefaults.always_append) {
		rxfile.open(fname.c_str(), ios::app);
	} else {
		rxfile.open(fname.c_str(), ios::out);
	}
	if (!rxfile) return;
	if (progdefaults.add_fsq_msg_dt) {
		rxfile << "Received: " << zdate() << ", " << ztime() << "\n";
		rxfile << rx_text.substr(p2+1) << "\n";
	} else
		rxfile << rx_text.substr(p2+1);

	rxfile.close();

	display_fsq_rx_text(toprint.append(rx_text).append("\n"), FTextBase::FSQ_DIR);

	std::string response = station_calling;
	if (!relay.empty()) response.append(";").append(relay);
	response.append(" ack");
	reply(response);
}

void fsq::parse_plus(std::string relay)
{
	size_t p1 = NIT, p2 = NIT;
	std::string fname = "";
	p1 = rx_text.find('[');
	if (p1 != NIT) {
		p2 = rx_text.find(']', p1);
		if (p2 != NIT) {
			fname = rx_text.substr(p1 + 1, p2 - p1 - 1);
			fname = fl_filename_name(fname.c_str());
		} else p2 = 0;
	}
	if (fname.empty()) {
		if (!relay.empty()) fname = relay;
		else fname = station_calling;
		fname.append(".txt");
	}

	std::ifstream txfile;
	bool append = (fname == station_calling);
	std::string pathname = TempDir;
	if (append) {
		pathname.append(fname).append(".txt");
		txfile.open(pathname.c_str());
	} else {
		pathname.append(fname);
		txfile.open(pathname.c_str());
	}
	if (!txfile) {
		reply(std::string(station_calling).append(" not found"));
		return;
	}
	stringstream outtext(station_calling);
	outtext << " [" << fname << "]\n";
	char ch = txfile.get();
	while (!txfile.eof()) {
		outtext << ch;
		ch = txfile.get();
	}
	txfile.close();

	std::string response = station_calling;
	if (!relay.empty()) response.append(";").append(relay);
	response.append(outtext.str());
	reply(response);
}

void fsq::parse_minus()
{
	display_fsq_rx_text(toprint.append(rx_text).append(" nia\n"), FTextBase::FSQ_DIR);
	reply(std::string(station_calling).append(" not supported"));
}

void fsq::parse_at(std::string relay)
{
	std::string response = station_calling;
	if (!relay.empty()) response.append(";").append(relay);
	response.append(" ").append(progdefaults.myQth);
	reply(response);
	display_fsq_rx_text(toprint.append(rx_text).append("\n"), FTextBase::FSQ_DIR);
}

void fsq::parse_amp(std::string relay)
{
	std::string response = station_calling;
	if (!relay.empty()) response.append(";").append(relay);
	response.append(" ").append(progdefaults.fsqQTCtext);
	reply(response);
	display_fsq_rx_text(toprint.append(rx_text).append("\n"), FTextBase::FSQ_DIR);
}

void fsq::parse_carat(std::string relay)
{
	std::string response = station_calling;
	if (!relay.empty()) response.append(";").append(relay);
	response.append(" fldigi ").append(PACKAGE_VERSION);
	reply(response);
	display_fsq_rx_text(toprint.append(rx_text).append("\n"), FTextBase::FSQ_DIR);
}

void fsq::parse_pcnt()
{
	switch (rx_text[2]) {
		case 'L' :
			image_mode = 0; picW = 320; picH = 240;
			break;
		case 'S' :
			image_mode = 1; picW = 160; picH = 120;
			break;
		case 'F' :
			image_mode = 2; picW = 640; picH = 480;
			break;
		case 'V' :
			image_mode = 3; picW = 640; picH = 480;
			break;
		case 'P' :
			image_mode = 4; picW = 240; picH = 300;
			break;
		case 'p' :
			image_mode = 5; picW = 240; picH = 300;
			break;
		case 'M' :
			image_mode = 6; picW = 120; picH = 150;
			break;
		case 'm' :
			image_mode = 7; picW = 120; picH = 150;
			break;

	}
	REQ( fsq_showRxViewer, picW, picH, rx_text[2] );

	image_counter = 0;

	picf = 0;
	row = col = rgb = 0;
	state = IMAGE;

	display_fsq_rx_text(toprint.append(rx_text).append("\n"), FTextBase::FSQ_DIR);
}

static string alert;
static bool alert_pending = false;
static std::string delayed_alert = "";

void post_alert(void *)
{
	fl_alert2("%s", alert.c_str());
	if (active_modem->get_mode() == MODE_FSQ) {
		active_modem->send_ack(delayed_alert);
	}
	alert_pending = false;
}

void fsq::parse_vline(std::string relay)
{
	if (alert_pending) return;
	display_fsq_rx_text(toprint.append(rx_text).append("\n"), FTextBase::FSQ_DIR);
	alert = "Message received from ";
	if (relay.empty()) alert.append(station_calling);
	else alert.append(relay);
	alert.append("\n").append("at: ").append(zshowtime()).append("\n");
	alert.append(rx_text.substr(1));
	delayed_alert = relay;
	alert_pending = true;
	Fl::awake(post_alert);
}

void fsq::send_ack(std::string relay)
{
	std::string response = station_calling;
	if (!relay.empty()) response.append(";").append(relay);
	response.append(" ack");
	reply(response);
}


void fsq::parse_greater(std::string relay)
{
	std::string response;
	response.assign(station_calling);
	if (!relay.empty()) response.append(";").append(relay);

	double spd = progdefaults.fsqbaud;
	if (spd == 2.0) {
		spd = 3.0;
		response.append(" 3.0 baud");
	} else if (spd == 3.0) {
		spd = 4.5;
		response.append(" 4.5 baud");
	} else if (spd == 4.5) {
		spd = 6.0;
		response.append(" 6.0 baud");
	} else if (spd == 6.0) {
		response.append(" 6.0 baud");
	}
	progdefaults.fsqbaud = spd;
	adjust_for_speed();
	reply(response);
	display_fsq_rx_text(toprint.append(rx_text).append("\n"), FTextBase::FSQ_DIR);
}

void fsq::parse_less(std::string relay)
{
	std::string response;
	response.assign(station_calling);
	if (!relay.empty()) response.append(";").append(relay);

	double spd = progdefaults.fsqbaud;
	if (spd == 2.0) {
		response.append(" 2.0 baud");
	} else if (spd == 3.0) {
		spd = 2.0;
		response.append(" 2.0 baud");
	} else if (spd == 4.5) {
		spd = 3.0;
		response.append(" 3.0 baud");
	} else if (spd == 6.0) {
		spd = 4.5;
		response.append(" 4.5 baud");
	}
	progdefaults.fsqbaud = spd;
	adjust_for_speed();
	reply(response);
	display_fsq_rx_text(toprint.append(rx_text).append("\n"), FTextBase::FSQ_DIR);
}

void fsq::lf_check(int ch)
{
	static char lfpair[3] = "01";
	static char bstrng[4] = "012";

	lfpair[0] = lfpair[1];
	lfpair[1] = 0xFF & ch;

	bstrng[0] = bstrng[1];
	bstrng[1] = bstrng[2];
	bstrng[2] = 0xFF & ch;

	b_bot = b_eol = b_eot = false;
	if (bstrng[0] == FSQEOT[0]    // find SP SP BS SP
		&& bstrng[1] == FSQEOT[1]
		&& bstrng[2] == FSQEOT[2]
		) {
		b_eot = true;
	} else if (lfpair[0] == FSQBOL[0] && lfpair[1] == FSQBOL[1]) {
		b_bot = true;
	} else if (lfpair[0] == FSQEOL[0] && lfpair[1] == FSQEOL[1]) {
		b_eol = true;
	}
}

void fsq::process_symbol(int sym)
{
	int nibble = 0;
	int curr_ch = -1;

	symbol = sym;

	nibble = symbol - prev_symbol;
	if (nibble < -99 || nibble > 99) {
		prev_symbol = symbol;
		return;
	}
	nibble = nibbles[nibble + 99];

// -1 is our idle symbol, indicating we already have our symbol
	if (nibble >= 0) { // process nibble
		curr_nibble = nibble;

// single-nibble characters
		if ((prev_nibble < 29) & (curr_nibble < 29)) {
			curr_ch = wsq_varidecode[prev_nibble];

// double-nibble characters
		} else if ( (prev_nibble < 29) &&
					 (curr_nibble > 28) &&
					 (curr_nibble < 32)) {
			curr_ch = wsq_varidecode[prev_nibble * 32 + curr_nibble];
		}
		if (curr_ch > 0) {
			audit_log << fsq_ascii[curr_ch];// & 0xFF];
			if (curr_ch == '\n') audit_log << '\n';
			audit_log.flush();

			lf_check(curr_ch);

			if (b_bot) {
				ch_sqlch_open = true;
				rx_text.clear();
			}

			if (fsq_squelch_open()) {
				write_rx_mon_char(curr_ch);
				if (b_bot)
					display_fsq_mon_text( fsq_bot, FTextBase::CTRL);
				if (b_eol) {
					display_fsq_mon_text( fsq_eol, FTextBase::CTRL);
					noisefilt->reset();
					noisefilt->run(1);
					sigfilt->reset();
					sigfilt->run(1);
					snprintf(szestimate, sizeof(szestimate), "%.0f db", s2n );
				}
				if (b_eot) {
					snprintf(szestimate, sizeof(szestimate), "%.0f db", s2n );
					noisefilt->reset();
					noisefilt->run(1);
					sigfilt->reset();
					sigfilt->run(1);
					display_fsq_mon_text( fsq_eot, FTextBase::CTRL);
				}
			}

			if ( valid_char(curr_ch) || b_eol || b_eot ) {
				if (rx_text.length() > 32768) rx_text.clear();
				if ( fsq_squelch_open() || !progStatus.sqlonoff ) {
					rx_text += curr_ch;
					if (b_eot) {
						parse_rx_text();
						if (state == TEXT)
							ch_sqlch_open = false;
					}
				}
			}
			if (fsq_squelch_open() && (b_eot || b_eol)) {
				ch_sqlch_open = false;
			}
		}
		prev_nibble = curr_nibble;
	}

	prev_symbol = symbol;
}

// find the maximum bin
// 908 Hz and 1351 Hz respectively for original center frequency of 1145 Hz
// 1280 to 1720 for a 1500 Hz center frequency

static double sig = 0;

void fsq::process_tones()
{
	max = 0;
	peak = NUMBINS / 2;

// examine FFT bin contents over bandwidth +/- ~ 50 Hz
// 8 * 12000 / 2048 = 46.875 Hz
	int firstbin = frequency * FSQ_SYMLEN / samplerate - NUMBINS / 2;

	double sigval = 0;

	double mins[4];
	double min = 1e8;
	double temp;
	int k = 0;
	for (int i = 0; i < NUMBINS; ++i) {
		val = norm(fft_data[i + firstbin]);

		tones[i] = binfilt[i]->run(val);
		if (tones[i] > max) {
			max = tones[i];
			peak = i;
		}
// looking for minimum signal in a 3 bin sequence
		mins[k++] = val;
		if (k == 3) {
			temp = mins[0] + mins[1] + mins[2];
			if (temp < min) min = temp;
			k = 0;
		}
	}

	sigval = tones[peak-1] + tones[peak] + tones[peak+1];
	sigval /= 3;
	min /= 3;

	if (min <= 0.001 || sigval <= 0.001) {
		sig = 1;
		noise = 1;
	} else {
		sig = .95 * sig + .05 * sigval;
		noise = .99 * noise + .01 * min;
	}

	s2n = 10 * snfilt->run(log10(sig / noise)) - 36;

	if (s2n > 0) s2n *= 1.3;  // very empirical

//if (s2n > -30) {
//FILE *fsqtxt = fopen("fsq.txt", "a");
//fprintf(fsqtxt, "%d,%f,%f,%f,%f,%f\n", peak, sigval, sig, min, noise, s2n);
//fclose(fsqtxt);
//}

//scale to -25 to +45 db range
// -25 -> 0 linear
// 0 - > 45 compressed by 2

	if (s2n < -25) s2n = -25;
	if (s2n > 45) s2n = 45;

	if (s2n <= 0) metric = 2 * (25 + s2n);
	if (s2n > 0) metric = 50 *( 1 + s2n / 45);

	display_metric(metric);

	if (metric < progStatus.sldrSquelchValue && ch_sqlch_open)
		ch_sqlch_open = false;

// requires consecutive hits
	if (peak == prev_peak) {
		peak_counter++;
	} else {
		peak_counter = 0;
	}

	if ((peak_counter >= peak_hits) &&
		(peak != last_peak) &&
		(fsq_squelch_open() || !progStatus.sqlonoff)) {
		process_symbol(peak);
		peak_counter = 0;
		last_peak = peak;
	}

	prev_peak = peak;
}

void fsq::recvpic(double smpl)
{
	phase -= phidiff;
	if (phase < 0) phase += 2.0 * M_PI;

	cmplx z = smpl * cmplx( cos(phase), sin(phase ) );
	picfilter->run( z, currz);
	pixel += arg(conj(prevz) * currz);// * samplerate / TWOPI;
	amplitude += norm(currz);
	prevz = currz;

	if (image_counter <= -RXspp) {
		pixel = 0;
		amplitude = 0;
		image_counter++;
		return;
	}

	if ((image_counter++ % RXspp) == 0) {

		amplitude /= RXspp;
		pixel /= RXspp;
		pixel *= (samplerate / TWOPI);
		byte = pixel / 1.5 + 128;
		byte = (int)CLAMP( byte, 0.0, 255.0);

// FSQCAL sends blue-green-red
		static int RGB[] = {2, 1, 0};

		if (image_mode == 2 || image_mode == 5 || image_mode == 7) { // grey scale
			pixelnbr = 3 * (col + row * picW);
			REQ(fsq_updateRxPic, byte, pixelnbr);
			REQ(fsq_updateRxPic, byte, pixelnbr + 1);
			REQ(fsq_updateRxPic, byte, pixelnbr + 2);
			if (++ col == picW) {
				col = 0;
				row++;
				if (row >= picH) {
					state = TEXT;
					REQ(fsq_enableshift);
				}
			}
		} else {
			pixelnbr = RGB[rgb] + 3 * (col + row * picW);
			REQ(fsq_updateRxPic, byte, pixelnbr);
			if (++col == picW) {
				col = 0;
				if (++rgb == 3) {
					rgb = 0;
					row ++;
				}
			}
			if (row >= picH) {
				state = TEXT;
				REQ(fsq_enableshift);
			}
		}

		s2n = 10 * log10(snfilt->run(12000*amplitude/noise)) + 3.0;

		snprintf(szestimate, sizeof(szestimate), "%.0f db", s2n );

		metric = 2 * (s2n + 20);
		metric = CLAMP(metric, 0, 100.0);  // -20 to +30 db range
		display_metric(metric);

		pixel = 0;
		amplitude = 0;
	}
}

int fsq::rx_process(const double *buf, int len)
{
	if (peak_hits != progdefaults.fsqhits) restart();
	if (movavg_size != progdefaults.fsq_movavg) restart();
	if (speed != progdefaults.fsqbaud) restart();
	if (heard_log_fname != progdefaults.fsq_heard_log ||
		audit_log_fname != progdefaults.fsq_audit_log) restart();


	if (sounder_interval != progdefaults.fsq_sounder) {
		sounder_interval = progdefaults.fsq_sounder;
		start_sounder(sounder_interval);
	}

	if (bkptr < 0) bkptr = 0;
	if (bkptr >= SHIFT_SIZE) bkptr = 0;

	if (len > 512) {
		LOG_ERROR("fsq rx stream overrun %d", len);
	}

	if (progStatus.fsq_rx_abort) {
		state = TEXT;
		progStatus.fsq_rx_abort = false;
		REQ(fsq_clear_rximage);
	}

	while (len) {
		if (state == IMAGE) {
			recvpic(*buf);
			len--;
			buf++;
		} else {
			rx_stream[BLOCK_SIZE + bkptr] = *buf;
			len--;
			buf++;
			bkptr++;

			if (bkptr == SHIFT_SIZE) {
				bkptr = 0;
				memcpy(	rx_stream,							// to
						&rx_stream[SHIFT_SIZE],				// from
						BLOCK_SIZE*sizeof(*rx_stream));	// # bytes
				memset(fft_data, 0, sizeof(fft_data));
				for (int i = 0; i < BLOCK_SIZE; i++) {
					double d = rx_stream[i] * a_blackman[i];
					fft_data[i] = cmplx(d, d);
				}
				fft->ComplexFFT(fft_data);
				process_tones();
			}
		}
	}
	return 0;
}

//=====================================================================
// transmit processing
//=====================================================================

// implement the symbol counter using a new thread whose thread loop
// time is equal to a symbol length 4096/12000 = 341 milliseconds
// symbol loop decrements symbol counts and send_symbol increments them
// flush_buffer will then awake for the symbol count to be zero
// have not observed a remaining count > 1 so this might be an over kill!
// typical awake period is 90 msec

void fsq::flush_buffer()
{
	for (int i = 0; i < 64; i++) outbuf[i] = 0;
	ModulateXmtr(outbuf, 64);
	return;
}

#include "confdialog.h"

void fsq::send_tone(int tone)
{
	double phaseincr;
	double freq;

	if (speed != progdefaults.fsqbaud) restart();

	freq = (tx_basetone + tone * spacing) * samplerate / FSQ_SYMLEN;
	if (grpNoise->visible() && btnOffsetOn->value()==true)
		freq += ctrl_freq_offset->value();
	phaseincr = 2.0 * M_PI * freq / samplerate;
	prevtone = tone;

	int send_symlen = symlen;
	if (fsq_tx_image) send_symlen = 4096; // must use 3 baud symlen for image xfrs

	for (int i = 0; i < send_symlen; i++) {
		outbuf[i] = cos(txphase);
		txphase -= phaseincr;
		if (txphase < 0) txphase += TWOPI;
	}
	ModulateXmtr(outbuf, send_symlen);
}

void fsq::send_symbol(int sym)
{

	tone = (prevtone + sym + 1) % 33;
	send_tone(tone);
}

void fsq::send_idle()
{
	send_symbol(28);
	send_symbol(30);
}

static bool send_eot = false;

void fsq::send_char(int ch)
{
	if (!ch) return send_idle();

	int sym1 = fsq_varicode[ch][0];
	int sym2 = fsq_varicode[ch][1];

	send_symbol(sym1);
	if (sym2 > 28)
		send_symbol(sym2);

	if (valid_char(ch) && !(send_bot || send_eot))
		put_echo_char(ch);

	write_mon_tx_char(ch);
}

void fsq::send_image()
{
	int W = 640, H = 480;  // grey scale transfer (FAX)
	bool color = true;
	float freq, phaseincr;
	float radians = 2.0 * M_PI / samplerate;

	if (!fsqpicTxWin || !fsqpicTxWin->visible()) {
		return;
	}

	switch (selfsqpicSize->value()) {
		case 0 : W = 160; H = 120; break;
		case 1 : W = 320; H = 240; break;
		case 2 : W = 640; H = 480; color = false; break;
		case 3 : W = 640; H = 480; break;
		case 4 : W = 240; H = 300; break;
		case 5 : W = 240; H = 300; color = false; break;
		case 6 : W = 120; H = 150; break;
		case 7 : W = 120; H = 150; color = false; break;
	}

	REQ(fsq_clear_tximage);

	freq = frequency - 200;
	#define PHASE_CORR  200
	phaseincr = radians * freq;
	for (int n = 0; n < PHASE_CORR; n++) {
		outbuf[n] = cos(txphase);
		txphase -= phaseincr;
		if (txphase < 0) txphase += TWOPI;
	}
	ModulateXmtr(outbuf, 10);

	if (color == false) {  // grey scale image
		for (int row = 0; row < H; row++) {
			memset(outbuf, 0, 10 * sizeof(*outbuf));
			for (int col = 0; col < W; col++) {
				if (stopflag) return;
				tx_pixelnbr = col + row * W;
				tx_pixel =	0.3 * fsqpic_TxGetPixel(tx_pixelnbr, 0) +   // red
							0.6 * fsqpic_TxGetPixel(tx_pixelnbr, 1) +   // green
							0.1 * fsqpic_TxGetPixel(tx_pixelnbr, 2);    // blue
				REQ(fsq_updateTxPic, tx_pixel, tx_pixelnbr*3 + 0);
				REQ(fsq_updateTxPic, tx_pixel, tx_pixelnbr*3 + 1);
				REQ(fsq_updateTxPic, tx_pixel, tx_pixelnbr*3 + 2);
				freq = frequency - 200 + tx_pixel * 1.5;
				phaseincr = radians * freq;
				for (int n = 0; n < 10; n++) {
					outbuf[n] = cos(txphase);
					txphase -= phaseincr;
					if (txphase < 0) txphase += TWOPI;
				}
				ModulateXmtr(outbuf, 10);
				Fl::awake();
			}
		}
	} else {
		for (int row = 0; row < H; row++) {
			for (int color = 2; color >= 0; color--) {
				memset(outbuf, 0, 10 * sizeof(*outbuf));
				for (int col = 0; col < W; col++) {
					if (stopflag) return;
					tx_pixelnbr = col + row * W;
					tx_pixel = fsqpic_TxGetPixel(tx_pixelnbr, color);
					REQ(fsq_updateTxPic, tx_pixel, tx_pixelnbr*3 + color);
					freq = frequency - 200 + tx_pixel * 1.5;
					phaseincr = radians * freq;
					for (int n = 0; n < 10; n++) {
						outbuf[n] = cos(txphase);
						txphase -= phaseincr;
						if (txphase < 0) txphase += TWOPI;
					}
					ModulateXmtr(outbuf, 10);
				}
				Fl::awake();
			}
		}
	}
}

void fsq::send_string(std::string s)
{
	for (size_t n = 0; n < s.length(); n++)
		send_char(s[n]);
	if ((s == FSQEOT || s == FSQEOL) && fsq_tx_image) send_image();
}

void fsq::fsq_send_image(std::string s) {
	fsq_tx_image = true;
	fsq_string = std::string("IMAGE:").append(s);
	write_fsq_que(fsq_string);
	fsq_xmt(s);
}

int fsq::tx_process()
{
	if (send_bot) {
		std::string send;
		send.assign(" ").append(FSQBOL).append(mycall).append(":");
		if (progdefaults.fsq_directed)
			send.append(crc.sval(mycall));
		send_string(send);
		send_bot = false;
	}
	int c = get_tx_char();

	if (c == GET_TX_CHAR_ETX  || c == -1) { // end of text or empty tx buffer
		send_eot = true;
		if (progdefaults.fsq_directed)
			send_string(std::string(FSQEOT));
		else
			send_string(std::string(FSQEOL));
		send_eot = false;
		put_echo_char('\n');
		if (c == -1) REQ(&FTextTX::clear, TransmitText);
		flush_buffer();
		stopflag = false;
		fsq_tx_image = false;
		return -1;
	}
	if ( stopflag ) { // aborts transmission
		static std::string aborted = " !ABORTED!\n";
		for (size_t n = 0; n < aborted.length(); n++)
			put_echo_char(aborted[n]);
		fsq_tx_image = false;
		stopflag = false;
		clear_xmt_arrays();
		return -1;
	}
	send_char(c);
	return 0;
}

//==============================================================================
// delayed transmit
//==============================================================================

static pthread_mutex_t fsq_tx_mutex = PTHREAD_MUTEX_INITIALIZER;
static float xmt_repeat_try = 6.0;
static string tx_text_queue = "";

static vector<string> commands;
#define NUMCOMMANDS 10
static size_t next = 0;

void  clear_xmt_arrays()
{
	commands.erase(commands.begin(), commands.begin());
	tx_text_queue.clear();
	fsq_tx_text->clear();
}

double fsq_xmtdelay() // in seconds
{
#define MIN_DELAY  50
#define MAX_DELAY  500
	srand((int)clock());
	double scaled = (double)rand()/RAND_MAX;
	double delay = ((MAX_DELAY - MIN_DELAY + 1 ) * scaled + MIN_DELAY) / 1000.0;
	if (delay < 0.05) delay = 0.05;
	if (delay > 0.5) delay = 0.5;
	return delay;
}

void fsq_repeat_last_command()
{
	using ::next;  // disambiguate from std::next
	if (commands.empty()) return;
	fsq_tx_text->clear();
	fsq_tx_text->addstr(sz2utf8(commands[next].c_str()));
	next++;
	if (next == commands.size()) {
		next = 0;
	}
}

int get_fsq_tx_char(void)
{
	guard_lock tx_proc_lock(&fsq_tx_mutex);

	if (tx_text_queue.empty()) return (GET_TX_CHAR_NODATA);

	int c = tx_text_queue[0];
	tx_text_queue.erase(0,1);

	if (c == GET_TX_CHAR_ETX) {
		return c;
	}
	if (c == -1)
		return(GET_TX_CHAR_NODATA);

	if (c == '^') {
		c = tx_text_queue[0];
		tx_text_queue.erase(0,1);

		if (c == -1) return(GET_TX_CHAR_NODATA);

		switch (c) {
			case 'r':
				return(GET_TX_CHAR_ETX);
				break;
			case 'R':
				return(GET_TX_CHAR_ETX);
				break;
			default: ;
		}
	}
	return(c);
}

void try_transmit(void *)
{
	if (active_modem != fsq_modem) return;

	if (!active_modem->fsq_squelch_open() && trx_state == STATE_RX) {
		::next = 0;
		fsq_que_clear();
//LOG_WARN("%s", "start_tx()");
		start_tx();
		return;
	}

	if (xmt_repeat_try > 0) {
		xmt_repeat_try -= 0.5;
		static char szwaiting[50];
		snprintf(szwaiting, sizeof(szwaiting), "Waiting %4.2f", xmt_repeat_try);
		fsq_que_clear();
		write_fsq_que(std::string(szwaiting).append("\n").append(fsq_string));
//LOG_WARN("%s", szwaiting);
		Fl::repeat_timeout(0.5, try_transmit);
		return;
	} else {
		static const char szsquelch[50] = "Squelch open.  Transmit timed out!";
		display_fsq_rx_text(string("\n").append(szsquelch).append("\n").c_str(), FTextBase::ALTR);
		tx_text_queue.clear();
		fsq_que_clear();
		if (active_modem->fsq_tx_image) active_modem->fsq_tx_image = false;
//LOG_WARN("%s", szsquelch);
		return;
	}
	return;
}

inline void _fsq_xmt(string s)
{
	tx_text_queue.clear();
	if (commands.size() > NUMCOMMANDS)
		commands.pop_back();

	commands.insert(commands.begin(), 1, s);
	s.append("^r");
	tx_text_queue = s;

	xmt_repeat_try = progdefaults.fsq_time_out;
	Fl::add_timeout(0.5 + fsq_xmtdelay(), try_transmit);
}

void fsq_xmt_mt(void *cs = (void *)0)
{
	guard_lock tx_proc_lock(&fsq_tx_mutex);

	if (active_modem != fsq_modem) return;
    if (!cs) return;

	string s;
	s.assign((char *) cs);
	delete (char *) cs;

	if(!s.empty()) {
		_fsq_xmt(s);
    }
}

void fsq_xmt(string s)
{
	guard_lock tx_proc_lock(&fsq_tx_mutex);

	if (active_modem != fsq_modem) return;

	if(!s.empty()) {
		_fsq_xmt(s);
    }
}

void fsq_transmit(void *a = (void *)0)
{
	guard_lock tx_proc_lock(&fsq_tx_mutex);

	if (active_modem != fsq_modem) return;

	if (!tx_text_queue.empty()) {
		size_t p = tx_text_queue.find("^r");
		tx_text_queue.erase(p);
		tx_text_queue += ' ';
		int nxt = fsq_tx_text->nextChar();
		while (nxt != -1) {
			tx_text_queue += nxt;
			nxt = fsq_tx_text->nextChar();
		}
		commands.erase(commands.begin(), commands.begin());
		commands.insert(commands.begin(), 1, tx_text_queue);
		tx_text_queue.append("^r");
		fsq_tx_text->clear();
		return;
//LOG_WARN("A: %s", tx_text_queue.c_str());
	}

	int nxt = fsq_tx_text->nextChar();
	while (nxt != -1) {
		tx_text_queue += nxt;
		nxt = fsq_tx_text->nextChar();
	}
	if (commands.size() > NUMCOMMANDS)
		commands.pop_back();
	commands.insert(commands.begin(), 1, tx_text_queue);
	tx_text_queue.append("^r");
	fsq_tx_text->clear();
//LOG_WARN("B: %s", tx_text_queue.c_str());

	xmt_repeat_try = progdefaults.fsq_time_out;
	Fl::add_timeout(0.5 + fsq_xmtdelay(), try_transmit);
}

void timed_xmt(void *)
{
//LOG_WARN("%s", fsq_delayed_string.c_str());
	fsq_xmt(fsq_delayed_string);
}

static float secs = 0;

void fsq_add_tx_timeout(void *a = 0)
{
	Fl::add_timeout(secs, timed_xmt);
}

void fsq::reply(std::string s)
{
	fsq_string = std::string("SEND: ").append(s);
	write_fsq_que(fsq_string);
	char *cs = (char *)0;
	cs = new char[s.size() + 1];
    if(!cs) return;
	cs[s.size()] = 0;
	memcpy(cs, s.c_str(), s.size());
	Fl::awake(fsq_xmt_mt, (void *) cs);
}

void fsq::delayed_reply(std::string s, int delay)
{
	fsq_string = std::string("DELAYED SEND: ").append(s);
	write_fsq_que(fsq_string);
	fsq_delayed_string = s;
	secs = delay;
	Fl::awake(fsq_add_tx_timeout, 0);
//LOG_WARN("%s : %d", s.c_str(), delay);
}

//==============================================================================
// Heard list aging
//==============================================================================
void aging(void *who)
{
	fsq *me = (fsq *)who;
	if (me != active_modem) return;
	age_heard_list();
	Fl::repeat_timeout(60.0, aging, me);
}

void fsq_start_aging(void *who)
{
	fsq	*me = (fsq *)who;
	Fl::remove_timeout(aging);
	Fl::add_timeout(60.0, aging, me);
}

void fsq::start_aging()
{
	Fl::awake(fsq_start_aging, this);
}

void fsq_stop_aging(void *)
{
	Fl::remove_timeout(aging);
}

void fsq::stop_aging()
{
	Fl::awake(fsq_stop_aging);
}

//==============================================================================
// Sounder support
//==============================================================================
static int sounder_tries = 10;
static double sounder_secs = 60;

void sounder(void *)
{
	if (active_modem != fsq_modem) return;

	if (trx_state == STATE_TX) {
		Fl::repeat_timeout(active_modem->fsq_xmtdelay(), timed_xmt);
		return;
	}
	if (active_modem->fsq_squelch_open()) {
		if (--sounder_tries < 0) {
			display_fsq_rx_text("\nSounder timed out!\n", FTextBase::ALTR);
			sounder_tries = 10;
			Fl::repeat_timeout(sounder_secs, sounder);
			return;
		}
		Fl::repeat_timeout(10, sounder); // retry in 10 seconds
		return;
	}
	sounder_tries = 10;
	std::string xmtstr = FSQBOL;
	xmtstr.append(active_modem->fsq_mycall()).append(":").append(FSQEOT);
	int numsymbols = xmtstr.length();
	int xmtsecs = (int)(1.0 * numsymbols * (fsq::symlen / 4096.0) / SR);
	if (fsq_tx_text->eot()) {
		std::string stime = ztime();
		stime.erase(4);
		stime.insert(2,":");
		std::string sndx = "\nSounded @ ";
		sndx.append(stime);
		fsq_string = std::string("SOUNDEX:").append(sndx);
		write_fsq_que(fsq_string);
		fsq_xmt(sndx);
	}
	Fl::repeat_timeout(sounder_secs + xmtsecs, sounder);
}

void fsq_start_sounder()
{
	if (active_modem != fsq_modem) return;
	Fl::remove_timeout(sounder);
	Fl::add_timeout(sounder_secs, sounder);
}

void fsq_stop_sounder()
{
	Fl::remove_timeout(sounder);
}

void fsq::stop_sounder()
{
	REQ(fsq_stop_sounder);
}

void fsq::start_sounder(int interval)
{
	if (interval == 0) {
		REQ(fsq_stop_sounder);
		return;
	}
	switch (interval) {
		case 0: return;
		case 1: sounder_secs = 60; break;   // 1 minute
		case 2: sounder_secs = 600; break;  // 10 minutes
		case 3: sounder_secs = 1800; break; // 30 minutes
		case 4: sounder_secs = 3600; break; // 60 minutes
		default: sounder_secs = 600;
	}
	REQ(fsq_start_sounder);
}

#include "bitmaps.cxx"
