#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

struct MAG2RGBI {
	int R;
	int G;
	int B;
	int I;
	} mag2RGBI[256];

void setcolors() {
	double di;
	for (int i = 0; i < 256; i++) {
		di = sqrt(i/256.0);
		mag2RGBI[i].I = (int)(256 * di);
		if ( i < 32 ) {
			mag2RGBI[i].R = 0;
			mag2RGBI[i].G = 0;
			mag2RGBI[i].B = i*8;
		} else if (i < 160) {
			mag2RGBI[i].R = 0;
			mag2RGBI[i].G = (i - 32) * 2;
			mag2RGBI[i].B = 256 - (i - 32) * 2;
		} else if ( i < 224) {
			mag2RGBI[i].R = (i - 160) * 4;
			mag2RGBI[i].G = 256 - (i-160)*4;
			mag2RGBI[i].B = 0;
		} else {
			mag2RGBI[i].R = 255;
			mag2RGBI[i].G = (i - 224)*8;
			mag2RGBI[i].B = (i - 224)*8;
		}
	}
}

int main(int argc, char *argv[]) {
	ofstream out("colormap.csv");
	if (!out) return 1;
	setcolors();
	for (int i = 0; i < 256; i++)
		out << mag2RGBI[i].R << "," 
			<< mag2RGBI[i].G << "," 
			<< mag2RGBI[i].B << "," 
			<< mag2RGBI[i].I << endl;
	out.close();
	
	return 0;
}
