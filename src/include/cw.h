// ----------------------------------------------------------------------------
// cw.h  --  morse code modem
//
// Copyright (C) 2006-2009
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.  Adapted in part from code contained in 
// gmfsk source code distribution.
//  gmfsk Copyright (C) 2001, 2002, 2003
//  Tomi Manninen (oh2bns@sral.fi)
//  Copyright (C) 2004
//  Lawrence Glaister (ve7it@shaw.ca)
//
// Fldigi is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#ifndef _CW_H
#define _CW_H

#include <cstring>
#include <string>

#include "modem.h"
#include "filters.h"
#include "fftfilt.h"
#include "mbuffer.h"

#include "view_cw.h"

#define	CW_SAMPLERATE	8000

#define	CWMaxSymLen		4096		// AG1LE: - was 4096 

#define MAX_MORSE_ELEMENTS 6 // maximum of 6 elements in a Morse character 256

// CW function return status codes. 
#define	CW_SUCCESS	0
#define	CW_ERROR	-1

#define	ASC_NUL		'\0'	// End of string 
#define	ASC_SPACE	' '	// ASCII space char 

// Tone and timing magic numbers. 
#define	KWPM		(12 * CW_SAMPLERATE/10)		// # samples in dot = KWPM / WPM
#define CWKNUM 		((KWPM) / 10)				// 640, 1/2 dot length at 5 wpm

#define	TONE_SILENT	0	// 0Hz = silent 'tone' 
#define	USECS_PER_SEC	1000000	// Microseconds in a second 

#define	INITIAL_SEND_SPEED		18	// Initial send speed in WPM 
#define	INITIAL_RECEIVE_SPEED	18	// Initial receive speed in WPM 

// Initial adaptive speed threshold 
#define	INITIAL_THRESHOLD	(((KWPM) / INITIAL_RECEIVE_SPEED) * 2)

// Initial noise filter threshold 
#define	INITIAL_NOISE_THRESHOLD	(((KWPM) / CW_MAX_SPEED) / 2)

#define TRACKING_FILTER_SIZE 16

#define MAX_PIPE_SIZE (22 * CW_SAMPLERATE * 12 / 800)

enum CW_RX_STATE {
	RS_IDLE = 0,
	RS_IN_TONE,
	RS_AFTER_TONE
};

enum CW_EVENT {
	CW_RESET_EVENT,
	CW_KEYDOWN_EVENT,
	CW_KEYUP_EVENT,
	CW_QUERY_EVENT
};

class cw : public modem {

#define CLRCOUNT 16
#define	DEC_RATIO	16
// Maximum number of signs (dit or dah) in a Morse char.
#define WGT_SIZE 7

struct SOM_TABLE {
	std::string		rpr;			// The printable representation of the character
	float			wgt[WGT_SIZE];	// Dot-dash weight vector
};

protected:
	int			symbollen;		// length of a dot in sound samples (tx)
	int			fsymlen;        	// length of extra interelement space (farnsworth)
	double		phaseacc;		// used by NCO for rx/tx tones
	double		FFTphase;
	double		FFTvalue;
	unsigned int	smpl_ctr;		// sample counter for timing cw rx
	double		agc_peak;		// threshold for tone detection 

	bool		use_matched_filter;

	double		upper_threshold;
	double		lower_threshold;

	fftfilt			*cw_FFT_filter; // sinc / matched filter

	Cmovavg		*bitfilter;
	Cmovavg		*trackingfilter;

	int bitfilterlen;

	CW_RX_STATE		cw_receive_state;	// Indicates receive state 
	CW_RX_STATE		old_cw_receive_state;
	CW_EVENT		cw_event;			// functions used by cw process routine 
	 
	double pipe[MAX_PIPE_SIZE+1];			// storage for sync scope data
	double clearpipe[MAX_PIPE_SIZE+1];
	mbuffer<double, MAX_PIPE_SIZE + 1, 4> scopedata;
	int pipeptr;
	int pipesize;
	bool scope_clear;
	
// user configurable data - local copy passed in from gui
	int cw_speed;
	int cw_bandwidth;
	int cw_squelch;
	int cw_send_speed;				// Initially 18 WPM 
	int cw_receive_speed;				// Initially 18 WPM 
	bool usedefaultWPM;				// use default WPM
	
	int cw_upper_limit;
	int cw_lower_limit;
	
	long int cw_noise_spike_threshold;		// Initially ignore any tone < 10mS 
	int cw_in_sync;					// Synchronization flag 

// Sending parameters: 
	long int cw_send_dot_length;			// Length of a send Dot, in Usec 
	long int cw_send_dash_length;			// Length of a send Dash, in Usec
	int lastsym;					// last symbol sent
	double risetime;			    	// leading/trailing edge rise time (msec)
	int knum;					// number of samples on edges
	int qnum;					// number of samples on QSK signal edges
	int QSKshape;                   		// leading/trailing edge shape factor
	double qskbuf[OUTBUFSIZE];			// signal array for qsk drive
	double qskphase;				//
	bool firstelement;
	bool lastelement;
	double maxval;

//	double *keyshape;				// array defining leading edge
	
// Receiving parameters: 
	long int cw_receive_dot_length;		// Length of a receive Dot, in Usec 
	long int cw_receive_dash_length;		// Length of a receive Dash, in Usec 

// Receive buffering
	std::string rx_rep_buf;
	int cw_rr_current;				// Receive buffer current location 
	unsigned int cw_rr_start_timestamp;		// Tone start timestamp 
	unsigned int cw_rr_end_timestamp;		// Tone end timestamp 

	long int two_dots;		// 2-dot threshold for adaptive speed 
	int in_replay; 					//AG1LE: if we have replay even, set to 1 otherwise = 0 ; 

// Receive adaptive speed tracking.
	double dot_tracking;
	double dash_tracking;
	
	inline double nco(double freq);
	inline double qsknco();
	void	update_syncscope();
	void    clear_syncscope();
	void	update_Status();
	void	sync_parameters();
	void	reset_rx_filter();
	int		handle_event(int cw_event, std::string &sc);
	inline	int usec_diff(unsigned int earlier, unsigned int later);
	void	send_symbol(int symbol, int len, int state);
	void	send_ch(int c);
	bool 	tables_init();
	unsigned int tokenize_representation(char *representation);
	void	update_tracking(int dot, int dash);

	static const SOM_TABLE som_table[];
	float cw_buffer[512];
	int cw_ptr;
	int clrcount;

	double lowerwpm;
	double upperwpm;

	int synchscope;
	double noise_floor;
	double sig_avg;
	double siglevel;

	bool use_paren;
	std::string prosigns;

	cmplx mixer(cmplx in);

// transmit wave shaping
	int		nusymbollen;
	int		nufsymlen;
	int		kpre;
	int		kpost;

	double	wpm;
	double	fwpm;

	void	create_edges();
	void	sync_transmit_parameters();
	void	flush_audio();

	void send_CW(int);

	view_cw	viewcw;

public:
	cw();
	~cw();
	void	init();
	void	rx_init();
	void	tx_init();
	void	restart() {};

	int		rx_process(const double *buf, int len);
	void	rx_FFTprocess(const double *buf, int len);
	void	rx_FIRprocess(const double *buf, int len);
	void	decode_stream(double);

	int		tx_process();
	void	incWPM();
	void	decWPM();
	void	toggleWPM();

	int normalize(float *v, int n, int twodots);
	std::string	find_winner (float *inbuf, int twodots);

};

extern bool CW_table_changed;

extern bool CW_KEYLINE_isopen;
extern void close_CW_KEYLINE();
extern int open_CW_KEYLINE();
extern bool CW_KEYLINE_isopen;

extern void cwio_ptt(int on);
extern void cwio_key(int on);
extern void cal_cwio();
extern void cwio_display_calibration();
extern void calibrate_cwio();

extern void CAT_keying_calibrate();
extern void flrig_cwio_send(char);

extern pthread_mutex_t cwio_ptt_mutex;

#endif
