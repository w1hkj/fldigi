#ifndef	_COEFF_H
#define	_COEFF_H

#define	FIRLEN	64

extern double gmfir1c[FIRLEN];
extern double gmfir2c[FIRLEN];
extern double syncfilt[16];

extern void raisedcosfilt(double *);
extern void wsincfilt(double *, double fc, bool blackman);

#endif
