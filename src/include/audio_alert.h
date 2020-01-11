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

#define TTY_BELL 4763
extern int int_tty_bell[];

class Caudio_alert {

private:
	static int int_phone_ring[];
	static int int_audio_beeboo[];

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
	void tty_bell();
	void standard_tone();
	void file(std::string sndfile);

	void alert(std::string s);

	void monitor(double *buffer, int len, int _sr);
	void monitor(cmplx *z, int len, double wf, int _sr);

	Caudio_alert();
	~Caudio_alert();

	int  open() {
		if (sc_audio) return sc_audio->open();
		return 0;
	}
	void close() {
		if (sc_audio) sc_audio->close();
	}

	c_portaudio *pa() { return sc_audio; }
	void init_filter() { sc_audio->init_filter(); }

};

extern Caudio_alert *audio_alert;

extern void reset_audio_alerts();

extern void center_rxfilt_at_track();

#endif
