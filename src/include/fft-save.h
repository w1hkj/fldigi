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

#include <math.h>
#include <stdio.h>

enum fftPrefilter {FFT_RECT, FFT_HAMMING, FFT_HANNING};

// Rectangular - no pre filtering of data array
inline void RectWindow(double *array, int n) {
	for (int i = 0; i < n; i++)
		array[i] = 1.0;
}

// Hamming - used by gmfsk
inline void HammingWindow(double *array, int n) {
	double omega = 2*M_PI/(n-1);
	for(int i = 0; i < n; i++)
		array[i] = .54 - .46*cos( i * omega);
}

// Hanning - best separation of signals on waterfall		
inline void HanningWindow(double *array, int n) {
	double omega = 2*M_PI/(n-1);
	for(int i = 0; i < n; i++)
		array[i] = (.5 - .5*cos( i * omega ) );
}


class Cfft {
private:
	double xi;
	double *SinCos;
	double	*Filter;
	int  *ip;
	int  fftlen;
	int  smplsize;
	bool isRDFT;
    void InitSinCos();
    void makect();
    void bitrv2(int n, int *ip, double *a);
	void bitrv2conj(int n, int *ip, double *a);
    void cftfsub(int n, double *a);
	void cftbsub(int n, double *a);
    void rftfsub(int n, double *a, int nc, double *c);
	void cftmdl(int n, int l, double *a);
	void cft1st(int n, double *a);
public:
	Cfft(int n, fftPrefilter T = FFT_RECT, bool RDFT = false);
	~Cfft();
	void cdft(double *a);
	void icdft(double *a);
	void rdft(double *a);
	void fft(short int *siData, double *out);
};

#endif
