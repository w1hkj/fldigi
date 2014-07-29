// ----------------------------------------------------------------------------
// initl.c --  bayesian morse code decoder
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
#include "configuration.h"



/* Initialized data */


/* Table of constant values */


int morse::initl_(void)
{
	isave = PATHS;
	n = 0;
	ncall = 0;
	ndelay = NDELAY;
	nmax = 0;
	mmax = 0;
	kd = 0;
	ndelst = 0;
	iend = 0;
	ixlast = 0;
	curstate = 0;
	newstate = 0;
	initial = 0;

//	float delta = (progdefaults.CWupperlimit - progdefaults.CWlowerlimit)/ (float)PATHS;

	for(int i=0;i<PATHS;i++) {
		ilrate[i]= progdefaults.CWspeed; //((i/5+1)*10);//progdefaults.CWlowerlimit + i*delta/2;
//		ilrate[i]=progdefaults.CWspeed;
		printf("\nilrate[%d]:%d",i,(int)ilrate[i]);
	}
	for(int i=0;i<PATHS;i++) {
		lambda[i] = 5;
		dur[i]=1000.f;
		pathsv[i]=5;
		ykkip[i] = .5f;
		pkkip[i] = .1f;
	}
	for(int i=0;i<30*PATHS;i++) {
		p[i]=1.f;
		lamsav[i]=5;
		dursav[i]=0.f;
		ilrsav[i]=20;
	}

	return 0;
} /* initl_ */

