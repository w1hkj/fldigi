#ifndef	_COEFF_H
#define	_COEFF_H

#define	FIRLEN	64

extern double fir1c[FIRLEN];
extern double fir2c[FIRLEN];
extern double syncfilt[16];

extern void raisedcosfilt(double *);
extern void wsincfilt(double *, double fc);

#endif
