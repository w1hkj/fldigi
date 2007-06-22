#include <stdlib.h>
#include <iostream>
#include <math.h>

using namespace std;

#undef  CLAMP
#define CLAMP(x,low,high)       (((x)>(high))?(high):(((x)<(low))?(low):(x)))

int symlen = 3072;
int numtones = 5;
int numchars = 2;
double w[10];
double outbuf[3072];
int frequency = 500;
int tonespacing = 6;
double sr = 8000;

void make_tones()
{
	double f;
	for (int j = 0; j < numchars; j++)
		for (int i = 0; i < numtones; i++) {
			f = frequency + numtones*tonespacing - i*tonespacing - j*(numtones + 1)*tonespacing;
			w[i + numtones * j] = 2 * M_PI * f / sr;
//			cout << i + numtones * j << "\t" << f << "\t" << w[i + numchars *j] << endl;
		}
}

double peakval(int symbol, int mask)
{
	int i, j;
	int sym;
	int msk;
	double maximum = 0.0;
	for (i = 0; i < symlen; i++) {
		outbuf[i] = 0.0;
		sym = symbol;
		msk = mask;
		for (j = 0; j < numtones * numchars; j++) {
			if (sym & 1 == 1)
				outbuf[i] += (msk & 1 == 1 ? -1 : 1 ) * sin(w[j] * i);
			sym = sym >> 1;
			msk = msk >> 1;
		}
		if (fabs(outbuf[i]) > maximum)
			maximum = fabs(outbuf[i]);
	}
	return maximum;
}

void findmins()
{
	double	peak;
	int		mask;
	double	testval;
	int i,j;
	cout << "0x00, ";
	for (i = 1; i < 1024; i++) {
		peak = 1e10;
		for (j = 0; j <= i; j++) {
			testval = peakval(i,j);
			if (peak > testval) {
				peak = testval;
				mask = j;
			}
		}
		cout << hex << "0x" << mask << ", " ;
		if (i % 16 == 0)
			cout << endl;
	}
}

int main (int argc, char *argv[])
{
	make_tones();
	findmins();
}

