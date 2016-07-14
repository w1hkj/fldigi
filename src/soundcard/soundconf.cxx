// ----------------------------------------------------------------------------
// soundconf.cxx
//
// Copyright (C) 2008-2010, Stelios Bounanos, M0GLD
// Copyright (C) 2014       David Freese, W1HKJ
// Copyright (C) 2015       Robert Stiles, KK5VD
//
// This file is part of fldigi.
//
// Fldigi is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#include <config.h>

#if USE_PORTAUDIO
#  include <map>
#  include <list>
#endif

#include <cstdlib>
#include <cstring>
#include <string>
#if USE_OSS
#  include <glob.h>
#endif

#include "soundconf.h"
#include "sound.h"
#include "main.h"
#include "configuration.h"
#include "confdialog.h"
#include "debug.h"
#include "util.h"

LOG_FILE_SOURCE(debug::LOG_AUDIO);

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

static PaDeviceIndex get_default_portaudio_device(int dir)
{
#ifndef __linux__
	goto ret_def;
#else
	// Recent PortAudio snapshots prefer ALSA over OSS for the default device, but there are
	// still versions out there that try OSS first.  We check the default host api type and,
	// if it is not ALSA, return the ALSA default device instead.
	PaHostApiIndex api_idx;
	if ((api_idx = Pa_GetDefaultHostApi()) < 0)
		goto ret_def;
	const PaHostApiInfo* host_api;
	if ((host_api = Pa_GetHostApiInfo(api_idx)) == NULL || host_api->type == paALSA)
		goto ret_def;

	LOG_DEBUG("Default host API is %s, trying default ALSA %s device instead",
			  host_api->name, (dir == 0 ? "input" : "output"));
	api_idx = Pa_GetHostApiCount();
	if (api_idx < 0)
		goto ret_def;
	for (PaHostApiIndex i = 0; i < api_idx; i++)
		if ((host_api = Pa_GetHostApiInfo(i)) && host_api->type == paALSA)
			return dir == 0 ? host_api->defaultInputDevice : host_api->defaultOutputDevice;
#endif // __linux__

ret_def:
	return dir == 0 ? Pa_GetDefaultInputDevice() : Pa_GetDefaultOutputDevice();
}

#include <cerrno>

std::string str_pa_devices;

static void init_portaudio(void)
{
	try {
		SoundPort::initialize();
	}
	catch (const SndException& e) {
//		if (e.error() == ENODEV) // don't complain if there are no devices
//			return;
		str_pa_devices.assign("\nPortaudio devices init failure:");
		str_pa_devices.assign(e.what());
		AudioPort->deactivate();
		btnAudioIO[SND_IDX_PORT]->deactivate();
		if (progdefaults.btnAudioIOis == SND_IDX_PORT)
			progdefaults.btnAudioIOis = SND_IDX_NULL;
		return;
	}

	pa_api_prio.clear();
#if defined(__APPLE__)
	pa_api_prio[paASIO] = 0;
	pa_api_prio[paCoreAudio] = 1;
#elif defined(__WOE32__)
	pa_api_prio[paASIO] = 0;
	pa_api_prio[paWASAPI] = 1;
	pa_api_prio[paMME] = 2;
	pa_api_prio[paDirectSound] = 3;
#else
	pa_api_prio[paALSA] = 0;
	pa_api_prio[paJACK] = 1;
	pa_api_prio[paOSS] = 2;
#endif

	list<padev> devlist;
	int devnbr = 0;
	for (SoundPort::device_iterator idev = SoundPort::devices().begin();
		 idev != SoundPort::devices().end(); ++idev) {
		devlist.push_back( padev(*idev, idev - SoundPort::devices().begin(),
								 Pa_GetHostApiInfo((*idev)->hostApi)->type) );
		devnbr++;
	}
	devlist.sort();

	str_pa_devices.assign("\nPortaudio devices:\n");
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

		str_pa_devices.append(menu_item).append("\n");

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
			PaDeviceIndex def = get_default_portaudio_device(0);
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
			PaDeviceIndex def = get_default_portaudio_device(1);
			if (def != paNoDevice) {
				progdefaults.PortOutDevice = (*(SoundPort::devices().begin() + def))->name;
				progdefaults.PortOutIndex = def;
			}
		}
		else
			progdefaults.PortOutDevice = progdefaults.PAdevice;
	}

	// select the correct menu items
	pa_set_dev(menuPortInDev,  progdefaults.PortInDevice,  progdefaults.PortInIndex);
	pa_set_dev(menuPortOutDev, progdefaults.PortOutDevice, progdefaults.PortOutIndex);
}

int pa_set_dev(Fl_Choice *loc_choice, std::string loc_dev_name, int loc_dev_index)
{
	const Fl_Menu_Item *loc_menu = (Fl_Menu_Item *)0;
	int loc_size = 0;
	int loc_dev_found = PA_DEV_NOT_FOUND;
	int loc_idx = -1;

	if(!loc_choice) return loc_dev_found;

	loc_menu = loc_choice->menu();
	loc_size = loc_choice->size();

	for (int loc_i = 0; loc_i < loc_size - 1; loc_i++, loc_menu++) {
		if (loc_menu->label() && loc_dev_name == loc_menu->label()) {
			loc_idx = loc_i;
			loc_dev_found = PA_DEV_FOUND;
			if (reinterpret_cast<intptr_t>(loc_menu->user_data()) == loc_dev_index ||
				loc_dev_found == -1) { // exact match, or index was never saved
				loc_dev_found = PA_EXACT_DEV_FOUND;
			}
		}
	}

	if (loc_idx >= 0) {
		loc_choice->value(loc_idx);
		loc_choice->set_changed();
	}

	return loc_dev_found;
}

#else
static void init_portaudio(void) { }
#endif // USE_PORTAUDIO

static void build_srate_listbox(Fl_ListBox* lbox, const double* rates, size_t length, double defrate = -1.0)
{
	lbox->clear();
	lbox->add("Auto");
	lbox->add("Native");

	char s[16];
	for (size_t i = 0; i < length; i++) {
		if (defrate != rates[i])
			snprintf(s, sizeof(s), "%.0f", rates[i]);
		else
			snprintf(s, sizeof(s), "%.0f (native)", rates[i]);
		lbox->add(s);
	}
}

int sample_rate_converters[FLDIGI_NUM_SRC] = {
	SRC_SINC_BEST_QUALITY,
	SRC_SINC_MEDIUM_QUALITY,
	SRC_SINC_FASTEST
#if !(defined(__ppc__) || defined(__powerpc__) || defined(__PPC__))
	, SRC_LINEAR
#endif
};

static void sound_init_options(void)
{
	build_srate_listbox(menuInSampleRate, std_sample_rates,
						sizeof(std_sample_rates)/sizeof(*std_sample_rates) - 1);
	build_srate_listbox(menuOutSampleRate, std_sample_rates,
						sizeof(std_sample_rates)/sizeof(*std_sample_rates) - 1);

	for (int i = 0; i < FLDIGI_NUM_SRC; i++)
		menuSampleConverter->add(src_get_name(sample_rate_converters[i]));
	// Warn if we are using ZOH
	if (progdefaults.sample_converter == SRC_ZERO_ORDER_HOLD) {
		progdefaults.sample_converter = SRC_LINEAR;
		LOG_WARN("The Zero Order Hold sample rate converter should not be used! "
				 "Your setting has been changed to Linear.");
	}
#if defined(__ppc__) || defined(__powerpc__) || defined(__PPC__)
	// SRC_LINEAR may crash with 11025Hz modems. Change to SINC_FASTEST.
	if (progdefaults.sample_converter == SRC_LINEAR) {
		progdefaults.sample_converter = SRC_SINC_FASTEST;
		LOG_WARN("Linear sample rate converter may not work on this architecture. "
				 "Your setting has been changed to Fastest Sinc");
	}
#endif
	for (int i = 0; i < FLDIGI_NUM_SRC; i++) {
		if (sample_rate_converters[i] == progdefaults.sample_converter) {
			menuSampleConverter->index(i);
			menuSampleConverter->tooltip(src_get_description(progdefaults.sample_converter));
			break;
		}
	}

	menuOSSDev->value(progdefaults.OSSdevice.c_str());
	inpPulseServer->value(progdefaults.PulseServer.c_str());

	char sr[6+1];
	if (progdefaults.in_sample_rate == SAMPLE_RATE_UNSET &&
		(progdefaults.in_sample_rate = progdefaults.sample_rate) == SAMPLE_RATE_UNSET)
		progdefaults.in_sample_rate = SAMPLE_RATE_NATIVE;
	else if (progdefaults.in_sample_rate < SAMPLE_RATE_OTHER)
		menuInSampleRate->index(progdefaults.in_sample_rate);
	else {
		snprintf(sr, sizeof(sr), "%d", progdefaults.in_sample_rate);
		for (int i = SAMPLE_RATE_NATIVE + 1; i < menuInSampleRate->lsize(); i++) {
			menuInSampleRate->index(i);
			if (strstr(menuInSampleRate->value(), sr))
				break;
		}
	}

	if (progdefaults.out_sample_rate == SAMPLE_RATE_UNSET &&
		(progdefaults.out_sample_rate = progdefaults.sample_rate) == SAMPLE_RATE_UNSET)
		progdefaults.out_sample_rate = SAMPLE_RATE_NATIVE;
	else if (progdefaults.out_sample_rate < SAMPLE_RATE_OTHER)
		menuOutSampleRate->index(progdefaults.out_sample_rate);
	else {
		snprintf(sr, sizeof(sr), "%d", progdefaults.out_sample_rate);
		for (int i = SAMPLE_RATE_NATIVE + 1; i < menuOutSampleRate->lsize(); i++) {
			menuOutSampleRate->index(i);
			if (strstr(menuOutSampleRate->value(), sr))
				break;
		}
	}

	cntRxRateCorr->value(progdefaults.RX_corr);
	cntTxRateCorr->value(progdefaults.TX_corr);
	cntTxOffset->value(progdefaults.TxOffset);
}

#if USE_PULSEAUDIO
#  include <pulse/context.h>
#  include <pulse/mainloop.h>
#  include <pulse/version.h>

#  if PA_API_VERSION < 12
static inline int PA_CONTEXT_IS_GOOD(pa_context_state_t x) {
	return  x == PA_CONTEXT_CONNECTING || x == PA_CONTEXT_AUTHORIZING ||
	x == PA_CONTEXT_SETTING_NAME || x == PA_CONTEXT_READY;
}
#  endif

static bool probe_pulseaudio(void)
{
	pa_mainloop* loop = pa_mainloop_new();
	if (!loop)
		return false;

	bool ok = false;
	pa_context* context = pa_context_new(pa_mainloop_get_api(loop), PACKAGE_TARNAME);
	if (context && pa_context_connect(context, NULL, PA_CONTEXT_NOAUTOSPAWN, NULL) >= 0) {
		pa_context_state_t state;
		do { // iterate main loop until the context connection fails or becomes ready
			if (!(ok = (pa_mainloop_iterate(loop, 1, NULL) >= 0 &&
						PA_CONTEXT_IS_GOOD(state = pa_context_get_state(context)))))
				break;
		} while (state != PA_CONTEXT_READY);
	}

	if (context) {
		pa_context_disconnect(context);
		pa_context_unref(context);
	}
	pa_mainloop_free(loop);

	return ok;
}
#else
static bool probe_pulseaudio(void) { return false; }
#endif // USE_PULSEAUDIO

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
		int io[4] = { SND_IDX_PORT, SND_IDX_PULSE, SND_IDX_OSS, SND_IDX_NULL };
		if (probe_pulseaudio()) { // prefer pulseaudio
			io[0] = SND_IDX_PULSE;
			io[1] = SND_IDX_PORT;
		}
		for (size_t i = 0; i < sizeof(io)/sizeof(*io); i++) {
			if (btnAudioIO[io[i]]->active()) {
				progdefaults.btnAudioIOis = io[i];
				break;
			}
		}
	}

	sound_init_options();

	sound_update(progdefaults.btnAudioIOis);

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
			Fl_ListBox* listbox[2] = { menuInSampleRate, menuOutSampleRate };
			for (size_t i = 0; i < 2; i++) {
				char* label = strdup(listbox[i]->value());
				const vector<double>& srates = SoundPort::get_supported_rates(scDevice[i], i);

				switch (srates.size()) {
					case 0: // startup; no devices initialised yet
						build_srate_listbox(listbox[i], std_sample_rates,
											sizeof(std_sample_rates)/sizeof(*std_sample_rates) - 1);
						break;
					case 1: // default sample rate only, build menu with all std rates
						build_srate_listbox(listbox[i], std_sample_rates,
											sizeof(std_sample_rates)/sizeof(*std_sample_rates) - 1, srates[0]);

						break;
					default: // first element is default sample rate, build menu with rest
						build_srate_listbox(listbox[i], &srates[0] + 1, srates.size() - 1, srates[0]);
						break;
				}

				for (int j = 0; j < listbox[i]->lsize(); j++) {
					listbox[i]->index(j);
					if (strstr(listbox[i]->value(), label))
						break;
				}
				free(label);

				listbox[i]->activate();
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

