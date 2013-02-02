#ifndef _status_H
#define _status_H

#include <string>

#include "globals.h"

struct status {
	trx_mode	lastmode;
	std::string	mode_name;
	int		mainX;
	int		mainY;
	int		mainW;
	int		mainH;
	bool	WF_UI;
	bool	NO_RIGLOG;
	bool	Rig_Log_UI;
	bool	Rig_Contest_UI;
	bool	DOCKEDSCOPE;

	int		RxTextHeight;
	int		tiled_group_x;
	bool	show_channels;

	int		rigX;
	int		rigY;
	int		rigW;
	int		rigH;
	int		carrier;
	int		noCATfreq;
	std::string	noCATmode;
	std::string noCATwidth;
	int		mag;
	int		offset;
	int		speed;
	double	reflevel;
	double	ampspan;
	unsigned int	VIEWERnchars;
	unsigned int	VIEWERxpos;
	unsigned int	VIEWERypos;
	unsigned int	VIEWERwidth;
	unsigned int	VIEWERheight;
	double	VIEWER_psksquelch;
	double	VIEWER_rttysquelch;
	bool	VIEWERvisible;
	int		tile_x;
	int		tile_w;
	int		tile_y;
	int		tile_h;
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

	int		repeatMacro;
	float	repeatIdleTime;
	int		timer;
	int		timerMacro;
	std::string	LastMacroFile;
	int		n_rsids;

	bool	spot_recv;
	bool	spot_log;
	bool	contest;
	bool	quick_entry;
	bool	rx_scroll_hints;
	bool	rx_word_wrap;
	bool	tx_word_wrap;

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
	
// Contestia, Olivia, RTTY state values
	int		contestiatones;
	int		contestiabw;
	int		contestiamargin;
	int		contestiainteg;
	bool	contestia8bit;

	int		oliviatones;
	int		oliviabw;
	int		oliviamargin;
	int		oliviainteg;
	bool	olivia8bit;

	int		rtty_shift;
	int		rtty_custom_shift;
	int		rtty_baud;
	int		rtty_bits;
	int		rtty_parity;
	int		rtty_stop;
	bool	rtty_reverse;
	bool	rtty_crcrlf;
	bool	rtty_autocrlf;
	int		rtty_autocount;
	int		rtty_afcspeed;
	bool	rtty_filter_changed;
	bool	useFSKkeyline;
	bool	useFSKkeylineDTR;
	bool	FSKisLSB;
	bool	useUART;
	bool	PreferXhairScope;
	bool	PseudoFSK;
	bool	UOSrx;
	bool	UOStx;
// end Contestia, Olivia, RTTY state values

	std::string	browser_search;

	bool	bLastStateRead;

	void initLastState();
	void saveLastState();
	void loadLastState();

};

extern status progStatus;

#endif
