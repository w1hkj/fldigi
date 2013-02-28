#ifndef arq_H
#define arq_H

// ----------------------------------------------------------------------------
// arq module arq.h
// Copyright (c) 2007-2009, Dave Freese, W1HKJ
//
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

//	link layer spec for fldigi_arq
//
//	generic Frame format:
//	   <SOH>dcl[info])12EF<EOT|SOH>
//		|	||| |     |    |
//		|	||| |     |    +--ASCII <SOH> or <EOT> (0x04) character 
//		|	||| |     +-------checksum (4xAlphaNum)
//		|	||| +-------------Payload (1 ... 2^N chars, N 4, 5, 6, 7 8)
//		|	||+---------------Block type 
//		|	|+----------------Stream id
//		|	+-----------------Protocol version number
//		+---------------------ASCII <SOH> (0x01) character			  
// BLOCKSIZE = 2^n
//

#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <list>
#include <cctype>

#include <FL/Fl.H>

using namespace std;

#define DEBUG

#define arq_Version "arq 0.1"
//=====================================================================
// following Block Types are defined in K9PS ARQ Protocol specification
#define IDENT		'i'
#define CONREQ		'c'
#define CONACK		'k'
#define REFUSED		'r'
#define DISREQ		'd'
#define STATUS		's'
#define POLL		'p'
#define FMTFAIL		'f'
// following Block Types are extensions to the K9PS specification
#define _ABORT		'a'
#define _ACKABORT   'o'
#define _DISACK		'b'
#define _UNPROTO	'u'
#define _TALK	    't'
//=====================================================================
#define SOH			0X01
#define STX			0X02
#define ACK			0X06
#define SUB			0X1A
#define EOT			0X04
//=====================================================================
//ARQ defaults
#define	MAXHEADERS  	8	// Max. number of missing blocks
#define MAXCOUNT		64  // DO NOT CHANGE THIS CONSTANT
#define EXPONENT		7	// Bufferlength = 2 ^ EXPONENT = 128
//=====================================================================
//link timing defaults
#define	RETRIES			5
#define RETRYTIME		10000	// # milliseconds between retries
#define TXDELAY			500     // # milliseconds from xmt to rcv
#define TIMEOUT			60000	// # milliseconds before TIMED OUT
#define ARQLOOPTIME		100		// # msec for loop timing
//=====================================================================
//link states
#define DOWN		  0
#define TIMEDOUT	  1
#define ABORT         3
#define CONNECTING    4
#define CONNECTED	  5
#define WAITING		  6
#define WAITFORACK    7
#define DISCONNECT    8
#define DISCONNECTING 9
#define ABORTING      10

#define SENDING       0x80;
	
//=====================================================================

extern char *ARQASCII[];

// crc 16 cycle redundancy check sum for data block integrity

class Ccrc16 {
private:
	unsigned int crcval;
	char ss[5];
public:
	Ccrc16() { crcval = 0xFFFF; }
	~Ccrc16() {};
	void reset() { crcval = 0xFFFF;}
	unsigned int val() {return crcval;}
	string sval() {
		snprintf(ss, sizeof(ss), "%04X", crcval);
		return ss;
	}
	void update(char c) {
		crcval ^= c & 255;
        for (int i = 0; i < 8; ++i) {
            if (crcval & 1)
                crcval = (crcval >> 1) ^ 0xA001;
            else
                crcval = (crcval >> 1);
        }
	}
	unsigned int crc16(char c) { 
		update(c); 
		return crcval;
	}
	unsigned int crc16(string s) {
		reset();
		for (size_t i = 0; i < s.length(); i++)
			update(s[i]);
		return crcval;
	}
	string scrc16(string s) {
		crc16(s);
		return sval();
	}
};

// text block; block # and string of text
class cTxtBlk {
private:
	int number;
	string txt;
public:
	cTxtBlk() {number = -1; txt = "";}
	cTxtBlk(int n, string text) { number = n; txt = text; }
	~cTxtBlk() {}
	void nbr(int n) { number = n;}
	int nbr() { return number; }
	string text() { return txt; }
	void text(string t) { txt = t;}
	bool operator <(const cTxtBlk &b)const { return number < b.number; }
	bool operator ==(const cTxtBlk b)const { return number == b.number; }
};


class arq {

private:
	bool		arqstop;

	string	MyCall;
	string	UrCall;

	string	Header;
	string	Frame;
	string	Payload;
	string	rcvPayload;
	
	string	logfile;

	char	MyStreamID;
	char	UrStreamID;
	
	char	MyBlockLengthChar;
	char	UrBlockLengthChar;
	char	BlockNumberChar;
	char	fID;
	int		blknbr;
	
// queues //
	string	TxTextQueue;			// Text out to mail engine
	string	TxPlainTextQueue;		// plain text transmit queu
	string	RxTextQueue;			// Text in from mail engine
	string	RxFrameQueue;
	char	lastRxChar;
	bool	TXflag;

	int		Bufferlength;
	int		maxheaders;
	int		exponent;

// status variables
	int 	payloadlength;	// Average length of payload received
	int	totalRx;		// total number of frames received
	int	totalTx;		// total number of frames transmitted
	int	nbrbadRx;		// number with crc errors
	int	nbrbadTx;		// total number of repeats required
//	int 	max_idle;		// Dynamic timing slot initial value
	int	SessionNumber;
	bool	PollOK;			// used for status handshake
	bool	wrappedFlag;	// set true if missing blocks bit count
							// has wrapped around
	int	retrytime;
	int	RetryTime;
	int	retries;
	int	Retries;
	int	timeout;
	int	Timeout;
	int	tx2txdelay;
	int	TxDelay;
	int    loopcount;
	
	int	baseRetryTime;
	int	baseTimeout;
	int	baseRetries;
	
	bool	immediate;
	bool    primary;
	
	Ccrc16	framecrc;

// My status
	int		Firstsent;					// First Header  I sent last turn
	int		LastHeader;					// Last Header I sent last turn
	int		Lastqueued;					// Last Header in static send queue

	int		EndHeader;					// Last I received o.k.
	int		GoodHeader;					// Last Header received consecutively
	int		blkcount;
	int		Blocks2Send;				// number of blocks at beginning of Tx

	vector<int>	MyMissing;				// missing Rx blocks
	string MissingRxBlocks;
	vector<cTxtBlk> RxPending;			// RxPending Rx blocks (not consecutive)
	
	list<cTxtBlk> TxBlocks;				// fifo of transmit buffers
	list<cTxtBlk> TxMissing;			// fifo of sent; RxPending Status report
	list<cTxtBlk> TxPending;			// fifo of transmitted buffers pending print

// Ur status
	int		UrGoodHeader;				// Other station's Good Header
	int		UrLastHeader;				// Other station's Header last sent
	int		UrEndHeader;				// Other station's last received Header
	vector<int>	UrMissing;				// Other station's missing Headers

	int		LinkState;					// status of ARQ link
	int		Sending;
	
	bool	bABORT;
	
// Link quality for sending *** used for testing only !! ***
//	double	sendquality;
	
	
	void	reset();
	void	resetTx();
	void	resetRx();
	int     rtry();
	
	void	setBufferlength();
	
	void	checkblocks();
	string	upcase(string s);
	void	newblocknumber();
	void	newHeader();
	void	IdHeader();
	void	UnkHeader();

	void	connectFrame();
	void	disackFrame();
	void	ackFrame();
	void	ttyconnectFrame();
	void	ttyackFrame();
	void	pollFrame();
	void	identFrame();
	void	pingFrame();
	void	statFrame();
	void	disconnectFrame();
	void	abortFrame();
	void	ackAbortFrame();
	void	beaconFrame(string txt);
	void	textFrame(cTxtBlk block);
	void    talkFrame(string txt);
	
	void	addToTxQue(string s);
	
	void	sendblocks();
	void	transmitdata();
	
	string	frame() {return Frame;}

	bool	isUrcall();
	void	parseIDENT();
	void	parseCONREQ();
	void	parseCONACK();
	void	parseREFUSED();
	void	parseDISREQ();
	void	parseDISACK();
	void	parseABORT();
	void	parseACKABORT();
	void	parseUNPROTO();
	void	parseSTATUS();
	void	parsePOLL();
	void	parseDATA();
	void	parseTALK();

	int		parseFrame(string txt);
	
// external functions called by arq class	
	void	(*sendfnc)(const string& s);
	bool	(*getc1)(char &);
	void	(*rcvfnc)();
	void	(*printRX)(string s);
	void	(*printTX)(string s);
	void	(*printRX_DEBUG)(string s);
	void	(*printTX_DEBUG)(string s);
	void	(*printTALK)(string s);
	void	(*abortfnc)();
	void	(*disconnectfnc)();
	void	(*rxUrCall)(string s);
	void	(*qualityfnc)(string s);
	void	(*printSTATUS)(string s, double disptime);
	
public:
	arq();
	~arq() {};

	friend	void	arqloop(void *me);
	void	start_arq();

	string	checksum(string &s);

	void	myCall(string s) { MyCall = upcase(s);}
	string	myCall() { return MyCall;}
	
	void	urCall(string s) { UrCall = s;}
	string	urCall() { return UrCall;}
	
	void	newsession();
	
	void	setSendFunc( void (*f)(const string& s)) { sendfnc = f;}
	void	setGetCFunc( bool (*f)(char &)) { getc1 = f;}
	void	setRcvFunc( void (*f)()) { rcvfnc = f;}
	
	void	setPrintRX( void (*f)(string s)) { printRX = f;}
	void	setPrintTX( void (*f)(string s)) { printTX = f;}
	void	setPrintTALK (void (*f)(string s)) {printTALK = f;}
	void	setPrintRX_DEBUG (void (*f)(string s)){printRX_DEBUG = f;}
	void	setPrintTX_DEBUG (void (*f)(string s)) {printTX_DEBUG = f;}
	void	setPrintSTATUS (void (*f)(string s, double disptime)) { printSTATUS = f;}
	
	void	setMaxHeaders( int mh ) { maxheaders = mh; }
	void	setExponent( int exp ) { exponent = exp; setBufferlength(); }
	int		getExponent() { return (int) exponent;}
	void	setWaitTime( int rtime ) { RetryTime = rtime; baseRetryTime = rtime; }
	int		getWaitTime() { return (int) RetryTime; }
	void	setRetries ( int rtries ) { 
				retries = Retries = baseRetries = rtries; }
	int		getRetries() { return (int) Retries; }
	void	setTimeout ( int tout ) { Timeout = tout; baseTimeout = tout; }
	int		getTimeout() { return (int) Timeout; }
	int		getTimeLeft() { return (int) timeout * ARQLOOPTIME / 1000; }
	void	setTxDelay ( int r2t ) { TxDelay = r2t; }
	int		getTxDelay() { return (int) TxDelay; }
	int		getRetryCount() { return (int)(Retries - retries + 1); }

	void	setrxUrCall( void (*f)(string s)) { rxUrCall = f;}
	void	setQualityValue( void (*f)(string s)) { qualityfnc = f;}
	void	setAbortedTransfer( void (*f)()) { abortfnc = f;};
	void	setDisconnected( void (*f)()) { disconnectfnc = f;};
	
	void	rcvChar( char c );

	void	connect(string callsign);//, int blocksize = 6, int retries = 4);

	void	sendblocks( string txt );
	
	void	sendBeacon (string txt);
	void	sendPlainText( string txt );

	string	getText() { return RxTextQueue;}
	void	sendText(string txt);

	bool	connected() { return (LinkState == CONNECTED); }
	void	disconnect();
	void	abort();
	
	int		state() { return (LinkState + Sending);}
	
	int     TXblocks() { return totalTx;}
	int     TXbad() { return nbrbadTx;}
	int     RXblocks() { return totalRx;}
	int     RXbad() { return nbrbadRx;}
	
	double	quality() { 
		if (totalTx == 0) return 1.0;
		return ( 1.0 * (totalTx - nbrbadTx) / totalTx );
	}
	
	float  percentSent() {
		if (Blocks2Send == 0) return 0.0;
		if ((TxBlocks.empty() && TxMissing.empty())) return 1.0;
		return (1.0 * (Blocks2Send - TxBlocks.size() - TxMissing.size()) / Blocks2Send);
	}
	
	bool	transferComplete() {
		if (TxMissing.empty() == false) return false;
		if (TxBlocks.empty() == false) return false;
		return true;
	}
};

#endif

