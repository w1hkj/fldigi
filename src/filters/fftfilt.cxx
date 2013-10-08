// ----------------------------------------------------------------------------
//	fftfilt.cxx  --  Fast convolution Overlap-Add filter
//
// Filter implemented using overlap-add FFT convolution method
// h(t) characterized by Windowed-Sinc impulse response
//
// Reference:
//	 "The Scientist and Engineer's Guide to Digital Signal Processing"
//	 by Dr. Steven W. Smith, http://www.dspguide.com
//	 Chapters 16, 18 and 21
//
// Copyright (C) 2006-2008 Dave Freese, W1HKJ
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

#include <memory.h>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cmath>
#include <typeinfo>

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <memory.h>

#include "misc.h"
#include "fftfilt.h"

//------------------------------------------------------------------------------
// initialize the filter
// create forward and reverse FFTs
//------------------------------------------------------------------------------

// probably only need a single instance of g_fft !!
// use for both forward and reverse

void fftfilt::init_filter()
{
	flen2 = flen >> 1;
	fft			= new g_fft<double>(flen);

	filter		= new cmplx[flen];
	timedata	= new cmplx[flen];
	freqdata	= new cmplx[flen];
	output		= new cmplx[flen];
	ovlbuf		= new cmplx[flen2];
	ht			= new cmplx[flen];

	memset(filter, 0, flen * sizeof(cmplx));
	memset(timedata, 0, flen * sizeof(cmplx));
	memset(freqdata, 0, flen * sizeof(cmplx));
	memset(output, 0, flen * sizeof(cmplx));
	memset(ovlbuf, 0, flen2 * sizeof(cmplx));
	memset(ht, 0, flen * sizeof(cmplx));

	inptr = 0;
}

//------------------------------------------------------------------------------
// fft filter
// f1 < f2 ==> band pass filter
// f1 > f2 ==> band reject filter
// f1 == 0 ==> low pass filter
// f2 == 0 ==> high pass filter
//------------------------------------------------------------------------------
fftfilt::fftfilt(double f1, double f2, int len)
{
	flen	= len;
	init_filter();
	create_filter(f1, f2);
}

//------------------------------------------------------------------------------
// low pass filter
//------------------------------------------------------------------------------
fftfilt::fftfilt(double f, int len)
{
	flen	= len;
	init_filter();
	create_lpf(f);
}

fftfilt::~fftfilt()
{
	if (fft) delete fft;

	if (filter) delete [] filter;
	if (timedata) delete [] timedata;
	if (freqdata) delete [] freqdata;
	if (output) delete [] output;
	if (ovlbuf) delete [] ovlbuf;
	if (ht) delete [] ht;
}

void fftfilt::create_filter(double f1, double f2)
{
// initialize the filter to zero
	memset(ht, 0, flen * sizeof(cmplx));

// create the filter shape coefficients by fft
// filter values initialized to the ht response h(t)
	bool b_lowpass, b_highpass;//, window;
	b_lowpass = (f2 != 0);
	b_highpass = (f1 != 0);

	for (int i = 0; i < flen2; i++) {
		ht[i] = 0;
//combine lowpass / highpass
// lowpass @ f2
		if (b_lowpass) ht[i] += fsinc(f2, i, flen2);
// highighpass @ f1
		if (b_highpass) ht[i] -= fsinc(f1, i, flen2);
	}
// highpass is delta[flen2/2] - h(t)
	if (b_highpass && f2 < f1) ht[flen2 / 2] += 1;

	for (int i = 0; i < flen2; i++)
		ht[i] *= _blackman(i, flen2);

// this may change since green fft is in place fft
	memcpy(filter, ht, flen * sizeof(cmplx));

// ht is flen complex points with imaginary all zero
// first half describes h(t), second half all zeros
// perform the cmplx forward fft to obtain H(w)
// filter is flen/2 complex values

	fft->ComplexFFT(filter);
//	fft->transform(ht, filter);

// normalize the output filter for unity gain
	double scale = 0, mag;
	for (int i = 0; i < flen2; i++) {
		mag = abs(filter[i]);
		if (mag > scale) scale = mag;
	}
	if (scale != 0) {
		for (int i = 0; i < flen; i++)
			filter[i] /= scale;
	}

// perform the reverse fft to obtain h(t)
// for testing
// uncomment to obtain filter characteristics
/*
	cmplx *revht = new cmplx[flen];
	memcpy(revht, filter, flen * sizeof(cmplx));

	fft->InverseComplexFFT(revht);

	std::fstream fspec;
	fspec.open("fspec.csv", std::ios::out);
	fspec << "i,imp.re,imp.im,filt.re,filt.im,filt.abs,revimp.re,revimp.im\n";
	for (int i = 0; i < flen2; i++)
		fspec
			<< i << "," << ht[i].real() << "," << ht[i].imag() << ","
			<< filter[i].real() << "," << filter[i].imag() << ","
			<< abs(filter[i]) << ","
			<< revht[i].real() << "," << revht[i].imag() << ","
			<< std::endl;
	fspec.close();
	delete [] revht;
*/
	pass = 2;
}

/*
 * Filter with fast convolution (overlap-add algorithm).
 */

int fftfilt::run(const cmplx & in, cmplx **out)
{
// collect flen/2 input samples
	timedata[inptr++] = in;

	if (inptr < flen2)
		return 0;
	if (pass) --pass; // filter output is not stable until 2 passes

// FFT transpose to the frequency domain
	memcpy(freqdata, timedata, flen * sizeof(cmplx));
	fft->ComplexFFT(freqdata);

// multiply with the filter shape
	for (int i = 0; i < flen; i++)
		freqdata[i] *= filter[i];

// transform back to time domain
	fft->InverseComplexFFT(freqdata);

// overlap and add
// save the second half for overlapping next inverse FFT
	for (int i = 0; i < flen2; i++) {
		output[i] = ovlbuf[i] + freqdata[i];
		ovlbuf[i] = freqdata[i+flen2];
	}

// clear inbuf pointer
	inptr = 0;

// signal the caller there is flen/2 samples ready
	if (pass) return 0;

	*out = output;
	return flen2;
}

//------------------------------------------------------------------------------
// rtty filter
//------------------------------------------------------------------------------

//bool print_filter = true; // flag to inhibit printing multiple copies

void fftfilt::rtty_filter(double f)
{
// Raised cosine filter designed iaw Section 1.2.6 of
// Telecommunications Measurements, Analysis, and Instrumentation
// by Dr. Kamilo Feher / Engineers of Hewlett-Packard
//
// Frequency scaling factor determined hueristically by testing various values 
// and measuring resulting decoder CER with input s/n = - 9 dB
//
//    K     CER
//   1.0   .0244
//   1.1   .0117
//   1.2   .0081
//   1.3   .0062
//   1.4   .0054
//   1.5   .0062
//   1.6   .0076

	f *= 1.4;

	double dht;
	for( int i = 0; i < flen2; ++i ) {
		double x = (double)i/(double)(flen2);	

// raised cosine response (changed for -1.0...+1.0 times Nyquist-f
// instead of books versions ranging from -1..+1 times samplerate)

		dht =
			x <= 0 ? 1.0 :
			x > 2.0 * f ? 0.0 :
			cos((M_PI * x) / (f * 4.0));

		dht *= dht; // cos^2

// amplitude equalized nyquist-channel response
		dht /= sinc(2.0 * i * f);

		filter[i].real() = dht*cos((double)i* - 0.5*M_PI);
		filter[i].imag() = dht*sin((double)i* - 0.5*M_PI);

		filter[(flen-i)%flen].real() = dht*cos((double)i*+0.5*M_PI);
		filter[(flen-i)%flen].imag() = dht*sin((double)i*+0.5*M_PI);
	}

// perform the reverse fft to obtain h(t)
// for testing
// uncomment to obtain filter characteristics
/*
	cmplx *revht = new cmplx[flen];
	memcpy(revht, filter, flen * sizeof(cmplx));

	fft->InverseComplexFFT(revht);

	std::fstream fspec;
	fspec.open("rtty_filter.csv", std::ios::out);
	fspec << "i,filt.re,filt.im,filt.abs,,revimp.re,revimp.im\n";
	for (int i = 0; i < flen; i++)
		fspec
			<< i << ","
			<< filter[i].real() << "," << filter[i].imag() << "," << abs(filter[i]) 
			<< ",," << revht[i].real() << "," << revht[i].imag()
			<< std::endl;
	fspec.close();
	delete [] revht;
*/
// start outputs after 2 full passes are complete
	pass = 2;
}

