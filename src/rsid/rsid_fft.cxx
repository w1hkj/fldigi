/*
CC=================================================================CC
CC                                                                 CC
CC  Subroutine RSRFFT(X,M):                                        CC
CC      A real-valued, in-place, split-radix FFT program           CC
CC      Decimation-in-time, cos/sin in second loop                 CC
CC      and computed recursively                                   CC
CC      Output in order:                                           CC
CC              [ Re(0),Re(1),....,Re(N/2),Im(N/2-1),...Im(1)]     CC
CC                                                                 CC
CC  Input/output                                                   CC
CC      X    Array of input/output (length >= N)                   CC
CC      M    Transform length is N=2**M                            CC
CC                                                                 CC
CC  Calls:                                                         CC
CC      RSTAGE,RBITREV                                             CC
CC                                                                 CC
CC  Author:                                                        CC
CC      H.V. Sorensen,   University of Pennsylvania,  Oct. 1985    CC
CC                       Arpa address: hvs@ee.upenn.edu            CC
CC  Modified:                                                      CC
CC      F. Bonzanigo,    ETH-Zurich,                  Sep. 1986    CC
CC      H.V. Sorensen,   University of Pennsylvania,  Mar. 1987    CC
CC      H.V. Sorensen,   University of Pennsylvania,  Oct. 1987    CC
CC                                                                 CC
CC  Reference:                                                     CC
CC      Sorensen, Jones, Heideman, Burrus :"Real-valued fast       CC
CC      Fourier transform algorithms", IEEE Tran. ASSP,            CC
CC      Vol. ASSP-35, No. 6, pp. 849-864, June 1987                CC
CC                                                                 CC
CC      This program may be used and distributed freely as         CC
CC      as long as this header is included                         CC
CC                                                                 CC
CC=================================================================CC
*/

#include <math.h>
#include "rsid_fft.h"

void rsrfft( double *x, int m )
{
	int n, is, id, i0, n2, n4, k;
	double t1;

	x--;
	n = 1 << m;
/*C-------Digit reverse counter---------------------------------------C*/
	rbitrev( x, m);
/*C-----Length two butterflies----------------------------------------C*/
	is = 1;
	id = 4;
	do {
		for (i0 = is; i0 <= n; i0 += id) {
			t1 = x[i0];
			x[i0]   = t1 + x[i0+1];
			x[i0+1] = t1 - x[i0+1];
		}
		is = 2*id-1;
		id = 4*id;
	} while (is < n);
/*C-------L shaped butterflies----------------------------------------C*/
	n2 = 2;
	for (k = 2; k <= m; k++) {
		n2 = n2 * 2;
		n4 = n2 / 4;
		rstage(n, n2, n4, &x[0], &x[n4], &x[2 * n4], &x[3 * n4]);
	}
	return;
}

/*
C===================================================================C
C  Subroutine RSTAGE - the work-horse of the RFFT                   C
C       Computes a stage of a real-valued split-radix length N      C
C       transform.                                                  C
C  Author                                                           C
C       H.V. Sorensen,   University of Pennsylvania,  Mar. 1987     C
C===================================================================C
*/
void rstage(
		int n, int n2, int n4, 
		double *x1, double *x2, double *x3, double *x4)
{
	int n8, is, id, i1, i2, j, jn;
	double t1, t2, e, t3, t4, t5;
	double ss1, sd1, ss3, sd3;
	double cc1, cd1, cc3, cd3;

	n8 = n2 / 8;
	is = 0;
	id = n2*2;

	do {
		for (i1 = is + 1; i1 <= n; i1 += id) {
			t1     = x4[i1]+x3[i1];
			x4[i1] = x4[i1]-x3[i1];
			x3[i1] = x1[i1]-t1;
			x1[i1] = x1[i1]+t1;
		}
		is = 2*id - n2;
		id = 4*id;
	} while (is < n);

	if (n4-1 <= 0) return;
	  
	is = 0;
	id = n2 * 2;
	do {
		for (i2 = is + 1 + n8; i2 <= n; i2 += id) {
			t1     = (x3[i2] + x4[i2]) * .7071067811865475;
			t2     = (x3[i2] - x4[i2]) * .7071067811865475;
			x4[i2] =  x2[i2] - t1;
			x3[i2] = -x2[i2] - t1;
			x2[i2] =  x1[i2] - t2;
			x1[i2] =  x1[i2] + t2;
		}
		is = 2 * id - n2;
		id = 4 * id;
	} while (is < n);

	if (n8-1 <= 0) return;

	e  = 2 * 3.14159265358979323 / n2;
	ss1 = sin(e);
	sd1 = ss1;
	sd3 = 3 * sd1 - 4 * (sd1 * sd1 * sd1);
	ss3 = sd3;
	cc1 = cos(e);
	cd1 = cc1;
	cd3 = 4 * (cd1 * cd1 * cd1) - 3.0 * cd1;
	cc3 = cd3;

	for (j = 2; j <= n8; j++) {
		is = 0;
		id = 2 * n2;
		jn = n4 - 2*j + 2;
		do {
			for (i1 = is + j; i1 <= n; i1 += id) {
				i2 = i1 + jn;
				t1 = x3[i1]*cc1 + x3[i2]*ss1;
				t2 = x3[i2]*cc1 - x3[i1]*ss1;
				t3 = x4[i1]*cc3 + x4[i2]*ss3;
				t4 = x4[i2]*cc3 - x4[i1]*ss3;
				t5 = t1 + t3;
				t3 = t1 - t3;
				t1 = t2 + t4;
				t4 = t2 - t4;
				x3[i1] = t1 - x2[i2];
				x4[i2] = t1 + x2[i2];
				x3[i2] = -x2[i1] - t3;
				x4[i1] =  x2[i1] - t3;
				x2[i2] = x1[i1] - t5;
				x1[i1] = x1[i1] + t5;
				x2[i1] = x1[i2] + t4;
				x1[i2] = x1[i2] - t4;
			}
			is = 2*id - n2;
			id = 4*id;
		} while (is < n);

		t1  = cc1*cd1 - ss1*sd1;
		ss1 = cc1*sd1 + ss1*cd1;
		cc1 = t1;
		t3  = cc3*cd3 - ss3*sd3;
		ss3 = cc3*sd3 + ss3*cd3;
		cc3 = t3;
    }
 return;
}


/*
CC=================================================================CC
CC                                                                 CC
CC Subroutine RBITREV(X,M):                                        CC
CC      Bitreverses the array X of length 2**M. It generates a     CC
CC      table ITAB (minimum length is SQRT(2**M) if M is even      CC
CC      or SQRT(2*2**M) if M is odd). ITAB need only be generated  CC
CC      once for a given transform length.                         CC
CC                                                                 CC
CC Author:                                                         CC
CC      H.V. Sorensen,   University of Pennsylvania,  Aug. 1987    CC
CC                       Arpa address: hvs@ee.upenn.edu            CC
CC                                                                 CC
CC      This program may be used and distributed freely as long    CC
CC      as this header is included.                                CC
CC                                                                 CC
CC=================================================================CC
*/
void rbitrev(double *x, int m)
{
	int itab[257];
	int m2, nbit, imax, lbss, i, j, k, l, j0;
	double t1; 
/*C-------Initialization of ITAB array--------------------------------C*/
	m2 = m / 2;
	nbit = 1 << m2;
	if (2 * m2 != m) m2 = m2 + 1;
	itab[1] = 0;
	itab[2] = 1;
	imax = 1;

	for (lbss = 2; lbss <= m2; lbss++) {
		imax = 2 * imax;
		for (i=1;i<=imax;i++) {
			itab[i]      = 2 * itab[i];
			itab[i+imax] = 1 + itab[i];
		}
	}

/*C-------The actual bitreversal--------------------------------------C*/
	for ( k =2; k <= nbit; k++) {
		j0 = nbit * itab[k] + 1;
		i = k;
		j = j0;
		for (l=2;l<=itab[k]+1;l++) {
			t1   = x[i];
			x[i] = x[j];
			x[j] = t1;
			i = i + nbit;
			j = j0 + itab[l];
		}
	}
	return;
}

