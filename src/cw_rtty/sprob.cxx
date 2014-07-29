// ----------------------------------------------------------------------------
// sprob.c --  bayesian morse code decoder 
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

#include <stdio.h>

extern char debug; 
extern PARAMS params; 

int morse::sprob_(float *p, long int *isave, long int *ilrsav, float *pelm, long int *khat, float *spdhat, float *px)
{

    /* Local variables */
	long int i, j, k, m, n;
	float pselem[6];



/*    SPROB COMPUTES THE POSTERIOR PROBS OF THE ELEMENT */
/*    STATES, DATA RATE STATES, AND KEYSTATES BY SUMMING */
/*    OVER THE APPROPRIATE PATHS. */

/*    VARIABLE: */
/*    P-		INPUT PATH PROBABILITIES */
/*    ISAVE- 	NUMBER OF PATHS SAVED */
/*    PSELEM-	OUTPUT ELEMENT PROB */
/*    SPDHAT-	OUTPUT SPEED ESTIMATE (DATA RATE WPM) */
/*    PX- 		OUTPUT KEYSTATE PROBABILITY */

/* 	INITIALIZE: */

    /* Function Body */
    *spdhat = 0.f;
    *px = 0.f;
/* 	FOR EACH STATE EXTENSION OF PATH M: */
/* 	OBTAIN ELEMENT STATE PROBS,KEYSTATE PROBS,SPEED EST: */
    for (k = 1; k <= 6; ++k) {
		pselem[k - 1] = 0.f;
		for (i = 1; i <= 5; ++i) {
			n = (i - 1) * 6 + k;
			for (m = 1; m <= *isave; ++m) {
				j = (m - 1) * 30 + n;
				pselem[k - 1] += p[j-1];
				*spdhat += ilrsav[j-1] * p[j-1];
				if (k <= 2) {
					*px += p[j-1];
				}
			}
		}
    }
    *pelm = 0.f;

    for (k = 1; k <= 6; ++k) {
		// IF WANT ELEMENT PROBABILITIES BY SAMPLE ENABLE debug flag -var
		if (params.print_variables && initial) {
			printf("\t%f",pselem[k - 1]);
		}
		if (pselem[k - 1] >= *pelm) {
			*pelm = pselem[k - 1];
			*khat = k;
		}
    }
   	if (params.print_variables && initial) 
   		printf("\t");
    initial = 1; 
    return 0;
} /* sprob_ */

