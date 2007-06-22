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
	int		lastmode;
	int		mainX;
	int		mainY;
	int		mainW;
	int		mainH;
	bool	rigShown;
	int		rigX;
	int		rigY;
public:
	void saveModeState(trx_mode m);
	void initLastState();
	void saveLastState();
	friend std::istream &operator>>(std::istream &stream, status &c);
	friend std::ostream &operator<<(std::ostream &ostream, status c);
};

extern status progStatus;

#endif
