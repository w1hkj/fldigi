// ----------------------------------------------------------------------------
// spdtr.c --  bayesian morse code decoder 
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
#include "configuration.h"



double morse::spdtr_(long int isrt, long int ilrt, long int iselm, long int ilelm)
{
    /* System generated locals */
    float ret_val;

    /* Local variables */
    long int ind1, idel, idelsp, israte;


/* 	THIS FUNCTION RETURNS THE DATA RATE (SPEED) TRANSITION */
/* 	PROBABILITY BASED ON THE CURRENT ELEM TYPE. THE ALLOWABLE */
/* 	TRANSITION PROBS ARE STORED IN THE TABLE RTRANS. */

/* 	VARIABLES: */
/* 	ISRT	- DATA RATE IDENTITY FOR STATE TO WHICH PATH */
/* 		 	 IS BEING EXTENDED */
/* 	ILRT	- DATA RATE ON CURRENT PATH */
/* 	ISELM 	- ELEM TYPE FOR NEXT STATE */
/* 	ILELM	- ELEM TYPE ON CURRENT PATH */


/*  PAGES 103-104 IN THESIS - SYMBOL CONDITIONAL TRANSITION PROBABILITIES */
/* 	IF SAVED ELEMENT AND NEW ELEMENT ARE THE SAME */
/* 	THEN THERE CAN BE NO SPEED CHANGE: */

    if (ilelm != iselm) {
		goto L100;
    }
    ret_val = 1.f;  // initialize ret_val 
    

/* SAVED ELEMENT AND NEW ELEMENT ARE THE SAME */
/* IF DATA RATE STATE IS NOT 3 THEN SPEED TRANSITION PROBABILITY = 0.0  */
    if (isrt != 3) 
		return 0.f;	

/* 	OTHERWISE, OBTAIN SPEED TRANSITION PROB */

L100:
    idel = memdel[iselm-1][ilelm-1];
    ind1 =  mempr[iselm-1][ilelm-1];
    if (ind1 == 0) 
       return 0.f;


/*  */
    idelsp = (isrt - 3) * idel;
    ret_val = rtrans[ind1-1][isrt-1];
    israte = ilrt + idelsp;
    if (israte > progdefaults.CWupperlimit) ret_val = 0.f;	// if speed rate is > 60 WPM TRANSITION PROBABILITY = 0 
    if (israte < progdefaults.CWlowerlimit) ret_val = 0.f; // if speed rate is < 10 WPM TRANSITION PROBABILITY = 0 
    return ret_val;
} /* spdtr_ */

