/*
 *    fftfilt.h  --  Fast convolution FIR filter
*/

#ifndef	_FFTFILT_H
#define	_FFTFILT_H

#include "complex.h"
#include "gfft.h"

//----------------------------------------------------------------------

class fftfilt {
enum {NONE, BLACKMAN, HAMMING, HANNING};

protected:
	int flen;
	int flen2;
	g_fft<double> *fft;
	g_fft<double> *ift;
	cmplx *ht;
	cmplx *filter;
	cmplx *timedata;
	cmplx *freqdata;
	cmplx *ovlbuf;
	cmplx *output;
	int inptr;
	int pass;
	int window;

	inline double fsinc(double fc, int i, int len) {
		return (i == len/2) ? 2.0 * fc: 
				sin(2 * M_PI * fc * (i - len/2)) / (M_PI * (i - len/2));
	}
	inline double _blackman(int i, int len) {
		return (0.42 - 
				 0.50 * cos(2.0 * M_PI * i / len) + 
				 0.08 * cos(4.0 * M_PI * i / len));
	}
	void init_filter();

public:
	fftfilt(double f1, double f2, int len);
	fftfilt(double f, int len);
	~fftfilt();
// f1 < f2 ==> bandpass
// f1 > f2 ==> band reject
	void create_filter(double f1, double f2);
	void create_lpf(double f) {
		create_filter(0, f);
	}
	void create_hpf(double f) {
		create_filter(f, 0);
	}
	void rtty_filter(double);

	int run(const cmplx& in, cmplx **out);
};

#endif
