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
	cmplx *filter;
	cmplx *filtdata;
	cmplx *ovlbuf;
	cmplx *ht;
	int inptr;
	int pass;
public:
	fftfilt(double f1, double f2, int len);
	fftfilt(double f, int len);
	~fftfilt();
	void create_filter(double f1, double f2);
	void create_lpf(double f);
	void create_rttyfilt(double f);
	void rtty_order(double, int, double twarp = 1.275, double alpha = 1.0);

	int run(const cmplx& in, cmplx **out);
};

#endif
