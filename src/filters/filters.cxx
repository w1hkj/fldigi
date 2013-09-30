// ----------------------------------------------------------------------------
//
// filters.cxx  --  Several Digital Filter classes used in fldigi
//
// Copyright (C) 2006-2008 Dave Freese, W1HKJ
//
// These filters are based on the gmfsk design and the design notes given in
// "Digital Signal Processing, A Practical Guid for Engineers and Scientists"
// by Steven W. Smith.
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

#include <config.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "filters.h"

#include <iostream>


//=====================================================================
// C_FIR_filter
//
//   a class of Finite Impulse Response (FIR) filters with 
//   decimate in time capability
//   
//=====================================================================

C_FIR_filter::C_FIR_filter () {
	pointer = counter = length = 0;
	decimateratio = 0;
	ifilter = qfilter = (double *)0;
	ffreq = 0.0;
}

C_FIR_filter::~C_FIR_filter() {
	if (ifilter) delete [] ifilter;
	if (qfilter) delete [] qfilter;
}

void C_FIR_filter::init(int len, int dec, double *itaps, double *qtaps) {
	length = len;
	decimateratio = dec;
	if (ifilter) {
		delete [] ifilter;
		ifilter = (double *)0;
	}
	if (qfilter) {
		delete [] qfilter;
		qfilter = (double *)0;
	}

	for (int i = 0; i < FIRBufferLen; i++)
		ibuffer[i] = qbuffer[i] = 0.0;
	
	if (itaps) {
            ifilter = new double[len];
		for (int i = 0; i < len; i++) ifilter[i] = itaps[i];
	}
	if (qtaps) {
		qfilter = new double[len];
		for (int i = 0; i < len; i++) qfilter[i] = qtaps[i];
	}

	pointer = len;
	counter = 0;
}


//=====================================================================
// Create a band pass FIR filter with 6 dB corner frequencies
// of 'f1' and 'f2'. (0 <= f1 < f2 <= 0.5)
//=====================================================================

double * C_FIR_filter::bp_FIR(int len, int hilbert, double f1, double f2)
{
	double *fir;
	double t, h, x;

	fir = new double[len];

	for (int i = 0; i < len; i++) {
		t = i - (len - 1.0) / 2.0;
		h = i * (1.0 / (len - 1.0));

		if (!hilbert) {
			x = (2 * f2 * sinc(2 * f2 * t) -
			     2 * f1 * sinc(2 * f1 * t)) * hamming(h);
		} else {
			x = (2 * f2 * cosc(2 * f2 * t) -
			     2 * f1 * cosc(2 * f1 * t)) * hamming(h);
// The actual filter code assumes the impulse response
// is in time reversed order. This will be anti-
// symmetric so the minus sign handles that for us.
			x = -x;
		}

		fir[i] = x;
	}

	return fir;
}

//=====================================================================
// Filter will be a lowpass with
// length = len
// decimation = dec
// 0.5 frequency point = freq
//=====================================================================

void C_FIR_filter::init_lowpass (int len, int dec, double freq) {
	double *fi = bp_FIR(len, 0, 0.0, freq);
	ffreq = freq;
	init (len, dec, fi, fi);
	delete [] fi;
}

//=====================================================================
// Filter will be a bandpass with
// length = len
// decimation = dec
// 0.5 frequency points of f1 (low) and f2 (high)
//=====================================================================

void C_FIR_filter::init_bandpass (int len, int dec, double f1, double f2) {
	double *fi = bp_FIR (len, 0, f1, f2);
	init (len, dec, fi, fi);
	delete [] fi;
}

//=====================================================================
// Filter will the Hilbert form
//=====================================================================

void C_FIR_filter::init_hilbert (int len, int dec) {
	double *fi = bp_FIR(len, 0, 0.05, 0.45);
	double *fq = bp_FIR(len, 1, 0.05, 0.45);
	init (len, dec, fi, fq);
	delete [] fi;
	delete [] fq;
}


//=====================================================================
// Run
// passes a cmplx value (in) and receives the cmplx value (out)
// function returns 0 if the filter is not yet stable
// returns 1 when stable and decimated cmplx output value is valid
//=====================================================================

int C_FIR_filter::run (const cmplx &in, cmplx &out) {
	ibuffer[pointer] = in.real();
	qbuffer[pointer] = in.imag();
	counter++;
	if (counter == decimateratio)
		out = cmplx (	mac(&ibuffer[pointer - length], ifilter, length),
						mac(&qbuffer[pointer - length], qfilter, length) );
	pointer++;
	if (pointer == FIRBufferLen) {
		/// memmove is necessary if length >= FIRBufferLen/2 , theoretically possible.
		memmove (ibuffer, ibuffer + FIRBufferLen - length, length * sizeof (double) );
		memmove (qbuffer, qbuffer + FIRBufferLen - length, length * sizeof (double) );
		pointer = length;
	}
	if (counter == decimateratio) {
		counter = 0;
		return 1;
	}
	return 0;
}

//=====================================================================
// Run the filter for the Real part of the cmplx variable
//=====================================================================

int C_FIR_filter::Irun (const double &in, double &out) {
	double *iptr = ibuffer + pointer;

	pointer++;
	counter++;

	*iptr = in;

	if (counter == decimateratio) {
		out = mac(iptr - length, ifilter, length);
	}

	if (pointer == FIRBufferLen) {
		iptr = ibuffer + FIRBufferLen - length;
		memcpy(ibuffer, iptr, length * sizeof(double));
		pointer = length;
	}

	if (counter == decimateratio) {
		counter = 0;
		return 1;
	}

	return 0;
}

//=====================================================================
// Run the filter for the Imaginary part of the cmplx variable
//=====================================================================

int C_FIR_filter::Qrun (const double &in, double &out) {
	double *qptr = ibuffer + pointer;

	pointer++;
	counter++;

	*qptr = in;

	if (counter == decimateratio) {
		out = mac(qptr - length, qfilter, length);
	}

	if (pointer == FIRBufferLen) {
		qptr = qbuffer + FIRBufferLen - length;
		memcpy(qbuffer, qptr, length * sizeof(double));
		pointer = length;
	}

	if (counter == decimateratio) {
		counter = 0;
		return 1;
	}

	return 0;
}


//=====================================================================
// Moving average filter
//
// Simple in concept, sublime in implementation ... the fastest filter
// in the west.  Also optimal for the processing of time domain signals
// characterized by a transition edge.  The is the perfect signal filter
// for CW, RTTY and other signals of that type.  For a given filter size
// it provides the greatest s/n improvement while retaining the sharpest
// leading edge on the filtered signal.
//=====================================================================

Cmovavg::Cmovavg (int filtlen)
{
	len = filtlen;
	in = new double[len];
	empty = true;
}

Cmovavg::~Cmovavg()
{
	if (in) delete [] in;
}

double Cmovavg::run(double a)
{
	if (!in) {
		return a;
	}
	if (empty) {
		empty = false;
		for (int i = 0; i < len; i++) {
			in[i] = a;
		}
		out = a * len;
		pint = 0;
		return a;
	}
	out = out - in[pint] + a;
	in[pint] = a;
	if (++pint >= len) pint = 0;
	return out / len;
}

void Cmovavg::setLength(int filtlen)
{
	if (filtlen > len) {
		if (in) delete [] in;
		in = new double[filtlen];
	}
	len = filtlen;
	empty = true;
}

void Cmovavg::reset()
{
	empty = true;
}

//=====================================================================
// Sliding FFT filter
// Sliding Fast Fourier Transform
//
// The sliding FFT ingeniously exploits the properties of a time-delayed 
// input and the property of linearity for its derivation.
//
// First of all, the N-point transform of a sequence x(n) is equal to the
// summation of the transforms of N separate transforms where each transform
// has just one of the original samples at it's original sample time.
//
//i.e.
//	  transform of [x0, x1, x2, x3, x4, x5,...xN-1]
// is equal to
//	  transform of [x0, 0, 0, 0, 0, 0,...0]
//	+ transform of [0, x1, 0, 0, 0, 0,...0]
//	+ transform of [0, 0, x2, 0, 0, 0,...0]
//	+ transform of [0, 0, 0, x3, 0, 0,...0]
//	.
//	.
//	.
//	+ transform of [0, 0, 0, 0, 0, 0,...xN-1]
//
// Secondly, the transform of a time-delayed sequence is a phase-rotated 
// version of the transform of the original sequence. i.e.
//
// If 		x(n) transforms to X(k),
// Then		x(n-m) transforms to X(k)(Wn)^(-mk)
//
// where N is the FFT size, m is the delay in sample periods, and WN is the 
// familiar phase-rotating coefficient or twiddle factor e^(-j2p/N)
//
// Therefore, if the N-point transform X(k) of an individual sample is considered,
// and then the sample is moved back in time by one sample period, all frequency 
// bins of X(k) are phase-rotated by 2pk/N radians.
//
// The important thing here is that the transform is not performed again because
// the previous frequency results can be used by simply application of the correct
// coefficients.
//
// This is the technique that is applied when the rectangular sampling window 
// slides along by one sample.  The contributions of all samples that are 
// included in both the original and the new windows are simply phase rotated. 
// The end effects are that the transform of the new sample must be added, and 
// the transform of the oldest sample that disappeared off the end must be 
// subtracted. These end-effects are easy to perform if we treat the new sample
// as occurring at time t = 0, because the transform of a single sample at t = 0,
// say (a + bj), simply has all frequency bins equal to (a + bj). Similarly, the
// oldest sample that has just disappeared off the end of the window is exactly N
// samples old. I.e. it occurred at t = -N. The transform of this sample, 
// say (c + dj), is also straightforward since every frequency bin has now been
// phase-rotated an integer number of times from when the sample was at t = 0. 
// (The kth frequency bin has been rotated by 2pk radians). The transform of the
// sample at t = -N is therefore the same as if it was still at t = 0. I.e. it
// has all frequency bins equal to (c + dj).
//
// All that is needed therefore is to 
//	phase rotate each frequency bin in F(k) by WN^(k) and then 
//	add [(a + bj) + (c + dj)] to each frequency bin. 
//
// One cmplx multiplication and two cmplx additions per frequency bin are 
// therefore required, per sample period, regardless of the size of the transform.
//
// For example, a traditional 1024-point FFT needs 5120 cmplx multiplies 
// and 10240 cmplx additions to calculate all 1024 frequency bins. A 1024-point 
// Sliding FFT however needs 1024 cmplx multiplies and 2048 cmplx additions 
// for all 1024 frequency bins, and as each frequency bin is calculated separately, 
// it is only necessary to calculate the ones that are of interest.
//
// One drawback of the Sliding FFT is that in using feedback from previous 
// frequency bins, there is potential for instability if the coefficients are not 
// infinitely precise. Without infinite precision, stability can be guaranteed by 
// making each phase-rotation coefficient have a  magnitude of slightly less than 
// unity. E.g. 0.9999. 
//
// This then has to taken into account when the Nth sample is subtracted, because 
// the factor 0.9999 has been applied N times to the transform of this sample. 
// The sample cannot therefore be directly subtracted, it must first be multiplied 
// by the factor of 0.9999^N. This unfortunately means there is another multipli-
// cation to perform per frequency bin. Another drawback is that a circular buffer
// is needed in which to keep N samples, so that the oldest sample, (from t= -N),
// can be subtracted each time.
//
// This filter is ideal for extracting a finite number of frequency bins
// with a very long kernel length.  The filter only needs to calculate the 
// values for the bins of interest and not the entire spectrum.  It does
// require the store of the history associated with those bins over the 
// kernel length.
//
// Use in the MFSK / DOMINO modem for extraction of the frequency spectra
//
//=====================================================================

struct sfft::vrot_bins_pair {
	cmplx vrot;
	cmplx bins;
} ;

sfft::sfft(int len, int _first, int _last)
{
	vrot_bins = new vrot_bins_pair[len];
	delay  = new cmplx[len];
	fftlen = len;
	first = _first;
	last = _last;
	ptr = 0;
	double phi = 0.0, tau = 2.0 * M_PI/ len;
	k2 = 1.0;
	for (int i = 0; i < len; i++) {
		vrot_bins[i].vrot = cmplx( K1 * cos (phi), K1 * sin (phi) );
		phi += tau;
		delay[i] = vrot_bins[i].bins = 0.0;
		k2 *= K1;
	}
}

sfft::~sfft()
{
	delete [] vrot_bins;
	delete [] delay;
}

// Sliding FFT, cmplx input, cmplx output
// FFT is computed for each value from first to last
// Values are not stable until more than "len" samples have been processed.
// Copies the frequencies to a pointer with a given stride.
void sfft::run(const cmplx& input, cmplx * __restrict__ result, int stride )
{
	cmplx & de = delay[ptr];
	const cmplx z( input.real() - k2 * de.real(), input.imag() - k2 * de.imag());
	de = input;

	++ptr ;
	if( ptr >= fftlen ) ptr = 0 ;

	// It is more efficient to have vrot and bins very close to each other.
	for(	vrot_bins_pair
			* __restrict__ itr = vrot_bins + first,
			* __restrict__ end = vrot_bins + last ;
		itr != end ;
		++itr, result += stride ) {
		*result = itr->bins = itr->bins * itr->vrot + z * itr->vrot;
	}
}

// ============================================================================
// Goertzel filter
// Optimized implementation of a DFT for a single frequency of interest
// SR = sample rate
// N = Block size (does not need to be a factor of 2!)
//     bin size = SR / N
// K = frequency bin of interest = (N * freq / SR)
//     N should be selected to make K an integer if possible
//
// Q0 = current sample
// Q1 = previous sample (1 delay)
// Q2 = previous sample (2 delay)
// w  = (2 * pi * K / N)
// k1 = cos(w)
// k2 = sin(w)
// k3 = 2.0 * k1

// Q0, Q1, Q2 are initialized to zero
// Iterate N times:
// Q0 = k3*Q1 - Q2 + sample
// Q2 = Q1
// Q1 = Q0
//
// After N interations:
// real = (Q1 - Q2 * k1)
// imag = Q2 * k2
// or
// mag = Q1*Q1 + Q2*Q2 - Q1*Q2*k1
// ============================================================================

goertzel::goertzel(int n, double freq, double sr)
{
	double w;
	w = 2 * M_PI * freq / sr;
	k1 = cos(w);
	k2 = sin(w);
	k3 = 2.0 * k1;
	Q0 = Q1 = Q2 = 0.0;
	count = N = n;
}

goertzel::~goertzel()
{
}

void goertzel::reset()
{
	Q0 = Q1 = Q2 = 0.0;
	count = N;
	isvalid = false;
}

void goertzel::reset(int n, double freq, double sr)
{
	double w;
	w = 2 * M_PI * freq / sr;
	k1 = cos(w);
	k2 = sin(w);
	k3 = 2.0 * k1;
	Q0 = Q1 = Q2 = 0.0;
	count = N = n;
	isvalid = false;
}

bool goertzel::run(double sample)
{
    Q0 = sample + k3*Q1 - Q2;
    Q2 = Q1;
    Q1 = Q0;
    if (--count == 0) {
	count = N;
	return true;
    }
    return false;
}

double goertzel::real()
{
    return ((0.5*k3*Q1 - Q2)/N);
}

double goertzel::imag()
{
    return ((k2*Q1)/N);
}

double goertzel::mag()
{
    return (Q2*Q2 + Q1*Q1 - k3*Q2*Q1);
}
