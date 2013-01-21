// ----------------------------------------------------------------------------
//    fftfilt.cxx  --  Fast convolution Overlap-Add filter
//
// Filter implemented using overlap-add FFT convolution method
// h(t) characterized by Windowed-Sinc impulse response
// 
// Reference: 
//     "The Scientist and Engineer's Guide to Digital Signal Processing"
//     by Dr. Steven W. Smith, http://www.dspguide.com
//     Chapters 16, 18 and 21
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

#include <stdio.h>

#include <cmath>
#include "misc.h"

#include "fftfilt.h"


fftfilt::fftfilt(double f1, double f2, int len)
{
	filterlen = len;
	fft = new Cfft(filterlen);
	ift = new Cfft(filterlen);

	ovlbuf		= new complex[filterlen/2];
	filter		= new complex[filterlen];
	filtdata	= new complex[filterlen];
	
	for (int i = 0; i < filterlen; i++)
		filter[i].re = filter[i].im =
		filtdata[i].re = filtdata[i].im = 0.0;
	for (int i = 0; i < filterlen/2; i++)
		ovlbuf[i].re = ovlbuf[i].im = 0.0;

	inptr = 0;

	create_filter(f1, f2);
}

fftfilt::fftfilt(double f, int len)
{
	filterlen = len;
	fft = new Cfft(filterlen);
	ift = new Cfft(filterlen);

	ovlbuf		= new complex[filterlen/2];
	filter		= new complex[filterlen];
	filtdata	= new complex[filterlen];
	
	for (int i = 0; i < filterlen; i++)
		filter[i].re = filter[i].im =
		filtdata[i].re = filtdata[i].im = 0.0;
	for (int i = 0; i < filterlen/2; i++)
		ovlbuf[i].re = ovlbuf[i].im = 0.0;

	inptr = 0;

	create_lpf(f);
}

fftfilt::~fftfilt()
{
	if (fft) delete fft;
	if (ift) delete ift;
	if (ovlbuf) delete [] ovlbuf;
	if (filter) delete [] filter;
	if (filtdata) delete [] filtdata;
}


void fftfilt::create_filter(double f1, double f2)
{
	int len = filterlen / 2 + 1;
	double t, h, x, it;
	Cfft *tmpfft;
	tmpfft = new Cfft(filterlen);
	
// initialize the filter to zero	
	for (int i = 0; i < filterlen; i++)
		filter[i].re   = filter[i].im   = 0.0;

// create the filter shape coefficients by fft
// filter values initialized to the impulse response h(t)
	for (int i = 0; i < len; i++) {
		it = (double) i;
		t = it - (len - 1) / 2.0;
		h = it / (len - 1);

		x = f2 * sinc(2 * f2 * t) - f1 * sinc(2 * f1 * t); // sinc(x)
//		x *= hamming(t);
//		x *= hanning(h);
		x *= blackman(h);	// windowed by Blackman function
		x *= filterlen;		// scaled for unity in passband
		filter[i].re = x;
	}
// perform the complex forward fft to obtain H(w)
	tmpfft->cdft(filter);
// start outputs after 2 full passes are complete
	pass = 2;
	delete tmpfft;
}


void fftfilt::create_lpf(double f)
{
	int len = filterlen / 2 + 1;
	double t, h, x, it;
	Cfft *tmpfft;
	tmpfft = new Cfft(filterlen);
	
// initialize the filter to zero	
	for (int i = 0; i < filterlen; i++)
		filter[i].re   = filter[i].im   = 0.0;

// create the filter shape coefficients by fft
// filter values initialized to the impulse response h(t)
	for (int i = 0; i < len; i++) {
		it = (double) i;
		t = it - (len - 1) / 2.0;
		h = it / (len - 1);

		x = f * sinc(2 * f * t);
		x *= blackman(h);	// windowed by Blackman function
		x *= filterlen;		// scaled for unity in passband
		filter[i].re = x;
	}
// perform the complex forward fft to obtain H(w)
	tmpfft->cdft(filter);
// start outputs after 2 full passes are complete
	pass = 2;
	delete tmpfft;
}

void fftfilt::create_rttyfilt(double f)
{
    int len = filterlen / 2 + 1;
	double t, h, x, it;
	Cfft *tmpfft;
	tmpfft = new Cfft(filterlen);
	
    // initialize the filter to zero	
	for (int i = 0; i < filterlen; i++)
		filter[i].re   = filter[i].im   = 0.0;

    // get an array to hold the sinc-respose
    double* sinc_array = new double[ len ];
    
    // create the impulse-response in it
    for (int i = 0; i < len; ++i) {
        it = (double)i;
        t  = it - ( (double)len - 1.0) / 2.0;
        h  = it / ( (double)len - 1.0);
        
        // create the filter impulses with an additional zero at 1.5f
        // remark: sinc(..) is scaled by 2, see misc.h
        
        sinc_array[i] = sinc( 4.0 * f * t       )+ 
                        sinc( 4.0 * f * t - 1.0 )+ 
                        sinc( 4.0 * f * t + 1.0 );
    }
    
    // normalize the impulse-responses
    double sum = 0.0;
    for (int i = 0; i < len; ++i) {
        sum += sinc_array[i];
        }
    for (int i = 0; i < len; ++i) {
        sinc_array[i] /= 8*sum;
        }
                
    // setup windowed-filter
    for (int i = 0; i < len; ++i) {
        it = (double)i;
        t  = it - ( (double)len - 1.0) / 2.0;
        h  = it / ( (double)len - 1.0);
        
        filter[i].re = ( sinc_array[i] ) * (double)filterlen * blackman(h);
        }
        
    delete [] sinc_array;

//printf("Raised Cosine filter\n");
//printf("h(t)\n");
//for (int i = 0; i < len; i++) printf("%f, %f\n", filter[i].re, filter[i].im);
//printf("\n");
// perform the complex forward fft to obtain H(w)
	tmpfft->cdft(filter);
//printf("H(w)\n");
//for (int i = 0; i < len; i++) printf("%f, %f\n", filter[i].re, filter[i].im);

// start outputs after 2 full passes are complete
	pass = 2;
	delete tmpfft;
}

/*
 * Filter with fast convolution (overlap-add algorithm).
 */
int fftfilt::run(const complex& in, complex **out)
{
// collect filterlen/2 input samples
	const int filterlen_div2 = filterlen / 2 ;
	filtdata[inptr++] = in;

	if (inptr < filterlen_div2)
		return 0;
	if (pass) --pass; // filter output is not stable until 2 passes

// zero the rest of the input data
	for (int i = filterlen_div2 ; i < filterlen; i++)
		filtdata[i].re = filtdata[i].im = 0.0;
	
// FFT transpose to the frequency domain
	fft->cdft(filtdata);

// multiply with the filter shape
	for (int i = 0; i < filterlen; i++)
//		filtdata[i] = filtdata[i] * filter[i];
		filtdata[i] *= filter[i];

// IFFT transpose back to the time domain
	ift->icdft(filtdata);

// overlap and add
	for (int i = 0; i < filterlen_div2; i++) {
		filtdata[i] += ovlbuf[i];
	}
	*out = filtdata;

// save the second half for overlapping
	// Memcpy is allowed because complex are POD objects.
	memcpy( ovlbuf, filtdata + filterlen_div2, sizeof( ovlbuf[0] ) * filterlen_div2 );


// clear inbuf pointer
	inptr = 0;

// signal the caller there is filterlen/2 samples ready
	if (pass) return 0;
	
	return filterlen_div2;
}
