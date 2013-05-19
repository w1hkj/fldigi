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
#include "fft.h"

#define RSID_SAMPLE_RATE 11025.0

#define RSID_FFT_SAMPLES 512
#define RSID_FFT_SIZE    1024
#define RSID_ARRAY_SIZE	 (RSID_FFT_SIZE * 2)

#define RSID_NSYMBOLS    15
#define RSID_RESOL       2
#define RSID_NTIMES      (RSID_NSYMBOLS * RSID_RESOL)
#define RSID_HASH_LEN    256
#define RSID_PRECISION   2.7 // detected frequency precision in Hz

// each rsid symbol has a duration equal to 1024 samples at 11025 Hz smpl rate
#define RSID_SYMLEN		(1024.0 / RSID_SAMPLE_RATE) // 0.09288 // duration of each rsid symbol

enum {
	RSID_BANDWIDTH_500 = 0,
	RSID_BANDWIDTH_1K,
	RSID_BANDWIDTH_WIDE,
};

struct RSIDs { unsigned short rs; trx_mode mode; const char* name; };

class cRsId {

protected:
// note: hamming distance > 5 causes false detection on second burst
enum { HAMMING_HIGH = 2, HAMMING_MED = 4, HAMMING_LOW = 5 };// 6 };
enum { INITIAL, EXTENDED, WAIT };

private:
	// Table of precalculated Reed Solomon symbols
	unsigned char   *pCodes;
	unsigned char   *pCodes2;

	static const RSIDs  rsid_ids[];
	static const int rsid_ids_size;
	static const int Squares[];
	static const int indices[];

	static const RSIDs  rsid_ids2[];
	static const int rsid_ids_size2;

	int rsid_secondary_time_out;

	int hamming_resolution;

// Span of FFT bins, in which the RSID will be searched for
	int		nBinLow;
	int		nBinHigh;
	float	aInputSamples[RSID_ARRAY_SIZE];
	double	fftwindow[RSID_ARRAY_SIZE];
	double  aFFTReal[RSID_ARRAY_SIZE];
	double	aFFTAmpl[RSID_FFT_SIZE];
	Cfft	*rsfft;

	// Hashing tables
	unsigned char	aHashTable1[RSID_HASH_LEN];
	unsigned char	aHashTable2[RSID_HASH_LEN];

	unsigned char	aHashTable1_2[RSID_HASH_LEN];
	unsigned char	aHashTable2_2[RSID_HASH_LEN];

	bool	bPrevTimeSliceValid;
	int		iPrevDistance;
	int		iPrevBin;
	int		iPrevSymbol;
	int		iTime; // modulo RSID_NTIMES
	int		aBuckets[RSID_NTIMES][RSID_FFT_SIZE];

	bool	bPrevTimeSliceValid2;
	int		iPrevDistance2;
	int		iPrevBin2;
	int		iPrevSymbol2;
	int		iTime2; // modulo RSID_NTIMES

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
	bool	search_amp2( int &pSymbolOut, int &pBinOut);
	void	search(void);
	void	apply (int iSymbol, int iBin);
	void	apply2 (int iSymbol, int iBin);
	void	setup_mode(int m);
public:
	cRsId();
	~cRsId();
	void	reset();
	void	receive(const float* buf, size_t len);
	void	send(bool postidle);

friend void reset_rsid(void *who);
};

#endif
