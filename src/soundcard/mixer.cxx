#include <config.h>

#include "mixer.h"
#include "configuration.h"

cMixer::cMixer() {
	strcpy (szDevice, "/dev/mixerX");
	mixer = "/dev/mixer";
	mixer_fd		= -1;
	findNumMixers();
}

cMixer::~cMixer()
{
	closeMixer();
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
	} 
	catch (...) {
		throw;
	}
	initValues();
}

void cMixer::closeMixer()
{
	if (mixer_fd == -1) return;
	restoreValues();
	close(mixer_fd);
	mixer_fd = -1;
}

void cMixer::initValues()
{
	int devnbr;

	inpsrc0 = GetCurrentInputSource();

	devnbr = InputSourceNbr("Line");
	SetCurrentInputSource(devnbr);
	linelevel0 = InputVolume();

	devnbr = InputSourceNbr("Mic");
	SetCurrentInputSource(devnbr);	
	miclevel0 = InputVolume();
	
	pcmlevel0 = PCMVolume();
	vollevel0 = OutVolume();
/*
	std::cout << "Sound card initial state:" << std::endl;
	std::cout << "  Dev mask " << hex << devmask << std::endl;
	std::cout << "  Rec mask " << hex << recmask << std::endl;
	std::cout << "  Rec src  " << hex << recsrc << std::endl;
	std::cout << "  Current input source # " << GetInputSourceName(inpsrc0) << std::endl;
	std::cout << "  Line Level = " << linelevel0 << std::endl;
	std::cout << "  Mic  Level = " << miclevel0  << std::endl;
	std::cout << "  Pcm  Level = " << pcmlevel0  << std::endl;
	std::cout << "  Vol  Level = " << vollevel0  << std::endl;
*/
}

void cMixer::restoreValues()
{
	int devnbr;
	devnbr = InputSourceNbr("Line");
	SetCurrentInputSource(devnbr);
	InputVolume(linelevel0);
	
	devnbr = InputSourceNbr("Mic");
	SetCurrentInputSource(devnbr);
	InputVolume(miclevel0);
	
	PCMVolume(pcmlevel0);
	OutVolume(vollevel0);

//	SetCurrentInputSource(inpsrc0);
	ioctl(mixer_fd, MIXER_WRITE(SOUND_MIXER_READ_RECSRC), &recsrc0);
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

	if((ioctl(mixer_fd, MIXER_READ(SOUND_MIXER_READ_RECMASK), &recmask)) == -1) 
		return errno;

	if ((ioctl(mixer_fd, MIXER_READ(SOUND_MIXER_READ_RECSRC), &recsrc)) == -1)
		return errno;
		
	devmask0 = devmask;
	recmask0 = recmask;
	recsrc0 = recsrc;
		
	outmask = devmask ^ recmask;

	num_out = 0;
	num_rec = 0;
	for( int i = 0; i < SOUND_MIXER_NRDEVICES; i++) {
		if (recmask & (1 << i)) {
			recs[num_rec++] = i;
		}
		else if (devmask & (1<<i)) {
			outs[num_out++] = i;
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

int cMixer::InputSourceNbr(const char *source)
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
	if (mixer_fd == -1) return -1;
	for(int i = 0; i < num_rec; i++)
		if (recsrc & (1 << (recs[i])))
			return i;
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
//	int i = GetCurrentInputSource();
//	if (i < 0)
//		return 0.0;
	return ChannelVolume(SOUND_MIXER_IGAIN);
}

void cMixer::InputVolume( double volume )
{
	int vol;
	vol = (int)((volume * 100.0) + 0.5);
	vol = (vol | (vol<<8));
	ioctl(mixer_fd, MIXER_WRITE(SOUND_MIXER_IGAIN), &vol);
}

/*
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

void cMixer::SetMuteInput(bool b)
{
	return;
	if (b == 1)
		SetPlaythrough(0.0);
	else
		SetPlaythrough(playthrough0);
}

*/
