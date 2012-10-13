// ----------------------------------------------------------------------------
// arq_io.cxx
//
// support for ARQ server/client system such as pskmail and fl_arq
//
// Copyright (C) 2006-2010
//		Dave Freese, W1HKJ
// Copyright (C) 2008-2010
//		Stelios Bounanos, M0GLD
// Copyright (C) 2009-2010
//		John Douyere, VK2ETA
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
#include <string>
#include <cstdlib>
#include <ctime>
#include <errno.h>

#include <sys/types.h>
#if !defined(__WOE32__) && !defined(__APPLE__)
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

using namespace std;

//======================================================================
// test code for pskmail eol issues

static const char *asc[128] = {
	"<NUL>", "<SOH>", "<STX>", "<ETX>",
	"<EOT>", "<ENQ>", "<ACK>", "<BEL>",
	"<BS>",  "<TAB>", "<LF>",  "<VT>", 
	"<FF>",  "<CR>",  "<SO>",  "<SI>",
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

string noctrl(string src)
{
	static string retstr;
	retstr.clear();
	for (size_t i = 0; i < src.length(); i++) retstr.append(asc[(int)src[i] & 0x7F]);
	return retstr;
}

//======================================================================

static string arqtext;
//string::iterator pText;
size_t pText;

bool arq_text_available = false;
string txstring;

extern void send0x06();
extern void parse_arqtext(string &toparse);

static void set_button(Fl_Button* button, bool value)
{
	button->value(value);
	button->do_callback();
}

void ParseMode(string src)
{
	if (src.find("XMTTUNE") != string::npos) {
		int msecs = 100;
		if (src.length() > 7) {
			int ret = sscanf( src.substr(7, src.length() - 7).c_str(), "%d", &msecs);
			if (ret != 1 || msecs < 10 || msecs > 20000) msecs = 100;
		}
LOG_INFO("%s %5.2f sec", "ARQ tune on", msecs/1000.0);
		trx_tune();
		MilliSleep(msecs);
LOG_INFO("%s", "ARQ tune off");
		trx_receive();
		return;
	}
	if (src.find("PTTTUNE") != string::npos) {
		int msecs = 100;
		if (src.length() > 7) {
			int ret = sscanf( src.substr(7, src.length() - 7).c_str(), "%d", &msecs);
			if (ret != 1 || msecs < 10 || msecs > 20000) msecs = 100;
		}
LOG_INFO("%s %5.2f sec", "ARQ set ptt on", msecs/1000.0);
		push2talk->set(true);
		REQ(&waterfall::set_XmtRcvBtn, wf, true);
		MilliSleep(msecs);
LOG_INFO("%s", "ARQ set ptt off");
		push2talk->set(false);
		REQ(&waterfall::set_XmtRcvBtn, wf, false);
		return;
	}
	for (size_t i = 0; i < NUM_MODES; ++i) {
		if (strlen(mode_info[i].pskmail_name) > 0) {
			if (src == mode_info[i].pskmail_name) {
				while (trx_state != STATE_RX) {
					MilliSleep(10);
				}
LOG_INFO("Setting modem to %s", mode_info[i].pskmail_name);
				REQ_SYNC(init_modem_sync, mode_info[i].mode, 0);
				break;
			}
		}
	}
}

void ParseRSID(string src)
{
	if (src == "ON") {
		LOG_VERBOSE("%s", "RsID turned ON");
		REQ(set_button, btnRSID, 1);
	}
	if (src == "OFF") {
		LOG_VERBOSE("%s", "RsID turned OFF");
		REQ(set_button, btnRSID, 0);
	}
}


void ParseTxRSID(string src)
{
	if (src == "ON") {
		LOG_VERBOSE("%s", "TxRsID turned ON");
		REQ(set_button, btnTxRSID, 1);
	}
	if (src == "OFF") {
		LOG_VERBOSE("%s", "TxRsID turned OFF");
		REQ(set_button, btnTxRSID, 0);
	}
}

void parse_arqtext(string &toparse)
{
static	string strCmdText;
static	string strSubCmd;
	unsigned long int idxCmd, idxCmdEnd, idxSubCmd, idxSubCmdEnd;

	LOG_DEBUG("Arq text: %s", noctrl(toparse).c_str());

	idxCmd = toparse.find("<cmd>");
	idxCmdEnd = toparse.find("</cmd>");

	while ( idxCmd != string::npos && idxCmdEnd != string::npos && idxCmdEnd > idxCmd ) {

		LOG_DEBUG("Cmd text: %s", toparse.substr(idxCmd, idxCmdEnd + 6).c_str());
		strCmdText = toparse.substr(idxCmd + 5, idxCmdEnd - idxCmd - 5);
		if (strCmdText == "server" && mailserver == false && mailclient == false) {
			mailserver = true;
			mailclient = false;
			string PskMailLogName;
			PskMailLogName.assign(PskMailDir);
			PskMailLogName.append("gMFSK.log");
			Maillogfile = new cLogfile(PskMailLogName.c_str());
			Maillogfile->log_to_file_start();
			REQ(set_button, wf->xmtlock, 1);
			if (progdefaults.PSKmailSweetSpot)
				active_modem->set_freq(progdefaults.PSKsweetspot);
			active_modem->set_freqlock(true);
			LOG_VERBOSE("%s", "ARQ is set to pskmail server");
		} else if (strCmdText == "client" && mailclient == false && mailserver == false) {
			mailclient = true;
			mailserver = false;
			string PskMailLogName;
			PskMailLogName.assign(PskMailDir);
			PskMailLogName.append("gMFSK.log");
			Maillogfile = new cLogfile(PskMailLogName.c_str());
			Maillogfile->log_to_file_start();
			REQ(set_button, wf->xmtlock, 0);
			active_modem->set_freqlock(false);
			LOG_VERBOSE("%s", "ARQ is set to pskmail client");
		} else if (strCmdText == "normal") {
			mailserver = false;
			mailclient = false;
			if (Maillogfile) {
				delete Maillogfile;
				Maillogfile = 0;
			}
			REQ(set_button, wf->xmtlock, 0);
			active_modem->set_freqlock(false);
			LOG_VERBOSE("%s", "ARQ is reset to normal ops");
		} else if ((idxSubCmd = strCmdText.find("<mode>")) != string::npos) {
			idxSubCmdEnd = strCmdText.find("</mode>");
			if (	idxSubCmdEnd != string::npos &&
					idxSubCmdEnd > idxSubCmd ) {
				strSubCmd = strCmdText.substr(idxSubCmd + 6, idxSubCmdEnd - idxSubCmd - 6);
				ParseMode(strSubCmd);
				LOG_VERBOSE("%s %s", "ARQ mode ", strSubCmd.c_str());
			}
		} else if ((idxSubCmd = strCmdText.find("<rsid>")) != string::npos) {
			idxSubCmdEnd = strCmdText.find("</rsid>");
			if (	idxSubCmdEnd != string::npos &&
					idxSubCmdEnd > idxSubCmd ) {
				strSubCmd = strCmdText.substr(idxSubCmd + 6, idxSubCmdEnd - idxSubCmd - 6);
				ParseRSID(strSubCmd);
				LOG_VERBOSE("%s %s", "ARQ rsid ", strSubCmd.c_str());
			}
		} else if ((idxSubCmd = strCmdText.find("<txrsid>")) != string::npos) {
			idxSubCmdEnd = strCmdText.find("</txrsid>");
			if (	idxSubCmdEnd != string::npos &&
					idxSubCmdEnd > idxSubCmd ) {
				strSubCmd = strCmdText.substr(idxSubCmd + 8, idxSubCmdEnd - idxSubCmd - 8);
				ParseTxRSID(strSubCmd);
				LOG_VERBOSE("%s %s", "ARQ txrsid ", strSubCmd.c_str());
			}
		}

		toparse.erase(idxCmd, idxCmdEnd - idxCmd + 6);
		while (toparse[0] == '\n' || toparse[0] == '\r') toparse.erase(0, 1);

		idxCmd = toparse.find("<cmd>");
		idxCmdEnd = toparse.find("</cmd>");
	}
}

#define TIMEOUT 180 // 3 minutes

bool bSend0x06 = false;

//-----------------------------------------------------------------------------
// SysV ARQ used only on Linux / Free-BSD or Unix type OS
//-----------------------------------------------------------------------------

#if !defined(__WOE32__) && !defined(__APPLE__)
void process_msgque()
{
	memset(txmsgst.buffer, 0, ARQBUFSIZ);
	int nbytes = msgrcv (txmsgid, (void *)&txmsgst, ARQBUFSIZ, 0, IPC_NOWAIT);
	if (nbytes > 0) {
		txstring.append(txmsgst.buffer);
		parse_arqtext(txstring);

		if (!bSend0x06 && arqtext.empty() && !txstring.empty()) {
			arqtext = txstring;
			if (mailserver && progdefaults.PSKmailSweetSpot)
				active_modem->set_freq(progdefaults.PSKsweetspot);
			pText = 0;
			arq_text_available = true;
			active_modem->set_stopflag(false);
			LOG_DEBUG("SYSV ARQ string: %s", arqtext.c_str());
			start_tx();
			txstring.clear();
		}
	}
}

bool SysV_arqRx()
{
   	txmsgid = msgget( (key_t) progdefaults.tx_msgid, 0666 );
   	if (txmsgid != -1) {
   		process_msgque();
   		return true;
   	}
	return false;
}
#endif

//-----------------------------------------------------------------------------
// Gmfsk ARQ file i/o used only on Linux
//-----------------------------------------------------------------------------
// checkTLF
// look for files named
//	TLFfldigi ==> tlfio is true and
//			  ==> mailclient is true
// in $HOME

void checkTLF() {
static	string TLFfile;
static	string TLFlogname;
	ifstream testFile;

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

bool TLF_arqRx()
{
	time_t start_time, prog_time;
	static char mailline[1000];
	static string sAutoFile;
	sAutoFile.assign(PskMailDir);
	sAutoFile.append("gmfsk_autofile");

	ifstream autofile(sAutoFile.c_str());
	if(autofile) {
		arqtext = "";
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
		if (arqtext.empty() && !txstring.empty()) {
			arqtext = txstring;
			if (mailserver && progdefaults.PSKmailSweetSpot)
				active_modem->set_freq(progdefaults.PSKsweetspot);
			pText = 0;
			arq_text_available = true;
			active_modem->set_stopflag(false);
LOG_INFO("%s", arqtext.c_str());
			start_tx();
			txstring.clear();
		}

	}
	return true;
}

//-----------------------------------------------------------------------------
// Auto transmit of file contained in WRAP_auto_dir
//-----------------------------------------------------------------------------
bool WRAP_auto_arqRx()
{
	time_t start_time, prog_time;
	static char mailline[1000];
	static string sAutoFile;
	sAutoFile.assign(FLMSG_WRAP_auto_dir);
	sAutoFile.append("wrap_auto_file");

	ifstream autofile;
	autofile.open(sAutoFile.c_str());
	if (!autofile) {
		sAutoFile.assign(WRAP_auto_dir);
		sAutoFile.append("wrap_auto_file");
		autofile.open(sAutoFile.c_str());
	}
	if(autofile) {
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
			pText = 0;
			arq_text_available = true;
			LOG_DEBUG("%s", arqtext.c_str());
			start_tx();
			txstring.clear();
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// Socket ARQ i/o used on all platforms
//-----------------------------------------------------------------------------

extern void arq_run(Socket s);
extern void arq_stop();

string errstring;
string cmdstring;
string response;
bool isTxChar = false;
bool isCmdChar = false;

bool isNotMULTIPSK = true;

static pthread_t* arq_socket_thread = 0;
ARQ_SOCKET_Server* ARQ_SOCKET_Server::inst = 0;

Socket arqclient;
bool isSocketConnected = false;

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
	if (inst)
		return false;

	inst = new ARQ_SOCKET_Server;

	try {
		inst->server_socket->open(Address(node, service));
		inst->server_socket->bind();
#ifdef __WOE32__
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

void ARQ_SOCKET_Server::stop(void)
{
// FILEME - uncomment when we have an ARQ_SOCKET_Server than can be
// interrupted
//	if (!inst)
//		return;
//	delete inst;
//	inst = 0;
}

void* ARQ_SOCKET_Server::thread_func(void*)
{
	SET_THREAD_ID(ARQSOCKET_TID);

	SET_THREAD_CANCEL();

	// On POSIX we block indefinitely and are interrupted by a signal.
	// On woe32 we block for a short time and test for cancellation.
	while (inst->run) {
		try {
#ifdef __WOE32__
			if (inst->server_socket->wait(0))
#endif
				arq_run(inst->server_socket->accept());
			TEST_THREAD_CANCEL();
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
	arq_stop();
	inst->server_socket->close();
	return NULL;
}

void arq_run(Socket s)
{
	struct timeval t = { 0, 20000 };
	arqclient = s;
	arqclient.set_timeout(t);
	arqclient.set_nonblocking();
	isSocketConnected = true;
	arqmode = true;
}

void arq_stop()
{
	arqclient.close();
	isSocketConnected = false;
	arqmode = false;
}

void WriteARQsocket(unsigned char* data, size_t len)
{
static string instr;
	instr.clear();
	try {
		size_t n = arqclient.recv(instr);
		if ( n > 0) txstring.append(instr);
	} catch (const SocketException& e) {
		arq_stop();
		return;
	}
	if (!isSocketConnected) return;
	try {
		arqclient.send(data, len);
	}
	catch (const SocketException& e) {
		LOG_ERROR("%s", e.what());
		arq_stop();
	}
}

bool Socket_arqRx()
{
	if (!isSocketConnected) return false;

	static string instr;
	instr.clear();

	try {
		size_t n = arqclient.recv(instr);
		if ( n > 0)
			txstring.append(instr);

		if (!bSend0x06 && arqtext.empty() && !txstring.empty()) {
			arqtext = txstring;
			parse_arqtext(arqtext);
			if (!arqtext.empty()) {
				if (mailserver && progdefaults.PSKmailSweetSpot)
					active_modem->set_freq(progdefaults.PSKsweetspot);
				pText = 0;//arqtext.begin();
				arq_text_available = true;
				active_modem->set_stopflag(false);
				LOG_DEBUG("%s", arqtext.c_str());
				start_tx();
			}
			txstring.clear();
			cmdstring.clear();
			return true;
		}
		cmdstring.clear();
		return false;
	}
	catch (const SocketException& e) {
		arq_stop();
		return false;
	}
}

//-----------------------------------------------------------------------------
// Send ARQ characters to ARQ client
//-----------------------------------------------------------------------------
#if !defined(__WOE32__) && !defined(__APPLE__)
void WriteARQSysV(unsigned char data)
{
	rxmsgid = msgget( (key_t) progdefaults.rx_msgid, 0666);
	if ( rxmsgid != -1) {
		rxmsgst.msg_type = 1;
		rxmsgst.c = data;
		msgsnd (rxmsgid, (void *)&rxmsgst, 1, IPC_NOWAIT);
	}
}
#endif

//-----------------------------------------------------------------------------
// Write End of Transmit character to ARQ client
//-----------------------------------------------------------------------------

void send0x06()
{
	if (trx_state == STATE_RX) {
		bSend0x06 = false;
		WriteARQ(0x06);
	}
}

// ============================================================================
// Implementation using thread vice the fldigi timeout facility
// ============================================================================
static pthread_t arq_thread;
static pthread_mutex_t arq_mutex = PTHREAD_MUTEX_INITIALIZER;

static void *arq_loop(void *args);

static bool arq_exit = false;
static bool arq_enabled;

string tosend = "";
string enroute = "";

void WriteARQ(unsigned char data)
{
	pthread_mutex_lock (&arq_mutex);
	tosend += data;
	pthread_mutex_unlock (&arq_mutex);
	return;

//	WriteARQsocket(&data, 1);
//#if !defined(__WOE32__) && !defined(__APPLE__)
//	WriteARQSysV(data);
//#endif
}

static void *arq_loop(void *args)
{
	SET_THREAD_ID(ARQ_TID);

	for (;;) {
	/* see if we are being canceled */
		if (arq_exit)
			break;

		pthread_mutex_lock (&arq_mutex);

		if (!tosend.empty()) {
			enroute = tosend;
			tosend.clear();
			pthread_mutex_unlock (&arq_mutex);
			WriteARQsocket((unsigned char*)enroute.c_str(), enroute.length());
#if !defined(__WOE32__) && !defined(__APPLE__)
			for (size_t i = 0; i < enroute.length(); i++)
				WriteARQSysV((unsigned char)enroute[i]);
#endif
		} else
			pthread_mutex_unlock (&arq_mutex);

		if (bSend0x06)
			send0x06();

#if !defined(__WOE32__) && !defined(__APPLE__)
		// order of precedence; Socket, Wrap autofile, TLF autofile
		if (!Socket_arqRx())
			if (!SysV_arqRx())
				WRAP_auto_arqRx();
				if (!WRAP_auto_arqRx())
					TLF_arqRx();
#else
		if (!Socket_arqRx())
			WRAP_auto_arqRx();
#endif
//		pthread_mutex_unlock (&arq_mutex);
		MilliSleep(100);

	}
// exit the arq thread
	return NULL;
}

void arq_init()
{
	arq_enabled = false;

	txstring.clear();
	cmdstring.clear();

	if (!ARQ_SOCKET_Server::start( progdefaults.arq_address.c_str(), progdefaults.arq_port.c_str() ))
		return;

	if (pthread_create(&arq_thread, NULL, arq_loop, NULL) < 0) {
		LOG_ERROR("arq init: pthread_create failed");
		return;
	}

	arq_enabled = true;
}

void arq_close(void)
{
	if (!arq_enabled) return;

	ARQ_SOCKET_Server::stop();

// tell the arq thread to kill it self
	arq_exit = true;

// and then wait for it to die
	pthread_join(arq_thread, NULL);
	arq_enabled = false;

	arq_exit = false;
}

char arq_get_char()
{
	char c = 0;
	pthread_mutex_lock (&arq_mutex);
	if (arq_text_available) {
		if (pText != arqtext.length())
			c = arqtext[pText++];
		else {
			arqtext.clear();
			pText = 0;
			bSend0x06 = true;
			arq_text_available = false;
			c = 0x03;
		}
	}
	pthread_mutex_unlock (&arq_mutex);
	return c;
}

// following function used if the T/R button is pressed to stop a transmission
// that is servicing the ARQ text buffer.  It allows the ARQ client to reset
// itself properly

void AbortARQ() {
	pthread_mutex_lock (&arq_mutex);
	arqtext.clear();
	pText = 0;
	arq_text_available = false;
	bSend0x06 = true;
	pthread_mutex_unlock (&arq_mutex);
}

// Special notification for PSKMAIL: new mode marked only, in following
// format: "<DC2><Mode:newmode>", with <DC2> = '0x12'.
void pskmail_notify_rsid(trx_mode mode)
{
	char buf[64];
	int n = snprintf(buf, sizeof(buf), "%c<Mode:%s>\n", 0x12, mode_info[mode].name);
	if (n > 0 && n < (int)sizeof(buf)) {
		WriteARQsocket((unsigned char*)buf, n);
#if !defined(__WOE32__) && !defined(__APPLE__)
		for (int ii=0; ii < n; ii++)
			WriteARQSysV((unsigned char)buf[ii]);
#endif
		ReceiveText->addstr(buf, FTextBase::CTRL);
	}
}

// Special notification for PSKMAIL: signal to noise measured by decoder
// format "<DC2><s2n: CC, A.a, D.d>"
// where CC = count, A.a = average s/n, D.d = Std dev of s/n
void pskmail_notify_s2n(double s2n_ncount, double s2n_avg, double s2n_stddev)
{
	char buf[64];
	int n = snprintf(buf, sizeof(buf), "%c<s2n: %1.0f, %1.1f, %1.1f>",
			 0x12, s2n_ncount, s2n_avg, s2n_stddev);
	if (n > 0 && n < (int)sizeof(buf)) {
		WriteARQsocket((unsigned char*)buf, n);
#if !defined(__WOE32__) && !defined(__APPLE__)
		for (int ii=0; ii < n; ii++)
			WriteARQSysV((unsigned char)buf[ii]);
#endif
		ReceiveText->addstr(buf, FTextBase::CTRL);
	}
}

