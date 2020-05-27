// Pulse Class

#include <queue>
#include <stack>
#include <string>

#include "play.pa.h"
#include "misc.h"
#include "debug.h"
#include "configuration.h"
#include "fl_digi.h"
#include "qrunner.h"
#include "confdialog.h"

#define DR_MP3_IMPLEMENTATION
#include "dr_mp3.h"

#define CHANNELS                  2
#define SCRATE                 8000
#define FRAMES_PER_BUFFER      1024 // lower values causes audio distortion on pi3
#define RBUFF_SIZE            16384 // 4096

static pthread_t       alert_pthread;
static pthread_cond_t  alert_cond;
static pthread_mutex_t alert_mutex = PTHREAD_MUTEX_INITIALIZER;

static pthread_mutex_t	filter_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t	rx_stream_mutex = PTHREAD_MUTEX_INITIALIZER;

static pthread_t		filelist_pthread;
static pthread_mutex_t	filelist_mutex = PTHREAD_MUTEX_INITIALIZER;

static void start_alert_thread(void);
static void start_filelist_thread(void);
static void stop_alert_thread(void);
static void stop_filelist_thread(void);

static bool alert_thread_running   = false;
static bool alert_terminate_flag   = false;
//static bool stream_ready           = false;

enum { NONE, START, OPEN, CLOSE, TERMINATE };
static int alert_process_flag     = NONE;

struct PLAYLIST {
	c_portaudio *cpa;
	float *fbuff;
	unsigned long int bufflen;
	unsigned long int data_ptr;
	unsigned long int frames;
	int src;
};

std::queue<PLAYLIST *> playlist;

static PLAYLIST *plist = 0;

struct FILELIST {
	c_portaudio *cpa;
	std::string fn;
	FILELIST() { cpa = 0; fn = ""; }
	FILELIST( c_portaudio *_cpa, std::string _fn ) {
		cpa = _cpa; fn = _fn;
	}
	~FILELIST() {};
};

std::stack<FILELIST> filelist;

/**********************************************************************************
 * AUDIO_ALERT process event.
 **********************************************************************************/
int csinc = 2;// 0 - best, 1 - medium, 2 - fastest, 3 - zoh, 4 - linear
static sf_count_t
rate_convert (float *inbuff, int len, float *outbuff, int outlen, double src_ratio, int channels)
{
	SRC_DATA	src_data ;

	src_data.data_in		= inbuff;				// pointer to the input data samples.
	src_data.input_frames	= len / channels;		// number of frames of data pointed to by data_in.
	src_data.data_out		= outbuff;				// pointer to the output data samples.
	src_data.output_frames	= outlen / channels;	// Maximum number of frames pointed to by data_out.
	src_data.src_ratio		= src_ratio;			// output_sample_rate / input_sample_rate.

	int error = src_simple (&src_data, csinc, channels) ;

	return error;
}

int stream_process(
			const void* in, void* out, unsigned long nframes,
			const PaStreamCallbackTimeInfo *time_info,
			PaStreamCallbackFlags flags, void* data)
{
	float* outf = reinterpret_cast<float*>(out);
	memset(outf, 0, nframes * 2 * sizeof(float));

	if (!plist) {
		if (playlist.empty()) {
			guard_lock rx_lock(&rx_stream_mutex);
			c_portaudio *cpa = (c_portaudio *)data;
			unsigned long len = nframes * cpa->paStreamParameters.channelCount;
			unsigned long available = cpa->monitor_rb->read_space();
			if (progdefaults.mon_xcvr_audio && available >= len) {
				cpa->monitor_rb->read((float *)out, len);
			}
			return paContinue;
		}
		guard_lock que_lock(&alert_mutex);
		plist = playlist.front();
		playlist.pop();
	}

	c_portaudio* cpa = plist->cpa;
	int chcnt = cpa->paStreamParameters.channelCount;
	unsigned long int nbr_frames = plist->bufflen / chcnt;

	unsigned int ncopy = nbr_frames - plist->data_ptr;
	if (ncopy > nframes) ncopy = nframes;

	memcpy(	outf,
			plist->fbuff + plist->data_ptr * chcnt,
			ncopy * chcnt * sizeof(float));
	plist->data_ptr += ncopy;

	float outvol = 0.01 * progdefaults.alert_volume;
	for (unsigned int n = 0; n < ncopy * chcnt; n++) outf[n] *= outvol;

	if (nbr_frames && plist->src == c_portaudio::ALERT) {
		static char progress[20];
		snprintf(progress, sizeof(progress), "%d %%", int(100.0 * plist->data_ptr / nbr_frames));
		put_status(progress, 2.0, STATUS_CLEAR);
	}

	if (plist->data_ptr >= nbr_frames) {
		plist = NULL;
	}

	return paContinue;
}

static int paStatus;
static void StreamFinished( void* userData )
{
	paStatus = paComplete;
}

void process_alert()
{
	return;
}

/**********************************************************************************
 * AUDIO_ALERT processing loop.
 * syncs to requests for audio alert output
 **********************************************************************************/
static c_portaudio *requester = 0;

static void * alert_loop(void *args)
{
	SET_THREAD_ID(AUDIO_ALERT_TID);

	alert_thread_running   = true;
	alert_terminate_flag   = false;

	while(1) {
		pthread_mutex_lock(&alert_mutex);
		pthread_cond_wait(&alert_cond, &alert_mutex);
		pthread_mutex_unlock(&alert_mutex);

		if (alert_process_flag == OPEN) {
			if (requester)
				requester->open();
			alert_process_flag = NONE;
			requester = 0;
		}
		if (alert_process_flag == CLOSE) {
			if (requester)
				requester->close();
			alert_process_flag = NONE;
			requester = 0;
		}

		if (alert_process_flag == TERMINATE)
			break;

	}
	return (void *)0;
}

void open_alert_port(c_portaudio *cpa)
{
	alert_process_flag = OPEN;
	requester = cpa;
	pthread_cond_signal(&alert_cond);
}

void close_alert_port(c_portaudio *cpa)
{
	alert_process_flag = CLOSE;
	requester = cpa;
	pthread_cond_signal(&alert_cond);
}

/**********************************************************************************
 * Start AUDIO_ALERT Thread
 **********************************************************************************/
static void start_alert_thread(void)
{
	if(alert_thread_running) return;

	memset((void *) &alert_pthread, 0, sizeof(alert_pthread));
	memset((void *) &alert_mutex,   0, sizeof(alert_mutex));
	memset((void *) &alert_cond,    0, sizeof(alert_cond));

	if(pthread_cond_init(&alert_cond, NULL)) {
		LOG_ERROR("Alert thread create fail (pthread_cond_init)");
		return;
	}

	if(pthread_mutex_init(&alert_mutex, NULL)) {
		LOG_ERROR("AUDIO_ALERT thread create fail (pthread_mutex_init)");
		return;
	}

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

	struct PLAYLIST *plist = 0;
	while (!playlist.empty()) {
		plist = playlist.front();
		delete [] plist->fbuff;
		playlist.pop();
	}

	alert_process_flag = TERMINATE;
	pthread_cond_signal(&alert_cond);

	MilliSleep(10);

	pthread_join(alert_pthread, NULL);

	LOG_VERBOSE("%s", "audio alert thread - stopped");

	pthread_mutex_destroy(&alert_mutex);
	pthread_cond_destroy(&alert_cond);

	memset((void *) &alert_pthread, 0, sizeof(alert_pthread));
	memset((void *) &alert_mutex,   0, sizeof(alert_mutex));

	alert_thread_running   = false;
	alert_terminate_flag   = false;
}

static void add_alert(c_portaudio * _cpa, float *buffer, int len, int src)
{
	if(alert_thread_running) {
		if (_cpa->paStreamParameters.device == -1) return;
		struct PLAYLIST *plist = new PLAYLIST;
		plist->fbuff = buffer;
		plist->bufflen = len;
		plist->cpa = _cpa;
		plist->data_ptr = 0;
		plist->frames = len / _cpa->paStreamParameters.channelCount;

		guard_lock que_lock(&alert_mutex);
		playlist.push(plist);
		LOG_VERBOSE("play list contains %d items", (int)(playlist.size()));
	}
}

// Initialize the c_portaudio class
c_portaudio::c_portaudio()
{
	PaError paError = Pa_Initialize();
	if (paError != paNoError) {
		LOG_ERROR("pa Error # %d, %s", paError, Pa_GetErrorText(paError));
		throw cPA_exception(paError);
	}

	stream = 0;
	fbuffer = new float[1024];
	nubuffer = new float[1024 * 6];
	data_frames = new float[ FRAMES_PER_BUFFER * CHANNELS ];

	paStreamParameters.device = -1;
	sr = 44100;
	paStreamParameters.channelCount = 2;
	paStreamParameters.sampleFormat = paFloat32;
	paStreamParameters.hostApiSpecificStreamInfo = NULL;

	sr = SCRATE;
	b_sr = SCRATE;
	b_len = 0;
	rc = src_new (progdefaults.sample_converter, 1, &rc_error) ;
	monitor_rb = new ringbuffer<float>(RBUFF_SIZE);

	bpfilt = new C_FIR_filter();

	start_alert_thread();
	start_filelist_thread();
}

c_portaudio::~c_portaudio()
{
	close();
	stop_filelist_thread();
	stop_alert_thread();
	Pa_Terminate();

	delete monitor_rb;
	delete [] fbuffer;
	delete [] nubuffer;
	delete [] data_frames;
	src_delete(rc);
	delete bpfilt;
}

void c_portaudio::init_filter()
{
	guard_lock filter_lock(&filter_mutex);

	if (!bpfilt) bpfilt = new C_FIR_filter();

	flo = 1.0 * progdefaults.RxFilt_low / sr;
	fhi = 1.0 * progdefaults.RxFilt_high / sr;
	double fmid = progdefaults.RxFilt_mid / sr;

	bpfilt->init_bandpass (511, 1, flo, fhi);

	{
		C_FIR_filter *testfilt = new C_FIR_filter();
		testfilt->init_bandpass(511, 1, flo, fhi);
		double amp = 0;
		double sum_in = 0, sum_out = 0;
		double inp = 0;
		for (int i = 0; i <  100 / fmid; i++) {
			inp = cos (TWOPI * i * fmid);
			if (testfilt->Irun( inp, amp ) ) {
				sum_in += fabs(inp);
				sum_out += fabs(amp);
			}
		}
		gain = 0.98 * sum_in / sum_out;
		delete testfilt;
	}

//	std::cout << "############################################" << std::endl;
//	std::cout << "Sampling rate: " << sr << std::endl;
//	std::cout << "BW : " << progdefaults.RxFilt_bw << " : [ " << progdefaults.RxFilt_low <<
//				 " | " << progdefaults.RxFilt_mid <<
//				 " | " << progdefaults.RxFilt_high << " ]" <<
//				 std::endl;
//	std::cout << "bpfilt :       " << flo << " | " << fhi << std::endl;
//	std::cout << "gain :         " << gain << std::endl;
//	std::cout << "############################################" << std::endl;

}

int c_portaudio::open()//void *data)
{
	paStreamParameters.device = progdefaults.AlertIndex;
	sr = Pa_GetDeviceInfo(paStreamParameters.device)->defaultSampleRate;
	paStreamParameters.channelCount = 2;
	paStreamParameters.sampleFormat = paFloat32;
	paStreamParameters.suggestedLatency = Pa_GetDeviceInfo(paStreamParameters.device)->defaultLowOutputLatency;
	paStreamParameters.hostApiSpecificStreamInfo = NULL;

	LOG_INFO("\n\
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

	paError = Pa_OpenStream(
		&stream,
		NULL, &paStreamParameters,
		sr,
		FRAMES_PER_BUFFER, paClipOff,
		stream_process, this);

	if (paError != paNoError) {
		LOG_ERROR("pa Error # %d, %s", paError, Pa_GetErrorText(paError));
		stream = 0;
		return 0;
	}

	paError = Pa_SetStreamFinishedCallback( stream, StreamFinished );
	if (paError != paNoError) {
		LOG_ERROR("pa Error # %d, %s", paError, Pa_GetErrorText(paError));
		paError = Pa_StopStream(stream );
		paError = Pa_CloseStream(stream);
		stream = 0;
		return 0;
	}

	paError = Pa_StartStream(stream );
	if (paError != paNoError) {
		LOG_ERROR("pa Error # %d, %s", paError, Pa_GetErrorText(paError));
		paError = Pa_StopStream(stream );
		paError = Pa_CloseStream(stream);
		stream = 0;
		return 0;
	}

LOG_INFO("OPENED pa stream %p @ %f samples/sec", stream, sr);

	init_filter();

	return 1;
}

void c_portaudio::close()
{
	if (stream) {
		paError = Pa_StopStream(stream );
		paError = Pa_CloseStream(stream);
		if (paError != paNoError) {
			LOG_ERROR("pa Error # %d, %s", paError, Pa_GetErrorText(paError));
		}
		else
			LOG_VERBOSE("closed stream %p", stream);
	}
}

// Play 2 channel buffer

void c_portaudio::play_buffer(float *buffer, int len, int _sr, int src)
{
// do not delete [] nubuffer
// deleted after use
	float *nubuffer = new float[len];
	int nusize = len;

	if (sr == _sr) { // do not resample if sample rate is default
		for (int i = 0; i < len; i++)
			nubuffer[i] = buffer[i];
	} else {
		double		src_ratio = 1.0 * sr / _sr;
// resize nubuffer
		nusize  = len * src_ratio;
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

	data_ptr = 0;
	add_alert(this, nubuffer, nusize, src);

	return;
}

// play mono buffer
void c_portaudio::play_sound(int *buffer, int len, int _sr)
{
	float *fbuff = new float[2];
	try {
		delete [] fbuff;
		fbuff = new float[2*len];
		for (int i = 0; i < len; i++)
			fbuff[2*i] = fbuff[2*i+1] = buffer[i] / 33000.0; //32768.0;
		play_buffer(fbuff, 2*len, _sr, ALERT);
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
	float *fbuff = new float[2];
	try {
		delete [] fbuff;
		fbuff = new float[2*len];
		for (int i = 0; i < len; i++) {
			fbuff[2*i] = fbuff[2*i+1] = buffer[i];
		}
		play_buffer(fbuff, 2 * len, _sr, ALERT);
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

    FILE* pFile;
    pFile = fopen(fname.c_str(), "rb");
    if (pFile == NULL) {
        return;
    }
    fclose(pFile);

	drmp3_config config;
	drmp3_uint64 frame_count;

	float* mp3_buffer =  drmp3_open_file_and_read_f32(
						fname.c_str(), &config, &frame_count );

	if (!mp3_buffer) {
		LOG_ERROR("File must be mp3 float 32 format");
		return;
	}

	LOG_INFO("\n\
MP3 channels: %d\n\
 sample rate: %d\n\
 frame count: %ld\n",
       config.outputChannels,
       config.outputSampleRate,
       long(frame_count));

	if (config.outputChannels == 2)
		play_buffer(mp3_buffer, config.outputChannels * frame_count, config.outputSampleRate);
	else
		play_sound(mp3_buffer, frame_count, config.outputSampleRate);

	drmp3_free(mp3_buffer);
}

void c_portaudio::play_wav(std::string fname)
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

void c_portaudio::do_play_file(std::string fname)
{
	if ((fname.find(".mp3") != std::string::npos) ||
		(fname.find(".MP3") != std::string::npos)) {
//std::cout << "do_play_file(" << fname << ")" << std::endl;
		return play_mp3(fname);
	}
	if ((fname.find(".wav") != std::string::npos) ||
		(fname.find(".WAV") != std::string::npos)) {
		return play_wav(fname);
	}
	LOG_ERROR("%s : Audio file format must be either wav or mp3", fname.c_str());
}

void c_portaudio::play_file(std::string fname)
{
	guard_lock filelock(&filelist_mutex);
//std::cout << "filelist.push(this, " << fname << ")" << std::endl;
	filelist.push( FILELIST(this, fname));
}

// write len elements from monophonic audio stream to ring buffer
// ring buffer is stereo; LRLRLR...

void c_portaudio::mon_write(double *buffer, int len, int mon_sr)
{
	float vol = 0.01 * progdefaults.RxFilt_vol;
	float *rsbuffer = 0;

	if (!bpfilt) init_filter();

	try {

// do not resample if alert samplerate == modem samplerate
		if (sr == mon_sr) {
			if (progdefaults.mon_dsp_audio) {
				guard_lock filter_lock(&filter_mutex);
				double out;
				for (int n = 0; n < len; n++) {
					if (bpfilt->Irun(buffer[n], out)) {
						nubuffer[2*n] = nubuffer[2*n + 1] = vol * gain * out;
					}
				}
			} else
				for (int i = 0; i < len; i++)
					nubuffer[2*i] = nubuffer[2*i+1] = vol * buffer[i];
			monitor_rb->write(nubuffer, 2 * len);
			return;
		}


// sample rates not equal; resample monophonic
		else {
			for (int i = 0; i < len; i++) fbuffer[i] = vol * buffer[i];

			double src_ratio = 1.0 * sr / mon_sr;

			rcdata.data_in		 = fbuffer;    // pointer to the input data samples.
			rcdata.input_frames	 = len;        // number of frames of data pointed to by data_in.
			rcdata.data_out		 = nubuffer;   // pointer to the output data samples.
			rcdata.output_frames = 512 * 6;    //nusize;     // Maximum number of frames pointed to by data_out.
			rcdata.src_ratio	 = src_ratio;  // output_sample_rate / input_sample_rate.
			rcdata.end_of_input	 = 0;

			// resample before filtering
			int erc;
			if ((erc = src_process (rc, &rcdata)) != 0) {
				LOG_ERROR("rate converter failed: %s", src_strerror (erc));
				throw;
			}
			int flen = rcdata.output_frames_gen;

			float *rsbuffer = new float[2*flen];
			if (progdefaults.mon_dsp_audio) {
				guard_lock filter_lock(&filter_mutex);
				double out;
				for (int n = 0; n < flen; n++) {
					if (bpfilt->Irun(nubuffer[n], out)) {
						rsbuffer[2*n] = rsbuffer[2*n+1] = gain * out;
					}
				}
				monitor_rb->write(rsbuffer, 2*flen);
				delete [] rsbuffer;
				return;
			}
			else {
				for (int n = 0; n < flen; n++) {
					rsbuffer[2*n] = rsbuffer[2*n+1] = nubuffer[n];
				}
				monitor_rb->write(rsbuffer, 2*flen);
				delete [] rsbuffer;
				return;
			}
		}

	} catch (...) {
		if (rsbuffer) { delete [] rsbuffer; rsbuffer = 0;}
		throw;
	}

}

// read len elements of 2 channel audio from ring buffer
//size_t c_portaudio::mon_read(float *buffer, int len)
//{
//	return monitor_rb->read(buffer, len);
//}

/**********************************************************************************
 * AUDIO FILELIST processing loop.
 * syncs to requests for file / clip playback
 **********************************************************************************/

bool filelist_thread_running = false;
bool filelist_terminate_flag = false;

static void * filelist_loop(void *args)
{
//	SET_THREAD_ID(AUDIO_ALERT_TID);

	alert_thread_running   = true;
	alert_terminate_flag   = false;
	FILELIST fl;
	while(1) {
		MilliSleep(50);

		if (filelist_terminate_flag) break;
		{
			guard_lock filelock(&filelist_mutex);
			if (!filelist.empty()) {
				fl = filelist.top();
				filelist.pop();
				fl.cpa->do_play_file(fl.fn);
			}
		}
	}
	return (void *)0;
}

/**********************************************************************************
 * Start FILELIST Thread
 **********************************************************************************/
static void start_filelist_thread(void)
{
	if(filelist_thread_running) return;

	memset((void *) &filelist_pthread, 0, sizeof(filelist_pthread));
	memset((void *) &filelist_mutex,   0, sizeof(filelist_mutex));

	if(pthread_mutex_init(&filelist_mutex, NULL)) {
		LOG_ERROR("AUDIO_ALERT thread create fail (pthread_mutex_init)");
		return;
	}

	memset((void *) &filelist_pthread, 0, sizeof(filelist_pthread));

	if (pthread_create(&filelist_pthread, NULL, filelist_loop, NULL) < 0) {
		pthread_mutex_destroy(&filelist_mutex);
		LOG_ERROR("AUDIO_ALERT thread create fail (pthread_create)");
	}

	LOG_VERBOSE("started audio alert thread");

	MilliSleep(10);
}

/**********************************************************************************
 * Stop AUDIO_ALERT Thread
 **********************************************************************************/
static void stop_filelist_thread(void)
{
	if(!filelist_thread_running) return;

	filelist_terminate_flag = true;

	MilliSleep(10);

	pthread_join(filelist_pthread, NULL);

	LOG_VERBOSE("%s", "pa filelist thread - stopped");

	pthread_mutex_destroy(&filelist_mutex);

	memset((void *) &filelist_pthread, 0, sizeof(filelist_pthread));
	memset((void *) &filelist_mutex,   0, sizeof(filelist_mutex));

	filelist_thread_running   = false;
	filelist_terminate_flag   = false;
}

