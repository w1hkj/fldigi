// ----------------------------------------------------------------------------
//
//      sound.h
//
// Copyright (C) 2006-2007
//              Dave Freese, W1HKJ
//
// Copyright (C) 2007-2009
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

#ifndef _SOUND_H
#define _SOUND_H

#include <string>
#include <cstring>
#include <climits>

#if USE_SNDFILE
#  include <sndfile.h>
#endif

#include <samplerate.h>

#define SCBLOCKSIZE 512


class SndException : public std::exception
{
public:
	SndException(int err_ = 0)
		: err(err_), msg(std::string("Sound error: ") + err_to_str(err_))
	{ }
	SndException(const char* msg_)
		: err(1), msg(msg_)
	{ }
	SndException(int err_, const std::string& msg_) : err(err_), msg(msg_) { }
	virtual ~SndException() throw() { }

	const char*	what(void) const throw() { return msg.c_str(); }
	int		error(void) const { return err; }

protected:
	const char* err_to_str(int e) { return strerror(e); }

	int		err;
	std::string	msg;
};


class SoundBase {
protected:
	int		sample_frequency;
	int		txppm;
	int		rxppm;

	// for interface to the samplerate resampling library
	SRC_STATE	*tx_src_state;
	SRC_STATE	*rx_src_state;
	double		*wrt_buffer;

#if USE_SNDFILE

	SNDFILE* ofCapture;
	SNDFILE* ifPlayback;
	SNDFILE* ofGenerate;

	SRC_STATE	*writ_src_state;
	SRC_STATE	*play_src_state;

	SRC_DATA	*writ_src_data;
	SRC_DATA	*play_src_data;

	float		*src_out_buffer;
	float		*src_inp_buffer;
	float		*inp_pointer;

	SF_INFO		play_info;

	float modem_wr_sr;
	float modem_play_sr;

	bool   new_playback;

	sf_count_t  read_file(SNDFILE* file, float* buf, size_t count);
	void         write_file(SNDFILE* file, float* buf, size_t count);
	void         write_file(SNDFILE* file, double* buf, size_t count);

	bool	 format_supported(int format);
	void	 tag_file(SNDFILE *sndfile, const char *title);

#endif

	bool	capture;
	bool	playback;
	bool	generate;

public:
	SoundBase();
	virtual ~SoundBase();
	virtual int	Open(int mode, int freq = 8000) = 0;
	virtual void    Close(unsigned dir = UINT_MAX) = 0;
	virtual void    Abort(unsigned dir = UINT_MAX) = 0;
	virtual size_t	Write(double *, size_t) = 0;
	virtual size_t	Write_stereo(double *, double *, size_t) = 0;
	virtual size_t	Read(float *, size_t) = 0;
	virtual void    flush(unsigned dir = UINT_MAX) = 0;
	virtual bool	must_close(int dir = 0) = 0;
#if USE_SNDFILE
	void	get_file_params(const char* def_fname, const char** fname, int* format);
	int		Capture(bool val);
	int		Playback(bool val);
	int		Generate(bool val);
#endif
};


#if USE_OSS

class SoundOSS : public SoundBase {
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

	SRC_DATA	*rx_src_data;
	SRC_DATA	*tx_src_data;
	float		*snd_buffer;
	float		*src_buffer;

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
	SoundOSS(const char *dev = "/dev/dsp");
	~SoundOSS();
	int		Open(int mode, int freq = 8000);
	void	Close(unsigned dir = UINT_MAX);
	void	Abort(unsigned dir = UINT_MAX) { Close(dir); }
	size_t		Write(double *, size_t);
	size_t		Write_stereo(double *, double *, size_t);
	size_t		Read(float *, size_t);
	bool		must_close(int dir = 0) { return true; }
	void		flush(unsigned dir = UINT_MAX) { wait_till_finished(); }

private:
	int		Fd() { return device_fd; }
	int		Frequency() { return sample_frequency;};
	int		Version() {return version;};
	int		Capabilities() {return capability_mask;};
	int		Formats() { return format_mask;};
	int		Channels() { return channels;};
	int		Format() { return play_format;};
	bool	FormatOK() { return formatok;};
};

#endif // USE_OSS


#if USE_PORTAUDIO
#  include <pthread.h>
#  include <semaphore.h>
#  include <vector>
#  include <portaudio.h>
#  include "ringbuffer.h"

class SoundPort : public SoundBase
{
public:
        typedef std::vector<const PaDeviceInfo*>::const_iterator device_iterator;
        static void	initialize(void);
        static void	terminate(void);
        static const std::vector<const PaDeviceInfo*>& devices(void);
        static void devices_info(std::string& in, std::string& out);
	static const std::vector<double>& get_supported_rates(const std::string& name, unsigned dir);

public:
        SoundPort(const char *in_dev, const char *out_dev);
        ~SoundPort();
	int 		Open(int mode, int freq = 8000);
	void 		Close(unsigned dir = UINT_MAX);
	void 		Abort(unsigned dir = UINT_MAX);
	size_t 		Write(double *buf, size_t count);
	size_t		Write_stereo(double *bufleft, double *bufright, size_t count);
	size_t 		Read(float *buf, size_t count);
	bool		must_close(int dir = 0);
	void		flush(unsigned dir = UINT_MAX);

private:
        void		src_data_reset(unsigned dir);
        static long	src_read_cb(void* arg, float** data);
        size_t          resample_write(float* buf, size_t count);
	device_iterator name_to_device(const std::string& name, unsigned dir);
        void 		init_stream(unsigned dir);
        void 		start_stream(unsigned dir);
        void 		pause_stream(unsigned dir);
        bool		stream_active(unsigned dir);
        bool		full_duplex_device(const PaDeviceInfo* dev);
        double		find_srate(unsigned dir);
	static void	probe_supported_rates(const device_iterator& idev);
        void		pa_perror(int err, const char* str = 0);
        static void	init_hostapi_ext(void);
        static PaStreamCallback stream_process;
        static PaStreamFinishedCallback stream_stopped;

private:
        static bool                             pa_init;
	static std::vector<const PaDeviceInfo*> devs;
        double	 				req_sample_rate;
        float* 					fbuf;
	float*					src_buffer;
	SRC_DATA	*tx_src_data;

        enum {
                spa_continue = paContinue, spa_complete = paComplete,
                spa_abort = paAbort, spa_drain, spa_pause
        };
        struct stream_data {
                std::string device;
                device_iterator idev;

                PaStream* stream;
                PaStreamParameters params;

                unsigned frames_per_buffer;
                double dev_sample_rate;
                double src_ratio;

                sem_t* rwsem;
		pthread_mutex_t* cmutex;
		pthread_cond_t* ccond;
                int state;
                ringbuffer<float>* rb;
		size_t blocksize;
                size_t advance;
        } sd[2];
};

class SndPortException : public SndException
{
public:
	SndPortException(int err_ = 0)
		: SndException(err_, std::string("PortAudio error: ") + err_to_str(err_))
	{ }
	SndPortException(const char* msg_) : SndException(msg_) { }
protected:
	const char* err_to_str(int e) { return Pa_GetErrorText(e); }
};

#endif // USE_PORTAUDIO


#if USE_PULSEAUDIO
#  include <pulse/simple.h>
#  include <pulse/error.h>
extern "C" { const char* pa_get_library_version(void); };

class SoundPulse : public SoundBase
{
public:
	SoundPulse(const char* dev);
	virtual ~SoundPulse();

	int	Open(int mode, int freq = 8000);
	void    Close(unsigned dir = UINT_MAX);
	void    Abort(unsigned dir = UINT_MAX);
	size_t	Write(double* buf, size_t count);
	size_t	Write_stereo(double* bufleft, double* bufright, size_t count);
	size_t	Read(float *buf, size_t count);
	bool	must_close(int dir = 0) { return false; }
	void	flush(unsigned dir = UINT_MAX);

private:
	void	src_data_reset(int mode);
        static long	src_read_cb(void* arg, float** data);
	size_t	resample_write(float* buf, size_t count);

private:
	struct stream_data {
		pa_simple*	stream;
		pa_sample_spec	stream_params;
		pa_buffer_attr  buffer_attrs;
		pa_stream_direction_t dir;
		double		src_ratio;
		size_t		blocksize;
	} sd[2];

	SRC_DATA* tx_src_data;
	float* fbuf;
	float* snd_buffer;
	float* src_buffer;
};

class SndPulseException : public SndException
{
public:
	SndPulseException(int err_ = 0)
		: SndException(err_, std::string("PulseAudio error: ") + err_to_str(err_))
	{ }
	SndPulseException(const char* msg_) : SndException(msg_) { }
protected:
	const char* err_to_str(int e) { return pa_strerror(e); }
};

#endif // USE_PULSEAUDIO


class SoundNull : public SoundBase
{
public:
	int	Open(int mode, int freq = 8000) { sample_frequency = freq; return 0; }
	void    Close(unsigned) { }
	void    Abort(unsigned) { }
	size_t	Write(double* buf, size_t count);
	size_t	Write_stereo(double* bufleft, double* bufright, size_t count);
	size_t	Read(float *buf, size_t count);
	bool	must_close(int dir = 0) { return false; }
	void	flush(unsigned) { }
};

#endif // SOUND_H
