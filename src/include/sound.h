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

#include <iostream>
#include <string>

#include "samplerate/samplerate.h"

#define MAXSC 32767.0;
#define maxsc 32000.0
//#define maxsc 16384.0
#define SCBLOCKSIZE 512

#define	SND_BUF_LEN		65536
//#define	SRC_BUF_LEN		(8*SND_BUF_LEN)

class SndException {
public:
	char	szError[80];
	int		error;
	SndException() { *szError = 0; error = 0; }
	SndException(int e) {
		sprintf(szError,"Error: %d, %s", e, strerror(e));
		error = e;
	}
	SndException(char *s) {
		sprintf(szError,"Error: %s", s);
		error = 1;
	}
};

class cSound {
	
private:
	std::string	device;
	int		device_fd;
	int		version;
	int		capability_mask;
	int		format_mask;
	int		channels;
	int		play_format;
	int		sample_frequency;
	int		txppm;
	int		rxppm;
	int		mode;
	bool	formatok;

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
// for interface to the samplerate resampling library
	SRC_STATE *tx_src_state;
	SRC_DATA *tx_src_data;
	SRC_STATE *rx_src_state;
	SRC_DATA *rx_src_data;
	float	*snd_buffer;
	float	*src_buffer;
	unsigned char *cbuff;
public:
	cSound(const char *dev = "/dev/dsp");
	~cSound();
	int		Open(int mode, int freq = 8000);
	void	Close();
	int		Write(unsigned char *, int);
	int		write_samples(double *, int);
	int		write_stereo(double *, double *, int);
	int		Read(unsigned char *, int);
	int		Read(double *, int);
	int		Fd() { return device_fd; }
	int		Frequency() { return sample_frequency;};
	int		Version() {return version;};
	int		Capabilities() {return capability_mask;};
	int		Formats() { return format_mask;};
	int		Channels() { return channels;};
	int		Format() { return play_format;};
	bool	FormatOK() { return formatok;};
};

#endif
