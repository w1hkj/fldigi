// ----------------------------------------------------------------------------
// fsq.h  -- FSQCALL compatible modem
//
// Copyright (C) 2006
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.
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

#ifndef _FSQ_H
#define _FSQ_H

#include <string>
#include <iostream>
#include <fstream>

#include "trx.h"
#include "modem.h"
#include "complex.h"
#include "filters.h"
#include "crc8.h"
#include "picture.h"
#include <FL/Fl_Shared_Image.H>

class fsq : public modem {

#define	SR			12000
#define FFTSIZE		4096
#define FSQ_SYMLEN	4096

#define NUMBINS		144 // 200 //((FFTSIZE / 4) - 2)

#define BLOCK_SIZE	FFTSIZE
#define SHIFT_SIZE	(FSQ_SYMLEN / 16)

enum STATE {TEXT, IMAGE};

friend void timed_xmt(void *);
friend void sounder(void *);
friend void aging(void *);
friend void fsq_add_tx_timeout(void *);
friend void fsq_stop_aging();
friend void try_transmit(void *);
friend void fsq_transmit(void *);

public:

protected:
// Rx
	double			rx_stream[BLOCK_SIZE + SHIFT_SIZE];
	cmplx			fft_data[2*FFTSIZE];
	double			a_blackman[BLOCK_SIZE];
	double			tones[NUMBINS];
	Cmovavg			*binfilt[NUMBINS];
	int				movavg_size;
	int 			bkptr;
	g_fft<double>	*fft;
	Cmovavg			*snfilt;
	Cmovavg			*sigfilt;
	Cmovavg			*noisefilt;
	Cmovavg			*baudfilt;
	double			val;
	double			max;
	double			noise;
	int				peak;
	int				prev_peak;
	int				last_peak;
	int				peak_counter;
	int				peak_hits;
	int				symbol;
	int				prev_symbol;
	int				curr_nibble;
	int				prev_nibble;
	int				nibbles[199];
	void			lf_check(int);
	void			process_symbol(int);
	double			s2n;
	char			szestimate[40];
	std::string		rx_text;
	std::string		toprint;
	int				valid_callsign(std::string s);
	void			parse_rx_text();
	void			parse_space(bool);
	void			parse_qmark(std::string relay = "");
	void			parse_star();
	void			parse_repeat();
	void			parse_delayed_repeat();
	void			parse_pound(std::string relay = "");
	void			parse_dollar(std::string relay = "");
	void			parse_at(std::string relay = "");
	void			parse_amp(std::string relay = "");
	void			parse_carat(std::string relay = "");
	void			parse_pcnt();
	void			parse_vline(std::string relay = "");
	void			parse_greater(std::string relay = "");
	void			parse_less(std::string relay = "");
	void			parse_plus(std::string relay = "");
	void			parse_minus();
	void			parse_relay();
	void			parse_relayed();

	bool			b_bot;
	bool			b_eol;
	bool			b_eot;

// Tx
//	C_FIR_filter	*xmtfilt;
	int				tone;
	int				prevtone;
	double			txphase;
	void			send_string(std::string);
	bool			send_bot;
	void			flush_buffer ();
	void			send_char (int);
	void			send_idle ();
	void			send_symbol(int sym);
	void			send_tone(int tone);
	void			reply(std::string);
	void			delayed_reply(std::string, int delay);
	void			send_ack(std::string relay = "");

// Sounder
	double			sounder_interval;
	void			start_sounder(int); // 0, 1, 2, 3
	void			stop_sounder();

// Aging
	void			start_aging();
	void			stop_aging();

// RxTx
	int				fsq_frequency;  // 0 / 1
	int				spacing;
	int				basetone;
	int				tx_basetone;
	double			speed;
	double			metric;
	bool			ch_sqlch_open;
	CRC8			crc;
	std::string		station_calling;
	std::string		mycall;
	std::string		heard_log_fname;
	std::string		audit_log_fname;
	std::ofstream	heard_log;
	std::ofstream	audit_log;

	void			show_mode();
	void			adjust_for_speed();
	void			process_tones();

	void init_nibbles();

	void			set_freq(double);

	bool			valid_char(int);

	STATE			state;

public:
	fsq (trx_mode md);
	~fsq ();
	void	init ();
	void	rx_init ();
	void	restart ();
	void	tx_init (SoundBase *sc);
	int		rx_process (const double *buf, int len);

	int		tx_process ();

	std::string fsq_mycall() { return mycall; }

	bool	fsq_squelch_open();

	static int		symlen;

// support for fsq image transfers
private:
	double amplitude;
	double pixel;
	unsigned char tx_pixel;
	int tx_pixelnbr;
	int image_mode;

public:
	int		byte;
	double	picf;
	double picpeak;

	C_FIR_filter *picfilter;
	double phidiff;
	double phase;
	cmplx	prevz;
	cmplx	currz;

	double image_freq[10];
	int		image_counter;
	int		picW;
	int		picH;
	int		row;
	int		col;
	int		rgb;
	int		pixelnbr;
	int		RXspp;
	int		TXspp;
	void	recvpic(double smpl);
	void	send_image();
	void	fsq_send_image(std::string s);

};

#endif
