// ============================================================================
// arq_io.cxx
//
// support for ARQ server/client system such as pskmail and fl_arq
//
// Copyright (C) 2006
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.
//
// fldigi is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// ============================================================================


#include <config.h>

#include <fstream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <errno.h>

#include <sys/types.h>
#ifndef __CYGWIN__
#  include <sys/ipc.h>
#  include <sys/msg.h>
#endif

#include <signal.h>

#include "main.h"
#include "configuration.h"
#include "fl_digi.h"
#include "arq_io.h"

#include "threads.h"
#include "socket.h"
#include "debug.h"
#include "qrunner.h"

#include <FL/Fl.H>
#include <FL/fl_ask.H>

using namespace std;

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

static void popup_msg(void* msg)
{
	fl_message((const char*)msg);
}

void ParseMode(string src)
{
	if (src.find("PTTTUNE") != string::npos) {
		int msecs = 100;
		if (src.length() > 7)
			sscanf( src.substr(7, src.length() - 7).c_str(), "%d", &msecs);
		REQ_SYNC(&PTT::set, push2talk, true);//push2talk->set(true);
		MilliSleep(msecs);
		REQ_SYNC(&PTT::set, push2talk, false);//push2talk->set(false);
LOG_DEBUG("ARQ ptt toggled");
		return;
	}
	for (size_t i = 0; i < NUM_MODES; ++i) {
		if (strlen(mode_info[i].pskmail_name) > 0) {
			if (src == mode_info[i].pskmail_name) {
				while (trx_state != STATE_RX) MilliSleep(50);
				REQ_SYNC(init_modem_sync, mode_info[i].mode);
LOG_DEBUG("ARQ new modem set to %s", mode_info[i].pskmail_name);
				break;
			}
		}
	}
}

void parse_arqtext(string &toparse)
{
	string strCmdText;
	string strSubCmd;
	unsigned long int idxCmd, idxCmdEnd, idxSubCmd, idxSubCmdEnd;

	idxCmd = toparse.find("<cmd>");
	idxCmdEnd = toparse.find("</cmd>");
	
	if ( idxCmd != string::npos && idxCmdEnd != string::npos && idxCmdEnd > idxCmd ) {
		
LOG_DEBUG("Command string: %s", toparse.substr(idxCmd, idxCmdEnd + 6).c_str());
		strCmdText = toparse.substr(idxCmd + 5, idxCmdEnd - idxCmd - 5);
		if (strCmdText == "server" && mailserver == false && mailclient == false) {
			mailserver = true;
			mailclient = false;
			string PskMailLogName = PskMailDir;
			PskMailLogName += "gMFSK.log";
			Maillogfile = new cLogfile(PskMailLogName.c_str());
			Maillogfile->log_to_file_start();
			REQ(set_button, wf->xmtlock, 1);
			if (progdefaults.PSKmailSweetSpot)
				active_modem->set_freq(progdefaults.PSKsweetspot);
			active_modem->set_freqlock(true);
LOG_DEBUG("ARQ is set to pskmail server");
		} else if (strCmdText == "client" && mailclient == false && mailserver == false) {
			mailclient = true;
			mailserver = false;
			string PskMailLogName = PskMailDir;
			PskMailLogName += "gMFSK.log";
			Maillogfile = new cLogfile(PskMailLogName.c_str());
			Maillogfile->log_to_file_start();
			REQ(set_button, wf->xmtlock, 0);
			active_modem->set_freqlock(false);
LOG_DEBUG("ARQ is set to pskmail client");
		} else if (strCmdText == "normal") {
			mailserver = false;
			mailclient = false;
			if (Maillogfile) {
				delete Maillogfile;
				Maillogfile = 0;
			}
			REQ(set_button, wf->xmtlock, 0);
			active_modem->set_freqlock(false);
LOG_DEBUG("ARQ is reset to normal ops");
		} else {
			if ((idxSubCmd = strCmdText.find("<mode>")) != string::npos) {
				idxSubCmdEnd = strCmdText.find("</mode>");
				if (	idxSubCmdEnd != string::npos && 
						idxSubCmdEnd > idxSubCmd ) {
					strSubCmd = strCmdText.substr(idxSubCmd + 6, idxSubCmdEnd - idxSubCmd - 6);
					ParseMode(strSubCmd);
				}
			}
		}
		toparse.erase(idxCmd, idxCmdEnd - idxCmd + 6);
		if (toparse.length() == 1 && toparse[0] == '\n')
			toparse = "";
	}
}

#define TIMEOUT 180 // 3 minutes

bool bSend0x06 = false;

//-----------------------------------------------------------------------------
// SysV ARQ used only on Linux / Free-BSD or Unix type OS
//-----------------------------------------------------------------------------

#ifndef __CYGWIN__

void process_msgque()
{
	memset(txmsgst.buffer, ARQBUFSIZ, 0);
	int nbytes = msgrcv (txmsgid, (void *)&txmsgst, ARQBUFSIZ, 0, IPC_NOWAIT);
	if (nbytes > 0) { 
		txstring.append(txmsgst.buffer);
		parse_arqtext(txstring);

		if (arqtext.empty() && !txstring.empty()) {
			arqtext = txstring;
			if (mailserver && progdefaults.PSKmailSweetSpot)
				active_modem->set_freq(progdefaults.PSKsweetspot);
			pText = 0;
			arq_text_available = true;
			active_modem->set_stopflag(false);
			start_tx();
LOG_DEBUG("SYSV ARQ string: %s", txstring.c_str());
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
//    TLFfldigi ==> tlfio is true and
//              ==> mailclient is true
// in $HOME

void checkTLF() {
	string TLFfile;
	string TLFlogname;
	ifstream testFile;

	tlfio = mailserver = mailclient = false;
	
	TLFfile = PskMailDir;	
	TLFfile += "TLFfldigi";
	
	testFile.open(TLFfile.c_str());
	if (testFile.is_open()) {
		testFile.close();
		mailclient = true;
		tlfio = true;
		TLFlogname = PskMailDir;
		TLFlogname += "gMFSK.log";
		Maillogfile = new cLogfile(TLFlogname.c_str());
		Maillogfile->log_to_file_start();
	}
}

bool TLF_arqRx()
{
    time_t start_time, prog_time;
	static char mailline[1000];
    string sAutoFile = PskMailDir;
	sAutoFile += "gmfsk_autofile";

	ifstream autofile(sAutoFile.c_str());
	if(autofile) {
		arqtext = "";
        time(&start_time);
		while (!autofile.eof()) {
			memset(mailline,0,1000);
			autofile.getline(mailline, 998); // leave space for "\n" and null byte
			txstring.append(mailline);
			txstring.append("\n");
            time(&prog_time);
            if (prog_time - start_time > TIMEOUT) {
				LOG_ERROR("TLF file_i/o failure");
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
			start_tx();
LOG_DEBUG(txstring.c_str());
			txstring.clear();
		}

	}
	return true;
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
		pthread_kill(*arq_socket_thread, SIGUSR2);
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
	}
	catch (const SocketException& e) {
		errstring = "Could not start ARQ server (";
		errstring.append(e.what()).append(")");
		if (e.error() == EADDRINUSE)
			errstring.append("\nMultiple instances of fldigi??");
		LOG_ERROR("%s", errstring.c_str());
		fl_message(errstring.c_str());

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
	if (!inst)
		return;
	delete inst;
	inst = 0;
}

void* ARQ_SOCKET_Server::thread_func(void*)
{
	SET_THREAD_ID(ARQSOCKET_TID);

	{
		sigset_t usr2;
		sigemptyset(&usr2);
		sigaddset(&usr2, SIGUSR2);
		pthread_sigmask(SIG_UNBLOCK, &usr2, NULL);
	}

	while (inst->run) {
		try {
			arq_run(inst->server_socket->accept());
		}
		catch (const SocketException& e) {
			if (e.error() != EINTR) {
				errstring = e.what();
				LOG_ERROR("%s", errstring.c_str());
				Fl::add_timeout(0.0, popup_msg, (void*)errstring.c_str());
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
	isSocketConnected = false;
	arqmode = false;
}

void WriteARQsocket(unsigned int data)
{
	if (!isSocketConnected) return;
	char c = data;
	try {
		arqclient.send(&c, 1);
	}
	catch (const SocketException& e) {
		arq_stop();
	}
}

bool Socket_arqRx()
{
	if (!isSocketConnected) return false;
	
	string instr;

	try {
		size_t n = arqclient.recv(instr);
		if ( n > 0)
			txstring.append(instr);
			
		if (progdefaults.rsid == true) {
			send0x06();
			arqtext.clear();
			txstring.clear();
			cmdstring.clear();
			return true;
		}
	
		if (arqtext.empty() && !txstring.empty()) {
			arqtext = txstring;
			parse_arqtext(arqtext);
			if (!arqtext.empty()) {
				if (mailserver && progdefaults.PSKmailSweetSpot)
					active_modem->set_freq(progdefaults.PSKsweetspot);
				pText = 0;//arqtext.begin();
				arq_text_available = true;
				active_modem->set_stopflag(false);
				start_tx();
LOG_DEBUG(txstring.c_str());
			}
			txstring.clear();
		}
		cmdstring.clear();
		return true;
	}
	catch (const SocketException& e) {
		arq_stop();
		return false;
	}
}

//-----------------------------------------------------------------------------
// Send ARQ characters to ARQ client
//-----------------------------------------------------------------------------
#ifndef __CYGWIN__
void WriteARQSysV(unsigned int data)
{
	rxmsgid = msgget( (key_t) progdefaults.rx_msgid, 0666);
	if ( rxmsgid != -1) {
		rxmsgst.msg_type = 1;
		rxmsgst.c = data;
		msgsnd (rxmsgid, (void *)&rxmsgst, 1, IPC_NOWAIT);
	}
}
#endif

void WriteARQ( unsigned int data)
{
	WriteARQsocket(data);
#ifndef __CYGWIN__
	WriteARQSysV(data);
#endif
}

// following function used if the T/R button is pressed to stop a transmission
// that is servicing the ARQ text buffer.  It allows the ARQ client to reset
// itself properly

void AbortARQ() {
	if (arq_text_available) {
		arqtext.clear();
		pText = 0;//arqtext.begin();
		arq_text_available = false;
		WriteARQ(0x06);
	}
}

//-----------------------------------------------------------------------------
// Write End of Transmit character to ARQ client
//-----------------------------------------------------------------------------

void send0x06()
{
	if (trx_state == STATE_RX) {
		bSend0x06 = false;
		arq_text_available = false;
		WriteARQ(0x06);
	}
}

char arq_get_char()
{
	if (pText != arqtext.length()-1) //arqtext.end())
		return arqtext[pText++];
//		return *pText++;

	arqtext.clear();
	pText = 0;
	bSend0x06 = true;
//	arq_text_available = false;
	return 0x03; // tells psk modem to return to rx
}

// ============================================================================
// Implementation using thread vice the fldigi timeout facility
// ============================================================================
static pthread_t arq_thread;

static void *arq_loop(void *args);

static bool arq_exit = false;
static bool arq_enabled;

static void *arq_loop(void *args)
{
	SET_THREAD_ID(ARQ_TID);

	for (;;) {
	/* see if we are being canceled */
		if (arq_exit)
			break;

		if (bSend0x06)
			send0x06();

#ifdef __CYGWIN__
	Socket_arqRx();
#else
// order of precedence; Socket, SysV, GMFSKfile
		if (Socket_arqRx() == false)
			if (SysV_arqRx() == false)
				if (tlfio == true)
					TLF_arqRx();
#endif
		MilliSleep(50);
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
		fl_message("arq init: pthread_create failed");
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
