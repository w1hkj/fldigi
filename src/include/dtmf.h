#include <string>

#include "filters.h"

class cDTMF {
private:
//#define N        240 // 30 msec interval at 8000 sps

#define RANGE  0.5         /* any thing higher than RANGE*peak is "on" */
#define THRESH 1000        /* 6 dB s/n for detection */
#define FLUSH_TIME 10      /* 10 frames ~ 330 millisecond */

#define NUMTONES 8

private:
	double power[NUMTONES];
	double thresh;
	double maxpower;
	double minpower;
	double data[350];

	goertzel *filt[NUMTONES];

	int framesize;

	static double coef[];
	static int k[];
	static const char *dtran[];

	static int row[];
	static int col[];
	static const char rc[];

	double outbuf[16384];
	double shape[128];
	int RT;
	int duration;
	int silence_time;
	int last;
	std::string dtmfchars;

public:
	cDTMF() {
		for (int i = 0; i < 4; i++) filt[i] = new goertzel(240, row[i], 8000);
		for (int i = 0; i < 4; i++) filt[i+4] = new goertzel(240, col[i], 8000);
		for (int i = 0; i < 240; i++) data[i] = 0;
		dtmfchars.clear();
		framesize = 240; // 8000 sr default
		silence_time = 0;
		last = ' ';
	}
	~cDTMF() {};
// receive
	void calc_power();
	int decode();
	void receive(const float* buf, size_t len);
// transmit
	void makeshape();
	void silence(int);
	void two_tones(int);
	void send();
};
