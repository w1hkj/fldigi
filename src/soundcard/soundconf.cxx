#include <config.h>

#if USE_PORTAUDIO
#  include <map>
#  include <list>
#endif

#include "soundconf.h"
#include "sound.h"
#include "main.h"
#include "configuration.h"
#include "confdialog.h"

using namespace std;


static void init_oss(void)
{
#if USE_OSS
	glob_t gbuf;
	glob("/dev/dsp*", 0, NULL, &gbuf);
	for (size_t i = 0; i < gbuf.gl_pathc; i++)
		menuOSSDev->add(gbuf.gl_pathv[i]);
	if (progdefaults.OSSdevice.length() == 0 && gbuf.gl_pathc)
		progdefaults.OSSdevice = gbuf.gl_pathv[0];
	menuOSSDev->value(progdefaults.OSSdevice.c_str());
	globfree(&gbuf);

	glob("/dev/mixer*", 0, NULL, &gbuf);
	for (size_t i = 0; i < gbuf.gl_pathc; i++)
		menuMix->add(gbuf.gl_pathv[i]);
	if (progdefaults.MXdevice.length() == 0 && gbuf.gl_pathc)
		progdefaults.MXdevice = gbuf.gl_pathv[0];
	globfree(&gbuf);
	menuMix->value(progdefaults.MXdevice.c_str());
#else
	progdefaults.EnableMixer = false;
        tabMixer->deactivate();
#endif // USE_OSS
}


#if USE_PORTAUDIO

map<PaHostApiTypeId, unsigned> pa_api_prio;

struct padev
{
public:
	padev(const PaDeviceInfo* dev_, PaDeviceIndex idx_, PaHostApiTypeId api_)
		: dev(dev_), idx(idx_), api(api_) { }

	bool operator<(const padev& rhs) const
	{
		return pa_api_prio.find(api) != pa_api_prio.end() &&
			pa_api_prio.find(rhs.api) != pa_api_prio.end() &&
			pa_api_prio[api] < pa_api_prio[rhs.api];
	}

	const PaDeviceInfo* dev;
	PaDeviceIndex idx;
	PaHostApiTypeId api;
};

#endif // USE_PORTAUDIO


static void init_portaudio(void)
{
#if USE_PORTAUDIO
	try {
		SoundPort::initialize();
	}
	catch (const SndPortException& e) {
		cerr << e.what() << endl;
	       	AudioPort->deactivate();
		btnAudioIO[SND_IDX_PORT]->deactivate();
		progdefaults.btnAudioIOis = SND_IDX_NULL;
		return;
	}

	pa_api_prio.clear();
#if defined(__APPLE__)
	pa_api_prio[paASIO] = 0;
	pa_api_prio[paCoreAudio] = 1;
#elif defined(__CYGWIN__)
	pa_api_prio[paASIO] = 0;
	pa_api_prio[paWASAPI] = 1;
	pa_api_prio[paDirectSound] = 2;
	pa_api_prio[paMME] = 3;
#else
	pa_api_prio[paALSA] = 0;
	pa_api_prio[paJACK] = 1;
	pa_api_prio[paOSS] = 2;
#endif

	list<padev> devlist;
	for (SoundPort::device_iterator idev = SoundPort::devices().begin();
	     idev != SoundPort::devices().end(); ++idev)
		devlist.push_back( padev(*idev, idev - SoundPort::devices().begin(),
					 Pa_GetHostApiInfo((*idev)->hostApi)->type) );
	devlist.sort();

	PaHostApiTypeId first_api = devlist.begin()->api;
	for (list<padev>::const_iterator ilist = devlist.begin();
	     ilist != devlist.end(); ilist++) {
		string menu_item;
		string::size_type i = 0;
		if (ilist->api != first_api) { // add a submenu
			menu_item.append(Pa_GetHostApiInfo(ilist->dev->hostApi)->name).append(" devices/");
			i = menu_item.length();
		}
		menu_item.append(ilist->dev->name);
		// backslash-escape any slashes in the device name
		while ((i = menu_item.find('/', i)) != string::npos) {
			menu_item.insert(i, 1, '\\');
			i += 2;
		}
		// add to menu
		if (ilist->dev->maxInputChannels > 0)
			menuPortInDev->add(menu_item.c_str(), 0, NULL,
					   reinterpret_cast<void *>(ilist->idx), 0);
		if (ilist->dev->maxOutputChannels > 0)
			menuPortOutDev->add(menu_item.c_str(), 0, NULL,
					    reinterpret_cast<void *>(ilist->idx), 0);
	}

	if (progdefaults.PortInDevice.length() == 0) {
		if (progdefaults.PAdevice.length() == 0) {
			PaDeviceIndex def = Pa_GetDefaultInputDevice();
			if (def != paNoDevice) {
				progdefaults.PortInDevice = (*(SoundPort::devices().begin() + def))->name;
				progdefaults.PortInIndex = def;
			}
		}
		else
			progdefaults.PortInDevice = progdefaults.PAdevice;
	}

	if (progdefaults.PortOutDevice.length() == 0) {
		if (progdefaults.PAdevice.length() == 0) {
			PaDeviceIndex def = Pa_GetDefaultOutputDevice();
			if (def != paNoDevice) {
				progdefaults.PortOutDevice = (*(SoundPort::devices().begin() + def))->name;
				progdefaults.PortOutIndex = def;
			}
		}
		else
			progdefaults.PortOutDevice = progdefaults.PAdevice;
	}

	// select the correct menu items
	const Fl_Menu_Item* menu;
	int size;

	menu = menuPortInDev->menu();
	size = menuPortInDev->size();
	for (int i = 0; i < size - 1; i++, menu++) {
		if (menu->label() && progdefaults.PortInDevice == menu->label()) {
			if (progdefaults.PortInIndex != -1 &&
			    reinterpret_cast<intptr_t>(menu->user_data()) != progdefaults.PortInIndex)
				continue;
			menuPortInDev->value(i);
			menuPortInDev->set_changed();
			break;
		}
	}

	menu = menuPortOutDev->menu();
	size = menuPortOutDev->size();
	for (int i = 0; i < size - 1; i++, menu++) {
		if (menu->label() && progdefaults.PortOutDevice == menu->label()) {
			if (progdefaults.PortOutIndex != -1 &&
			    reinterpret_cast<intptr_t>(menu->user_data()) != progdefaults.PortOutIndex)
				continue;
			menuPortOutDev->value(i);
			menuPortOutDev->set_changed();
			break;
		}
	}
#endif
}

static void sound_init_options(void)
{
	static const Fl_Menu_Item sample_rate_menu[] = {
		{ "Auto" }, { "Native", 0, 0, 0, FL_MENU_DIVIDER },
		{ "8000" }, { "9600" }, { "11025" }, { "12000" }, { "16000" },
		{ "22050" }, { "24000" }, { "32000" }, { "44100" }, { "48000" },
		{ "88200" }, { "96000" }, { "192000" }, { 0 }
	};
	menuInSampleRate->menu(sample_rate_menu);
	menuOutSampleRate->menu(sample_rate_menu);

	const char* cname;
	for (int i = 0; (cname = src_get_name(i)); i++) {
		if (strstr( cname, "ZOH") != 0) continue;
		if (strstr( cname, "Linear") != 0) continue;
		menuSampleConverter->add(cname);
	}
	menuSampleConverter->value(progdefaults.sample_converter);
	menuSampleConverter->tooltip(src_get_description(progdefaults.sample_converter));


	valPCMvolume->value(progdefaults.PCMvolume);
	btnMicIn->value(progdefaults.MicIn);
	btnLineIn->value(progdefaults.LineIn);

	menuOSSDev->value(progdefaults.OSSdevice.c_str());
	inpPulseServer->value(progdefaults.PulseServer.c_str());

	btnMixer->value(progdefaults.EnableMixer);
	menuMix->value(progdefaults.MXdevice.c_str());

	enableMixer(progdefaults.EnableMixer);

	char sr[6+1];
	if (progdefaults.in_sample_rate == SAMPLE_RATE_UNSET &&
		(progdefaults.in_sample_rate = progdefaults.sample_rate) == SAMPLE_RATE_UNSET)
		progdefaults.in_sample_rate = SAMPLE_RATE_NATIVE;
	else if (progdefaults.in_sample_rate > SAMPLE_RATE_OTHER)
		snprintf(sr, sizeof(sr), "%d", progdefaults.in_sample_rate);
	if (progdefaults.in_sample_rate <= SAMPLE_RATE_NATIVE)
		menuInSampleRate->value(progdefaults.in_sample_rate);
	else
		menuInSampleRate->value(menuInSampleRate->find_item(sr));

	if (progdefaults.out_sample_rate == SAMPLE_RATE_UNSET &&
		(progdefaults.out_sample_rate = progdefaults.sample_rate) == SAMPLE_RATE_UNSET)
		progdefaults.out_sample_rate = SAMPLE_RATE_NATIVE;
	else if (progdefaults.out_sample_rate > SAMPLE_RATE_OTHER)
		snprintf(sr, sizeof(sr), "%d", progdefaults.out_sample_rate);
	if (progdefaults.out_sample_rate <= SAMPLE_RATE_NATIVE)
		menuOutSampleRate->value(progdefaults.out_sample_rate);
	else
		menuOutSampleRate->value(menuOutSampleRate->find_item(sr));

	cntRxRateCorr->value(progdefaults.RX_corr);
	cntTxRateCorr->value(progdefaults.TX_corr);
	cntTxOffset->value(progdefaults.TxOffset);
}

void sound_init(void)
{
	init_oss();

	init_portaudio();

// set the Sound Card configuration tab to the correct initial values
#if !USE_OSS
	AudioOSS->deactivate();
	btnAudioIO[SND_IDX_OSS]->deactivate();
#endif
#if !USE_PORTAUDIO
	AudioPort->deactivate();
	btnAudioIO[SND_IDX_PORT]->deactivate();
#endif
#if !USE_PULSEAUDIO
	AudioPulse->deactivate();
	btnAudioIO[SND_IDX_PULSE]->deactivate();
#endif
	if (progdefaults.btnAudioIOis == SND_IDX_UNKNOWN ||
	    !btnAudioIO[progdefaults.btnAudioIOis]->active()) { // or saved sound api now disabled
		for (size_t i = 0; i < sizeof(btnAudioIO)/sizeof(*btnAudioIO); i++) {
			if (btnAudioIO[i]->active()) {
				progdefaults.btnAudioIOis = i;
				break;
			}
		}
	}

	sound_init_options();

	sound_update(progdefaults.btnAudioIOis);

	resetMixerControls();
}

void sound_close(void)
{
#if USE_PORTAUDIO
	SoundPort::terminate();
#endif
}

void sound_update(unsigned idx)
{
	// radio button
	for (size_t i = 0; i < sizeof(btnAudioIO)/sizeof(*btnAudioIO); i++)
		btnAudioIO[i]->value(i == idx);

	// devices
	menuOSSDev->deactivate();
	menuPortInDev->deactivate();
	menuPortOutDev->deactivate();
	inpPulseServer->deactivate();

	// settings
	menuInSampleRate->deactivate();
	menuOutSampleRate->deactivate();

	progdefaults.btnAudioIOis = idx;
	switch (idx) {
	case SND_IDX_OSS:
		menuOSSDev->activate();
		scDevice[0] = scDevice[1] = menuOSSDev->value();
		break;
	case SND_IDX_PORT:
		menuPortInDev->activate();
		menuPortOutDev->activate();
		menuInSampleRate->activate();
		menuOutSampleRate->activate();
		if (menuPortInDev->text())
			scDevice[0] = menuPortInDev->text();
		if (menuPortOutDev->text())
			scDevice[1] = menuPortOutDev->text();
		break;
	case SND_IDX_PULSE:
		inpPulseServer->activate();
		menuInSampleRate->activate();
		menuOutSampleRate->activate();
		scDevice[0] = scDevice[1] = inpPulseServer->value();
		break;
	case SND_IDX_NULL:
		scDevice[0] = scDevice[1] = "";
		break;
	};
}
