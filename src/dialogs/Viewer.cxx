#include "Viewer.h"
#include "main.h"
#include "viewpsk.h"
#include "configuration.h"
#include "waterfall.h"
#include <FL/Enumerations.H>

#include <string>

extern viewpsk *pskviewer;

Fl_Double_Window *dlgViewer = (Fl_Double_Window *)0;

Fl_Button *btnCloseViewer=(Fl_Button *)0;
Fl_Button *btnClearViewer=(Fl_Button *)0;
Fl_Hold_Browser *brwsViewer=(Fl_Hold_Browser *)0;
Fl_Input  *inpSeek = (Fl_Input *)0;

string bwsrfreq; //[25];
string bwsrline[25];
string ucaseline[25];
string tofind;
static char empty[] = "@f                                        ";
//                    "@f1234567890123456789012345678901234567890";
static char szFreq[40];
static int  brwsFreq[25];
static int  freq;
static double dfreq;

static long long rfc;
static bool usb;

string dkred;
string dkblue;
string dkgreen;
string bkblue;
string white;

static void make_colors()
{
	char tempstr[20];
	sprintf(tempstr,"@C%d", 
			fl_color_cube(128 * (FL_NUM_RED - 1) / 255,
            0 * (FL_NUM_GREEN - 1) / 255,
            0 * (FL_NUM_BLUE - 1) / 255)); // dark red
    dkred = tempstr;
	sprintf(tempstr,"@C%d", 
			fl_color_cube(0 * (FL_NUM_RED - 1) / 255,
            128 * (FL_NUM_GREEN - 1) / 255,
            0 * (FL_NUM_BLUE - 1) / 255)); // dark green
    dkgreen = tempstr;
	sprintf(tempstr,"@C%d", 
			fl_color_cube(0 * (FL_NUM_RED - 1) / 255,
            0 * (FL_NUM_GREEN - 1) / 255,
            128 * (FL_NUM_BLUE - 1) / 255)); // dark blue
    dkblue = tempstr;
	sprintf(tempstr,"@B%d", 
			fl_color_cube(0 * (FL_NUM_RED - 1) / 255,
            0 * (FL_NUM_GREEN - 1) / 255,
            160 * (FL_NUM_BLUE - 1) / 255)); // dard blue background
    bkblue = tempstr;
	sprintf(tempstr,"@C%d", 
			fl_color_cube(248 * (FL_NUM_RED - 1) / 255,
            248 * (FL_NUM_GREEN - 1) / 255,
            248 * (FL_NUM_BLUE - 1) / 255)); // white foreground
    white = tempstr;
}

static void cb_btnCloseViewer(Fl_Button*, void*) {
  dlgViewer->hide();
}

static void ClearViewer() {
  	brwsViewer->clear();
  	usb = wf->USB();
  	rfc = wf->rfcarrier();
  	
  	for (int i = 0; i < 25; i++) {
  		if (pskviewer) freq = pskviewer->get_freq(24 - i);
  		else 		   freq = 500 + 100 * (24 - i);
  		if (rfc != 0) {
  			if (usb) dfreq = rfc + freq;
  			else     dfreq = rfc - freq;
  		} else dfreq = freq;
  			
  		brwsFreq[i] = freq;
//  		sprintf(szFreq,"@f@b%11.3f", dfreq / 1.0e3);
		sprintf(szFreq,"@f%s%s%11.3f", bkblue.c_str(), white.c_str(), dfreq / 1.0e3);
		
  		bwsrfreq = szFreq;
  		bwsrfreq += '\t';
  		bwsrline[i] = empty;
  		ucaseline[i] = empty;
  		brwsViewer->add(bwsrfreq.c_str());
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

static void cb_Seek(Fl_Input *, void *)
{
	tofind = inpSeek->value();
	for (size_t i = 0; i < tofind.length(); i++)
		tofind[i] = toupper(tofind[i]);
}

int cols[] = {100, 0};

Fl_Double_Window* createViewer() {
	Fl_Double_Window* w;
	Fl_Pack *p;
	make_colors();
	w = new Fl_Double_Window(450, 480, "Psk Viewer");
	p = new Fl_Pack(0,0,450,490);
		Fl_Pack *p1 = new Fl_Pack(0, 0, 450, 25);
			p1->type(1);
			Fl_Box *bx = new Fl_Box(0,0,50, 25);
	    	inpSeek = new Fl_Input(50, 5, 200, 25, "Find: "); 
    		inpSeek->callback((Fl_Callback*)cb_Seek);
    		inpSeek->when(FL_WHEN_CHANGED);
    		inpSeek->value("CQ");
    		bx = new Fl_Box(250, 5, 200, 25);
    		p1->resizable(bx);
    	p1->end();
    	tofind = "CQ";

    	brwsViewer = new Fl_Hold_Browser(2, 35, 446, 430);
    	brwsViewer->callback((Fl_Callback*)cb_brwsViewer);
    	brwsViewer->column_widths(cols);
		
		Fl_Pack *p2 = new Fl_Pack(0, 470, 450, 25);
			p2->type(1);
			bx = new Fl_Box(0,470, 10, 25);
    		btnClearViewer = new Fl_Button(10, 470, 65, 25, "Clear");
    		btnClearViewer->callback((Fl_Callback*)cb_btnClearViewer);
    		bx = new Fl_Box(75, 470, 10, 25);
	    	btnCloseViewer = new Fl_Button(85, 470, 65, 25, "Close");
    		btnCloseViewer->callback((Fl_Callback*)cb_btnCloseViewer);
    		bx = new Fl_Box(150, 470, 300, 25);
    		p2->resizable(bx);
		p2->end();
		p->resizable(brwsViewer);
	p->end();
	w->resizable(p);
    w->end();
	return w;
}

void openViewer() {
	if (!dlgViewer) {
		dlgViewer = createViewer();
		ClearViewer();
	}
	dlgViewer->show();
	dlgViewer->redraw();
}

void viewer_redraw()
{
	if (!dlgViewer) return;
  	usb = wf->USB();
  	rfc = wf->rfcarrier();
  	
  	for (int i = 0; i < 25; i++) {
  		if (pskviewer) freq = pskviewer->get_freq(24 - i);
  		else 		   freq = 500 + 100 * (24 - i);
  		if (rfc) {
  			if (usb) dfreq = rfc + freq;
  			else     dfreq = rfc - freq;
  		} else dfreq = freq;
  			
  		brwsFreq[i] = freq;
		sprintf(szFreq,"@f%s%s%11.3f", bkblue.c_str(), white.c_str(), dfreq / 1.0e3);
  		bwsrfreq = szFreq;
  		bwsrfreq += '\t';
  		brwsViewer->text(i + 1, bwsrfreq.c_str());
  	}
}

void viewaddchr(int ch, int freq, char c) {
	if (!dlgViewer) return;

	if (rfc != wf->rfcarrier() || usb != wf->USB()) viewer_redraw();
		
	static string nuline;
	bwsrline[24 - ch].erase(2,1);
	ucaseline[24 - ch].erase(2,1);
	if (c >= ' ' && c <= 'z') {
		bwsrline[24 - ch] += c;
		ucaseline[24 - ch] += toupper(c);
	} else {
		bwsrline[24 - ch] += ' ';
		ucaseline[24 - ch] += ' ';
	}
	brwsFreq[24 - ch] = freq;
	if (rfc) {
  		if (usb) dfreq = rfc + freq;
  		else     dfreq = rfc - freq;
  	} else dfreq = freq;
	
	sprintf(szFreq,"@f%s%s%11.3f", bkblue.c_str(), white.c_str(), dfreq / 1.0e3);
	
  	bwsrfreq = szFreq;
  	bwsrfreq += '\t';
	nuline = bwsrfreq;

	if (!tofind.empty())
		if (ucaseline[24 - ch].find(tofind) != string::npos)
			nuline.append(dkred);
	else if (!progdefaults.myCall.empty())
		if (ucaseline[24 - ch].find(progdefaults.myCall) != string::npos)
			nuline.append(dkgreen);
			
	nuline.append(bwsrline[24 - ch]);
	brwsViewer->text(25 - ch, nuline.c_str());
	brwsViewer->redraw();
}
