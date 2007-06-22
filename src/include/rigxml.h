#ifndef XML_H
#define XML_H

#include <string>
#include <list>
#include <map>
#include <strstream>
#include <iostream>
#include <ctype.h>

struct MODE {
	string SYMBOL;
	string BYTES;
	MODE(string nm, string b) { SYMBOL = nm; BYTES = b;}
	MODE(string nm, char c) { SYMBOL = nm; BYTES += c;}
};

struct BW {
	string SYMBOL;
	string BYTES;
	BW(string nm, string b) { SYMBOL = nm; BYTES = b;}
	BW(string nm, char c) { SYMBOL = nm; BYTES += c;}
};

struct DATA {
	string dtype;
	int size;
	int max;
	int min;
	float resolution;
	bool reverse;
	int	andmask;
	int shiftbits;
	void clear() {
		size = 0;
		dtype = "";
		max = 199999999;
		min = 0;
		resolution = 1.0;
		reverse = false;
		andmask = 0xFF;
		shiftbits = 0;
	}
};

struct XMLIOS {
	string	SYMBOL;
	int 	size;
	string	str1;
	string	str2;
	DATA	data;
	int		fill1;
	int		fill2;
	string	info;
	string	ok;
	string	bad;
	void clear() { 
		SYMBOL = str1 = str2 = info = ok = bad = ""; 
		size = fill1 = fill2 = 0;
		data.clear();
	};
};

struct TAGS { char *tag; void (*fp)(size_t &);};

struct XMLRIG {
	string	SYMBOL;
	string	port;
	int		baud;
	bool	echo;
	int		timeout;
	int		retries;
	int		wait;
	bool	dtr;
	bool	dtrptt;
	bool	rts;
	bool	rtsptt;
	bool	rtscts;
	void clear() {
		SYMBOL = "";
		port = "";
		baud = 1200;
		echo = false;
		timeout = 200;
		retries = 4;
		wait = 0;
		dtr = false;
		dtrptt = false;
		rts = false;
		rtsptt = false;
		rtscts = false;
	}
};
	
extern list<XMLIOS> commands;
extern list<XMLIOS> reply;
extern list<MODE> lmodes;
extern list<MODE> lmodeCMD;
extern list<MODE> lmodeREPLY;
extern list<BW> lbws;
extern list<BW> lbwCMD;
extern list<BW> lbwREPLY;
extern list<string> LSBmodes;
extern XMLRIG rig;

extern bool readRigXML();

#endif

