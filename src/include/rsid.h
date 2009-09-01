// ----------------------------------------------------------------------------
//
//	rsid.h
//
// Copyright (C) 2008, 2009
//		Dave Freese, W1HKJ
// Copyright (C) 2009
//		Stelios Bounanos, M0GLD
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

#include <samplerate.h>

#include "ringbuffer.h"
#include "globals.h"
#include "modem.h"

#define RSID_SAMPLE_RATE 11025.0

#define RSID_FFT_SAMPLES 512
#define RSID_FFT_SIZE    1024
#define RSID_ARRAY_SIZE	 (RSID_FFT_SIZE * 2)

#define RSID_NSYMBOLS    15
#define RSID_RESOL       2
#define RSID_NTIMES      (RSID_NSYMBOLS * RSID_RESOL)
#define RSID_HASH_LEN    256

// each rsid symbol has a duration equal to 1024 samples at 11025 Hz smpl rate
#define RSID_SYMLEN		(1024.0 / RSID_SAMPLE_RATE) // 0.09288 // duration of each rsid symbol

enum {
	RSID_BANDWIDTH_500 = 0,
	RSID_BANDWIDTH_1K,
	RSID_BANDWIDTH_WIDE,
};

struct RSIDs { unsigned char rs; trx_mode mode; };

class cRsId {
private:
	// Table of precalculated Reed Solomon symbols
	unsigned char   *pCodes;

	static const RSIDs  rsid_ids[];
	static const int rsid_ids_size;

	static const int Squares[];
	static const int indices[];

// Span of FFT bins, in which the RSID will be searched for
	int		nBinLow;
	int		nBinHigh;
	float	aInputSamples[RSID_ARRAY_SIZE];
	double	fftwindow[RSID_ARRAY_SIZE];
	double  aFFTReal[RSID_ARRAY_SIZE];
	double	aFFTAmpl[RSID_FFT_SIZE];

	// Hashing tables
	unsigned char	aHashTable1[RSID_HASH_LEN];
	unsigned char	aHashTable2[RSID_HASH_LEN];

	bool		bPrevTimeSliceValid;
	int		iPrevDistance;
	int		iPrevBin;
	int		iPrevSymbol;
	int		iTime; // modulo RSID_NTIMES
	int		aBuckets[RSID_NTIMES][RSID_FFT_SIZE];

	int		DistanceOut;
	int		MetricsOut;

// resample
	SRC_STATE* 	src_state;
	SRC_DATA	src_data;
	float*		inptr;
	static long	src_callback(void* cb_data, float** data);

// transmit
	double	*outbuf;
	size_t  symlen;

private:
	void	Encode(int code, unsigned char *rsid);
	int		HammingDistance(int iBucket, unsigned char *p2);
	void	CalculateBuckets(const double *pSpectrum, int iBegin, int iEnd);
	bool	search_amp( int &pSymbolOut, int &pBinOut);
	void	search(void);
	void	apply (int iSymbol, int iBin);
public:
	cRsId();
	~cRsId();
	void	reset();
	void	receive(const float* buf, size_t len);
	void	send(bool postidle);
};

#endif
