#include "mixer.h"

cMixer::cMixer() {
	strcpy (szDevice, "/dev/mixerX");
	mixer = "/dev/mixer";
	mixer_fd		= -1;
	findNumMixers();
}

cMixer::~cMixer()
{
}

//=======================================
// mixer methods
//=======================================
void cMixer::openMixer(const char *dev)
{
	int err;
	if (mixer_fd != -1) closeMixer();
	mixer = dev;
	try {
		mixer_fd =  open(mixer.c_str(), O_RDWR);
		if (mixer_fd == -1)
			throw MixerException(errno);
		if ((err = initMask()) != 0)
			throw MixerException(err);
		SetPlaythrough(0.0);
	} 
	catch (...) {
		throw;
	}
	inpsrc0 = GetCurrentInputSource();
	inplevel0 = InputVolume();
	pcmlevel0 = PCMVolume();
	vollevel0 = OutVolume();
//	std::cout << "Input = " << inpsrc0 << ", " << GetInputSourceName(inpsrc0) << std::endl;
//	std::cout << "Input Level = " << inplevel0 << std::endl;
//	std::cout << "Pcm Level = " << pcmlevel0 << std::endl;
//	std::cout << "Output Level = " << vollevel0 << std::endl;
}

void cMixer::closeMixer()
{
	if (mixer_fd == -1) return;
	PCMVolume(pcmlevel0);
	OutVolume(vollevel0);
	InputVolume(inplevel0);
	SetCurrentInputSource(inpsrc0);
	close(mixer_fd);
	mixer_fd = -1;
}

void cMixer::findNumMixers()
{
	int fd;
	NumMixers = 0;
	for (int i = 0; i< 11; i++) {
		if (i == 0)
			szDevice[10] = 0;
		else
			szDevice[10] = '0'+(i-1);
		fd = open(szDevice, O_RDWR);
		if (fd >= 0) {
			Devices[NumMixers] = i;
			NumMixers++;
			close(fd);
		}
   }
}

const char * cMixer::MixerName( int index )
{
	if (NumMixers <= 0)
		findNumMixers();

	if (index < 0 || index >= NumMixers)
		return NULL;

	if (Devices[index] == 0)
		szDevice[10] = 0;
	else
		szDevice[10] = '0' + (Devices[index]-1);
	return szDevice;
}

void cMixer::setXmtLevel(double v)
{
	if (mixer_fd == -1) return;
	OutVolume(v);
}

void cMixer::setRcvGain(double v)
{
	if (mixer_fd == -1) return;
	InputVolume(v);
}

int cMixer::initMask()
{
	if (mixer_fd == -1) return -1;
	
	if((ioctl(mixer_fd, MIXER_READ(SOUND_MIXER_READ_DEVMASK), &devmask)) == -1) 
		return errno;
//	std::cout << "dev mask " << devmask << std::endl;
	if((ioctl(mixer_fd, MIXER_READ(SOUND_MIXER_READ_RECMASK), &recmask)) == -1) 
		return errno;
//	std::cout << "rec mask " << recmask << std::endl;
	if ((ioctl(mixer_fd, MIXER_READ(SOUND_MIXER_READ_RECSRC), &recsrc)) == -1)
		return errno;
//	std::cout << "recsrc mask " << recsrc << std::endl;
		
	outmask = devmask ^ recmask;

	num_out = 0;
	num_rec = 0;
	for( int i = 0; i < SOUND_MIXER_NRDEVICES; i++) {
		if (recmask & (1 << i)) {
			recs[num_rec++] = i;
//			std::cout << "Input channel " << GetInputSourceName(i) << " is active\n";
		}
		else if (devmask & (1<<i)) {
//		if (devmask & (1<<i)) {
			outs[num_out++] = i;
//			std::cout << "Output channel " << OutputVolumeName(i) << " is available\n";
		}
	}
	return 0;
}

// returns value between 0.0 and 1.0
double cMixer::ChannelVolume(int channel)
{
	int vol;
	int stereo;

	if (ioctl(mixer_fd, SOUND_MIXER_READ_STEREODEVS, &stereo) == 0)
		stereo = ((stereo & (1 << channel)) != 0);
	else
		stereo = 0;
	if (ioctl(mixer_fd, MIXER_READ(channel), &vol) == -1)
		return 0.0;

	if (stereo)
		return ((vol & 0xFF)/200.0) + (((vol>>8) & 0xFF)/200.0);
	else
		return (vol & 0xFF)/100.0;
}

/*
 Master (output) volume
*/

double cMixer::OutVolume()
{
	if (mixer_fd == -1) return 0.0;

	return ChannelVolume(SOUND_MIXER_VOLUME);
}

void cMixer::OutVolume(double volume)
{
	if (mixer_fd == -1) return;
	int vol = (int)((volume * 100.0) + 0.5);
	vol = (vol | (vol<<8));
	ioctl(mixer_fd, MIXER_WRITE(SOUND_MIXER_VOLUME), &vol);
}

double cMixer::PCMVolume()
{
	if (mixer_fd == -1) return 0.0;
	return ChannelVolume(SOUND_MIXER_PCM);
}

void cMixer::PCMVolume(double volume )
{
	if (mixer_fd == -1) return;
	
	int vol = (int)((volume * 100.0) + 0.5);
	vol = (vol | (vol<<8));
	ioctl(mixer_fd, MIXER_WRITE(SOUND_MIXER_PCM), &vol);
}

int cMixer::NumOutputVolumes()
{
	return num_out;
}

const char *cMixer::OutputVolumeName( int i )
{
	const char *labels[] = SOUND_DEVICE_LABELS;
	return labels[outs[i]];
}

double cMixer::OutputVolume( int i )
{
	return ChannelVolume(outs[i]);
}

void cMixer::OutputVolume( int i, double volume )
{
	int vol = (int)((volume * 100.0) + 0.5);
	vol = (vol | (vol<<8));
	ioctl(mixer_fd, MIXER_WRITE(outs[i]), &vol);
}

int cMixer::GetNumInputSources()
{
	return num_rec;
}

const char *cMixer::GetInputSourceName( int i)
{
	const char *labels[] = SOUND_DEVICE_LABELS;
	return labels[recs[i]];
}

int cMixer::InputSourceNbr(char *source)
{
	const char *labels[] = SOUND_DEVICE_LABELS;
	char lbl[80];
	int len;
	for (int i = 0; i < num_rec; i++) {
		strcpy(lbl, labels[recs[i]]);
		len = strlen(lbl);
		while (len > 0 && lbl[len-1] == ' ') {
			lbl[len-1] = 0;
			len--;
		}
		if (!strncasecmp(lbl, source, strlen(lbl) ) )
			return i;
	}
  return -1;
}

int cMixer::GetCurrentInputSource()
{
//	int recmask;
   
	if (mixer_fd == -1) return -1;

//	if (ioctl(mixer_fd, MIXER_READ(SOUND_MIXER_READ_RECSRC), &recmask) == -1) {
//		std::cout << "Error reading Record Source\n";
//		return -1; /* none / error */
//	}
	for(int i = 0; i < num_rec; i++)
		if (recmask & (1 << (recs[i])))
			return i;
//	std::cout << "Cannot find input source\n";
	return -1; /* none */
}

void cMixer::SetCurrentInputSource( int i )
{
	if (mixer_fd == -1) return;
	int newrecsrcmask = (1 << (recs[i]));
	ioctl(mixer_fd, MIXER_WRITE(SOUND_MIXER_READ_RECSRC), &newrecsrcmask);
}

/*
 Input volume
*/

double cMixer::InputVolume()
{
	if (mixer_fd == -1) return 0.0;
	int i = GetCurrentInputSource();
	if (i < 0)
		return 0.0;
	return ChannelVolume(SOUND_MIXER_IGAIN);
}

void cMixer::InputVolume( double volume )
{
	int vol;
//	int i = GetCurrentInputSource();
   
//	if (i < 0)
//		return;

	vol = (int)((volume * 100.0) + 0.5);
	vol = (vol | (vol<<8));
	ioctl(mixer_fd, MIXER_WRITE(SOUND_MIXER_IGAIN), &vol);
}


double cMixer::GetPlaythrough()
{
	int i = GetCurrentInputSource();
	if (i < 0)
		return 0.0;
	return ChannelVolume(recs[i]);
}

void cMixer::SetPlaythrough( double volume )
{
	if (mixer_fd == -1) return;
	
	int vol;
	int i = GetCurrentInputSource();
	if (i < 0)
		return;

	vol = (int)((volume * 100.0) + 0.5);
	vol = (vol | (vol<<8));
	ioctl(mixer_fd, MIXER_WRITE(recs[i]), &vol);
}

