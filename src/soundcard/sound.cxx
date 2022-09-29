// ----------------------------------------------------------------------------
//
//	sound.cxx
//
// Copyright (C) 2006-2013
//			Dave Freese, W1HKJ
//
// Copyright (C) 2007-2010
//			Stelios Bounanos, M0GLD
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

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>
#include <iterator>

#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <unistd.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <limits.h>

#if USE_OSS
#  include <sys/ioctl.h>
#  if defined(__OpenBSD__)
#   include <soundcard.h>
#  else
#   include <sys/soundcard.h>
#  endif
#endif
#include <math.h>

#if HAVE_DLOPEN
#  include <dlfcn.h>
#endif

#include "gettext.h"
#include "sound.h"
#include "configuration.h"
#include "status.h"
#include "fileselect.h"
#include "trx.h"
#include "fl_digi.h"
#include "threads.h"
#include "timeops.h"
#include "ringbuffer.h"
#include "debug.h"
#include "qrunner.h"
#include "icons.h"
#include "macros.h"
#include "util.h"

#include "estrings.h"

#include "dr_mp3.h"

#define SND_BUF_LEN	 65536
#define SND_RW_LEN	(8 * SND_BUF_LEN)

#define SNDFILE_CHANNELS 2

int sndfile_samplerate[7] = {8000, 11025, 16000, 22050, 24000, 44100, 48000};

using namespace std;

LOG_FILE_SOURCE(debug::LOG_AUDIO);

namespace SND_SUPPORT {
	bool format_supported(int format) {
		SF_INFO info = {
			0,
			sndfile_samplerate[progdefaults.wavSampleRate],
			progdefaults.record_both_channels ? 2 : 1,
			format, 0, 0 };
		SNDFILE* sndf = sf_open("temp.audio", SFM_WRITE, &info);
		sf_close(sndf);
		remove("temp.audio");
		if (sndf) return true;
		return false;
	}

	void get_file_params(std::string def_fname, std::string &fname, int &format, bool check) {
		bool isplayback = (def_fname.find("playback") != std::string::npos);
		std::string filters;
		if (isplayback)
			filters = "Audio format\t*.{mp3,wav}\n";
		else
			filters = "Audio format\t*.wav\n";
		format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
		int fsel = 0;
		const char *fn = 0;
		if (isplayback)
			fn = FSEL::select(_("Audio file"), filters.c_str(), def_fname.c_str(), &fsel);
		else
			fn = FSEL::saveas(_("Audio file"), filters.c_str(), def_fname.c_str(), &fsel);
		if (!fn || !*fn) {
			fname = "";
			return;
		}
		fname = fn;

		if (!isplayback && (fname.find(".wav") == std::string::npos))
			fname.append(".wav");

		if (check) {
			FILE *f = fopen(fname.c_str(), "r");
			if (f) {
				fclose(f);
				int ans = fl_choice("Replace %s?", "Yes", "No", 0, fname.c_str());
				if ( ans == 1) fname = "";
			}
		}
	}

	void tag_file(SNDFILE *sndfile, const char *title) {
		int err;
		if ((err = sf_set_string(sndfile, SF_STR_TITLE, title)) != 0) {
			LOG_VERBOSE("sf_set_string STR_TITLE: %s", sf_error_number(err));
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
		if (strftime(s, sizeof(s), "%Y-%m-%dT%H:%M:%Sz", &zt) > 0)
			sf_set_string(sndfile, SF_STR_DATE, s);
	}

};

SoundBase::SoundBase()
		: sample_frequency(0),
	  txppm(progdefaults.TX_corr), rxppm(progdefaults.RX_corr),
		  tx_src_state(0), rx_src_state(0),
		  wrt_buffer(new double[SND_BUF_LEN]),
		  ofCapture(0), ifPlayback(0), ofGenerate(0)
{
	memset(wrt_buffer, 0, SND_BUF_LEN * sizeof(*wrt_buffer));

	int err;
	writ_src_data_left = new SRC_DATA;
	writ_src_data_right = new SRC_DATA;
	play_src_data = new SRC_DATA;

	writ_src_state_left = src_new(progdefaults.sample_converter, 1, &err);
	if (writ_src_state_left == 0)
		throw SndException(src_strerror(err));

	writ_src_state_right = src_new(progdefaults.sample_converter, 1, &err);
	if (writ_src_state_right == 0)
		throw SndException(src_strerror(err));

	play_src_state = src_new(progdefaults.sample_converter, 1, &err);
	if (play_src_state == 0)
		throw SndException(src_strerror(err));

	if (play_src_state == 0)
		throw SndException(src_strerror(err));

	src_write_buffer_left = new float [SND_RW_LEN];
	if (!src_write_buffer_left)
		throw SndException(src_strerror(err));

	src_write_buffer_right = new float [SND_RW_LEN];
	if (!src_write_buffer_right)
		throw SndException(src_strerror(err));

	src_rd_inp_buffer = new float [SND_RW_LEN];
	if (!src_rd_inp_buffer)
		throw SndException(src_strerror(err));

	src_rd_out_buffer = new float [SND_RW_LEN];
	if (!src_rd_out_buffer)
		throw SndException(src_strerror(err));

	modem_wr_sr = modem_play_sr = 0;
	out_pointer = src_rd_out_buffer;
}

SoundBase::~SoundBase()
{
	delete [] wrt_buffer;

	if (ofGenerate)
		sf_close(ofGenerate);
	if (ofCapture)
		sf_close(ofCapture);
	if (ifPlayback)
		sf_close(ifPlayback);
	delete writ_src_data_left;
	delete writ_src_data_right;
	delete play_src_data;
	delete [] src_write_buffer_left;
	delete [] src_write_buffer_right;
	delete [] src_rd_inp_buffer;
	delete [] src_rd_out_buffer;
}

void SoundBase::stopCapture()
{
	if (ofCapture) {
		int err;
		if ((err = sf_close(ofCapture)) != 0)
			LOG_ERROR("sf_close error: %s", sf_error_number(err));
		ofCapture = 0;
	}
}

int SoundBase::startCapture(std::string fname, int format)
{
	// frames (ignored), freq, channels, format, sections (ignored), seekable (ignored)
	SF_INFO info = { 0, sndfile_samplerate[progdefaults.wavSampleRate], 
		progdefaults.record_both_channels ? 2 : 1,
//		SNDFILE_CHANNELS,
		format, 0, 0 };
	if ((ofCapture = sf_open(fname.c_str(), SFM_WRITE, &info)) == NULL) {
		LOG_ERROR("Could not write %s:%s", fname.c_str(), sf_strerror(NULL) );
		return 0;
	}
	if (sf_command(ofCapture, SFC_SET_UPDATE_HEADER_AUTO, NULL, SF_TRUE) != SF_TRUE)
		LOG_ERROR("ofCapture update header command failed: %s", sf_strerror(ofCapture));

	SND_SUPPORT::tag_file(ofCapture, "Captured audio");

	return 1;
}

void SoundBase::stopGenerate()
{
	if (ofGenerate) {
		int err;
		if ((err = sf_close(ofGenerate)) != 0)
			LOG_ERROR("sf_close error: %s", sf_error_number(err));
		ofGenerate = 0;
	}
}

//int SoundBase::startGenerate(bool val, std::string fname, int format)
int SoundBase::startGenerate(std::string fname, int format)
{
	SF_INFO info = { 0, sndfile_samplerate[progdefaults.wavSampleRate], 
		progdefaults.record_both_channels ? 2 : 1,
//		SNDFILE_CHANNELS,
		format, 0, 0 };
	if ((ofGenerate = sf_open(fname.c_str(), SFM_WRITE, &info)) == NULL) {
		LOG_ERROR("Could not write %s", fname.c_str());
		return 0;
	}
	if (sf_command(ofGenerate, SFC_SET_UPDATE_HEADER_AUTO, NULL, SF_TRUE) != SF_TRUE)
		LOG_ERROR("ofGenerate update header command failed: %s", sf_strerror(ofGenerate));

	SND_SUPPORT::tag_file(ofGenerate, "Generated audio");

	modem_wr_sr = sample_frequency;

	writ_src_data_left->src_ratio = 1.0 * sndfile_samplerate[progdefaults.wavSampleRate] / modem_wr_sr;
	src_set_ratio(writ_src_state_left, writ_src_data_left->src_ratio);

	writ_src_data_right->src_ratio = 1.0 * sndfile_samplerate[progdefaults.wavSampleRate] / modem_wr_sr;
	src_set_ratio(writ_src_state_right, writ_src_data_right->src_ratio);

	return 1;
}

void SoundBase::stopPlayback()
{
	if (ifPlayback) {
		int err;
		if ((err = sf_close(ifPlayback)) != 0)
			LOG_ERROR("sf_close error: %s", sf_error_number(err));
		ifPlayback = 0;
	}
	progdefaults.loop_playback = false;
}

int SoundBase::startPlayback(std::string fname, int format)
{
	play_info.frames = 0;
	play_info.samplerate = 0;
	play_info.channels = 0;
	play_info.format = 0;
	play_info.sections = 0;
	play_info.seekable = 0;

	if ((ifPlayback = sf_open(fname.c_str(), SFM_READ, &play_info)) == NULL) {
		LOG_ERROR("Could not open %s:%s", fname.c_str(), sf_strerror(NULL) );
		return 1;
	}

LOG_VERBOSE
("wav file stats:\n\
frames     : %d\n\
samplerate : %d\n\
channels   : %d\n\
format     : %d\n\
sections   : %d\n\
seekable   : %d",
static_cast<unsigned int>(play_info.frames),
play_info.samplerate,
play_info.channels,
play_info.format,
play_info.sections,
play_info.seekable);

	modem_play_sr = sample_frequency;
	play_src_data->src_ratio = 1.0 * modem_play_sr / play_info.samplerate;
	src_set_ratio(play_src_state, play_src_data->src_ratio);

	LOG_VERBOSE("src ratio %f", play_src_data->src_ratio);

	new_playback = true;

	return 0;
}

// ---------------------------------------------------------------------
// read_file
//    can be simplified from the equivalent read audio stream
//   source sr is arbitrary, requested is either 8000 or 11025 depending
// on the modem in use
// read from file and resample until a "count" number of converted samples
// is available, or until at the end of the input file
// ---------------------------------------------------------------------
extern int fmt_auto_record;

sf_count_t SoundBase::read_file(SNDFILE* file, float* buf, size_t count)
{
	sf_count_t r = 0, rd_count = 0;
	int err = 0;

	if (new_playback || modem_play_sr != sample_frequency) {
		modem_play_sr = sample_frequency;
		play_src_data->src_ratio = 1.0 * modem_play_sr / play_info.samplerate;
		src_set_ratio(play_src_state, play_src_data->src_ratio);
		LOG_VERBOSE("src ratio %f", play_src_data->src_ratio);
		new_playback = true;
	}
	if (new_playback) {
		fmt_auto_record = 1;
	}

#define RDBLKSIZE 1024
	float rdbuf[2 * RDBLKSIZE];
	int ch = play_info.channels;
	while ( static_cast<size_t>(out_pointer - src_rd_out_buffer) < count) {
		memset(src_rd_inp_buffer, 0, RDBLKSIZE * sizeof(float));
		if (new_playback) {
			new_playback = false;
			rd_count = RDBLKSIZE;
		}
		else {
			memset(rdbuf, 0, 2 * RDBLKSIZE * sizeof(float));
			rd_count = sf_readf_float(file, rdbuf, RDBLKSIZE);
			if (!rd_count) break;
			for (int i = 0; i < rd_count; i++)
				src_rd_inp_buffer[i] = rdbuf[i * ch];
		}

		play_src_data->data_in = src_rd_inp_buffer;
		play_src_data->input_frames = rd_count;
		play_src_data->data_out = out_pointer;
		play_src_data->output_frames = SND_RW_LEN - (out_pointer - src_rd_out_buffer);
		play_src_data->end_of_input = 0;

		if ((err = src_process(play_src_state, play_src_data)) != 0)
			throw SndException(src_strerror(err));

		out_pointer += play_src_data->output_frames_gen;

	}

	if ( static_cast<size_t>(out_pointer - src_rd_out_buffer) >= count) {
		memcpy(buf, src_rd_out_buffer, count * sizeof(float));
		memmove(src_rd_out_buffer, src_rd_out_buffer + count, (SND_RW_LEN - count) * sizeof(float));
		out_pointer -= count;
		r = count;
	}

	if (r == 0) {
		src_reset (play_src_state);
		out_pointer = src_rd_out_buffer;
		if (!progdefaults.loop_playback) {
			stopPlayback();
			bHighSpeed = false;
			REQ(reset_mnuPlayback);
		} else {
			memset(buf, count, sizeof(*buf));
			sf_seek(file, 0, SEEK_SET);
		}
	}
	return r;
}

//----------------------------------------------------------------------
// Audio
// Adds ability to transmit an audio file using new macro tag:
//   <AUDIO:path-filename>
//   macro editor opens an OS select file dialog when the tag is
//   selected from the pick list.
//   suggested use:
//     <MODEM:NULL><TX>
//     <AUDIO:path-filename-1>
//     <AUDIO:path-filename-2>
//     <RX><@MODEM:BPSK31>
//   or modem type of choice
//   Audio file may be either wav or mp3 format, either mono or stereo 
//   any sample rate
//   Returning to Rx stops current and any pending audio playback.  Post
//   Tx macro tags are then executed.
//   T/R button or Escape key will abort the playback.
// Please use responsibly - know and understand your license limitations
// for transmitting audio files, especially music and/or copyrighted
// material.
//----------------------------------------------------------------------
int SoundBase::AudioMP3(std::string fname)
{
	drmp3_config config;
	drmp3_uint64 frame_count;

	float* mp3_buffer =  drmp3_open_file_and_read_f32(
						fname.c_str(), &config, &frame_count );

	if (!mp3_buffer) {
		LOG_ERROR("File must be mp3 float format");
		return 0;
	}

	LOG_INFO("\n\
MP3 parameters\n\
      channels: %d\n\
   sample rate: %d\n\
   frame count: %ld\n", 
       config.outputChannels,
       config.outputSampleRate,
       long(frame_count));

	float *buffer = new float[2 * frame_count];
	if (!buffer) {
		LOG_ERROR("Could not allocate audio buffer");
		drmp3_free(mp3_buffer);
		return 0;
	}
	if (config.outputChannels == 2) {
		float maxval = 1.0;
		for (unsigned int n = 0; n < frame_count; n++) {
			buffer[2 * n] = mp3_buffer[2 * n] + mp3_buffer[2 * n + 1];
			buffer[2 * n + 1] = 0;
			if (fabs(buffer[2 * n]) > maxval) maxval = fabs(buffer[ 2 * n]);
		}
		for (unsigned int n = 0; n < frame_count; n++)
			buffer[2 * n] /= maxval;
	} else {
		for (unsigned int n = frame_count - 1; n >= 0; n--) {
			buffer[2 * n] = mp3_buffer[n];
			buffer[2 * n + 1] = 0;
		}
	}
	drmp3_free(mp3_buffer);

	double save_sample_rate = req_sample_rate;
	req_sample_rate = config.outputSampleRate;

	unsigned int n = 0;
	int incr = SCBLOCKSIZE;
	while (n < frame_count) {
		if (active_modem->get_stopflag())  {
			Rx_queue_execute();
			break;
		}
		if (n + incr < frame_count)
			resample_write(&buffer[n*2], incr);
		else
			resample_write(&buffer[n*2], frame_count - n);
		n += incr;
	}

	delete [] buffer;
	req_sample_rate = save_sample_rate;
	return n;

}

int SoundBase::AudioWAV(std::string fname)
{
	SNDFILE *playback;
	play_info.frames = 0;
	play_info.samplerate = 0;
	play_info.channels = 0;
	play_info.format = 0;
	play_info.sections = 0;
	play_info.seekable = 0;

	if ((playback = sf_open(fname.c_str(), SFM_READ, &play_info)) == NULL) {
		LOG_ERROR("Could not open %s", fname.c_str());
		return 0;
	}
	LOG_INFO("\
\nAudio file: %s\
\nframes:     %ld\
\nsamplerate: %d\
\nchannels:   %d",
fname.c_str(), (long)play_info.frames, play_info.samplerate, play_info.channels);

	int ch = play_info.channels;

	if (ch > 2) return 0;

	int fsize = play_info.frames * 2;

	float *buffer = new float[fsize];
	if (!buffer)
		return 0;
	memset(buffer, 0, fsize * sizeof(*buffer));

	int ret = sf_readf_float( playback, buffer, play_info.frames);
	if (!ret) {
		sf_close(playback);
		delete [] buffer;
		return 0;
	}

	double save_sample_rate = req_sample_rate;
	req_sample_rate = play_info.samplerate;
	if (ch == 1) {
		for (long int n = play_info.frames - 1; n >= 0; n--) {
			buffer[2 * n] = buffer[n];
			buffer[2 * n + 1] = 0;
		}
	} else {
		float maxval = 1.0;
		for (long int n = play_info.frames - 1; n >= 0; n--) {
			buffer[2 * n] = buffer[2 * n] + buffer[2 * n + 1];
			buffer[2 * n + 1] = 0;
			if (fabs(buffer[2*n] > maxval)) maxval = fabs(buffer[2*n]);
		}
		for (long int n = 0; n < play_info.frames; n++)
			buffer[2*n] /= maxval;
	}

	unsigned int n = 0;
	int incr = SCBLOCKSIZE;
	while (n < play_info.frames) {
		if (active_modem->get_stopflag())  {
			Rx_queue_execute();
			break;
		}
		if (n + incr < play_info.frames)
			resample_write(&buffer[n*2], incr);
		else
			resample_write(&buffer[n*2], play_info.frames - n);
		n += incr;
	}

	sf_close(playback);
	delete [] buffer;
	req_sample_rate = save_sample_rate;
	return play_info.frames;
}

int SoundBase::Audio(std::string fname)
{
	if (fname.empty())
		return 0;
	if ((fname.find("mp3") != std::string::npos) || 
		(fname.find("MP3") != std::string::npos ))
		return AudioMP3(fname);
	else
		return AudioWAV(fname);
}
// ---------------------------------------------------------------------
// write_file
// All sound buffer data is resampled to a specified sample rate
//    progdefaults.wavSampleRate
// resultant data (left channel only) is written to a wav file
//----------------------------------------------------------------------
void SoundBase::write_file(SNDFILE* file, float* bufleft, float* bufright, size_t count)
{
	bool must_delete = false;
	if (bufright == NULL || !progdefaults.record_both_channels) {
		bufright = new float[count];
		memset(bufright, 0, count * sizeof(float));
		must_delete = true;
	}

	int err;
	size_t output_size = count;
	float *bufl = bufleft;
	float *bufr = bufright;

	if (modem_wr_sr != sample_frequency) {
		modem_wr_sr = sample_frequency;
		writ_src_data_left->src_ratio = 1.0 * sndfile_samplerate[progdefaults.wavSampleRate] / modem_wr_sr;
		writ_src_data_left->src_ratio = 1.0 * sndfile_samplerate[progdefaults.wavSampleRate] / modem_wr_sr;
		src_set_ratio(writ_src_state_left, writ_src_data_left->src_ratio);

		writ_src_data_right->src_ratio = 1.0 * sndfile_samplerate[progdefaults.wavSampleRate] / modem_wr_sr;
		writ_src_data_right->src_ratio = 1.0 * sndfile_samplerate[progdefaults.wavSampleRate] / modem_wr_sr;
		src_set_ratio(writ_src_state_right, writ_src_data_right->src_ratio);
	}
	writ_src_data_left->data_in = bufleft;
	writ_src_data_left->input_frames = count;
	writ_src_data_left->data_out = src_write_buffer_left;
	writ_src_data_left->output_frames = SND_RW_LEN;
	writ_src_data_left->end_of_input = 0;

	writ_src_data_right->data_in = bufright;
	writ_src_data_right->input_frames = count;
	writ_src_data_right->data_out = src_write_buffer_right;
	writ_src_data_right->output_frames = SND_RW_LEN;
	writ_src_data_right->end_of_input = 0;

	if ((err = src_process(writ_src_state_left, writ_src_data_left)) != 0) {
		if (must_delete) {
			delete [] bufright;
		}
		throw SndException(src_strerror(err));
	}
	if ((err = src_process(writ_src_state_right, writ_src_data_right)) != 0) {
		if (must_delete) {
			delete [] bufright;
		}
		throw SndException(src_strerror(err));
	}

	output_size = writ_src_data_left->output_frames_gen;
	bufl = src_write_buffer_left;
	bufr = src_write_buffer_right;

	if (output_size) {
		if (progdefaults.record_both_channels) {
			float buffer[2*output_size];
			memset(buffer, 0, 2 * output_size * sizeof(float));
			for (size_t i = 0; i < output_size; i++) {
				buffer[2*i] = bufl[i];//0.9 * bufl[i];
				buffer[2*i + 1] = bufr[i];//0.9 * bufr[i];
			}
			sf_write_float(file, buffer, 2 * output_size);
		} else {
			sf_write_float(file, bufl, output_size);
		}
	}

		if (must_delete) {
			delete [] bufright;
		}

	return;
}

void SoundBase::write_file(SNDFILE* file, double* bufleft, double *bufright, size_t count)
{
	float *outbuf_l = new float[count];
	float *outbuf_r = new float[count];

	for (size_t i = 0; i < count; i++) {
		outbuf_l[i] = bufleft[i];
		outbuf_r[i] = (bufright ? bufright[i] : 0);
	}
	write_file(file, outbuf_l, outbuf_r, count);
	delete [] outbuf_l;
	delete [] outbuf_r;
	return;
}

#if USE_OSS

#define MAXSC 32767.0f
#define maxsc 32000.0

SoundOSS::SoundOSS(const char *dev ) {
	device		  = dev;
	cbuff		   = 0;
	try {
		Open(O_RDONLY);
		getVersion();
		getCapabilities();
		getFormats();
		Close();
	}
	catch (const SndException& e) {
		LOG_ERROR("device %s error: %s", device.c_str(), e.what());
	}

	snd_buffer = new float [2 * SND_BUF_LEN];
	src_buffer = new float [2 * SND_BUF_LEN];
	cbuff	 = new unsigned char [4 * SND_BUF_LEN];

	memset(snd_buffer, 0, 2 * SND_BUF_LEN * sizeof(*snd_buffer));
	memset(src_buffer, 0, 2 * SND_BUF_LEN * sizeof(*src_buffer));
	memset(cbuff,	 0, 4 * SND_BUF_LEN * sizeof(*cbuff));

	tx_src_data = new SRC_DATA;
	rx_src_data = new SRC_DATA;

	int err;
	rx_src_state = src_new(progdefaults.sample_converter, 2, &err);
	if (rx_src_state == 0)
		throw SndException(src_strerror(err));

	tx_src_state = src_new(progdefaults.sample_converter, 2, &err);
	if (tx_src_state == 0)
		throw SndException(src_strerror(err));

	rx_src_data->src_ratio = 1.0/(1.0 + rxppm/1e6);
	src_set_ratio ( rx_src_state, 1.0/(1.0 + rxppm/1e6));

	tx_src_data->src_ratio = 1.0 + txppm/1e6;
	src_set_ratio ( tx_src_state, 1.0 + txppm/1e6);
}

SoundOSS::~SoundOSS()
{
	Close();

	delete tx_src_data;
	delete rx_src_data;

	if (rx_src_state)
		src_delete(rx_src_state);
	if (tx_src_state)
		src_delete(tx_src_state);

	delete [] snd_buffer;
	delete [] src_buffer;
	delete [] cbuff;
}

void SoundOSS::setfragsize()
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

int SoundOSS::Open(int md, int freq)
{
	Close();

	mode = md;
	try {
		int oflags = md;
#	   ifdef HAVE_O_CLOEXEC
			oflags = oflags | O_CLOEXEC;
#	   endif

#ifdef __FreeBSD__
/*
 * In FreeBSD sound devices e.g. /dev/dsp0.0 can only be open once
 * whereas /dev/dsp0 can be open multiple times. fldigi tries
 * to open /dev/dsp0.0 multiple times which fails. Also see man 4 sound.
 * "For specific sound card access, please instead use /dev/dsp or /dev/dsp%d"
 * This is a hack. XXX - db VA3DB
 */
		char *fixed_name;
		char *p;
		/* Look for a '.' if found, blow it away */
		fixed_name = strdup(device.c_str());
		p = strchr(fixed_name, '.');
		if(p != NULL)
			*p = '\0';
		device_fd = fl_open(fixed_name, oflags, 0);
		free(fixed_name);
#else
		device_fd = fl_open(device.c_str(), oflags, 0);
#endif
		if (device_fd == -1)
			throw SndException(errno);

		Format(AFMT_S16_LE);	// default: 16 bit little endian
		Channels(2);			//		2 channels
		Frequency(freq);
		setfragsize();
	}
	catch (...) {
		throw;
	}
	return 0;
}

void SoundOSS::Close(unsigned dir)
{
	if (device_fd == -1)
		return;
	close(device_fd);
	device_fd = -1;
}

void SoundOSS::getVersion()
{
	version = 0;

	if (ioctl(device_fd, OSS_GETVERSION, &version) == -1) {
		version = -1;
		throw SndException("OSS Version");
	}
}

void SoundOSS::getCapabilities()
{
	capability_mask = 0;
	if (ioctl(device_fd, SNDCTL_DSP_GETCAPS, &capability_mask) == -1) {
		capability_mask = 0;
		throw SndException("OSS capabilities");
	}
}

void SoundOSS::getFormats()
{
	format_mask = 0;
	if (ioctl(device_fd, SNDCTL_DSP_GETFMTS, &format_mask) == -1) {
		format_mask = 0;
		throw SndException("OSS formats");
	}
}

void SoundOSS::Format(int format)
{
	play_format = format;
	if (ioctl(device_fd, SNDCTL_DSP_SETFMT, &play_format) == -1) {
		device_fd = -1;
		formatok = false;
		throw SndException("Unsupported snd card format");
	}
	formatok = true;
}

void SoundOSS::Channels(int nuchannels)
{
	channels = nuchannels;
	if (ioctl(device_fd, SNDCTL_DSP_CHANNELS, &channels) == -1) {
		device_fd = -1;
		throw "Snd card channel request failed";
	}
}

void SoundOSS::Frequency(int frequency)
{
	sample_frequency = frequency;
	if (ioctl(device_fd, SNDCTL_DSP_SPEED, &sample_frequency) == -1) {
		device_fd = -1;
		throw SndException("Cannot set frequency");
	}
}

int SoundOSS::BufferSize( int seconds )
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

bool SoundOSS::wait_till_finished()
{
	if (ioctl(device_fd, SNDCTL_DSP_POST, (void*)1) == -1 )
		return false;
	if (ioctl(device_fd, SNDCTL_DSP_SYNC, (void*)0) == -1)
		return false; /* format (or ioctl()) not supported by device */
	return true; /* all sound has been played */
}

bool SoundOSS::reset_device()
{
	if (ioctl(device_fd, SNDCTL_DSP_RESET, 0) == -1) {
		device_fd = -1;
		return false; /* format (or ioctl()) not supported by device */
	}
	return 1; /* sounddevice has been reset */
}

size_t SoundOSS::Read(float *buffer, size_t buffersize)
{
	short int *ibuff = (short int *)cbuff;
	int numread;

	numread = read(device_fd, cbuff, buffersize * 4);
	if (numread == -1)
		throw SndException(errno);

	for (size_t i = 0; i < buffersize * 2; i++)
		src_buffer[i] = ibuff[i] / MAXSC;

	for (size_t i = 0; i < buffersize; i++)
		buffer[i] = src_buffer[2*i + (progdefaults.ReverseRxAudio ? 1 : 0)];

	if (ofCapture)
		write_file(ofCapture, buffer, NULL, buffersize);
	if (ifPlayback) {
		read_file(ifPlayback, buffer, buffersize);
		return buffersize;
	}

	if (rxppm != progdefaults.RX_corr) {
		rxppm = progdefaults.RX_corr;
		rx_src_data->src_ratio = 1.0/(1.0 + rxppm/1e6);
		src_set_ratio ( rx_src_state, 1.0/(1.0 + rxppm/1e6));
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
		throw SndException(src_strerror(numread));

	numread = rx_src_data->output_frames_gen;

	for (int i = 0; i < numread; i++)
		buffer[i] = snd_buffer[2*i + (progdefaults.sig_on_right_channel ? 1 : 0)];

	return numread;

}

size_t SoundOSS::Write(double *buf, size_t count)
{
	int retval;
	short int *wbuff;
	unsigned char *p;

	if (ofGenerate)
		write_file(ofGenerate, buf, NULL, count);

	if (PERFORM_CPS_TEST || active_modem->XMLRPC_CPS_TEST) {
		return count;
	}


	if (txppm != progdefaults.TX_corr) {
		txppm = progdefaults.TX_corr;
		tx_src_data->src_ratio = 1.0 + txppm/1e6;
		src_set_ratio ( tx_src_state, 1.0 + txppm/1e6);
	}

	if (txppm == 0) {
		wbuff = new short int[2*count];
		p = (unsigned char *)wbuff;
		for (size_t i = 0; i < count; i++) {
			wbuff[2*i] = wbuff[2*i+1] = (short int)(buf[i] * maxsc);
		}
		count *= sizeof(short int);
		retval = write(device_fd, p, 2*count);
		delete [] wbuff;
		if (retval == -1)
			throw SndException(errno);
	}
	else {
		float *inbuf;
		inbuf = new float[2*count];
		size_t bufsize;
		for (size_t i = 0; i < count; i++)
			inbuf[2*i] = inbuf[2*i+1] = buf[i];
		tx_src_data->data_in = inbuf;
		tx_src_data->input_frames = count;
		tx_src_data->data_out = src_buffer;
		tx_src_data->output_frames = SND_BUF_LEN;
		tx_src_data->end_of_input = 0;

		retval = src_process(tx_src_state, tx_src_data);
		delete [] inbuf;
		if (retval != 0)
			throw SndException(src_strerror(retval));

		bufsize = tx_src_data->output_frames_gen;
		wbuff = new short int[2*bufsize];
		p = (unsigned char *)wbuff;

		for (size_t i = 0; i < 2*bufsize; i++)
			wbuff[i] = (short int)(src_buffer[i] * maxsc);
		int num2write = bufsize * 2 * sizeof(short int);

		retval = write(device_fd, p, num2write);
		delete [] wbuff;
		if (retval != num2write)
			throw SndException(errno);
		retval = count;
	}

	return retval;
}

size_t SoundOSS::resample_write(float *buf, size_t count)
{
	double *samples = new double[2*count];
	Write(samples, count);
	delete [] samples;
	return 0;
}

size_t SoundOSS::Write_stereo(double *bufleft, double *bufright, size_t count)
{
	int retval;
	short int *wbuff;
	unsigned char *p;

	if (ofGenerate)
		write_file(ofGenerate, bufleft, bufright, count);

	if (PERFORM_CPS_TEST || active_modem->XMLRPC_CPS_TEST) {
		return count;
	}

	if (txppm != progdefaults.TX_corr) {
		txppm = progdefaults.TX_corr;
		tx_src_data->src_ratio = 1.0 + txppm/1e6;
		src_set_ratio ( tx_src_state, 1.0 + txppm/1e6);
	}

	if (txppm == 0) {
		wbuff = new short int[2*count];
		p = (unsigned char *)wbuff;
		for (size_t i = 0; i < count; i++) {
			if (progdefaults.ReverseAudio) {
				wbuff[2*i+1] = (short int)(bufleft[i] * maxsc);
				wbuff[2*i] = (short int)(bufright[i] * maxsc);
			} else {
				wbuff[2*i] = (short int)(bufleft[i] * maxsc);
				wbuff[2*i+1] = (short int)(bufright[i] * maxsc);
			}
		}
		count *= sizeof(short int);
		retval = write(device_fd, p, 2*count);
		delete [] wbuff;
		if (retval == -1)
			throw SndException(errno);
	}
	else {
		float *inbuf;
		inbuf = new float[2*count];
		size_t bufsize;
		for (size_t i = 0; i < count; i++) {
			if (progdefaults.ReverseAudio) {
				inbuf[2*i+1] = bufleft[i];
				inbuf[2*i] = bufright[i];
			} else {
				inbuf[2*i] = bufleft[i];
				inbuf[2*i+1] = bufright[i];
			}
		}
		tx_src_data->data_in = inbuf;
		tx_src_data->input_frames = count;
		tx_src_data->data_out = src_buffer;
		tx_src_data->output_frames = SND_BUF_LEN;
		tx_src_data->end_of_input = 0;

		retval = src_process(tx_src_state, tx_src_data);
		delete [] inbuf;
		if (retval != 0)
			throw SndException(src_strerror(retval));

		bufsize = tx_src_data->output_frames_gen;
		wbuff = new short int[2*bufsize];
		p = (unsigned char *)wbuff;

		for (size_t i = 0; i < 2*bufsize; i++)
			wbuff[i] = (short int)(src_buffer[i] * maxsc);

		int num2write = bufsize * 2 * sizeof(short int);
		retval = write(device_fd, p, num2write);
		delete [] wbuff;
		if (retval != num2write)
			throw SndException(errno);
		retval = count;
	}

	return retval;
}
#endif // USE_OSS


#if USE_PORTAUDIO

bool SoundPort::pa_init = false;
std::vector<const PaDeviceInfo*> SoundPort::devs;

static ostringstream device_text[2];
static pthread_mutex_t device_mutex = PTHREAD_MUTEX_INITIALIZER;

map<string, vector<double> > supported_rates[2];

void SoundPort::initialize(void)
{
	if (pa_init)
		return;

	init_hostapi_ext();

	int err;
	if ((err = Pa_Initialize()) != paNoError) {
#if __WIN32__
		LOG_PERROR(win_error_string(err).c_str());
#else
		LOG_PERROR("Portaudio Initialize error");
#endif
		throw SndPortException(err);
	}
	pa_init = true;

	PaDeviceIndex ndev;
	if ((ndev = Pa_GetDeviceCount()) < 0) {
		LOG_PERROR("Portaudio device count error");
		throw SndPortException(ndev);
	}
	if (ndev == 0) {
		LOG_PERROR("Portaudio, no audio devices");
		throw SndException(ENODEV, "No available audio devices");
	}

	devs.reserve(ndev);
	for (PaDeviceIndex i = 0; i < ndev; i++)
		devs.push_back(Pa_GetDeviceInfo(i));
}
void SoundPort::terminate(void)
{
	if (!pa_init)
		return;
	static_cast<void>(Pa_Terminate());
	pa_init = false;
	devs.clear();
	supported_rates[0].clear();
	supported_rates[1].clear();
}
const std::vector<const PaDeviceInfo*>& SoundPort::devices(void)
{
		return devs;
}
void SoundPort::devices_info(string& in, string& out)
{
	guard_lock devices_lock(&device_mutex);
	in = device_text[0].str();
	out = device_text[1].str();
}

const vector<double>& SoundPort::get_supported_rates(const string& name, unsigned dir)
{
	return supported_rates[dir][name];
}


SoundPort::SoundPort(const char *in_dev, const char *out_dev)
{
	req_sample_rate = 0;

	sd[0].device = in_dev;
	sd[0].params.channelCount = 2; // init_stream can change this to 0 or 1
	sd[0].stream = 0;
	sd[0].frames_per_buffer = paFramesPerBufferUnspecified;
	sd[0].dev_sample_rate = 0;
	sd[0].state = spa_continue;
	sd[0].rb = 0;
	sd[0].advance = 0;

	sd[1].device = out_dev;
	sd[1].params.channelCount = 2;
	sd[1].stream = 0;
	sd[1].frames_per_buffer = paFramesPerBufferUnspecified;
	sd[1].dev_sample_rate = 0;
	sd[1].state = spa_continue;
	sd[1].rb = 0;
	sd[1].advance = 0;

	sem_t** sems[] = { &sd[0].rwsem, &sd[1].rwsem };
#if USE_NAMED_SEMAPHORES
	char sname[32];
#endif
	for (size_t i = 0; i < sizeof(sems)/sizeof(*sems); i++) {
#if USE_NAMED_SEMAPHORES
		unsigned int un = i;
		snprintf(sname, sizeof(sname), "%u-%u-%s", un, getpid(), PACKAGE_TARNAME);
		if ((*sems[i] = sem_open(sname, O_CREAT | O_EXCL, 0600, 0)) == (sem_t*)SEM_FAILED) {
			pa_perror(errno, sname);
			throw SndException(errno);
		}
#  if HAVE_SEM_UNLINK
		if (sem_unlink(sname) == -1) {
			pa_perror(errno, sname);
			throw SndException(errno);
		}
#  endif
#else
		*sems[i] = new sem_t;
		if (sem_init(*sems[i], 0, 0) == -1) {
#if __WIN32__
		int err = GetLastError();
		LOG_PERROR(win_error_string(err).c_str());
#endif
			pa_perror(errno, "sem_init error");
			throw SndException(errno);
		}
#endif // USE_NAMED_SEMAPHORES
	}

	for (size_t i = 0; i < 2; i++) {
		sd[i].cmutex = new pthread_mutex_t;
		pthread_mutex_init(sd[i].cmutex, NULL);
		sd[i].ccond = new pthread_cond_t;
		pthread_cond_init(sd[i].ccond, NULL);
	}

	tx_src_data = new SRC_DATA;
	src_buffer = new float[sd[1].params.channelCount * SND_BUF_LEN];
	fbuf = new float[2 * SND_BUF_LEN];

	memset(src_buffer, 0, sd[1].params.channelCount * SND_BUF_LEN * sizeof(*src_buffer));
	memset(fbuf, 0, 2 * SND_BUF_LEN * sizeof(*fbuf));
}

SoundPort::~SoundPort()
{
	Close();

	sem_t* sems[] = { sd[0].rwsem, sd[1].rwsem };
	for (size_t i = 0; i < sizeof(sems)/sizeof(*sems); i++) {
#if USE_NAMED_SEMAPHORES
		if (sem_close(sems[i]) == -1)
			LOG_PERROR("sem_close");
#else
		if (sem_destroy(sems[i]) == -1)
			LOG_PERROR("sem_destroy");
		delete sems[i];
#endif
	}

	for (size_t i = 0; i < 2; i++) {
		if (pthread_mutex_destroy(sd[i].cmutex) == -1) {
			pa_perror(errno, "pthread mutex destroy");
			terminate(); //throw SndException(errno);
		}
		delete sd[i].cmutex;
		if (pthread_cond_destroy(sd[i].ccond) == -1) {
			pa_perror(errno, "pthread cond destroy");
			terminate(); //throw SndException(errno);
		}
		delete sd[i].ccond;
	}

	delete sd[0].rb;
	delete sd[1].rb;

	if (rx_src_state)
		src_delete(rx_src_state);
	if (tx_src_state)
		src_delete(tx_src_state);

	delete tx_src_data;
	delete [] src_buffer;
	delete [] fbuf;
}

int SoundPort::Open(int mode, int freq)
{
	int old_sample_rate = (int)req_sample_rate;
	req_sample_rate = sample_frequency = freq;

	// do we need to (re)initialise the streams?
	int ret = 1;
	int sr[2] = { progdefaults.in_sample_rate, progdefaults.out_sample_rate };

	// initialize stream if it is a JACK device, regardless of mode
	device_iterator idev;
	int device_type = 0;
	if (mode == O_WRONLY && (idev = name_to_device(sd[0].device, 0)) != devs.end() &&
		(device_type = Pa_GetHostApiInfo((*idev)->hostApi)->type) == paJACK)
		mode = O_RDWR;
	if (mode == O_RDONLY && (idev = name_to_device(sd[1].device, 1)) != devs.end() &&
		(device_type = Pa_GetHostApiInfo((*idev)->hostApi)->type) == paJACK)
		mode = O_RDWR;

	size_t start = (mode == O_RDONLY || mode == O_RDWR) ? 0 : 1,
		end = (mode == O_WRONLY || mode == O_RDWR) ? 1 : 0;
	for (size_t i = start; i <= end; i++) {
		if ( !(stream_active(i) && (Pa_GetHostApiInfo((*sd[i].idev)->hostApi)->type == paJACK ||
						old_sample_rate == freq ||
						sr[i] != SAMPLE_RATE_AUTO)) ) {
			Close(i);
			try {
				init_stream(i);
			} catch (const SndException& e) {
				LOG_ERROR("SndException: %s", e.what());
				throw SndPortException(paDeviceUnavailable);
			}
			src_data_reset(i);

			// reset the semaphore
			while (sem_trywait(sd[i].rwsem) == 0);
			if (errno && errno != EAGAIN) {
				pa_perror(errno, "open failed");
				throw SndException(errno);
			}
			start_stream(i);

			ret = 0;
		}
		else {
			pause_stream(i);
			src_data_reset(i);
			sd[i].state = spa_continue;
		}
	}

	static char pa_open_str[500];
	snprintf(pa_open_str, sizeof(pa_open_str),
"\
Port Audio open mode = %s\n\
device type = %s\n\
device name = %s\n\
# input channels %d\n\
# output channels %d",
		mode == O_WRONLY ? "Write" : 
		mode == O_RDONLY ? "Read" : 
		mode == O_RDWR ? "Read/Write" : "unknown",
		device_type == 0 ? "paInDevelopment" :
		device_type == 1 ? "paDirectSound" :
		device_type == 2 ? "paMME" :
		device_type == 3 ? "paASIO" :
		device_type == 4 ? "paSoundManager" :
		device_type == 5 ? "paCoreAudio" :
		device_type == 7 ? "paOSS" :
		device_type == 8 ? "paALSA" :
		device_type == 9 ? "paAL" :
		device_type == 10 ? "paBeOS" :
		device_type == 11 ? "paWDMKS" :
		device_type == 12 ? "paJACK" :
		device_type == 13 ? "paWASAPI" :
		device_type == 14 ? "paAudioScienceHPI" : "unknown",
		mode == O_WRONLY ? sd[1].device.c_str() :
		mode == O_RDONLY ? sd[0].device.c_str() : "unknown",
		sd[0].params.channelCount,
		sd[1].params.channelCount );
	LOG_INFO( "%s", pa_open_str);

	return ret;
}

void SoundPort::pause_stream(unsigned dir)
{
	if (sd[dir].stream == 0 || !stream_active(dir))
		return;

	pthread_mutex_lock(sd[dir].cmutex);
		sd[dir].state = spa_pause;
	if (pthread_cond_timedwait_rel(sd[dir].ccond, sd[dir].cmutex, 5.0) == -1 && errno == ETIMEDOUT)
		LOG_ERROR("stream %u wedged", dir);
	pthread_mutex_unlock(sd[dir].cmutex);
}

void SoundPort::Close(unsigned dir)
{
	unsigned start, end;
	if (dir == UINT_MAX) {
		start = 0;
		end = 1;
	}
	else
		start = end = dir;

	for (unsigned i = start; i <= end; i++) {
		if (!stream_active(i))
			continue;

		pthread_mutex_lock(sd[i].cmutex);
		sd[i].state = spa_complete;
		// first wait for buffers to be drained and for the
		// stop callback to signal us that the stream has
		// been stopped
		if (pthread_cond_timedwait_rel(sd[i].ccond, sd[i].cmutex, 5.0) == -1 &&
			errno == ETIMEDOUT)
			LOG_ERROR("stream %u wedged", i);
		pthread_mutex_unlock(sd[i].cmutex);
		sd[i].state = spa_continue;

		int err;
		if ((err = Pa_CloseStream(sd[i].stream)) != paNoError)
			pa_perror(err, "Pa_CloseStream");

		sd[i].stream = 0;
	}
}

void SoundPort::Abort(unsigned dir)
{
	unsigned start, end;
	if (dir == UINT_MAX) {
		start = 0;
		end = 1;
	}
	else
		start = end = dir;

	int err;
	for (unsigned i = start; i <= end; i++) {
		if (!stream_active(i))
			continue;
		if ((err = Pa_AbortStream(sd[i].stream)) != paNoError)
#if __WIN32__
		LOG_PERROR(win_error_string(err).c_str());
#endif
			pa_perror(err, "Pa_AbortStream");
		sd[i].stream = 0;
	}
}


#define WAIT_FOR_COND(cond, s, t) \
do { \
	while (!(cond)) { \
		if (sem_timedwait_rel(s, t) == -1) { \
			if (errno == ETIMEDOUT) { \
				timeout = true;	 \
				break; \
			} else if (errno == EINTR) { \
				continue; \
			} \
			LOG_PERROR("sem_timedwait"); \
			throw SndException(errno); \
		} \
	} \
} while (0)


size_t SoundPort::Read(float *buf, size_t count)
{
	if (ifPlayback) {
		read_file(ifPlayback, buf, count);
		if (!ofCapture) {
			if (!bHighSpeed)
				MilliSleep((long)ceil((1e3 * count) / req_sample_rate));
			return count;
		}
	}

	if (rxppm != progdefaults.RX_corr)
		rxppm = progdefaults.RX_corr;

	sd[0].src_ratio = req_sample_rate / (sd[0].dev_sample_rate * (1.0 + rxppm / 1e6));
	src_set_ratio(rx_src_state, sd[0].src_ratio);

	size_t maxframes = (size_t)floor(sd[0].rb->length() * sd[0].src_ratio / sd[0].params.channelCount);

	if (unlikely(count > maxframes)) {
		size_t n = 0;
#define PA_TIMEOUT_TRIES 10
		int pa_timeout = PA_TIMEOUT_TRIES;
// possible to lock up in this while block if the Read(...) fails
		while (count > maxframes) {
			n += Read(buf, maxframes);
			buf += maxframes * sd[0].params.channelCount;
			count -= n;//maxframes;
			pa_timeout--;
			if (pa_timeout == 0) {
#if __WIN32__
				int err = GetLastError();
				LOG_PERROR(win_error_string(err).c_str());
#endif
				pa_perror(1, "Portaudio read error #1");
				throw SndException("Portaudio read error 1");
			}
		}
		if (count > 0)
			n += Read(buf, count);
		return n;
	}

	float* rbuf = fbuf;
	if (req_sample_rate != sd[0].dev_sample_rate || rxppm != 0) {
		long r;
		size_t n = 0;
		sd[0].blocksize = SCBLOCKSIZE;
		while (n < count) {
			if  ((r = src_callback_read(rx_src_state, sd[0].src_ratio,
							count - n, rbuf + n * sd[0].params.channelCount)) == 0) {
				pa_perror(2, "Portaudio read error #2");
				throw SndException("Portaudio read error 2");
			}
			n += r;
		}
	}
	else {
		bool timeout = false;
		WAIT_FOR_COND( (sd[0].rb->read_space() >= count * sd[0].params.channelCount / sd[0].src_ratio), sd[0].rwsem,
				   (MAX(1.0, 2 * count * sd[0].params.channelCount / sd->dev_sample_rate)) );
		if (timeout) {
			pa_perror(3, "Portaudio read error #3");
			throw SndException("Portaudio read error 3");
		}
		ringbuffer<float>::vector_type vec[2];
		sd[0].rb->get_rv(vec);
		if (vec[0].len >= count * sd[0].params.channelCount) {
			rbuf = vec[0].buf;
			sd[0].advance = vec[0].len;
		}
		else
			sd[0].rb->read(rbuf, count * sd[0].params.channelCount);
	}
	if (sd[0].advance) {
		sd[0].rb->read_advance(sd[0].advance);
		sd[0].advance = 0;
	}

	// transfer active input channel; left == 0, right == 1
	size_t n;
	for (size_t i = 0; i < count; i++) {
		n = sd[0].params.channelCount * i;
		if (sd[0].params.channelCount == 2)
			n += progdefaults.ReverseRxAudio;
		buf[i] = rbuf[n];
	}

	if (ofCapture)
		write_file(ofCapture, buf, NULL, count);

	return count;
}

size_t SoundPort::Write(double *buf, size_t count)
{
	if (ofGenerate)
		write_file(ofGenerate, buf, NULL, count);

	if (PERFORM_CPS_TEST || active_modem->XMLRPC_CPS_TEST) {
		return count;
	}

// copy input to both channels if right channel enabled
	for (size_t i = 0; i < count; i++)
		if (progdefaults.sig_on_right_channel)
			fbuf[sd[1].params.channelCount * i] = fbuf[sd[1].params.channelCount * i + 1] = buf[i];
		else if (progdefaults.ReverseAudio) {
				fbuf[sd[1].params.channelCount * i + 1] = buf[i];
				fbuf[sd[1].params.channelCount * i] = 0;
		} else {
				fbuf[sd[1].params.channelCount * i] = buf[i];
				fbuf[sd[1].params.channelCount * i + 1] = 0;
		}

	return resample_write(fbuf, count);
}

size_t SoundPort::Write_stereo(double *bufleft, double *bufright, size_t count)
{
	if (sd[1].params.channelCount != 2)
		return Write(bufleft, count);

	if (ofGenerate)
		write_file(ofCapture, bufleft, bufright, count);

	if (PERFORM_CPS_TEST || active_modem->XMLRPC_CPS_TEST) {
		return count;
	}

// interleave into fbuf
	for (size_t i = 0; i < count; i++) {
		if (progdefaults.ReverseAudio) {
			fbuf[sd[1].params.channelCount * i + 1] = bufleft[i];
			fbuf[sd[1].params.channelCount * i] = bufright[i];
		} else {
			fbuf[sd[1].params.channelCount * i] = bufleft[i];
			fbuf[sd[1].params.channelCount * i + 1] = bufright[i];
		}
	}

	return resample_write(fbuf, count);
}


size_t SoundPort::resample_write(float* buf, size_t count)
{
	size_t maxframes = (size_t)floor((sd[1].rb->length() / sd[1].params.channelCount) / tx_src_data->src_ratio);
	maxframes /= 2; // don't fill the buffer

	if (unlikely(count > maxframes)) {
		size_t n = 0;
#define PA_TIMEOUT_TRIES 10
		int pa_timeout = PA_TIMEOUT_TRIES;
// possible to lock up in this while block if the resample_write(...) fails
		while (count > maxframes) {
			n += resample_write(buf, maxframes);
			buf += sd[1].params.channelCount * maxframes;
			count -= maxframes;
			pa_timeout--;
			if (pa_timeout == 0) {
				pa_perror(1, "Portaudio write error #1");
				throw SndException("Portaudio write error 1");
			}
		}
		if (count > 0)
			n += resample_write(buf, count);
		return n;
	}

	assert(count * sd[1].params.channelCount * tx_src_data->src_ratio <= sd[1].rb->length());

	ringbuffer<float>::vector_type vec[2];
	sd[1].rb->get_wv(vec);
	float* wbuf = buf;
	if (req_sample_rate != sd[1].dev_sample_rate || progdefaults.TX_corr != 0) {
		if (vec[0].len >= sd[1].params.channelCount * (size_t)ceil(count * tx_src_data->src_ratio))
			wbuf = vec[0].buf; // direct write in the rb
		else
			wbuf = src_buffer;

		if (txppm != progdefaults.TX_corr)
			txppm = progdefaults.TX_corr;

		tx_src_data->src_ratio = sd[1].dev_sample_rate * (1.0 + txppm / 1e6) / req_sample_rate;
		src_set_ratio(tx_src_state, tx_src_data->src_ratio);

		tx_src_data->data_in = buf;
		tx_src_data->input_frames = count;
		tx_src_data->data_out = wbuf;
		tx_src_data->output_frames = (wbuf == vec[0].buf ? vec[0].len : SND_BUF_LEN);
		tx_src_data->end_of_input = 0;
		int r;
		if ((r = src_process(tx_src_state, tx_src_data)) != 0) {
			pa_perror(2, "Portaudio write error #2");
			throw SndException("Portaudio write error 2");
		}
		if (tx_src_data->output_frames_gen == 0) // input was too small
			return count;

		count = tx_src_data->output_frames_gen;
		if (wbuf == vec[0].buf) { // advance write pointer and return
			sd[1].rb->write_advance(sd[1].params.channelCount * count);
			sem_trywait(sd[1].rwsem);
			return count;
		}
	}

	// if we didn't do a direct resample into the rb, or didn't resample at all,
	// we must now copy buf into the ringbuffer, possibly waiting for space first
	bool timeout = false;
	WAIT_FOR_COND( (sd[1].rb->write_space() >= sd[1].params.channelCount * count), sd[1].rwsem,
				   (MAX(1.0, 2 * sd[1].params.channelCount * count / sd[1].dev_sample_rate)) );
	if (timeout) {
		pa_perror(3, "Portaudio write error #3");
		throw SndException("Portaudio write error 3");
	}

	sd[1].rb->write(wbuf, sd[1].params.channelCount * count);

	return count;
}

void SoundPort::flush(unsigned dir)
{
	unsigned start, end;
	if (dir == UINT_MAX) {
		start = 0;
		end = 1;
	}
	else
		start = end = dir;

	for (unsigned i = start; i <= end; i++) {
		if (!stream_active(i))
			continue;

		pthread_mutex_lock(sd[i].cmutex);
		sd[i].state = spa_drain;
		if (pthread_cond_timedwait_rel(sd[i].ccond, sd[i].cmutex, 5.0) == -1
			&& errno == ETIMEDOUT)
			LOG_ERROR("stream %u wedged", i);
		pthread_mutex_unlock(sd[i].cmutex);
		sd[i].state = spa_continue;
	}
}

void SoundPort::src_data_reset(unsigned dir)
{
	size_t rbsize;

	int err;
	if (dir == 0) {
		if (rx_src_state)
			src_delete(rx_src_state);
		rx_src_state = src_callback_new(src_read_cb, progdefaults.sample_converter,
										sd[0].params.channelCount, &err, &sd[0]);
		if (!rx_src_state) {
			pa_perror(err, src_strerror(err));
			throw SndException(src_strerror(err));
		}
		sd[0].src_ratio = req_sample_rate / (sd[0].dev_sample_rate * (1.0 + rxppm / 1e6));
	}
	else if (dir == 1) {
		if (tx_src_state)
			src_delete(tx_src_state);
		tx_src_state = src_new(progdefaults.sample_converter, sd[1].params.channelCount, &err);
		if (!tx_src_state) {
			pa_perror(err, src_strerror(err));
			throw SndException(src_strerror(err));
		}
		tx_src_data->src_ratio = sd[1].dev_sample_rate * (1.0 + txppm / 1e6) / req_sample_rate;
	}

	rbsize = 2 * MAX(ceil2(
					(unsigned)(2 * sd[dir].params.channelCount * SCBLOCKSIZE *
					MAX(req_sample_rate, sd[dir].dev_sample_rate) /
					MIN(req_sample_rate, sd[dir].dev_sample_rate))),
					8192);
	stringstream info;
	info << "rbsize = " << rbsize;
	LOG_VERBOSE("%s", info.str().c_str());
	if (sd[dir].rb) delete sd[dir].rb;
	sd[dir].rb = new ringbuffer<float>(rbsize);
}

long SoundPort::src_read_cb(void* arg, float** data)
{
	struct stream_data* sd = reinterpret_cast<stream_data*>(arg);

	// advance read pointer for previous read
	if (sd->advance) {
		sd->rb->read_advance(sd->advance);
		sd->advance = 0;
	}

	// wait for data
	bool timeout = false;
	WAIT_FOR_COND( (sd->rb->read_space() >= (size_t)sd[0].params.channelCount * SCBLOCKSIZE), sd->rwsem,
				   (MAX(1.0, 2 * sd[0].params.channelCount * SCBLOCKSIZE / sd->dev_sample_rate)) );
	if (timeout) {
		*data = 0;
		return 0;
	}

	ringbuffer<float>::vector_type vec[2];
	sd->rb->get_rv(vec);

	*data = vec[0].buf;
	sd->advance = vec[0].len;

	return vec[0].len / sd[0].params.channelCount;
}

SoundPort::device_iterator SoundPort::name_to_device(std::string &name, unsigned dir)
{
	device_iterator i;
	for (i = devs.begin(); i != devs.end(); ++i)
		if (name == (*i)->name && (dir ? (*i)->maxOutputChannels : (*i)->maxInputChannels))
			break;
	return i;
}

void SoundPort::init_stream(unsigned dir)
{
	const char* dir_str[2] = { "input", "output" };
	PaDeviceIndex idx = paNoDevice;

	LOG_DEBUG("looking for device \"%s\"", sd[dir].device.c_str());

	if ((sd[dir].idev = name_to_device(sd[dir].device, dir)) != devs.end())
		idx = sd[dir].idev - devs.begin();
	if (idx == paNoDevice) { // no match
		LOG_ERROR("Could not find device \"%s\", using default device", sd[dir].device.c_str());
		PaDeviceIndex def = (dir == 0 ? Pa_GetDefaultInputDevice() : Pa_GetDefaultOutputDevice());
		if (def == paNoDevice) {
			pa_perror(paDeviceUnavailable, "Portaudio device unavailable");
			throw SndPortException(paDeviceUnavailable);
		}
		sd[dir].idev = devs.begin() + def;
		idx = def;
	}
	else if (sd[dir].idev == devs.end()) // if we only found a near-match point the idev iterator to it
		sd[dir].idev = devs.begin() + idx;

	const PaHostApiInfo* host_api = Pa_GetHostApiInfo((*sd[dir].idev)->hostApi);
	int max_channels = dir ? (*sd[dir].idev)->maxOutputChannels :
		(*sd[dir].idev)->maxInputChannels;
	if ((host_api->type == paALSA || host_api->type == paOSS) && max_channels == 0) {
		pa_perror(EBUSY, "Portaudio device busy");
		throw SndException(EBUSY);
	}

	if (dir == 0) {
		sd[0].params.device = idx;
		sd[0].params.sampleFormat = paFloat32;
		sd[0].params.suggestedLatency = (*sd[dir].idev)->defaultHighInputLatency;
		sd[0].params.hostApiSpecificStreamInfo = NULL;
		if (max_channels < 2)
				sd[0].params.channelCount = max_channels;
		if (max_channels == 0) {
			pa_perror(EBUSY, "Portaudio device cannot open for read");
			throw SndException(EBUSY);
		}
	}
	else {
		sd[1].params.device = idx;
		sd[1].params.sampleFormat = paFloat32;
		if (host_api->type == paMME)
						sd[1].params.suggestedLatency = (*sd[dir].idev)->defaultLowOutputLatency;
				else
						sd[1].params.suggestedLatency = (*sd[dir].idev)->defaultHighOutputLatency;
		sd[1].params.hostApiSpecificStreamInfo = NULL;
		if (max_channels < 2)
			sd[1].params.channelCount = max_channels;
	}

	const vector<double>& rates = supported_rates[dir][(*sd[dir].idev)->name];
	if (rates.size() <= 1)
		probe_supported_rates(sd[dir].idev);
	ostringstream ss;
	if (rates.size() > 1)
		copy(rates.begin() + 1, rates.end(), ostream_iterator<double>(ss, " "));
	else
		ss << "Unknown";

	{
	guard_lock devices_lock(&device_mutex);

	device_text[dir].str("");
		device_text[dir]
		<< "index: " << idx
		<< "\nname: " << (*sd[dir].idev)->name
		<< "\nhost API: " << host_api->name
		<< "\nmax input channels: " << (*sd[dir].idev)->maxInputChannels
		<< "\nmax output channels: " << (*sd[dir].idev)->maxOutputChannels
		<< "\ndefault sample rate: " << (*sd[dir].idev)->defaultSampleRate
		<< "\nsupported sample rates: " << ss.str()
		<< boolalpha
		<< "\ninput only: " << ((*sd[dir].idev)->maxOutputChannels == 0)
		<< "\noutput only: " << ((*sd[dir].idev)->maxInputChannels == 0)
		<< "\nfull duplex: " << full_duplex_device(*sd[dir].idev)
		<< "\nsystem default input: " << (idx == Pa_GetDefaultInputDevice())
		<< "\nsystem default output: " << (idx == Pa_GetDefaultOutputDevice())
		<< "\nhost API default input: " << (idx == host_api->defaultInputDevice)
		<< "\nhost API default output: " << (idx == host_api->defaultOutputDevice)
		<< "\ndefault low input latency: " << (*sd[dir].idev)->defaultLowInputLatency
		<< "\ndefault high input latency: " << (*sd[dir].idev)->defaultHighInputLatency
		<< "\ndefault low output latency: " << (*sd[dir].idev)->defaultLowOutputLatency
		<< "\ndefault high output latency: " << (*sd[dir].idev)->defaultHighOutputLatency
		<< "\n";
	}

	LOG_VERBOSE("using %s (%d ch) device \"%s\":\n%s", dir_str[dir], sd[dir].params.channelCount,
		sd[dir].device.c_str(), device_text[dir].str().c_str());

		sd[dir].dev_sample_rate = find_srate(dir);
		if (sd[dir].dev_sample_rate != req_sample_rate)
		LOG_DEBUG("%s: resampling %f <=> %f", dir_str[dir],
			  sd[dir].dev_sample_rate, req_sample_rate);

		if (progdefaults.PortFramesPerBuffer > 0) {
				sd[dir].frames_per_buffer = progdefaults.PortFramesPerBuffer;
		LOG_DEBUG("%s: frames_per_buffer=%u", dir_str[dir], sd[dir].frames_per_buffer);
	}
}

void SoundPort::start_stream(unsigned dir)
{
	int err;

	PaStreamParameters* sp[2];
	sp[dir] = &sd[dir].params;
	sp[!dir] = NULL;

LOG_INFO("\n\
open pa stream for %s:\n\
  samplerate    : %.0f\n\
  device number : %d\n\
  # channels    : %d\n\
  latency       : %f\n\
  sample Format : paFloat32",
(dir == 1 ? "write" : "read"),
sd[dir].dev_sample_rate,
sp[dir]->device,
sp[dir]->channelCount,
sp[dir]->suggestedLatency);

	err = Pa_OpenStream(&sd[dir].stream, sp[0], sp[1],
			sd[dir].dev_sample_rate, sd[dir].frames_per_buffer,
			paNoFlag,
			stream_process, &sd[dir]);
	if (err != paNoError) {
		throw SndPortException(err);
	}

	if ((err = Pa_SetStreamFinishedCallback(sd[dir].stream, stream_stopped)) != paNoError)
		throw SndPortException(err);

	if ((err = Pa_StartStream(sd[dir].stream)) != paNoError) {
		pa_perror(err, "Portaudio stream start stream error");
		Close();
		throw SndPortException(err);
	}
}


int SoundPort::stream_process(
			const void* in, void* out, unsigned long nframes,
			const PaStreamCallbackTimeInfo *time_info,
			PaStreamCallbackFlags flags, void* data)
{
	struct stream_data* sd = reinterpret_cast<struct stream_data*>(data);

#ifndef NDEBUG
	struct {
		PaStreamCallbackFlags f;
		const char* s;
	} fa[] = { { paInputUnderflow, "Input underflow" },
			   { paInputOverflow,  "Input overflow" },
			   { paOutputUnderflow, "Output underflow" },
			   { paOutputOverflow, "Output overflow" }
			};
	for (size_t i = 0; i < sizeof(fa)/sizeof(*fa); i++)
		if (flags & fa[i].f)
			LOG_DEBUG("%s", fa[i].s);
#endif

	if (unlikely(sd->state == spa_abort || sd->state == spa_complete)) // finished
		return sd->state;

	if (in) {
		switch (sd->state) {
			case spa_continue: // write into the rb, post rwsem if we wrote anything
				if (sd->rb->write(reinterpret_cast<const float*>(in), sd->params.channelCount * nframes))
					sem_post(sd->rwsem);
				break;
			case spa_drain: case spa_pause: // signal the cv
				pthread_mutex_lock(sd->cmutex);
				pthread_cond_signal(sd->ccond);
				pthread_mutex_unlock(sd->cmutex);
		}
	}
	else if (out) {
		float* outf = reinterpret_cast<float*>(out);
		// if we are paused just pretend that the rb was empty
		size_t nread = (sd->state == spa_pause) ? 0 : sd->rb->read(outf, sd->params.channelCount * nframes);
		memset(outf + nread, 0, (sd->params.channelCount * nframes - nread) * sizeof(float)); // fill rest with 0

		switch (sd->state) {
			case spa_continue: // post rwsem if we read anything
				if (nread > 0)
					sem_post(sd->rwsem);
				break;
			case spa_drain: // signal the cv when we have emptied the buffer
				if (nread > 0)
					break;
			// else fall through
			case spa_pause:
				pthread_mutex_lock(sd->cmutex);
				pthread_cond_signal(sd->ccond);
				pthread_mutex_unlock(sd->cmutex);
				break;
		}
	}

	return paContinue;
}

void SoundPort::stream_stopped(void* data)
{
	struct stream_data* sd = reinterpret_cast<struct stream_data*>(data);

	if (sd->rb)
		sd->rb->reset();
	pthread_mutex_lock(sd->cmutex);
	pthread_cond_signal(sd->ccond);
	pthread_mutex_unlock(sd->cmutex);
}


bool SoundPort::stream_active(unsigned dir)
{
	if (!sd[dir].stream)
		return false;

	int err;
	if ((err = Pa_IsStreamActive(sd[dir].stream)) < 0) {
		pa_perror(err, "Portaudio stream active error");
		throw SndPortException(err);
	}
	return err == 1;
}

bool SoundPort::full_duplex_device(const PaDeviceInfo* dev)
{
	return dev->maxInputChannels > 0 && dev->maxOutputChannels > 0;
}

bool SoundPort::must_close(int dir)
{
	return Pa_GetHostApiInfo((*sd[dir].idev)->hostApi)->type != paJACK;
}

double SoundPort::find_srate(unsigned dir)
{
	int sr = (dir == 0 ? progdefaults.in_sample_rate : progdefaults.out_sample_rate);
		switch (sr) {
		case SAMPLE_RATE_UNSET: case SAMPLE_RATE_AUTO:
				break;
		case SAMPLE_RATE_NATIVE:
				return (*sd[dir].idev)->defaultSampleRate;
		default:
				return sr;
		}

	const vector<double>& rates = supported_rates[dir][(*sd[dir].idev)->name];
	for (vector<double>::const_iterator i = rates.begin(); i != rates.end(); i++)
		if (req_sample_rate == *i || (*sd[dir].idev)->defaultSampleRate == *i)
			return *i;

	pa_perror(0, "Portaudio - no supported sample rate found");
	throw SndException("No supported sample rate found");
}

void SoundPort::probe_supported_rates(const device_iterator& idev)
{
	PaStreamParameters params[2];
	params[0].device = params[1].device = idev - devs.begin();
	params[0].channelCount = 2;
	params[1].channelCount = 2;
	params[0].sampleFormat = params[1].sampleFormat = paFloat32;
	params[0].suggestedLatency = (*idev)->defaultHighInputLatency;
	params[1].suggestedLatency = (*idev)->defaultHighOutputLatency;
	params[0].hostApiSpecificStreamInfo = params[1].hostApiSpecificStreamInfo = NULL;

	supported_rates[0][(*idev)->name].clear();
	supported_rates[1][(*idev)->name].clear();
	supported_rates[0][(*idev)->name].push_back((*idev)->defaultSampleRate);
	supported_rates[1][(*idev)->name].push_back((*idev)->defaultSampleRate);
	extern double std_sample_rates[];
	for (const double* r = std_sample_rates; *r > 0.0; r++) {
		if (Pa_IsFormatSupported(&params[0], NULL, *r) == paFormatIsSupported)
			supported_rates[0][(*idev)->name].push_back(*r);
		if (Pa_IsFormatSupported(NULL, &params[1], *r) == paFormatIsSupported)
			supported_rates[1][(*idev)->name].push_back(*r);
	}
}

void SoundPort::pa_perror(int err, const char* str)
{
	if (str)
	LOG_ERROR("%s: %s", str, Pa_GetErrorText(err));

	if (err == paUnanticipatedHostError) {
		const PaHostErrorInfo* hosterr = Pa_GetLastHostErrorInfo();
		PaHostApiIndex i = Pa_HostApiTypeIdToHostApiIndex(hosterr->hostApiType);

		if (i < 0) { // PA failed without setting its "last host error" info. Sigh...
			LOG_ERROR("Host API error info not available");
			if ( ((sd[0].stream && Pa_GetHostApiInfo((*sd[0].idev)->hostApi)->type == paOSS) ||
				(sd[1].stream && Pa_GetHostApiInfo((*sd[1].idev)->hostApi)->type == paOSS)) &&
				errno )
				LOG_ERROR("Possible OSS error %d: %s", errno, strerror(errno));
		}
		else
			LOG_ERROR("%s error %ld: %s", Pa_GetHostApiInfo(i)->name,
				  hosterr->errorCode, hosterr->errorText);
	}
}

void SoundPort::init_hostapi_ext(void)
{
#if HAVE_DLOPEN && !defined(__WOE32__)
	void* handle = dlopen(NULL, RTLD_LAZY);
	if (!handle)
		return;

	PaError (*set_jack_client_name)(const char*);
	const char* err = dlerror();
	set_jack_client_name = (PaError (*)(const char*))dlsym(handle, "PaJack_SetClientName");
	if (!(err = dlerror()))
		set_jack_client_name(main_window_title.c_str());
#  ifndef NDEBUG
		else
		LOG_VERBOSE("dlsym(PaJack_SetClientName) error: %s", err);
#  endif
#endif
}

#endif // USE_PORTAUDIO


#if USE_PULSEAUDIO

SoundPulse::SoundPulse(const char *dev)
{
	sd[0].stream = 0;
	sd[0].stream_params.channels = 2;
	sd[0].dir = PA_STREAM_RECORD; 
	sd[0].stream_params.format = PA_SAMPLE_FLOAT32LE;
	sd[0].buffer_attrs.maxlength = (uint32_t)-1;
	sd[0].buffer_attrs.minreq = (uint32_t)-1;
	sd[0].buffer_attrs.prebuf = (uint32_t)-1;
	sd[0].buffer_attrs.fragsize = SCBLOCKSIZE * sizeof(float);
	sd[0].buffer_attrs.tlength = (uint32_t)-1;

	sd[1].stream = 0;
	sd[1].dir = PA_STREAM_PLAYBACK;
	sd[1].stream_params.format = PA_SAMPLE_FLOAT32LE;
	sd[1].stream_params.channels = 2;
	sd[1].buffer_attrs.maxlength = (uint32_t)-1;
	sd[1].buffer_attrs.minreq = (uint32_t)-1;
	sd[1].buffer_attrs.prebuf = (uint32_t)-1;
	sd[1].buffer_attrs.fragsize = (uint32_t)-1;
	sd[1].buffer_attrs.tlength = SCBLOCKSIZE * sizeof(float);

	tx_src_data = new SRC_DATA;

	snd_buffer = new float[sd[0].stream_params.channels * SND_BUF_LEN];
	rbuf       = new float[sd[0].stream_params.channels * SND_BUF_LEN];

	src_buffer = new float[sd[1].stream_params.channels * SND_BUF_LEN];
	fbuf       = new float[sd[1].stream_params.channels * SND_BUF_LEN];

	memset(snd_buffer, 0, sd[0].stream_params.channels * SND_BUF_LEN * sizeof(*snd_buffer));
	memset(rbuf,       0, sd[0].stream_params.channels * SND_BUF_LEN * sizeof(*rbuf));

	memset(src_buffer, 0, sd[1].stream_params.channels * SND_BUF_LEN * sizeof(*src_buffer));
	memset(fbuf,       0, sd[1].stream_params.channels * SND_BUF_LEN * sizeof(*fbuf));
}

SoundPulse::~SoundPulse()
{
	Close();

	delete tx_src_data;
	if (rx_src_state)
		src_delete(rx_src_state);
	if (tx_src_state)
		src_delete(tx_src_state);

	delete [] snd_buffer;
	delete [] src_buffer;
	delete [] fbuf;
	delete [] rbuf;
}

int SoundPulse::Open(int dir, int freq)
{
	const char* server = (progdefaults.PulseServer.length() ?
				  progdefaults.PulseServer.c_str() : NULL);
	char sname[32];
	int err;

	sample_frequency = freq;

	src_data_reset(1 << O_RDONLY | 1 << O_WRONLY);
	if ((unsigned)freq != sd[dir].stream_params.rate)
		Close(dir);

	sd[dir].stream_params.rate = freq;
	snprintf(sname, sizeof(sname), "%s (%u)", (dir ? "playback" : "capture"), getpid());
	setenv("PULSE_PROP_application.icon_name", PACKAGE_TARNAME, 1);
	sd[dir].stream = pa_simple_new(server, main_window_title.c_str(), sd[dir].dir, NULL,
				 sname, &sd[dir].stream_params, NULL,
				 &sd[dir].buffer_attrs, &err);
	if (!sd[dir].stream)
		throw SndPulseException(err);

	return 0;
}

void SoundPulse::Close(unsigned dir)
{
	if (dir == 1 || dir == UINT_MAX)
		flush(1);
	Abort(dir);
}

void SoundPulse::Abort(unsigned dir)
{
		unsigned start, end;
		if (dir == UINT_MAX) {
				start = 0;
				end = 1;
		}
		else
				start = end = dir;

		for (unsigned i = start; i <= end; i++) {
				if (sd[i].stream) {
						pa_simple_free(sd[i].stream);
						sd[i].stream = 0;
				}
		}

}

void SoundPulse::flush(unsigned dir)
{
	int err = PA_OK;
	if ((dir == 1 || dir == UINT_MAX) && sd[1].stream) {
		// wait for audio to finish playing
		MilliSleep(SCBLOCKSIZE * 1000 / sd[1].stream_params.rate);
		pa_simple_flush(sd[1].stream, &err);
	}

	if ((dir == 0 || dir == UINT_MAX) && sd[0].stream) {
		// We need to flush the captured audio that PA has been
		// buffering for us while we were transmitting.  We will use
		// pa_simple_get_latency() which, contrary to the docs, also
		// works for capture streams.  It tells us how much earlier the
		// data that would be returned by pa_simple_read() was actually
		// captured, and we read and discard all that data.
		pa_usec_t t = pa_simple_get_latency(sd[0].stream, &err);
		if (t && err == PA_OK) {
			size_t bytes = pa_usec_to_bytes(t, &sd[0].stream_params);
			while (bytes > SND_BUF_LEN) {
				pa_simple_read(sd[0].stream, snd_buffer, SND_BUF_LEN, &err);
				if (err != PA_OK)
					break;
				bytes -= SND_BUF_LEN;
			}
			if (bytes)
				pa_simple_read(sd[0].stream, snd_buffer, bytes, &err);
		}
	}
}

size_t SoundPulse::Write(double* buf, size_t count)
{
	if (ofGenerate)
		write_file(ofGenerate, buf, NULL, count);

	if (PERFORM_CPS_TEST || active_modem->XMLRPC_CPS_TEST) {
		return count;
	}

// copy input to both channels
	for (size_t i = 0; i < count; i++)
		if (progdefaults.sig_on_right_channel)
			fbuf[sd[1].stream_params.channels * i] = fbuf[sd[1].stream_params.channels * i + 1] = buf[i];
		else if (progdefaults.ReverseAudio) {
			fbuf[sd[1].stream_params.channels * i + 1] = buf[i];
			fbuf[sd[1].stream_params.channels * i] = 0;
		} else {
			fbuf[sd[1].stream_params.channels * i] = buf[i];
			fbuf[sd[1].stream_params.channels * i + 1] = 0;
		}

	return resample_write(fbuf, count);
}

size_t SoundPulse::Write_stereo(double* bufleft, double* bufright, size_t count)
{
	if (sd[1].stream_params.channels != 2)
		return Write(bufleft, count);

	if (ofGenerate)
		write_file(ofGenerate, bufleft, bufright, count);

	if (PERFORM_CPS_TEST || active_modem->XMLRPC_CPS_TEST) {
		return count;
	}

	for (size_t i = 0; i < count; i++) {
		if (progdefaults.ReverseAudio) {
			fbuf[sd[1].stream_params.channels * i + 1] = bufleft[i];
			fbuf[sd[1].stream_params.channels * i] = bufright[i];
		} else {
			fbuf[sd[1].stream_params.channels * i] = bufleft[i];
			fbuf[sd[1].stream_params.channels * i + 1] = bufright[i];
		}
	}

	return resample_write(fbuf, count);
}

size_t SoundPulse::resample_write(float* buf, size_t count)
{
	int err;
		float *wbuf = buf;

	if (progdefaults.TX_corr != 0) {
		if (txppm != progdefaults.TX_corr) {
			txppm = progdefaults.TX_corr;
			tx_src_data->src_ratio = 1.0 + txppm / 1e6;
			src_set_ratio(tx_src_state, tx_src_data->src_ratio);
		}
		tx_src_data->data_in = wbuf;
		tx_src_data->input_frames = count;
		tx_src_data->data_out = src_buffer;
		tx_src_data->output_frames = SND_BUF_LEN;
		tx_src_data->end_of_input = 0;
		if ((err = src_process(tx_src_state, tx_src_data)) != 0)
			throw SndException(src_strerror(err));
		if (tx_src_data->output_frames_gen == 0) // input was too small
			return count;

		wbuf = tx_src_data->data_out;
		count = tx_src_data->output_frames_gen;
	}

	if (!active_modem) return count;

	if (pa_simple_write(sd[1].stream, wbuf, count * sd[1].stream_params.channels * sizeof(float), &err) == -1)
		throw SndPulseException(err);

	return count;
}

long SoundPulse::src_read_cb(void* arg, float** data)
{
	SoundPulse* p = reinterpret_cast<SoundPulse*>(arg);

	int err;
	int nread = 0;
	if ((nread = pa_simple_read(p->sd[0].stream, p->snd_buffer, 
			p->sd[0].stream_params.channels * sizeof(float) * p->sd[0].blocksize, &err)) == -1) {
		LOG_ERROR("%s", pa_strerror(err));
		*data = 0;
		return 0;
	}

	*data = p->snd_buffer;
	return p->sd[0].blocksize;
}

size_t SoundPulse::Read(float *buf, size_t count)
{
	if (ifPlayback) {
		read_file(ifPlayback, buf, count);
		if (!ofCapture) {
			flush(0);
			if (!bHighSpeed)
				MilliSleep((long)ceil((1e3 * count) / sample_frequency));
			return count;
		}
	}

	size_t n = 0;
	long r = 0;

	if (progdefaults.RX_corr != 0) {
		if (rxppm != progdefaults.RX_corr) {
			rxppm = progdefaults.RX_corr;
			sd[0].src_ratio = 1.0 / (1.0 + rxppm / 1e6);
			src_set_ratio(rx_src_state, sd[0].src_ratio);
		}
		sd[0].blocksize = SCBLOCKSIZE;
		while (n < count) {
			if ((r = src_callback_read(rx_src_state, sd[0].src_ratio, count - n, rbuf + n)) == 0)
				break;
			n += r;
		}
	}
	else {
		int err;
		if ((r = pa_simple_read(sd[0].stream, rbuf, 
				sd[0].stream_params.channels * sizeof(float) * count, &err)) == -1)
			throw SndPulseException(err);
	}

	// transfer active input channel; left == 0, right == 1
	size_t i = 0;
	if (sd[0].stream_params.channels == 2) n = progdefaults.ReverseRxAudio;
	else n = 0;
	while (i < count) {
		buf[i] = rbuf[n];
		i++;
		n += sd[0].stream_params.channels;
	}

	if (ofCapture)
		write_file(ofCapture, buf, NULL, count);

	return count;
}

void SoundPulse::src_data_reset(int mode)
{
		int err;
		if (mode & 1 << O_RDONLY) {
				if (rx_src_state)
						src_delete(rx_src_state);
				rx_src_state = src_callback_new(src_read_cb, progdefaults.sample_converter,
						sd[0].stream_params.channels, &err, this);
				if (!rx_src_state)
						throw SndException(src_strerror(err));
				sd[0].src_ratio = 1.0 / (1.0 + rxppm / 1e6);
		}
		if (mode & 1 << O_WRONLY) {
				if (tx_src_state)
						src_delete(tx_src_state);
				tx_src_state = src_new(progdefaults.sample_converter, sd[1].stream_params.channels, &err);
				if (!tx_src_state)
						throw SndException(src_strerror(err));
				tx_src_data->src_ratio = 1.0 + txppm / 1e6;
		}
}

#endif // USE_PULSEAUDIO


size_t SoundNull::Write(double* buf, size_t count)
{
	if (ofGenerate)
		write_file(ofGenerate, buf, NULL, count);

	if (PERFORM_CPS_TEST || active_modem->XMLRPC_CPS_TEST) {
		return count;
	}

	MilliSleep((long)ceil((1e3 * count) / sample_frequency));

	return count;
}

size_t SoundNull::Write_stereo(double* bufleft, double* bufright, size_t count)
{
	if (ofGenerate)
		write_file(ofGenerate, bufleft, bufright, count);

	MilliSleep((long)ceil((1e3 * count) / sample_frequency));

	return count ? count : 1;
}

size_t SoundNull::Read(float *buf, size_t count)
{
	if (ifPlayback)
		read_file(ifPlayback, buf, count);
	else
		memset(buf, 0, count * sizeof(*buf));
	if (ofCapture)
		write_file(ofCapture, buf, NULL, count);
	if (!bHighSpeed)
		MilliSleep((long)ceil((1e3 * count) / sample_frequency));

	return count;

}

void SoundNull::flush(unsigned)
{
	if (ofGenerate)
		sf_close(ofGenerate);
}

