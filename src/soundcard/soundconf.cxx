#include <config.h>

#if USE_PORTAUDIO
#  include <map>
#  include <list>
#endif

#include <cstdlib>
#include <cstring>

#include "soundconf.h"
#include "sound.h"
#include "main.h"
#include "configuration.h"
#include "confdialog.h"

using namespace std;

double std_sample_rates[] = { 8000.0, 9600.0, 11025.0, 12000.0, 16000.0, 22050.0, 24000.0,
			      32000.0, 44100.0, 48000.0, 88200.0, 96000.0, 192000.0, -1.0 };

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
	int idx;

	idx = -1;
	menu = menuPortInDev->menu();
	size = menuPortInDev->size();
	for (int i = 0; i < size - 1; i++, menu++) {
		if (menu->label() && progdefaults.PortInDevice == menu->label()) {
			idx = i; // near match
			if (reinterpret_cast<intptr_t>(menu->user_data()) == progdefaults.PortInIndex ||
			    progdefaults.PortInIndex == -1) // exact match, or index was never saved
				break;
		}
	}
	if (idx >= 0) {
		menuPortInDev->value(idx);
		menuPortInDev->set_changed();
	}

	idx = -1;
	menu = menuPortOutDev->menu();
	size = menuPortOutDev->size();
	for (int i = 0; i < size - 1; i++, menu++) {
		if (menu->label() && progdefaults.PortOutDevice == menu->label()) {
			idx = i;
			if (reinterpret_cast<intptr_t>(menu->user_data()) == progdefaults.PortOutIndex ||
			    progdefaults.PortOutIndex == -1)
				break;
		}
	}
	if (idx >= 0) {
		menuPortOutDev->value(idx);
		menuPortOutDev->set_changed();
	}
#endif
}

static void build_srate_menu(Fl_Menu_* menu, const double* rates, size_t length, double defrate = -1.0)
{
	menu->clear();
	menu->add("Auto");
	menu->add("Native", 0, 0, 0, FL_MENU_DIVIDER);

	char s[16];
	for (size_t i = 0; i < length; i++) {
		if (defrate != rates[i])
			snprintf(s, sizeof(s), "%.0f", rates[i]);
		else
			snprintf(s, sizeof(s), "%.0f (native)", rates[i]);
		menu->add(s);
	}
}

static void sound_init_options(void)
{
	build_srate_menu(menuInSampleRate, std_sample_rates,
			 sizeof(std_sample_rates)/sizeof(*std_sample_rates) - 1);
	build_srate_menu(menuOutSampleRate, std_sample_rates,
			 sizeof(std_sample_rates)/sizeof(*std_sample_rates) - 1);

	const char* cname;
	for (int i = 0; (cname = src_get_name(i)); i++) {
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
#if USE_OSS
	case SND_IDX_OSS:
		menuOSSDev->activate();
		scDevice[0] = scDevice[1] = menuOSSDev->value();
		break;
#endif

#if USE_PORTAUDIO
	case SND_IDX_PORT:
		menuPortInDev->activate();
		menuPortOutDev->activate();
		if (menuPortInDev->text())
			scDevice[0] = menuPortInDev->text();
		if (menuPortOutDev->text())
			scDevice[1] = menuPortOutDev->text();

		{
			Fl_Menu_* menus[2] = { menuInSampleRate, menuOutSampleRate };
			for (size_t i = 0; i < 2; i++) {
				char* label = strdup(menus[i]->text());
				const vector<double>& srates = SoundPort::get_supported_rates(scDevice[i], i);

				switch (srates.size()) {
				case 0: // startup; no devices initialised yet
					build_srate_menu(menus[i], std_sample_rates,
							 sizeof(std_sample_rates)/sizeof(*std_sample_rates) - 1);
					break;
				case 1: // default sample rate only, build menu with all std rates
					build_srate_menu(menus[i], std_sample_rates,
							 sizeof(std_sample_rates)/sizeof(*std_sample_rates) - 1, srates[0]);

					break;
				default: // first element is default sample rate, build menu with rest
					build_srate_menu(menus[i], &srates[0] + 1, srates.size() - 1, srates[0]);
					break;
				}

				for (const Fl_Menu_Item* item = menus[i]->menu(); item->text; item++) {
					if (strstr(item->text, label)) {
						menus[i]->value(item);
						break;
					}
				}
				free(label);

				menus[i]->activate();
			}
		}
		break;
#endif

#if USE_PULSEAUDIO
	case SND_IDX_PULSE:
		inpPulseServer->activate();
		scDevice[0] = scDevice[1] = inpPulseServer->value();
		break;
#endif

	case SND_IDX_NULL:
		scDevice[0] = scDevice[1] = "";
		break;
	};
}
