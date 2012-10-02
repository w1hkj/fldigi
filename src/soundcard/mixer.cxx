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
// Fldigi is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#if USE_OSS
#  include <sys/ioctl.h>
#  if defined(__OpenBSD__)
#    include <soundcard.h>
#  else
#    include <sys/soundcard.h>
#  endif
#endif
#include <math.h>

#include <string>
#include <cstring>

#include "mixer.h"
#include "configuration.h"
#include "debug.h"

LOG_FILE_SOURCE(debug::LOG_AUDIO);

#if USE_OSS

MixerOSS::MixerOSS() {
	strcpy (szDevice, "/dev/mixerX");
	mixer = "/dev/mixer";
	mixer_fd		= -1;
	findNumMixers();
}

MixerOSS::~MixerOSS()
{
	closeMixer();
}

//=======================================
// mixer methods
//=======================================
void MixerOSS::openMixer(const char *dev)
{
	int err;
	if (mixer_fd != -1) closeMixer();
	mixer = dev;
	try {

	int oflags = O_RDWR;
#	ifdef HAVE_O_CLOEXEC
		oflags = oflags | O_CLOEXEC;
#	endif

		mixer_fd =  open(mixer.c_str(), oflags);
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

void MixerOSS::closeMixer()
{
	if (mixer_fd == -1) return;
	restoreValues();
	close(mixer_fd);
	mixer_fd = -1;
}

void MixerOSS::initValues()
{
	if (mixer_fd == -1) return;
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

	LOG_DEBUG("Sound mixer initial state:\n" "Dev mask: %02x\n"
		  "Rec mask: %02x\n" "Rec src: %02x\n" "Current input source #: %s\n"
		  "Line level: %f\n" "Mic level: %f\n" "Pcm level: %f\n" "Vol level: %f\n",
		  devmask, recmask, recsrc, GetInputSourceName(inpsrc0),
		  linelevel0, miclevel0, pcmlevel0, vollevel0);
}

void MixerOSS::restoreValues()
{
	if (mixer_fd == -1) return;
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

void MixerOSS::findNumMixers()
{
	int fd;
	NumMixers = 0;
	for (int i = 0; i< 11; i++) {
		if (i == 0)
			szDevice[10] = 0;
		else
			szDevice[10] = '0'+(i-1);
		int oflags = O_RDWR;
#		ifdef HAVE_O_CLOEXEC
			oflags = oflags | O_CLOEXEC;
#		endif

		fd = open(szDevice, oflags);
		if (fd >= 0) {
			Devices[NumMixers] = i;
			NumMixers++;
			close(fd);
		}
   }
}

const char * MixerOSS::MixerName( int index )
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

void MixerOSS::setXmtLevel(double v)
{
	if (mixer_fd == -1) return;
	OutVolume(v);
}

void MixerOSS::setRcvGain(double v)
{
	if (mixer_fd == -1) return;
	InputVolume(v);
}

int MixerOSS::initMask()
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
double MixerOSS::ChannelVolume(int channel)
{
	if (mixer_fd == -1) return 0.0;
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

double MixerOSS::OutVolume()
{
	if (mixer_fd == -1) return 0.0;

	return ChannelVolume(SOUND_MIXER_VOLUME);
}

void MixerOSS::OutVolume(double volume)
{
	if (mixer_fd == -1) return;
	int vol = (int)((volume * 100.0) + 0.5);
	vol = (vol | (vol<<8));
	ioctl(mixer_fd, MIXER_WRITE(SOUND_MIXER_VOLUME), &vol);
}

double MixerOSS::PCMVolume()
{
	if (mixer_fd == -1) return 0.0;
	return ChannelVolume(SOUND_MIXER_PCM);
}

void MixerOSS::PCMVolume(double volume )
{
	if (mixer_fd == -1) return;
	
	int vol = (int)((volume * 100.0) + 0.5);
	vol = (vol | (vol<<8));
	ioctl(mixer_fd, MIXER_WRITE(SOUND_MIXER_PCM), &vol);
}

int MixerOSS::NumOutputVolumes()
{
	if (mixer_fd == -1) return 0;
	return num_out;
}

const char *MixerOSS::OutputVolumeName( int i )
{
	if (mixer_fd == -1) return NULL;
	const char *labels[] = SOUND_DEVICE_LABELS;
	return labels[outs[i]];
}

double MixerOSS::OutputVolume( int i )
{
	if (mixer_fd == -1) return 0.0;
	return ChannelVolume(outs[i]);
}

void MixerOSS::OutputVolume( int i, double volume )
{
	if (mixer_fd == -1) return;
	int vol = (int)((volume * 100.0) + 0.5);
	vol = (vol | (vol<<8));
	ioctl(mixer_fd, MIXER_WRITE(outs[i]), &vol);
}

int MixerOSS::GetNumInputSources()
{
	if (mixer_fd == -1) return 0;
	return num_rec;
}

const char *MixerOSS::GetInputSourceName( int i)
{
	if (mixer_fd == -1) return NULL;
	const char *labels[] = SOUND_DEVICE_LABELS;
	return labels[recs[i]];
}

int MixerOSS::InputSourceNbr(const char *source)
{
	if (mixer_fd == -1) return -1;
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

int MixerOSS::GetCurrentInputSource()
{
	if (mixer_fd == -1) return -1;
	for(int i = 0; i < num_rec; i++)
		if (recsrc & (1 << (recs[i])))
			return i;
	return -1; /* none */
}

void MixerOSS::SetCurrentInputSource( int i )
{
	if (mixer_fd == -1) return;
	int newrecsrcmask = (1 << (recs[i]));
	ioctl(mixer_fd, MIXER_WRITE(SOUND_MIXER_READ_RECSRC), &newrecsrcmask);
}

/*
 Input volume
*/

double MixerOSS::InputVolume()
{
	if (mixer_fd == -1) return 0.0;
//	int i = GetCurrentInputSource();
//	if (i < 0)
//		return 0.0;
	return ChannelVolume(SOUND_MIXER_IGAIN);
}

void MixerOSS::InputVolume( double volume )
{
	if (mixer_fd == -1) return;
	int vol;
	vol = (int)((volume * 100.0) + 0.5);
	vol = (vol | (vol<<8));
	ioctl(mixer_fd, MIXER_WRITE(SOUND_MIXER_IGAIN), &vol);
}

/*
double MixerOSS::GetPlaythrough()
{
	int i = GetCurrentInputSource();
	if (i < 0)
		return 0.0;
	return ChannelVolume(recs[i]);
}

void MixerOSS::SetPlaythrough( double volume )
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

void MixerOSS::SetMuteInput(bool b)
{
	return;
	if (b == 1)
		SetPlaythrough(0.0);
	else
		SetPlaythrough(playthrough0);
}

*/

#endif // USE_OSS
