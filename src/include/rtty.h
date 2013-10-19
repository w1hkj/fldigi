// ----------------------------------------------------------------------------
// rtty.h  --  RTTY modem
//
// Copyright (C) 2012
//		Dave Freese, W1HKJ
//		Stefan Fendt, DO2SMF
//
// This file is part of fldigi.
//
// This code bears some resemblance to code contained in gmfsk from which
// it originated.  Much has been changed, but credit should still be 
// given to Tomi Manninen (oh2bns@sral.fi), who so graciously distributed
// his gmfsk modem under the GPL.
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

#ifndef _RTTY_H
#define _RTTY_H

#include <iostream>

#include "complex.h"
#include "modem.h"
#include "globals.h"
#include "filters.h"
#include "fftfilt.h"
#include "digiscope.h"

#define	RTTY_SampleRate	8000
//#define RTTY_SampleRate 11025
//#define RTTY_SampleRate 12000

#define MAXPIPE			1024
#define MAXBITS			(2 * RTTY_SampleRate / 23 + 1)

#define	LETTERS	0x100
#define	FIGURES	0x200

#define dispwidth 100

enum RTTY_RX_STATE {
	RTTY_RX_STATE_IDLE = 0,
	RTTY_RX_STATE_START,
	RTTY_RX_STATE_DATA,
	RTTY_RX_STATE_PARITY,
	RTTY_RX_STATE_STOP,
	RTTY_RX_STATE_STOP2
};

enum RTTY_PARITY {
	RTTY_PARITY_NONE = 0,
	RTTY_PARITY_EVEN,
	RTTY_PARITY_ODD,
	RTTY_PARITY_ZERO,
	RTTY_PARITY_ONE
};

// simple oscillator-class
class Oscillator
{
public:
	Oscillator( double samplerate );
	~Oscillator() {}
	double Update( double frequency );

private:
	double m_phase;
	double m_samplerate;
};

class SymbolShaper
{
public:
	SymbolShaper(double baud = 45.45, double sr = 8000.0);
	~SymbolShaper() {}
	void reset();
	void Preset(double baud, double sr);
	void print_sinc_table();
	double Update( bool state );

private:
	int		 m_table_size;
	double*	 m_sinc_table;

	bool		m_State;
	double		m_Accumulator;
	long		m_Counter0;
	long		m_Counter1;
	long		m_Counter2;
	long		m_Counter3;
	long		m_Counter4;
	long		m_Counter5;
	double		m_Factor0;
	double		m_Factor1;
	double		m_Factor2;
	double		m_Factor3;
	double		m_Factor4;
	double		m_Factor5;
	double		m_SincTable[1024];

	double		baudrate;
	double		samplerate;
};

//enum TTY_MODE { LETTERS, FIGURES };

class rtty : public modem {
public:
	static const double SHIFT[];
	static const double BAUD[];
	static const int	BITS[];

private:

	Oscillator		*m_Osc1;
	Oscillator		*m_Osc2;
	SymbolShaper	*m_SymShaper1;
	SymbolShaper	*m_SymShaper2;

	double shift;
	int symbollen;
	int nbits;
	int stoplen;
	int msb;
	bool useFSK;

	double		phaseacc;
	double		rtty_squelch;
	double		rtty_shift;
	double		rtty_BW;
	double		rtty_baud;
	int 		rtty_bits;
	RTTY_PARITY	rtty_parity;
	int			rtty_stop;
	bool		rtty_msbfirst;

	double		mark_noise;
	double		space_noise;
	Cmovavg		*bits;
	bool		nubit;
	bool		bit;

	bool		bit_buf[MAXBITS];

	double mark_phase;
	double space_phase;
	fftfilt *mark_filt;
	fftfilt *space_filt;

	double *pipe;
	double *dsppipe;
	int pipeptr;

	cmplx mark_history[MAXPIPE];
	cmplx space_history[MAXPIPE];

	RTTY_RX_STATE rxstate;

	int counter;
	int bitcntr;
	int rxdata;
	double cfreq; // center frequency between MARK/SPACE tones
	double shift_offset; // 1/2 rtty_shift

	double prevsymbol;
	cmplx prevsmpl;

	double xy_phase;
	double rotate;

	cmplx QI[MAXPIPE];
	int inp_ptr;

	cmplx xy;

	bool   clear_zdata;
	double sigpwr;
	double noisepwr;
	double avgsig;

	double mark_mag;
	double space_mag;
	double mark_env;
	double space_env;
	double	noise_floor;

	double FSKbuf[OUTBUFSIZE];		// signal array for qrq drive
	double FSKphaseacc;
	double FSKnco();

	unsigned char lastchar;

	int rxmode;
	int txmode;
	bool preamble;

	void Clear_syncscope();
	void Update_syncscope();

	double IF_freq;
	inline cmplx mixer(double &phase, double f, cmplx in);

	unsigned char Bit_reverse(unsigned char in, int n);
	int decode_char();
	int rttyparity(unsigned int);
	bool rx(bool bit);
// transmit
	double nco(double freq);
	void send_symbol(int symbol, int len);
	void send_stop();
	void send_char(int c);
	void send_idle();
	int rttyxprocess();
	int baudot_enc(unsigned char data);
	char baudot_dec(unsigned char data);
	void Metric();

	bool is_mark_space(int &);
	bool is_mark();

public:
	rtty(trx_mode mode);
	~rtty();
	void init();
	void rx_init();
	void tx_init(SoundBase *sc);
	void restart();
	void reset_filters();
	int rx_process(const double *buf, int len);
	int tx_process();
	void flush_stream();

	void searchDown();
	void searchUp();

};

#endif
