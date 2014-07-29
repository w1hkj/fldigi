// ----------------------------------------------------------------------------
// bmorse.h --  bayesian morse code decoder
//
// Copyright (C) 2012-2014
//		     (C) Mauri Niininen, AG1LE
//
// This file is part of Bayesian Morse code decoder

// bmorse is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// bmorse is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with bmorse.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#define FSAMPLE 8000.0			// Sampling Frequency  FLDIGI=8000   MORSE.M =4000
#define DECIMATE  40			// Decimation     FLDIGI=40    MORSE.M=20
#define SAMPLEDURATION  (1000. * DECIMATE) / FSAMPLE	// 1000*DECIMATE / FSAMPLE SHOULD BE  5 msec
#define NDELAY  200				// 200 SAMPLES * 5 msec = 1000 msec decoding delay
#define BAYES_RATE 200			// Bayes decoder expects to get signal envelope at 200 Hz
#define PATHS 25				// 25-30 paths normal

#define TRUE 	1
#define FALSE 	0

typedef struct
{	int print_variables ;
	int print_symbols;
	int print_speed;
	int process_textfile;
	int print_text;
	int print_xplot;
	int width, speclen ;
	int bfv;
	double frequency;
	double sample_duration;
	double sample_rate;
	double delta;
	double amplify;
	int fft;
	int agc;
	int speed;
	int dec_ratio;
} PARAMS ;

extern PARAMS params;

class morse {
protected:
	int initl_(void);
	int likhd_(float z, float rn, long int ip, long int lambda, float dur, long int ilrate);

	int path_(long int ip, long int lambda, float dur, long int ilrate, long int *lamsav, float *dursav, long int *ilrsav);
	double spdtr_(long int isrt, long int ilrt, long int iselm, long int ilelm);
	int ptrans_(long int kelem, long int irate, long int lambda, long int ilrate, float ptrx, float *psum, float *pin, long int n);
	int trprob_(long int ip, long int lambda, float dur, long int ilrate);
	int transl_(int ltr,char *buf);
	int trelis_(long int *isave, long int *pathsv, long int *lambda, long int *imax, long int *ipmax,char *buf);
	float kalfil_(float z, long int ip, float rn, long int ixs, long int kelem, long int jnode, float dur, long int ilrate, float pin);
	int savep_(float *p, long int *pathsv, long int *isave, long int
		*imax, long int *lamsav, float *dursav, long int *ilrsav, long int *
		lambda, float *dur, long int *ilrate, long int *sort, float *pmax);
	int model_(float , long int , long int , long int , float *, float *);
	int probp_(float *, long int *);
	int  sprob_(float *, long int *, long int *, float *, long int *, float *, float *);
	double xtrans_(long int *, float , long int );


// static data members
// Xtrans variables - Initialized data
	static const long int kimap[6];
	static const float aparm[3];
	static const long int isx[6];
	static const float rtrans[2][5];
	static const long int mempr[6][6];
	static const long int memdel[6][6];
	static const float elemtr[6][16];
	static const long int memfcn[6][400];
	static const long int ielmst[400];
	static const long int ilami[16];
	static const long int ilamx[6];
// end of static data members

// Proces variables -  Initialized data

    long int isave;
    long int lambda[PATHS];
	long int ilrate[PATHS];
    float dur[PATHS];
    long int pathsv[PATHS];
    long int sort[PATHS];

    float p[30*PATHS];
    long int lamsav[30*PATHS];
    float dursav[30*PATHS];
    long int ilrsav[30*PATHS];

// Trelis variables -  Initialized data

    long int lmdsav[PATHS][NDELAY];	/* was [200][PATHS] */
    long int n;
    long int ndelay;
    long int ipnod[PATHS];
    long int ltrsv[NDELAY];
    long int pthtrl[PATHS][NDELAY];	/* was [200][PATHS] */

    long int ncall;
    long int nmax;
    long int mmax;
    long int kd;
    long int ndelst;
    long int iend;

// Transl variables - Initialized data
	long int ixlast;
	int curstate;
	int newstate;

//  Sprob variables
	int initial;

// Kalman filter parameters
    float ykkip[PATHS];
    float pkkip[PATHS];
    float ykksv[30*PATHS];
    float pkksv[30*PATHS];

// used in trprob and in likhd
    float pin[30][PATHS];	/* was pin[750] -  	PIN[N,I]- COMPUTED TRANS PROB FROM PATH I TO STATE N */
// used in likhd
    float lkhd[30][PATHS];	// was lkhd[750] - LKHD[N,I]- LIKELIHOOD VALUE FOR EACH PATH I AND STATE N */

public:
	morse()
	{
		initl_();
	};
	~morse();
	void init() {
		initl_();
	};
	int noise_(double, float *, float *);
	int proces_(float z, float rn, long int *xhat, float *px, long int *elmhat, float *spdhat, long int *imax, float *pmax, char *buf);
};



