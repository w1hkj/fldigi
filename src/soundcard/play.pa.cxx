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

#define FRAMES_PER_BUFFER 64  // not to exceed MAX_FRAMES_PER_BUFFER / 2

static pthread_t       alert_pthread;
static pthread_cond_t  alert_cond;
static pthread_mutex_t alert_mutex = PTHREAD_MUTEX_INITIALIZER;

static pthread_mutex_t	filter_mutex = PTHREAD_MUTEX_INITIALIZER;

static pthread_t		filelist_pthread;
static pthread_mutex_t	filelist_mutex = PTHREAD_MUTEX_INITIALIZER;

static void start_alert_thread(void);
static void start_filelist_thread(void);
static void stop_alert_thread(void);
static void stop_filelist_thread(void);

static bool alert_thread_running   = false;
static bool alert_terminate_flag   = false;
static bool alert_process_flag     = false;

static int  alert_pending = c_portaudio::MONITOR;

#define CHANNELS 2

struct PLAYLIST {
	c_portaudio *cpa;
	float *fbuff;
	unsigned long int bufflen;
	unsigned long int data_ptr;
	int src;
};

std::queue<PLAYLIST *> playlist;

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
int csinc = 1;// 0 - best, 1 - medium, 2 - fastest, 3 - zoh, 4 - linear
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

static int paStatus;

static void show_progress(unsigned long read_frames, unsigned long played)
{
	static char progress[20];
	if ( !read_frames ) return;
	snprintf(progress, sizeof(progress), "%d %%", int(100.0 * played / read_frames));
	put_status(progress, 2.0, STATUS_CLEAR);
}

static PLAYLIST *plist = 0;

int stream_process(
			const void* in, void* out, unsigned long nframes,
			const PaStreamCallbackTimeInfo *time_info,
			PaStreamCallbackFlags flags, void* data)
{

	float* outf = reinterpret_cast<float*>(out);
	memset(outf, 0, nframes * 2 * sizeof(float));

	if (!plist && playlist.empty()) {
		c_portaudio *cpa = (c_portaudio *)data;
		cpa->mon_read((float *)out, nframes);
		return paContinue;
	}

	if ( !plist && !playlist.empty() ) { // 	block to guard the play list queue
		guard_lock que_lock(&alert_mutex);
		plist = playlist.front();
		playlist.pop();
	}

	c_portaudio* cpa = plist->cpa;
	int chcnt = cpa->paStreamParameters.channelCount;
	unsigned long int nbr_frames = plist->bufflen / chcnt;
	float *data_frames = plist->fbuff;

	unsigned int remain = nbr_frames - plist->data_ptr;
	if (remain > nframes) {
		for (unsigned long int n = 0; n < nframes; n++) {
			outf[n * chcnt] = data_frames[ (plist->data_ptr + n) * chcnt ];
			outf[n * chcnt + 1] = data_frames[ (plist->data_ptr + n) * chcnt  + 1];
		}
		plist->data_ptr += nframes;
	} else {
		for (unsigned long int n = 0; n < remain; n++) {
			outf[n * chcnt] = data_frames[ (plist->data_ptr + n) * chcnt ];
			outf[n * chcnt + 1] = data_frames[ (plist->data_ptr + n) * chcnt + 1];
		}
		plist->data_ptr += remain;
	}
	
	if (plist->src == c_portaudio::ALERT)
		show_progress(nbr_frames, plist->data_ptr);

	if (plist->data_ptr >= nbr_frames) {
		plist = NULL;
	}

	return paContinue;
}

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

		if (alert_terminate_flag) break;
// execute a single request for audio stream processing
		if (requester) {
			requester->process_mon();
			requester = 0;
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

	alert_terminate_flag = true;
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
		struct PLAYLIST *plist = new PLAYLIST;
		plist->fbuff = buffer;//new float[len];
		plist->bufflen = len; // # floats 
		plist->cpa = _cpa;
		plist->data_ptr = 0;

		alert_pending = src;

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
	fbuffer = 0;
	nubuffer = 0;
	sr = SCRATE;
	b_sr = SCRATE;
	b_len = 0;
	rc = src_new (1, 2, &rc_error) ;
	monitor_rb = new ringbuffer<float>(8192);
	start_alert_thread();
	start_filelist_thread();
	open(NULL);

	bpfilt = 0;
	init_filter();
}

c_portaudio::~c_portaudio()
{
	close();
	stop_filelist_thread();
	stop_alert_thread();
	delete monitor_rb;
	delete [] fbuffer;
	delete [] nubuffer;
	src_delete(rc);
	delete bpfilt;
	Pa_Terminate();
}

void c_portaudio::init_filter()
{
	guard_lock filter_lock(&filter_mutex);

//	if (bpfilt) delete bpfilt;
//	bpfilt = new C_FIR_filter();

	if (!bpfilt) bpfilt = new C_FIR_filter();

	flo = 1.0 * progdefaults.RxFilt_low / sr;
	fhi = 1.0 * progdefaults.RxFilt_high / sr;
	bpfilt->init_bandpass (511, 1, flo, fhi);

	double fmid = progdefaults.RxFilt_mid / sr;
	double amp = 0;
	double sum_in = 0, sum_out = 0;
	double inp = 0;
	for (int i = 0; i <  100 / fmid; i++) {
		inp = cos (TWOPI * i * fmid);
		if (bpfilt->Irun( inp, amp ) ) {
			sum_in += fabs(inp);
			sum_out += fabs(amp);
		}
	}
	gain = 0.98 * sum_in / sum_out;
	for (int i = 0; i < 256; i++) bpfilt->Irun(0.0, amp);
/*
std::cout << "############################################" << std::endl;
std::cout << "Sampling rate: " << sr << std::endl;
std::cout << "BW : " << progdefaults.RxFilt_bw << " : [ " << progdefaults.RxFilt_low << 
             " | " << progdefaults.RxFilt_mid << 
             " | " << progdefaults.RxFilt_high << " ]" <<
             std::endl;
std::cout << "bpfilt :       " << flo << " | " << fhi << std::endl;
std::cout << "gain :         " << gain << std::endl;
std::cout << "############################################" << std::endl;
*/
}

int c_portaudio::open(void *data)
{
	paStreamParameters.device = progdefaults.AlertIndex;
	sr = Pa_GetDeviceInfo(paStreamParameters.device)->defaultSampleRate;
	paStreamParameters.channelCount = CHANNELS;
	paStreamParameters.sampleFormat = paFloat32;
	paStreamParameters.suggestedLatency = Pa_GetDeviceInfo(paStreamParameters.device)->defaultLowOutputLatency;
	paStreamParameters.hostApiSpecificStreamInfo = NULL;

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

	paError = Pa_OpenStream(
		&stream,
		NULL, &paStreamParameters,
		sr,
		FRAMES_PER_BUFFER, paClipOff,
		stream_process, this);
	if (paError != paNoError) {
		LOG_ERROR("%s", Pa_GetErrorText(paError));
		return 0;
	}

	paError = Pa_SetStreamFinishedCallback( stream, StreamFinished );
	if (paError != paNoError) {
		LOG_ERROR("%s", Pa_GetErrorText(paError));
		return 0;
	}

	paError = Pa_StartStream(stream );
	if (paError != paNoError) {
		LOG_ERROR("%s", Pa_GetErrorText(paError));
		return 0;
	}

LOG_INFO("opened pa stream %p @ %f samples/sec", stream, sr);

	return 1;
}

void c_portaudio::close()
{
	paError = Pa_StopStream(stream );
	paError = Pa_CloseStream(stream);
	if (paError != paNoError) {
		LOG_ERROR("pa close failed");
		throw cPA_exception(paError);
	}
	else
		LOG_VERBOSE("closed stream %p", stream);
}

// Play 2 channel buffer

void c_portaudio::play_buffer(float *buffer, int len, int _sr, int src)
{
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

	drmp3_config config;
	drmp3_uint64 frame_count;

	drmp3 mp3;
    if (!drmp3_init_file(&mp3, fname.c_str(), NULL)) {
        LOG_ERROR("Failed to open mp3 file");
        return;
    }
	drmp3_uninit(&mp3);

	float* mp3_buffer =  drmp3_open_file_and_read_f32(
						fname.c_str(), &config, &frame_count );

	if (!mp3_buffer) {
		LOG_ERROR("File must be mp3 float 32 format");
		return;
	}

	LOG_VERBOSE("\n\
MP3 parameters\n\
      channels: %d\n\
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
	filelist.push( FILELIST(this, fname));
}

// =====================================================================
// Modem monitor
// play current unprocessed Rx audio stream
// play current post filtered Rx audio stream
// =====================================================================

// write len elements from monophonic audio stream to ring buffer

void c_portaudio::process_mon()
{
	if (nubuffer) delete [] nubuffer;
	nubuffer = 0;

	try {
		guard_lock filter_lock(&filter_mutex);

// do not resample if sample rate is default
		if (sr == b_sr) {
			if (progdefaults.mon_dsp_audio) {
				double out;
				for (int n = 0; n < b_len; n++) {
					if (bpfilt->Irun(fbuffer[2*n], out)) {
						fbuffer[2*n] = fbuffer[2*n+1] = gain * out; 
					}
				}
			}
			monitor_rb->write(fbuffer, 2 * b_len);	
		} else {
			double src_ratio = 1.0 * sr / b_sr;
			int    nusize  = b_len * src_ratio;

			nubuffer = new float[2 * nusize];
			memset(nubuffer, 0, 2 * nusize * sizeof(float));

			rcdata.data_in		 = fbuffer;	// pointer to the input data samples.
			rcdata.input_frames	 = b_len;      // number of frames of data pointed to by data_in.
			rcdata.data_out		 = nubuffer; // pointer to the output data samples.
			rcdata.output_frames = nusize;	// Maximum number of frames pointed to by data_out.
			rcdata.src_ratio	 = src_ratio;	// output_sample_rate / input_sample_rate.

			if (src_process (rc, &rcdata) != 0) {
				LOG_ERROR("rate converter failed");
				throw;
			}
			if (progdefaults.mon_dsp_audio) {
				double out;
				for (int n = 0; n < nusize; n++) {
					if (bpfilt->Irun(nubuffer[2*n], out)) {
						nubuffer[2*n] = nubuffer[2*n+1] = gain * out; 
					}
				}
			}
			monitor_rb->write(nubuffer, 2 * rcdata.output_frames_gen);
		}

	} catch (...) {
		delete [] nubuffer;
		nubuffer = 0;
		alert_process_flag = false;
		throw;
	}
	delete [] nubuffer;
	nubuffer = 0;
	alert_process_flag = false;

	return;
}

void c_portaudio::mon_write(double *buffer, int len, int mon_sr)
{
	if (!fbuffer)
		fbuffer = new float[2 * len];
	for (int i = 0; i < len; i++)
		fbuffer[2*i] = fbuffer[2*i+1] = 0.01 * progdefaults.RxFilt_vol * buffer[i];
	
	b_len = len;
	b_sr = mon_sr;

	alert_process_flag = true;
	requester = this;
	pthread_cond_signal(&alert_cond);
}

// read len elements of 2 channel audio from ring buffer
size_t c_portaudio::mon_read(float *buffer, int len)
{
	size_t ret = monitor_rb->read(buffer, 2*len);
	return ret;
}

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

		if (!filelist.empty()) {
			{
				guard_lock filelock(&filelist_mutex);
				fl = filelist.top();
				filelist.pop();
			}
			fl.cpa->do_play_file(fl.fn);
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

