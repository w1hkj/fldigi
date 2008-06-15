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
	complex(double r = 0.0, double i = 0.0)
	    : re(r), im(i) { }

	double real() { return re; };
	void real(double R) {re = R;};
	double imag() { return im; };
	void imag(double I) {im = I;};

// Z = X * Y
	complex& operator*=(const complex& y) {
		double temp = re * y.re - im * y.im;
		im = re * y.im + im * y.re;
		re = temp;
		return *this;
	}
	complex operator*(const complex& y) const {
		return complex(re * y.re - im * y.im,  re * y.im + im * y.re);
	}

// Z = X * y
	complex& operator*=(double y) {
		re *= y;
		im *= y;
		return *this;
	}
	complex operator*(double y) const {
		return complex(re * y, im * y);
	}

// Z = X + Y
	complex& operator+=(const complex& y) {
		re += y.re;
		im += y.im;
		return *this;
        }
	complex operator+(const complex& y) const {
		return complex(re + y.re,  im + y.im);
	}

// Z = X - Y
	complex& operator-=(const complex& y) {
		re -= y.re;
		im -= y.im;
		return *this;
	}
	complex operator-(const complex& y) const {
		return complex(re - y.re,  im - y.im);
	}

// Z = X / Y
	complex& operator/=(const complex& y) {
		double temp, denom = y.re*y.re + y.im*y.im;
		if (denom == 0.0) denom = 1e-10;
		temp = (re * y.re + im * y.im) / denom;
		im = (im * y.re - re * y.im) / denom;
		re = temp;
		return *this;
	}
	complex operator/(const complex& y) const {
		double denom = y.re*y.re + y.im*y.im;
		if (denom == 0.0) denom = 1e-10;
		return complex((re * y.re + im * y.im) / denom,  (im * y.re - re * y.im) / denom);
	}
	
// Z = (complex conjugate of X) * Y
// Z1 = x1 + jy1, or Z1 = |Z1|exp(jP1)
// Z2 = x2 + jy2, or Z2 = |Z2|exp(jP2)
// Z = (x1 - jy1) * (x2 + jy2)
// or Z = |Z1|*|Z2| exp (j (P2 - P1))
	complex& operator%=(const complex& y) {
		double temp = re * y.re + im * y.im;
		im = re * y.im - im * y.re;
		re = temp;
		return *this;
	}
	complex operator%(const complex& y) const {
		complex z;
		z.re = re * y.re + im * y.im;
		z.im = re * y.im - im * y.re;
		return z;
	}

// n = |Z| * |Z| 	
	double norm() const {
		return (re * re + im * im);
	}

// n = |Z|
	double mag() const {
		return sqrt(norm());
	}

// Z = x + jy
// Z = |Z|exp(jP)
// arg returns P
	double arg() const {
		return atan2(im, re);
	}

};

inline 	complex cmac (const complex *a, const complex *b, int ptr, int len) {
		complex z;
		ptr %= len;
		for (int i = 0; i < len; i++) {
			z += a[i] * b[ptr];
			ptr = (ptr + 1) % len;
		}
		return z;
	}
	

#endif
