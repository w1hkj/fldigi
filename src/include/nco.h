// ---------------------------------------------------------------------
// nco.h  --  a generic NCO class
//
//
// This file is a proposed part of fldigi.
//
// Copyright (C) 2010
//    Dave Freese, W1HKJ
//    Chris Sylvain, KB3CS
//
// fldigi is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi; if not, write to the
//
//  Free Software Foundation, Inc.
//  51 Franklin Street, Fifth Floor
//  Boston, MA  02110-1301 USA.
//
// ---------------------------------------------------------------------

#ifndef NCO_H
#define NCO_H

#include "complex.h"

class NCO {
#define NCO_SINETABLE_LEN (1<<8) // == 256
 private:
    double SampleRate;
    double Frequency;
    double Phase;
    double Phase_incr;

    bool sinetable_ready;
    double sinetable[NCO_SINETABLE_LEN+1];

 public:
    NCO() { sinetable_ready = false; }
    ~NCO() { };

    void init(double freq, double phase, double sr) {
	SampleRate = sr;
	Frequency = freq;
	Phase = phase / TWOPI; // normalize radians to [0,1]
	Phase_incr = freq / sr;

	if (sinetable_ready == false) {
	    double step = 1.0 / NCO_SINETABLE_LEN;

	    for (int i = 0; i < NCO_SINETABLE_LEN; i++)
		sinetable[i] = sin(step * i * TWOPI);

	    sinetable[NCO_SINETABLE_LEN] = sinetable[0]; /* == 0.0 */

	    sinetable_ready = true;
	}
    }

    inline void setfreq(double freq) {
	Frequency = freq;
	Phase_incr = freq / SampleRate;
    }

    inline double getfreq() { return Frequency; }

    inline void setphase(double phase) { Phase = phase / TWOPI; }

    inline double getphase() { return Phase * TWOPI; }

    inline void setphaseacc(double phase) { Phase = phase; }

    inline double getphaseacc() { return Phase; }

#ifndef NDEBUG
    double interpolate(double phase) {
#else
    inline double interpolate(double phase) {
#endif
	int iphase;
	double frac1, frac2;

	iphase = (int) floor(phase * NCO_SINETABLE_LEN);
	frac1 = (phase * NCO_SINETABLE_LEN) - iphase;
	frac2 = 1.0 - frac1;

	iphase %= NCO_SINETABLE_LEN;

	return (sinetable[iphase] * frac2) + (sinetable[iphase+1] * frac1);
    }

    double sample() {
	double sample = interpolate(Phase);

	Phase += Phase_incr;

	if (Phase > 1) {
	    Phase -= 1;
	    // Phase might be .le. abs(machine-epsilon)
	    if (Phase < 0.0)  Phase = 0.0;
	}

	return sample;
    }

    cmplx cmplx_sample() {
	cmplx sample;

	// M_PI_2 / TWOPI = M_PI / 2 / 2 / M_PI = 1 / 4
	double t = Phase + 0.25;
	if (t > 1.0) {
	    t -= 1;
	    // Phase might be .le. abs(machine-epsilon)
	    if (t < 0.0)  t = 0.0;
	}

	sample = cmplx(interpolate(t), interpolate(Phase));

	Phase += Phase_incr;

	if (Phase > 1.0) {
	    Phase -= 1;
	    if (Phase < 0.0)  Phase = 0.0;
	}

	return sample;
    }
};

#endif
