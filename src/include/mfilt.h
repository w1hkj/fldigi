/*
 *    mfilt.h  --  Matched Filter using convolution
*/

#ifndef	_MFILT_H
#define	_MFILT_H

#include "complex.h"
#include "fft.h"

//----------------------------------------------------------------------

class mfilt {
protected:
	int filterlen;
	int inbuflen;
	double *filter;
	double *inbuf;
	double *outbuf;
	int inptr;

public:
	mfilt(double freq, int Fs, int speed, int buflen);
	~mfilt();
	void create_filter(double freq, int Fs, int speed, int buflen);
	bool convolve(double *X,double *Y, double *Z, int lenx, int leny);
	int run(const double *in, double **out, int *len);
};

#endif
