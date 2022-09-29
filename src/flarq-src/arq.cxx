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

#include <sys/time.h>

#include <iostream>

#include "arq.h"

//=============================================================================
// status messages
//=============================================================================
std::string TXPOLL		= "TX: Send Blocks Report";
std::string STIMEDOUT	= "Timed out";
std::string ABORTXFR		= "ABORT transfer";
std::string RXIDENT		= "RX: Link Still Active";
std::string RXCONREQ		= "RX: Connect Request";
std::string RXCONACK		= "RX: Connect OK";
std::string RXDISCONN	= "RX: Disconnect Request";
std::string RXDISCONACK	= "RX: Disconnect OK";
std::string RXSTATUS		= "RX: Status Report";
std::string RXPOLL		= "RX: Send Blocks Report";
std::string TXSTATUS		= "TX: Blocks Received OK";
std::string TXDISCONN	= "TX: Disconnect Request";
std::string TXDISACK		= "TX: Disconnect OK";
std::string TXBEACON		= "TX: Beacon";
std::string TXCONNECT	= "TX: Connect";
std::string TXCONNACK	= "TX: Connect OK";
std::string TXIDENT		= "TX: Watchdog %d";

//bool bPoll = false;

std::string arq::upcase(std::string s)
{
	for (size_t i = 0; i < s.length(); i++)
		s[i] = toupper(s[i]);
	return s;
}

arq::arq()
{
	sendfnc = NULL;
	rcvfnc = NULL;
	printRX = NULL;
	abortfnc = NULL;
	disconnectfnc = NULL;
	qualityfnc = NULL;
	printRX_DEBUG = NULL;
	printTX_DEBUG = NULL;
	rxUrCall = NULL;

	Header.erase();

	MyStreamID = '0';
	UrStreamID = '0';

	UrCall.erase();
	MyCall.erase();

	logfile = "server.log";

// queues //
	TxTextQueue.clear();//erase();		// Text out to mail engine
	RxTextQueue.clear();//erase();		// Text in from mail engine
	TxPlainTextQueue.clear();//erase();
	RxFrameQueue.clear();//erase();
	lastRxChar = 0;
	TXflag = false;

	SessionNumber = 0;

	exponent = EXPONENT;
	maxheaders = MAXHEADERS;
	RetryTime = RETRYTIME;
	Retries = RETRIES;
	Timeout = TIMEOUT;
	TxDelay = TXDELAY;
	immediate = false;
	primary = false;

	setBufferlength();


// status variables //
//	totalRx = 0;
//	nbrbadRx = 0;
	totalTx = 0;
	nbrbadTx = 0;
	payloadlength = 32;		// Average length of payload received

// static status
	Firstsent = MAXCOUNT - 1; 		// First Header  I sent last turn
	LastHeader = MAXCOUNT - 1;		// Last Header I sent last turn
	Lastqueued = MAXCOUNT - 1;		// Last Header in static send queue

	EndHeader = MAXCOUNT - 1;		// Last  I received o.k.
	GoodHeader = MAXCOUNT - 1;		// Last Header received consecutive

// Ur status
	UrGoodHeader = MAXCOUNT - 1;	// Other station's Good Header
	UrLastHeader = MAXCOUNT - 1;	// Other station's Header last sent
	UrEndHeader = MAXCOUNT - 1;		// Other station's last received Header
	blkcount = -1;

	TXflag = false;					// TX on
	LinkState = DOWN;				// ARQ link is initially down
	Sending = 0;
	PollOK = false;
//	bABORT = false;

	MyMissing.clear();
	MissingRxBlocks = "";

	TxBlocks.clear();
	TxMissing.clear();
	TxPending.clear();

	RxPending.clear();

 	arqstop = false;

 	retries = baseRetries = Retries;
 	baseRetryTime = RetryTime;
 	baseTimeout = Timeout;

 	retrytime = RetryTime / ARQLOOPTIME;
 	timeout = Timeout / ARQLOOPTIME;
 	loopcount = 0;
 	set_idtimer();

 	tx2txdelay = 0;//TxDelay / ARQLOOPTIME;

//	srand(time(NULL));
}

void arq::setBufferlength()
{
	Bufferlength = 1;
	for (int i = 0; i < exponent; i++) Bufferlength *= 2;
	MyBlockLengthChar = '0' + exponent;
}

void arq::resetTx()
{
	Firstsent = MAXCOUNT - 1; 		// First Header  I sent last turn
	LastHeader = MAXCOUNT - 1;		// Last Header I sent last turn
	Lastqueued = MAXCOUNT - 1;		// Last Header in static send queue
	TxMissing.clear();
	TxBlocks.clear();
	TxPending.clear();
	TxTextQueue.clear();
//	UrMissing.clear();
}

void arq::resetRx()
{
	RxTextQueue.clear();//erase();		// Text in from mail engine
	RxFrameQueue.clear();//erase();
	lastRxChar = 0;
	EndHeader = MAXCOUNT - 1;		// Last  I received o.k.
	GoodHeader = MAXCOUNT - 1;		// Last Header I received conseq. o.k, 1st in send queue
	RxPending.clear();
	MissingRxBlocks = "";
}

void arq::reset()
{
	resetTx();
	resetRx();
	immediate = false;
	primary = false;
	blkcount = -1;
//	bABORT = false;
}

// new session number
// unknown stream id = 0
// known id's from 1 to 63
void arq::newsession()
{
	if (++SessionNumber == 64) SessionNumber = 1;
	MyStreamID = SessionNumber + '0';
}

// get new blocknumber
void arq::newblocknumber()
{
	Lastqueued++;
	Lastqueued %= MAXCOUNT;
}

// Checksum of header + Header
std::string arq::checksum(std::string &s)
{
	framecrc.reset();
	return framecrc.scrc16(s);
}

// Start header when MyStreamID has been assigned
void arq::newHeader()
{
	Header.erase();
	Header += SOH;
	Header += '0'; // protocol version;
}

void arq::IdHeader()
{
	newHeader();
	Header += UrStreamID;
}

void arq::UnkHeader()
{
	newHeader();
	Header += '0';
}

char crlf[3] = "  ";

void arq::addToTxQue(std::string s)
{
//	TxTextQueue += "\r\n";
	crlf[0] = 0x0D;
	crlf[1] = 0x0A;
	TxTextQueue.append(s);
	TxTextQueue.append(crlf);
	totalTx++;
}

// Connect (client:port, server:port)
// c Header = Client:port Server:port <streamnr. (0)> <max. Headerlen>
// e.g.: '00cW1HKJ:1025 KH6TY:24 4'
//
void arq::connectFrame()
{
	char szGlobals[24];
	reset();
	UnkHeader();
	Header += CONREQ;

	Payload.erase();
	Payload.append(MyCall);
	Payload.append(":1025");
	Payload += ' ';
	Payload.append(UrCall);
	Payload.append(":24");
	Payload += ' ';
	Payload += MyStreamID;
	Payload += ' ';
	Payload += MyBlockLengthChar;

	snprintf(szGlobals, 23, " T%dR%dW%d", Timeout/1000, Retries, RetryTime/1000);
	Payload.append(szGlobals);

	Frame = Header + Payload;

	Frame = Frame + checksum(Frame);
	Frame += EOT;

	addToTxQue(Frame);

	LinkState = ARQ_CONNECTING;
	printSTATUS(TXCONNECT, 5.0);

	set_idtimer();
}


// Connect acknowledge (server:port, client:port)
// k Header = Server:port Client:port <streamnr.> <max. Headerlen>
// e.g: '00kKH6TY:24 W1HKJ:1024 8 6'
//
void arq::ackFrame ()
{
	reset();
	IdHeader();
	Header += CONACK;

	Payload.erase();
	Payload.append(MyCall);
	Payload.append(":24");
	Payload += ' ';
	Payload.append(UrCall);
	Payload += ' ';
	Payload += MyStreamID;
	Payload += ' ';
	Payload += MyBlockLengthChar;

	Frame = Header + Payload;
	Frame = Frame + checksum(Frame);
	Frame += EOT;

	addToTxQue(Frame);
	printSTATUS(TXCONNACK, 5.0);
	set_idtimer();
}

// Connect (caller:port, static:port)
// c Header = Caller:port static:port <streamnr. (0)> <max. Headerlen>
// e.g.: '00cW1HKJ:87 KH6TY:87 4'
//
void arq::ttyconnectFrame()
{
	UnkHeader();
	Header += CONREQ;

	Payload.erase();
	Payload.append(MyCall);
	Payload.append(":87");
	Payload += ' ';
	Payload.append(UrCall);
	Payload.append(":87");
	Payload += ' ';
	Payload += MyStreamID;
	Payload += ' ';
	Payload += MyBlockLengthChar;

	Frame = Header + Payload;
	Frame = Frame + checksum(Frame);
	Frame += EOT;

	addToTxQue(Frame);
	set_idtimer();
}

// Connect acknowledge (server:port, client:port)
// k Header = Server:port Client:port <streamnr.> <max. Headerlen>
// e.g: '00kKH6TY:87 W1HKJ 4'
// Service id # 87 is keyboard-to-keyboard
//
void arq::ttyackFrame()
{
	IdHeader();
	Header += CONACK;

	Payload.erase();
	Payload.append(MyCall);
	Payload.append(":87");
	Payload += ' ';
	Payload.append(UrCall);
	Payload += ' ';
	Payload += MyBlockLengthChar;

	Frame = Header + Payload;
	Frame = Frame + checksum(Frame);
	Frame += EOT;

	addToTxQue(Frame);
	set_idtimer();
}

// Identify
//i frame = '00iKH6TY de W1HKJ'
void arq::identFrame()
{
	IdHeader();
	Header += IDENT;

	Payload.erase();
	Payload.append(UrCall);
	Payload.append(" de ");
	Payload.append(MyCall);

	Frame = Header + Payload;
	Frame = Frame + checksum(Frame);
	Frame += EOT;

	addToTxQue(Frame);

	char szIDENT[80];
	snprintf(szIDENT,sizeof(szIDENT), TXIDENT.c_str(), retries);
	printSTATUS(szIDENT, 5.0);

	set_idtimer();
}

// e.g. Ping frame
// u Header = From:port
// e.g: '00uKH6TY:7 '
void arq::pingFrame()
{
	IdHeader();
	Header += _UNPROTO;

	Payload.erase();
	Payload.append(MyCall);
	Payload.append(":7");
	Payload += ' ';

	Frame = Header + Payload;
	Frame = Frame + checksum(Frame);
	Frame += EOT;

	addToTxQue(Frame);
	set_idtimer();
}

// talk frame
// similar to UNPROTO frame
// but only sent if ARQ_CONNECTED
void arq::talkFrame(std::string txt)
{
	IdHeader();
	Header += _TALK;

	Payload.erase();
	Payload.append(MyCall);
	Payload.append(":73");
	Payload += ' ';
	if (txt.length() > (size_t)Bufferlength)
		Payload.append(txt.substr(0, Bufferlength));
	else
		Payload.append(txt);
	Frame = Header + Payload;
	Frame = Frame + checksum(Frame);
	Frame += EOT;

	addToTxQue(Frame);
	set_idtimer();
}

void arq::ackAbortFrame()
{
	IdHeader();
	Header += _ACKABORT;

	Payload.erase();
	Payload += (LastHeader + 0x20);
	Payload += (GoodHeader + 0x20);
	Payload += (EndHeader + 0x20);
	Payload.append(MissingRxBlocks);

	Frame = Header + Payload;
	Frame = Frame + checksum(Frame);
	Frame += EOT;

	addToTxQue(Frame);
	printSTATUS(TXSTATUS, 5.0);
}

// Status report (End, Good, Lastrx, Missing)
//p frame = <last Header tx><last Header rx ok><last Header rx> <missing Headers>
//e.g.: '00sXHCAB'
//
void arq::statFrame()
{
	if (idtimer && _idtimer <= 0) talkFrame("auto ID");

	IdHeader();
	Header += STATUS;

	Payload.erase();
	Payload += (LastHeader + 0x20);
	Payload += (GoodHeader + 0x20);
	Payload += (EndHeader + 0x20);
	Payload.append(MissingRxBlocks);

	Frame = Header + Payload;
	Frame = Frame + checksum(Frame);
	Frame += EOT;

	addToTxQue(Frame);
	printSTATUS(TXSTATUS, 5.0);
}

// Disconnect session
//d frame = ""
//e.g.: '00d'

void arq::disconnectFrame()
{
	IdHeader();
	Header += DISREQ;

	Payload.erase();
	Payload.append(MyCall);
	Payload.append(":90");
	Frame = Header + Payload;
	Frame = Frame + checksum(Frame);
	Frame += EOT;

	addToTxQue(Frame);
	printSTATUS(TXDISCONN, 5.0);
	set_idtimer();
}

void arq::disackFrame()
{
	IdHeader();
	Header += _DISACK;

	Payload.erase();
	Payload.append(MyCall);
	Payload.append(":91");
	Frame = Header + Payload;
	Frame = Frame + checksum(Frame);
	Frame += EOT;
	addToTxQue(Frame);
	printSTATUS(TXDISACK, 5.0);
	set_idtimer();
}

// ABORT session
//a frame = ""
//e.g.: '00a'
void arq::abortFrame()
{
	IdHeader();
	Header += _ABORT;

	Payload.erase();
	Payload.append(MyCall);
	Payload.append(":92");
	Frame = Header + Payload;
	Frame = Frame + checksum(Frame);
	Frame += EOT;

	addToTxQue(Frame);
	set_idtimer();
}

// Beacon frame
// u Header = From:port  data
// e.g: '00uKH6TY:72 Beacon text '
//
void arq::beaconFrame(std::string txt)
{
	UnkHeader();
	Header += _UNPROTO;

	Payload.erase();
	Payload.append(MyCall);
	Payload.append(":72");
	Payload += ' ';
	if (txt.length() > (size_t)Bufferlength)
		Payload.append(txt.substr(0, Bufferlength));
	else
		Payload.append(txt);
	Frame = Header + Payload;
	Frame = Frame + checksum(Frame);
	Frame += EOT;

	addToTxQue(Frame);
	printSTATUS(TXBEACON, 5.0);
	set_idtimer();
}

// poll
//p frame = <last Header tx><last Header rx ok><last Header rx> <missing Headers>
//e.g.: '00pXHCAB'
void arq::pollFrame()
{

	IdHeader();
	Frame = Header;
	Frame += POLL;
	Frame.append(MyCall);
	Frame += SUB;
	Frame += (LastHeader + 0x20);
	Frame += (GoodHeader + 0x20);
	Frame += (EndHeader + 0x20);
	Frame.append(MissingRxBlocks);
	Frame.append(checksum(Frame));
	Frame += EOT;

	addToTxQue(Frame);
	printSTATUS(TXPOLL, 5.0);
	set_idtimer();
}

// Text frame
void arq::textFrame(cTxtBlk block)
{
	IdHeader();
	Frame = Header;
	Frame += (block.nbr() + 0x20);
	Frame.append(block.text());
	Frame.append(checksum(Frame));
	Frame += SOH;

	addToTxQue(Frame);
}

//=====================================================================

void arq::parseIDENT()
{
	timeout = Timeout / ARQLOOPTIME;
	statFrame();
	immediate = true;
	printSTATUS(RXIDENT, 5.0);
}

void arq::parseCONREQ()
{

	size_t p1 = 0, p2 = rcvPayload.find(':');
	if (p2 == std::string::npos)
		return;
//	if (LinkState == ARQ_CONNECTED || LinkState == WAITFORACK) return; // disallow multiple connects

// requesting stations callsign
	UrCall = upcase(rcvPayload.substr(p1, p2 - p1));
	p1 = rcvPayload.find(' ', p2+1);
	if (p1 == std::string::npos) {
		UrCall.erase();
		return;
	}

	p1++;
	p2 = rcvPayload.find(":", p1);
	std::string testcall = upcase(rcvPayload.substr(p1, p2 - p1));
	if (testcall != MyCall) {
		UrCall.erase();
		return;
	}

	p1 = rcvPayload.find(' ', p2 +1);
	if (p1 == std::string::npos) {
		UrCall.erase();
		return;
	}

	p1++; // *p1 ==> StreamID for requesting station
	UrStreamID = rcvPayload[p1];
	p1++; // *p1 ==> requested block size
	UrBlockLengthChar = rcvPayload[p1];

	p1 += 3; // *p1 ==>" TnnnRnnnWnnn"
	if (p1 < rcvPayload.length()) {
		char num[7];
		if (rcvPayload[p1] == 'T') {
			int n = 0;
			while (rcvPayload[++p1] != 'R' && n < 6) num[n++] = rcvPayload[p1];
			num[n] = 0;
			sscanf(num, "%d", &Timeout);
			Timeout *= 1000;
			if (p1 < rcvPayload.length() && rcvPayload[p1] == 'R') {
				int n = 0;
				while (rcvPayload[++p1] != 'W' && n < 6) num[n++] = rcvPayload[p1];
				num[n] = 0;
				sscanf(num, "%d", &Retries);
				if (p1 < rcvPayload.length() && rcvPayload[p1] == 'W') {
					int n = 0;
					while (++p1 < rcvPayload.length() && n < 6) num[n++] = rcvPayload[p1];
					num[n] = 0;
					sscanf(num, "%d", &RetryTime);
					RetryTime *= 1000;
					Timeout += Retries * RetryTime;
				}
			}
/*
			char line[80];
			std::string NewValues = "Temporary control parameters set to\n";
			snprintf(line, 79, "  Retries   = %d\n", Retries);
			NewValues.append(line);
			snprintf(line, 79, "  Wait time = %d secs\n", RetryTime / 1000);
			NewValues.append(line);
			snprintf(line, 79, "  Timeout   = %d secs\n", Timeout / 1000);
			NewValues.append(line);
			printRX(NewValues);
*/
		}
	}

	reset();

	LinkState = WAITFORACK;
	newsession();

	if (rxUrCall) rxUrCall(UrCall);

	TxTextQueue.clear();
	ackFrame();
	immediate = true;
	printSTATUS(RXCONREQ, 5.0);

}

void arq::parseCONACK()
{
	if (LinkState < ARQ_CONNECTING ) { //!= ARQ_CONNECTING) {
		return; // Connect Acknowledge only valid during a connect
	}

	size_t p1 = 0, p2 = rcvPayload.find(':');
//	LinkState = DOWN;
	if (p2 == std::string::npos)
		return;
// requesting stations callsign
	UrCall = upcase(rcvPayload.substr(p1, p2 - p1));
	p1 = rcvPayload.find(' ', p2+1);
	if (p1 == std::string::npos) {
		UrCall.erase();
		return;
	}

	p1++;
	p2 = rcvPayload.find(" ", p1);
	std::string testcall = upcase(rcvPayload.substr(p1, p2 - p1));
	if (testcall != MyCall) {
		UrCall.erase();
		return;
	}

	p1++; // *p1 ==> StreamID for requesting station
	UrStreamID = rcvPayload[p1];
	p1++; // *p1 ==> requested block size
	UrBlockLengthChar = rcvPayload[p1];

	RxTextQueue.clear();//erase();

	LinkState = ARQ_CONNECTED;
	timeout = Timeout / ARQLOOPTIME;

	statFrame();
	immediate = true;
	primary = true;
	printSTATUS(RXCONACK, 5.0);
}

void arq::parseDISREQ()
{
	if (LinkState == DOWN) return;
	TxTextQueue.clear();//erase();
	TxMissing.clear();
	TxBlocks.clear();
	TxPlainTextQueue.clear();
	disackFrame();
	immediate = true;
	LinkState = DOWN;
	if (rxUrCall) rxUrCall("");
	if (disconnectfnc) disconnectfnc();
	printSTATUS(RXDISCONN, 5.0);
}

void arq::parseDISACK()
{
	if (rxUrCall) rxUrCall("");
	LinkState = DOWN;
	printSTATUS(RXDISCONACK, 5.0);
}

void arq::parseABORT()
{
	reset();
	if (abortfnc) abortfnc();
	ackAbortFrame();
	immediate = true;
	LinkState = ARQ_CONNECTED;
}

void arq::parseACKABORT()
{
	reset();
	if (abortfnc) abortfnc();
	LinkState = ARQ_CONNECTED;
}

void arq::parseUNPROTO()
{
	size_t p1 = 0, p2 = rcvPayload.find(':');
	if (p2 == std::string::npos)
		return;
// requesting stations callsign
	UrCall = upcase(rcvPayload.substr(p1, p2 - p1));
	if (rxUrCall) rxUrCall(UrCall);
	if (printRX) printRX(rcvPayload + "\n");
}

void arq::parseTALK()
{
	size_t p1 = rcvPayload.find(":73");
	if (p1 == std::string::npos)
		return;
	std::string talktext = rcvPayload.substr(p1 + 4);
	if (printTALK) printTALK(talktext);
}

void arq::parseSTATUS()
{
// create the missing list
// all reported missing blocks
	if (LinkState >= ARQ_CONNECTED) {
		UrLastHeader = rcvPayload[0] - 0x20;	// Other station's Header last sent
		UrGoodHeader = rcvPayload[1] - 0x20;	// Other station's Good Header
		UrEndHeader = rcvPayload[2] - 0x20;		// Other station's last received Header

		size_t nummissing = rcvPayload.length() - 3;
		std::vector<int> missing;
// append those reported missing
		if (nummissing > 0)
			for (size_t i = 0; i < nummissing; i++)
				missing.push_back(rcvPayload[i+3] - 0x20);
// append those not reported missing from UrEndHeader to LastHeader
		if (UrEndHeader != LastHeader) {
			int m = UrEndHeader + 1;
			if (m > MAXCOUNT) m -= MAXCOUNT;
			while (m != LastHeader) {
				missing.push_back(m);
				m++;
				if (m > MAXCOUNT) m -= MAXCOUNT;
			}
			missing.push_back(LastHeader);
		}

		if (missing.empty())
			TxMissing.clear();

		if (TxMissing.empty() == false) {
			std::list<cTxtBlk> keep;
			std::list<cTxtBlk>::iterator p = TxMissing.begin();
			while (p != TxMissing.end()) {
				for (size_t n = 0; n < missing.size(); n++) {
					if (p->nbr() == missing[n]) {
						keep.push_back(*p);
						break;
					}
				}
				p++;
			}
			TxMissing = keep;
		}
	}

// print any txpending blocks up to and including UrGoodHeader
	std::list<cTxtBlk>::iterator p1 = TxPending.begin();

	p1 = TxPending.begin();
	while (p1 != TxPending.end()) {
		if(p1->nbr() == UrGoodHeader) {
			if (printTX) printTX(p1->text());
			TxPending.erase(p1);
			break;
		} else
			if (printTX) printTX(p1->text());
		TxPending.erase(p1);
		p1 = TxPending.begin();
	}

	switch (LinkState) {
		case WAITFORACK :
			LinkState = ARQ_CONNECTED;
			break;
		case DISCONNECTING :
			if (rxUrCall) rxUrCall("");
			LinkState = DOWN;
			break;
		case WAITING :
			LinkState = ARQ_CONNECTED;
			break;
//		case ABORTING :
//			reset();
//			if (abortfnc) abortfnc();
//			LinkState = ARQ_CONNECTED;
//			break;
//		case ABORT :
//			break;
		default: break;
	}

	printSTATUS(RXSTATUS, 5.0);
}

void arq::parsePOLL()
{
	if (LinkState == DISCONNECTING || LinkState == DOWN ||
	    LinkState == TIMEDOUT || LinkState == ABORT )
	    return;

	statFrame();
	immediate = true;
	LinkState = ARQ_CONNECTED;
	printSTATUS(RXPOLL, 5.0);
}

void arq::parseDATA()
{
	std::vector<cTxtBlk>::iterator p1, p2;
	int n1, n2;

	if (LinkState < ARQ_CONNECTED) return; // do not respond if DOWN or TIMEDOUT

	for (p1 = RxPending.begin(); p1 < RxPending.end(); p1++)
		if (blknbr == p1->nbr()) {
			return;
		}

	char szStatus[80];
	snprintf(szStatus, sizeof(szStatus),"RX: data block %d", blknbr);
	printSTATUS(szStatus, 5.0);

	cTxtBlk tempblk(blknbr, rcvPayload);
	RxPending.push_back (tempblk);

	for (p1 = RxPending.begin(); p1 < RxPending.end() - 1; p1++) {
		n1 = p1->nbr();
		if (n1 < GoodHeader) n1 += MAXCOUNT;
		for (p2 = p1 + 1; p2 < RxPending.end(); p2++) {
			n2 = p2->nbr();
			if (n2 < GoodHeader) n2 += MAXCOUNT;
			if (n2 < n1) {
				tempblk = *p1;
				*p1 = *p2;
				*p2 = tempblk;
			}
		}
	}
// compute new EndHeader
	EndHeader = GoodHeader;
	if (!RxPending.empty()) {
		p1 = RxPending.end() - 1;
		EndHeader = p1->nbr();
	}

// add RxPending blocks that are consecutive to GoodHeader
	p1 = RxPending.begin();
	while (!RxPending.empty()) {
		if ((p1->nbr() != (GoodHeader +1) % MAXCOUNT))
			break;
		RxTextQueue.append(p1->text());
		GoodHeader = p1->nbr();
		if (printRX) printRX(p1->text());
		RxPending.erase(p1);
		p1 = RxPending.begin();
	}

	MissingRxBlocks = "";

	if (RxPending.empty())
		return;

	int start = (GoodHeader + 1)%MAXCOUNT;
	int end = (EndHeader + 1)%MAXCOUNT;
	int test;
	bool ok;
	if (end < start) end += MAXCOUNT;
	for (int i = start; i < end; i++) {
		test = (i % MAXCOUNT);
		ok = false;
		for (p1 = RxPending.begin(); p1 < RxPending.end(); p1++) {
			if (test == p1->nbr()) {
				ok = true;
				break;
			}
		}
		if (!ok)
			MissingRxBlocks += test + 0x20;
	}
}

bool arq::isUrcall()
{
	if (UrCall.empty())
		return false;
	if (rcvPayload.find(UrCall) != std::string::npos)
		return true;
	return false;
}


// expects to receive a full frame
// txt[0] == SOH
// txt[len - 3] ... txt[len] == CRC
// returns
//   -1  invalid frame
//   -n  failed CRC for text type n
//    n  valid frame
//       rcvPayload will contain the valid payload
//
int arq::parseFrame(std::string txt)
{
	if ( txt.length() < 8 ) {
		return -1; // not a valid frame
	}
	Ccrc16 testcrc;
	size_t len = txt.length();

	rcvPayload = txt.substr(4, len - 8);
	fID = txt[3];

// treat unproto TALK as a special case
// no effort made to confirm the data by the CRC value
	if (fID == _TALK) {
		if (LinkState >= ARQ_CONNECTED) {
		 	timeout = Timeout / ARQLOOPTIME;
			parseTALK();
			retries = Retries;
		}
		return -1;
	}

	std::string sRcvdCRC = testcrc.scrc16( txt.substr(0, len - 4));

	if (sRcvdCRC != txt.substr(len - 4) ) {
		if (printRX_DEBUG)
			printRX_DEBUG("CRC failed\n");
		return -1; // failed CRC test
	}

 	retries = Retries;

	switch (fID) {
		case IDENT :
			if (!isUrcall())
				break;
			blknbr = fID - 0x20;
			parseIDENT();
			if (printRX_DEBUG) {
				printRX_DEBUG("IDENT:");
			}
			break;
		case CONREQ :
			if (LinkState > TIMEDOUT)
				break; // disallow multiple connects
			blknbr = fID - 0x20;
			parseCONREQ();
			if (printRX_DEBUG) {
				printRX_DEBUG("CONREQ:");
			}
			break;
		case CONACK :
			if (!isUrcall())
				break;
			blknbr = fID - 0x20;
			parseCONACK();
			if (printRX_DEBUG) {
				printRX_DEBUG("CONACK:");
			}
			break;
		case DISREQ :
			if (!isUrcall())
				break;
			blknbr = fID - 0x20;
			parseDISREQ();
			if (printRX_DEBUG) {
				printRX_DEBUG("DISREQ:");
			}
			break;
		case _DISACK :
			if (!isUrcall())
				break;
			blknbr = fID - 0x20;
			parseDISACK();
			if (printRX_DEBUG) {
				printRX_DEBUG("DISACK: ");
			}
			break;
		case STATUS :
			if (LinkState == DOWN || LinkState == TIMEOUT)
				break;
			blknbr = fID - 0x20;
			parseSTATUS();
			if (printRX_DEBUG) {
				printRX_DEBUG("STATUS:");
			}
			break;
		case POLL :
			if (!isUrcall()) {
				break;
			}
			blknbr = fID - 0x20;
			parsePOLL();
			if (printRX_DEBUG) {
				printRX_DEBUG("POLL:");
			}
			break;
		case _ABORT :
			if (!isUrcall())
				break;
			blknbr = fID - 0x20;
			parseABORT();
			if (printRX_DEBUG) {
				printRX_DEBUG("RCVD ABORT:");
			}
			break;
		case _ACKABORT :
			blknbr = fID - 0x20;
			parseACKABORT();
			if (printRX_DEBUG) {
				printRX_DEBUG("RCVD ACK_ABORT:");
			}
			break;
		case _UNPROTO :
			if (LinkState >TIMEDOUT && !isUrcall())
				break; // disallow interruption
			blknbr = fID - 0x20;
			parseUNPROTO();
			if (printRX_DEBUG) {
				printRX_DEBUG("UNPROTO:");
			}
			break;
		default :
			blknbr = fID - 0x20;
			parseDATA();
			if (printRX_DEBUG) {
				printRX_DEBUG("DATA:");
			}
	}
	if (printRX_DEBUG) {
		printRX_DEBUG(txt); printRX_DEBUG("\n");
	}


	if (LinkState == ARQ_CONNECTED)
	 	timeout = Timeout / ARQLOOPTIME;

	return fID;
}


void arq::rcvChar( char c )
{
	if ( c == 0x06 ) {
		Sending = 0;
		retrytime = rtry();
		timeout = Timeout / ARQLOOPTIME;
		tx2txdelay = TxDelay / ARQLOOPTIME;
		return;
	}

	if (lastRxChar == SOH && c == SOH) // consecutive <SOH> characters
		return;

	if (lastRxChar == EOT && c == EOT) // consecutive <EOT> characters
		return;

	if (RxFrameQueue.empty()) {
		if (c == SOH)
			RxFrameQueue += c;
	} else {
		if (c == SOH || c == EOT) {
			parseFrame(RxFrameQueue);
			RxFrameQueue.clear();//erase();
			if (c == SOH) RxFrameQueue += c;
		} else
			RxFrameQueue += c;
	}

	lastRxChar = c;
}

//=====================================================================

void arq::sendText (std::string txt)
{
	size_t offset = 0;
	cTxtBlk tempblk;
	if (LinkState < ARQ_CONNECTED) return;

	Blocks2Send = 0;
	while (offset < txt.length()) {
		newblocknumber();
		tempblk.nbr(Lastqueued);
		tempblk.text(txt.substr(offset, Bufferlength));
		offset += Bufferlength;
		TxBlocks.push_back(tempblk);
		Blocks2Send++;
	}
}

void arq::sendblocks()
{
	char szStatus[80];
	int missedblks = 0, newblks = 0;
	int framecount = 0;
	cTxtBlk tempblk;

	if (TxMissing.empty() == false) {
		std::list<cTxtBlk>::iterator p = TxMissing.begin();
		while (p != TxMissing.end()) {
			textFrame(*p);
			p++;
			framecount++;
		}
	}
	missedblks = framecount;
	if (!TxBlocks.empty()) {
		while (TxBlocks.empty() == false && framecount < maxheaders) {
			tempblk = TxBlocks.front();
			if ((tempblk.nbr() + 2)%MAXCOUNT == UrGoodHeader)
				break;
			TxBlocks.pop_front();
			TxMissing.push_back(tempblk);
			TxPending.push_back(tempblk);
			textFrame(tempblk);
			LastHeader = tempblk.nbr();
			framecount++;
		}
	}
	newblks = framecount - missedblks;
	snprintf(szStatus, sizeof(szStatus),"TX: repeat %d new %d",
		missedblks, newblks);
	printSTATUS(szStatus, 0.0);

	if (!TxMissing.empty() || !TxBlocks.empty())
		pollFrame();

	if (LinkState != ABORT && LinkState != ABORTING)
		LinkState = WAITING;
}

void arq::connect(std::string callsign)
{
	UrCall = callsign;
	for (size_t i = 0; i < UrCall.length(); i++)
		UrCall[i] = toupper(UrCall[i]);

	if (rxUrCall) rxUrCall(UrCall);
	TxTextQueue.clear();
	connectFrame();
	LinkState = ARQ_CONNECTING;
	immediate = true;
}

void arq::disconnect()
{
	Retries = baseRetries;
	Timeout = baseTimeout;
	RetryTime = baseRetryTime;
	totalTx = 0;
	nbrbadTx = 0;

	LinkState = DISCONNECT;
}

void arq::abort()
{
//	bABORT = true;
	LinkState = ABORT;
}

void arq::sendBeacon (std::string txt)
{
	std::string deText = "<<< FLARQ Beacon >>> de ";
	deText.append(MyCall);
	TxTextQueue.clear();//erase();
	addToTxQue(deText);
	beaconFrame(txt);
	immediate = true;
	LinkState = DOWN;
}

void arq::sendPlainText( std::string txt )
{
	size_t p = 0;
	while (p < txt.length()) {
		talkFrame(txt.substr(p, Bufferlength));
		p += Bufferlength;
	}
}

void arq::transmitdata()
{
	if (TxTextQueue.empty() == false) {
		sendfnc(TxTextQueue);
		Sending = 0x80;
		if (printTX_DEBUG)
			printTX_DEBUG(TxTextQueue);
		TxTextQueue.clear();
		tx2txdelay = TxDelay / ARQLOOPTIME;
		return;
	}
	if (TxPlainTextQueue.empty() == false) {
		sendfnc(TxPlainTextQueue);
		Sending = 0x80;
		if (printTX_DEBUG)
			printTX_DEBUG(TxPlainTextQueue);
		TxPlainTextQueue.clear();
		tx2txdelay = TxDelay / ARQLOOPTIME;
	}
}

int arq::rtry()
{
	return RetryTime / ARQLOOPTIME;
//	return (RetryTime + rand() * 5000 / RAND_MAX) / ARQLOOPTIME;
}

//---------------------------------------------------------------------

void arqloop(void *who)
{
	arq *me = (arq *)who;
	char c;

	if (idtimer) me->_idtimer--;

// check for received chars including 0x06 for Sending = 0
	if (me->getc1(c) == true) {
		me->rcvChar(c);
		while (me->getc1(c) == true)
			me->rcvChar(c);
		if (me->tx2txdelay < me->TxDelay / ARQLOOPTIME)
			me->tx2txdelay = me->TxDelay / ARQLOOPTIME;
	}
	if (me->Sending == 0) { // not transmitting
// wait period between transmits
		if (me->tx2txdelay > 0) {
			me->tx2txdelay--;
		} else {
			if (me->immediate == true) {
				me->transmitdata();
				me->retrytime = me->rtry();
				me->retries = me->Retries;
				me->immediate = false;
			} else {
				switch (me->LinkState) {
				case  ARQ_CONNECTING :
					break;
				case  DISCONNECT :
					me->LinkState = DISCONNECTING;
					me->TxTextQueue.clear();
					me->TxMissing.clear();
					me->TxBlocks.clear();
					me->TxPlainTextQueue.clear();
					me->disconnectFrame();
					me->immediate = true;
					break;
				case  DISCONNECTING :
					if (me->retrytime-- == 0) {
						me->retrytime = me->rtry();
						if (--me->retries) {
							me->TxTextQueue.clear();
							me->TxMissing.clear();
							me->TxBlocks.clear();
							me->TxPlainTextQueue.clear();
							me->disconnectFrame();
							me->transmitdata();
							me->timeout = me->Timeout / ARQLOOPTIME;
						}
					}
					break;
				case ABORT :
					me->LinkState = ABORTING;
					me->TxTextQueue.clear();
					me->TxMissing.clear();
					me->TxBlocks.clear();
					me->TxPlainTextQueue.clear();
					me->tx2txdelay = 5000/ ARQLOOPTIME; // 5 sec delay for abort
					me->abortFrame();
					me->immediate = true;
					break;
				case ABORTING :
					if (--me->retrytime == 0) {
						me->retrytime = me->rtry();
						if (me->retries--) {
							me->TxTextQueue.clear();
							me->TxMissing.clear();
							me->TxBlocks.clear();
							me->TxPlainTextQueue.clear();
							me->abortFrame();
							me->transmitdata();
							me->timeout = me->Timeout / ARQLOOPTIME;
						}
					}
					break;
				case  WAITING :
					if (me->retrytime-- == 0) {
						me->retrytime = me->rtry();
						if (--me->retries) {
							me->TxTextQueue.clear();
							me->pollFrame();
							me->transmitdata();
							me->nbrbadTx++;
							me->timeout = me->Timeout / ARQLOOPTIME;
						}
					}
					break;
				case WAITFORACK :
					if (me->retrytime-- == 0) {
						me->retrytime = me->rtry();
						if (--me->retries) {
							me->TxTextQueue.clear();
							me->ackFrame();
							me->transmitdata();
							me->nbrbadTx++;
							me->timeout = me->Timeout / ARQLOOPTIME;
						}
					}
					break;

				case ARQ_CONNECTED :
				default:
					if (me->TxTextQueue.empty() == false) {
						me->transmitdata();
					} else if ( (me->TxMissing.empty() == false) || (me->TxBlocks.empty() == false)) {
						me->nbrbadTx += me->TxMissing.size();
						me->sendblocks();
						me->transmitdata();
					} else if ( me->TxPlainTextQueue.empty() == false ) {
						me->transmitdata();
					}
					break;
				}
				me->timeout--;
				if (me->timeout == 0 // 10000 / ARQLOOPTIME // 10 seconds remaining
				    && me->LinkState == ARQ_CONNECTED // link is connected
				    && me->primary == true ) { // this is the connecting station
					if (--me->retries) { // repeat Retries and then allow timeout
						me->TxTextQueue.clear();
						me->identFrame(); // send an identity frame to try to keep
						me->transmitdata(); // the link up
						me->timeout = me->rtry(); //10000 / ARQLOOPTIME + 1;
					}
				}
				if (me->timeout == 0) {
					if (me->LinkState == ARQ_CONNECTED)
						me->LinkState = TIMEDOUT;
					else
						me->LinkState = DOWN;
					me->Retries = me->baseRetries;
					me->Timeout = me->baseTimeout;
					me->RetryTime = me->baseRetryTime;

					me->retries = me->Retries;
					me->retrytime = me->rtry();
					me->TxMissing.clear();
					me->TxBlocks.clear();
					me->TxTextQueue.clear();
					me->TxPlainTextQueue.clear();
					me->timeout = me->Timeout / ARQLOOPTIME;
					if (me->rxUrCall) me->rxUrCall("");
				}
				if (me->retries == 0) {
					me->LinkState = DOWN;
					me->Retries = me->baseRetries;
					me->Timeout = me->baseTimeout;
					me->RetryTime = me->baseRetryTime;

					me->retries = me->Retries;
					me->retrytime = me->rtry();
					me->TxMissing.clear();
					me->TxBlocks.clear();
					me->TxTextQueue.clear();
					me->TxPlainTextQueue.clear();
					me->timeout = me->Timeout / ARQLOOPTIME;
					if (me->rxUrCall) me->rxUrCall("");
					me->printSTATUS(STIMEDOUT, 10.0);
				}
			}
		}
	}

	if (me->arqstop) {
		me->LinkState = STOPPED;
		me->arqstop = false;

		me->LinkState	= DOWN;
		me->Retries		= me->baseRetries;
		me->Timeout		= me->baseTimeout;
		me->RetryTime	= me->baseRetryTime;
		me->retries		= me->Retries;
		me->retrytime	= me->rtry();

		me->TxMissing.clear();
		me->TxBlocks.clear();
		me->TxTextQueue.clear();
		me->TxPlainTextQueue.clear();
		me->timeout = me->Timeout / ARQLOOPTIME;

		if (me->rxUrCall) me->rxUrCall("");

		me->printSTATUS(STIMEDOUT, 10.0);
		Fl::repeat_timeout(	1.0, arqloop, me);
		return;
	}

	Fl::repeat_timeout(	ARQLOOPTIME/1000.0, arqloop, me);
}


void arq::start_arq()
{
	Fl::add_timeout(1.0, arqloop, this);
}

void arq::restart_arq() { 
	arqstop = true;
}

//---------------------------------------------------------------------
