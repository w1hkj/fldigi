#ifndef PSK_BROWSER_H
#define PSK_BROWSER_H

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Hold_Browser.H>

#include <string>

#include <config.h>
#include "viewpsk.h"
#include "globals.h"
#include "charsetdistiller.h"

#include "re.h"

#define BWSR_BORDER 2

class pskBrowser : public Fl_Hold_Browser{
private:
	static std::string hilite_color_1;
	static std::string hilite_color_3;
	static std::string hilite_color_2;
	static std::string bkselect;
	static std::string white;
	static std::string bkgnd[];

	std::string fline;
	std::string nuline;

	std::string bwsrline[MAXCHANNELS];
	int bwsrfreq[MAXCHANNELS];
	CharsetDistiller *cdistiller;
	

	int labelwidth[VIEWER_LABEL_NTYPES];

	Fl_Font fnt;
	Fl_Color HiLite_1;
	Fl_Color HiLite_2;
	Fl_Color HiLite_3;

	int siz;
	int cols[2];
	char szLine[32];
	size_t nchars;
	size_t linechars[32];

public:
	static int cwidth;
	static int cheight;
	static int sbarwidth;

	long long rfc;
	bool usb;
	fre_t *seek_re;

public:
	pskBrowser(int x, int y, int w, int h, const char *l = "");
	~pskBrowser();
	void makecolors();
	void evalcwidth();
	void setfont(Fl_Font font, int sz) { fnt = font; siz = sz; evalcwidth();}
	void HighLight_1(Fl_Color clr) { HiLite_1 = clr; }
	Fl_Color HighLight_1() { return HiLite_1; }
	void HighLight_2(Fl_Color clr) { HiLite_3 = clr; }
	Fl_Color HighLight_2() { return HiLite_2; }
	void HighLight_3(Fl_Color clr) { HiLite_3 = clr; }
	Fl_Color HighLight_3() { return HiLite_3; }
	void columns(int a) { cols[0] = a; cols[1] = 0; column_widths(cols); }
	void resize(int x, int y, int w, int h);
	void addchr(int ch, int freq, unsigned char c, int md);
	std::string freqformat (int i);
	void set_freq(int i, int freq);
	void clearline(int i) { bwsrline[i] = ""; }
	std::string line(int i) { return (i < 1 ? "" : i > MAXCHANNELS ? "" : bwsrline[i - 1]); }
	int  freq(int i);
	void clear();
	void clearch(int n, int freq);
	void swap(int, int);
	int  numchars() { return nchars; }
	void set_input_encoding(int encoding_id);
};

#endif
