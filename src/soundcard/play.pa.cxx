// Pulse Class

#include <queue>

#include "play.pa.h"
#include "misc.h"
#include "debug.h"

static pthread_t       alert_pthread;
static pthread_mutex_t alert_mutex;

	   bool alert_thread_running   = false;
static bool alert_terminate_flag   = false;

struct PLAYLIST { c_portaudio *cpa; float *fbuff; int bufflen; int sr; };

std::queue<PLAYLIST *> playlist;

/**********************************************************************************
 * AUDIO_ALERT process event.
 **********************************************************************************/
void process_alert()
{
	if (playlist.empty()) return;

	struct PLAYLIST *plist = playlist.front();
	if (plist->cpa == 0) {
//LOG_INFO("%s", "plist->cpa == 0");
		guard_lock que_lock(&alert_mutex);
		playlist.pop();
		delete [] plist->fbuff;
		delete plist;
		return;
	}

// opening a stream creates a new service thread in port audio
	plist->cpa->open(plist->sr);
	if (!plist->cpa->stream) {
		plist->cpa->close();
		playlist.pop();
		return;
	}

	int paError = Pa_StartStream(plist->cpa->stream);

	if (paError == paNoError) {
		#define FBUFLEN 512
		float fbuff[FBUFLEN];
		int   len = 2 * plist->bufflen;

		int   n = 0;
		while (n < (len - FBUFLEN)) {
			memset(fbuff, 0, FBUFLEN * sizeof(*fbuff));
			memcpy(fbuff, &plist->fbuff[n], FBUFLEN * sizeof(float));
			paError = Pa_WriteStream(plist->cpa->stream, fbuff, FBUFLEN/2);
			n += FBUFLEN;
		}
		if (n < len) {
			memset(fbuff, 0, FBUFLEN * sizeof(*fbuff));
			memcpy(fbuff, &plist->fbuff[n], (len - n) * sizeof(float));
			paError = Pa_WriteStream(plist->cpa->stream, fbuff, FBUFLEN/2);
		}

		paError = Pa_StopStream(plist->cpa->stream);

	}

	if (paError != paNoError)
		LOG_ERROR("paError = %d", paError);

// closing the stream terminates the service thread in port audio
	plist->cpa->close();

	delete [] plist->fbuff;
	delete plist;

	guard_lock que_lock(&alert_mutex);

	playlist.pop();
//LOG_INFO("%d items remain in play list", (int)(playlist.size()));

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

		if(trx_state == STATE_RX) {
			process_alert();
		}
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

	LOG_INFO("%s", "started audio alert thread");

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

	LOG_INFO("%s", "audio alert thread - stopped");

	pthread_mutex_destroy(&alert_mutex);

	memset((void *) &alert_pthread, 0, sizeof(alert_pthread));
	memset((void *) &alert_mutex,   0, sizeof(alert_mutex));

	alert_thread_running   = false;
	alert_terminate_flag   = false;
}

/**********************************************************************************
 * Signal AUDIO_ALERT to process Waterfall power level information.
 **********************************************************************************/

static void add_alert(c_portaudio * _cpa, float *buffer, int len, int sr)
{
	if(alert_thread_running) {
		struct PLAYLIST *plist = new PLAYLIST;
		plist->fbuff = new float[2*len];
		plist->bufflen = len;
		plist->sr = sr;
		for (int i = 0; i < 2*len; i++) plist->fbuff[i] = buffer[i];
		plist->cpa = _cpa;

		guard_lock que_lock(&alert_mutex);
		playlist.push(plist);
//LOG_INFO("play list contains %d items", (int)(playlist.size()));
	}
}

// Initialize the c_portaudio class
c_portaudio::c_portaudio()
{
	PaError paError = Pa_Initialize();
	if (paError != paNoError)
		throw cPA_exception(paError);

	stream = 0;
	paStreamParameters.device = Pa_GetDefaultOutputDevice();
	paStreamParameters.channelCount = 2;
	paStreamParameters.sampleFormat = paFloat32;
	paStreamParameters.suggestedLatency = Pa_GetDeviceInfo(paStreamParameters.device)->defaultLowOutputLatency;
	paStreamParameters.hostApiSpecificStreamInfo = NULL;

	start_alert_thread();
}

c_portaudio::~c_portaudio()
{
	stop_alert_thread();
	Pa_Terminate();
}

void c_portaudio::open(int samplerate)
{
	sr = samplerate;
	paError = Pa_OpenStream( &stream,
		0,
		&paStreamParameters,
		sr,
		paFramesPerBufferUnspecified, paClipOff,
		0,
		0);

	if (paError != paNoError) {
		throw cPA_exception(paError);
	}
// else
//LOG_INFO("opened pa stream %p @ %d samples/sec", stream, samplerate);
}

void c_portaudio::close()
{
	paError = Pa_CloseStream(stream);
	if (paError != paNoError)
		throw cPA_exception(paError);
//	else
//LOG_INFO("closed stream %p", stream);
}

// Play stereo buffer
void c_portaudio::play_buffer(float *buffer, int len, int _sr)
{
	sr = _sr;
	add_alert(this, buffer, len, sr);

	return;
}

// play mono buffer
void c_portaudio::play_sound(int *buffer, int len, int _sr)
{
	try {
		float fbuff[2*len];
		for (int i = 0; i < len; i++)
			fbuff[2*i] = fbuff[2*i+1] = buffer[i] / 32768.0;
		play_buffer(fbuff, len, _sr);
	} catch (...) {
		throw;
	}
	return;
}

// play mono buffer
void c_portaudio::play_sound(float *buffer, int len, int _sr)
{
	try {
		float fbuff[2*len];
		for (int i = 0; i < len; i++)
			fbuff[2*i] = fbuff[2*i+1] = buffer[i];
		play_buffer(fbuff, len, _sr);
	} catch (...) {
		throw;
	}
	return;
}

void c_portaudio::silence(float secs, int _sr)
{
	int len = secs * SCRATE;
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
//		throw("SF open error");

	int ch = playinfo.channels;
	int fsize = playinfo.frames * ch;

	float buffer[fsize];
	memset(buffer, 0, fsize * sizeof(float));

	if (sf_readf_float( playback, buffer, fsize )) {
		if (ch == 2) {
			play_buffer(buffer, fsize, playinfo.samplerate);
		} else {
			play_sound(buffer, fsize, playinfo.samplerate);
		}
	}

	sf_close(playback);

}
