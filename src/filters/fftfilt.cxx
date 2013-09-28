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

#include <fstream>

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

	ovlbuf		= new cmplx[filterlen/2];
	filter		= new cmplx[filterlen];
	filtdata	= new cmplx[filterlen];
	ht			= new cmplx[filterlen];

	for (int i = 0; i < filterlen; i++)
		filter[i].real() = filter[i].imag() =
		filtdata[i].real() = filtdata[i].imag() = 0.0;
	for (int i = 0; i < filterlen/2; i++)
		ovlbuf[i].real() = ovlbuf[i].imag() = 0.0;

	inptr = 0;

	create_filter(f1, f2);
}

fftfilt::fftfilt(double f, int len)
{
	filterlen = len;
	fft = new Cfft(filterlen);
	ift = new Cfft(filterlen);

	ovlbuf		= new cmplx[filterlen/2];
	filter		= new cmplx[filterlen];
	filtdata	= new cmplx[filterlen];
	ht			= new cmplx[filterlen];

	for (int i = 0; i < filterlen; i++)
		filter[i].real() = filter[i].imag() =
		filtdata[i].real() = filtdata[i].imag() = 0.0;
	for (int i = 0; i < filterlen/2; i++)
		ovlbuf[i].real() = ovlbuf[i].imag() = 0.0;

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
	if (ht) delete [] ht;
}

/*
 * Filter with fast convolution (overlap-add algorithm).
 */
int fftfilt::run(const cmplx& in, cmplx **out)
{
// collect filterlen/2 input samples
	const int filterlen_div2 = filterlen / 2 ;
	filtdata[inptr++] = in;

	if (inptr < filterlen_div2)
		return 0;
	if (pass) --pass; // filter output is not stable until 2 passes

// zero the rest of the input data
	for (int i = filterlen_div2 ; i < filterlen; i++)
		filtdata[i].real() = filtdata[i].imag() = 0.0;

// FFT transpose to the frequency domain
	fft->cdft(filtdata);

// multiply with the filter shape
	for (int i = 0; i < filterlen; i++)
		filtdata[i] *= filter[i];

// IFFT transpose back to the time domain
	ift->icdft(filtdata);

// overlap and add
	for (int i = 0; i < filterlen_div2; i++) {
		filtdata[i] += ovlbuf[i];
	}
	*out = filtdata;

// save the second half for overlapping
	// Memcpy is allowed because cmplx are POD objects.
	memcpy( ovlbuf, filtdata + filterlen_div2, sizeof( ovlbuf[0] ) * filterlen_div2 );

// clear inbuf pointer
	inptr = 0;

// signal the caller there is filterlen/2 samples ready
	if (pass) return 0;

	return filterlen_div2;
}

void fftfilt::create_filter(double f1, double f2)
{
	int len = filterlen / 2 + 1;
	double t, h, x, it;
	Cfft *tmpfft;
	tmpfft = new Cfft(filterlen);

// initialize the filter to zero
	for (int i = 0; i < filterlen; i++)
		filter[i].real()   = filter[i].imag()   = 0.0;

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
		filter[i].real() = x;
	}
// perform the cmplx forward fft to obtain H(w)
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
		filter[i].real()   = filter[i].imag()   = 0.0;

// create the filter shape coefficients by fft
// filter values initialized to the impulse response h(t)
	for (int i = 0; i < len; i++) {
		it = (double) i;
		t = it - (len - 1) / 2.0;
		h = it / (len - 1);

		x = f * sinc(2 * f * t);
		x *= blackman(h);	// windowed by Blackman function
		x *= filterlen;		// scaled for unity in passband
		filter[i].real() = x;
	}
// perform the cmplx forward fft to obtain H(w)
	tmpfft->cdft(filter);
// start outputs after 2 full passes are complete
	pass = 2;
	delete tmpfft;
}

//bool print_filter = true; // flag to inhibit printing multiple copies

void fftfilt::create_rttyfilt(double f)
{
	int len = filterlen / 2 + 1;
	double t, h, it;
	Cfft *tmpfft;
	tmpfft = new Cfft(filterlen);

	// initialize the filter to zero
	for (int i = 0; i < filterlen; i++)
		filter[i].real()   = filter[i].imag()   = 0.0;

	// get an array to hold the sinc-respose
	double* sinc_array = new double[ len ];

	// create the impulse-response in it
	for (int i = 0; i < len; ++i) {
		it = (double)i;
		t  = it - ( (double)len - 1.0) / 2.0;
		h  = it / ( (double)len - 1.0);

		// create the filter impulses with an additional zero at 1.5f
		// remark: sinc(..) is scaled by 2, see misc.h

// Modified Lanzcos filter see http://en.wikipedia.org/wiki/Lanczos_resampling
		sinc_array[i] = 
			( sinc( 3.0 * f * t			 ) +
			  sinc( 3.0 * f * t - 1.0	   ) * 0.8 +
			  sinc( 3.0 * f * t + 1.0	   ) * 0.8 ) *
			( sinc( 4.0 * f * t / 3.0	   ) +
			  sinc( 4.0 * f * t / 3.0 - 1.0 ) * 0.8 +
			  sinc( 4.0 * f * t / 3.0 + 1.0 ) * 0.8 );
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

		filter[i].real() = ( sinc_array[i] ) * (double)filterlen * blackman(h);
		sinc_array[i] = filter[i].real();
		}

/*
// create an identical filter impulse response for testing
// ht_B should be identical to ht_A within limits of math processing
// Hw is the frequency response of filter created using ht_A impulse
// response
	Cfft test_fft(filterlen);
	cmplx ht_A[filterlen]; // original impulse response
	cmplx ht_B[filterlen]; // computed impulse response
	cmplx Hw[filterlen];   // computed H(w)

// ht_A retains the original normalized impulse response
// ht_B used for forward / reverse FFT
	for (int i = 0; i < len; ++i)
		ht_B[i] = ht_A[i] = filter[i];

// perform the cmplx forward fft to obtain H(w)
	test_fft.cdft(ht_B);
	for (int i = 0; i < len; ++i)
		Hw[i] = ht_B[i];

// perform the cmplx reverse fft to obtain h(t) again
	test_fft.icdft(ht_B);

// ht_B should be equal to ht_A
	std::fstream file1("filter_debug.csv", std::ios::out );
	for (int i = 0; i < len; ++i)
		file1 << ht_A[i].real() << "," << ht_A[i].imag() << "," << 
				ht_B[i].real() << "," << ht_B[i].imag() << "," <<
				Hw[i].real() << "," << Hw[i].imag() << "," << Hw[i].mag() << "\n";
	file1.close();
*/

// perform the cmplx forward fft to obtain H(w)
	tmpfft->cdft(filter);
/*
	if (print_filter) {
		std::fstream file2("filter_response.csv", std::ios::out );
		file2 << "Modified Lanzcos 1.5 stop bit filter\n\n";
		file2 << "h(t), |H(w)|, dB\n\n";
		double dc = 20*log10(filter[0].mag());
		for (int i = 0; i < len; i++)
			file2 << sinc_array[i] << "," << filter[i].mag() << ","
					20*log10(filter[i].mag()) - dc << "\n";
		file2.close();
		print_filter = false;
	}
*/

// start outputs after 2 full passes are complete
	pass = 2;
	delete tmpfft;
	delete [] sinc_array;

}

double xrcos(double t, double T, int order, double alpha = 1)
{
	if (order == 1) return rcos(t, T, alpha);
	order--;
	return xrcos(2*t - T/2, T, order, alpha) + xrcos(2*t + T/2, T, order, alpha);
}

double stefan(double t, double T)
{
// Stefan implementation
	double a=.7;
	double h = rcos( t		, T/4.0, a );
	h += rcos( t - T/4.0, T/4.0, a );
	h += rcos( t + T/4.0, T/4.0, a );
	return h;
}

double matched(double t, double T)
{
	if (t > -T/2 && t < T/2) return 1;
	return 0;
}

double sinc_filter(double t, double T)
{
	return sinc(t / T);
}

void fftfilt::rtty_order(double f, int N, double twarp, double alpha)
{
	int len = filterlen / 2 + 1;
	double ft;
	Cfft tmpfft(filterlen);

	// create the impulse-response
	for (int i = 0; i < filterlen; ++i) {
		if (i > len) {
			ht[i].real() = ht[i].imag() = 0.0;
			continue;
		}
		ft = f * (1.0* i - len / 2.0);
		switch(N) {
		default:
		case 0:
			ft *= twarp; // compromise filter CPFSK vs SHAPED_AFSK
			ht[i] = xrcos( ft, 1.0, 1, alpha);
			break;
		case 1:
			ft *= 1.1;
			ht[i] = xrcos( ft, 1.0, 2, alpha );
			break;
		case 2:
//			ft *= 1.0;
			ht[i] = xrcos( ft, 1.0, 3, alpha );
			break;
		case 3:
			ft *= 1.5;
			ht[i] = rcos( ft, 1.0 );
			break;
		case 4:
			ft *= (1.0 + M_PI/2.0);
			ht[i]  = rcos( ft - 0.5, 1.0 );
			ht[i] += rcos( ft + 0.5, 1.0 );
			break;
		case 5:
			ft *= (3.0 + M_PI/2.0);
			ht[i]  = rcos( ft - 1.5, 1.0 );
			ht[i] += rcos( ft - 0.5, 1.0 );
			ht[i] += rcos( ft + 0.5, 1.0 );
			ht[i] += rcos( ft + 1.5, 1.0 );
			break;
		case 6:
			ft *= M_PI / 2.0;
			ht[i] = sinc_filter(ft, 1.0 );
			break;
		case 7:
			ft *= (2.0 + M_PI/2.0);
			ht[i]  = rcos( ft - 1.0, 1.0 );
			ht[i] += rcos( ft, 1.0 );
			ht[i] += rcos( ft + 1.0, 1.0 );
			break;
		case 8:
//			ft *= 1.0+0.57079/10.0E10; // simulating inf
			ht[i] = matched(ft, 1.0);
			break;
		}
	}

// normalize the impulse-response
	double sum = 0.0;
	for (int i = 0; i <= len; ++i) {
		sum += ht[i].real();
	}
	for (int i = 0; i < filterlen; ++i) {
		ht[i].real() *= filterlen/sum;
		filter[i] = ht[i];
	}

/*
// create an identical filter impulse response for testing
// ht_B should be identical to ht_A within limits of math processing
// Hw is the frequency response of filter created using ht_A impulse
// response
	Cfft test_fft(filterlen);
	cmplx ht_A[filterlen]; // original impulse response
	cmplx ht_B[filterlen]; // computed impulse response
	cmplx Hw[filterlen];   // computed H(w)

// ht_A retains the original normalized impulse response
// ht_B used for forward / reverse FFT
	for (int i = 0; i < filterlen; i++)
		ht_B[i] = ht_A[i] = filter[i];

// perform the cmplx forward fft to obtain H(w)
	test_fft.cdft(ht_B);
	for (int i = 0; i < filterlen; i++)
		Hw[i] = ht_B[i];

// perform the cmplx reverse fft to obtain h(t) again
	test_fft.icdft(ht_B);

// ht_B should be equal to ht_A
	std::fstream file1("filter_debug.csv", std::ios::out );
	for (int i = 0; i < filterlen; i++)//len; ++i)
		file1 << ht_A[i].real() << "," << ht_B[i].real() << "," 
			  << ht_A[i].real() - ht_B[i].real() << ","
			  << Hw[i].mag() << "\n";
	file1.close();
*/

// perform the cmplx forward fft to obtain H(w)
//	tmpfft->cdft(filter);
	tmpfft.cdft(filter);

// start outputs after 2 full passes are complete
	pass = 2;
//	delete tmpfft;

// Stefan's latest

	f*=1.275; // This factor is ominous to me. I can't explain it. It shouldn't
			  // be there. But if I leave it out ht(f) differs inbetween the 
			  // raised cosine from above and this one. And if left out the error
			  // rate increases... So, this is an unsolved mystery for now.

	for( int i = 0; i < filterlen/2; ++i ) {
		double a = 1.0; 
		double x = (double)i/(double)(filterlen/2);		

	// raised cosine response (changed for -1.0...+1.0 times Nyquist-f
	// instead of books versions ranging from -1..+1 times samplerate)

		double ht =
			fabs(x) <= (1.0 - a)/(1.0/f) ? 1.0:
			fabs(x) >  (1.0 + a)/(1.0/f) ? 0.0:
			cos(M_PI/(f*4.0*a)*(fabs(x)-(1.0-a)/(1.0/f)));
		ht *= ht; // cos^2

		// equalized nyquist-channel response
		double eq = 1.0/sinc((double)i*f*2);  

		// compensate for "awkward" FFT-implementation. For every other imple-
		// mentation of a FFT this would have been just...

		filter[i].real() = eq*ht*sin((double)i* - 0.5*M_PI);
		filter[i].imag() = eq*ht*cos((double)i* - 0.5*M_PI);

		filter[(filterlen-i)%filterlen].real() = eq*ht*sin((double)i*+0.5*M_PI);
		filter[(filterlen-i)%filterlen].imag() = eq*ht*cos((double)i*+0.5*M_PI);

		// ... this (caused most headache):
		//filter[i].real() = eq*ht*0.7071;
		//filter[i].imag() = eq*ht*0.7071;
		//filter[(filterlen-i)%filterlen].real() = eq*ht*0.7071;
		//filter[(filterlen-i)%filterlen].imag() = eq*ht*0.7071;

	}
	std::fstream file1("filter_debug.csv", std::ios::out );
	for (int i = 0; i < filterlen/2; i++)
		file1 << filter[i].real() << "," << filter[i].imag() << ","
			  << abs(filter[i]) << "\n";
	file1.close();

}
