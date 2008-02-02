// ----------------------------------------------------------------------------
//
//      sound.cxx
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

#include <iostream>

#include <cstdio>
#include <cstdlib>
#include <errno.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <sys/soundcard.h>
#include <math.h>

#include "sound.h"
#include "configuration.h"
#include <FL/Fl.H>
#include "File_Selector.h"


cSound::cSound()
        : sample_frequency(0), txppm(progdefaults.TX_corr), rxppm(progdefaults.RX_corr),
          tx_src_state(0), tx_src_data(0), rx_src_state(0), rx_src_data(0),
          snd_buffer(0), src_buffer(0),
#if USE_SNDFILE
          ofCapture(0), ifPlayback(0), ofGenerate(0),
#endif
	  capture(false), playback(false), generate(false)
{ }

cSound::~cSound()
{
	if (snd_buffer) delete [] snd_buffer;
	if (src_buffer) delete [] src_buffer;
	if (tx_src_data) delete tx_src_data;
	if (rx_src_data) delete rx_src_data;
	if (rx_src_state) src_delete (rx_src_state);
	if (tx_src_state) src_delete (tx_src_state);
#if USE_SNDFILE
	if (ofGenerate) sf_close(ofGenerate);
	if (ofCapture) sf_close(ofCapture);
	if (ifPlayback) sf_close(ifPlayback);
#endif
}

#if USE_SNDFILE
void cSound::get_file_params(const char* def_fname, char** fname, int* format)
{
	const char* suffixes;
	if (format_supported(SF_FORMAT_FLAC | SF_FORMAT_PCM_16))
		suffixes = "*.{wav,flac,au}";
	else
		suffixes = "*.{wav,au}";

	if ((*fname = File_Select("Audio file", suffixes, def_fname, 0)) == 0)
		return;

	char* suffix = strrchr(*fname, '.');
	if (suffix && !strcasecmp(suffix, ".flac"))
		*format = SF_FORMAT_FLAC | SF_FORMAT_PCM_16;
	else if (suffix && !strcasecmp(suffix, ".au"))
		*format = SF_FORMAT_AU | SF_FORMAT_FLOAT | SF_ENDIAN_CPU;
	else
		*format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
}

int cSound::Capture(bool val)
{
	if (!val) {
		if (ofCapture) {
			int err;
			if ((err = sf_close(ofCapture)) != 0)
				cerr << "sf_close error: " << sf_error_number(err) << '\n';
			ofCapture = 0;
		}
		capture = false;
		return 1;
	}

	char* fname;
	int format;
	get_file_params("./capture.wav", &fname, &format);
	if (!fname)
		return 0;

	// frames (ignored), freq, channels, format, sections (ignored), seekable (ignored)
	SF_INFO info = { 0, sample_frequency, 1, format, 0, 0 };
	if ((ofCapture = sf_open(fname, SFM_WRITE, &info)) == NULL) {
		cerr << "Could not write " << fname << '\n';
		return 0;
	}
	if (sf_command(ofCapture, SFC_SET_UPDATE_HEADER_AUTO, NULL, SF_TRUE) != SF_TRUE)
		cerr << "ofCapture update header command failed: " << sf_strerror(ofCapture) << '\n';
	tag_file(ofCapture, "Captured audio");

	capture = true;
	return 1;
}

int cSound::Playback(bool val)
{
	if (!val) {
		if (ifPlayback) {
			int err;
			if ((err = sf_close(ifPlayback)) != 0)
				cerr << "sf_close error: " << sf_error_number(err) << '\n';
			ifPlayback = 0;
		}
		playback = false;
		return 1;
	}
	char* fname;
	int format;
	get_file_params("./playback.wav", &fname, &format);
	if (!fname)
		return 0;

	SF_INFO info = { 0, 0, 0, 0, 0, 0 };
	if ((ifPlayback = sf_open(fname, SFM_READ, &info)) == NULL) {
		cerr << "Could not read " << fname << '\n';
		return 0;
	}

	playback = true;
	return 1;
}

int cSound::Generate(bool val)
{
	if (!val) {
		if (ofGenerate) {
			int err;
			if ((err = sf_close(ofGenerate)) != 0)
				cerr << "sf_close error: " << sf_error_number(err) << '\n';
			ofGenerate = 0;
		}
		generate = false;
		return 1;
	}

	char* fname;
	int format;
	get_file_params("./generate.wav", &fname, &format);
	if (!fname)
		return 0;

	// frames (ignored), freq, channels, format, sections (ignored), seekable (ignored)
	SF_INFO info = { 0, sample_frequency, 1, format, 0, 0 };
	if ((ofGenerate = sf_open(fname, SFM_WRITE, &info)) == NULL) {
		cerr << "Could not write " << fname << '\n';
		return 0;
	}
	if (sf_command(ofGenerate, SFC_SET_UPDATE_HEADER_AUTO, NULL, SF_TRUE) != SF_TRUE)
		cerr << "ofGenerate update header command failed: " << sf_strerror(ofGenerate) << '\n';
	tag_file(ofGenerate, "Generated audio");

	generate = true;
	return 1;
}
#endif // USE_SNDFILE

void cSound::writeGenerate(double *buff, int count)
{
#if USE_SNDFILE
	sf_writef_double(ofGenerate, buff, count);
#endif
}

void cSound::writeCapture(double *buff, int count)
{
#if USE_SNDFILE
	sf_writef_double(ofCapture, buff, count);
#endif
}

int cSound::readPlayback(double *buff, int count)
{
#if USE_SNDFILE
	sf_count_t r = sf_readf_double(ifPlayback, buff, count);

	while (r < count) {
		sf_seek(ifPlayback, 0, SEEK_SET);
		r += sf_readf_double(ifPlayback, buff + r, count - r);
                if (r == 0)
                        break;
        }

	return r;
#else
	return 0;
#endif
}

#if USE_SNDFILE
bool cSound::format_supported(int format)
{

        SF_INFO fmt_test = { 0, sample_frequency, 2, format, 0, 0 };
        return sf_format_check(&fmt_test);
}

void cSound::tag_file(SNDFILE *sndfile, const char *title)
{
	int err;
	if ((err = sf_set_string(sndfile, SF_STR_TITLE, title)) != 0) {
		cerr << "sf_set_string STR_TITLE: " << sf_error_number(err) << '\n';
		return;
	}

	sf_set_string(sndfile, SF_STR_COPYRIGHT, progdefaults.myName.c_str());
	sf_set_string(sndfile, SF_STR_SOFTWARE, PACKAGE_NAME "-" PACKAGE_VERSION);
	sf_set_string(sndfile, SF_STR_ARTIST, progdefaults.myCall.c_str());

	char s[64];
	snprintf(s, sizeof(s), "%s freq=%s",
		 active_modem->get_mode_name(), inpFreq->value());
	sf_set_string(sndfile, SF_STR_COMMENT, s);

	time_t t = time(0);
	struct tm zt;
	(void)gmtime_r(&t, &zt);
	if (strftime(s, sizeof(s), "%F %Tz", &zt) > 0)
		sf_set_string(sndfile, SF_STR_DATE, s);
}
#endif // USE_SNDFILE


cSoundOSS::cSoundOSS(const char *dev ) {
	device			= dev;
	cbuff			= 0;
	try {
		Open(O_RDONLY);
		getVersion();
		getCapabilities();
		getFormats();
		Close();
	}
	catch (SndException e) {
		std::cout << e.what()
			 << " <" << device.c_str()
			 << ">" << std::endl;
	}

	int err;
	try {
		snd_buffer	= new float [2*SND_BUF_LEN];
		src_buffer	= new float [2*SND_BUF_LEN];
		cbuff		= new unsigned char [4 * SND_BUF_LEN];
	}
	catch (const std::bad_alloc& e) {
		cerr << "Cannot allocate libsamplerate buffers\n";
		throw;
	}
	for (int i = 0; i < 2*SND_BUF_LEN; i++)
		snd_buffer[i] = src_buffer[i] = 0.0;
	for (int i = 0; i < 4 * SND_BUF_LEN; i++)
		cbuff[i] = 0;

	try {
		tx_src_data = new SRC_DATA;
		rx_src_data = new SRC_DATA;
	}
	catch (const std::bad_alloc& e) {
		cerr << "Cannot create libsamplerate data structures\n";
		throw;
	}

	rx_src_state = src_new(SRC_SINC_FASTEST, 2, &err);
	if (rx_src_state == 0)
		throw SndException(src_strerror(err));

	tx_src_state = src_new(SRC_SINC_FASTEST, 2, &err);
	if (tx_src_state == 0)
		throw SndException(src_strerror(err));

	rx_src_data->src_ratio = 1.0/(1.0 + rxppm/1e6);
	src_set_ratio ( rx_src_state, 1.0/(1.0 + rxppm/1e6));

	tx_src_data->src_ratio = 1.0 + txppm/1e6;
	src_set_ratio ( tx_src_state, 1.0 + txppm/1e6);
}

cSoundOSS::~cSoundOSS()
{
	Close();
	if (cbuff) delete [] cbuff;
}

void cSoundOSS::setfragsize()
{
	int sndparam;
// Try to get ~100ms worth of samples per fragment
	sndparam = (int)log2(sample_frequency * 0.1);
// double since we are using 16 bit samples
	sndparam += 1;
// Unlimited amount of buffers for RX, four for TX
	if (mode == O_RDONLY)
		sndparam |= 0x7FFF0000;
	else
		sndparam |= 0x00040000;

	if (ioctl(device_fd, SNDCTL_DSP_SETFRAGMENT, &sndparam) < 0)
		throw SndException(errno);
}

int cSoundOSS::Open(int md, int freq)
{
	mode = md;
	try {
		device_fd = open(device.c_str(), mode, 0);
		if (device_fd == -1)
			throw SndException(errno);
		Format(AFMT_S16_LE);	// default: 16 bit little endian
//		Channels(1);			//          1 channel
		Channels(2);			//          2 channels
		Frequency(freq);
		setfragsize();
	}
	catch (...) {
		throw;
	}
	return device_fd;
}

void cSoundOSS::Close()
{
	if (device_fd == -1)
		return;
	close(device_fd);
	device_fd = -1;
}

void cSoundOSS::getVersion()
{
	version = 0;
#ifndef __FreeBSD__
 	if (ioctl(device_fd, OSS_GETVERSION, &version) == -1) {
 		version = -1;
 		throw SndException("OSS Version");
 	}
#endif
}

void cSoundOSS::getCapabilities()
{
	capability_mask = 0;
	if (ioctl(device_fd, SNDCTL_DSP_GETCAPS, &capability_mask) == -1) {
		capability_mask = 0;
		throw SndException("OSS capabilities");
	}
}

void cSoundOSS::getFormats()
{
	format_mask = 0;
	if (ioctl(device_fd, SNDCTL_DSP_GETFMTS, &format_mask) == -1) {
		format_mask = 0;
		throw SndException("OSS formats");
	}
}

void cSoundOSS::Format(int format)
{
	play_format = format;
	if (ioctl(device_fd, SNDCTL_DSP_SETFMT, &play_format) == -1) {
		device_fd = -1;
		formatok = false;
		throw SndException("Unsupported snd card format");
    }
	formatok = true;
}

void cSoundOSS::Channels(int nuchannels)
{
	channels = nuchannels;
	if (ioctl(device_fd, SNDCTL_DSP_CHANNELS, &channels) == -1) {
		device_fd = -1;
		throw "Snd card channel request failed";
	}
}

void cSoundOSS::Frequency(int frequency)
{
	sample_frequency = frequency;
	if (ioctl(device_fd, SNDCTL_DSP_SPEED, &sample_frequency) == -1) {
		device_fd = -1;
		throw SndException("Cannot set frequency");
    }
}

int cSoundOSS::BufferSize( int seconds )
{
	int bytes_per_channel = 0;
	switch (play_format) {
        case AFMT_MU_LAW:
        case AFMT_A_LAW:
        case AFMT_IMA_ADPCM:
            bytes_per_channel = 0; /* format not supported by this program */
			break;
        case AFMT_S16_BE:
        case AFMT_U16_LE:
        case AFMT_U16_BE:
        case AFMT_MPEG:
        case AFMT_S16_LE:
			bytes_per_channel = 2;
			break;
		case AFMT_U8:
        case AFMT_S8:
            bytes_per_channel = 1;
            break;
	}
  return seconds * sample_frequency * bytes_per_channel * channels;
}

bool cSoundOSS::wait_till_finished()
{
	if (ioctl(device_fd, SNDCTL_DSP_POST, 1) == -1 )
		return false;
	if (ioctl(device_fd, SNDCTL_DSP_SYNC, 0) == -1)
		return false; /* format (or ioctl()) not supported by device */
	return true; /* all sound has been played */
}

bool cSoundOSS::reset_device()
{
	if (ioctl(device_fd, SNDCTL_DSP_RESET, 0) == -1) {
		device_fd = -1;
		return false; /* format (or ioctl()) not supported by device */
    }
	return 1; /* sounddevice has been reset */
}

int cSoundOSS::Write(unsigned char *buffer, int buffersize)
{
	if (device_fd == -1)
		return -1;
	return write (device_fd, buffer, buffersize);
}

int cSoundOSS::Read(unsigned char *buffer, int buffersize)
{
	if (device_fd == -1)
		return -1;

	return read (device_fd, buffer, buffersize);
}

int cSoundOSS::Read(double *buffer, int buffersize)
{
	short int *ibuff = (short int *)cbuff;
	int numread;
	if (device_fd == -1)
		return -1;

	numread = Read(cbuff, buffersize * 4);
	for (int i = 0; i < buffersize * 2; i++)
		src_buffer[i] = ibuff[i] / MAXSC;

	for (int i = 0; i < buffersize; i++)
		buffer[i] = src_buffer[2*i];

	if (rxppm != progdefaults.RX_corr) {
		rxppm = progdefaults.RX_corr;
		rx_src_data->src_ratio = 1.0/(1.0 + rxppm/1e6);
		src_set_ratio ( rx_src_state, 1.0/(1.0 + rxppm/1e6));
	}

	if (capture) writeCapture( buffer, buffersize);

	if (playback) {
		readPlayback( buffer, buffersize);
		if (progdefaults.EnableMixer) {
			double vol = valRcvMixer->value();
			for (int i = 0; i < buffersize; i++)
				buffer[i] *= vol;
		}
		return buffersize;
	}

	if (rxppm == 0)
		return buffersize;

// process using samplerate library

	rx_src_data->data_in = src_buffer;
	rx_src_data->input_frames = buffersize;
	rx_src_data->data_out = snd_buffer;
	rx_src_data->output_frames = SND_BUF_LEN;
	rx_src_data->end_of_input = 0;

	if ((numread = src_process(rx_src_state, rx_src_data)) != 0)
		return -1;

	numread = rx_src_data->output_frames_gen;

	for (int i = 0; i < numread; i++)
		buffer[i] = snd_buffer[2*i];

	return numread;

}

int cSoundOSS::write_samples(double *buf, int count)
{
	int retval;
	short int *wbuff;
	unsigned char *p;

	if (generate) writeGenerate( buf, count );

	if (device_fd == -1 || count <= 0)
		return -1;

	if (txppm != progdefaults.TX_corr) {
		txppm = progdefaults.TX_corr;
		tx_src_data->src_ratio = 1.0 + txppm/1e6;
		src_set_ratio ( tx_src_state, 1.0 + txppm/1e6);
	}

	if (txppm == 0) {
		wbuff = new short int[2*count];
		p = (unsigned char *)wbuff;
		for (int i = 0; i < count; i++) {
			wbuff[2*i] = wbuff[2*i+1] = (short int)(buf[i] * maxsc);
		}
		count *= sizeof(short int);
		retval = Write(p, 2*count);
		delete [] wbuff;
	}
	else {
		float *inbuf;
		inbuf = new float[2*count];
		int bufsize;
		for (int i = 0; i < count; i++)
			inbuf[2*i] = inbuf[2*i+1] = buf[i];
		tx_src_data->data_in = inbuf;
		tx_src_data->input_frames = count;
		tx_src_data->data_out = src_buffer;
		tx_src_data->output_frames = SND_BUF_LEN;
		tx_src_data->end_of_input = 0;

		if (src_process(tx_src_state, tx_src_data) != 0) {
			delete [] inbuf;
			return -1;
		}
		delete [] inbuf;
		bufsize = tx_src_data->output_frames_gen;
		wbuff = new short int[2*bufsize];
		p = (unsigned char *)wbuff;

		for (int i = 0; i < 2*bufsize; i++)
			wbuff[i] = (short int)(src_buffer[i] * maxsc);
		int num2write = bufsize * 2 * sizeof(short int);

		retval = Write(p, num2write);
		delete [] wbuff;
		if (retval != num2write)
			return -1;
		retval = count;
	}

	return retval;
}

int cSoundOSS::write_stereo(double *bufleft, double *bufright, int count)
{
	int retval;
	short int *wbuff;
	unsigned char *p;

	if (generate) writeGenerate( bufleft, count );

	if (device_fd == -1 || count <= 0)
		return -1;

	if (txppm != progdefaults.TX_corr) {
		txppm = progdefaults.TX_corr;
		tx_src_data->src_ratio = 1.0 + txppm/1e6;
		src_set_ratio ( tx_src_state, 1.0 + txppm/1e6);
	}

	if (txppm == 0) {
		wbuff = new short int[2*count];
		p = (unsigned char *)wbuff;
		for (int i = 0; i < count; i++) {
			wbuff[2*i] = (short int)(bufleft[i] * maxsc);
			wbuff[2*i + 1] = (short int)(bufright[i] * maxsc);
		}
		count *= sizeof(short int);
		retval = Write(p, 2*count);
		delete [] wbuff;
	}
	else {
		float *inbuf;
		inbuf = new float[2*count];
		int bufsize;
		for (int i = 0; i < count; i++) {
			inbuf[2*i] = bufleft[i];
			inbuf[2*i+1] = bufright[i];
		}
		tx_src_data->data_in = inbuf;
		tx_src_data->input_frames = count;
		tx_src_data->data_out = src_buffer;
		tx_src_data->output_frames = SND_BUF_LEN;
		tx_src_data->end_of_input = 0;

		if (src_process(tx_src_state, tx_src_data) != 0) {
			delete [] inbuf;
			return -1;
		}
		delete [] inbuf;
		bufsize = tx_src_data->output_frames_gen;
		wbuff = new short int[2*bufsize];
		p = (unsigned char *)wbuff;

		for (int i = 0; i < 2*bufsize; i++)
			wbuff[i] = (short int)(src_buffer[i] * maxsc);

		int num2write = bufsize * 2 * sizeof(short int);
		retval = Write(p, num2write);
		delete [] wbuff;
		if (retval != num2write)
			return -1;
		retval = count;
	}

	return retval;
}


#if USE_PORTAUDIO

bool cSoundPA::pa_init = false;
std::vector<const PaDeviceInfo*> cSoundPA::devs;
void cSoundPA::initialize(void)
{
        if (pa_init)
                return;

        int err;

        if ((err = Pa_Initialize()) != paNoError)
                throw SndException(err);
        pa_init = true;

        PaDeviceIndex ndev = Pa_GetDeviceCount();
        if ((ndev = Pa_GetDeviceCount()) < 0)
                throw SndException(ndev);
        if (ndev == 0)
                throw SndException("No available audio devices");

        devs.reserve(ndev);
        for (PaDeviceIndex i = 0; i < ndev; i++)
                devs.push_back(Pa_GetDeviceInfo(i));

}
void cSoundPA::terminate(void)
{
        static_cast<void>(Pa_Terminate());
        pa_init = false;
        devs.clear();
}
const std::vector<const PaDeviceInfo*>& cSoundPA::devices(void)
{
        return devs;
}

cSoundPA::cSoundPA(const char *dev)
        : device(dev), stream(0), frames_per_buffer(paFramesPerBufferUnspecified),
          req_sample_rate(0), dev_sample_rate(0), fbuf(0)
{
        try {
                rx_src_data = new SRC_DATA;
                tx_src_data = new SRC_DATA;
        }
        catch (const std::bad_alloc& e) {
                cerr << "Cannot create libsamplerate data structures\n";
                throw;
        }

        try {
                snd_buffer = new float[2 * SND_BUF_LEN];
                src_buffer = new float[2 * SND_BUF_LEN];
                fbuf = new float[2 * SND_BUF_LEN];
        }
        catch (const std::bad_alloc& e) {
                cerr << "Cannot allocate libsamplerate buffers\n";
                throw;
        }

        memset(snd_buffer, 0, 2 * SND_BUF_LEN);
        memset(src_buffer, 0, 2 * SND_BUF_LEN);
        memset(fbuf, 0, 2 * SND_BUF_LEN);
}

cSoundPA::~cSoundPA()
{
        Close();
        delete [] fbuf;
}

int cSoundPA::Open(int mode, int freq)
{
        int old_sample_rate = (int)req_sample_rate;
        req_sample_rate = sample_frequency = freq;

        // Try to keep the stream open if we are using jack, or if we
        // are in full duplex mode and the sample rate has not changed.
        if (stream_active()) {
                if (Pa_GetHostApiInfo((*idev)->hostApi)->type == paJACK) {
                        // If we have a new sample rate, we must reset the src data.
                        if (old_sample_rate != freq)
                                src_data_reset(1 << O_RDONLY | 1 << O_WRONLY);
                        return 0;
                }
                else if (full_duplex_device(*idev) && old_sample_rate == freq)
                        return 0;
        }

        Close();
        init_stream();
#ifndef NDEBUG
        if (dev_sample_rate != req_sample_rate)
                cerr << "PA_debug: resampling " << dev_sample_rate
                     << " <-> " << req_sample_rate << endl;
#endif

        mode = full_duplex()  ?  1 << O_RDONLY | 1 << O_WRONLY  :  1 << mode;
        if (!(mode & 1 << O_WRONLY))
                stream_params[STREAM_OUT] = NULL;
        if (!(mode & 1 << O_RDONLY))
                stream_params[STREAM_IN] = NULL;
        src_data_reset(mode);

        start_stream();

        return 0;
}

void cSoundPA::Close(void)
{
        if (!stream_active())
                return;

        int err;

        if ((err = Pa_StopStream(stream)) != paNoError)
                pa_perror(err, "Pa_StopStream");
        if ((err = Pa_CloseStream(stream)) != paNoError)
                pa_perror(err, "Pa_CloseStream");

        stream = 0;
}

int cSoundPA::Read(double *buf, int count)
{
        int ncount = (int)floor(MIN(count, SND_BUF_LEN) / rx_src_data->src_ratio);

        int err;
        static int retries = 0;
        if ((err = Pa_ReadStream(stream, fbuf, ncount)) != paNoError) {
                pa_perror(err, "Pa_ReadStream");
                switch (err) {
                case paInputOverflowed:
                        return adjust_stream() ? Read(buf, count) : 0;
                case paUnanticipatedHostError:
                        if (Pa_GetHostApiInfo((*idev)->hostApi)->type == paOSS && retries++ < 8) {
                                cerr << "Retrying read\n";
                                return Read(buf, count);
                        }
                        else
                                cerr << "Giving up\n";
                        // fall through
                default:
                        throw SndException(err);
                }
        }
        retries = 0;

	if (capture)
                writeCapture(buf, count);
	if (playback) {
		readPlayback(buf, count);
		if (progdefaults.EnableMixer) {
	                double vol = valRcvMixer->value();
	                for (int i = 0; i < count; i++)
        	                buf[i] *= vol;
		}
		return count;
	}

        float *rbuf = fbuf;
        if (req_sample_rate != dev_sample_rate || progdefaults.RX_corr != 0) {
                resample(1 << O_RDONLY, rbuf, ncount, count);
                rbuf = rx_src_data->data_out;
                count = rx_src_data->output_frames_gen;
        }

        for (int i = 0; i < count; i++)
                buf[i] = rbuf[2*i];

        return count;
}

int cSoundPA::write_samples(double *buf, int count)
{
	if (generate)
                writeGenerate(buf, count);

        for (int i = 0; i < count; i++)
                fbuf[2*i] = fbuf[2*i + 1] = buf[i];

        float *wbuf = fbuf;
        if (req_sample_rate != dev_sample_rate || progdefaults.TX_corr != 0) {
                resample(1 << O_WRONLY, wbuf, count);
                wbuf = tx_src_data->data_out;
                count = tx_src_data->output_frames_gen;
        }

        int err;
        static int retries = 0;
        if ((err = Pa_WriteStream(stream, wbuf, count)) != paNoError) {
                pa_perror(err, "Pa_WriteStream");
                switch (err) {
                case paOutputUnderflowed:
                        return adjust_stream() ? write_samples(buf, count) : 0;
                case paUnanticipatedHostError:
                        if (Pa_GetHostApiInfo((*idev)->hostApi)->type == paOSS && retries++ < 8) {
                                cerr << "Retrying write\n";
                                return write_samples(buf, count);
                        }
                        else
                                cerr << "Giving up\n";
                        // fall through
                default:
                        throw SndException(err);
                }
        }
        retries = 0;

        return count;
}

int cSoundPA::write_stereo(double *bufleft, double *bufright, int count)
{
	if (generate)
                writeGenerate(bufleft, count);

        for (int i = 0; i < count; i++) {
                fbuf[2*i] = bufleft[i];
                fbuf[2*i + 1] = bufright[i];
        }

        float *wbuf = fbuf;
        if (req_sample_rate != dev_sample_rate || progdefaults.TX_corr != 0) {
                resample(1 << O_WRONLY, wbuf, count);
                wbuf = tx_src_data->data_out;
                count = tx_src_data->output_frames_gen;
        }

        int err;
        static int retries = 0;
        if ((err = Pa_WriteStream(stream, wbuf, count)) != paNoError) {
                pa_perror(err, "Pa_WriteStream");
                switch (err) {
                case paOutputUnderflowed:
                        return adjust_stream() ? write_stereo(bufleft, bufright, count) : 0;
                case paUnanticipatedHostError:
                        if (Pa_GetHostApiInfo((*idev)->hostApi)->type == paOSS && retries++ < 8) {
                                cerr << "Retrying write\n";
                                return write_stereo(bufleft, bufright, count);
                        }
                        else
                                cerr << "Giving up\n";
                        // fall through
                default:
                        throw SndException(err);
                }

                throw SndException(err);
        }
        retries = 0;

        return count;
}

bool cSoundPA::full_duplex(void)
{
        extern bool pa_allow_full_duplex;
        return (pa_allow_full_duplex && full_duplex_device(*idev)) ||
                Pa_GetHostApiInfo((*idev)->hostApi)->type == paJACK;
}

void cSoundPA::src_data_reset(int mode)
{
        int err;
        if (mode & 1 << O_RDONLY) {
                if (rx_src_state)
                        src_delete(rx_src_state);
                rx_src_state = src_new(SRC_SINC_FASTEST, 2, &err);
                if (!rx_src_state)
                        throw SndException(src_strerror(err));
                rx_src_data->src_ratio = req_sample_rate / (dev_sample_rate * (1.0 + rxppm / 1e6));
        }
        if (mode & 1 << O_WRONLY) {
                if (tx_src_state)
                        src_delete(tx_src_state);
                tx_src_state = src_new(SRC_SINC_FASTEST, 2, &err);
                if (!tx_src_state)
                        throw SndException(src_strerror(err));
                tx_src_data->src_ratio = dev_sample_rate * (1.0 + txppm / 1e6) / req_sample_rate;
        }
}

void cSoundPA::resample(int mode, float *buf, int count, int max)
{
        if (mode & 1 << O_RDONLY) {
                if (rxppm != progdefaults.RX_corr) {
                        rxppm = progdefaults.RX_corr;
                        rx_src_data->src_ratio = req_sample_rate
                                / dev_sample_rate
                                * (1.0 + rxppm / 1e6);
                        src_set_ratio(rx_src_state, rx_src_data->src_ratio);
                }

                rx_src_data->data_in = buf;
                rx_src_data->input_frames = count;
                rx_src_data->data_out = snd_buffer;
                rx_src_data->output_frames = max ? max : SND_BUF_LEN;
                rx_src_data->end_of_input = 0;

                src_process(rx_src_state, rx_src_data);
        }
        else if (mode & 1 << O_WRONLY) {
                if (txppm != progdefaults.TX_corr) {
                        txppm = progdefaults.TX_corr;
                        tx_src_data->src_ratio = dev_sample_rate
                                * (1.0 + txppm / 1e6)
                                / req_sample_rate;
                        src_set_ratio(tx_src_state, tx_src_data->src_ratio);
                }

                tx_src_data->data_in = buf;
                tx_src_data->input_frames = count;
                tx_src_data->data_out = src_buffer;
                tx_src_data->output_frames = max ? max : SND_BUF_LEN;
                tx_src_data->end_of_input = 0;

                src_process(tx_src_state, tx_src_data);
        }
}

void cSoundPA::init_stream(void)
{
#ifndef NDEBUG
        cerr << "PA_debug: looking for \"" << device << "\"\n";
#endif
        for (idev = devs.begin(); idev != devs.end(); ++idev)
                if (device == (*idev)->name)
                        break;
        if (idev == devs.end()) {
                cerr << "PA_debug: could not find device \"" << device << "\"\n";
                idev = devs.begin();
        }
        PaDeviceIndex idx = idev - devs.begin();

#ifndef NDEBUG
        cerr << "PA_debug: using device:"
             << "\n index: " << idx
             << "\n name: " << (*idev)->name
             << "\n hostAPI: " << Pa_GetHostApiInfo((*idev)->hostApi)->name
             << "\n maxInputChannels: " << (*idev)->maxInputChannels
             << "\n maxOutputChannels: " << (*idev)->maxOutputChannels
             << "\n defaultLowInputLatency: " << (*idev)->defaultLowInputLatency
             << "\n defaultHighInputLatency: " << (*idev)->defaultHighInputLatency
             << "\n defaultLowOutputLatency: " << (*idev)->defaultLowOutputLatency
             << "\n defaultHighOutputLatency: " << (*idev)->defaultHighOutputLatency
             << "\n defaultSampleRate: " << (*idev)->defaultSampleRate
             << boolalpha
             << "\n isInputOnlyDevice: " << ((*idev)->maxOutputChannels == 0)
             << "\n isOutputOnlyDevice: " << ((*idev)->maxInputChannels == 0)
             << "\n isFullDuplexDevice: " << full_duplex_device(*idev)
             << "\n isSystemDefaultInputDevice: " << (idx == Pa_GetDefaultInputDevice())
             << "\n isSystemDefaultOutputDevice: " << (idx == Pa_GetDefaultOutputDevice())
             << "\n isHostApiDefaultInputDevice: " << (idx == Pa_GetHostApiInfo((*idev)->hostApi)->defaultInputDevice)
             << "\n isHostApiDefaultOutputDevice: " << (idx == Pa_GetHostApiInfo((*idev)->hostApi)->defaultOutputDevice)
             << "\n\n";
#endif

        // we are unlikely to have an output-only device
        if ((*idev)->maxInputChannels == 0)
                throw SndException(EBUSY);

        in_params.device = idx;
        in_params.channelCount = 2;
        in_params.sampleFormat = paFloat32;
        in_params.suggestedLatency = (*idev)->defaultHighInputLatency;
        in_params.hostApiSpecificStreamInfo = NULL;
        stream_params[STREAM_IN] = &in_params;

        out_params.device = idx;
        out_params.channelCount = 2;
        out_params.sampleFormat = paFloat32;
        out_params.suggestedLatency = (*idev)->defaultHighOutputLatency;
        out_params.hostApiSpecificStreamInfo = NULL;
        stream_params[STREAM_OUT] = &out_params;

        dev_sample_rate = find_srate();

        max_frames_per_buffer = ceil2(MIN(SND_BUF_LEN, (unsigned)(SCBLOCKSIZE *
                                                                  dev_sample_rate / req_sample_rate)));
#ifndef NDEBUG
        cerr << "PA_debug: max_frames_per_buffer = " << max_frames_per_buffer << endl;
#endif
        extern int pa_frames_per_buffer;
        if (pa_frames_per_buffer)
                frames_per_buffer = pa_frames_per_buffer;
}

void cSoundPA::start_stream(void)
{
        int err;

        err = Pa_OpenStream(&stream, stream_params[STREAM_IN], stream_params[STREAM_OUT],
                            dev_sample_rate, frames_per_buffer, paNoFlag, NULL, NULL);
        if (err != paNoError)
                throw SndException(err);
        if ((err = Pa_StartStream(stream)) != paNoError) {
                Close();
                throw SndException(err);
        }
}

bool cSoundPA::stream_active(void)
{
        if (!stream)
                return false;

        int err;

        if ((err = Pa_IsStreamActive(stream)) < 0)
                throw SndException(err);

        return err == 1;
}

bool cSoundPA::full_duplex_device(const PaDeviceInfo* dev)
{
        return dev->maxInputChannels > 0 && dev->maxOutputChannels > 0;
}

bool cSoundPA::adjust_stream(void)
{
        if (frames_per_buffer == max_frames_per_buffer)
                return false;

        if (frames_per_buffer != paFramesPerBufferUnspecified)
                frames_per_buffer *= 2;
        else
                frames_per_buffer = SCBLOCKSIZE;

        if (!powerof2(frames_per_buffer))
                frames_per_buffer = ceil2(frames_per_buffer);

        frames_per_buffer = MIN(max_frames_per_buffer, frames_per_buffer);

        cerr << "PA_debug: adjusting frames_per_buffer to "
             << frames_per_buffer << endl;
        Close();
        start_stream();

        return true;
}

// Determine the sample rate that we will use. We try the modem's rate
// first and fall back to the device's default rate. If there is a user
// setting we just return that without making any checks.
double cSoundPA::find_srate(void)
{
        switch (progdefaults.sample_rate) {
        case 0:
                break;
        case 1:
                return (*idev)->defaultSampleRate;
        default:
                return progdefaults.sample_rate;
        }

        double srates[] = { req_sample_rate, (*idev)->defaultSampleRate };
        int err;
        for (size_t i = 0; i < sizeof(srates)/sizeof(srates[0]); i++) {
#ifndef NDEBUG
                cerr << "PA_debug: trying " << srates[i] << " Hz" << endl;
#endif
                if ((err = Pa_IsFormatSupported(&in_params, &out_params, srates[i])) == paFormatIsSupported)
                        return srates[i];
#ifndef NDEBUG
                else
                        pa_perror(err, "Pa_IsFormatSupported");
#endif
        }

        throw SndException(err);
        return -1;
}

void cSoundPA::pa_perror(int err, const char* str)
{
        if (str)
                cerr << str << ": " << Pa_GetErrorText(err) << '\n';

        if (err == paUnanticipatedHostError) {
                const PaHostErrorInfo* hosterr = Pa_GetLastHostErrorInfo();
                PaHostApiIndex i = Pa_HostApiTypeIdToHostApiIndex(hosterr->hostApiType);

                if (i < 0) { // PA failed without setting its "last host error" info. Sigh...
                        cerr << "Host API error info not available\n";
                        if (Pa_GetHostApiInfo((*idev)->hostApi)->type == paOSS && errno)
                                cerr << "Possible OSS error " << errno << ": "
                                     << strerror(errno) << '\n';
                }
                else
                        cerr << Pa_GetHostApiInfo(i)->name << " error "
                             << hosterr->errorCode << ": " << hosterr->errorText << '\n';
        }
}

#endif // USE_PORTAUDIO
