// pskmail.cxx
// support for pskmail server/client system

#include <iostream>
#include <string>
#include <ctime>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "main.h"
#include "configuration.h"
#include "fl_digi.h"

using namespace std;

static string mailtext;
string::iterator pText;
static char mailline[1000];

bool pskmail_text_available = false;

void ParseMode(string src)
{
	if (src.find("QPSK31") != string::npos)
		initQPSK31();
	else if (src.find("QPSK63") != string::npos)
		initQPSK63();
	else if (src.find("QPSK125") != string::npos)
		initQPSK125();
	else if (src.find("PSK31") != string::npos)
		initPSK31();
	else if (src.find("PSK63") != string::npos)
		initPSK63();
	else if (src.find("PSK125") != string::npos)
		initPSK125();
	else if (src.find("PSK250") != string::npos)
		initPSK250();
	else if (src.find("DOMINOEX4") != string::npos)
		initDOMINOEX4();
	else if (src.find("DOMINOEX5") != string::npos)
		initDOMINOEX5();
	else if (src.find("DOMINOEX8") != string::npos)
		initDOMINOEX8();
	else if (src.find("DOMINOEX11") != string::npos)
		initDOMINOEX11();
	else if (src.find("DOMINOEX16") != string::npos)
		initDOMINOEX16();
	else if (src.find("DOMINOEX22") != string::npos)
		initDOMINOEX22();
	else if (src.find("MFSK8") != string::npos)
		initMFSK8();
	else if (src.find("MFSK16") != string::npos)
		initMFSK16();
	else if (src.find("RTTY") != string::npos)
		initRTTY();
	else if (src.find("CW") != string::npos)
		initCW();
	else if (src.find("PTTTUNE") != string::npos)
	{
		int msecs = 100;
		if (src.length() > 7)
			sscanf( src.substr(7, src.length() - 7).c_str(), "%d", &msecs);
		push2talk->set(true);
		MilliSleep(msecs);
		push2talk->set(false);
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
		while ((idxSubCmd = strCmdText.find("<mode>")) != string::npos) {
			idxSubCmdEnd = strCmdText.find("</mode>");
			if (	idxSubCmdEnd != string::npos && 
					idxSubCmdEnd > idxSubCmd ) {
				strSubCmd = strCmdText.substr(idxSubCmd + 6, idxSubCmdEnd - idxSubCmd - 6);
				ParseMode(strSubCmd);
				strCmdText.erase(idxSubCmd, idxSubCmdEnd - idxSubCmd + 7);
			}
		}
		mailtext.erase(idxCmd, idxCmdEnd - idxCmd + 8);
		if (mailtext.length() == 1 && mailtext[0] == '\n')
			mailtext = "";
	}
}

/*
size_t mailstrftime( char *s, size_t max, const char *fmt, const struct tm *tm) {
	return strftime(s, max, fmt, tm);
}

void mailZDT(string &s)
{
	char szDt[80];
	time_t tmptr;
	tm sTime;
	time (&tmptr);
	gmtime_r(&tmptr, &sTime);
	mailstrftime(szDt, 79, "%x %H:%M %Z", &sTime);
	s = szDt;
}
*/

#define TIMEOUT 180 // 3 minutes


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

void check_formail() {
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
}

void pskmail_loop(void *)
{
	check_formail();
	Fl::repeat_timeout(1.0, pskmail_loop);
}

char pskmail_get_char()
{
	if (pText != mailtext.end())
		return *pText++;

   	rxmsgid = msgget( (key_t) progdefaults.rx_msgid, 0666 );
   	if ( rxmsgid != -1) {
		rxmsgst.msg_type = 1;
		rxmsgst.c = 0x06;  // tell arq client that transmit complete
		msgsnd (rxmsgid, (void *)&rxmsgst, 1, IPC_NOWAIT);
	}

	pskmail_text_available = false;
	return 0x03; // tells psk modem to return to rx
}
