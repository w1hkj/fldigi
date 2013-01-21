/*
 *    fftfilt.h  --  Fast convolution FIR filter
*/

#ifndef	_FFTFILT_H
#define	_FFTFILT_H

#include "complex.h"
#include "fft.h"

//----------------------------------------------------------------------

class fftfilt {
protected:
	int filterlen;
    Cfft *fft;
    Cfft *ift;
	complex *filter;
	complex *filtdata;
	complex *ovlbuf;
	int inptr;
	int pass;
public:
	fftfilt(double f1, double f2, int len);
	fftfilt(double f, int len);
	~fftfilt();
	void create_filter(double f1, double f2);
	void create_lpf(double f);
	void create_rttyfilt(double f);
	int run(const complex& in, complex **out);
};

#endif
