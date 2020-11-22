// Class Caudio_alert
//
// play various canned play_sounds or wav file using port audio interface

#include "audio_alert.h"
#include "configuration.h"
#include "confdialog.h"
#include "rxmon.h"

#define SC_RATE 8000
#define PHONERING 15000

int Caudio_alert::int_phone_ring[PHONERING];

#define BEEBOO 48000
int Caudio_alert::int_audio_beeboo[BEEBOO];

void Caudio_alert::bark()
{
	try {
		sc_audio->play_sound(int_audio_bark, BARK_SIZE, SC_RATE);
		sc_audio->silence(0.5, SC_RATE);
		sc_audio->play_sound(int_audio_bark, BARK_SIZE, SC_RATE);
	} catch (...) {
		throw;
	} 
}

void Caudio_alert::checkout()
{
	try {
		sc_audio->play_sound(int_audio_checkout, CHECKOUT_SIZE, SC_RATE);
	} catch (...) {
		throw;
	}
}

void Caudio_alert::doesnot()
{
	try {
		sc_audio->play_sound(int_audio_doesnot, DOESNOT_SIZE, SC_RATE);
	} catch (...) {
		throw;
	}
}

void Caudio_alert::diesel()
{
	try {
		sc_audio->play_sound(int_audio_diesel, DIESEL_SIZE, SC_RATE);
		sc_audio->silence(0.5, SC_RATE);
		sc_audio->play_sound(int_audio_diesel, DIESEL_SIZE, SC_RATE);
	} catch (...) {
		throw;
	}
}

void Caudio_alert::steam_train()
{
	try {
		sc_audio->play_sound(int_steam_train, STEAM_TRAIN_SIZE, SC_RATE);
	} catch (...) {
		throw;
	}
}

void Caudio_alert::beeboo()
{
	try {
		sc_audio->play_sound(int_audio_beeboo, BEEBOO, SC_RATE);
	} catch (...) {
		throw;
	}
}

void Caudio_alert::phone()
{
	try {
		sc_audio->play_sound(int_phone_ring, PHONERING, SC_RATE);
		sc_audio->silence(1.0, SC_RATE);
		sc_audio->play_sound(int_phone_ring, PHONERING, SC_RATE);
	} catch (...) {
		throw;
	}
}

void Caudio_alert::dinner_bell()
{
	try {
		sc_audio->play_sound(int_dinner_bell, DINNER_BELL, SC_RATE);
	} catch (...) {
		throw;
	}
}

void Caudio_alert::tty_bell()
{
	try {
		sc_audio->play_sound(int_tty_bell, TTY_BELL, SC_RATE);
	} catch (...) {
		throw;
	}
}

void Caudio_alert::standard_tone()
{
	try{
		float st[16000];
		float mod;
		for (int i = 0; i < 16000; i++) {
			mod = i < 800 ? sin(M_PI*i/1600.0) :
				  i > 15200 ? (cos(M_PI*(i - 15200)/1600.0)) : 1.0;
			st[i] = 0.9 * mod * sin(2.0*M_PI*i*440.0/8000.0);
		}
		sc_audio->play_sound(st, 16000, 8000.0);
	} catch (...) {
		throw;
	}
}

void Caudio_alert::file(std::string sndfile)
{
	try {
		sc_audio->play_file(sndfile);
	} catch (...) {
		throw;
	}
}

void Caudio_alert::create_beeboo()
{
	float bee = 800;
	float boo = 500;
	float val;
	float sr = 8000;
	for (int i = 0; i < BEEBOO; i++) {
		val = sin( (2.0 * M_PI * i / sr) *
				(i / 2000 % 2 == 0 ? bee : boo));
		if (i < 500) val *= 1.0 * i / 500;
		if (i > (BEEBOO-500)) val *= 1.0 * (BEEBOO - i) / 500;
		int_audio_beeboo[i] = 32500 * val;
	}
}

void Caudio_alert::create_phonering()
{
	int ntones = 60;
	float freq = 500;
	float sr = 8000;
	int mod = PHONERING/ntones;
	memset(int_phone_ring, 0, sizeof(int_phone_ring));
	for (int i = 0; i <= (PHONERING - mod); i++)
		int_phone_ring[i] = 32000 * fabs(sin(M_PI * i / mod)) * sin(2.0 * M_PI * freq * i / sr);;
}

void Caudio_alert::alert(std::string s)
{
	if (s.empty()) return;
	if (s == "bark") bark();
	else if (s == "checkout") checkout();
	else if (s == "doesnot" ) doesnot();
	else if (s == "diesel" ) diesel();
	else if (s == "steam_train") steam_train();
	else if (s == "beeboo") beeboo();
	else if (s == "phone") phone();
	else if (s == "dinner_bell") dinner_bell();
	else if (s == "rtty_bell") tty_bell();
	else if (s == "standard_tone") standard_tone();
	else file(s);
}

void Caudio_alert::monitor(double *buffer, int len, int _sr)
{
	if (progdefaults.mon_xcvr_audio)
		sc_audio->mon_write(buffer, len, _sr);
}

Caudio_alert::Caudio_alert()
{
	try {
		sc_audio = new c_portaudio;
		create_phonering();
		create_beeboo();
	} catch (...) {
		throw;
	}
}

Caudio_alert::~Caudio_alert()
{
	delete sc_audio;
}

Caudio_alert *audio_alert = 0;

void center_rxfilt_at_track()
{
	progdefaults.RxFilt_mid = active_modem->get_freq();

	int bw2 = progdefaults.RxFilt_bw / 2;
	progdefaults.RxFilt_low = progdefaults.RxFilt_mid - bw2;
	if (progdefaults.RxFilt_low < 100) progdefaults.RxFilt_low = 100;

	progdefaults.RxFilt_high = progdefaults.RxFilt_mid + bw2;
	if (progdefaults.RxFilt_high > 4000) progdefaults.RxFilt_high = 4000;

	sldrRxFilt_mid->value(progdefaults.RxFilt_mid);
	sldrRxFilt_mid->redraw();

	sldrRxFilt_low->value(progdefaults.RxFilt_low);
	sldrRxFilt_low->redraw();

	sldrRxFilt_high->value(progdefaults.RxFilt_high);
	sldrRxFilt_high->redraw();

	progdefaults.changed = true;

	if (audio_alert)
		audio_alert->init_filter();
}

void reset_audio_alerts()
{
	if (!audio_alert) return;

	if (progdefaults.enable_audio_alerts && audio_alert) {
		if (audio_alert->open())
			LOG_INFO("Opened audio alert stream on %s", progdefaults.AlertDevice.c_str());
		else {
			progdefaults.enable_audio_alerts = 0;
			btn_enable_audio_alerts->value(0);
			LOG_ERROR("Open audio device %s FAILED", progdefaults.AlertDevice.c_str());
		}
	} else {
		audio_alert->close();
		LOG_INFO("Closed audio alert device %s", progdefaults.AlertDevice.c_str());
	}
}
