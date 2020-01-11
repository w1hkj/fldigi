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
#include <samplerate.h>

#include "threads.h"
#include "trx.h"
#include "ringbuffer.h"
#include "filters.h"

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

friend void process_alert();
friend void stream_process();

public:

	enum { ALERT, MONITOR };

	int		paError;
	float	*data_frames; //[ MAX_FRAMES_PER_BUFFER * NUM_CHANNELS ];
	int		ptr;
	double	sr;				// sample rate of output sound codec
	int		data_ptr;
	int		num_frames;
	int		state;

	SRC_STATE * rc;
	SRC_DATA  rcdata;
	int rc_error;

	PaStream *stream;
	PaStreamParameters paStreamParameters;

// sndfile interface
	SF_INFO playinfo;
	SNDFILE *playback;

// transfer buffer from trx loop
	double	*dbuffer;
	float   *fbuffer;
	float	*nubuffer;
	double	b_sr;			// sample rate of source monitor stream
	int		b_len;
// ringbuffer for streaming audio
	ringbuffer<float> * monitor_rb;

// bandpass filter for streaming audio
	C_FIR_filter	*bpfilt;
	double			flo;
	double			fhi;
	double			gain;

public:

	c_portaudio();
	~c_portaudio();

	int 	open();//void *);
	void	close();
	int		is_open() { return stream != 0; }

	void	play_buffer(float *buffer, int len, int _sr, int src = ALERT);

	void	play_sound(int *sndbuffer, int len, int _sr);
	void	play_sound(float *sndbuffer, int len, int _sr);
	void	silence(float secs, int _sr);

	void	mon_write(double *buffer, int len, int _sr);

	void	process_mon();
	size_t	mon_read(float *buffer, int len);

	void	do_play_file(std::string fname);

	void	play_file(std::string fname);
	void	play_mp3(std::string fname);
	void	play_wav(std::string fname);

	void	init_filter();
};

void open_alert_port(c_portaudio *cpa);
void close_alert_port(c_portaudio *cpa);

#endif
