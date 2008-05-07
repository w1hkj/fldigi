#ifndef _status_H
#define _status_H

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

	void initLastState();
	void saveLastState();
	void loadLastState();
};

extern status progStatus;

#endif
