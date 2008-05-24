#ifndef RSID_FFT_H
#define RSID_FFT_H

extern void rsrfft( double *x, int m );

extern void rstage(
		int n, int n2, int n4, 
		double *x1, double *x2, double *x3, double *x4);

extern void rbitrev(double *x, int m);

#endif
