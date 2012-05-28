// ----------------------------------------------------------------------------
//    mfilt.cxx  --  Matched Filter 
//
// Filter implemented using matched filter convolution method
// h(t) characterized by CW pulse of "dit" length
// 
// Reference: 
//     http://en.wikipedia.com/wiki/Matched_filter
//     
//     
//
// Copyright (C) 2012 Mauri Niininen, AG1LE
//
// This file is part of fldigi.
//
// Fldigi is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#include <config.h>

#include <cmath>
#include <string.h>

#include <iostream>
using namespace std; 

#include "misc.h"

#include "mfilt.h"


mfilt::mfilt(double freq, int Fs, int speed, int buflen)
{
	outbuf = 0;
	inbuf  = 0;
	filter = 0; 

	create_filter(freq, Fs, speed, buflen);
}

mfilt::~mfilt()
{
	if (outbuf) delete [] outbuf;
	if (filter) delete [] filter;
	if (inbuf) delete [] inbuf;
}


void mfilt::create_filter(double freq, int Fs, int speed, int buflen)
{
	int i;

	double dit_time = 1.2/(double)speed;	// calculate "dit" time based on morse speed 
	
	filterlen = Fs*dit_time; 		// how many samples needed for "dit" template (kernel) 

	inbuflen = buflen; 		// input buffer needs to be min 4x "dit" length 

//	cout << "\ndeleting buffers - inbuflen=" << buflen;

	// if this is an update delete previous buffers
	if (outbuf) delete [] outbuf;
	if (filter) delete [] filter;
	if (inbuf) delete [] inbuf;

//	cout << "\nallocating buffers - inbuflen=" << buflen;
	// allocate buffers
	outbuf = new double[inbuflen+filterlen+1];	// output buffer
	filter = new double[filterlen];			// dit template buffer
	inbuf  = new double[inbuflen+1];		// input buffer 

	inptr = 0;

//	cout << "\ncreating burst - filterlen=" << filterlen;
	// create a "dit" filter template - to be used in convolution
	double t = 0;
	for ( i = 0; i < filterlen; i++) {
		filter[i] = sin(2*M_PI*freq*t);
		t +=1.0/Fs;
	}

//cout << "fl & Inbuflen" << filterlen <<" "<< inbuflen << "\n";

}


bool mfilt::convolve(double *X,double *Y, double *Z, int lenx, int leny)
{
// Routine peforms linear convolution by straight forward calculation
// calculates  z= x convolve y
// Written by Clay S. Turner
//
// inputs:
//  X  array of data comprising vector #1
//  Y  array of data comprising vector #2
//  Z  pointer to place to save resulting data - needs to be lenx+leny-1 long
//  lenx  # of items in vector 1
//  leny  # of items in vector 2


	double *zptr,s,*xp,*yp;
	int lenz;
	int i,n,n_lo,n_hi;

	lenz=lenx+leny-1;
	zptr=Z;

	for (i=0;i<lenz;i++) {
		s=0.0;
		n_lo=0>(i-leny+1)?0:i-leny+1;
		n_hi=lenx-1<i?lenx-1:i;
		xp=X+n_lo;
		yp=Y+i-n_lo;
		for (n=n_lo;n<=n_hi;n++) {
			s+=*xp * *yp;
			xp++;
			yp--;
		}
		*zptr=s;
		zptr++;
	}

    return true;
}


/*
 * Filter with  convolution 
 */
int mfilt::run(const double *in, double **out, int *len)
{
	int  i, retval;

// collect inbuflen amount of input samples - need to be 4x dit length minimum
// if len = 512 need to copy data to inbuf until inptr == inbuflen
// if len > 512 we have a replay event, need to copy len amount to inbuf
	

	for (i=0; i < *len; i++) {
		inbuf[inptr++] = in[i];	
		if (inptr == inbuflen ) break; 
	} 

	if (inptr < inbuflen) return 1; 

// enough input samples collected - now do the convolution

	retval = convolve(inbuf, filter, outbuf, inbuflen, filterlen);
	*out = outbuf;
	*len = inbuflen+filterlen+1;
	inptr = 0;
	return 0;

}
