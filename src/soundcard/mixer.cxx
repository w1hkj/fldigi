// ----------------------------------------------------------------------------
//
//      mixer.cxx
//
// Copyright (C) 2006-2007
//              Dave Freese, W1HKJ
//
// Copyright (C) 2007-2008
//              Stelios Bounanos, M0GLD
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#if USE_OSS
#    include <sys/soundcard.h>
#endif
#include <math.h>

#include <iostream>
#include <string>
#include <cstring>

#include "mixer.h"
#include "configuration.h"


#if USE_OSS

cMixerOSS::cMixerOSS() {
	strcpy (szDevice, "/dev/mixerX");
	mixer = "/dev/mixer";
	mixer_fd		= -1;
	findNumMixers();
}

cMixerOSS::~cMixerOSS()
{
	closeMixer();
}

//=======================================
// mixer methods
//=======================================
void cMixerOSS::openMixer(const char *dev)
{
	int err;
	if (mixer_fd != -1) closeMixer();
	mixer = dev;
	try {
		mixer_fd =  open(mixer.c_str(), O_RDWR);
		if (mixer_fd == -1)
			throw MixerException(errno);
		if ((err = initMask()) != 0)
			throw MixerException(err);
	} 
	catch (...) {
		throw;
	}
	initValues();
}

void cMixerOSS::closeMixer()
{
	if (mixer_fd == -1) return;
	restoreValues();
	close(mixer_fd);
	mixer_fd = -1;
}

void cMixerOSS::initValues()
{
	int devnbr;

	inpsrc0 = GetCurrentInputSource();

	devnbr = InputSourceNbr("Line");
	SetCurrentInputSource(devnbr);
	linelevel0 = InputVolume();

	devnbr = InputSourceNbr("Mic");
	SetCurrentInputSource(devnbr);	
	miclevel0 = InputVolume();
	
	pcmlevel0 = PCMVolume();
	vollevel0 = OutVolume();
/*
	std::cout << "Sound card initial state:" << std::endl;
	std::cout << "  Dev mask " << hex << devmask << std::endl;
	std::cout << "  Rec mask " << hex << recmask << std::endl;
	std::cout << "  Rec src  " << hex << recsrc << std::endl;
	std::cout << "  Current input source # " << GetInputSourceName(inpsrc0) << std::endl;
	std::cout << "  Line Level = " << linelevel0 << std::endl;
	std::cout << "  Mic  Level = " << miclevel0  << std::endl;
	std::cout << "  Pcm  Level = " << pcmlevel0  << std::endl;
	std::cout << "  Vol  Level = " << vollevel0  << std::endl;
*/
}

void cMixerOSS::restoreValues()
{
	int devnbr;
	devnbr = InputSourceNbr("Line");
	SetCurrentInputSource(devnbr);
	InputVolume(linelevel0);
	
	devnbr = InputSourceNbr("Mic");
	SetCurrentInputSource(devnbr);
	InputVolume(miclevel0);
	
	PCMVolume(pcmlevel0);
	OutVolume(vollevel0);

//	SetCurrentInputSource(inpsrc0);
	ioctl(mixer_fd, MIXER_WRITE(SOUND_MIXER_READ_RECSRC), &recsrc0);
}

void cMixerOSS::findNumMixers()
{
	int fd;
	NumMixers = 0;
	for (int i = 0; i< 11; i++) {
		if (i == 0)
			szDevice[10] = 0;
		else
			szDevice[10] = '0'+(i-1);
		fd = open(szDevice, O_RDWR);
		if (fd >= 0) {
			Devices[NumMixers] = i;
			NumMixers++;
			close(fd);
		}
   }
}

const char * cMixerOSS::MixerName( int index )
{
	if (NumMixers <= 0)
		findNumMixers();

	if (index < 0 || index >= NumMixers)
		return NULL;

	if (Devices[index] == 0)
		szDevice[10] = 0;
	else
		szDevice[10] = '0' + (Devices[index]-1);
	return szDevice;
}

void cMixerOSS::setXmtLevel(double v)
{
	if (mixer_fd == -1) return;
	OutVolume(v);
}

void cMixerOSS::setRcvGain(double v)
{
	if (mixer_fd == -1) return;
	InputVolume(v);
}

int cMixerOSS::initMask()
{
	if (mixer_fd == -1) return -1;
	
	if((ioctl(mixer_fd, MIXER_READ(SOUND_MIXER_READ_DEVMASK), &devmask)) == -1) 
		return errno;

	if((ioctl(mixer_fd, MIXER_READ(SOUND_MIXER_READ_RECMASK), &recmask)) == -1) 
		return errno;

	if ((ioctl(mixer_fd, MIXER_READ(SOUND_MIXER_READ_RECSRC), &recsrc)) == -1)
		return errno;
		
	devmask0 = devmask;
	recmask0 = recmask;
	recsrc0 = recsrc;
		
	outmask = devmask ^ recmask;

	num_out = 0;
	num_rec = 0;
	for( int i = 0; i < SOUND_MIXER_NRDEVICES; i++) {
		if (recmask & (1 << i)) {
			recs[num_rec++] = i;
		}
		else if (devmask & (1<<i)) {
			outs[num_out++] = i;
		}
	}
	return 0;
}

// returns value between 0.0 and 1.0
double cMixerOSS::ChannelVolume(int channel)
{
	int vol;
	int stereo;

	if (ioctl(mixer_fd, SOUND_MIXER_READ_STEREODEVS, &stereo) == 0)
		stereo = ((stereo & (1 << channel)) != 0);
	else
		stereo = 0;
	if (ioctl(mixer_fd, MIXER_READ(channel), &vol) == -1)
		return 0.0;

	if (stereo)
		return ((vol & 0xFF)/200.0) + (((vol>>8) & 0xFF)/200.0);
	else
		return (vol & 0xFF)/100.0;
}

/*
 Master (output) volume
*/

double cMixerOSS::OutVolume()
{
	if (mixer_fd == -1) return 0.0;

	return ChannelVolume(SOUND_MIXER_VOLUME);
}

void cMixerOSS::OutVolume(double volume)
{
	if (mixer_fd == -1) return;
	int vol = (int)((volume * 100.0) + 0.5);
	vol = (vol | (vol<<8));
	ioctl(mixer_fd, MIXER_WRITE(SOUND_MIXER_VOLUME), &vol);
}

double cMixerOSS::PCMVolume()
{
	if (mixer_fd == -1) return 0.0;
	return ChannelVolume(SOUND_MIXER_PCM);
}

void cMixerOSS::PCMVolume(double volume )
{
	if (mixer_fd == -1) return;
	
	int vol = (int)((volume * 100.0) + 0.5);
	vol = (vol | (vol<<8));
	ioctl(mixer_fd, MIXER_WRITE(SOUND_MIXER_PCM), &vol);
}

int cMixerOSS::NumOutputVolumes()
{
	return num_out;
}

const char *cMixerOSS::OutputVolumeName( int i )
{
	const char *labels[] = SOUND_DEVICE_LABELS;
	return labels[outs[i]];
}

double cMixerOSS::OutputVolume( int i )
{
	return ChannelVolume(outs[i]);
}

void cMixerOSS::OutputVolume( int i, double volume )
{
	int vol = (int)((volume * 100.0) + 0.5);
	vol = (vol | (vol<<8));
	ioctl(mixer_fd, MIXER_WRITE(outs[i]), &vol);
}

int cMixerOSS::GetNumInputSources()
{
	return num_rec;
}

const char *cMixerOSS::GetInputSourceName( int i)
{
	const char *labels[] = SOUND_DEVICE_LABELS;
	return labels[recs[i]];
}

int cMixerOSS::InputSourceNbr(const char *source)
{
	const char *labels[] = SOUND_DEVICE_LABELS;
	char lbl[80];
	int len;
	for (int i = 0; i < num_rec; i++) {
		strcpy(lbl, labels[recs[i]]);
		len = strlen(lbl);
		while (len > 0 && lbl[len-1] == ' ') {
			lbl[len-1] = 0;
			len--;
		}
		if (!strncasecmp(lbl, source, strlen(lbl) ) )
			return i;
	}
  return -1;
}

int cMixerOSS::GetCurrentInputSource()
{
	if (mixer_fd == -1) return -1;
	for(int i = 0; i < num_rec; i++)
		if (recsrc & (1 << (recs[i])))
			return i;
	return -1; /* none */
}

void cMixerOSS::SetCurrentInputSource( int i )
{
	if (mixer_fd == -1) return;
	int newrecsrcmask = (1 << (recs[i]));
	ioctl(mixer_fd, MIXER_WRITE(SOUND_MIXER_READ_RECSRC), &newrecsrcmask);
}

/*
 Input volume
*/

double cMixerOSS::InputVolume()
{
	if (mixer_fd == -1) return 0.0;
//	int i = GetCurrentInputSource();
//	if (i < 0)
//		return 0.0;
	return ChannelVolume(SOUND_MIXER_IGAIN);
}

void cMixerOSS::InputVolume( double volume )
{
	int vol;
	vol = (int)((volume * 100.0) + 0.5);
	vol = (vol | (vol<<8));
	ioctl(mixer_fd, MIXER_WRITE(SOUND_MIXER_IGAIN), &vol);
}

/*
double cMixerOSS::GetPlaythrough()
{
	int i = GetCurrentInputSource();
	if (i < 0)
		return 0.0;
	return ChannelVolume(recs[i]);
}

void cMixerOSS::SetPlaythrough( double volume )
{
	if (mixer_fd == -1) return;
	
	int vol;
	int i = GetCurrentInputSource();
	if (i < 0)
		return;

	vol = (int)((volume * 100.0) + 0.5);
	vol = (vol | (vol<<8));
	ioctl(mixer_fd, MIXER_WRITE(recs[i]), &vol);
}

void cMixerOSS::SetMuteInput(bool b)
{
	return;
	if (b == 1)
		SetPlaythrough(0.0);
	else
		SetPlaythrough(playthrough0);
}

*/

#endif // USE_OSS
