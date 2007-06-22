#include "sound.h"
#include "configuration.h"

cSound::cSound(const char *dev ) {
	device			= dev;
	capture = playback = generate = false;

	tx_src_state = 0;
	tx_src_data = 0;
	rx_src_state = 0;
	rx_src_data = 0;
	cbuff = 0;
	snd_buffer = 0;
	src_buffer = 0;
	txppm = progdefaults.TX_corr;
	rxppm = progdefaults.RX_corr;
	
	try {
		Open(O_RDONLY);
		getVersion();
		getCapabilities();
		getFormats();
		Close();
	}
	catch (SndException e) {
		std::cout << e.szError - 1 
			 << " <" << device.c_str()
			 << ">" << std::endl;
    }
}

cSound::~cSound()
{
	Close();
	if (cbuff) delete [] cbuff;
	if (snd_buffer) delete [] snd_buffer;
	if (src_buffer) delete [] src_buffer;
	if (tx_src_data) delete tx_src_data;
	if (rx_src_data) delete rx_src_data;
	if (rx_src_state) src_delete (rx_src_state);
	if (tx_src_state) src_delete (tx_src_state);
	tx_src_state = 0;
	tx_src_data = 0;
	rx_src_state = 0;
	rx_src_data = 0;
	cbuff = 0;
	snd_buffer = 0;
	src_buffer = 0;
}

void cSound::setfragsize()
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
		throw errno;
}

int cSound::Open(int md, int freq, int nchan)
{
	mode = md;
	try {
		device_fd = open(device.c_str(), mode, 0);
		if (device_fd == -1)
			throw SndException(errno);
		Format(AFMT_S16_LE);	// default: 16 bit little endian
		Channels(nchan);		// 2 channels is default
		Frequency(freq);		// 8000 Hz is default
		setfragsize();
	} 
	catch (...) {
		throw;
	}

	if (cbuff) delete [] cbuff;
	if (snd_buffer) delete [] snd_buffer;
	if (src_buffer) delete [] src_buffer;
	if (tx_src_data) delete tx_src_data;
	if (rx_src_data) delete rx_src_data;
	if (rx_src_state) src_delete (rx_src_state);
	if (tx_src_state) src_delete (tx_src_state);

	try {
		int err;
		snd_buffer	= new float [channels * SND_BUF_LEN];
		src_buffer	= new float [channels * SND_BUF_LEN];
		cbuff 		= new unsigned char [channels * 2 * SND_BUF_LEN];
		if (!snd_buffer || !src_buffer || !cbuff)
			throw("Cannot create src buffers");
		for (int i = 0; i < channels * SND_BUF_LEN; i++)
			snd_buffer[i] = src_buffer[i] = 0.0;
		for (int i = 0; i < channels * 2 * SND_BUF_LEN; i++)
			cbuff[i] = 0;

		tx_src_data = new SRC_DATA;
		rx_src_data = new SRC_DATA;
		if (!tx_src_data || !rx_src_data)
			throw("Cannot create source data structures");
			
		rx_src_state = src_new(SRC_SINC_FASTEST, channels, &err);
		if (rx_src_state == 0)
			throw(src_strerror(err));
			
		tx_src_state = src_new(SRC_SINC_FASTEST, channels, &err);
		if (tx_src_state == 0)
			throw(src_strerror(err));
			
		rx_src_data->src_ratio = 1.0/(1.0 + rxppm/1e6);
		src_set_ratio ( rx_src_state, 1.0/(1.0 + rxppm/1e6));
		
		tx_src_data->src_ratio = 1.0 + txppm/1e6;
		src_set_ratio ( tx_src_state, 1.0 + txppm/1e6);
	}
	catch (SndException){
		exit(1);
	};

	return device_fd;
}

void cSound::Close()
{
	if (device_fd == -1)
		return;
	close(device_fd);
	device_fd = -1;
}

void cSound::getVersion()
{
	version = 0;
#ifndef __FreeBSD__
 	if (ioctl(device_fd, OSS_GETVERSION, &version) == -1) {
 		version = -1;
 		throw SndException("OSS Version");
 	}
#endif
}

void cSound::getCapabilities()
{
	capability_mask = 0;
	if (ioctl(device_fd, SNDCTL_DSP_GETCAPS, &capability_mask) == -1) {
		capability_mask = 0;
		throw SndException("OSS capabilities");
	}
}

void cSound::getFormats()
{
	format_mask = 0;
	if (ioctl(device_fd, SNDCTL_DSP_GETFMTS, &format_mask) == -1) {
		format_mask = 0;
		throw SndException("OSS formats");
	}
}

void cSound::Format(int format)
{
	play_format = format;
	if (ioctl(device_fd, SNDCTL_DSP_SETFMT, &play_format) == -1) {
		device_fd = -1;
		formatok = false;
		throw SndException("Unsupported snd card format");
    }
	formatok = true;
}

void cSound::Channels(int nuchannels)
{
	channels = nuchannels;
	if (ioctl(device_fd, SNDCTL_DSP_CHANNELS, &channels) == -1) {
		device_fd = -1;
		throw "Snd card channel request failed";
	}
}

void cSound::Frequency(int frequency)
{	
	sample_frequency = frequency;
	if (ioctl(device_fd, SNDCTL_DSP_SPEED, &sample_frequency) == -1) {
		device_fd = -1;
		throw SndException("Cannot set frequency");
    }
}

int cSound::BufferSize( int seconds )
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

bool cSound::wait_till_finished()
{
	if (ioctl(device_fd, SNDCTL_DSP_POST, 1) == -1 )
		return false;
	if (ioctl(device_fd, SNDCTL_DSP_SYNC, 0) == -1)
		return false; /* format (or ioctl()) not supported by device */
	return true; /* all sound has been played */
}

bool cSound::reset_device()
{
	if (ioctl(device_fd, SNDCTL_DSP_RESET, 0) == -1) {
		device_fd = -1;
		return false; /* format (or ioctl()) not supported by device */
    }
	return 1; /* sounddevice has been reset */
}

int cSound::Write(unsigned char *buffer, int buffersize)
{
	if (device_fd == -1)
		return -1;
	return write (device_fd, buffer, buffersize);
}

int cSound::Read(unsigned char *buffer, int buffersize)
{
	if (device_fd == -1)
		return -1;
	
	return read (device_fd, buffer, buffersize);
}

int cSound::Read(double *buffer, int buffersize)
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
	else if (playback) readPlayback( buffer, buffersize);
		
	if (rxppm == 0 || playback)
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

int cSound::write_samples(double *buf, int count)
{
	int retval;
	short int *wbuff;
	unsigned char *p;

	if (generate) writeGenerate( buf, count);
	
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

int cSound::write_stereo(double *bufleft, double *bufright, int count)
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


void cSound::Capture(bool on) 
{
	if (on)
		ofCapture.open("capture.snd");
	else
		ofCapture.close();
	capture = on;
}

void cSound::Playback(bool on) 
{
//	std::cout << "Playback " << on << endl;
	if (on) {
		ifPlayback.open("playback.snd", ios::in | ios::binary);
		if (ifPlayback.is_open() == true)
			playback = true;
		return;
	} else
		ifPlayback.close();
	playback = false;
}

void cSound::Generate(bool on) 
{
	if (on)
		ofGenerate.open("generate.snd");
	else
		ofGenerate.close();
	generate = on;
}

void cSound::writeGenerate(double *buff, int count)
{
	char *cbuff = (char *)buff;
	ofGenerate.write(cbuff, count * sizeof(double) );
}

void cSound::writeCapture(double *buff, int count)
{
	char *cbuff = (char *)buff;
	ofCapture.write(cbuff, count * sizeof(double) );
}

int  cSound::readPlayback(double *buff, int count)
{
	char *cbuff = (char *)buff;
	if (ifPlayback.eof() == true) {
		ifPlayback.close();
		ifPlayback.open("playback.snd", ios::in | ios::binary);
	}
	ifPlayback.read(cbuff, count * sizeof(double) );
//	std::cout << count << " " << ifPlayback.tellg() << endl;

	return count;
}

