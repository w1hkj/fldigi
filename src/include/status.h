#ifndef _status_H
#define _status_H

#include <string>

#include "globals.h"

struct status {
	trx_mode	lastmode;
	int		mainX;
	int		mainY;
	int		mainW;
	int		mainH;
	int		RxTextHeight;
	bool	rigShown;
	int		rigX;
	int		rigY;
	int		rigW;
	int		rigH;
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
	string	LastMacroFile;
	bool	spot_recv;
	bool	spot_log;
	bool	contest;

	int		logbook_x;
	int		logbook_y;
	int		logbook_w;
	int		logbook_h;
	bool	logbook_reverse;
	int		logbook_col_0;
	int		logbook_col_1;
	int		logbook_col_2;
	int		logbook_col_3;
	int		logbook_col_4;
	int		logbook_col_5;

	bool	bLastStateRead;

	void initLastState();
	void saveLastState();
	void loadLastState();
	
};

extern status progStatus;

#endif
