// Pulse Audio header

#ifndef PLAY_PA_H
#define PLAY_PA_H

#include <portaudio.h>

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <string>
#include <iostream>
#include <unistd.h>
#include <sys/time.h>
#include <string>

#include <sndfile.h>

#include "threads.h"
#include "trx.h"

class cPA_snd_exception : public std::exception
{
public:
	cPA_snd_exception(int err_ = 0)
		: err(err_), msg(std::string("Sound error: ") + err_to_str(err_))
	{ }
	cPA_snd_exception(const char* msg_)
		: err(1), msg(msg_)
	{ }
	cPA_snd_exception(int err_, const std::string& msg_) : err(err_), msg(msg_) { }
	virtual ~cPA_snd_exception() throw() { }

	const char*	what(void) const throw() { return msg.c_str(); }
	int		error(void) const { return err; }

protected:
	const char* err_to_str(int e) { return strerror(e); }

	int		err;
	std::string	msg;
};

class cPA_exception : public cPA_snd_exception
{
public:
	cPA_exception(int err_ = 0)
		: cPA_snd_exception(err_, std::string("PortAudio error: ") + err_to_str(err_))
	{ }
	cPA_exception(const char* msg_) : cPA_snd_exception(msg_) { }
protected:
	const char* err_to_str(int e) { return Pa_GetErrorText(e); }
};

class c_portaudio {
#define NUMSAMPLES 2048
#define SCRATE 8000

friend void process_alert();

private:

	int		paError;
	float	sndbuff[ 2 * NUMSAMPLES ];
	int		ptr;
	int		sr;

	PaStream *stream;
	PaStreamParameters paStreamParameters;

// sndfile interface
	SF_INFO playinfo;
	SNDFILE *playback;

public:

	c_portaudio();
	~c_portaudio();

	void	open(int samplerate);
	void	close();

	void	play_buffer(float *buffer, int len, int _sr);
	void	play_sound(int *sndbuffer, int len, int _sr);
	void	play_sound(float *sndbuffer, int len, int _sr);
	void	silence(float secs, int _sr);

	void	play_file(std::string fname);
};

#endif
