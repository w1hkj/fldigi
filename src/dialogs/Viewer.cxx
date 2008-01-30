#include "Viewer.h"
#include "main.h"
#include "viewpsk.h"

#include <string>

extern viewpsk *pskviewer;

Fl_Double_Window *dlgViewer = (Fl_Double_Window *)0;

Fl_Button *btnCloseViewer=(Fl_Button *)0;
Fl_Button *btnClearViewer=(Fl_Button *)0;
Fl_Hold_Browser *brwsViewer=(Fl_Hold_Browser *)0;

string bwsrfreq[25];
string bwsrline[25];
static char empty[] = "@f                                        ";
//                    "@f1234567890123456789012345678901234567890";
static char szFreq[20];
static int  brwsFreq[25];
static int  freq;

static void cb_btnCloseViewer(Fl_Button*, void*) {
  dlgViewer->hide();
}

static void ClearViewer() {
  	brwsViewer->clear();
  	for (int i = 0; i < 25; i++) {
  		if (pskviewer) freq = pskviewer->get_freq(i);
  		else 		   freq = 500 + 100 * i;
  		brwsFreq[i] = freq;
  		sprintf(szFreq,"@f@b%4d", freq);
  		bwsrfreq[i] = szFreq;
  		bwsrfreq[i] += '\t';
  		bwsrline[i] = empty;
  		brwsViewer->add(bwsrfreq[i].c_str());
  	}
}

static void cb_btnClearViewer(Fl_Button*, void*) {
	ClearViewer();
}

static void cb_brwsViewer(Fl_Hold_Browser*, void*) {
	int sel = brwsViewer->value();
	if (sel == 0) return;
	if (sel > 25) return;
	active_modem->set_freq(brwsFreq[sel - 1]);
}

int cols[] = {60, 0};

Fl_Double_Window* createViewer() {
	Fl_Double_Window* w;
	w = new Fl_Double_Window(400, 470, "Psk Viewer");
    	w->color(FL_DARK2);
    	w->selection_color((Fl_Color)51);
    	w->align(FL_ALIGN_CLIP|FL_ALIGN_INSIDE);
    	btnCloseViewer = new Fl_Button(295, 440, 100, 25, "Close");
    	btnCloseViewer->callback((Fl_Callback*)cb_btnCloseViewer);
    	btnClearViewer = new Fl_Button(170, 440, 100, 25, "Clear");
    	btnClearViewer->callback((Fl_Callback*)cb_btnClearViewer);
    	brwsViewer = new Fl_Hold_Browser(0, 5, 390, 430);
    	brwsViewer->callback((Fl_Callback*)cb_brwsViewer);
    	brwsViewer->column_widths(cols);
    w->end();
	return w;
}

void openViewer() {
	if (!dlgViewer) {
		dlgViewer = createViewer();
		ClearViewer();
	}
	dlgViewer->show();
}

void viewaddchr(int ch, int freq, char c) {
	if (!dlgViewer) return;
	
	static string nuline;
	bwsrline[ch].erase(2,1);
	if (c >= ' ' && c <= 'z')
		bwsrline[ch] += c;
	else
		bwsrline[ch] += ' ';
	brwsFreq[ch] = freq;
  	sprintf(szFreq,"@f@b%4d", freq);
  	bwsrfreq[ch] = szFreq;
  	bwsrfreq[ch] += '\t';
	nuline = bwsrfreq[ch];
	if (bwsrline[ch].find("CQ ") != string::npos ||
	    bwsrline[ch].find("cq ") != string::npos ||
	    bwsrline[ch].find("Cq ") != string::npos ||
	    bwsrline[ch].find("cQ ") != string::npos)
		nuline.append("@C128");
	nuline.append(bwsrline[ch]);
	brwsViewer->text(ch + 1, nuline.c_str());
	brwsViewer->redraw();
}
