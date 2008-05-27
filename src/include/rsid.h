// ----------------------------------------------------------------------------
//
//	rsid.h
//
// Copyright (C) 2006
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.
//
// fldigi is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Tone separation: 10.766Hz
// Integer tone separator (x 16): 172
// Error on 16 tones: 0.25Hz

// Tone duration: 0.093 sec
// Tone duration, #samples at 8ksps: 743
// Error on 15 tones: negligible

// 1024 samples -> 512 tones
// 2048 samples, second half zeros

// each 512 samples new FFT
// ----------------------------------------------------------------------------

#ifndef RSID_H
#define RSID_H

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <sys/time.h>
#include <cstring>
#include <FL/Fl.H>

#include "globals.h"
#include "modem.h"
#include "fft.h"
#include "misc.h"

#define RSID_FFT_SIZE	1024
//#define RSID_FFT_SIZE	512
#define RSID_NSYMBOLS   15
//#define RSID_ARRAY_SIZE	1024
#define RSID_ARRAY_SIZE	(RSID_FFT_SIZE * 2)
#define RSID_NTIMES		(RSID_NSYMBOLS * 2)

// each rsid symbol has a duration equal to 1024 samples at 11025 Hz smpl rate
#define RSID_SYMLEN		(1024.0 / 11025.0) // 0.09288 // duration of each rsid symbol

enum {
	RSID_BANDWIDTH_500 = 0,
	RSID_BANDWIDTH_1K,
	RSID_BANDWIDTH_WIDE,
};

struct RSIDs { uchar rs; trx_mode mode; };

class cRsId {
private:
	int		_samplerate;
	// Table of precalculated Reed Solomon symbols
	uchar   *pCodes;
		
	static  RSIDs  rsid_ids[];
	int		rsid_ids_size;
	
	static const int Squares[];
	static const int indices[];
	
// Span of FFT bins, in which the RSID will be searched for
	int		nBinLow;
	int		nBinHigh;
	double	aInputSamples[RSID_ARRAY_SIZE];
	double	fftwindow[RSID_ARRAY_SIZE];
	double  aFFTReal[RSID_ARRAY_SIZE];
	double	aFFTAmpl[RSID_FFT_SIZE];

	// Hashing tables
	uchar	aHashTable1[256];
	uchar	aHashTable2[256];

	bool	bPrevTimeSliceValid;
	int		iPrevDistance;
	int		iPrevBin;
	int		iPrevSymbol;
	int		iTime; // modulo RSID_NTIMES
	int		aBuckets[RSID_NTIMES][RSID_FFT_SIZE];

	int		DistanceOut;
	int		MetricsOut;

// transmit
	double	phase;
	double	*outbuf;
	

private:
	void	Encode(int code, uchar *rsid);
	int		HammingDistance(int iBucket, uchar *p2);
	void	CalculateBuckets(const double *pSpectrum, int iBegin, int iEnd);
	bool	search_amp( int &pSymbolOut, int &pBinOut);
public:
	cRsId();
	~cRsId();
	bool	encode(trx_mode mode, int submode, uchar *rsid);
	bool	search( const double *pSamples, int nSamples );
//	, bool bReverse, 
//					int *pSymbolOut, int *pBinOut, int *pDistanceOut, 
//					int *pMetricsOut);
	void	apply (int iSymbol, int iBin);
	void	send();
	
	int		samplerate() { return _samplerate;}
	
};

#endif
