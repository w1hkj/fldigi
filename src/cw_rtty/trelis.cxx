// ----------------------------------------------------------------------------
// trelis.c --  bayesian morse code decoder 
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

#include <stdio.h> 
#include "bmorse.h"

#include "debug.h"

//#define DEBUG 1


int morse::trelis_(long int *isave, long int *pathsv, long int *lambda, long int *imax, long int *ipmax, char *buf)
{
    /* Local variables */
    int i, k, ip, ieq, ltr, ndel = 1, retstat;
//    static int isavg;
    static int init=0;
    static float xsavg, xmmax, xnmax;
//    int ndlavg;
    static float xdlavg;

/*    THIS SUBROUTINE STORES THE SAVED NODES AT EACH */
/*    STAGE AND FORMS THE TREE OF SAVED PATHS LINKING */
/*    THE NODES. DECODING IS ACCOMPLISHED BY FINDING */
/*    THE CONVERGENT PATH IF IT OCCURS WITHIN A MAXIMUM */
/*    DELAY SET BY THE PARAMETER NDELAY. IF CONVERGENCE */
/*    TO A SINGLE PATH DOES NOT OCCURS, THEN DECODING IS */
/*    DONE BY READING THE LETTER ON THE PATH WITH HIGHEST */
/*    PROBABILITY */

    /* Parameter adjustments */
    --lambda;
    --pathsv;

    /* initialize variables*/
 	if (init ==0) {
 		init = 1; 
		for (i=0; i< NDELAY; i++)
			ltrsv[i] = 0;
		for (int row =0; row < PATHS; row++) {
			ipnod[row] = 1;
			for (int col =0; col < NDELAY; col++){
					lmdsav[row][col] = 0; 
					pthtrl[row][col] = 0;
			}
		}
	}
    
    
/* 	KEEP AVERAGE OF ISAVE, NDEL FOR DATA ANALYSIS: */

	retstat = 1;
    ++ncall;
    if (iend == 1) {
//		isavg = xsavg;
//		ndlavg = xdlavg;
		iend = 0;
		LOG_INFO("AVG # OF PATHS SAVED:%4.2f,AVG DECODE DELAY:%4.2f)", xsavg, xdlavg);
		LOG_INFO("PERCENT OF TIME PATHS = 25: %3.2f, PERCENT OF TIME DELAY = 200: %3.2f", xmmax, xnmax);
//		printf("\nAVG # OF PATHS SAVED:%4.2f,AVG DECODE DELAY:%4.2f)", xsavg, xdlavg);
//		printf("\nPERCENT OF TIME PATHS = 25: %3.2f, PERCENT OF TIME DELAY = 200: %3.2f", xmmax, xnmax);
    }
    xsavg = (xsavg * (ncall - 1) + *isave) / ncall;
    xdlavg = (xdlavg * (ncall - 1) + ndel) / ncall;
    if (ndel == ndelay) {
		++nmax;
		xnmax = (float) nmax;
		xnmax /= ncall;
    }
    if (*isave == PATHS) {
		++mmax;
		xmmax = (float) mmax;
		xmmax /= ncall;
    }

/* 	STORE PATHSV AND CORRESPONDING LAMBDA IN THE */
/* 	TRELLIS USING A CIRCULAR BUFFER OF LENGTH NDELAY : */
    ++n;
    if (n == ndelay + 1) {
		n = 1;
    }

    for (i = 1; i <= *isave; ++i) {
		pthtrl[i-1][n-1] = pathsv[i];		
		lmdsav[i-1][n-1] = (long int)lambda[i];
    }

/* 	PERFORM DYNAMIC PROGRAM ROUTINE TO FIND CONVERGENT PATH: */
    k = 0;
    for (i = 1; i <= *isave; ++i) {
		ipnod[i - 1] = i;
    }
L190:
    ++k;
    if (k == ndelay) {
		goto L700;
    }

/* 	IF IP EQUALS INDEX OF HIGHEST PROBABILITY NODE, STORE NODE TO IPMAX */
    for (ip = 1; ip <= *isave; ++ip) {
		i = n - k + 1;
		if (i <= 0) {
			i = ndelay + i;
		}
		ipnod[ip - 1] = pthtrl[ipnod[ip - 1]-1][i-1];
		if (ip == *imax) {
			*ipmax = ipnod[ip - 1];
		}
    }

/* 	IF ALL NODES ARE EQUAL,THEN PATHS CONVERGE: */

    for (ieq = 2; ieq <= *isave; ++ieq) {
		if (ipnod[0] != ipnod[ieq - 1]) {
			goto L190;
		}
    }

/* 	PATHS CONVERGE; SET NDEL: */
    ndel = k + 1;

/* 	IF POINT OF CONVERGENCE IS SAME AS IT WAS ON */
/* 	LAST CALL, THEN NO NEED TO RE-DECODE SAME NODE: */
    if (ndel == ndelst + 1) {
		goto L800;
    }
/* 	IF POINT OF CONVERGENCE OCCURS AT SAME DELAY AS LAST CALL, THEN TRANSLATE: */
    if (ndel != ndelst) {
		goto L350;
    }
    i = n - ndel + 1;
    if (i <= 0) {
		i = ndelay + i;
    }
    ltr = lmdsav[ipnod[0]-1][i-1];
    retstat = transl_(ltr,buf);
    goto L800;

/* 	OTHERWISE,POINT OF CONVERGENCE HAS OCCURED */
/* 	EARLIER ON THIS CALL, SO NEED TO TRANSLATE */
/* 	EVERYTHING ON THE CONVERGENT PATH FROM */
/* 	PREVIOUS POINT OF CONVERGENCE TO THIS POINT: */
L350:
    kd = 0;
    ip = ipnod[0];
    for (k = ndel; k <= ndelst; ++k) {
		++kd;
		i = n - k + 1;
		if (i <= 0) {
			i = ndelay + i;
		}
		ltrsv[kd - 1] = lmdsav[ip-1][i-1];
		ip = pthtrl[ip-1][i-1];
    }

/* 	REVERSE ORDER OF DECODED LETTERS, SINCE THEY */
/* 	WERE OBTAINED FROM THE TRELLIS IN REVERSE; */
/* 	TRANSLATE EACH: */

    for (i = 1; i <= kd; ++i) {
		ltr = ltrsv[kd - i];
		retstat = transl_(ltr,buf);
    }
    goto L800;

L700:
/* 	PATHS HAVE NOT CONVERGED AT MAXIMUM ALLOWABLE */
/* 	DELAY, SO TRANSLATE WHAT IS ON HIGHEST */
/* 	PROBABILITY PATH: */
    ndel = ndelay;
    i = n - ndelay + 1;
    if (i <= 0) {
		i = ndelay + i;
    }
    ltr = lmdsav[*ipmax-1][i -1];
    retstat = transl_(ltr,buf);

/* 	PRUNE AWAY NODES WHICH ARE NOT ON THIS PATH: */
    for (k = 1; k <= *isave; ++k) {
		if (ipnod[k - 1] != *ipmax) {
			lambda[k] = 0;
		}
    }

L800:
    ndelst = ndel;
    return retstat;
} /* trelis_ */

