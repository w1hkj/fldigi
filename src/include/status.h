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
	bool	tbar_is_docked;

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
	unsigned int	fsqMONITORxpos;
	unsigned int	fsqMONITORypos;
	unsigned int	fsqMONITORwidth;
	unsigned int	fsqMONITORheight;
	int		tile_x;
	int		tile_w;
	int		tile_y;
	int		tile_h;
	double	tile_y_ratio;
	double	fsq_ratio;
	double	ifkp_ratio;
	bool	LOGenabled;
	double  sldrSquelchValue;
	double  sldrPwrSquelchValue;
	bool	afconoff;
	bool	sqlonoff;
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
	bool	cluster_connected;

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

	int		dxdialog_x;
	int		dxdialog_y;
	int		dxdialog_w;
	int		dxdialog_h;

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
	bool	shaped_rtty;
	bool	UOSrx;
	bool	UOStx;
// end Contestia, Olivia, RTTY state values

	std::string xmlrpc_address;
	std::string xmlrpc_port;
	std::string arq_address;
	std::string arq_port;
	std::string kiss_address;
	std::string kiss_io_port;
	std::string kiss_out_port;
	int kiss_dual_port_enabled;
	int data_io_enabled;
	bool ax25_decode_enabled;
	bool enableBusyChannel;
	int busyChannelSeconds;
	int kpsql_attenuation;
	bool csma_enabled;
	bool kiss_tcp_io;
	bool kiss_tcp_listen;
    bool kpsql_enabled;
    int csma_persistance;
    int csma_slot_time;
    int csma_transmit_delay;
    int psm_flush_buffer_timeout;
    int psm_minimum_bandwidth;
    int psm_minimum_bandwidth_margin;
    bool psm_use_histogram;
    int psm_histogram_offset_threshold;
    int psm_hit_time_window;
    int tx_buffer_timeout;
    bool kiss_io_modem_change_inhibit;
	bool ip_lock;
	double squelch_value;
    bool psk8DCDShortFlag;

	std::string	browser_search;

	bool	meters;

	bool	fsq_rx_abort;
	bool	ifkp_rx_abort;

	bool	bLastStateRead;

	void initLastState();
	void saveLastState();
	void loadLastState();

};

extern status progStatus;

#endif
