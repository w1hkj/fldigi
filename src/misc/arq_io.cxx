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

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <exception>
#include <cstdlib>
#include <ctime>
#include <errno.h>

#include <sys/types.h>
#ifndef __CYGWIN__
#  include <sys/ipc.h>
#  include <sys/msg.h>
#endif

#include "main.h"
#include "configuration.h"
#include "fl_digi.h"
#include "arq_io.h"

#include "threads.h"
#include "socket.h"

#include <FL/Fl.H>
#include <FL/fl_ask.H>

using namespace std;

static string arqtext;
string::iterator pText;

bool arq_text_available = false;

void ParseMode(string src)
{
	if (src.find("PTTTUNE") != string::npos) {
		int msecs = 100;
		if (src.length() > 7)
			sscanf( src.substr(7, src.length() - 7).c_str(), "%d", &msecs);
		push2talk->set(true);
		MilliSleep(msecs);
		push2talk->set(false);
		return;
	}
	for (size_t i = 0; i < NUM_MODES; ++i) {
		if (strlen(mode_info[i].pskmail_name) > 0) 
			if (src == mode_info[i].pskmail_name) {
				init_modem(mode_info[i].mode);
				break;
			}
	}
}

void parse_arqtext()
{
	string strCmdText;
	string strSubCmd;
	unsigned long int idxCmd, idxCmdEnd, idxSubCmd, idxSubCmdEnd;

	idxCmd = arqtext.find("<cmd>");
	idxCmdEnd = arqtext.find("</cmd>");
	
	if ( idxCmd != string::npos && idxCmdEnd != string::npos && idxCmdEnd > idxCmd ) {

		strCmdText = arqtext.substr(idxCmd + 5, idxCmdEnd - idxCmd - 5);
		if (strCmdText == "server" && mailserver == false && mailclient == false) {
			mailserver = true;
			mailclient = false;
			string PskMailLogName = PskMailDir;
			PskMailLogName += "gMFSK.log";
			Maillogfile = new cLogfile(PskMailLogName.c_str());
			Maillogfile->log_to_file_start();
		} else if (strCmdText == "client" && mailclient == false && mailserver == false) {
			mailclient = true;
			mailserver = false;
			string PskMailLogName = PskMailDir;
			PskMailLogName += "gMFSK.log";
			Maillogfile = new cLogfile(PskMailLogName.c_str());
			Maillogfile->log_to_file_start();
		} else if (strCmdText == "normal") {
			mailserver = false;
			mailclient = false;
			if (Maillogfile) {
				delete Maillogfile;
				Maillogfile = 0;
			}
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
		arqtext.erase(idxCmd, idxCmdEnd - idxCmd + 8);
		if (arqtext.length() == 1 && arqtext[0] == '\n')
			arqtext = "";
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
	int nbytes = msgrcv (txmsgid, (void *)&txmsgst, BUFSIZ, 0, IPC_NOWAIT);
	if (nbytes > 0) { 
		arqtext = txmsgst.buffer;
		parse_arqtext();
		if (arqtext.length() > 0) {
			if (mailserver && progdefaults.PSKmailSweetSpot)
				active_modem->set_freq(progdefaults.PSKsweetspot);

			pText = arqtext.begin();
			arq_text_available = true;

			active_modem->set_stopflag(false);

			start_tx();
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

bool Gmfsk_arqRx()
{
    time_t start_time, prog_time;
    string sAutoFile = PskMailDir;
	static char mailline[1000];

    if (! (mailserver || mailclient) )
    	return false;
    		 
    if (gmfskmail == true)
		sAutoFile += "gmfsk_autofile";
	else
	    sAutoFile += "pskmail_out";

	ifstream autofile(sAutoFile.c_str());
	if(autofile) {
		arqtext = "";
        time(&start_time);
		while (!autofile.eof()) {
			memset(mailline,0,1000);
			autofile.getline(mailline, 998); // leave space for "\n" and null byte
			arqtext.append(mailline);
			arqtext.append("\n");
            FL_AWAKE();
            time(&prog_time);
            if (prog_time - start_time > TIMEOUT) {
                std::cout << "pskmail_out failure" << std::endl;
                std::cout.flush();
                autofile.close();
                std::remove (sAutoFile.c_str());
                return false;
            }
		}
		autofile.close();
		std::remove (sAutoFile.c_str());
		
		parse_arqtext();
		if (arqtext.length() > 0) {
			if (mailserver && progdefaults.PSKmailSweetSpot)
				active_modem->set_freq(progdefaults.PSKsweetspot);

			pText = arqtext.begin();
			arq_text_available = true;

			active_modem->set_stopflag(false);
			start_tx();
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
// Socket ARQ i/o used on all platforms
// Socket implementation emulates the MultiPsk socket i/o
//-----------------------------------------------------------------------------
#define MPSK_TX   "TX"
#define MPSK_RX   "RX"
#define MPSK_TX2RX "RX_AFTER_TX"
#define MPSK_BYTE 25
#define MPSK_CMD  26
#define MPSK_END  27
#define MPSK_ISTX 28
#define MPSK_ISRX 29
#define MPSK_ISCMD 30
#define MPSK_CMDEND 31

extern void arq_run(Socket s);
extern void arq_stop();

string txstring = "";
string cmdstring = "";
string response;
bool isTxChar = false;
bool isCmdChar = false;
bool processCmd = false;

static Fl_Thread* arq_socket_thread;
ARQ_SOCKET_Server* ARQ_SOCKET_Server::inst = 0;

Socket arqclient;
bool isSocketConnected = false;

ARQ_SOCKET_Server::ARQ_SOCKET_Server()
{
	server_socket = new Socket;
	arq_socket_thread = new Fl_Thread;
	run = true;
}

ARQ_SOCKET_Server::~ARQ_SOCKET_Server()
{
	run = false;
	pthread_join(*arq_socket_thread, NULL);
	delete arq_socket_thread;
}

bool ARQ_SOCKET_Server::start(const char* node, const char* service)
{
	if (inst)
		return false;

	inst = new ARQ_SOCKET_Server;

	try {
		inst->server_socket->open(Address(node, service));
	}
	catch (const SocketException& e) {
		string errstr = "Could not start ARQ server (";
		errstr.append(e.what());
		errstr.append(")");
		cerr << errstr << "\n";
		fl_message(errstr.c_str());
		delete inst;
		inst = 0;
		return false;
	}

	try {
		inst->server_socket->bind();
		struct timeval t = { 0, 200000 };
		inst->server_socket->set_timeout(t);
		inst->server_socket->set_nonblocking();
	}
	catch (const SocketException& e) {
		string errstr = "Could not start ARQ server (";
		errstr.append(e.what());
		errstr.append(")");
		errstr.append("\nMultiple instance of fldigi ??");
		cerr << errstr << "\n";
		fl_message(errstr.c_str());
		return false;
	}

	fl_create_thread(*arq_socket_thread, thread_func, NULL);
	return true;
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

	while (inst->run) {
		try {
			arq_run(inst->server_socket->accept());
		}
		catch (const SocketException& e) {
			if (e.error() != ETIMEDOUT) {
				string errstr = e.what();
				cerr << errstr << "\n";
				fl_message(errstr.c_str());
				break;
			}
		}
	}
	arq_stop();
	inst->server_socket->close();
	return NULL;
}

void arq_run(Socket s)
{
	struct timeval t = { 0, 20000 }; // 0.02 second timeout
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
	
	string response;

	if (data == 0x06) {
		response = MPSK_ISCMD;
		response.append("RX_AFTER_TX OK");
		response += MPSK_CMDEND;
	} else {
		response = MPSK_ISRX;
		response += data;
	}
	try {
		arqclient.send(response);
	}
	catch (const SocketException& e) {
		arq_stop();
	}
}

bool Socket_arqRx()
{
	if (!isSocketConnected) return false;
	
	string instr;
	char cs;

	try {
		size_t n = arqclient.recv(instr);

		if ( n == 0) return false;
	
		for (size_t i = 0; i < n; i++) {
			cs = instr[i];
			if (isTxChar) {
				txstring += cs;
				isTxChar = false;
				continue;
			}
			if (isCmdChar) {
				if (cs == MPSK_END)
					isCmdChar = false;
				cmdstring += cs;
				continue;
			}
			if (cs == MPSK_BYTE) {
				isTxChar = true;
				continue;
			}
			if (cs == MPSK_CMD) {
				isCmdChar = true;
				continue;
			}
		}

		if (arqtext.empty()) {
			arqtext = txstring;
			pText = arqtext.begin();
			arq_text_available = true;
			active_modem->set_stopflag(false);
			start_tx();
		} else {
			arqtext.append(txstring);
		}
		txstring.clear();
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

char arq_get_char()
{
	if (pText != arqtext.end())
		return *pText++;

	arqtext.clear();
	bSend0x06 = true;
	arq_text_available = false;
	return 0x03; // tells psk modem to return to rx
}

// ============================================================================
// Implementation using thread vice the fldigi timeout facility
// ============================================================================

static Fl_Thread arq_thread;

static void *arq_loop(void *args);

static bool arq_exit = false;
static bool arq_enabled;
static int	arq_dummy;

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
				Gmfsk_arqRx();
#endif
// delay for 50 msec interval
		MilliSleep(50);
	}
// exit the arq thread
	return NULL;
}

void arq_init()
{
	arq_enabled = false;
	
	if (!ARQ_SOCKET_Server::start( progdefaults.arq_address.c_str(), progdefaults.arq_port.c_str() ))
		return;

	if (fl_create_thread(arq_thread, arq_loop, &arq_dummy) < 0) {
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
	fl_join(arq_thread);
	arq_enabled = false;

	arq_exit = false;
}
