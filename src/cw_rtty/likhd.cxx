// ----------------------------------------------------------------------------
// likhd.c --  bayesian morse code decoder
//
// Copyright (C) 2012-2014
//			 (C) Mauri Niininen, AG1LE
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
// ---------------------------------------------------------------------------

#include <config.h>

#include "bmorse.h"
#include <stdio.h>

int morse::likhd_(float z, float rn, long int ip, long int lambda, float dur, long int ilrate)
{

	/* Local variables */
	long int i, j, k, n;
	float pinr;
//	long int ilx;
	long int ixs;
//	float lkhdj;
	long int kelem;
//	long int israte;

/* 	THIS SUBROUTINE CALCULATES,FOR EACH PATH */
/* 	EXTENSION TO STATE N, THE LIKELIHOOD OF THAT */
/* 	TRANSITION GIVEN THE MEASUREMENTZ. IT USES */
/* 	AN ARRAY OF LINEAR (KALMAN) FILTERS TO DO SO. */

/* 	VARIABLES: */
/* 	Z- 	INPUT MEASUREMENT */
/* 	RN-	INPUT NOISE POWER ESTIMATE */
/* 	IP-	INPUT SAVED PATH IDENTITY */
/* 	LAMBDA-	INPUT SAVED LTR STATE IDENTITY */
/* 	DUR-	INPUT SAVED DURATION OF ELEMENT ON PATH IP */
/* 	ILRATE-	INPUT SAVED DATA RATE (SPEED) */
/* 	P-	INPUT TRANSITION PROBABILITIES */
/* 	LKHD-	OUTPUT COMPUTED LIKELIHOODS FOR EACH TRANS */

/*  SUBROUTINES USED: */
/* 	KALFIL-KALMAN FILTER FOR EACH NEW PATH */

/*   OBTAIN SAVED KEYSTATE: */


	/* Function Body */
	if (lambda == 0)
		return 0;

	kelem = ilami[ielmst[lambda - 1] - 1];

/* 	FOR EACH STATE: */
	for (k = 1; k <= 6; ++k) {
		for (i = 1; i <= 5; ++i) {
/* 	OBTAIN KEYSTATE, RATE STATE, STATE N, NEW NODE: */
			ixs = isx[k - 1];
			n = (i - 1) * 6 + k;
			j = (ip ) * 30 + n;
			pinr = pin[n-1][ip];
/* 	COMPUTE AND STORE LIKELIHOOD: */

			lkhd[n-1][ip] = kalfil_(z, ip, rn, ixs, kelem, j, dur, ilrate, pinr);
		}
	}
	return 0;
} /* likhd_ */

