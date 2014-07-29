// ----------------------------------------------------------------------------
// xtrans.c --  bayesian morse code decoder 
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
#include <math.h>

double morse::xtrans_(long int *ielem, float dur, long int irate)
{


    /* System generated locals */
    float ret_val;


    /* Local variables */
    float b0, b1, p0, p1, alpha;
    long int mscale;
    float rscale;

/* 	THIS FUNCTION IMPLEMENTS THE CALCULATION OF KEYSTATE */
/* 	TRANSITION PROBABILITY, CONDITIONED ON ELEMENT TYPE, */
/* 	CURRENT DURATION, AND DATA RATE. */
/* 	VARIABLES: */
/* 	IELEM- 	INPUT CURRENT ELEMENT TYPE */
/* 	DUR- 	INPUT CURRENT ELEMENT DURATION */
/* 	IRATE - INPUT CURRENT DATA RATE */

/* 	TABLES IN COMMON CONTAIN DENSITY PARMS FOR EACH ELEMENT TYPE, DATA RATE. */

/* 	SCALE DURATION AND OBTAIN DENSITY PARAMETER: */

    mscale = kimap[*ielem - 1];
    rscale = 1200.f / irate;
    b0 = dur / (mscale * rscale);
    b1 = (dur + 5.f) / (mscale * rscale);
    
    switch (*ielem) {
    case 6:		// element is Pause 
	    alpha = aparm[2] * 14.f;
	    break;
    case 5:		// element is word space 
	    alpha = aparm[1] * 7.f;    
	    break;
    default:	// element is dit, dah, e-space or chr-space
	    alpha = mscale * aparm[0];
    }

    if (b1 <= 1.f) {
		p1 = 1.f - exp(alpha * (b1 - 1.f)) * .5f;
		p0 = 1.f - exp(alpha * (b0 - 1.f)) * .5f;
		ret_val = p1 / p0;
	    return ret_val;
    }
    if (b0 < 1.f && b1 > 1.f) {
		p1 = exp(-alpha * (b1 - 1.f)) * -.5f;
		p0 = 1.f - exp(alpha * (b0 - 1.f)) * .5f;
		ret_val = p1 / p0;
	    return ret_val;
    }

    ret_val = exp(-alpha * (b1 - b0));
    return ret_val;
} /* xtrans_ */

