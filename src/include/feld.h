// ----------------------------------------------------------------------------
//	feld.h  --  FELDHELL modem
//
// Copyright (C) 2006-2010
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.  Adapted from code contained in gmfsk source code
// distribution.
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


#ifndef _FELD_H
#define _FELD_H

#include "modem.h"
#include "filters.h"
#include "fftfilt.h"
#include "mbuffer.h"

#define	FeldSampleRate	8000
#define FeldMaxSymLen	1024

#define	RxColumnLen	30
#define	TxColumnLen	14

#define	PIXMAP_W	14
#define	PIXMAP_H	(TxColumnLen)
#define MAXLEN 512

class feld : public modem {
enum FELD_STATE {PREAMBLE, POSTAMBLE, DATA};
protected:
//rx
	double rxphacc;
	double rxdelta;
	double rxcounter;
	double agc;
	double peakval;
	double peakhold;
	double minhold;
	
	double rxpixrate;
	double txpixrate;
	double downsampleinc;
	double upsampleinc;
	double phi2freq;

	C_FIR_filter	*hilbert;
	fftfilt			*bpfilt;
	Cmovavg			*bbfilt;
	Cmovavg			*minmaxfilt;
	Cmovavg			*average;
//tx
	FELD_STATE	tx_state;
	double txphacc;
	double txcounter;
	double hell_bandwidth;
	double filter_bandwidth;
	
	int depth;
	int dxmode;
	int halfwidth;
	bool blackboard;
	bool hardkeying;
	double feldcolumnrate;

	int preamble;
	int postamble;
	int prevsymb;
	cmplx prev;
	
	double OnShape[MAXLEN];
	double OffShape[MAXLEN];
	
	mbuffer<int, 2*RxColumnLen> col_data;
	int col_pointer;
	int fntnbr;
	
	cmplx mixer(cmplx);
	double nco(double);
	void	rx(cmplx);
	void	FSKHELL_rx(cmplx);
	void	send_symbol(int currsymbol, int nextsymbol);
	void	send_null_column();
	void	tx_char(char);
	void	initKeyWaveform();
public:
	feld(trx_mode);
	~feld();
	void	init();
	void	rx_init();
	void	tx_init(SoundBase *sc);
	void 	restart();
	int		rx_process(const double *buf, int len);
	int		tx_process();
	int		get_font_data(unsigned char c, int col);
};


#endif
