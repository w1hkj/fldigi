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

#include <string>

#include <samplerate.h>

#include "ringbuffer.h"
#include "globals.h"
#include "modem.h"
#include "gfft.h"

#define RSID_SAMPLE_RATE 11025.0

#define RSID_FFT_SAMPLES 	512
#define RSID_FFT_SIZE		1024
#define RSID_ARRAY_SIZE	 	(RSID_FFT_SIZE * 2)
#define RSID_BUFFER_SIZE	(RSID_ARRAY_SIZE * 2)

#define RSID_NSYMBOLS    15
#define RSID_NTIMES      (RSID_NSYMBOLS * 2)
#define RSID_PRECISION   2.7 // detected frequency precision in Hz

// each rsid symbol has a duration equal to 1024 samples at 11025 Hz smpl rate
#define RSID_SYMLEN		(1024.0 / RSID_SAMPLE_RATE) // 0.09288 // duration of each rsid symbol

enum {
	RSID_BANDWIDTH_500 = 0,
	RSID_BANDWIDTH_1K,
	RSID_BANDWIDTH_WIDE,
};

typedef double rs_fft_type;
typedef std::complex<rs_fft_type> rs_cpx_type;

struct RSIDs { unsigned short rs; trx_mode mode; const char* name; };

class cRsId {

protected:
enum { INITIAL, EXTENDED, WAIT };

private:
	// Table of precalculated Reed Solomon symbols
	unsigned char   *pCodes1;
	unsigned char   *pCodes2;

	bool found1;
	bool found2;

	static const RSIDs  rsid_ids_1[];
	static const int rsid_ids_size1;
	static const int Squares[];
	static const int indices[];

	static const RSIDs  rsid_ids_2[];
	static const int rsid_ids_size2;

	int rsid_secondary_time_out;

	int hamming_resolution;

// Span of FFT bins, in which the RSID will be searched for
	int		nBinLow;
	int		nBinHigh;

	float			aInputSamples[RSID_ARRAY_SIZE * 2];
	rs_fft_type		fftwindow[RSID_ARRAY_SIZE];
	rs_cpx_type		aFFTcmplx[RSID_ARRAY_SIZE];
	rs_fft_type		aFFTAmpl[RSID_FFT_SIZE];

	g_fft<rs_fft_type>		*rsfft;

	bool	bPrevTimeSliceValid;
	int		iPrevDistance;
	int		iPrevBin;
	int		iPrevSymbol;

	int		fft_buckets[RSID_NTIMES][RSID_FFT_SIZE];

	bool	bPrevTimeSliceValid2;
	int		iPrevDistance2;
	int		iPrevBin2;
	int		iPrevSymbol2;

// resample
	SRC_STATE* 	src_state;
	SRC_DATA	src_data;
	int			inptr;
	static long	src_callback(void* cb_data, float** data);

// transmit
	double	*outbuf;
	size_t  symlen;
	unsigned short rmode;
	unsigned short rmode2;

private:
	void	Encode(int code, unsigned char *rsid);
	void	search(void);
	void	setup_mode(int m);

	void	CalculateBuckets(const rs_fft_type *pSpectrum, int iBegin, int iEnd);
	inline int		HammingDistance(int iBucket, unsigned char *p2);
	bool	search_amp( int &bin_out, int &symbol_out, unsigned char *pcode_table );
	void	apply ( int iBin, int iSymbol, int extended );

public:
	cRsId();
	~cRsId();
	void	reset();
	void	receive(const float* buf, size_t len);
	void	send(bool postidle);
	bool	assigned(trx_mode mode);

friend void reset_rsid(void *who);
};

#endif
