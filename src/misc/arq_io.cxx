// ----------------------------------------------------------------------------
// arq_io.cxx
//
// support for ARQ server/client system such as pskmail and fl_arq
//
// Copyright (C) 2006-2017
//		Dave Freese, W1HKJ
// Copyright (C) 2008-2013
//		Stelios Bounanos, M0GLD
// Copyright (C) 2009-2013
//		John Douyere, VK2ETA
// Copyright (c) 2013
//		Remi Chateauneu, F4ECW
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

#ifdef __MINGW32__
#  include "compat.h"
#endif

#include <fstream>
#include <sstream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <errno.h>

#include <sys/types.h>
#if !defined(__WIN32__) && !defined(__APPLE__)
#  include <sys/ipc.h>
#  include <sys/msg.h>
#endif

#include <signal.h>

#include "main.h"
#include "configuration.h"
#include "fl_digi.h"
#include "trx.h"
#include "arq_io.h"

#include "threads.h"
#include "socket.h"
#include "debug.h"
#include "qrunner.h"

#include <FL/Fl.H>
#include <FL/fl_ask.H>

LOG_FILE_SOURCE(debug::LOG_ARQCONTROL);

// =====================================================================
static pthread_t arq_thread;
static pthread_mutex_t arq_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t arq_rx_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t tosend_mutex = PTHREAD_MUTEX_INITIALIZER;

static void *arq_loop(void *args);

static bool arq_exit = false;
static bool arq_enabled;
static bool abort_flag = false;

/// Any access to shared variables must be protected.
static std::string tosend = "";   // Protected by tosend_mutex
//static std::string enroute = "";  // Protected by tosend_mutex

static std::string arqtext = "";  // Protected by arq_rx_mutex
static std::string txstring = ""; // Protected by arq_rx_mutex
bool arq_text_available = false; // Protected by arq_rx_mutex
								 // Beware 'arq_text_available' is accessed by other modules.

// =====================================================================

static const char *asc[128] = {
	"<NUL>", "<SOH>", "<STX>", "<ETX>",
	"<EOT>", "<ENQ>", "<ACK>", "<BEL>",
	"<BS>",  "<TAB>", "\n",  "<VT>",
	"<FF>",  "",  "<SO>",  "<SI>",
	"<DLE>", "<DC1>", "<DC2>", "<DC3>",
	"<DC4>", "<NAK>", "<SYN>", "<ETB>",
	"<CAN>", "<EM>",  "<SUB>", "<ESC>",
	"<FS>",  "<GS>",  "<RS>",  "<US>",
	" ",     "!",     "\"",    "#",
	"$",     "%",     "&",     "\'",
	"(",     ")",     "*",     "+",
	",",     "-",     ".",     "/",
	"0",     "1",     "2",     "3",
	"4",     "5",     "6",     "7",
	"8",     "9",     ":",     ";",
	"<",     "=",     ">",     "?",
	"@",     "A",     "B",     "C",
	"D",     "E",     "F",     "G",
	"H",     "I",     "J",     "K",
	"L",     "M",     "N",     "O",
	"P",     "Q",     "R",     "S",
	"T",     "U",     "V",     "W",
	"X",     "Y",     "Z",     "[",
	"\\",    "]",     "^",     "_",
	"`",     "a",     "b",     "c",
	"d",     "e",     "f",     "g",
	"h",     "i",     "j",     "k",
	"l",     "m",     "n",     "o",
	"p",     "q",     "r",     "s",
	"t",     "u",     "v",     "w",
	"x",     "y",     "z",     "{",
	"|",     "}",     "~",     "<DEL>"
};

std::string noctrl(std::string src)
{
	static std::string retstr;
	retstr.clear();
	char hexstr[10];
	int c;
	for (size_t i = 0; i < src.length(); i++)  {
		c = src[i];
		if ( c > 0 && c < 128)
			retstr.append(asc[c]);
		else {
			snprintf(hexstr, sizeof(hexstr), "<%0X>", c & 0xFF);
			retstr.append(hexstr);
		}
	}
	return retstr;
}

//======================================================================

extern void parse_arqtext(std::string &toparse);

static void set_button(Fl_Button* button, bool value)
{
	button->value(value);
	button->do_callback();
}

void ParseMode(std::string src)
{
LOG_INFO("%s", src.c_str());
	if ((src.find("XMTTUNE") != std::string::npos) ||
		(src.find("PTTTUNE") != std::string::npos)) {
		int msecs = 100;
		if (src.length() > 7) {
			int ret = sscanf( src.substr(7, src.length() - 7).c_str(), "%d", &msecs);
			if (ret != 1 || msecs < 10 || msecs > 20000) msecs = 100;
		}
//		if (debug_pskmail)
			LOG_INFO("%s %5.2f sec", "ARQ set ptt-tune on", msecs/1000.0);
		REQ_SYNC(&waterfall::set_XmtRcvBtn, wf, true);
		REQ_SYNC(trx_tune);
		MilliSleep(msecs);
//		if (debug_pskmail)
			LOG_INFO("%s", "ARQ set ptt-tune off");
		REQ_SYNC(&waterfall::set_XmtRcvBtn, wf, false);
		REQ_SYNC(trx_receive);
	}
	else for (size_t i = 0; i < NUM_MODES; ++i) {
		if (strlen(mode_info[i].pskmail_name) > 0) {
			if (src == mode_info[i].pskmail_name) {
				if (active_modem->get_mode() == mode_info[i].mode) {
					LOG_INFO("Active modem already set to %s", src.c_str());
				} else {
					REQ_SYNC(init_modem_sync, mode_info[i].mode, 0);
					MilliSleep(100);
//				AbortARQ();
//				if (debug_pskmail)
					LOG_INFO("Modem set to %s", mode_info[i].pskmail_name);
				}
				break;
			}
		}
	}
	WriteARQ('\002');
}

void ParseRSID(std::string src)
{
	if (src == "ON") {
//		if (debug_pskmail)
			LOG_INFO("%s", "RsID turned ON");
		REQ(set_button, btnRSID, 1);
	}
	if (src == "OFF") {
//		if (debug_pskmail)
			LOG_INFO("%s", "RsID turned OFF");
		REQ(set_button, btnRSID, 0);
	}
}


void ParseTxRSID(std::string src)
{
	if (src == "ON") {
//		if (debug_pskmail)
			LOG_INFO("%s", "TxRsID turned ON");
		REQ(set_button, btnTxRSID, 1);
	}
	if (src == "OFF") {
//		if (debug_pskmail)
			LOG_INFO("%s", "TxRsID turned OFF");
		REQ(set_button, btnTxRSID, 0);
	}
}

void parse_arqtext(std::string &toparse)
{
	static	std::string strCmdText;
	static	std::string strSubCmd;
	unsigned long int idxCmd, idxCmdEnd, idxSubCmd, idxSubCmdEnd;

	if (toparse.empty()) return;

	LOG_VERBOSE("parsing: %s", noctrl(toparse).c_str());


	idxCmd = toparse.find("<cmd>");
	idxCmdEnd = toparse.find("</cmd>");

	while ( idxCmd != std::string::npos && idxCmdEnd != std::string::npos && idxCmdEnd > idxCmd ) {
		LOG_VERBOSE("Parsing: %s", noctrl(toparse.substr(idxCmd, idxCmdEnd - idxCmd + 6)).c_str());

		strCmdText = toparse.substr(idxCmd + 5, idxCmdEnd - idxCmd - 5);
		if (strCmdText == "server" && mailserver == false && mailclient == false) {
			mailserver = true;
			mailclient = false;
			std::string PskMailLogName;
			PskMailLogName.assign(PskMailDir);
			PskMailLogName.append("gMFSK.log");
			Maillogfile = new cLogfile(PskMailLogName.c_str());
			Maillogfile->log_to_file_start();
			REQ(set_button, wf->xmtlock, 1);
			if (progdefaults.PSKmailSweetSpot)
				active_modem->set_freq(progdefaults.PSKsweetspot);
			active_modem->set_freqlock(true);
			LOG_INFO("%s", "ARQ is set to pskmail server");
		} else if (strCmdText == "client" && mailclient == false && mailserver == false) {
			mailclient = true;
			mailserver = false;
			std::string PskMailLogName;
			PskMailLogName.assign(PskMailDir);
			PskMailLogName.append("gMFSK.log");
			Maillogfile = new cLogfile(PskMailLogName.c_str());
			Maillogfile->log_to_file_start();
			REQ(set_button, wf->xmtlock, 0);
			active_modem->set_freqlock(false);
			LOG_INFO("%s", "ARQ is set to pskmail client");
		} else if (strCmdText == "normal") {
			mailserver = false;
			mailclient = false;
			if (Maillogfile) {
				delete Maillogfile;
				Maillogfile = 0;
			}
			REQ(set_button, wf->xmtlock, 0);
			active_modem->set_freqlock(false);
			LOG_INFO("%s", "ARQ is reset to normal ops");
		} else if ((idxSubCmd = strCmdText.find("<mode>")) != std::string::npos) {
			idxSubCmdEnd = strCmdText.find("</mode>");
			if (	idxSubCmdEnd != std::string::npos &&
				idxSubCmdEnd > idxSubCmd ) {
				strSubCmd = strCmdText.substr(idxSubCmd + 6, idxSubCmdEnd - idxSubCmd - 6);
				LOG_INFO("%s %s", "ARQ mode ", strSubCmd.c_str());
				ParseMode(strSubCmd);
			}
		} else if ((idxSubCmd = strCmdText.find("<rsid>")) != std::string::npos) {
			idxSubCmdEnd = strCmdText.find("</rsid>");
			if (	idxSubCmdEnd != std::string::npos &&
				idxSubCmdEnd > idxSubCmd ) {
				strSubCmd = strCmdText.substr(idxSubCmd + 6, idxSubCmdEnd - idxSubCmd - 6);
				ParseRSID(strSubCmd);
				LOG_INFO("%s %s", "ARQ rsid ", strSubCmd.c_str());
			}
		} else if ((idxSubCmd = strCmdText.find("<txrsid>")) != std::string::npos) {
			idxSubCmdEnd = strCmdText.find("</txrsid>");
			if (	idxSubCmdEnd != std::string::npos &&
				idxSubCmdEnd > idxSubCmd ) {
				strSubCmd = strCmdText.substr(idxSubCmd + 8, idxSubCmdEnd - idxSubCmd - 8);
				ParseTxRSID(strSubCmd);
				LOG_INFO("%s %s", "ARQ txrsid ", strSubCmd.c_str());
			}
		} else if (strCmdText == "abort") {
			LOG_INFO("%s", "Abort current ARQ ops");
			abort_flag = true;
		}

		toparse.erase(idxCmd, idxCmdEnd - idxCmd + 6);
		while (toparse[0] == '\n' || toparse[0] == '\r') toparse.erase(0, 1);

		idxCmd = toparse.find("<cmd>");
		idxCmdEnd = toparse.find("</cmd>");
	}
	if (!toparse.empty())
		LOG_VERBOSE("Remaining text: %s", noctrl(toparse).c_str());
}

#define TIMEOUT 180 // 3 minutes

//======================================================================
// Gmfsk ARQ file i/o used only on Linux
//======================================================================
// checkTLF
// look for files named
//	TLFfldigi ==> tlfio is true and
//			  ==> mailclient is true
// in $HOME

void checkTLF() {
	static	std::string TLFfile;
	static	std::string TLFlogname;
	std::ifstream testFile;

	tlfio = mailserver = mailclient = false;

	TLFfile.assign(PskMailDir);
	TLFfile.append("TLFfldigi");

	testFile.open(TLFfile.c_str());
	if (testFile.is_open()) {
		testFile.close();
		mailclient = true;
		tlfio = true;
		TLFlogname.assign(PskMailDir);
		TLFlogname.append("gMFSK.log");
		Maillogfile = new cLogfile(TLFlogname.c_str());
		Maillogfile->log_to_file_start();
	}
}

static bool TLF_arqRx()
{
	/// The mutex is automatically unlocked when returning.
#if defined(__WIN32__) || defined(__APPLE__)
	return false;
#else
	time_t start_time, prog_time;
	static char mailline[1000];
	static std::string sAutoFile("");
	sAutoFile.assign(PskMailDir);
	sAutoFile.append("gmfsk_autofile");

	std::ifstream autofile(sAutoFile.c_str());
	if(autofile) {
		time(&start_time);
		while (!autofile.eof()) {
			memset(mailline, 0, sizeof(mailline));
			autofile.getline(mailline, 998); // leave space for "\n" and null byte
			txstring.append(mailline);
			txstring.append("\n");
			time(&prog_time);
			if (prog_time - start_time > TIMEOUT) {
				LOG_ERROR("TLF file I/O failure");
				autofile.close();
				std::remove (sAutoFile.c_str());
				return false;
			}
		}
		autofile.close();
		std::remove (sAutoFile.c_str());

		parse_arqtext(txstring);

		if (abort_flag) {
			AbortARQ();
			abort_flag = false;
			return true;
		}

		if (!txstring.empty()) {
			guard_lock arq_rx_lock(&arq_rx_mutex);
			if (arqtext.empty()) {
				arqtext = txstring;
				if (mailserver && progdefaults.PSKmailSweetSpot)
					active_modem->set_freq(progdefaults.PSKsweetspot);
				arq_text_available = true;
				active_modem->set_stopflag(false);
				start_tx();
			} else {
				arqtext.append(txstring);
				active_modem->set_stopflag(false);
			}
			txstring.clear();
		}
	}
	return true;
#endif
}

//======================================================================
// Auto transmit of file contained in WRAP_auto_dir
//======================================================================
bool WRAP_auto_arqRx()
{
	time_t start_time, prog_time;
	static char mailline[1000];
	static std::string sAutoFile("");

	std::ifstream autofile;

	if (sAutoFile.empty()) {
		sAutoFile.assign(FLMSG_WRAP_auto_dir);
		sAutoFile.append("wrap_auto_file");
		autofile.open(sAutoFile.c_str());
		if (!autofile) {
			sAutoFile.assign(WRAP_auto_dir);
			sAutoFile.append("wrap_auto_file");
			autofile.open(sAutoFile.c_str());
		}
	} else
		autofile.open(sAutoFile.c_str());

	if(autofile) {
		/// Mutex is unlocked when leaving the block.
		guard_lock arq_rx_lock(&arq_rx_mutex);
		txstring.clear();
		time(&start_time);
		while (!autofile.eof()) {
			memset(mailline,0,1000);
			autofile.getline(mailline, 998); // leave space for "\n" and null byte
			txstring.append(mailline);
			txstring.append("\n");
			time(&prog_time);
			if (prog_time - start_time > TIMEOUT) {
				LOG_ERROR("autowrap file I/O failure");
				autofile.close();
				std::remove (sAutoFile.c_str());
				return false;
			}
		}
		autofile.close();
		std::remove (sAutoFile.c_str());

		if (!txstring.empty()) {
			arqtext.assign("\n....start\n");
			arqtext.append(txstring);
			arqtext.append("\n......end\n");
			arq_text_available = true;
			LOG_DEBUG("%s", arqtext.c_str());
			start_tx();
			txstring.clear();
			return true;
		}
	}
	return false;
}

//======================================================================
// Socket ARQ i/o used on all platforms
//======================================================================

#define ARQLOOP_TIMING 50 // 100 // msec
#define CLIENT_TIMEOUT 5 // timeout after 5 secs

struct ARQCLIENT { Socket sock; time_t keep_alive; };
static std::string errstring;

static pthread_t* arq_socket_thread = 0;
ARQ_SOCKET_Server* ARQ_SOCKET_Server::inst = 0;
static std::vector<ARQCLIENT *> arqclient; // Protected by arq_mutex

void arq_run(Socket);

ARQ_SOCKET_Server::ARQ_SOCKET_Server()
{
	server_socket = new Socket;
	arq_socket_thread = new pthread_t;
	run = true;
}

ARQ_SOCKET_Server::~ARQ_SOCKET_Server()
{
	run = false;
	if (arq_socket_thread) {
		CANCEL_THREAD(*arq_socket_thread);
		pthread_join(*arq_socket_thread, NULL);
		delete arq_socket_thread;
		arq_socket_thread = 0;
	}
	delete server_socket;
}

bool ARQ_SOCKET_Server::start(const char* node, const char* service)
{
	if (inst) return false;

	inst = new ARQ_SOCKET_Server;

	try {
		inst->server_socket->open(Address(node, service));
		inst->server_socket->bind();
#ifdef __WIN32__
		inst->server_socket->listen();
		inst->server_socket->set_timeout(0.1);
#endif
	}
	catch (const SocketException& e) {
		errstring.assign("Could not start ARQ server (");
		errstring.append(e.what()).append(")");
		if (e.error() == EADDRINUSE)
			errstring.append("\nMultiple instances of fldigi??");
		LOG_ERROR("%s", errstring.c_str());

		delete arq_socket_thread;
		arq_socket_thread = 0;
		delete inst;
		inst = 0;
		return false;
	}

	return !pthread_create(arq_socket_thread, NULL, thread_func, NULL);
}

bool server_stopped = false;

void ARQ_SOCKET_Server::stop(void)
{
	if (!inst)
		return;
	inst->run = false;
	SET_THREAD_CANCEL();
	MilliSleep(50);

#if !defined(__WOE32__) && !defined(__APPLE__)
	int timeout = 10;
	while(!server_stopped) {
		MilliSleep(100);
		Fl::awake();
		if (--timeout == 0) break;
	}
#endif

	delete inst;
	inst = 0;

}

void* ARQ_SOCKET_Server::thread_func(void*)
{
	SET_THREAD_ID(ARQSOCKET_TID);

	SET_THREAD_CANCEL();

	// On POSIX we block indefinitely and are interrupted by a signal.
	// On WIN32 we block for a short time and test for cancellation.
	while (inst->run) {
		try {
#ifdef __WIN32__
			if (inst->server_socket->wait(0))
				arq_run(inst->server_socket->accept());
#else
			arq_run(inst->server_socket->accept());
			TEST_THREAD_CANCEL();
#endif
		}
		catch (const SocketException& e) {
			if (e.error() != EINTR) {
				errstring = e.what();
				LOG_ERROR("%s", errstring.c_str());
				break;
			}
		}
		catch (...) {
			break;
		}
	}

	{
		/// Mutex is unlocked when leaving the block.
		guard_lock arq_lock(&arq_mutex);

		if (!arqclient.empty()) {
			for (std::vector<ARQCLIENT *>::iterator p = arqclient.begin();
					p < arqclient.end();
					p++) {
				(*p)->sock.close();
				arqclient.erase(p);
			}
		}
	}

	inst->server_socket->close();
	server_stopped = true;
	return NULL;
}

void arq_reset()
{
	/// Mutex is unlocked when returning from function
	guard_lock arq_rx_lock(&arq_rx_mutex);
	arqmode = mailserver = mailclient = false;
//	txstring.clear();
//	arqtext.clear();
}

void arq_run(Socket s)
{
	/// Mutex is unlocked when returning from function
	guard_lock arq_lock(&arq_mutex);
	struct timeval t = { 0, 20000 };
	s.set_timeout(t);
	s.set_nonblocking();
	ARQCLIENT *client = new ARQCLIENT;
	client->sock = s;
	client->keep_alive = time(0);
	arqclient.push_back(client);
	arqmode = true;
	std::vector<ARQCLIENT *>::iterator p = arqclient.begin();
	std::ostringstream outs;
	outs << "Clients: ";
	while (p != arqclient.end()) {
		outs << (*p)->sock.fd() << " ";
		p++;
	}
	LOG_INFO("%s", outs.str().c_str());
}

void WriteARQsocket(unsigned char* data, size_t len)
{
	/// Mutex is unlocked when returning from function
	guard_lock arq_lock(&arq_mutex);
	if (arqclient.empty()) return;
	static std::string instr;
	instr.clear();

	std::string outs = "";
	for (unsigned int i = 0; i < len; i++)
		outs += asc[data[i] & 0x7F];
	LOG_INFO("%s", outs.c_str());

	std::vector<ARQCLIENT *>::iterator p;
	for (p = arqclient.begin(); p < arqclient.end(); p++) {
		try {
			(*p)->sock.wait(1);
			(*p)->sock.send(data, len);
			(*p)->keep_alive = time(0);
			p++;
		}
		catch (const SocketException& e) {
			LOG_INFO("closing socket fd %d %s", (*p)->sock.fd(), e.what());
			try {
				(*p)->sock.close();
			} catch (const SocketException& e) {
				LOG_ERROR("Socket error on # %d, %d: %s", (*p)->sock.fd(), e.error(), e.what());
			}
			arqclient.erase(p);
		}
	}

	if (arqclient.empty()) arq_reset();
}

void test_arq_clients()
{
/// Mutex is unlocked when returning from function
	guard_lock arq_lock(&arq_mutex);
	if (arqclient.empty()) return;
	static std::string instr;
	instr.clear();
	std::vector<ARQCLIENT *>::iterator p;
	p = arqclient.begin();
	time_t now;
	size_t ret;
	while (p < arqclient.end()) {
		if (difftime(now = time(0), (*p)->keep_alive) > CLIENT_TIMEOUT) {
			try {
				(*p)->sock.wait(1);
				ret = (*p)->sock.send("\0", 1);
				if (ret <= 0) {
					LOG_INFO("closing inactive socket %d", (int)((*p)->sock.fd()));
					(*p)->sock.close();
					arqclient.erase(p); // sets p to next iterator
				} else {
					(*p)->keep_alive = now;
					p++;
				}
			}
			catch (const SocketException& e) {
				LOG_INFO("socket %d timed out, error %d, %s", (*p)->sock.fd(), e.error(), e.what());
				try {
					(*p)->sock.close();
				} catch (const SocketException& e) {
					LOG_ERROR("Socket error on # %d, %d: %s", (*p)->sock.fd(), e.error(), e.what());
				}
				arqclient.erase(p);
			}
		} else {
			p++;
		}
	}
	if (arqclient.empty()) arq_reset();
}

bool Socket_arqRx()
{
	{
		/// Mutex is unlocked when leaving block
		guard_lock arq_lock(&arq_mutex);
		if (arqclient.empty()) return false;

		static std::string instr;
		std::vector<ARQCLIENT *>::iterator p = arqclient.begin();
		size_t n = 0;
		instr.clear();
		while (p != arqclient.end()) {
			try {
				(*p)->sock.wait(0);
				while ( (n = (*p)->sock.recv(instr)) > 0) {
					txstring.append(instr);
					LOG_VERBOSE("%s", txstring.c_str());
				}
				p++;
			}
			catch (const SocketException& e) {
				txstring.clear();
				LOG_INFO("closing socket fd %d, %d: %s", (*p)->sock.fd(), e.error(), e.what());
				try {
					(*p)->sock.close();
				} catch (const SocketException& e) {
					LOG_ERROR("socket error on # %d, %d: %s", (*p)->sock.fd(), e.error(), e.what());
				}
				arqclient.erase(p);
			}
		}

		if (arqclient.empty()) arq_reset();

	}

	if (!txstring.empty()) parse_arqtext(txstring);

	if (abort_flag) {
		AbortARQ();
		abort_flag = false;
		return true;
	}

	{
		/// Mutex is unlocked when leaving block
		guard_lock arq_rx_lock(&arq_rx_mutex);

		if (txstring.empty()) return false;

		arqtext.append(txstring);

		if (mailserver && progdefaults.PSKmailSweetSpot)
			active_modem->set_freq(progdefaults.PSKsweetspot);

		if (trx_state != STATE_TX)
			start_tx();

		txstring.clear();

		arq_text_available = true;
		active_modem->set_stopflag(false);

	}
	return true;
}

//======================================================================
// Implementation using thread vice the fldigi timeout facility
//======================================================================

void WriteARQ(unsigned char data)
{
	if (active_modem->get_mode() == MODE_FSQ) return;
	guard_lock tosend_lock(&tosend_mutex);
	tosend += data;
}

void WriteARQ(const char *data)
{
	if (active_modem->get_mode() == MODE_FSQ) return;
	guard_lock tosend_lock(&tosend_mutex);
	tosend.append(data);
}

static void *arq_loop(void *args)
{
	SET_THREAD_ID(ARQ_TID);

	for (;;) {
		/* see if we are being canceled */
		if (arq_exit)
			break;

		test_arq_clients();

		{
			/// Mutex is unlocked when exiting block
			guard_lock tosend_lock(&tosend_mutex);
//			enroute.clear();
			if (!tosend.empty()) {
//				enroute = tosend;
				WriteARQsocket((unsigned char*)tosend.c_str(), tosend.length());
				tosend.clear();
			}

//			if (!enroute.empty()) {
//				WriteARQsocket((unsigned char*)enroute.c_str(), enroute.length());
//			}
		}
		if (arq_exit) break;

		// order of precedence; Socket, Wrap autofile, TLF autofile
		if (!Socket_arqRx())
			if (!WRAP_auto_arqRx())
				TLF_arqRx();

		MilliSleep(ARQLOOP_TIMING);

	}
	// exit the arq thread
	return NULL;
}

bool arq_state(void)
{
	return arq_enabled;
}

void arq_init()
{
	arq_enabled = false;

	txstring.clear();
	arqclient.clear();

	if (!ARQ_SOCKET_Server::start( progdefaults.arq_address.c_str(), progdefaults.arq_port.c_str() )) {
		arq_enabled = false;
		return;
	}

	if (pthread_create(&arq_thread, NULL, arq_loop, NULL) < 0) {
		LOG_ERROR("arq init: pthread_create failed");
		arq_enabled = false;
		return;
	}

	arq_enabled = true;
}

void arq_close(void)
{
	if (!arq_enabled) return;

	ARQ_SOCKET_Server::stop();

	// tell the arq thread to kill it self
	{
		guard_lock arqclose(&tosend_mutex);
		arq_exit = true;
	}
// and then wait for it to die
	pthread_join(arq_thread, NULL);

	arq_enabled = false;
	LOG_INFO("ARQ closed");
	if(data_io_enabled == ARQ_IO)
		data_io_enabled = DISABLED_IO ;
	arq_exit = false;

}

int arq_get_char()
{
	/// Mutex is unlocked when returning from function
	guard_lock arq_rx_lock(&arq_rx_mutex);
	int c = 0;
	if (arq_text_available) {
		if (!arqtext.empty()) {
			c = arqtext[0] & 0xFF;
			arqtext.erase(0,1);
		} else {
			arq_text_available = false;
			c = GET_TX_CHAR_ETX;
		}
	}
	return c;
}

void flush_arq_tx_buffer(void)
{
	guard_lock arq_rx_lock(&arq_rx_mutex);
    arq_text_available = false;
//    arqtext.clear();
}

//======================================================================
// following function used if the T/R button is pressed to stop a transmission
// that is servicing the ARQ text buffer.  It allows the ARQ client to reset
// itself properly
//======================================================================
void AbortARQ() {
	/// Mutex is unlocked when returning from function
	guard_lock arq_lock(&arq_rx_mutex);
	arqtext.clear();
	txstring.clear();
	arq_text_available = false;
}

//======================================================================
// Special notification for PSKMAIL: new mode marked only, in following
// format: "<DC2><Mode:newmode>", with <DC2> = '0x12'.
//======================================================================
void pskmail_notify_rsid(trx_mode mode)
{
	static char buf[64];
	memset(buf, 0, sizeof(buf));
	int n = snprintf(buf, sizeof(buf),
					 "\x12<Mode:%s>\n",
					 mode_info[mode].name);
	if (n > 0 && n < (int)sizeof(buf)) {
		WriteARQ((const char *)buf);
		REQ(&FTextBase::addstr, ReceiveText, buf, FTextBase::CTRL);
		LOG_INFO("%s", buf);
	}
}

//======================================================================
// Special notification for PSKMAIL: signal to noise measured by decoder
// format "<DC2><s2n: CC, A.a, D.d>"
// where CC = count, A.a = average s/n, D.d = Std dev of s/n
//======================================================================
void pskmail_notify_s2n(double s2n_ncount, double s2n_avg, double s2n_stddev)
{
	static char buf[64];
	memset(buf, 0, sizeof(buf));
	int n = snprintf(buf, sizeof(buf),
					 "\x12<s2n: %1.0f, %1.1f, %1.1f>\n",
					 s2n_ncount, s2n_avg, s2n_stddev);
	if (n > 0 && n < (int)sizeof(buf)) {
		WriteARQ((const char *)buf);
		REQ(&FTextBase::addstr, ReceiveText, buf, FTextBase::CTRL);
		LOG_INFO("%s", buf);
	}
}

