// ----------------------------------------------------------------------------
// kalfil.c -- bayesian morse code decoder 
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
// ---------------------------------------------------------------------------

#include <config.h>

#include "bmorse.h"
#include <math.h> 
#include <stdio.h>


float morse::kalfil_(float z, long int ip, float rn, long int ixs, long int kelem, long int jnode, float dur, long int ilrate, float pin)
{
    /* Initialized data */

//    static float pinmin = 1e-4f;


    /* System generated locals */
//    float r1;


    /* Local variables */
    float a, g, qa, hz, pz, zr, phi, pkk, ykk, pest;
    float ppred, ypred, pzinv;


/*   THIS SUBROUTINE COMPUTES THE ARRAY OF KALMAN FILTER */
/*   RECURSIONS USED TO DETERMINE THE LIKELIHOODS. */

/*   VARIABLES: */
/*       Z -	INPUT MEASUREMENT */
/*       IP -	INPUT PATH IDENTITY */
/*       RN -	INPUT NOISE POWER ESTIMATE */
/*       ILX -	INPUT SAVED KEYSTATE ON PATH IP */
/*       IXS -	INPUT KEYSTAT OF NEW NODE */
/*       KELEM -	INPUT ELEM STATE OF NEW NODE */
/*       ISRATE 	INPUT SPEED STATE OF NEW NODE */
/*       DUR - 	INPUT CURRENT DURATION OF ELEMENT ON IP */
/*       ILRATE 	INPUT SPEED STATE ON PATH IP */
/*       PIN - 	TRANS PROB FROM PATH IP TO NODE N */
/*       LKHDJ - OUTPUT CALCULATED LIKELIHOOD VALUE */

/*   SUBROUTINES USED */
/*       MODEL - OBTAINS THE SIGNAL-STATE-DEPENDENT LINEAR */
/*       	     MODEL FOR THE KALMAN FILTER RECURSIONS */

/*   IF TRANSITION PROBABILITY IS VERY SMALL, DON'T */
/*   BOTHER WITH LIKELIHOOD CALCULATION: */

    if (pin <= 1e-4f) {  // was 1e-4f
		return 0.f;
    }

/*   OBTAIN STATE-DEPENDENT MODEL PARAMETERS: */
    model_(dur, kelem, ilrate,  ixs, &phi, &qa);

/* 	COMPUTE MEASUREMENT COEFFICIENT: */
    hz = (float)ixs;
    
/* 	GET PREVIOUS ESTIMATES FOR PATH IP */

    ykk = ykkip[ip];
    pkk = pkkip[ip];

/*  IMPLEMENT KALMAN FILTER FOR THIS TRANSITION */

    ypred = phi * ykk;
    ppred = phi * pkk * phi + qa;
    pz = hz * ppred + rn;
    pzinv = 1.f / pz;
    g = ppred * hz * pzinv;
    pest = (1.f - g * hz) * ppred;
    zr = z - hz * ypred;

    ykksv[jnode - 1] = ypred + g * zr;
    pkksv[jnode - 1] = pest;
    if (ykksv[jnode - 1] <= .01f) {
		ykksv[jnode - 1] = .01f;
    }
/* Computing 2nd power */
    a = .5f*pzinv*(zr * zr);
    if (a > 1e3f) {
		return 0.f;
    }
    return (1.f / sqrt(pz)) * exp(-a);
} /* kalfil_ */

