// Class Caudio_alert
//
// play various canned play_sounds or wav file using port audio interface

#include "audio_alert.h"

void Caudio_alert::bark()
{
	try {
		sc_audio->play_sound(int_audio_bark, BARK_SIZE, SCRATE);
		sc_audio->silence(0.5, SCRATE);
		sc_audio->play_sound(int_audio_bark, BARK_SIZE, SCRATE);
	} catch (...) {
		throw;
	} 
}

void Caudio_alert::checkout()
{
	try {
		sc_audio->play_sound(int_audio_checkout, CHECKOUT_SIZE, SCRATE);
	} catch (...) {
		throw;
	}
}

void Caudio_alert::doesnot()
{
	try {
		sc_audio->play_sound(int_audio_doesnot, DOESNOT_SIZE, SCRATE);
	} catch (...) {
		throw;
	}
}

void Caudio_alert::diesel()
{
	try {
		sc_audio->play_sound(int_audio_diesel, DIESEL_SIZE, SCRATE);
		sc_audio->silence(0.5, SCRATE);
		sc_audio->play_sound(int_audio_diesel, DIESEL_SIZE, SCRATE);
	} catch (...) {
		throw;
	}
}

void Caudio_alert::steam_train()
{
	try {
		sc_audio->play_sound(int_steam_train, STEAM_TRAIN_SIZE, SCRATE);
	} catch (...) {
		throw;
	}
}

void Caudio_alert::beeboo()
{
	try {
		sc_audio->play_sound(int_audio_beeboo, BEEBOO, SCRATE);
	} catch (...) {
		throw;
	}
}

void Caudio_alert::phone()
{
	try {
		sc_audio->play_sound(phonering, PHONERING, SCRATE);
		sc_audio->silence(1.0, SCRATE);
		sc_audio->play_sound(phonering, PHONERING, SCRATE);
	} catch (...) {
		throw;
	}
}

void Caudio_alert::dinner_bell()
{
	try {
		sc_audio->play_sound(int_dinner_bell, DINNER_BELL, SCRATE);
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
	int attack = 40;
	int ntones = 60;
	float freq = 480;
	float sr = 8000;
	int duration = PHONERING/ntones;
	float val;
	float modulation[duration];
	for (int i = 0; i < duration; i++) {
		val = 1.0;
		if (i < attack) val *= (1.0 * i / attack);
		if (i > duration - attack) val *= (1.0 * (duration - i) / attack);
		modulation[i] = val;
	}
	for (int i = 0; i < ntones; i++) {
		for (int j = 0; j < duration; j++) {
			val = modulation[j] * sin(2.0 * M_PI * freq * (duration * i + j) / sr);
			phonering[duration * i + j] = 32500 * val;
		}
	}
}

Caudio_alert *audio_alert = 0;
