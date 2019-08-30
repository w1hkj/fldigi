// Class Caudio_alert
//
// play various canned sounds or wav file using port audio interface

#ifndef AUDIO_ALERT_H
#define AUDIO_ALERT_H

#include "play.pa.h"

#define STEAM_TRAIN_SIZE 50225
extern int int_steam_train[];

#define BARK_SIZE 1501
extern int int_audio_bark[];

#define CHECKOUT_SIZE 11424
extern int int_audio_checkout[];

#define DOESNOT_SIZE 9927
extern int int_audio_doesnot[];

#define DIESEL_SIZE 7621
extern int int_audio_diesel[];

#define DINNER_BELL 15287
extern int int_dinner_bell[];

class Caudio_alert {
private:
#define BEEBOO 48000
#define PHONERING 16000

	int phonering[PHONERING];
	int int_audio_beeboo[BEEBOO];

	c_portaudio *sc_audio;

	void create_beeboo();
	void create_phonering();

public:
	void bark();
	void checkout();
	void doesnot();
	void diesel();
	void steam_train();
	void beeboo();
	void phone();
	void dinner_bell();
	void standard_tone();
	void file(std::string sndfile);

	void alert(std::string s) {
		if (s.empty()) return;
		if (s == "bark") bark();
		else if (s == "checkout") checkout();
		else if (s == "doesnot" ) doesnot();
		else if (s == "diesel" ) diesel();
		else if (s == "steam_train") steam_train();
		else if (s == "beeboo") beeboo();
		else if (s == "phone") phone();
		else if (s == "dinner_bell") dinner_bell();
		else if (s == "standard_tone") standard_tone();
		else file(s);
	}

	Caudio_alert()
	{
		try {
			sc_audio = new c_portaudio;
			create_phonering();
			create_beeboo();
		} catch (...) {
			throw;
		}
	}

	~Caudio_alert() {
		delete sc_audio;
	}

};

extern Caudio_alert *audio_alert;

#endif
