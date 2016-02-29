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

#ifndef XML_H
#define XML_H

#include <string>
#include <list>
#include <vector>

using namespace std;

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
		dtype.clear();
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
		SYMBOL.clear();
		str1.clear();
		str2.clear();
		info.clear();
		ok.clear();
		bad.clear();
		size = fill1 = fill2 = 0;
		data.clear();
	}
};

struct TAGS { const char *tag; void (*fp)(size_t &);};

struct PAIR { 
	int val;
	int mtr;
	PAIR(int v, int s) { val = v; mtr = s; }
	void clear() {val = 0; mtr = 0; }
};

struct XMLRIG {
	XMLRIG() {
		clear();
	}

	std::string	port;
	string rigTitle;
	int		baud;
	int		stopbits;
	bool	dtr;
	bool	dtrptt;
	bool	rts;
	bool	rtsptt;
	bool	rtscts;
	bool	restore_tio;
	int     write_delay;
	int		init_delay;
	int		wait_for_device;
	int		post_write_delay;
	int		timeout;
	int		retries;
	bool	echo;
	bool	cmdptt;
	bool	vsp;
	bool	logstr;
	int		pollinterval;
	bool	use_smeter;
	vector< PAIR > smeter;
	bool	use_pwrmeter;
	vector< PAIR > pmeter;
	bool	use_notch;
	vector< PAIR > notch;
	bool	use_pwrlevel;
	vector< PAIR >pwrlevel;

	bool	debug;
	bool	noserial;
	bool	ascii;
	bool	xmlok;

	void clear() {
		port.clear();
		baud = 1200;
		stopbits = 2;
		dtr = false;
		dtrptt = false;
		rts = false;
		rtsptt = false;
		rtscts = false;
		restore_tio = true;
		echo = false;
		cmdptt = false;
		vsp = false;
		logstr = false;
		write_delay = 0;
		init_delay = 0;
		wait_for_device = 0;
		post_write_delay = 50;
		timeout = 200;
		retries = 5;
		rigTitle = "";
		pollinterval = 100;

		debug = false;
		noserial = false;
		ascii = false;
		xmlok = false;

		use_smeter = false;
		use_pwrmeter = false;
		use_pwrlevel = false;
		smeter.clear();
		pmeter.clear();
		pwrlevel.clear();
		notch.clear();
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
extern XMLRIG xmlrig;

extern bool readRigXML();
extern void	selectRigXmlFilename();
extern void loadRigXmlFile(void);
#endif

