// ----------------------------------------------------------------------------
// Copyright (C) 2014
//              David Freese, W1HKJ
//
// This file is part of fldigi
//
// fldigi is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#include <string>

#include "filters.h"

class cDTMF {
private:
//#define N        240 // 30 msec interval at 8000 sps

#define RANGE  0.5         /* any thing higher than RANGE*peak is "on" */
#define THRESH 1000        /* 6 dB s/n for detection */
#define FLUSH_TIME 10      /* 10 frames ~ 330 millisecond */

#define NUMTONES 8

private:
	double power[NUMTONES];
	//double thresh;
	double maxpower;
	double minpower;
	double data[350];

	goertzel *filt[NUMTONES];

	int framesize;

	static double coef[];
	static int k[];
	static const char *dtran[];

	static int row[];
	static int col[];
	static const char rc[];

	double outbuf[16384];
	double shape[128];
	int RT;
	int duration;
	int silence_time;
	int last;
	std::string dtmfchars;

public:
	cDTMF() {
		for (int i = 0; i < 4; i++) filt[i] = new goertzel(240, row[i], 8000);
		for (int i = 0; i < 4; i++) filt[i+4] = new goertzel(240, col[i], 8000);
		for (int i = 0; i < 240; i++) data[i] = 0;
		dtmfchars.clear();
		framesize = 240; // 8000 sr default
		silence_time = 0;
		last = ' ';
	}
	~cDTMF() {};
// receive
	void calc_power();
	int decode();
	void receive(const float* buf, size_t len);
// transmit
	void makeshape();
	void silence(int);
	void two_tones(int);
	void send();
};
