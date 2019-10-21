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

#define DR_MP3_IMPLEMENTATION
#include "dr_mp3.h"

#define PAPLAY_CALLBACK 1
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
#ifdef PAPLAY_CALLBACK
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

#ifdef PAPLAY_CALLBACK
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

void c_portaudio::play_mp3(std::string fname)
{
	if (fname.empty()) return;

	drmp3_config config;
	drmp3_uint64 frame_count;

	float* buffer =  drmp3_open_file_and_read_f32(
						fname.c_str(), &config, &frame_count );
	LOG_VERBOSE("\n\
MP3 parameters\n\
      channels: %d\n\
   sample rate: %d\n\
       decoded: %s",
       config.outputChannels,
       config.outputSampleRate,
       (buffer ? "YES" : "NO"));

	if (buffer) {
		if (config.outputChannels == 2)
			play_buffer(buffer, config.outputChannels * frame_count, config.outputSampleRate);
		else
			play_sound(buffer, frame_count, config.outputSampleRate);
		drmp3_free(buffer);
	} else
		LOG_ERROR("File must be mp3 float format");
}

void c_portaudio::play_file(std::string fname)
{
	if ((fname.find(".mp3") != std::string::npos) ||
		(fname.find(".MP3") != std::string::npos)) {
		return play_mp3(fname);
	}

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
