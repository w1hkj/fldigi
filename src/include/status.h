#ifndef _status_H
#define _status_H

#include <iostream>
#include <fstream>
#include <string>
#include <list>

#include "main.h"
#include "globals.h"

using namespace std;

struct status {
	trx_mode	lastmode;
public:
	void readLastState();
	void saveModeState(trx_mode m);
	void initLastState();
	friend std::istream &operator>>(std::istream &stream, status &c);
	friend std::ostream &operator<<(std::ostream &ostream, status c);
};

extern status progStatus;

#endif
