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
#include <ctime>
#include <sys/types.h>
#ifndef __CYGWIN__
#  include <sys/ipc.h>
#  include <sys/msg.h>
#endif

#include "main.h"
#include "configuration.h"
#include "fl_digi.h"

#include "threads.h"

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

//-----------------------------------------------------------------------------
// SysV ARQ used only on Linux / Free-BSD or Unix type OS
//-----------------------------------------------------------------------------

bool SysV_arqRx()
{
#ifdef __CYGWIN__
	return false;
#else
   	txmsgid = msgget( (key_t) progdefaults.tx_msgid, 0666 );
   	if (txmsgid != -1) {
   		process_msgque();
   		arqmode = true;
   		return true;
   	}
	return false;
#endif
}

//-----------------------------------------------------------------------------
// File ARQ used on Windows OS
//-----------------------------------------------------------------------------

string str_dirname = "c:/NBEMS/";
string str_infile = "c:/NBEMS/ARQXFR/txfile";
string str_outfile = "c:/NBEMS/ARQXFR/rxfile";
string str_flarqon = "c:/NBEMS/ARQXFR/flarqON";

long infileptr = 0;

void initFilePtr()
{
	FILE *infile;
	infile = fopen(str_infile.c_str(), "rb");
	if (infile) {
		fseek(infile, 0, SEEK_END);
		infileptr = ftell(infile);
		fclose(infile);
	}
}

bool File_arqRx() 
{
	FILE *infile, *testarq;
	testarq = fopen(str_flarqon.c_str(), "r");
	arqmode = false;
	if (!testarq)
		return false;
	fclose(testarq);

	arqmode = true;
	infile = fopen(str_infile.c_str(), "rb");
	if (infile) {
		fseek(infile, 0, SEEK_END);
		long sz = ftell(infile);
		if (sz < infileptr)
			infileptr = 0; // txfile was probably deleted & restarted
		if (sz > infileptr) {
			arqtext = "";
			fseek(infile, infileptr, SEEK_SET);
				while (infileptr < sz) {
					arqtext += fgetc(infile);
					infileptr++;
				}
			if (arqtext.length() > 0) {
				parse_arqtext();
				if (arqtext.length() > 0) {
					pText = arqtext.begin();
					arq_text_available = true;
					active_modem->set_stopflag(false);
					start_tx();
				}
			}
		}
		fclose(infile);
	}
	return true;
}

string holdbuffer;
bool	havedir = false;

void trywrite(void *)
{
	if (holdbuffer.empty()) return;
	FILE *outfile;
	outfile = fopen(str_outfile.c_str(), "ab");
	if (outfile) {
		fputs(holdbuffer.c_str(), outfile);
		holdbuffer.clear();
		fclose(outfile);
		return;
	}
	Fl::repeat_timeout(0.1, trywrite);
}


void writeToARQfile(unsigned int data)
{
	FILE *outfile;
	if (!havedir) {
		DIR *dir = opendir(str_dirname.c_str());
		if (dir == 0)
			return;
		else {
			closedir(dir);
			havedir = true;
		}
	}
			
	outfile = fopen(str_outfile.c_str(), "ab");
	if (outfile) {
		if (!holdbuffer.empty()) {
			fputs(holdbuffer.c_str(), outfile);
			holdbuffer.clear();
		}
		putc((unsigned char)data, outfile );
		fclose(outfile);
	} else {
		holdbuffer += (unsigned char) data;
		trywrite(NULL);
	}
}

//-----------------------------------------------------------------------------
// Gmfsk ARQ file i/o used only on Linux
//-----------------------------------------------------------------------------

bool Gmfsk_arqRx()
{
    time_t start_time, prog_time;
    string sAutoFile = PskMailDir;
	static char mailline[1000];

	arqmode = false;
    
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
#define _host "127.0.0.1"
#define _port 3122

#define _TX   "TX"
#define _RX   "RX"
#define _TX2RX "RX_AFTER_TX"
#define _TXCH 25
#define _RXCH 29
#define _TXECHO 28
#define _CMD  26
#define _END  27

void WriteARQsocket(unsigned int data)
{
}

bool Socket_arqRx()
{
	return false;
}

//-----------------------------------------------------------------------------
// Send ARQ characters to ARQ client
//-----------------------------------------------------------------------------
void WriteARQSysV(unsigned int data)
{
	rxmsgid = msgget( (key_t) progdefaults.rx_msgid, 0666);
	if ( rxmsgid != -1) {
		rxmsgst.msg_type = 1;
		rxmsgst.c = data;
		msgsnd (rxmsgid, (void *)&rxmsgst, 1, IPC_NOWAIT);
	}
}

void WriteARQ( unsigned int data)
{
	WriteARQsocket(data);
#ifndef __CYGWIN__
	WriteARQSysV(data);
#endif
#ifdef __CYGWIN__
	WriteARQfile(data);
#endif
}

//-----------------------------------------------------------------------------
// Write End of Transmit character to ARQ client
//-----------------------------------------------------------------------------

void send0x06()
{
#ifndef __CYGWIN__
// SysV message que output
	if (trx_state == STATE_RX) {
		bSend0x06 = false;
	   	rxmsgid = msgget( (key_t) progdefaults.rx_msgid, 0666 );
   		if ( rxmsgid != -1) {
			rxmsgst.msg_type = 1;
			rxmsgst.c = 0x06;  // tell arq client that transmit complete
			msgsnd (rxmsgid, (void *)&rxmsgst, 1, IPC_NOWAIT);
		}
	}
#else
// process output strings for File i/o
	if (trx_state == STATE_RX) {
		bSend0x06 = false;
		writeToARQfile(0x06);
	}
#endif
}

char arq_get_char()
{
	if (pText != arqtext.end())
		return *pText++;

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
// order of precedence; Socket, SysV, GMFSKfile, ARQfile
		if (Socket_arqRx() == false)
			if (SysV_arqRx() == false)
				if (Gmfsk_arqRx() == false)
					File_arqRx();
// delay for 50 msec interval
		MilliSleep(50);
	}
// exit the arq thread
	return NULL;
}

void arq_init()
{
	arq_enabled = false;
#ifdef __CYGWIN__
	initFilePtr();
#endif
	
	if (fl_create_thread(arq_thread, arq_loop, &arq_dummy) < 0) {
		fl_message("arq init: pthread_create failed");
		return;
	} 

	arq_enabled = true;
}

void arq_close(void)
{
	if (!arq_enabled) return;

// tell the arq thread to kill it self
	arq_exit = true;
#ifdef __CYGWIN__
	remove(str_outfile.c_str());
#endif

// and then wait for it to die
	fl_join(arq_thread);
	arq_enabled = false;
	arq_exit = false;
}
