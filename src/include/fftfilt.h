/*
 *    fftfilt.h  --  Fast convolution FIR filter
*/

#ifndef	_FFTFILT_H
#define	_FFTFILT_H

#include "complex.h"
#include "fft.h"

//----------------------------------------------------------------------

class fftfilt {
enum {NONE, BLACKMAN, HAMMING, HANNING};

protected:
	int filterlen;
    Cfft *fft;
    Cfft *ift;
	complex *filter;
	complex *filtdata;
	complex *ovlbuf;
	int inptr;
	int pass;
	int window;
public:
	fftfilt(double f1, double f2, int len);
	fftfilt(double f, int len);
	~fftfilt();
	void create_filter(double f1, double f2);
	void create_lpf(double f);
	void create_rttyfilt(double f);
	void set_window(int w) { window = w; }
	int run(const complex& in, complex **out);
};

#endif
