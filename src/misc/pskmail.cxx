// pskmail.cxx
// support for pskmail server/client system

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

#ifdef __CYGWIN__
	string str_infile = "c:/NBEMS/ARQXFR/txfile";
	string str_outfile = "c:/NBEMS/ARQXFR/rxfile";
#endif

using namespace std;

static string mailtext;
string::iterator pText;

#ifndef __CYGWIN__
static char mailline[1000];
#endif

bool pskmail_text_available = false;

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
			if (src.find(mode_info[i].pskmail_name) != string::npos) {
				init_modem(mode_info[i].mode);
				break;
			}
	}
}

void parse_mailtext()
{
	string strCmdText;
	string strSubCmd;
	unsigned long int idxCmd, idxCmdEnd, idxSubCmd, idxSubCmdEnd;

	idxCmd = mailtext.find("<cmd>");
	idxCmdEnd = mailtext.find("</cmd>");
	
	if ( idxCmd != string::npos && idxCmdEnd != string::npos && idxCmdEnd > idxCmd ) {

		strCmdText = mailtext.substr(idxCmd + 5, idxCmdEnd - idxCmd - 5);
		if (strCmdText == "server" && mailserver == false && mailclient == false) {
			mailserver = true;
			mailclient = false;
#ifndef __CYGWIN__
			std::cout << "Starting pskmail server transport layer" << std::endl; std::cout.flush();
#endif
			string PskMailLogName = PskMailDir;
			PskMailLogName += "gMFSK.log";
			Maillogfile = new cLogfile(PskMailLogName.c_str());
			Maillogfile->log_to_file_start();
		} else if (strCmdText == "client" && mailclient == false && mailserver == false) {
			mailclient = true;
			mailserver = false;
#ifndef __CYGWIN__
			std::cout << "Starting pskmail client transport layer" << std::endl; std::cout.flush();
#endif
			string PskMailLogName = PskMailDir;
			PskMailLogName += "gMFSK.log";
			Maillogfile = new cLogfile(PskMailLogName.c_str());
			Maillogfile->log_to_file_start();
		} else if (strCmdText == "normal") {
#ifndef __CYGWIN__
			std::cout << "Closing pskmail transport layer" << std::endl; std::cout.flush();
#endif
			mailserver = false;
			mailclient = false;
			if (Maillogfile) {
				delete Maillogfile;
				Maillogfile = 0;
			}
		} else {
			while ((idxSubCmd = strCmdText.find("<mode>")) != string::npos) {
				idxSubCmdEnd = strCmdText.find("</mode>");
				if (	idxSubCmdEnd != string::npos && 
						idxSubCmdEnd > idxSubCmd ) {
					strSubCmd = strCmdText.substr(idxSubCmd + 6, idxSubCmdEnd - idxSubCmd - 6);
					ParseMode(strSubCmd);
					strCmdText.erase(idxSubCmd, idxSubCmdEnd - idxSubCmd + 7);
				}
			}
		}
		mailtext.erase(idxCmd, idxCmdEnd - idxCmd + 8);
		if (mailtext.length() == 1 && mailtext[0] == '\n')
			mailtext = "";
	}
}

#define TIMEOUT 180 // 3 minutes

bool bSend0x06 = false;

#ifndef __CYGWIN__
void process_msgque()
{
	int nbytes = msgrcv (txmsgid, (void *)&txmsgst, BUFSIZ, 0, IPC_NOWAIT);
	if (nbytes > 0) { 
		mailtext = txmsgst.buffer;
		parse_mailtext();
		if (mailtext.length() > 0) {
			if (mailserver && progdefaults.PSKmailSweetSpot)
				active_modem->set_freq(progdefaults.PSKsweetspot);

			pText = mailtext.begin();
			pskmail_text_available = true;

			active_modem->set_stopflag(false);

			fl_lock(&trx_mutex);
			trx_state = STATE_TX;
			fl_unlock(&trx_mutex);
			wf->set_XmtRcvBtn(true);
		}
	}
}
#endif


#ifdef __CYGWIN__
long infileptr = 0;
bool bInitFilePtr = false;

void initFilePtr()
{
	FILE *infile;
	infile = fopen(str_infile.c_str(), "rb");
	if (infile) {
		fseek(infile, 0, SEEK_END);
		infileptr = ftell(infile);
		fclose(infile);
	}
	bInitFilePtr = true;
}
#endif

void check_formail() {

#ifndef __CYGWIN__
    time_t start_time, prog_time;
    string sAutoFile = PskMailDir;

   	txmsgid = msgget( (key_t) progdefaults.tx_msgid, 0666 );
   	if (txmsgid != -1) {
   		process_msgque();
   		arqmode = true;
   		return;
   	}

	arqmode = false;
    
    if (! (mailserver || mailclient) )
    	return;
    		 
    if (gmfskmail == true)
		sAutoFile += "gmfsk_autofile";
	else
	    sAutoFile += "pskmail_out";

	ifstream autofile(sAutoFile.c_str());
	if(autofile) {
		mailtext = "";
        time(&start_time);
		while (!autofile.eof()) {
			memset(mailline,0,1000);
			autofile.getline(mailline, 998); // leave space for "\n" and null byte
			mailtext.append(mailline);
			mailtext.append("\n");
            FL_AWAKE();
            time(&prog_time);
            if (prog_time - start_time > TIMEOUT) {
                std::cout << "pskmail_out failure" << std::endl;
                std::cout.flush();
                autofile.close();
                std::remove (sAutoFile.c_str());
                return;
            }
		}
		autofile.close();
		std::remove (sAutoFile.c_str());
		
		parse_mailtext();
		if (mailtext.length() > 0) {
			if (mailserver && progdefaults.PSKmailSweetSpot)
				active_modem->set_freq(progdefaults.PSKsweetspot);

			pText = mailtext.begin();
			pskmail_text_available = true;

			active_modem->set_stopflag(false);

			fl_lock(&trx_mutex);
			trx_state = STATE_TX;
			fl_unlock(&trx_mutex);
			wf->set_XmtRcvBtn(true);
		}
	} 
#else
// Windows file handling for input strings
	FILE *infile;
	infile = fopen(str_infile.c_str(), "rb");
	if (infile) {
		fseek(infile, 0, SEEK_END);
		long sz = ftell(infile);
		if (sz < infileptr)
			infileptr = 0; // txfile was probably deleted & restarted
		if (sz > infileptr) {
			mailtext = "";
			fseek(infile, infileptr, SEEK_SET);
				while (infileptr < sz) {
					mailtext += fgetc(infile);
					infileptr++;
				}
			if (mailtext.length() > 0) {
				parse_mailtext();
				pText = mailtext.begin();
				pskmail_text_available = true;
				active_modem->set_stopflag(false);
				fl_lock(&trx_mutex);
				trx_state = STATE_TX;
				fl_unlock(&trx_mutex);
				wf->set_XmtRcvBtn(true);
				arqmode = true;
			}
		}
		fclose(infile);
	}
#endif
}

#ifdef __CYGWIN__
void writeToARQfile(unsigned int data)
{
	FILE *outfile;
	outfile = fopen(str_outfile.c_str(), "ab");
	if (outfile) {
		putc((unsigned char)data, outfile );
		fclose(outfile);
	}
}
#endif

void send0x06()
{
#ifndef __CYGWIN__
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
// process output strings for Windows file i/o
	if (trx_state == STATE_RX) {
		bSend0x06 = false;
		writeToARQfile(0x06);
	}
#endif
}

void pskmail_loop(void *)
{
#ifdef __CYGWIN__
	if (bInitFilePtr == false)
		initFilePtr();
#endif
	if (bSend0x06)
		send0x06();
	check_formail();
	Fl::repeat_timeout(0.2, pskmail_loop);//1.0, pskmail_loop);
}

char pskmail_get_char()
{
	if (pText != mailtext.end())
		return *pText++;

	bSend0x06 = true;
	pskmail_text_available = false;
	return 0x03; // tells psk modem to return to rx
}
