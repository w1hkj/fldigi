// ----------------------------------------------------------------------------
// flarq.cxx - Fast Light ARQ Application
//
// ----------------------------------------------------------------------------
// Copyright (C) 2014
//              David Freese, W1HKJ
//
// This file is part of fldigi
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
#include <unistd.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include <ctime>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/Enumerations.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Sys_Menu_Bar.H>
#include <FL/x.H>
#include <FL/Fl_Help_Dialog.H>
#include <FL/Fl_Menu_Item.H>

// this tests depends on a modified FL/filename.H in the Fltk-1.3.0
// change
//#  if defined(WIN32) && !defined(__CYGWIN__) && !defined(__WATCOMC__)
// to
//#  if defined(WIN32) && !defined(__CYGWIN__) && !defined(__WATCOMC__) && !defined(__WOE32__)

#include <FL/filename.H>

#ifdef __MINGW32__
#	include "compat.h"
#endif

#include <dirent.h>

#include "fileselect.h"
#include "socket.h"

#include "debug.h"
#include "threads.h"
#include "flarq.h"
#include "flarqenv.h"
#include "flmisc.h"
#include "stacktrace.h"
#include "icons.h"
#include "arq.h"
#include "arqdialogs.h"
#include "b64.h"
#include "gettext.h"

#include "xml_server.h"

#include "FTextView.h"

debug::level_e debug::level = debug::INFO_LEVEL;
unsigned int debug::mask;
void debug::log(debug::level_e, const char*, const char*, int line, const char* ...) {}

#define FLDIGI_port "7322"
#define MPSK_port "3122"

#define FLARQ_XML_PORT 7422

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

// directory structures for all NBEMS applications
static void checkdirectories(void);
std::string HomeDir;
std::string NBEMS_dir;
std::string ARQ_dir;
std::string ARQ_files_dir;
std::string ARQ_recv_dir;
std::string ARQ_send_dir;
std::string ARQ_mail_dir;
std::string ARQ_mail_in_dir;
std::string ARQ_mail_out_dir;
std::string ARQ_mail_sent_dir;
std::string WRAP_dir;
std::string WRAP_recv_dir;
std::string WRAP_send_dir;
std::string WRAP_auto_dir;
std::string ICS_dir;
std::string ICS_msg_dir;
std::string ICS_tmp_dir;

std::string MailFileName = "";
std::string MailSaveFileName = "";
std::string Logfile;

struct dirent *entry;
DIR *dp;

bool   SendingEmail = false;

bool SHOWDEBUG = false;

extern void STATUSprint(std::string s);

Fl_Text_Buffer_mod *txtbuffARQ;
Fl_Text_Buffer_mod *txtbuffRX;
Fl_Text_Buffer_mod *txtbuffComposer;

Fl_Double_Window *arqwin = 0;
Fl_Double_Window *dlgconfig = 0;
Fl_Double_Window *outdialog = 0;
Fl_Double_Window *composer = 0;

using namespace std;

arq *digi_arq;
bool traffic = false;
bool sendingfile = false;
int  arqstate = 0;
bool configured = false;

bool ioMPSK = false;
std::string arq_address =  "127.0.0.1";
std::string arq_port = FLDIGI_port;

std::string RX;
std::string TX;

std::string teststring;

std::string statusmsg;

std::string MyCall;
std::string beacontext;

#if !defined(__APPLE__) && !defined(__WOE32__) && USE_X
Pixmap flarq_icon_pixmap;
#endif

Socket *tcpip = 0;
std::string txbuffer;
std::string cmdbuffer;
std::string rxbuffer;

size_t  bufsize = 0;
size_t  bufptr = 0;

bool isRxChar = false;
bool isCmdChar = false;
bool isTxChar = false;
bool inLoop = false;

int    exponent = 5;
int	   txdelay = 500;
int    iretries = 5;
long   iwaittime = 10000;
long   itimeout = 60000;
int    idtimer = 10;
int	   bcnInterval = 30;

enum {OFF, ON, WAIT};
int    autobeacon = OFF;
bool   beaconrcvd = false;
bool   restart_beacon = false;

int    blocksSent = 0;

int mainX = 0, mainY = 0, mainW = 600, mainH = 500;

float  vers = 0;
float  version;

const char *ASCII[32] = {
	"<NUL>", "<SOH>", "<STX>", "<ETX>", // 0x00 - 0x03
	"<EOT>", "<ENQ>", "<ACK>", "<BEL>", // 0x04 - 0x07
	"<BX>",  "<TAB>", "<LF>",  "<VT>",  // 0x08 - 0x0B
	"<FF>",  "<CR>",  "<SO>",  "<SI>",  // 0x0C - 0x0F
	"<DLE>", "<DC1>", "<DC2>", "<DC3>", // 0x10 - 0x13
	"<DC4>", "<NAK>", "<SYN>", "<ETB>", // 0x14 - 0x17
	"<CAN>", "<EM>",  "<SUB>", "<ESC>", // 0x18 - 0x1B
	"<FS>",  "<GS>",  "<RS>",  "<US>"   // 0x1C - 0x1F
};

std::string AsciiChars;

std::string incomingText = "";
std::string txtarqload = "";
std::string rxfname = "";
std::string arqstart = "ARQ::STX\n";
std::string arqend   = "ARQ::ETX\n";
std::string arqfile = "ARQ:FILE::";
std::string arqemail = "ARQ:EMAIL::\n";
std::string arqascii = "ARQ:ENCODING::ASCII\n";
std::string arqbase64 = "ARQ:ENCODING::BASE64\n";
std::string arqsizespec = "ARQ:SIZE::";
size_t startpos = std::string::npos;
size_t endpos = std::string::npos;
size_t fnamepos = std::string::npos;
size_t indx = std::string::npos;
size_t sizepos = std::string::npos;
size_t lfpos = std::string::npos;
size_t arqPayloadSize = 0;
bool haveemail = false;
bool rxARQfile = false;
bool rxARQhavesize = false;
bool rxTextReady = false;

time_t StartTime_t;
time_t EndTime_t;
time_t TransferTime_t;
double TransferTime;

//=============================================================================
// email selector
//=============================================================================

int datedir = 1;
int todir = 1;
int subdir = 1;
std::string sendfilename = "";

void cb_SortByDate()
{
	if (datedir == 0) {
		tblOutgoing->sort(1, false);
		datedir = 1;
	} else {
		tblOutgoing->sort(1, true);
		datedir = 0;
	}
	tblOutgoing->redraw();
}

void cb_SortByTo()
{
	if (todir == 0) {
		tblOutgoing->sort(2, false);
		todir = 1;
	} else {
		tblOutgoing->sort(2, true);
		todir = 0;
	}
	tblOutgoing->redraw();
}

void cb_SortBySubj()
{
	if (subdir == 0) {
		tblOutgoing->sort(3, false);
		subdir = 1;
	} else {
		tblOutgoing->sort(3, true);
		subdir = 0;
	}
	tblOutgoing->redraw();
}

void sendOK()
{
	outdialog->hide();
	int sel = tblOutgoing->value();
	if (sel >= 0)
		sendfilename = tblOutgoing->valueAt(sel,0);
	else
		sendfilename.clear();
}

void sendCancel()
{
	outdialog->hide();
	sendfilename.clear();
}

void selectTrafficOut(bool ComposerOnly)
{
	if (outdialog == 0) {
		outdialog = arq_SendSelect();
		outdialog->xclass(PACKAGE_TARNAME);
		tblOutgoing->addHiddenColumn ("fnbr"); // column #0
		tblOutgoing->addColumn ("Date", 180); // column #1
		tblOutgoing->addColumn ("To", 120); // column #2
		tblOutgoing->addColumn ("Subj", 196); // column #3
		tblOutgoing->colcallback (1, cb_SortByDate);
		tblOutgoing->columnAlign(1, FL_ALIGN_LEFT);
		tblOutgoing->colcallback (2, cb_SortByTo);
		tblOutgoing->columnAlign(2, FL_ALIGN_LEFT);
		tblOutgoing->colcallback (3, cb_SortBySubj);
		tblOutgoing->columnAlign(3, FL_ALIGN_LEFT);
		tblOutgoing->allowSort(true);
		tblOutgoing->rowSize (14);
		tblOutgoing->headerSize (14);
		tblOutgoing->allowResize (true);
		tblOutgoing->gridEnabled (true);
	}
	tblOutgoing->clear();
	std::string fline, fname, fdate, fto, fsubj;
	char szline[10000];
	size_t p;

	std::string folder = ARQ_mail_out_dir;
	dp = 0;
	dp = opendir(folder.c_str());
	if (dp == 0) {
		std::string nfound = folder;
		nfound += " not found";
		fl_message("%s", nfound.c_str());
		return;
	}

	int nummails = 0;
	while ((entry = readdir(dp)) != 0) {
		if (entry->d_name[0] == '.')
			continue;
		fname = folder; fname.append(entry->d_name);
		if (fname.find(".eml") == std::string::npos)
			continue;
		int validlines = 0;
		ifstream emailtxt(fname.c_str());
		while (!emailtxt.eof()) {
			memset(szline, 0, 10000);
			emailtxt.getline(szline,10000);
			fline = szline;
			if ((p = fline.find("Date: ")) != std::string::npos) {
				fdate = fline.substr(p + 6);
				validlines++;
				continue;
			}
			if ((p = fline.find("To: ")) != std::string::npos) {
				fto = fline.substr(p + 4);
				p = fto.find('@');
				if (p != std::string::npos) fto.replace(p,1,"@@");
				p = fto.find("<");
				if (p != std::string::npos) fto.erase(p,1);
				p = fto.find(">");
				if (p != std::string::npos) fto.erase(p,1);
				validlines++;
				continue;
			}
			if ((p = fline.find("Subject: ")) != std::string::npos) {
				fsubj = fline.substr(p + 9);
				validlines++;
				continue;
			}
			if ((p = fline.find("//FLARQ COMPOSER")) != std::string::npos)
				validlines++;
		}
		emailtxt.close();
		if ((!ComposerOnly && validlines >= 3) || (ComposerOnly && validlines == 4)) {
			tblOutgoing->addRow (4, fname.c_str(), fdate.c_str(), fto.c_str(), fsubj.c_str());
			nummails++;
		}
	}
	if (nummails) {
		tblOutgoing->FirstRow();
		outdialog->show();
		while (outdialog->shown())
			Fl::wait();
	} else
		fl_message2("No emails for delivery");
}

//======================================================================================
// simple email composer
//======================================================================================
extern bool fileExists(std::string fname);

void cb_CancelComposeMail()
{
	composer->hide();
}

void readComposedFile(std::string filename)
{
	ifstream textfile;
	textfile.open(filename.c_str());
	if (textfile) {
		char szline[10000];
		std::string fline, tempstr;
		size_t p;
		txtMailText->clear();
		inpMailFrom->value("");
		inpMailTo->value("");
		inpMailSubj->value("");
		while (!textfile.eof()) {
			memset(szline,0, 10000);
			textfile.getline(szline,10000);
			fline = szline;
			if ((p = fline.find("//FLARQ COMPOSER")) != std::string::npos)
				continue;
			if ((p = fline.find("Date: ")) != std::string::npos)
				continue;
			if ((p = fline.find("Content-Type:")) != std::string::npos)
				continue;
			if ((p = fline.find("From: ")) != std::string::npos) {
				tempstr = fline.substr(p + 6);
				inpMailFrom->value(tempstr.c_str());
				continue;
			}
			if ((p = fline.find("To: ")) != std::string::npos) {
				tempstr = fline.substr(p + 4);
				p = tempstr.find("<");
				if (p != std::string::npos) tempstr.erase(p,1);
				p = tempstr.find(">");
				if (p != std::string::npos) tempstr.erase(p,1);
				inpMailTo->value(tempstr.c_str());
				continue;
			}
			if ((p = fline.find("Subject: ")) != std::string::npos) {
				inpMailSubj->value(fline.substr(p + 9).c_str());
				continue;
			}
			if (strlen(szline) == 0 && txtbuffComposer->length() == 0) continue;
			txtMailText->add(szline);
			txtMailText->add("\n");
		}
		textfile.close();
	}
}

void cb_UseTemplate()
{
	std::string templatename = ARQ_mail_out_dir;
	const char *p = FSEL::select("Load Template file", "*.tpl", templatename.c_str());
	if (!p) return;
	if (!*p) return;
	readComposedFile(p);
}

void cb_ClearComposer()
{
	sendfilename.clear();
	txtMailText->clear();
	inpMailFrom->value("");
	inpMailTo->value("");
	inpMailSubj->value("");
}

std::string nextEmailFile(std::string fname)
{
	int nbr = 0;
	char szNbr[20];
	std::string name;
	std::string ext;
	std::string nuname;
	size_t p;

	p = fname.find_last_of('.');
	if (p != std::string::npos) {
		ext = fname.substr(p);
		name = fname.substr(0,p);
	} else {
		ext = "";
		name = fname;
	}

	do {
		nbr++;
		nuname = name;
#ifdef __WOE32__
		snprintf(szNbr, sizeof(szNbr), "-%-d", nbr);
#else
		snprintf(szNbr, sizeof(szNbr), "%-d", nbr);
#endif
		nuname.append(szNbr);
		nuname.append(ext);
	} while (fileExists(nuname));
	return nuname;
}

void saveComposedText(std::string filename)
{
	ofstream textfile;
	textfile.open(filename.c_str());
	textfile << "//FLARQ COMPOSER" << endl;
	char szmaildt[100];
	time_t maildt = time(NULL);
	struct tm *gmt = gmtime(&maildt);
	strftime(szmaildt, sizeof(szmaildt), "%x %X", gmt);
	textfile << "Date: " << szmaildt << endl;
	textfile << "From: " << inpMailFrom->value() << endl;
	textfile << "To: " << inpMailTo->value() << endl;
	textfile << "Subject: " << inpMailSubj->value() << endl;
	textfile << "Content-Type: text/plain; charset=\"UTF-8\"" << endl;
	textfile << endl;
	textfile << txtbuffComposer->text() << endl;
	textfile.close();
	cb_ClearComposer();
}

void SaveComposeMail()
{
	if (strlen(inpMailTo->value()) == 0 ||
		strlen(inpMailSubj->value()) == 0)
		return;
	if (sendfilename.empty()) {
		sendfilename = ARQ_mail_out_dir;
		sendfilename += "flarqmail.eml";
		sendfilename = nextEmailFile(sendfilename);
	}
	saveComposedText(sendfilename);
}

void SaveComposeTemplate()
{
	std::string templatename = ARQ_mail_out_dir;
	const char *p = FSEL::saveas("Save Template file", "*.tpl", templatename.c_str());
	if (!p) return;
	if (!*p) return;

	saveComposedText(p);
}

void cb_SaveComposeMail()
{
	if (Fl::event_state(FL_SHIFT))
		SaveComposeTemplate();
	else
		SaveComposeMail();
}

void cb_OpenComposeMail()
{
	sendfilename.clear();
	selectTrafficOut(true);
	if (sendfilename.empty())
		return;
	readComposedFile(sendfilename);
}

void ComposeMail()
{
	if (composer == 0) {
		composer = arq_composer();
		composer->xclass(PACKAGE_TARNAME);
		txtbuffComposer = txtMailText->buffer();
		txtMailText->wrap_mode(1,80);
	}
	txtbuffComposer->text("");
	inpMailFrom->value("");
	inpMailTo->value("");
	inpMailSubj->value("");

	composer->show();
}

//======================================================================================

std::string noCR(std::string s)
{
	std::string text = s;
	size_t p;
	while ((p = text.find('\r')) != std::string::npos)
		text.erase(p,1);
	return text;
}

void createAsciiChars()
{
	AsciiChars = "";
	AsciiChars += 0x09; // tab
	AsciiChars += 0x0A; // lf
	AsciiChars += 0x0D; // cr
	for (int n = 20; n < 256; n++) AsciiChars += n;
}

void initVals()
{
	MyCall = "NOCALL";
	exponent = digi_arq->getExponent();
	iretries = digi_arq->getRetries();
	itimeout = digi_arq->getTimeout();
	txdelay = digi_arq->getTxDelay();
	iwaittime = digi_arq->getWaitTime();
	bcnInterval = 30;
	beacontext = "";
	cbMenuConfig();
	digi_arq->myCall(MyCall.c_str());

}

void testDirectory(std::string dir)
{
	std::string tstdir = ARQ_dir;
	tstdir += '/';
	tstdir.append(dir);
	ifstream test(tstdir.c_str());
	if (!test)
		mkdir(tstdir.c_str(), 0777);
	else
		test.close();
}

void readConfig()
{
	extern void cbMenuConfig();
	std::string configfname = ARQ_dir;
	configfname.append("flarq.config");
	ifstream configfile(configfname.c_str());
	if (configfile) {
		char tempstr[200];
		configfile.getline(tempstr,200);
		sscanf(tempstr,"%f", &vers);
		if (int(vers*10) != int(version*10))
			initVals();
		else {
			configfile >> MyCall;
			configfile >> exponent;
			configfile >> txdelay;
			configfile >> iretries;
			configfile >> iwaittime;
			configfile >> itimeout;
			configfile >> bcnInterval;
			configfile >> mainX;
			configfile >> mainY;
			configfile >> mainW;
			configfile >> mainH;
			configfile.ignore();
			configfile.getline(tempstr, 200);
			beacontext = tempstr;
			configfile >> restart_beacon;

			digi_arq->myCall(MyCall.c_str());
			digi_arq->setExponent(exponent);
			digi_arq->setRetries(iretries);
			digi_arq->setTimeout(itimeout);
			digi_arq->setTxDelay(txdelay);
			digi_arq->setWaitTime(iwaittime);
		}
		configfile.close();
	} else
		initVals();

}

void saveConfig()
{
	std::string configfname = ARQ_dir;
	configfname.append("flarq.config");
	ofstream configfile(configfname.c_str());
	if (configfile) {
		int mainX = arqwin->x();
		int mainY = arqwin->y();
		int mainW = arqwin->w();
		int mainH = arqwin->h();
		configfile << VERSION << endl;
		configfile << MyCall << endl;
		configfile << exponent << endl;
		configfile << txdelay << endl;
		configfile << iretries << endl;
		configfile << iwaittime << endl;
		configfile << itimeout << endl;
		configfile << bcnInterval << endl;
		configfile << mainX << endl;
		configfile << mainY << endl;
		configfile << mainW << endl;
		configfile << mainH << endl;
		configfile << beacontext.c_str() << endl;
		configfile << restart_beacon << endl;
		configfile.close();
	}
}

void cbSetConfig()
{
	digi_arq->setExponent(exponent);
	digi_arq->setRetries(iretries);
	digi_arq->setTimeout(itimeout);
	digi_arq->setTxDelay(txdelay);
	digi_arq->setWaitTime(iwaittime);
}

void closeConfig()
{
	if (dlgconfig)
		dlgconfig->hide();
	cbSetConfig();
}

void cb_idtimer()
{
	digi_arq->set_idtimer();
}

void cbMenuConfig()
{
	if (!dlgconfig) {
		dlgconfig = arq_configure();
		dlgconfig->xclass(PACKAGE_TARNAME);
		choiceBlockSize->add("16");
		choiceBlockSize->add("32");
		choiceBlockSize->add("64");
		choiceBlockSize->add("128");
		choiceBlockSize->add("256");
	}
	choiceBlockSize->index(exponent - 4);
	choiceBlockSize->redraw();
	dlgconfig->show();
}

void cbMenuAbout()
{
	fl_message2("flarq - ARQ client\nversion: %s\nw1hkj@@w1hkj.com", VERSION);
}

std::string txhold = "";

//=============================================================================

void mpsk_on()
{
	std::string s;
	s.append(1, MPSK_CMD).append(MPSK_TX).append(1, MPSK_END);
	try {
		tcpip->send(s);
	}
	catch (const SocketException& e) {
		cerr << e.what() << '\n';
	}
}

void mpsk_off_after_buffer_sent()
{
	std::string s;
	s.append(1, MPSK_CMD).append(MPSK_TX2RX).append(1, MPSK_END);
	try {
		tcpip->send(s);
	}
	catch (const SocketException& e) {
		cerr << e.what() << '\n';
	}
}

void mpsk_off()
{
	std::string s;
	s.append(1, MPSK_CMD).append(MPSK_RX).append(1, MPSK_END);
	try {
		tcpip->send(s);
	}
	catch (const SocketException& e) {
		cerr << e.what() << '\n';
	}
}

void MPSK_client_transmit(const std::string& s)
{
	if (s.empty())
		return;
	std::string tosend;
	tosend.reserve(s.length() * 2);
	for (size_t i = 0; i < s.length(); i++)
		tosend.append(1, MPSK_BYTE).append(1, s[i]);

	try {
		mpsk_on();
		tcpip->send(tosend);
		mpsk_off_after_buffer_sent();
	}
	catch (const SocketException& e) {
		cerr << e.what() << '\n';
	}
}

void MPSK_Socket_rcv_loop(void *)
{
	if (inLoop) Fl::wait(0.1);
	inLoop = true;

	char cs;

	try {
		while (tcpip->wait(0)) {
			tcpip->recv(&cs, 1);

			if (isRxChar) {
				rxbuffer += cs;
				isRxChar = false;
				continue;
			}
			if (isTxChar) {
				if (cs < 28 || cs > 31)
					txbuffer += cs;
				isTxChar = false;
				continue;
			}
			if (isCmdChar) {
				if (cs == MPSK_CMDEND) {
					isCmdChar = false;
					if (cmdbuffer.find("RX_AFTER_TX OK") != std::string::npos) {
						rxbuffer += 0x06;
						cmdbuffer.clear();
						txbuffer.clear();
					}
					continue;
				}
				cmdbuffer += cs;
				continue;
			}
			if (cs == MPSK_ISRX) {
				isRxChar = true;
				continue;
			}
			if (cs == MPSK_ISCMD) {
				isCmdChar = true;
				continue;
			}
			if (cs == MPSK_ISTX) {
				isTxChar = true;
				continue;
			}
		}
	}
	catch (const SocketException& e) {
		cerr << e.what() << '\n';
	}

	inLoop = false;
	Fl::add_timeout(0.01, MPSK_Socket_rcv_loop);
}

//=============================================================================

void client_transmit(const std::string& s )
{
	try {
		if (!s.empty())
			tcpip->send(s);
	}
	catch (const SocketException& e) {
		cerr << e.what() << '\n';
	}
}

void Socket_rcv_loop(void *)
{
	if (inLoop)
		Fl::wait(0.1);
	inLoop = true;

	try {
		tcpip->set_nonblocking(true);
		tcpip->recv(rxbuffer);
		tcpip->set_nonblocking(false);
	}
	catch (const SocketException& e) {
		cerr << e.what() << '\n';
	}

	inLoop = false;
	Fl::add_timeout(0.01, Socket_rcv_loop);
}

bool client_receive(char &c)
{
	if (inLoop) Fl::wait(0.1);
	inLoop = true;
	bufsize = rxbuffer.length();
	if (bufsize) {
		if (bufptr < bufsize) {
			c = rxbuffer[bufptr++];
			inLoop = false;
			return true;
		}
	}
	rxbuffer.clear();
	bufsize = 0;
	bufptr = 0;
	inLoop = false;
	return false;
}

int autobeaconcounter = 0;
char bcnMsg[40];

void arqAutoBeacon(void *)
{
	if (autobeacon == ON) {
		int currstate = digi_arq->state();
		btnCONNECT->deactivate();
		btnCONNECT->redraw();
		if (currstate > 0x7F) {
			txtBeaconing->value("Beaconing");
			btnBEACON->deactivate();
			btnBEACON->redraw();
			Fl::repeat_timeout(1.0, arqAutoBeacon);
			return;
		} else if (currstate == DOWN || currstate == TIMEDOUT) {
			if (autobeaconcounter == 0) {
				digi_arq->sendBeacon( beacontext );
				autobeaconcounter = bcnInterval;
				Fl::repeat_timeout(1.0 + txdelay / 1000.0, arqAutoBeacon);
			} else {
				snprintf(bcnMsg, sizeof(bcnMsg), "Bcn in: %d sec", autobeaconcounter);
				txtBeaconing->value(bcnMsg);
				btnBEACON->label("STOP");
				btnBEACON->redraw_label();
				btnBEACON->activate();
				btnBEACON->redraw();
				autobeaconcounter--;
				Fl::repeat_timeout(1.0, arqAutoBeacon);
			}
			return;
		} else {
			autobeaconcounter = 0;
			btnBEACON->value(0);
			btnBEACON->label("Beacon");
			btnBEACON->redraw_label();
			btnBEACON->activate();
			btnBEACON->redraw();
			txtBeaconing->value("Beacon Off");
		}
	} else if (autobeacon == OFF) {
		autobeaconcounter = 0;
		btnCONNECT->activate();
		btnCONNECT->redraw();
		btnBEACON->value(0);
		btnBEACON->label("Beacon");
		btnBEACON->redraw_label();
		btnBEACON->activate();
		btnBEACON->redraw();
		txtBeaconing->value("Beacon Off");
	} else { // autobeacon == WAIT
		btnBEACON->deactivate();
		btnBEACON->redraw();
		Fl::repeat_timeout(1.0, arqAutoBeacon);
	}
}

void arqBEACON()
{
	if (autobeacon != ON) {
		autobeacon = ON;
		btnBEACON->value(1);
		Fl::add_timeout(0.01, arqAutoBeacon);
	} else {
		autobeacon = OFF;
		btnBEACON->value(0);
	}
}

void printstring(std::string s)
{
	for (size_t n = 0; n < s.length(); n++)
		if (s[n] < ' ') printf("<%02d>",(int)s[n]);
		else printf("%c", s[n]);
	printf("\n");
}

void arqCLOSE()
{
	tcpip->close();
	saveConfig();
	exit_server();
	exit(0);
}

void clearText()
{
	txtarqload = "";
	txtbuffARQ->text("");
	txtStatus->value("");
	txtStatus2->value("");
}

void restart()
{
	TX.clear();
	rxfname = "";
	rxTextReady = false;
	prgStatus->value(0.0);
	prgStatus->label("");
	prgStatus->redraw();
	prgStatus->redraw_label();
	rxARQfile = false;
	sendingfile = false;
	incomingText.clear();
	clearText();
}

void arqCONNECT()
{
	int state = Fl::event_state();
	if ((state & FL_CTRL) == FL_CTRL) {
		digi_arq->restart_arq();
		txtURCALL->value("");
		restart();
		return;
	}
	if (digi_arq->state() < ARQ_CONNECTED) {
		if (strlen(txtURCALL->value()) > 0)
			digi_arq->connect(txtURCALL->value());
	} else {
		if (rxARQfile || sendingfile)
			abortTransfer();
		else {
			restart();
			digi_arq->disconnect();
			txtURCALL->value("");
		}
	}
}

bool fileExists(std::string fname)
{
	ifstream test(fname.c_str());
	if (test) {
		test.close();
		return true;
	}
	return false;
}

std::string nextFileName(std::string fname)
{
	int nbr = 0;
	char szNbr[20];
	std::string name;
	std::string ext;
	std::string nuname;
	size_t p;

	p = fname.find_last_of('.');
	if (p != std::string::npos) {
		ext = fname.substr(p);
		name = fname.substr(0,p);
	} else {
		ext = "";
		name = fname;
	}

	do {
		nbr++;
		nuname = name;
		snprintf(szNbr, sizeof(szNbr), ".dup%-d", nbr);
		nuname.append(szNbr);
		nuname.append(ext);
	} while (fileExists(nuname));

	return nuname;
}

void saveEmailFile()
{
	static char xfrmsg[80];
	std::string tempname;

	time(&EndTime_t);
	TransferTime = difftime(EndTime_t, StartTime_t);
	snprintf(xfrmsg, sizeof(xfrmsg), "Transfer Completed in %4.0f sec's", TransferTime);

	std::string savetoname = ARQ_mail_in_dir;

	if (rxfname.find(".eml") == std::string::npos)
		rxfname.append(".eml");
	savetoname.append(rxfname);
	while (fileExists(savetoname))
		savetoname = nextFileName(savetoname);

	ofstream tfile(savetoname.c_str(), ios::binary);
	if (tfile) {
		tfile << txtarqload;
		tfile.close();
	}

	txtStatus->value(xfrmsg);
	rxfname = "";
	txtarqload = "";
	rxTextReady = false;
}

void saveRxFile()
{
	static char xfrmsg[80];
	time(&EndTime_t);
	TransferTime = difftime(EndTime_t, StartTime_t);
	snprintf(xfrmsg, sizeof(xfrmsg), "Transfer Completed in %4.0f sec's", TransferTime);

	std::string savetoname = ARQ_recv_dir;
	savetoname.append(rxfname);
	if (fileExists(savetoname))
		savetoname = nextFileName(savetoname);

	ofstream tfile(savetoname.c_str(), ios::binary);
	if (tfile) {
		tfile << txtarqload;
		tfile.close();
	}

	txtStatus->value(xfrmsg);
	rxfname = "";
	txtarqload = "";
	rxTextReady = false;
}

void payloadText(std::string s)
{
	static char szPercent[10];
	std::string text = noCR(s);

	txtARQ->insert(text.c_str());
	txtARQ->show_insert_position();
	txtARQ->redraw();

	incomingText.append (s);

	if (!rxARQfile)
		if ((startpos = incomingText.find(arqstart)) != std::string::npos) {
			rxARQfile = true;
			startpos += arqstart.length();
			time(&StartTime_t);
		}
	if (rxARQfile) {
		if (!rxARQhavesize) {
			if ( (sizepos = incomingText.find(arqsizespec)) != std::string::npos) {
				sizepos += arqsizespec.length();
				if ((lfpos = incomingText.find('\n', sizepos)) != std::string::npos) {
					std::string sizechars = incomingText.substr(sizepos, lfpos - sizepos);
					unsigned int tempnbr;
					sscanf(sizechars.c_str(), "%u", &tempnbr);
					arqPayloadSize = tempnbr;
					rxARQhavesize = true;
					char statusmsg[40];
					snprintf(statusmsg, sizeof(statusmsg), "Rcvg: %d", 
						static_cast<int>(arqPayloadSize));
					txtStatus->value(statusmsg);
				}
			}
		} else {
			if (startpos != std::string::npos) {
				float partial = incomingText.length() - startpos;
				snprintf(szPercent, sizeof(szPercent), "%3.0f %%", 100.0 * partial / arqPayloadSize);
				prgStatus->value( partial / arqPayloadSize );
				prgStatus->label(szPercent);
			 } else {
				prgStatus->value(0.0);
				prgStatus->label("");
			 }
			prgStatus->redraw();
			prgStatus->redraw_label();
		}
		if ((endpos = incomingText.find(arqend)) != std::string::npos) {
			haveemail = false;
			fnamepos = incomingText.find(arqfile);
			fnamepos += arqfile.length();
			indx = incomingText.find('\n', fnamepos);
			rxfname = incomingText.substr(fnamepos, indx - fnamepos);
			txtarqload = incomingText.substr(startpos, endpos - startpos);
			if (incomingText.find(arqbase64) != std::string::npos) {
				base64 b64;
				txtarqload = b64.decode(txtarqload);
			}
			if (incomingText.find(arqemail) != std::string::npos)
				haveemail = true;
			startpos = std::string::npos;
			endpos = std::string::npos;
			fnamepos = std::string::npos;
			indx = std::string::npos;
			sizepos = std::string::npos;
			lfpos = std::string::npos;
			arqPayloadSize = 0;
			rxARQfile = false;
			rxARQhavesize = false;
			rxTextReady = true;
			if (incomingText.find("FLMSG_XFR") != std::string::npos)
				xml_rx_text_ready = true;
			incomingText = "";
			txtStatus->value("");
			prgStatus->value(0.0);
			prgStatus->label("");
			prgStatus->redraw();
			prgStatus->redraw_label();
		}
	}
}

void cbClearText()
{
	txtbuffARQ->text("");
}

void abortedTransfer()
{
	restart();
	txtARQ->insert("transfer aborted\n");
	btnCONNECT->activate();
}

void abortTransfer()
{
	sendingfile = false;
	SendingEmail = false;
	rxARQfile = false;
	btnCONNECT->label("ABORTING");
	btnCONNECT->redraw_label();
	btnCONNECT->deactivate();
	digi_arq->abort();
}

void rxBeaconCallsign(std::string s)
{
	txtURCALL->value(s.c_str());
	beaconrcvd = true;
}

void moveEmailFile()
{
	if (MailFileName.empty()) return;
	if (MailSaveFileName.empty()) return;

	ifstream infile(MailFileName.c_str(), ios::in | ios::binary);

	if (MailSaveFileName.find(".eml") == std::string::npos)
		MailSaveFileName.append(".eml");
	while (fileExists(MailSaveFileName))
		MailSaveFileName = nextFileName(MailSaveFileName);

	ofstream outfile(MailSaveFileName.c_str(), ios::out | ios::binary);
	char c;

	if (infile && outfile) {
		while (!infile.eof()) {
			infile.get(c);
			outfile.put(c);
		}
		infile.close();
		outfile.close();
		unlink(MailFileName.c_str());
	}
	MailFileName.clear();
	MailSaveFileName.clear();
}


void sendEmailFile()
{
	if (arqstate < ARQ_CONNECTED) {
		fl_alert2("Not connected");
		return;
	}
	sendfilename.clear();
	selectTrafficOut(false);

	if (sendfilename.empty())
		return;

	char cin;
	size_t txtsize;
	std::string textin = "";
	char sizemsg[40];
	size_t p;

	ifstream textfile;
	textfile.open(sendfilename.c_str(), ios::binary);
	if (textfile) {
		for (size_t i = 0; i < sendfilename.length(); i++)
			if (sendfilename[i] == '\\') sendfilename[i] = '/';
		MailFileName = sendfilename;
		TX.erase();
		TX.append(arqfile);
		MailSaveFileName = ARQ_mail_sent_dir;
		p = sendfilename.find_last_of('/');
		if (p != std::string::npos) p++;
		MailSaveFileName.append(sendfilename.substr(p));
		TX.append(sendfilename.substr(p));
		TX.append("\n");
		TX.append(arqemail);

// only allow ASCII printable characters
		while (textfile.get(cin)) textin += (cin & 0xFF);
		textfile.close();
		if ( textin.find_first_not_of(AsciiChars) != std::string::npos) {
			fl_alert2("File contains non-ASCII bytes and must be sent as binary.");
			return;
		}

		textfile.close();
		txtsize = textin.length();
		arqPayloadSize = txtsize;
		blocksSent = 0;
		snprintf(sizemsg, sizeof(sizemsg), "ARQ:SIZE::%d\n", 
			static_cast<int>(txtsize));
		TX.append(sizemsg);
		TX.append(arqstart);
		TX.append(textin);
		TX.append(arqend);
		traffic = true;
		statusmsg = "Sending email: ";
		statusmsg.append(sendfilename.substr(p));
		txtStatus->value(statusmsg.c_str());
		SendingEmail = true;
		sendingfile = true;
		cbClearText();
		return;
	}

	traffic = false;
	sendingfile = false;
	SendingEmail = false;
}


void sendAsciiFile()
{
	if (arqstate < ARQ_CONNECTED) {
		fl_alert2("Not connected");
		return;
	}
	std::string readfromname = ARQ_send_dir;
	readfromname.append(rxfname);
	const char *p = FSEL::select("ARQ text file", "*.txt\t*", readfromname.c_str());
	char cin;
	size_t txtsize;
	std::string textin = "";
	char sizemsg[40];
	if (p && *p) {
		ifstream textfile;
		textfile.open(p);
		if (textfile) {
			TX.erase();
			TX.append(arqfile);
			p = fl_filename_name(p);
			TX.append(p);
			TX.append("\n");
			TX.append(arqascii);

			while (textfile.get(cin)) textin += (cin & 0xFF);
			textfile.close();
			if ( textin.find_first_not_of(AsciiChars) != std::string::npos) {
				fl_alert2("File contains non-ASCII bytes and must be sent as binary.");
				return;
			}

			txtsize = textin.length();
			arqPayloadSize = txtsize;
			blocksSent = 0;
			snprintf(sizemsg, sizeof(sizemsg), "ARQ:SIZE::%d\n", 
				static_cast<int>(txtsize));
			TX.append(sizemsg);
			TX.append(arqstart);
			TX.append(textin);
			TX.append(arqend);
			traffic = true;
			sendingfile = true;
			statusmsg = "Sending ASCII file: ";
			statusmsg.append(p);
				txtStatus->value(statusmsg.c_str());
			cbClearText();
			return;
		}
	}
	traffic = false;
	sendingfile = false;
}

void sendImageFile()
{
	if (arqstate < ARQ_CONNECTED) {
		fl_alert2("Not connected");
		return;
	}
	const char *p = FSEL::select(_("ARQ image file"), "Images\t*.{png,jpg,bmp}", "");
	char cin;
	size_t b64size;
	std::string textin = "";
	std::string b64text;
	base64 b64(true);
	char sizemsg[40];
	if (p && *p) {
		ifstream textfile;
		textfile.open(p, ios::binary);
		if (textfile) {
			TX.erase();
			TX.append(arqfile);
			p = fl_filename_name(p);
			TX.append(p);
			TX.append("\n");
			TX.append(arqbase64);
			while (textfile.get(cin))
				textin += cin;
			textfile.close();
			b64text = b64.encode(textin);
			b64size = b64text.length();
			snprintf(sizemsg, sizeof(sizemsg), "ARQ:SIZE::%d\n", 
				static_cast<int>(b64size));
			arqPayloadSize = b64size;
			blocksSent = 0;
			TX.append(sizemsg);
			TX.append(arqstart);
			TX.append(b64text);
			TX.append(arqend);
			traffic = true;
			sendingfile = true;
			statusmsg = "Sending image: ";
			statusmsg.append(p);
				txtStatus->value(statusmsg.c_str());
			cbClearText();
			return;
		}
	}
	traffic = false;
	sendingfile = false;
}

void sendBinaryFile()
{
	if (arqstate < ARQ_CONNECTED) {
		fl_alert2("Not connected");
		return;
	}
	const char *p = FSEL::select("ARQ file", "*", "");
	char cin;
	size_t b64size;
	std::string textin = "";
	std::string b64text;
	base64 b64(true);
	char sizemsg[40];
	if (p && *p) {
		ifstream textfile;
		textfile.open(p, ios::binary);
		if (textfile) {
			TX.erase();
			TX.append(arqfile);
			p = fl_filename_name(p);
			TX.append(p);
			TX.append("\n");
			TX.append(arqbase64);
			while (textfile.get(cin))
				textin += cin;
			textfile.close();
			b64text = b64.encode(textin);
			b64size = b64text.length();
			snprintf(sizemsg, sizeof(sizemsg), "ARQ:SIZE::%d\n", 
				static_cast<int>(b64size));
			arqPayloadSize = b64size;
			blocksSent = 0;
			TX.append(sizemsg);
			TX.append(arqstart);
			TX.append(b64text);
			TX.append(arqend);
			traffic = true;
			sendingfile = true;
			statusmsg = "Sending binary: ";
			statusmsg.append(p);
				txtStatus->value(statusmsg.c_str());
			cbClearText();
			return;
		}
	}
	traffic = false;
	sendingfile = false;
}

void send_xml_text(std::string fname, std::string txt)
{
	if (arqstate < ARQ_CONNECTED) {
		fl_alert2("Not connected");
		return;
	}

	size_t txtsize;
	char sizemsg[40];

	if (!txt.empty()) {
		TX.erase();
		TX.append(arqfile);
		TX.append(fname);
		TX.append("\n");
		TX.append(arqascii);
		txtsize = txt.length();
		arqPayloadSize = txtsize;
		blocksSent = 0;
		snprintf(sizemsg, sizeof(sizemsg), "ARQ:SIZE::%d\n", 
			static_cast<int>(txtsize));
		TX.append(sizemsg);
		TX.append(arqstart);
		TX.append(txt);
		TX.append(arqend);
		traffic = true;
		sendingfile = true;
		statusmsg = "Sending XML payload: ";
		statusmsg.append(fname);
			txtStatus->value(statusmsg.c_str());
		cbClearText();
		return;
	}
	traffic = false;
	sendingfile = false;
}

static char statemsg[80];

void dispState()
{
	static int last_state = DOWN;
	int currstate = digi_arq->state();
	static char xfrmsg[80];
	static char szPercent[10];

	arqstate = currstate & 0x7F;
	if (last_state != currstate) {
		last_state = currstate;
		if (arqstate == DOWN  || arqstate == TIMEDOUT) {
			if (btnCONNECT->active()) {
				btnCONNECT->label("Connect");
			}
			btnBEACON->activate();
			mnu->redraw();
		}
		else if (arqstate == ARQ_CONNECTED || arqstate == WAITING) {
			if (btnCONNECT->active())
				btnCONNECT->label("Disconnect");
			if (!autobeacon)
				btnBEACON->deactivate();
			mnuSend->activate();
			mnu->redraw();
		}
		if (rxARQfile || sendingfile) {
			if (btnCONNECT->active())
				btnCONNECT->label("Abort");
		}
		if (btnCONNECT->active())
			btnCONNECT->redraw_label();

		if (currstate <= 0x7F) { // receiving
			switch (currstate) {
				case ARQ_CONNECTING :
					snprintf(statemsg, sizeof(statemsg), "CONNECTING: %d", digi_arq->getTimeLeft());
					txtState->value(statemsg);
					txtState->redraw();
					autobeacon = WAIT;
					break;
				case WAITFORACK :
					snprintf(statemsg, sizeof(statemsg), "WAITING FOR ACK   ");
					txtState->value(statemsg);
					txtState->redraw();
					autobeacon = WAIT;
					break;
				case ABORT:
				case ABORTING :
					txtState->value("ABORTING XFR");
					txtState->redraw();
					autobeacon = WAIT;
					break;
				case WAITING :
				case ARQ_CONNECTED :
					char szState[80];
					snprintf(szState, sizeof(szState),"CONNECTED - Quality = %4.2f",
						digi_arq->quality());
					indCONNECT->color(FL_GREEN);
					indCONNECT->redraw();
					txtBeaconing->value("");
					txtState->value(szState);
					txtURCALL->value( digi_arq->urCall().c_str() );
					autobeacon = WAIT;
					break;
				case TIMEDOUT :
					indCONNECT->color(FL_WHITE);
					indCONNECT->redraw();
					txtState->value("TIMED OUT");
					txtStatus->value("");
					if (restart_beacon && autobeacon != ON) {
						autobeacon = ON;
						Fl::remove_timeout(arqAutoBeacon);
						Fl::add_timeout(1.0 + txdelay / 1000.0, arqAutoBeacon);
					}
					beaconrcvd = false;
					break;
				case DISCONNECT:
				case DISCONNECTING:
					txtState->value("DISCONNECTING");
					break;
				case DOWN :
				default :
					if (autobeacon != ON && restart_beacon) {
						btnBEACON->activate();
						Fl::remove_timeout(arqAutoBeacon);
						Fl::add_timeout(1.0 + txdelay / 1000.0, arqAutoBeacon);
					}
					indCONNECT->color(FL_WHITE);
					indCONNECT->redraw();
					autobeacon = ON;
					txtState->value("NOT CONNECTED");
					txtStatus->value("");
			}
		}
	}

	if (sendingfile == true) {
		if (digi_arq->transferComplete()) {
			time(&EndTime_t);
			TransferTime = difftime(EndTime_t,StartTime_t);
			snprintf(xfrmsg, sizeof(xfrmsg), "Transfer Completed in %4.0f sec's", TransferTime);
			txtStatus->value(xfrmsg);
			blocksSent = 0;
			prgStatus->value(0.0);
			prgStatus->label("");
			prgStatus->redraw();
			prgStatus->redraw_label();
			sendingfile = false;
		}
		else {
			prgStatus->value( digi_arq->percentSent());
			snprintf(szPercent, sizeof(szPercent), "%3.0f %%", 100.0 * digi_arq->percentSent());
			prgStatus->label(szPercent);
			prgStatus->redraw();
			prgStatus->redraw_label();
		}
	}
	if (SendingEmail == true) {
		if (digi_arq->transferComplete()) {
			time(&EndTime_t);
			TransferTime = difftime(EndTime_t,StartTime_t);
			snprintf(xfrmsg, sizeof(xfrmsg), "Transfer Completed in %4.0f sec's", TransferTime);
			txtStatus->value(xfrmsg);
			moveEmailFile();
			blocksSent = 0;
			prgStatus->value(0.0);
			prgStatus->label("");
			prgStatus->redraw();
			prgStatus->redraw_label();
			SendingEmail = false;
		}
		else {
			prgStatus->value( digi_arq->percentSent());
			snprintf(szPercent, sizeof(szPercent), "%3.0f %%", 100.0 * digi_arq->percentSent());
			prgStatus->label(szPercent);
			prgStatus->redraw();
			prgStatus->redraw_label();
		}
	}
}

void mainloop(void *)
{

	if (traffic) {
		digi_arq->sendText(TX);
		traffic = false;
		time(&StartTime_t);
	}
	dispState();
	if (rxTextReady) {
		if (haveemail)
			saveEmailFile();
		else if (!xml_rx_text_ready)
			saveRxFile();
	}
	Fl::repeat_timeout(0.1, mainloop);
}

void changeMyCall(const char *nucall)
{
	int currstate = digi_arq->state();
	if (currstate != DOWN)
		return;

	MyCall = nucall;
	for (size_t i = 0; i < MyCall.length(); i++)
		MyCall[i] = toupper(MyCall[i]);

	txtMyCall->value(MyCall.c_str());
	digi_arq->myCall(MyCall.c_str());

	std::string title = "flarq ";
	title.append(VERSION);
	title.append(" - ");
	title.append(MyCall);
	arqwin->label(title.c_str());

}

void changeBeaconText(const char *txt)
{
	beacontext = txt;
}

void TALKprint(std::string s)
{
	txtRX->insert(s.c_str());
	txtRX->show_insert_position();
	txtRX->redraw();
}

void clear_STATUS(void* arg)
{
	txtStatus2->value("");
}


void STATUSprint(std::string s, double disptime)
{
	txtStatus2->value(s.c_str());
	if (disptime > 0.0) {
		Fl::remove_timeout( clear_STATUS );
		Fl::add_timeout( disptime, clear_STATUS );
	}
}

void cbSendTalk()
{
	std::string tosend;
	tosend = txtTX->value();
	if (tosend.empty()) return;
	tosend += '\n';
	digi_arq->sendPlainText(tosend);
	txtTX->value("");
	txtRX->insert(tosend.c_str());
	txtRX->show_insert_position();
	txtRX->redraw();
}

void arqlog(std::string nom,std::string s)
{
	static char szGMT[80];
	tm *now;
	time_t tm;
	std::string strdebug;

	time(&tm);
	now = localtime( &tm );
	strftime(szGMT, sizeof(szGMT), "[%X] ", now);

	for (unsigned int i = 0; i < s.length(); i++)
		if (s[i] < 32)
			strdebug += ASCII[(int)s[i]];
		else
			strdebug += s[i];
	ofstream logfile(Logfile.c_str(), ios::app);
	if (logfile)
		logfile << nom << szGMT << strdebug << endl;
}

void DEBUGrxprint(std::string s)
{
	std::string text = noCR(s);
	txtRX->insert(text.c_str());
	txtRX->show_insert_position();
	txtRX->redraw();
	arqlog("<RX>",s);
}

void DEBUGtxprint(std::string s)
{
	std::string text = noCR(s);
	txtRX->insert(text.c_str());
	txtRX->show_insert_position();
	txtRX->redraw();
	arqlog("<TX>",s);
}

void TXecho(std::string s)
{
	blocksSent += s.length();
	std::string text = noCR(s);
	txtARQ->insert(text.c_str());
	txtARQ->show_insert_position();
	txtARQ->redraw();
}


void style_unfinished_cb(int, void*) {
}

void cbClearTalk()
{
	txtbuffRX->text("");
}

void cb_arqwin(Fl_Widget *, void*)
{
	arqCLOSE();
}

std::string pname;

int main (int argc, char *argv[] )
{
	sscanf(VERSION, "%f", &version);

	set_unexpected(handle_unexpected);
	set_terminate(diediedie);
	setup_signal_handlers();

	NBEMS_dir.clear();
	{
		std::string appname = pname = argv[0];
		std::string appdir = argv[0];

#ifdef __WIN32__
		size_t p = appdir.find("FL_APPS\\");
		if (p == std::string::npos) p = appdir.find("FL_APPS/");
		if (p == std::string::npos) {
			char dirbuf[FL_PATH_MAX + 1];
			fl_filename_expand(dirbuf, sizeof(dirbuf) -1, "$USERPROFILE/");
			NBEMS_dir.assign(dirbuf);
		} else
			NBEMS_dir.assign(appdir.substr(0, p + 8));
		NBEMS_dir.append("NBEMS.files/");
#else
		char dirbuf[FL_PATH_MAX + 1];
		fl_filename_absolute(dirbuf, sizeof(dirbuf), argv[0]);
		appdir.assign(dirbuf);
		size_t p = appdir.rfind("flarq");
		if (p != std::string::npos)
			appdir.erase(p);
		p = appdir.find("FL_APPS/");
		if (p != std::string::npos)
			NBEMS_dir.assign(appdir.substr(0, p + 8));
		else {
			fl_filename_expand(dirbuf, FL_PATH_MAX, "$HOME/");
			NBEMS_dir = dirbuf;
		}

		DIR *isdir = 0;
		std::string test_dir;
		test_dir.assign(NBEMS_dir).append("NBEMS.files/");
		isdir = opendir(test_dir.c_str());
		if (isdir) {
			NBEMS_dir = test_dir;
			closedir(isdir);
		} else {
			NBEMS_dir.append(".nbems/");
		}
#endif
		HomeDir = NBEMS_dir;
	}

	checkdirectories();

	Logfile = ARQ_dir;
	Logfile.append("/").append("arqlog.txt");

	set_platform_ui();

	generate_option_help();
	generate_version_text();

	int arg_idx;
	if (Fl::args(argc, argv, arg_idx, parse_args) != argc) {
		cerr << PACKAGE_NAME << ": bad option `" << argv[arg_idx]
		     << "'\nTry `" << PACKAGE_NAME
		     << " --help' for more information.\n";
		exit(EXIT_FAILURE);
	}

	createAsciiChars(); // allowable ASCII text chars for ".txt" type of files

	FSEL::create();
	arqwin = arq_dialog();
	arqwin->callback(cb_arqwin);
	arqwin->xclass(PACKAGE_TARNAME);

	// FL_NORMAL_SIZE may have changed; update the menu items
	for (int i = 0; i < menu_mnu->size() - 1; i++)
		if (menu_mnu[i].text)
			menu_mnu[i].labelsize(FL_NORMAL_SIZE);


	txtbuffRX = txtRX->buffer();
	txtRX->wrap_mode(1,80);

	txtbuffARQ = txtARQ->buffer();
	txtARQ->wrap_mode(1,80);

	digi_arq = new arq();

	try {
		tcpip = new Socket(Address(arq_address.c_str(), arq_port.c_str()));
		tcpip->set_timeout(0.01);
		tcpip->connect();
	}
	catch (const SocketException& e) {
		std::string errmsg;
		errmsg.append("Could not connect to modem program.\nPlease start ");
		if (ioMPSK)
			errmsg.append("MultiPSK");
		else
			errmsg.append("fldigi");
		errmsg.append(" before flarq.");
		fl_alert2("%s", errmsg.c_str());
		exit(EXIT_FAILURE);
	}

	if (ioMPSK)
		Fl::add_timeout(0.1, MPSK_Socket_rcv_loop);
	else
		Fl::add_timeout(0.1, Socket_rcv_loop);

// the following sequence of assigning callback functions is mandatory
// for the arq class to function
	if (ioMPSK)
		digi_arq->setSendFunc (MPSK_client_transmit);
	else
		digi_arq->setSendFunc (client_transmit);
	digi_arq->setGetCFunc (client_receive);
	digi_arq->setAbortedTransfer(abortedTransfer);
	digi_arq->setDisconnected(restart);
	digi_arq->setrxUrCall (rxBeaconCallsign);

	digi_arq->setPrintRX (payloadText);
	digi_arq->setPrintTX (TXecho);
	digi_arq->setPrintTALK (TALKprint);
	digi_arq->setPrintSTATUS (STATUSprint);

	if (SHOWDEBUG) {
		digi_arq->setPrintRX_DEBUG (DEBUGrxprint);
		digi_arq->setPrintTX_DEBUG (DEBUGtxprint);
	}

	digi_arq->start_arq();

	readConfig();

	std::string title = "flarq ";
	title.append(VERSION);
	title.append(" - ");
	title.append(MyCall);
	arqwin->label(title.c_str());

	arqwin->resize(mainX, mainY, mainW, mainH);

	txtBeaconing->value("Beacon Off");

	Fl::add_timeout(0.1, mainloop);

#ifdef __WOE32__
#  ifndef IDI_ICON
#    define IDI_ICON 101
#  endif
	arqwin->icon((char*)LoadIcon(fl_display, MAKEINTRESOURCE(IDI_ICON)));
#elif !defined(__APPLE__) && USE_X
	make_pixmap(&flarq_icon_pixmap, flarq_icon, argc, argv);
	arqwin->icon((char *)flarq_icon_pixmap);
#endif

	start_xml_server(FLARQ_XML_PORT);

	arqwin->show(argc, argv);
	return Fl::run();
}

static void checkdirectories(void)
{
	struct DIRS {
		std::string& dir;
		const char* suffix;
		void (*new_dir_func)(void);
	};

	DIRS NBEMS_dirs[] = {
		{ NBEMS_dir,         0, 0 },
		{ ARQ_dir,           "ARQ", 0 },
		{ ARQ_files_dir,     "ARQ/files", 0 },
		{ ARQ_recv_dir,      "ARQ/recv", 0 },
		{ ARQ_send_dir,      "ARQ/send", 0 },
		{ ARQ_mail_dir,      "ARQ/mail", 0 },
		{ ARQ_mail_in_dir,   "ARQ/mail/in", 0 },
		{ ARQ_mail_out_dir,  "ARQ/mail/out", 0 },
		{ ARQ_mail_sent_dir, "ARQ/mail/sent", 0 },
		{ WRAP_dir,          "WRAP", 0 },
		{ WRAP_recv_dir,     "WRAP/recv", 0 },
		{ WRAP_send_dir,     "WRAP/send", 0 },
		{ WRAP_auto_dir,     "WRAP/auto", 0 },
		{ ICS_dir,           "ICS", 0 },
		{ ICS_msg_dir,       "ICS/messages", 0 },
		{ ICS_tmp_dir,       "ICS/templates", 0 },
	};

	int r;

	for (size_t i = 0; i < sizeof(NBEMS_dirs)/sizeof(*NBEMS_dirs); i++) {
		if (NBEMS_dirs[i].suffix)
			NBEMS_dirs[i].dir.assign(NBEMS_dir).append(NBEMS_dirs[i].suffix).append("/");

		if ((r = mkdir(NBEMS_dirs[i].dir.c_str(), 0777)) == -1 && errno != EEXIST) {
			cerr << _("Could not make directory") << ' ' << NBEMS_dirs[i].dir
			     << ": " << strerror(errno) << '\n';
			exit(EXIT_FAILURE);
		}
		else if (r == 0 && NBEMS_dirs[i].new_dir_func)
			NBEMS_dirs[i].new_dir_func();
	}
}

// This function may be called by the QRZ thread
void cb_mnuVisitURL(Fl_Widget*, void* arg)
{
	const char* url = reinterpret_cast<const char *>(arg);
#ifndef __WOE32__
	const char* browsers[] = {
#  ifdef __APPLE__
		getenv("FLDIGI_BROWSER"), // valid for any OS - set by user
		"open"                    // OS X
#  else
		"fl-xdg-open",            // Puppy Linux
		"xdg-open",               // other Unix-Linux distros
		getenv("FLDIGI_BROWSER"), // force use of spec'd browser
		getenv("BROWSER"),        // most Linux distributions
		"sensible-browser",
		"firefox",
		"mozilla"                 // must be something out there!
#  endif
	};
	switch (fork()) {
	case 0:
#  ifndef NDEBUG
		unsetenv("MALLOC_CHECK_");
		unsetenv("MALLOC_PERTURB_");
#  endif
		for (size_t i = 0; i < sizeof(browsers)/sizeof(browsers[0]); i++)
			if (browsers[i])
				execlp(browsers[i], browsers[i], url, (char*)0);
		exit(EXIT_FAILURE);
	case -1:
		fl_alert2(_("Could not run a web browser:\n%s\n\n"
			 "Open this URL manually:\n%s"),
			 strerror(errno), url);
	}
#else
	// gurgle... gurgle... HOWL
	// "The return value is cast as an HINSTANCE for backward
	// compatibility with 16-bit Windows applications. It is
	// not a true HINSTANCE, however. The only thing that can
	// be done with the returned HINSTANCE is to cast it to an
	// int and compare it with the value 32 or one of the error
	// codes below." (Error codes omitted to preserve sanity).
	if ((INT_PTR)ShellExecute(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL) <= 32)
		fl_alert2(_("Could not open url:\n%s\n"), url);
#endif
}

void open_nbems_file_folder()
{
	cb_mnuVisitURL(0, (void*)NBEMS_dir.c_str());
}
