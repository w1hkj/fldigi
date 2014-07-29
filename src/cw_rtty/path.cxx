// ----------------------------------------------------------------------------
// path.cxx --  bayesian morse code decoder
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

#include <config.h>

#include "bmorse.h"

int morse::path_(long int ip, long int lambda, float dur, long int ilrate, long int *lamsav, float *dursav, long int *ilrsav)
{
     /* Local variables */
    long int i, j, k, n, ixl, ixs, ilelm;



/*  PATH COMPUTES THE LTR STATE, DURATION, AND DATA RATE OF */
/*  EACH NEW PATH EXTENDED TO STATE N */

/*  VARIABLES: */
/*  IP-		SAVED PATH IDENTITY */
/*  LAMBDA-	LTR STATE OF SAVED PATH */
/*  DUR-		DURATION OF ELEMENT ON SAVED PATH */
/*  ILRATE-	DATA RATE OF ELEMENT ON SAVED PATH */
/*  LAMSAV-	NEW LTR STATES FOR EACH PATH EXTENSION */
/*  DURSAV-	NEW ELEM DURATIONS FOR EACH PATH EXTENSION */
/*  ILRSAV-	NEW DATA RATES FOR EACH PATH EXTENSION */
/*  J-		NEW PATH IDENTITY */
/*  THE LETTER TRANSITION TABLE, MEMFCN, IS STORED IN COMMON. */

/*  FOR EACH ELEM STATE K, AND EACH SPEED I, COMPUTE: */



    /* Function Body */
    for (k = 1; k <= 6; ++k) { 		// 6 element states 1=dit,2=dah, 3=e-spc, 4=chr-s, 5=wrd-s, 6=pause
		for (i = 1; i <= 5; ++i) { 	// 5 speed (rate) states -2 -1 0 1 2

/*  STATE IDENTITY N: */
			n = (i - 1) * 6 + k;

/*  NEW PATH IDENTITY: */
			j = (ip) * 30 + n;

/*  NEW LTR STATE: */

			if (lambda == 0) {
				lamsav[j-1] = 0;
				continue;
			}

			lamsav[j-1] = memfcn[k-1][lambda-1];
			if (lamsav[j-1] == 0) {
				continue;
			}

/*  NEW DURATION: OBTAIN KEYSTATE OF SAVED PATH AND NEW STATE: */
			ilelm = ilami[ielmst[lambda - 1] - 1];
			ixl = ilamx[ilelm - 1];
			ixs = isx[k - 1];

/* CALCULATE DURATION - ADD SAMPLE DURATION 5 ms FOR EACH VALID PATH */
			dursav[j-1] = dur * (1 - ixs - ixl + (ixs << 1) * ixl) + params.sample_duration;

/* 	NEW DATA RATE: */
			ilrsav[j-1] = ilrate + (i - 3) * memdel[k-1][ilelm -1];
		}
    }
    return 0;
} /* path_ */

