#ifndef XML_H
#define XML_H

#include <string>
#include <list>
#include <map>
#include <sstream>
#include <iostream>
#include <ctype.h>

struct MODE {
	std::string SYMBOL;
	std::string BYTES;
	MODE(std::string nm, std::string b) { SYMBOL = nm; BYTES = b;}
	MODE(std::string nm, char c) { SYMBOL = nm; BYTES += c;}
};

struct BW {
	std::string SYMBOL;
	std::string BYTES;
	BW(std::string nm, std::string b) { SYMBOL = nm; BYTES = b;}
	BW(std::string nm, char c) { SYMBOL = nm; BYTES += c;}
};

struct DATA {
	std::string dtype;
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
	std::string	SYMBOL;
	int 	size;
	std::string	str1;
	std::string	str2;
	DATA	data;
	int		fill1;
	int		fill2;
	std::string	info;
	std::string	ok;
	std::string	bad;
	void clear() { 
		SYMBOL = str1 = str2 = info = ok = bad = ""; 
		size = fill1 = fill2 = 0;
		data.clear();
	};
};

struct TAGS { const char *tag; void (*fp)(size_t &);};

struct XMLRIG {
	std::string	SYMBOL;
	std::string	port;
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
	
extern std::list<XMLIOS> commands;
extern std::list<XMLIOS> reply;
extern std::list<MODE> lmodes;
extern std::list<MODE> lmodeCMD;
extern std::list<MODE> lmodeREPLY;
extern std::list<BW> lbws;
extern std::list<BW> lbwCMD;
extern std::list<BW> lbwREPLY;
extern std::list<std::string> LSBmodes;
extern XMLRIG rig;

extern bool readRigXML();
extern void	selectRigXmlFilename();

#endif

