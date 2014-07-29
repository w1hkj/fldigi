// ----------------------------------------------------------------------------
// ptrans.c --  bayesian morse code decoder 
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



int morse::ptrans_(long int kelem, long int irate, long int lambda, long int ilrate, float ptrx, float *psum, float *pint, long int n)
{
	float pelem, prate;


/* 	THIS FUNCTION SUBROUTINE RETURNS THE PATH CONDITIONAL TRANSITION */
/* 	PROBABILITIES TO EACH ALLOWABLE STATE N. */
/* 	VARIABLES: */
/* 	KELEM-      INPUT CURRENT ELEMENT STATE */
/* 	IRATE-      INPUT CURRENT DATA RATE STATE */
/* 	LAMBDA-     INPUT IDENTITY OF CURRENT LTR STATE */
/* 	PTRX-       INPUT KEYSTATE TRANSITION PROBABILITY */
/* 	ELEMTR-     ELEMENT TRANSITION PROBABILITY MATRIX */

/* 	FUNCTION SUBROUTINE USED: */
/* 	SPDTR-      RETURNS DATA RATE TRANSITION PROBS,CONDITIONED ON CURRENT SPACE TYPE. */

/* 	IF THE SAVED ELEMENT AND THE ELEMENT OF THE STATE */
/* 	N TO WHICH THE PATH IS BEING EXTENDED ARE THE */
/* 	SAME, THEN THE STATE TRANS PROB IS SIMPLY KEYSTATE TRANS PROB: */
    if (kelem != ilami[ielmst[lambda - 1] - 1]) {
		goto L100;
    }
    pint[n-1] = ptrx;

/* 	HOWEVER, IF CURRENT DATA RATE STATE  != 3, THEN STATE TRANS PROB = 0 ... WHY ? */
/* See page 104 in thesis */
    if (irate != 3) {
		pint[n-1] = 0.f;
    }
    goto L200;

/* 	OTHERWISE: */
/* 	OBTAIN ELEM TRANS PROBS TABLE: */

L100:
    pelem = elemtr[kelem-1][ielmst[lambda - 1]-1];

/* 	NEXT COMPUTE ELEM-CONDITIONAL SPEED TRANS PROB: */
/* 	 1  2  3  4  5  6  7  8 9 10 11 12 13 14 15 16 */
/* 	.^ .~ .w .p -^ -~ -w -p ^. ^- ~. ~- w. w- p. p- */

// 	ilami { 3, 4, 5, 6, 3, 4, 5, 6, 1, 2, 1, 2, 1, 2, 1, 2},
//  ielmst { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 0, 0,...}

    prate = spdtr_(irate, ilrate, kelem, ilami[ielmst[lambda - 1] - 1]);

/* 	TRANS PROBABILITY IS THE PRODUCT: */
    pint[n-1] = (1.f - ptrx) * pelem * prate;
L200:
    *psum += pint[n-1];
    return 0;
} /* ptrans_ */

