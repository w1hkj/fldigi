// Pulse Class

#include <queue>
#include <string>
#include <samplerate.h>

#include "play.pa.h"
#include "misc.h"
#include "debug.h"
#include "configuration.h"
#include "fl_digi.h"
#include "qrunner.h"

#define CALLBACK 1
//#define PLAYBACK_SAMPLERATE 44100

static pthread_t       alert_pthread;
static pthread_mutex_t alert_mutex;

bool alert_thread_running   = false;
static bool alert_terminate_flag   = false;

struct PLAYLIST { c_portaudio *cpa; float *fbuff; unsigned long int bufflen; unsigned long int data_ptr; std::string lname; };

#define CHANNELS 2

std::queue<PLAYLIST *> playlist;

/**********************************************************************************
 * AUDIO_ALERT process event.
 **********************************************************************************/
static sf_count_t
rate_convert (float *inbuff, int len, float *outbuff, int outlen, double src_ratio, int channels)
{
	SRC_DATA	src_data ;

	src_data.data_in		= inbuff;				// pointer to the input data samples.
	src_data.input_frames	= len / channels;		// number of frames of data pointed to by data_in.
	src_data.data_out		= outbuff;				// pointer to the output data samples.
	src_data.output_frames	= outlen / channels;	// Maximum number of frames pointed to by data_out.
	src_data.src_ratio		= src_ratio;			// output_sample_rate / input_sample_rate.

	int error = src_simple (&src_data, SRC_SINC_FASTEST, channels) ;

	return error;
}

static int paStatus;

static void show_progress(unsigned long read_frames, unsigned long played)
{
	static char progress[10];
	snprintf(progress, sizeof(progress), "%d %%", int(100 * played / read_frames));
	put_status(progress, 2.0, STATUS_CLEAR);
}

static int stream_process(
			const void* in, void* out, unsigned long nframes,
			const PaStreamCallbackTimeInfo *time_info,
			PaStreamCallbackFlags flags, void* data)
{

	PLAYLIST* pl = reinterpret_cast<PLAYLIST *>(data);
	c_portaudio* cpa = pl->cpa;
	int chcnt = cpa->paStreamParameters.channelCount;
	unsigned long int nbr_frames = pl->bufflen / chcnt;
	float *data_frames = pl->fbuff;

	if (cpa->state == paAbort || cpa->state == paComplete) { // finished
		return cpa->state;
	}
	if (pl->data_ptr == nbr_frames)
		return (paStatus = paComplete);

	float* outf = reinterpret_cast<float*>(out);
	memset(outf, 0, chcnt * sizeof(float));

	if (nbr_frames - pl->data_ptr > nframes) {
		for (unsigned long int n = 0; n < nframes; n++) {
			outf[n * chcnt] = data_frames[ (pl->data_ptr + n) * chcnt ];
			outf[n * chcnt + 1] = data_frames[ (pl->data_ptr + n) * chcnt  + 1];
		}
		pl->data_ptr += nframes;
	} else {
		for (unsigned long int n = 0; n < (nbr_frames - pl->data_ptr); n++) {
			outf[n] = data_frames[ chcnt * (pl->data_ptr + n) ];
			outf[n + 1] = data_frames[ chcnt * (pl->data_ptr + n)  + 1];
		}
		pl->data_ptr = nbr_frames;
	}
	show_progress(nbr_frames, pl->data_ptr);
	return paContinue;
}

static void StreamFinished( void* userData )
{
	paStatus = paComplete;
}

void process_alert()
{
	guard_lock que_lock(&alert_mutex);

	struct PLAYLIST *plist = playlist.front();

	while (!playlist.empty()) {

		if (plist->cpa == 0) {
//LOG_INFO("%s", "plist->cpa == 0");
			while (!playlist.empty()) {
				plist = playlist.front();
				delete [] plist->fbuff;
				playlist.pop();
			}
			return;
		}

// opening a stream creates a new service thread in port audio
		if (!plist->cpa->open(plist)) {
			LOG_ERROR("cannot open pa stream");
			while (!playlist.empty()) {
				plist = playlist.front();
				delete [] plist->fbuff;
				playlist.pop();
			}
			return;
		}
		if (!plist->cpa->stream) {
			plist->cpa->close();
			while (!playlist.empty()) {
				plist = playlist.front();
				delete [] plist->fbuff;
				playlist.pop();
			}
			return;
		}

		int paError = 0;
#ifdef CALLBACK
		int terminate = (plist->bufflen/2)/plist->cpa->sr;
		terminate *= 10;
		terminate += 10;

		paStatus = paContinue;

		paError = Pa_SetStreamFinishedCallback( plist->cpa->stream, &StreamFinished );
		if (paError != paNoError) goto err_exit;

		paError = Pa_StartStream( plist->cpa->stream );
		if (paError != paNoError) goto err_exit;

		while ( paStatus != paComplete && terminate 
				&& (Pa_IsStreamActive(plist->cpa->stream) == 1)) {
			MilliSleep(100);
			terminate--;
		}

// closing the stream terminates the service thread in port audio
		paError = Pa_StopStream( plist->cpa->stream );

#else
/* send as a single block */
		paError = Pa_StartStream( plist->cpa->stream );
		if (paError != paNoError) goto err_exit;

		paError = Pa_WriteStream(plist->cpa->stream, plist->fbuff, plist->bufflen/2);
		if (paError != paNoError)
			goto err_exit;

#endif

err_exit:
		if (paError != paNoError) {
// closing the stream terminates the service thread in port audio
			Pa_StopStream( plist->cpa->stream );
std::cout << Pa_GetErrorText(paError) << std::endl;
		}

		plist->cpa->close();
		delete [] plist->fbuff;
		playlist.pop();
		plist = playlist.front();
	}
	return;

}

/**********************************************************************************
 * AUDIO_ALERT processing loop.
 * syncs to requests for audio alert output
 **********************************************************************************/
static void * alert_loop(void *args)
{
	SET_THREAD_ID(AUDIO_ALERT_TID);

	alert_thread_running   = true;
	alert_terminate_flag   = false;

	while(1) {
		MilliSleep(50);

		if (alert_terminate_flag) break;

		if (trx_state == STATE_RX && !playlist.empty())
			process_alert();

	}
	return (void *)0;
}

/**********************************************************************************
 * Start AUDIO_ALERT Thread
 **********************************************************************************/
static void start_alert_thread(void)
{
	if(alert_thread_running) return;

	memset((void *) &alert_pthread, 0, sizeof(alert_pthread));
	memset((void *) &alert_mutex,   0, sizeof(alert_mutex));

	if(pthread_mutex_init(&alert_mutex, NULL)) {
		LOG_ERROR("AUDIO_ALERT thread create fail (pthread_mutex_init)");
		return;
	}

	memset((void *) &alert_pthread, 0, sizeof(alert_pthread));

	if (pthread_create(&alert_pthread, NULL, alert_loop, NULL) < 0) {
		pthread_mutex_destroy(&alert_mutex);
		LOG_ERROR("AUDIO_ALERT thread create fail (pthread_create)");
	}

	LOG_VERBOSE("started audio alert thread");

	MilliSleep(10); // Give the CPU time to set 'alert_thread_running'
}

/**********************************************************************************
 * Stop AUDIO_ALERT Thread
 **********************************************************************************/
static void stop_alert_thread(void)
{
	if(!alert_thread_running) return;

	alert_terminate_flag = true;

	MilliSleep(10);

	pthread_join(alert_pthread, NULL);

	LOG_VERBOSE("%s", "audio alert thread - stopped");

	pthread_mutex_destroy(&alert_mutex);

	memset((void *) &alert_pthread, 0, sizeof(alert_pthread));
	memset((void *) &alert_mutex,   0, sizeof(alert_mutex));

	alert_thread_running   = false;
	alert_terminate_flag   = false;
}

static void add_alert(c_portaudio * _cpa, float *buffer, int len)
{
	if(alert_thread_running) {
		struct PLAYLIST *plist = new PLAYLIST;
		plist->fbuff = buffer;//new float[len];
		plist->bufflen = len; // # floats 
		plist->cpa = _cpa;
		plist->data_ptr = 0;

		guard_lock que_lock(&alert_mutex);
		playlist.push(plist);
		LOG_VERBOSE("play list contains %d items", (int)(playlist.size()));
	}
}

// Initialize the c_portaudio class
c_portaudio::c_portaudio()
{
	PaError paError = Pa_Initialize();
	if (paError != paNoError)
		throw cPA_exception(paError);

	stream = 0;
	start_alert_thread();
}

c_portaudio::~c_portaudio()
{
	stop_alert_thread();
	Pa_Terminate();
}

int c_portaudio::open(void *data)
{
	LOG_VERBOSE("\n\
open pa stream:\n\
  samplerate         : %.0f\n\
  device name        : %s\n\
  device number      : %d\n\
  # channels         : %d\n\
  latency            : %f\n\
  sample Format      : paFloat32",
	sr,
	progdefaults.AlertDevice.c_str(),
	paStreamParameters.device,
	paStreamParameters.channelCount,
	paStreamParameters.suggestedLatency);

	state = paContinue;

#ifdef CALLBACK
	paError = Pa_OpenStream(
		&stream,
		NULL, &paStreamParameters,
		sr,
		FRAMES_PER_BUFFER, paClipOff,
		stream_process, data);
#else
	paError = Pa_OpenStream(
		&stream,
		NULL, &paStreamParameters,
		sr,
		FRAMES_PER_BUFFER, paClipOff,
		NULL, NULL);
#endif

	if (paError != paNoError) {
		LOG_ERROR("open pa stream failed: %s", Pa_GetErrorText(paError));
		return 0;
	}
//LOG_INFO("opened pa stream %p @ %f samples/sec", stream, sr);
	return 1;
}

void c_portaudio::close()
{
	paError = Pa_CloseStream(stream);
	if (paError != paNoError) {
		LOG_ERROR("pa close failed");
		throw cPA_exception(paError);
	}
	else
		LOG_VERBOSE("closed stream %p", stream);
}

// Play 2 channel buffer

void c_portaudio::play_buffer(float *buffer, int len, int _sr)
{
	paStreamParameters.device = progdefaults.AlertIndex;
#ifdef PLAYBACK_SAMPLERATE
	sr = PLAYBACK_SAMPLERATE;
#else
	sr = Pa_GetDeviceInfo(paStreamParameters.device)->defaultSampleRate;
#endif
	paStreamParameters.channelCount = CHANNELS;
	paStreamParameters.sampleFormat = paFloat32;
	paStreamParameters.suggestedLatency = Pa_GetDeviceInfo(paStreamParameters.device)->defaultLowOutputLatency;
	paStreamParameters.hostApiSpecificStreamInfo = NULL;

	data_ptr = 0;

// do not delete [] nubuffer
// deleted after use
	float *nubuffer = new float[len];
	int nusize = len;
	if (sr == _sr) { // do not resample if sample rate is default
		for (int i = 0; i < len; i++)
			nubuffer[i] = buffer[i];
	} else {
		double		src_ratio = 1.0 * sr / _sr;
		nusize  = len * src_ratio;
// resize nubuffer
		delete [] nubuffer;
		nubuffer = new float[nusize];

		int num = rate_convert(
					buffer, len,
					nubuffer, nusize,
					src_ratio,
					paStreamParameters.channelCount);
		if (num != 0) {
			LOG_ERROR("rate converter failed");
			return;
		}
	}

// test for values exceeding |1.0|
	double max = 1.0;
	for (int i = 0; i < nusize / 2; i++) {
		if (fabs(nubuffer[2 * i]) > max) max = fabs(nubuffer[2 * i]);
		if (fabs(nubuffer[2 * i + 1]) > max) max = fabs(nubuffer[2 * i + 1]);
	}
	if (max > 1.0) {
		max *= 1.01;
		for (int i = 0; i < nusize / 2; i++) {
			nubuffer[2 * i] /= max;
			nubuffer[2 * i + 1] /= max;
		}
	}

	add_alert(this, nubuffer, nusize);

	return;
}

// play mono buffer
void c_portaudio::play_sound(int *buffer, int len, int _sr)
{
//std::cout << "play_sound(int *, " << len << ", " << _sr << ")\n";
	float *fbuff = new float[2];
	try {
		delete [] fbuff;
		fbuff = new float[2*len];
		for (int i = 0; i < len; i++)
			fbuff[2*i] = fbuff[2*i+1] = buffer[i] / 32768.0;
		play_buffer(fbuff, 2*len, _sr);
	} catch (...) {
		delete [] fbuff;
		throw;
	}
	delete [] fbuff;
	return;
}

// play mono buffer
void c_portaudio::play_sound(float *buffer, int len, int _sr)
{
//std::cout << "play_sound(float *, " << len << ", " << _sr << ")\n";
	float *fbuff = new float[2];
	try {
		delete [] fbuff;
		fbuff = new float[2*len];
		for (int i = 0; i < len; i++) {
			fbuff[2*i] = fbuff[2*i+1] = buffer[i];
		}
		play_buffer(fbuff, 2 * len, _sr);
	} catch (...) {
		delete [] fbuff;
		throw;
	}
	delete [] fbuff;
	return;
}

void c_portaudio::silence(float secs, int _sr)
{
	int len = secs * _sr;
	int nosound[len];
	memset(nosound, 0, sizeof(*nosound) * len);
	play_sound(nosound, len, _sr);
}

void c_portaudio::play_file(std::string fname)
{
	playinfo.frames = 0;
	playinfo.samplerate = 0;
	playinfo.channels = 0;
	playinfo.format = 0;
	playinfo.sections = 0;
	playinfo.seekable = 0;

	if ((playback = sf_open(fname.c_str(), SFM_READ, &playinfo)) == NULL)
		return;

	int ch = playinfo.channels;
	int fsize = playinfo.frames * ch;

	float *buffer = new float[fsize];
	memset(buffer, 0, fsize * sizeof(*buffer));

	if (sf_readf_float( playback, buffer, playinfo.frames )) {
		if (ch == 2) {
			play_buffer(buffer, fsize, playinfo.samplerate);
		} else {
			play_sound(buffer, fsize, playinfo.samplerate);
		}
	}

	sf_close(playback);
	delete [] buffer;

}

/*
//======================================================================

void c_portaudio::get_file_params(std::string def_fname, std::string &fname, int &format)
{
	std::string filters = _("Waveform Audio Format\t*.wav\n");
	filters.append(_("AU\t*.{au,snd}\n"));
	if (format_supported(SF_FORMAT_FLAC | SF_FORMAT_PCM_16)) {
		filters.append(_("Free Lossless Audio Codec\t*.flac"));
	}

	format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
	int fsel;
	const char *fn = 0;
	if (def_fname.find("playback") != std::string::npos)
		fn = FSEL::select(_("Audio file"), filters.c_str(), def_fname.c_str(), &fsel);
	else
		fn = FSEL::saveas(_("Audio file"), filters.c_str(), def_fname.c_str(), &fsel);

	if (!fn || !*fn) {
		fname = "";
		return;
	}
	fname = fn;

	bool check_replace = false;
	fsel = 0;
	switch (fsel) {
		default:
		case 0:
			format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
			if (fname.find(".wav") == std::string::npos) {
				fname.append(".wav");
				check_replace = true;
			}
			break;
		case 1:
			format = SF_FORMAT_AU | SF_FORMAT_FLOAT | SF_ENDIAN_CPU;
			if (fname.find(".au") == std::string::npos ||
				fname.find(".snd") == std::string::npos) {
				fname.append(".au");
				check_replace = true;
			}
			break;
		case 2:
			format = SF_FORMAT_FLAC | SF_FORMAT_PCM_16;
			if (fname.find(".flac") == std::string::npos) {
				fname.append(".flac");
				check_replace = true;
			}
			break;
	}

	if (check_replace) {
		FILE *f = fopen(fname.c_str(), "r");
		if (f) {
			fclose(f);
			int ans = fl_choice("Replace %s?", "Yes", "No", 0, fname.c_str());
			if ( ans == 1) fname = "";
		}
	}
}

int c_portaudio::Capture(bool val)
{
	if (!val) {
		if (ofCapture) {
			int err;
			if ((err = sf_close(ofCapture)) != 0)
				LOG_ERROR("sf_close error: %s", sf_error_number(err));
			ofCapture = 0;
		}
		capture = false;
		return 1;
	}

	std::string fname;
	int format;
	get_file_params("capture", fname, format);

	if (fname.empty())
		return 0;

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
	tag_file(ofCapture, "Captured audio");

	capture = true;
	return 1;
}

int c_portaudio::Generate(bool val)
{
	if (!val) {
		if (ofGenerate) {
			int err;
			if ((err = sf_close(ofGenerate)) != 0)
				LOG_ERROR("sf_close error: %s", sf_error_number(err));
			ofGenerate = 0;
		}
		generate = false;
		return 1;
	}

	std::string fname;
	int format;
	get_file_params("generate", fname, format);
	if (fname.empty())
		return 0;

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
	tag_file(ofGenerate, "Generated audio");

	generate = true;

	modem_wr_sr = sample_frequency;

	writ_src_data_left->src_ratio = 1.0 * sndfile_samplerate[progdefaults.wavSampleRate] / modem_wr_sr;
	src_set_ratio(writ_src_state_left, writ_src_data_left->src_ratio);

	writ_src_data_right->src_ratio = 1.0 * sndfile_samplerate[progdefaults.wavSampleRate] / modem_wr_sr;
	src_set_ratio(writ_src_state_right, writ_src_data_right->src_ratio);

	return 1;
}


int c_portaudio::Playback(bool val)
{
	if (!val) {
		if (ifPlayback) {
			int err;
			if ((err = sf_close(ifPlayback)) != 0)
				LOG_ERROR("sf_close error: %s", sf_error_number(err));
			ifPlayback = 0;
		}
		playback = false;
		return 1;
	}
	std::string fname;
	int format;
	get_file_params("playback", fname, format);
	if (fname.empty())
		return -1;

	play_info.frames = 0;
	play_info.samplerate = 0;
	play_info.channels = 0;
	play_info.format = 0;
	play_info.sections = 0;
	play_info.seekable = 0;

	if ((ifPlayback = sf_open(fname.c_str(), SFM_READ, &play_info)) == NULL) {
		LOG_ERROR("Could not read %s:%s", fname.c_str(), sf_strerror(NULL) );
		return -2;
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
LOG_VERBOSE
("src ratio %f", play_src_data->src_ratio);

	progdefaults.loop_playback = fl_choice2(_("Playback continuous loop?"), _("No"), _("Yes"), NULL);

	playback = true;
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
sf_count_t c_portaudio::read_file(SNDFILE* file, float* buf, size_t count)
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
			Playback(0);
			bHighSpeed = false;
//			REQ(reset_mnuPlayback);
		} else {
			memset(buf, count, sizeof(*buf));
			sf_seek(file, 0, SEEK_SET);
		}
	}
	return r;
}

// ---------------------------------------------------------------------
// write_file
// All sound buffer data is resampled to a specified sample rate
//    progdefaults.wavSampleRate
// resultant data (left channel only) is written to a wav file
//----------------------------------------------------------------------
void c_portaudio::write_file(SNDFILE* file, float* bufleft, float* bufright, size_t count)
{
	if (bufright == NULL || !progdefaults.record_both_channels) {
		bufright = new float[count];
		memset(bufright, 0, count * sizeof(float));
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
		throw SndException(src_strerror(err));
	}
	if ((err = src_process(writ_src_state_right, writ_src_data_right)) != 0) {
		throw SndException(src_strerror(err));
	}

	output_size = writ_src_data_left->output_frames_gen;
	bufl = src_write_buffer_left;
	bufr = src_write_buffer_right;

	if (output_size) {
		if (progdefaults.record_both_channels) {
			float buffer[2*output_size];
			for (size_t i = 0; i < output_size; i++) {
				buffer[2*i] = 0.9 * bufl[i];
				buffer[2*i + 1] = 0.9 * bufr[i];
			}
			sf_write_float(file, buffer, 2 * output_size);
		} else {
			sf_write_float(file, bufl, output_size);
		}
	}
	return;

}

void c_portaudio::write_file(SNDFILE* file, double* bufleft, double *bufright, size_t count)
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

bool c_portaudio::format_supported(int format)
{
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

void c_portaudio::tag_file(SNDFILE *sndfile, const char *title)
{
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

//======================================================================

*/
