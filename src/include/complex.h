//
// complex.h  --  Complex arithmetic
//

#ifndef _COMPLEX_H
#define _COMPLEX_H

#include <math.h>

class complex {
public:
	double re;
	double im;
	complex(double r = 0.0, double i = 0.0) {
		re = r;
		im = i;
	}
	~complex() {};
	double real() { return re; };
	void real(double R) {re = R;};
	double imag() { return im; };
	void imag(double I) {im = I;};
	complex conj () {
		complex z;
		z.re = re; z.im = -im;
		return z;
	}
	complex operator* (complex y) {
		complex z;
		z.re = re * y.re - im * y.im;
		z.im = re * y.im + im * y.re;
		return z;
	}
	complex operator* (double y) {
		complex z;
		z.re = y * z.re;
		z.im = y * z.im;
		return z;
	}
	complex operator+ (complex y) {
		complex z;
		z.re = re + y.re;
		z.im = im + y.im;
		return z;
	}
	complex operator- (complex y) {
		complex z;
		z.re = re - y.re;
		z.im = im - y.im;
		return z;
	}
	complex operator/ (complex y) {
		double denom = y.re*y.re + y.im*y.im;
		if (denom == 0.0) denom = 1e-10;
		complex z;
		z.re = (re * y.re + im * y.im) / denom;
		z.im = (im * y.re - re * y.im) / denom;
		return z;
	}
	
	complex ccor(complex x, complex y) { 
		complex z;
		z.re = x.re * y.re + x.im * y.im;
		z.im = x.re * y.im - x.im * y.re;
		return z;
	}
	
// (complex conjugate of X) * Y
	complex operator% (complex y) { // operator type for ccor in gmfsk code
		complex z;
		z.re = re * y.re + im * y.im;
		z.im = re * y.im - im * y.re;
		return z;
	}
	
	double ccorI(complex y) {
		return (re * y.re + im * y.im);
	}
	double ccorQ(complex y) {
		return (re * y.im - im * y.re);
	}
	 double norm() {
		return (re * re + im * im);
	}
	double mag() {
		return sqrt(norm());
	}
	double arg() {
		return atan2(im, re);
	}
	complex csqrt() {
		complex z;
		z.re = sqrt(mag() + re) / M_SQRT2;
		z.im = im / re / 2;
		return z;
	}
};

inline 	complex cmac (complex *a, complex *b, int ptr, int len) {
		complex z;
		ptr = ptr % len;
		for (int i = 0; i < len; i++) {
			z = z + a[i] * b[ptr];
			ptr = (ptr + 1) % len;
		}
		return z;
	}
	

#endif
