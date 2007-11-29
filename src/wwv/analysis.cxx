// ----------------------------------------------------------------------------
// anal.cxx  --  anal modem
//
// Copyright (C) 2006
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.  
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
// along with fldigi; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// ----------------------------------------------------------------------------

#include <config.h>

#include "analysis.h"
#include "waterfall.h"
#include "Config.h"
#include "configuration.h"

#define anal_BW         4

static char msg1[80];

void anal::tx_init(cSound *sc)
{
}

void anal::rx_init()
{
	phaseacc = 0;
}

void anal::init()
{
	modem::init();
	rx_init();
	digiscope->mode(Digiscope::RTTY);
}

anal::~anal()
{
	delete hilbert;
	delete bpfilt;
}

void anal::restart()
{
	double fhi;
	double flo;
	
	symbollen = analMaxSymLen;
	set_bandwidth(anal_BW);

	fhi = anal_BW * 1.1 / samplerate;
	flo = 0.0;
	if (bpfilt) 
		bpfilt->create_filter(flo, fhi);
	else
		bpfilt = new fftfilt(flo, fhi, 2048);

	ffilt->setLength(4000); // average over last 1/2 second of samples
	favg->setLength(120);    // average over last minute of samples
	
	sum = 0.0;
	fout_1 = fout_2 = 0.0;
	
	put_MODEstatus(mode);
}

anal::anal()
{
	mode = MODE_ANALYSIS;

	samplerate = anal_SampleRate;

	bpfilt = (fftfilt *)0;
	hilbert = new C_FIR_filter();
	hilbert->init_hilbert(37, 1);
	ffilt = new Cmovavg(512);
	favg = new Cmovavg(64);
	
	analysisFilename = HomeDir;
	analysisFilename.append("freqanalysis.csv");
	
	pipeptr = 0;
	restart();
}

void anal::clear_syncscope()
{
	set_scope(0, 0, false);
}

complex anal::mixer(complex in)
{
	complex z;
	z.re = cos(phaseacc);
	z.im = sin(phaseacc);
	z = z * in;

	phaseacc -= twopi * frequency / samplerate;
	if (phaseacc > M_PI) 
		phaseacc -= twopi;
	else if (phaseacc < M_PI) 
		phaseacc += twopi;

	return z;
}

void anal::writeFile()
{
	analysisFile.open(analysisFilename.c_str(), ios::app);
	analysisFile << wf->rfcarrier() << ", " << frequency << ", " << fout_2 << endl;
	analysisFile.close();
}

int anal::rx_process(const double *buf, int len)
{
	complex z, *zp;
	double fin;
	int n;
	static int dspcnt = symbollen;

	while (len-- > 0) {
// create analytic signal from sound card input samples
		z.re = z.im = *buf++;
		hilbert->run(z, z);
// mix it with the audio carrier frequency to create a baseband signal
		z = mixer(z);
// low pass filter using Windowed Sinc - Overlap-Add convolution filter
		n = bpfilt->run(z, &zp);
		for (int i = 0; i < n; i++) {
// measure phase difference between successive samples to determine
// the frequency of the baseband signal (+anal_baud or -anal_baud)
// see class complex definiton for operator %
			fin = (prevsmpl % zp[i]).arg() * samplerate / twopi;
			prevsmpl = zp[i];
// filter using moving average filter
			fout_1 = ffilt->run(fin);
// the values in the pipe are +/- 2 Hz ==> +/- 1.0			
			pipe[pipeptr] = fout_1 / 4;
			pipeptr = (pipeptr + 1) % symbollen;
			dspcnt--;
			if (dspcnt == 0) {
				set_scope(pipe, symbollen, false);
				pipe.next(); // change buffers
// filter using second moving average filter & display the result
				fout_2 = favg->run(fout_1);
				if (wf->USB())
					sprintf(msg1, "Freq: %12.2f", wf->rfcarrier() + frequency + fout_2 ); 
				else
					sprintf(msg1, "Freq: %12.2f", wf->rfcarrier() - frequency - fout_2 );
				put_status(msg1);
				writeFile();
// reset the display counter & the pipe pointer
				dspcnt = symbollen;
				pipeptr = 0;
			}
		}
	}
	return 0;
}

//=====================================================================
// anal transmit
//=====================================================================

int anal::tx_process()
{
	return -1;
}
