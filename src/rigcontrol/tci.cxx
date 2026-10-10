// ----------------------------------------------------------------------------
// tci.cxx  --  TCI 2.0 client for Zeus and ExpertSDR3
//
// Copyright (C) 2026
//		Chris, KC2RGW
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
//
// One WebSocket carries CAT text frames and RX/TX PCM binary frames.
// Dial frequency is VFO:<receiver>,<channel>,<Hz>;  channel 0 is VFO A.
// A two-argument form is accepted on input for older servers.
// PTT is sent as  trx:<rx>,true,tci;  so the server uses this stream
// instead of the station microphone. TX audio is paced by TX_CHRONO.
//
// Configure -> TCI (Zeus / ExpertSDR). Defaults: 127.0.0.1 port 40001.
// TCI is off until the operator enables it. Hamlib, rigCAT, flrig, and
// the sound card stay in use while it is off.
// ----------------------------------------------------------------------------

#include <config.h>

#include "tci.h"

#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <deque>
#include <sstream>
#include <string>
#include <vector>

#include <pthread.h>
#include <unistd.h>

#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Window.H>

#include "configuration.h"
#include "debug.h"
#include "gettext.h"
#include "fl_digi.h"
#include "macros.h"
#include "status.h"
#include "modem.h"
#include "misc.h"
#include "socket.h"
#include "sound.h"
#include "trx.h"

LOG_FILE_SOURCE(debug::LOG_RIGCONTROL);

#include <stdint.h>

static const int TCI_RX_AUDIO = 1;
static const int TCI_TX_AUDIO = 2;
static const int TCI_TX_CHRONO = 3;
static const int TCI_LINEOUT = 4;

// Receiver-audio frames (type 1) win while they are arriving. Line-out
// (type 4) is the copy ExpertSDR3 sends to every client without a key.
static time_t tci_last_rx_audio = 0;
static int tci_lineout_logged = 0;

static pthread_t tci_thread;
static pthread_mutex_t tci_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool tci_thread_started = false;
static bool tci_quit = false;
static bool tci_connected = false;
static bool tci_drop_socket = false;

static Socket* tci_sock = 0;

static std::string tci_status = "TCI idle";
static unsigned long tci_freq = 0;
static std::string tci_mode = "USB";
static bool tci_freq_dirty = false;
static bool tci_mode_dirty = false;
static std::string tci_modes_list;
static bool tci_modes_dirty = false;
static int tci_filt_lo = 0;
static int tci_filt_hi = 0;
static int tci_filt_sent_lo = -999999;
static int tci_filt_sent_hi = -999999;
static bool tci_filt_dirty = false;
static unsigned long tci_last_sent_freq = 0;
static std::string tci_last_sent_mode;
static std::string tci_device;

static int tci_dev_rate = 48000;
static int tci_sample_type = 3; // FLOAT32
static int tci_channels = 2;
static int tci_wire_channels = 2;
static bool tci_ptt = false;
static bool tci_tx_on = false;

static std::deque<float> tci_rx_q;
static double tci_rx_phase = 0.0;
static std::deque<float> tci_tx_q;
// Joins one modem block to the next so the 8 kHz -> 48 kHz step stays continuous.
static float tci_tx_hist = 0.f;
static double tci_tx_pos = 0.0;
static int tci_tx_rs_modem = 0;

static Fl_Window* tci_dlg = 0;
static Fl_Input* tci_in_host = 0;
static Fl_Input* tci_in_port = 0;
static Fl_Input* tci_in_rx = 0;
static Fl_Check_Button* tci_ck_en = 0;
static Fl_Check_Button* tci_ck_au = 0;
static Fl_Box* tci_status_box = 0;

static std::string lower_copy(std::string s)
{
	for (size_t i = 0; i < s.size(); i++) {
		if (s[i] >= 'A' && s[i] <= 'Z')
			s[i] = s[i] - 'A' + 'a';
	}
	return s;
}

static std::string trim_copy(const std::string& s)
{
	size_t a = 0;
	while (a < s.size() && (s[a] == ' ' || s[a] == '\t' || s[a] == '\r' || s[a] == '\n'))
		a++;
	size_t b = s.size();
	while (b > a && (s[b-1] == ' ' || s[b-1] == '\t' || s[b-1] == '\r' || s[b-1] == '\n'))
		b--;
	return s.substr(a, b - a);
}

static int sample_bytes(int fmt)
{
	if (fmt == 0) return 2;
	if (fmt == 1) return 3;
	return 4;
}

static bool mode_is_lsb(const std::string& m)
{
	std::string s = lower_copy(m);
	if (s == "lsb" || s == "digl" || s == "cwl" || s == "cw-l" || s == "lcw")
		return true;
	if (s.find("lsb") != std::string::npos)
		return true;
	return false;
}

static bool zeus_device()
{
	std::string d;
	pthread_mutex_lock(&tci_mutex);
	d = lower_copy(tci_device);
	pthread_mutex_unlock(&tci_mutex);
	return d.find("zeus") != std::string::npos;
}

// Two real channels, same sample on left and right. Zeus mixes that
// pair to mono the same way it does for WSJT-X.
static int wire_channels()
{
	int ch;
	pthread_mutex_lock(&tci_mutex);
	ch = tci_wire_channels;
	pthread_mutex_unlock(&tci_mutex);
	return (ch == 1) ? 1 : 2;
}

static std::string mode_to_tci(const char* fldigi_mode)
{
	if (!fldigi_mode || !*fldigi_mode)
		return "DIGU";
	std::string s = lower_copy(fldigi_mode);
	if (s == "cw" || s == "cwu" || s == "cw-u")
		return "CW";
	if (s == "cwl" || s == "cw-l")
		return "CW";
	if (mode_is_lsb(s))
		return "DIGL";
	return "DIGU";
}

bool tci_is_active()
{
	return progdefaults.tci_enable && tci_connected;
}

bool tci_audio_wanted()
{
	return progdefaults.tci_enable && progdefaults.tci_audio;
}

static void set_status(const std::string& s)
{
	pthread_mutex_lock(&tci_mutex);
	tci_status = s;
	pthread_mutex_unlock(&tci_mutex);
}

static std::string b64_16(const unsigned char raw[16])
{
	static const char* tab =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	std::string o;
	o.resize(24);
	int j = 0;
	for (int i = 0; i < 15; i += 3) {
		unsigned n = (raw[i] << 16) | (raw[i+1] << 8) | raw[i+2];
		o[j++] = tab[(n >> 18) & 63];
		o[j++] = tab[(n >> 12) & 63];
		o[j++] = tab[(n >> 6) & 63];
		o[j++] = tab[n & 63];
	}
	unsigned n = (raw[15] << 16);
	o[j++] = tab[(n >> 18) & 63];
	o[j++] = tab[(n >> 12) & 63];
	o[j++] = '=';
	o[j++] = '=';
	return o;
}

static bool sock_send_all(const void* data, size_t n)
{
	const char* p = (const char*)data;
	size_t off = 0;
	while (off < n) {
		if (tci_drop_socket || !tci_sock)
			return false;
		size_t w = 0;
		try {
			w = tci_sock->send(p + off, n - off);
		} catch (const SocketException& e) {
			LOG_ERROR("TCI send: %s", e.what());
			return false;
		}
		if (w == 0) {
			MilliSleep(5);
			continue;
		}
		off += w;
	}
	return true;
}

static bool ws_send(int opcode, const void* data, size_t n)
{
	unsigned char hdr[14];
	size_t hlen = 2;
	hdr[0] = 0x80 | (opcode & 0x0f);
	if (n < 126) {
		hdr[1] = 0x80 | (unsigned char)n;
	} else if (n <= 0xffff) {
		hdr[1] = 0x80 | 126;
		hdr[2] = (n >> 8) & 0xff;
		hdr[3] = n & 0xff;
		hlen = 4;
	} else {
		hdr[1] = 0x80 | 127;
		for (int i = 0; i < 8; i++)
			hdr[2 + i] = (unsigned char)((n >> (56 - 8 * i)) & 0xff);
		hlen = 10;
	}
	unsigned char mask[4];
	for (int i = 0; i < 4; i++)
		mask[i] = (unsigned char)(rand() & 0xff);
	memcpy(hdr + hlen, mask, 4);
	hlen += 4;

	std::vector<unsigned char> frame(hlen + n);
	memcpy(&frame[0], hdr, hlen);
	const unsigned char* src = (const unsigned char*)data;
	for (size_t i = 0; i < n; i++)
		frame[hlen + i] = src[i] ^ mask[i & 3];
	pthread_mutex_lock(&tci_mutex);
	bool ok = sock_send_all(&frame[0], frame.size());
	pthread_mutex_unlock(&tci_mutex);
	return ok;
}

static bool ws_send_text(const std::string& s)
{
	LOG_DEBUG("TCI >> %s", s.c_str());
	return ws_send(1, s.data(), s.size());
}

static void close_sock()
{
	pthread_mutex_lock(&tci_mutex);
	if (tci_sock) {
		try { tci_sock->close(); } catch (...) {}
		delete tci_sock;
		tci_sock = 0;
	}
	tci_connected = false;
	tci_rx_q.clear();
	tci_rx_phase = 0.0;
	pthread_mutex_unlock(&tci_mutex);
}

static bool handshake(Socket& s, const std::string& host, const std::string& port, std::string& leftover)
{
	leftover.clear();
	unsigned char rnd[16];
	FILE* ur = fopen("/dev/urandom", "rb");
	if (ur) {
		size_t n = fread(rnd, 1, 16, ur);
		fclose(ur);
		if (n != 16) {
			for (int i = 0; i < 16; i++)
				rnd[i] = (unsigned char)(rand() & 0xff);
		}
	} else {
		for (int i = 0; i < 16; i++)
			rnd[i] = (unsigned char)(rand() & 0xff);
	}
	std::string key = b64_16(rnd);
	std::ostringstream req;
	req << "GET / HTTP/1.1\r\n"
	    << "Host: " << host << ":" << port << "\r\n"
	    << "Upgrade: websocket\r\n"
	    << "Connection: Upgrade\r\n"
	    << "Sec-WebSocket-Key: " << key << "\r\n"
	    << "Sec-WebSocket-Version: 13\r\n"
	    << "\r\n";
	std::string out = req.str();
	size_t off = 0;
	while (off < out.size()) {
		size_t w = s.send(out.data() + off, out.size() - off);
		if (w == 0)
			return false;
		off += w;
	}
	std::string resp;
	char buf[512];
	for (int i = 0; i < 50 && resp.find("\r\n\r\n") == std::string::npos; i++) {
		size_t r = s.recv(buf, sizeof(buf));
		if (r == 0) {
			MilliSleep(20);
			continue;
		}
		resp.append(buf, r);
		if (resp.size() > 8192)
			break;
	}
	size_t hdr_end = resp.find("\r\n\r\n");
	if (resp.find("101") == std::string::npos || hdr_end == std::string::npos) {
		LOG_ERROR("TCI handshake failed: %.80s", resp.c_str());
		return false;
	}
	leftover = resp.substr(hdr_end + 4);
	return true;
}

static void subscribe_audio()
{
	int rx = progdefaults.tci_rx;
	if (rx < 0) rx = 0;
	// audio_start has to go out by itself. ExpertSDR3 stops parsing a
	// line at the first command it does not know, so putting the Zeus
	// negotiation in front of it meant the stream never started. VFO
	// commands are a separate message, which is why the dial still moved.
	char start[64];
	snprintf(start, sizeof(start), "audio_start:%d;", rx);
	ws_send_text(start);
	// Separate message. ExpertSDR3 stops reading a line at the first
	// command it does not know. Line-out is the receiver copy that
	// reaches every client, so the SunSDR can start audio with no TX.
	char line[64];
	snprintf(line, sizeof(line), "line_out_start:%d;", rx);
	ws_send_text(line);
	LOG_INFO("TCI audio_start and line_out_start rx %d", rx);
	if (!zeus_device())
		return;
	pthread_mutex_lock(&tci_mutex);
	tci_wire_channels = 2;
	pthread_mutex_unlock(&tci_mutex);
	char cmd[256];
	snprintf(cmd, sizeof(cmd),
		"audio_samplerate:48000;"
		"audio_stream_sample_type:float32;"
		"audio_stream_channels:2;"
		"audio_stream_samples:2048;");
	ws_send_text(cmd);
	ws_send_text(start);
}

static unsigned long parse_hz(const std::string& s)
{
	if (s.empty())
		return 0;
	char* end = 0;
	double v = strtod(s.c_str(), &end);
	if (end == s.c_str() || v < 1.0 || v > 25000000000.0)
		return 0;
	return (unsigned long)(v + 0.5);
}

static void note_remote_freq(unsigned long hz)
{
	if (hz == 0)
		return;
	pthread_mutex_lock(&tci_mutex);
	if (hz != tci_freq) {
		tci_freq = hz;
		tci_last_sent_freq = hz;
		tci_freq_dirty = true;
		LOG_INFO("TCI dial %lu Hz", hz);
	}
	pthread_mutex_unlock(&tci_mutex);
}

static void push_rx_samples(const float* s, size_t n)
{
	pthread_mutex_lock(&tci_mutex);
	for (size_t i = 0; i < n; i++)
		tci_rx_q.push_back(s[i]);
	size_t cap = (size_t)tci_dev_rate * 2;
	if (cap < 48000) cap = 48000;
	while (tci_rx_q.size() > cap)
		tci_rx_q.pop_front();
	pthread_mutex_unlock(&tci_mutex);
}

static void send_tx_frame(size_t nsamp, int chrono_rate, int fmt, int ch)
{
	(void)fmt;
	(void)ch;
	// One chrono is one block of real time: 2048 samples at 48 kHz. Send
	// exactly that many, every time. A skipped or short block is a hole in
	// the RF, and the station copying us loses the symbol clock even though
	// the tone still sounds right.
	if (nsamp < 2)
		return;
	if (chrono_rate < 8000)
		chrono_rate = 48000;
	nsamp &= ~1u;
	size_t frames = nsamp / 2;
	if (frames < 64)
		frames = 2048;
	if (frames > 65536)
		frames = 65536;

	// A short chrono is still a request for audio. Dropping it, which we
	// did when the queue was not already full, makes ExpertSDR3 stop
	// asking. Send the real samples on hand. Never hold the last sample
	// flat: that erases the PSK phase changes.
	for (int w = 0; w < 20; w++) {
		pthread_mutex_lock(&tci_mutex);
		bool enough = tci_tx_q.size() >= frames;
		bool tx = tci_tx_on;
		pthread_mutex_unlock(&tci_mutex);
		if (enough || !tx)
			break;
		MilliSleep(2);
	}

	std::vector<float> samps(frames, 0.f);
	size_t have = 0;
	pthread_mutex_lock(&tci_mutex);
	have = tci_tx_q.size() < frames ? tci_tx_q.size() : frames;
	for (size_t i = 0; i < have; i++) {
		samps[i] = tci_tx_q.front();
		tci_tx_q.pop_front();
	}
	pthread_mutex_unlock(&tci_mutex);
	if (have < 2)
		return;

	if (have > 200 && chrono_rate > 0) {
		int crosses = 0;
		for (size_t i = 1; i < have; i++) {
			if (samps[i - 1] <= 0.f && samps[i] > 0.f)
				crosses++;
		}
		if (crosses > 1) {
			int hz = (int)((double)crosses * chrono_rate / (double)have + 0.5);
			static int shown = -1;
			if (shown < 0 || hz > shown + 40 || hz < shown - 40) {
				shown = hz;
				char msg[64];
				snprintf(msg, sizeof(msg), "TCI TX audio %d Hz", hz);
				put_status(msg, 10.0);
			}
		}
	}

	uint32_t hdr[16];
	memset(hdr, 0, sizeof(hdr));
	hdr[0] = (uint32_t)progdefaults.tci_rx;
	hdr[1] = (uint32_t)chrono_rate;
	hdr[2] = 3; // float32
	int chans = wire_channels();
	hdr[5] = (uint32_t)(chans == 1 ? have : have * 2);
	hdr[6] = TCI_TX_AUDIO;
	hdr[7] = (uint32_t)chans;

	size_t nbytes = (size_t)chans * 4;
	std::vector<unsigned char> payload(64 + have * nbytes);
	memcpy(&payload[0], hdr, 64);
	for (size_t i = 0; i < have; i++) {
		float s = samps[i];
		if (s > 1.f) s = 1.f;
		if (s < -1.f) s = -1.f;
		unsigned char* d = &payload[64 + i * nbytes];
		memcpy(d, &s, 4);
		if (chans == 2)
			memcpy(d + 4, &s, 4);
	}
	ws_send(2, &payload[0], payload.size());
}

static void handle_text(const std::string& msg)
{
	std::string rest = msg;
	while (!rest.empty()) {
		size_t semi = rest.find(';');
		std::string one = trim_copy(semi == std::string::npos ? rest : rest.substr(0, semi));
		rest = (semi == std::string::npos) ? "" : rest.substr(semi + 1);
		if (one.empty())
			continue;
		size_t colon = one.find(':');
		std::string name = lower_copy(colon == std::string::npos ? one : one.substr(0, colon));
		std::string args = colon == std::string::npos ? "" : one.substr(colon + 1);
		std::vector<std::string> av;
		std::stringstream ss(args);
		std::string item;
		while (std::getline(ss, item, ','))
			av.push_back(trim_copy(item));

		int rx = progdefaults.tci_rx;
		if (name == "vfo" && av.size() >= 2) {
			int rxi = atoi(av[0].c_str());
			if (rxi != rx)
				continue;
			int channel = 0;
			unsigned long hz = 0;
			if (av.size() >= 3) {
				channel = atoi(av[1].c_str());
				hz = parse_hz(av[2]);
			} else {
				hz = parse_hz(av[1]);
			}
			if (channel != 0)
				continue;
			note_remote_freq(hz);
		} else if ((name == "modulation" || name == "mode") && av.size() >= 2) {
			int rxi = atoi(av[0].c_str());
			if (rxi != rx)
				continue;
			pthread_mutex_lock(&tci_mutex);
			if (av[1] != tci_last_sent_mode) {
				tci_mode = av[1];
				tci_mode_dirty = true;
			}
			pthread_mutex_unlock(&tci_mutex);
		} else if (name == "modulations_list" && !av.empty()) {
			std::string list;
			for (size_t i = 0; i < av.size(); i++) {
				if (av[i].empty())
					continue;
				if (!list.empty())
					list += ",";
				list += av[i];
			}
			pthread_mutex_lock(&tci_mutex);
			if (list != tci_modes_list) {
				tci_modes_list = list;
				tci_modes_dirty = true;
			}
			pthread_mutex_unlock(&tci_mutex);
		} else if (name == "rx_filter_band" && av.size() >= 3) {
			int rxi = atoi(av[0].c_str());
			if (rxi != rx)
				continue;
			int lo = atoi(av[1].c_str());
			int hi = atoi(av[2].c_str());
			pthread_mutex_lock(&tci_mutex);
			if (lo != tci_filt_sent_lo || hi != tci_filt_sent_hi) {
				tci_filt_lo = lo;
				tci_filt_hi = hi;
				tci_filt_dirty = true;
			}
			pthread_mutex_unlock(&tci_mutex);
		} else if (name == "device" && !av.empty()) {
			pthread_mutex_lock(&tci_mutex);
			tci_device = av[0];
			pthread_mutex_unlock(&tci_mutex);
			subscribe_audio();
		} else if (name == "trx" && av.size() >= 2) {
			int rxi = atoi(av[0].c_str());
			if (rxi != rx)
				continue;
			std::string b = lower_copy(av[1]);
			pthread_mutex_lock(&tci_mutex);
			tci_ptt = (b == "true" || b == "1");
			pthread_mutex_unlock(&tci_mutex);
		} else if (name == "audio_samplerate" && !av.empty()) {
			// Zeus keeps this stream at 48000. Ignore other advertisements.
		} else if (name == "audio_stream_sample_type" && !av.empty()) {
			std::string t = lower_copy(av[av.size() - 1]);
			int fmt = 3;
			if (t == "int16" || t == "0") fmt = 0;
			else if (t == "int24" || t == "1") fmt = 1;
			else if (t == "int32" || t == "2") fmt = 2;
			else fmt = 3;
			pthread_mutex_lock(&tci_mutex);
			tci_sample_type = fmt;
			pthread_mutex_unlock(&tci_mutex);
		} else if (name == "audio_stream_channels" && !av.empty()) {
			int ch = atoi(av[av.size() - 1].c_str());
			if (ch >= 1 && ch <= 2) {
				pthread_mutex_lock(&tci_mutex);
				tci_channels = ch;
				tci_wire_channels = ch;
				pthread_mutex_unlock(&tci_mutex);
			}
		}
	}
}

static void handle_binary(const unsigned char* data, size_t n)
{
	if (n < 64)
		return;
	uint32_t hdr[16];
	memcpy(hdr, data, 64);
	uint32_t rx = hdr[0];
	uint32_t rate = hdr[1];
	uint32_t fmt = hdr[2];
	uint32_t length = hdr[5];
	uint32_t type = hdr[6];
	uint32_t ch = hdr[7];
	(void)rx;
	// Zeus writes zeros in the channel field and sends left = right.
	// Treating that as mono counts every sample twice, so a 3000 Hz
	// edge shows up near 1500 Hz. ExpertSDR3 fills the field in.
	bool lineout = false;
	if (type == TCI_RX_AUDIO) {
		pthread_mutex_lock(&tci_mutex);
		tci_last_rx_audio = time(0);
		pthread_mutex_unlock(&tci_mutex);
	} else if (type == TCI_LINEOUT) {
		time_t last = 0;
		pthread_mutex_lock(&tci_mutex);
		last = tci_last_rx_audio;
		pthread_mutex_unlock(&tci_mutex);
		time_t now = time(0);
		if (last != 0 && now >= last && now - last < 2)
			return;
		lineout = true;
		if (!tci_lineout_logged) {
			tci_lineout_logged = 1;
			LOG_INFO("TCI line-out frame rate %u fmt %u len %u ch %u bytes %zu",
				rate, fmt, length, ch, n);
		}
		type = TCI_RX_AUDIO;
	}

	if (rate >= 8000 && rate <= 384000 && type == TCI_RX_AUDIO) {
		pthread_mutex_lock(&tci_mutex);
		tci_dev_rate = (int)rate;
		if (fmt <= 3) tci_sample_type = (int)fmt;
		pthread_mutex_unlock(&tci_mutex);
	} else if (rate >= 8000 && rate <= 384000 && type == TCI_TX_CHRONO) {
		pthread_mutex_lock(&tci_mutex);
		if (fmt <= 3) tci_sample_type = (int)fmt;
		pthread_mutex_unlock(&tci_mutex);
	}

	if (type == TCI_TX_CHRONO) {
		int chrono_rate = 48000;
		if (rate >= 8000 && rate <= 384000)
			chrono_rate = (int)rate;
		if (tci_tx_on && length < 200000)
			send_tx_frame(length > 1 ? length : 4096, chrono_rate, 3, (int)ch);
		return;
	}
	if (type != TCI_RX_AUDIO)
		return;

	int bps = sample_bytes((int)fmt);
	if (bps <= 0)
		return;
	size_t avail = (n - 64) / (size_t)bps;
	// ExpertSDR3: length is the sample count and the payload holds
	// length * channels floats. Zeus: length is already the float count.
	int chans = (ch == 1 || ch == 2) ? (int)ch : (zeus_device() ? 2 : 1);
	size_t frames = 0;
	bool sample_count = chans > 1 && length > 0 && length * (size_t)chans <= avail;
	if (sample_count && (zeus_device() == false || length * (size_t)chans == avail))
		frames = length;
	else if (length > 0 && length <= avail)
		frames = (chans == 2) ? length / 2 : length;
	else
		frames = (chans == 2) ? avail / 2 : avail;
	if (frames < 1)
		return;
	if (frames * (size_t)chans > avail)
		frames = (chans == 2) ? avail / 2 : avail;
	if (frames < 1)
		return;

	std::vector<float> mono(frames);
	const unsigned char* p = data + 64;
	for (size_t i = 0; i < frames; i++) {
		float acc = 0.f;
		int nch = chans;
		for (int c = 0; c < nch; c++) {
			float v = 0.f;
			if (fmt == 0) {
				int16_t s;
				memcpy(&s, p, 2);
				v = s / 32768.f;
			} else if (fmt == 1) {
				int32_t s = p[0] | (p[1] << 8) | (p[2] << 16);
				if (s & 0x800000) s |= ~0xffffff;
				v = s / 8388608.f;
			} else if (fmt == 2) {
				int32_t s;
				memcpy(&s, p, 4);
				v = s / 2147483648.f;
			} else {
				memcpy(&v, p, 4);
			}
			p += bps;
			acc += v;
		}
		mono[i] = acc / (float)nch;
	}
	push_rx_samples(&mono[0], mono.size());
	static int shown = 0;
	if (++shown >= 50) {
		shown = 0;
		char msg[64];
		int rr = rate >= 8000 ? (int)rate : tci_dev_rate;
		snprintf(msg, sizeof(msg), "%s %d Hz, %d ch",
			lineout ? "TCI line-out" : "TCI RX audio", rr, chans);
		put_status(msg, 2.0);
	}
}

static void reader_loop(Socket& s, const std::string& preload)
{
	tci_last_rx_audio = 0;
	tci_lineout_logged = 0;
	time_t audio_armed_at = time(0);
	int late_audio = 0;
	std::vector<unsigned char> acc(preload.begin(), preload.end());
	unsigned char buf[8192];
	while (!tci_quit && !tci_drop_socket && progdefaults.tci_enable) {
		// ExpertSDR3 ignores audio_start during its greeting. Send it
		// again once that burst has had a second to finish. No transmit.
		if (!late_audio && time(0) - audio_armed_at >= 1) {
			late_audio = 1;
			subscribe_audio();
		}
		size_t r = 0;
		try {
			r = s.recv(buf, sizeof(buf));
		} catch (const SocketException&) {
			break;
		}
		if (r == 0) {
			MilliSleep(10);
			continue;
		}
		acc.insert(acc.end(), buf, buf + r);
		while (acc.size() >= 2 && !tci_quit) {
			size_t hlen = 2;
			unsigned char b0 = acc[0];
			unsigned char b1 = acc[1];
			uint64_t plen = b1 & 0x7f;
			if (plen == 126) {
				if (acc.size() < 4) break;
				plen = (acc[2] << 8) | acc[3];
				hlen = 4;
			} else if (plen == 127) {
				if (acc.size() < 10) break;
				plen = 0;
				for (int i = 0; i < 8; i++)
					plen = (plen << 8) | acc[2 + i];
				hlen = 10;
			}
			bool masked = (b1 & 0x80) != 0;
			if (masked) hlen += 4;
			if (plen > 8 * 1024 * 1024) {
				acc.clear();
				return;
			}
			if (acc.size() < hlen + plen)
				break;
			unsigned char mask[4] = {0,0,0,0};
			if (masked)
				memcpy(mask, &acc[hlen - 4], 4);
			std::vector<unsigned char> payload(plen);
			for (uint64_t i = 0; i < plen; i++)
				payload[i] = acc[hlen + i] ^ (masked ? mask[i & 3] : 0);
			int opcode = b0 & 0x0f;
			bool fin = (b0 & 0x80) != 0;
			acc.erase(acc.begin(), acc.begin() + hlen + plen);
			// ExpertSDR3 splits a large audio block across websocket
			// frames. The follow-up frames have opcode 0. Dropping them
			// leaves the dial working and the waterfall silent.
			static std::vector<unsigned char> ws_msg;
			static int ws_msg_op = -1;
			if (opcode == 0x0) {
				ws_msg.insert(ws_msg.end(), payload.begin(), payload.end());
				if (!fin)
					continue;
				opcode = ws_msg_op;
				payload.swap(ws_msg);
				ws_msg.clear();
				ws_msg_op = -1;
			} else if (!fin && (opcode == 0x1 || opcode == 0x2)) {
				ws_msg_op = opcode;
				ws_msg.assign(payload.begin(), payload.end());
				continue;
			}
			if (opcode == 0x8)
				return;
			if (opcode == 0x9) {
				ws_send(0xA, payload.empty() ? "" : (const char*)&payload[0], payload.size());
				continue;
			}
			if (opcode == 0x1) {
				handle_text(std::string((char*)&payload[0], payload.size()));
			} else if (opcode == 0x2) {
				handle_binary(&payload[0], payload.size());
			}
		}
	}
}

static void* tci_thread_main(void*)
{
	while (!tci_quit) {
		if (!progdefaults.tci_enable) {
			close_sock();
			set_status(_("TCI disabled"));
			MilliSleep(200);
			continue;
		}
		std::string host = progdefaults.tci_host;
		std::string port = progdefaults.tci_port;
		if (host.empty()) host = "127.0.0.1";
		if (port.empty()) port = "40001";
		set_status(std::string(_("TCI connecting to ")) + host + ":" + port);
		tci_drop_socket = false;
		Socket* s = 0;
		std::string preload;
		try {
			Address addr(host.c_str(), port.c_str(), "tcp");
			s = new Socket(addr);
			s->connect();
			if (!s->is_connected()) {
				delete s;
				set_status(_("TCI connect failed"));
				MilliSleep(2000);
				continue;
			}
			if (!handshake(*s, host, port, preload)) {
				delete s;
				set_status(_("TCI handshake failed"));
				MilliSleep(2000);
				continue;
			}
			s->set_nonblocking(true);
			s->set_timeout(0.2);
		} catch (const SocketException& e) {
			delete s;
			set_status(std::string(_("TCI: ")) + e.what());
			LOG_ERROR("TCI connect: %s", e.what());
			MilliSleep(2000);
			continue;
		}
		pthread_mutex_lock(&tci_mutex);
		tci_sock = s;
		tci_connected = true;
		pthread_mutex_unlock(&tci_mutex);
		set_status(std::string(_("TCI connected ")) + host + ":" + port);
		LOG_INFO("TCI connected %s:%s rx %d", host.c_str(), port.c_str(), progdefaults.tci_rx);
		subscribe_audio();
		{
			int rx = progdefaults.tci_rx;
			if (rx < 0) rx = 0;
			char q[160];
			snprintf(q, sizeof(q),
				"vfo:%d,0;modulation:%d;rx_filter_band:%d;modulations_list;",
				rx, rx, rx);
			ws_send_text(q);
		}
		pthread_mutex_lock(&tci_mutex);
		if (tci_modes_list.empty()) {
			tci_modes_list = "USB,LSB,DIGU,DIGL,CWU,CWL,AM,SAM,FM,NFM,DSB";
			tci_modes_dirty = true;
		}
		pthread_mutex_unlock(&tci_mutex);
		reader_loop(*s, preload);
		close_sock();
		set_status(_("TCI disconnected"));
		LOG_INFO("%s", "TCI disconnected");
		if (!tci_quit)
			MilliSleep(1000);
	}
	close_sock();
	return 0;
}

static void tci_ui_tick(void*);

void tci_start()
{
	if (!tci_thread_started) {
		Fl::add_timeout(0.5, tci_ui_tick, 0);
	}
	if (tci_thread_started)
		return;
	tci_thread_started = true;
	if (pthread_create(&tci_thread, 0, tci_thread_main, 0) != 0) {
		tci_thread_started = false;
		LOG_ERROR("%s", "TCI thread failed to start");
	}
}

void tci_set_ptt(bool on)
{
	if (!tci_is_active())
		return;
	int rx = progdefaults.tci_rx;
	char cmd[80];
	snprintf(cmd, sizeof(cmd), "trx:%d,%s,tci;", rx, on ? "true" : "false");
	pthread_mutex_lock(&tci_mutex);
	tci_ptt = on;
	tci_tx_on = on;
	if (on) {
		tci_tx_q.clear();
		tci_tx_hist = 0.f;
		tci_tx_pos = 0.0;
		tci_tx_rs_modem = 0;
	}
	pthread_mutex_unlock(&tci_mutex);
	ws_send_text(cmd);
	if (on)
		subscribe_audio();
}

void tci_set_freq(unsigned long hz)
{
	if (!tci_is_active() || hz == 0)
		return;
	int rx = progdefaults.tci_rx;
	char cmd[80];
	snprintf(cmd, sizeof(cmd), "vfo:%d,0,%lu;", rx, hz);
	pthread_mutex_lock(&tci_mutex);
	tci_last_sent_freq = hz;
	if (tci_freq != hz) {
		tci_freq = hz;
		tci_freq_dirty = true;
	}
	pthread_mutex_unlock(&tci_mutex);
	ws_send_text(cmd);
}

void tci_set_mode(const char* fldigi_mode)
{
	if (!tci_is_active() || !fldigi_mode || !*fldigi_mode)
		return;
	std::string md;
	for (const char* p = fldigi_mode; *p; p++) {
		if (*p == ' ')
			continue;
		md += (char)toupper((unsigned char)*p);
	}
	bool letters = !md.empty();
	for (size_t i = 0; i < md.size(); i++) {
		if (!isalpha((unsigned char)md[i]))
			letters = false;
	}
	if (!letters)
		md = mode_to_tci(fldigi_mode);
	int rx = progdefaults.tci_rx;
	char cmd[80];
	snprintf(cmd, sizeof(cmd), "modulation:%d,%s;", rx, md.c_str());
	pthread_mutex_lock(&tci_mutex);
	tci_last_sent_mode = md;
	tci_mode = md;
	pthread_mutex_unlock(&tci_mutex);
	ws_send_text(cmd);
	if (wf)
		wf->USB(!mode_is_lsb(md));
}

void tci_set_filter(const char* label)
{
	if (!tci_is_active() || !label || !*label)
		return;
	int lo = 0, hi = 0;
	if (sscanf(label, "%d-%d", &lo, &hi) != 2)
		return;
	if (hi < lo) {
		int t = lo;
		lo = hi;
		hi = t;
	}
	int rx = progdefaults.tci_rx;
	char cmd[96];
	snprintf(cmd, sizeof(cmd), "rx_filter_band:%d,%d,%d;", rx, lo, hi);
	pthread_mutex_lock(&tci_mutex);
	tci_filt_sent_lo = lo;
	tci_filt_sent_hi = hi;
	tci_filt_lo = lo;
	tci_filt_hi = hi;
	pthread_mutex_unlock(&tci_mutex);
	ws_send_text(cmd);
}

static void tci_show_rig_menus()
{
	if (!qso_combos)
		return;
	if (!qso_combos->visible()) {
		if (smeter) smeter->hide();
		if (pwrmeter) pwrmeter->hide();
		qso_combos->show();
		progStatus.meters = false;
	}
	if (qso_opGROUP)
		qso_opGROUP->hide();
	if (qso_opBW)
		qso_opBW->show();
	if (qso_opMODE)
		qso_opMODE->show();
}

static const char* tci_filter_presets[] = {
	"0-5000",
	"80-3000",
	"80-2000",
	"950-2050",
	"1200-1800",
	"1325-1675",
	"1450-1550",
	"1460-1540",
	0
};

static void tci_fill_mode_menu(const std::string& list, const std::string& current)
{
	if (!qso_opMODE || list.empty())
		return;
	tci_show_rig_menus();
	qso_opMODE->clear();
	std::stringstream ss(list);
	std::string item;
	while (std::getline(ss, item, ',')) {
		if (!item.empty())
			qso_opMODE->add(item.c_str());
	}
	qso_opMODE->activate();
	if (!current.empty() && qso_opMODE->find_index(current.c_str()) >= 0)
		qso_opMODE->value(current);
	else if (qso_opMODE->lsize() > 0)
		qso_opMODE->index(0);
}

static void tci_fill_filter_menu(int lo, int hi)
{
	if (!qso_opBW)
		return;
	tci_show_rig_menus();
	char live[64];
	snprintf(live, sizeof(live), "%d-%d", lo, hi);
	qso_opBW->clear();
	bool have_live = false;
	for (int i = 0; tci_filter_presets[i]; i++) {
		qso_opBW->add(tci_filter_presets[i]);
		if (strcmp(tci_filter_presets[i], live) == 0)
			have_live = true;
	}
	if (!have_live)
		qso_opBW->add(live);
	qso_opBW->activate();
	qso_opBW->tooltip(_("Receiver filter, Hz"));
	qso_opBW->value(std::string(live));
}

static void tci_ui_tick(void*)
{
	unsigned long hz = 0;
	std::string mode;
	std::string modes;
	std::string st;
	bool freq_dirty = false;
	bool mode_dirty = false;
	bool modes_dirty = false;
	bool filt_dirty = false;
	int flo = 0, fhi = 0;
	pthread_mutex_lock(&tci_mutex);
	hz = tci_freq;
	mode = tci_mode;
	modes = tci_modes_list;
	st = tci_status;
	freq_dirty = tci_freq_dirty;
	mode_dirty = tci_mode_dirty;
	modes_dirty = tci_modes_dirty;
	filt_dirty = tci_filt_dirty;
	flo = tci_filt_lo;
	fhi = tci_filt_hi;
	tci_freq_dirty = false;
	tci_mode_dirty = false;
	tci_modes_dirty = false;
	tci_filt_dirty = false;
	pthread_mutex_unlock(&tci_mutex);

	if (tci_status_box)
		tci_status_box->copy_label(st.c_str());

	if (progdefaults.tci_enable && tci_connected) {
		if (modes_dirty)
			tci_fill_mode_menu(modes, mode);
		if (mode_dirty && qso_opMODE && !mode.empty()) {
			if (qso_opMODE->find_index(mode.c_str()) >= 0)
				qso_opMODE->value(mode);
		}
		if (filt_dirty)
			tci_fill_filter_menu(flo, fhi);
	}

	if (progdefaults.tci_enable && wf) {
		if (freq_dirty && hz > 0 && (unsigned long)wf->rfcarrier() != hz) {
			wf->rfcarrier(hz);
			wf->movetocenter();
			show_frequency(hz);
		}
		if (mode_dirty && !mode.empty())
			wf->USB(!mode_is_lsb(mode));
	}
	Fl::repeat_timeout(0.2, tci_ui_tick, 0);
}

void tci_apply(bool enable, bool audio, const char* host, const char* port, int rx)
{
	bool was_audio = tci_audio_wanted();
	progdefaults.tci_enable = enable;
	progdefaults.tci_audio = audio;
	if (host && host[0])
		progdefaults.tci_host = host;
	if (port && port[0])
		progdefaults.tci_port = port;
	if (rx < 0)
		rx = 0;
	progdefaults.tci_rx = rx;
	progdefaults.saveDefaults();
	progdefaults.initInterface();
	tci_drop_socket = true;
	if (was_audio != tci_audio_wanted())
		trx_reset();
	LOG_INFO("TCI apply enable=%d audio=%d %s:%s rx=%d",
		(int)progdefaults.tci_enable, (int)progdefaults.tci_audio,
		progdefaults.tci_host.c_str(), progdefaults.tci_port.c_str(),
		progdefaults.tci_rx);
}

void tci_copy_status(char* buf, int cap)
{
	if (!buf || cap < 1)
		return;
	pthread_mutex_lock(&tci_mutex);
	std::string text = tci_status;
	pthread_mutex_unlock(&tci_mutex);
	if (text.empty())
		text = progdefaults.tci_enable ? _("TCI idle") : _("TCI disabled");
	std::snprintf(buf, (size_t)cap, "%s", text.c_str());
}

static void tci_apply_cb(Fl_Widget*, void*)
{
	tci_apply(tci_ck_en->value(), tci_ck_au->value(),
		tci_in_host->value(), tci_in_port->value(),
		atoi(tci_in_rx->value()));
}

void tci_show_dialog()
{
	if (!tci_dlg) {
		tci_dlg = new Fl_Window(420, 280, _("TCI  --  Zeus / ExpertSDR3"));
		tci_ck_en = new Fl_Check_Button(20, 16, 380, 24, _("Enable TCI rig control"));
		tci_ck_au = new Fl_Check_Button(20, 42, 380, 24, _("Use TCI for RX and TX audio (no virtual cable)"));
		tci_in_host = new Fl_Input(110, 78, 280, 24, _("Host"));
		tci_in_port = new Fl_Input(110, 108, 120, 24, _("Port"));
		tci_in_rx = new Fl_Input(110, 138, 80, 24, _("Receiver"));
		tci_status_box = new Fl_Box(20, 168, 380, 22, _("TCI idle"));
		tci_status_box->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
		Fl_Box* note = new Fl_Box(20, 192, 380, 44,
			_("Click Apply to connect. Receiver and transmitter audio use the TCI stream."));
		note->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_WRAP);
		note->labelsize(12);
		Fl_Button* apply = new Fl_Button(300, 244, 90, 26, _("Apply"));
		apply->callback(tci_apply_cb);
		tci_dlg->end();
	}
	tci_ck_en->value(progdefaults.tci_enable);
	tci_ck_au->value(progdefaults.tci_audio);
	tci_in_host->value(progdefaults.tci_host.c_str());
	tci_in_port->value(progdefaults.tci_port.c_str());
	char rxb[16];
	snprintf(rxb, sizeof(rxb), "%d", progdefaults.tci_rx);
	tci_in_rx->value(rxb);
	tci_dlg->show();
}

void cb_mnu_tci(Fl_Widget*, void*)
{
	tci_show_dialog();
}

// --- modem audio device ----------------------------------------------------

SoundTCI::SoundTCI()
{
	open_mode = 0;
	sample_frequency = 8000;
}

int SoundTCI::Open(int mode, int freq)
{
	open_mode = mode;
	if (freq < 1000)
		freq = 8000;
	sample_frequency = freq;
	return 0;
}

void SoundTCI::Close(unsigned) { }
void SoundTCI::Abort(unsigned) { Close(); }
void SoundTCI::flush(unsigned) { }

size_t SoundTCI::Read(float* buf, size_t count)
{
	int modem = sample_frequency > 0 ? sample_frequency : 8000;
	if (modem < 1000)
		modem = 8000;

	// The radio is the clock. Waiting here keeps the modem in step with
	// the samples that actually arrive. Sleeping a whole block on top of
	// that falls behind, the queue hits its cap, and the samples thrown
	// away walk PSK off its symbol clock until the copy dies.
	for (int w = 0; w < 40; w++) {
		pthread_mutex_lock(&tci_mutex);
		int dev = tci_dev_rate > 0 ? tci_dev_rate : 48000;
		size_t need = (size_t)((double)count * (double)dev / (double)modem) + 4;
		bool ready = tci_rx_q.size() >= need;
		pthread_mutex_unlock(&tci_mutex);
		if (ready)
			break;
		MilliSleep(2);
	}

	pthread_mutex_lock(&tci_mutex);
	int dev = tci_dev_rate > 0 ? tci_dev_rate : 48000;
	double step = (double)dev / (double)modem;
	if (step < 1.0)
		step = 1.0;
	size_t need = (size_t)((double)count * step) + 4;
	size_t cushion = (size_t)dev / 25;
	if (cushion < need)
		cushion = need;
	if (tci_rx_q.size() > cushion + need) {
		size_t skip = tci_rx_q.size() - cushion;
		tci_rx_q.erase(tci_rx_q.begin(), tci_rx_q.begin() + skip);
		tci_rx_phase = 0.0;
	}
	for (size_t i = 0; i < count; i++) {
		if (tci_rx_phase + 1.0 >= (double)tci_rx_q.size()) {
			buf[i] = 0.f;
			continue;
		}
		size_t idx = (size_t)tci_rx_phase;
		float frac = (float)(tci_rx_phase - (double)idx);
		float a = tci_rx_q[idx];
		float b = tci_rx_q[idx + 1];
		buf[i] = a + (b - a) * frac;
		tci_rx_phase += step;
	}
	size_t drop = (size_t)tci_rx_phase;
	if (drop > 0 && drop < tci_rx_q.size()) {
		tci_rx_q.erase(tci_rx_q.begin(), tci_rx_q.begin() + drop);
		tci_rx_phase -= (double)drop;
	} else if (drop >= tci_rx_q.size()) {
		tci_rx_q.clear();
		tci_rx_phase = 0.0;
	}
	pthread_mutex_unlock(&tci_mutex);

	return count;
}

size_t SoundTCI::Write(double* buf, size_t count)
{
	if (!buf || count == 0)
		return count;

	int modem = sample_frequency > 0 ? sample_frequency : 8000;
	pthread_mutex_lock(&tci_mutex);
	const int dev = 48000;
	if (tci_tx_rs_modem != modem) {
		tci_tx_hist = 0.f;
		tci_tx_pos = 0.0;
		tci_tx_rs_modem = modem;
	}
	// 48000 / 8000 = 6. Emit that many samples for each modem sample so a
	// 1500 Hz PSK31 tone is still 1500 Hz when Zeus plays the block at 48 kHz.
	int ratio = dev / modem;
	if (ratio >= 1 && modem * ratio == dev) {
		float prev = tci_tx_hist;
		for (size_t i = 0; i < count; i++) {
			float s = (float)buf[i];
			for (int k = 1; k <= ratio; k++) {
				float t = (float)k / (float)ratio;
				tci_tx_q.push_back(prev * (1.f - t) + s * t);
			}
			prev = s;
		}
		tci_tx_hist = prev;
		tci_tx_pos = 0.0;
	} else {
		double step = (double)modem / (double)dev;
		if (step <= 0.0)
			step = 1.0;
		double pos = tci_tx_pos;
		while (pos < (double)count) {
			float s;
			if (pos >= 0.0) {
				size_t idx = (size_t)pos;
				float frac = (float)(pos - (double)idx);
				if (idx >= count)
					break;
				if (frac > 0.f && idx + 1 >= count)
					break;
				float a = (float)buf[idx];
				float b = (idx + 1 < count) ? (float)buf[idx + 1] : a;
				s = a * (1.f - frac) + b * frac;
			} else {
				float frac = (float)(pos + 1.0);
				if (frac < 0.f) frac = 0.f;
				if (frac > 1.f) frac = 1.f;
				s = tci_tx_hist * (1.f - frac) + (float)buf[0] * frac;
			}
			tci_tx_q.push_back(s);
			pos += step;
		}
		tci_tx_pos = pos - (double)count;
		tci_tx_hist = (float)buf[count - 1];
	}
	// Stay ahead of the radio, but never cap the queue below one chrono
	// or the modem blocks forever and the radio gets silence.
	size_t hold = 8192;
	size_t queued = tci_tx_q.size();
	bool pacing = tci_tx_on;
	pthread_mutex_unlock(&tci_mutex);

	for (int spins = 0; pacing && queued > hold && spins < 400; spins++) {
		MilliSleep(5);
		pthread_mutex_lock(&tci_mutex);
		queued = tci_tx_q.size();
		pacing = tci_tx_on;
		pthread_mutex_unlock(&tci_mutex);
	}
	return count;
}

size_t SoundTCI::Write_stereo(double* left, double* right, size_t count)
{
	std::vector<double> mix(count);
	for (size_t i = 0; i < count; i++)
		mix[i] = 0.5 * (left[i] + right[i]);
	return Write(&mix[0], count);
}
