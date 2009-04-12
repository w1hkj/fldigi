//=============================================================================
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

#include <config.h>

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
		filter[i].re = filter[i].im = 0.0;

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


/*
 * Filter with fast convolution (overlap-add algorithm).
 */
int fftfilt::run(const complex& in, complex **out)
{
// collect filterlen/2 input samples
	filtdata[inptr++] = in;

	if (inptr < filterlen / 2)
		return 0;
	if (pass) --pass; // filter output is not stable until 2 passes

// zero the rest of the input data
	for (int i = filterlen / 2 ; i < filterlen; i++)
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
	for (int i = 0; i < filterlen / 2; i++) {
		filtdata[i].re += ovlbuf[i].re;
		filtdata[i].im += ovlbuf[i].im;
	}
	*out = filtdata;

// save the second half for overlapping
	for (int i = 0; i < filterlen / 2; i++)
		ovlbuf[i] = filtdata[i + filterlen / 2];

// clear inbuf pointer
	inptr = 0;

// signal the caller there is filterlen/2 samples ready
	if (pass) return 0;
	
	return filterlen / 2;
}
