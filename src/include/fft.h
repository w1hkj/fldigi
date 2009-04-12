//===========================================================================
// Real Discrete Fourier Transform
//     dimension   :one
//     data length :power of 2, must be larger than 4
//     decimation  :frequency
//     radix       :4, 2
//     data        :inplace
// classes:
//	 Cfft: real discrete fourier transform class
// functions:
//	 Cfft::rdft : compute the forward real discrete fourier transform
//   Cfft::cdft : compute the forward complex discrete fourier transform
//   Cfft::fft  : compute the forward real dft on a set of integer values
//	
//	 This class is derived from the work of Takuya Ooura, who has kindly put his
//	 fft algorithims in the public domain.  Thank you Takuya Ooura!
//===========================================================================

#ifndef FFT_H
#define FFT_H

#include "complex.h"

enum fftPrefilter {FFT_NONE, FFT_HAMMING, FFT_HANNING, FFT_BLACKMAN, FFT_TRIANGULAR};

class Cfft {
private:
	double xi;
	double *w;
	int  *ip;
	double *fftwin;
	fftPrefilter wintype;
	int  fftlen;
	int  fftsiz;
    void makewt();
    void makect();
    void bitrv2(int n, int *ip, double *a);
	void bitrv2conj(int n, int *ip, double *a);
    void cftfsub(int n, double *a);
	void cftbsub(int n, double *a);
	void cftmdl(int n, int l, double *a);
	void cft1st(int n, double *a);
	void rftfsub(int n, double *a);
	void rftbsub(int n, double *a);
	
public:
	Cfft(int n);
	~Cfft();
	void resize(int n);
	void cdft(double *a);
	void cdft(complex *a) { cdft( (double *) a); }
	void icdft(double *a);
	void icdft(complex *a) { icdft( (double *) a); }
	void sifft(short int *siData, double *out);
	void sifft(short int *siData, complex *a) { sifft(siData, (double *) a); }
	void rdft(double *a);
	void rdft(complex *a) { rdft( (double *) a); }
	void irdft(double *a);
	void irdft(complex *a) { irdft( (double *) a); }
	
	void setWindow(fftPrefilter pf);
};

#endif
