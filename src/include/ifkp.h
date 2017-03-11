// ----------------------------------------------------------------------------
// ifkp.h  --  BASIS FOR ALL MODEMS
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

#ifndef _IFKP_H
#define _IFKP_H

#include <string>

#include "trx.h"
#include "modem.h"
#include "complex.h"
#include "filters.h"
#include "picture.h"
#include <FL/Fl_Shared_Image.H>

class ifkp : public modem {

#define IFKP_FFTSIZE		4096
#define IFKP_SYMLEN			4096

#define IFKP_BLOCK_SIZE		IFKP_FFTSIZE
#define IFKP_SHIFT_SIZE		(IFKP_SYMLEN / 16)

//#define	IFKP_SR				8000 //16000

#define IFKP_SPACING		3
#define IFKP_OFFSET			1
#define IFKP_NUMBINS		151 // 3 bin spacing
#define IMAGESPP			8 //16 , 12, 8

enum IFKP_STATE {TEXT, IMAGE_START, IMAGE_SYNC, IMAGE};

public:
	int			symlen;

protected:
// Rx
	double			rx_stream[IFKP_BLOCK_SIZE + IFKP_SHIFT_SIZE];
	cmplx			fft_data[2*IFKP_FFTSIZE];
	double			a_blackman[IFKP_BLOCK_SIZE];
	C_FIR_filter	*rxfilter;
	double			tones[IFKP_NUMBINS];
	Cmovavg			*binfilt[IFKP_NUMBINS];
	int				movavg_size;
	int 			bkptr;
	g_fft<double>	*fft;
	Cmovavg			*snfilt;
	double			val;
	double			max;
	double			noise;
	double			noisepower;
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
	void			process_symbol(int);
	void			parse_pic(int);
	double			s2n;
	char			szestimate[40];
	std::string		station_calling;
	std::string		rx_text;
	std::string		toprint;
	std::string		pic_str;

	IFKP_STATE		state;

// Tx
//	C_FIR_filter	*xmtfilt;
	double			baud;
	int				tone;
	int				prevtone;
	double			txphase;
	bool			send_bot;
	void			transmit(double *buf, int len);
	void			send_char (int);
	void			send_idle ();
	void			send_symbol(int sym);
	void			send_tone(int tone);
	std::string		xmt_string;
	double			xmtdelay();

// RxTx
	int				basetone;
	double			metric;
	bool			ch_sqlch_open;
	std::string		mycall;

	void			show_mode();
	void			process_tones();

	bool			valid_char(int);

	std::string		heard_log_fname;
	std::string		audit_log_fname;
	std::ofstream	heard_log;
	std::ofstream	audit_log;

public:
//----
// virtual in base class
	ifkp (trx_mode md);
	~ifkp ();
	void	init ();
	void	rx_init ();
	void	restart ();
	void	tx_init (SoundBase *sc);
	int		rx_process (const double *buf, int len);
	int		tx_process ();

	void	set_freq(double);

	void	init_nibbles();
//----

// support for ifkp image transfers
private:
	double amplitude;
	double pixel;
	double sync;
	bool    TX_IMAGE;
	bool    TX_AVATAR;
	unsigned char tx_pixel;
	int tx_pixelnbr;
	int image_mode;
	bool b_ava;
public:
	int		byte;
	double	picf;
	double picpeak;
	C_FIR_filter *picfilter;
	Cmovavg *pixfilter;
	Cmovavg *ampfilter;
	Cmovavg *syncfilter;
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
static int		IMAGEspp;
	int		TXspp;
	void	recvpic(double smpl);
	void	send_image();
	void	send_avatar();
	void	ifkp_send_avatar();
	void	ifkp_send_image(std::string s = "");

};

#endif
