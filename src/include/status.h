#ifndef _status_H
#define _status_H

#include <iostream>
#include <fstream>
#include <string>
#include <list>

#include "main.h"
#include "globals.h"

struct status {
	int		lastmode;
	int		mainX;
	int		mainY;
	int		mainW;
	int		mainH;
	int		RxTextHeight;
	bool	rigShown;
	int		rigX;
	int		rigY;
	int		carrier;
	int		mag;
	int		speed;
	double	reflevel;
	double	ampspan;
	unsigned int	VIEWERnchars;
	unsigned int	VIEWERxpos;
	unsigned int	VIEWERypos;
	bool	VIEWERvisible;
	bool	LOGenabled;
	double  sldrSquelchValue;
	bool	afconoff;
	bool	sqlonoff;
	double	RcvMixer;
	double	XmtMixer;
	int		scopeX;
	int		scopeY;
	bool	scopeVisible;
	int		scopeW;
	int		scopeH;
	
	bool	bLastStateRead;
	
	
public:
	void saveModeState(trx_mode m);
	void initLastState();
	void saveLastState();
	void loadLastState();
	friend std::istream &operator>>(std::istream &stream, status &c);
	friend std::ostream &operator<<(std::ostream &ostream, status c);
};

extern status progStatus;

#endif
