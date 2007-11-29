// ----------------------------------------------------------------------------
//
//      sound.h
//
// Copyright (C) 2006-2007
//              Dave Freese, W1HKJ
//
// Copyright (C) 2007
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

#ifndef _SOUND_H
#define _SOUND_H

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/soundcard.h>
#include <math.h>

#include <string>
#if USE_SNDFILE
	#include <sndfile.hh>
#endif
#include <iostream>

#if USE_PORTAUDIO
	#include <portaudiocpp/PortAudioCpp.hxx>
#endif

#include <samplerate.h>

#define MAXSC 32767.0;
#define maxsc 32000.0
//#define maxsc 16384.0
#define SCBLOCKSIZE 512

#define	SND_BUF_LEN		65536
//#define	SRC_BUF_LEN		(8*SND_BUF_LEN)

#define powerof2(n) ((((n) - 1) & (n)) == 0)

class SndException : public std::exception
{
public:
	SndException() { *szError = 0; error = 0; }
	SndException(int e) {
		snprintf(szError, sizeof(szError) - 1, "Error: %d, %s", e, strerror(e));
		error = e;
	}
	SndException(const char *s) {
		snprintf(szError, sizeof(szError) - 1, "Error: %s", s);
		error = 1;
	}
        const char *what(void) const throw() { return szError; }

private:
	char	szError[80];
	int		error;
};

class cSound {
protected:
	int		sample_frequency;
	int		txppm;
	int		rxppm;

// for interface to the samplerate resampling library
	SRC_STATE	*tx_src_state;
	SRC_DATA	*tx_src_data;
	SRC_STATE	*rx_src_state;
	SRC_DATA	*rx_src_data;
	float		*snd_buffer;
	float		*src_buffer;

#if USE_SNDFILE
	SndfileHandle* ofCapture;
	SndfileHandle* ifPlayback;
	SndfileHandle* ofGenerate;
#endif

	bool	capture;
	bool	playback;
	bool	generate;

	void writeGenerate(double *buff, int count);
	void writeCapture(double *buff, int count);
	int  readPlayback(double *buff, int count);
#if USE_SNDFILE
	bool format_supported(int format);
	void tag_file(SndfileHandle *fh, const char *title);
#endif
public:
	cSound();
	virtual ~cSound();
	virtual int	Open(int mode, int freq = 8000) = 0;
	virtual void    Close() = 0;
	virtual int	write_samples(double *, int) = 0;
	virtual int	write_stereo(double *, double *, int) = 0;
	virtual int	Read(double *, int) = 0;
	virtual bool	full_duplex(void) { return false; }
#if USE_SNDFILE
	int		Capture(bool on);
	int		Playback(bool on);
	int		Generate(bool on);
#endif
};

class cSoundOSS : public cSound {
private:
	std::string	device;
	int		device_fd;
	int		version;
	int		capability_mask;
	int		format_mask;
	int		channels;
	int		play_format;
	int		mode;
	bool	formatok;
	unsigned char	*cbuff;

	void	getVersion();
	void	getCapabilities();
	void	getFormats();
	void	setfragsize();
	void	Channels(int);
	void	Frequency(int);
	void	Format(int);
	int		BufferSize(int);
	bool	wait_till_finished();
	bool	reset_device();

public:
	cSoundOSS(const char *dev = "/dev/dsp");
	~cSoundOSS();
	int		Open(int mode, int freq = 8000);
	void	Close();
	int		write_samples(double *, int);
	int		write_stereo(double *, double *, int);
	int		Read(double *, int);

private:
	int		Read(unsigned char *, int);
	int		Write(unsigned char *, int);
	int		Fd() { return device_fd; }
	int		Frequency() { return sample_frequency;};
	int		Version() {return version;};
	int		Capabilities() {return capability_mask;};
	int		Formats() { return format_mask;};
	int		Channels() { return channels;};
	int		Format() { return play_format;};
	bool	FormatOK() { return formatok;};
};

#if USE_PORTAUDIO

class cSoundPA : public cSound
{
public:
        cSoundPA(const char *dev);
        ~cSoundPA();
	int 		Open(int mode, int freq = 8000);
	void 		Close();
	int 		write_samples(double *buf, int count);
	int		write_stereo(double *bufleft, double *bufright, int count);
	int 		Read(double *buf, int count);
	bool		full_duplex(void);

private:
        void		src_data_reset(int mode);
        void		resample(int mode, float *buf, int count, int max = 0);
        void 		init_stream(void);
        void		adjust_stream(void);
        double		get_best_srate(void);
        static unsigned ceil2(unsigned n);
        static unsigned floor2(unsigned n);

private:
        std::string	device;

        portaudio::System 			     &sys;
        portaudio::BlockingStream 		     stream;

        portaudio::System::DeviceIterator	     idev;
        portaudio::DirectionSpecificStreamParameters in_params;
        portaudio::DirectionSpecificStreamParameters out_params;
        portaudio::StreamParameters 		     stream_params;

        unsigned	frames_per_buffer;
        unsigned	max_frames_per_buffer;
        double	 	req_sample_rate;
        double		dev_sample_rate;
        float 		*fbuf;
        static double	std_sample_rates[];
};

#endif // USE_PORTAUDIO

#endif
