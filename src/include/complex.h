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

// Z = X * Y
	complex operator* (complex y) {
		complex z;
		z.re = re * y.re - im * y.im;
		z.im = re * y.im + im * y.re;
		return z;
	}

// Z = X * y
	complex operator* (double y) {
		complex z;
		z.re = y * z.re;
		z.im = y * z.im;
		return z;
	}

// Z = X + Y
	complex operator+ (complex y) {
		complex z;
		z.re = re + y.re;
		z.im = im + y.im;
		return z;
	}

// Z = X - Y
	complex operator- (complex y) {
		complex z;
		z.re = re - y.re;
		z.im = im - y.im;
		return z;
	}

// Z = X / Y
	complex operator/ (complex y) {
		double denom = y.re*y.re + y.im*y.im;
		if (denom == 0.0) denom = 1e-10;
		complex z;
		z.re = (re * y.re + im * y.im) / denom;
		z.im = (im * y.re - re * y.im) / denom;
		return z;
	}
	
// Z = (complex conjugate of X) * Y
// Z1 = x1 + jy1, or Z1 = |Z1|exp(jP1)
// Z2 = x2 + jy2, or Z2 = |Z2|exp(jP2)
// Z = (x1 - jy1) * (x2 + jy2)
// or Z = |Z1|*|Z2| exp (j (P2 - P1))
	complex operator% (complex y) {
		complex z;
		z.re = re * y.re + im * y.im;
		z.im = re * y.im - im * y.re;
		return z;
	}

// n = |Z| * |Z| 	
	double norm() {
		return (re * re + im * im);
	}

// n = |Z|
	double mag() {
		return sqrt(norm());
	}

// Z = x + jy
// Z = |Z|exp(jP)
// arg returns P
	double arg() {
		return atan2(im, re);
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
