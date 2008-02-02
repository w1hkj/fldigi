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

string bwsrfreq;
string bwsrline[MAXCHANNELS];
string ucaseline[MAXCHANNELS];
string tofind;

static char szFreq[40];
static int  brwsFreq[MAXCHANNELS];
static int  freq;
static double dfreq;
int wline;

static long long rfc;
static bool usb;

string dkred;
string dkblue;
string dkgreen;
string bkselect;
string white;
string snow;
string slategray1;
string bkgnd[2];

int cols[] = {100, 0};
int smcols[] = {30, 0};

static void make_colors()
{
	char tempstr[20];
	sprintf(tempstr,"@C%d", 
			fl_color_cube(128 * (FL_NUM_RED - 1) / 255,
            0 * (FL_NUM_GREEN - 1) / 255,
            0 * (FL_NUM_BLUE - 1) / 255)); // dark red
    dkred = "@b";
    dkred += tempstr;
	sprintf(tempstr,"@C%d", 
			fl_color_cube(0 * (FL_NUM_RED - 1) / 255,
            128 * (FL_NUM_GREEN - 1) / 255,
            0 * (FL_NUM_BLUE - 1) / 255)); // dark green
    dkgreen = "@b";
    dkgreen += tempstr;
	sprintf(tempstr,"@C%d", 
			fl_color_cube(0 * (FL_NUM_RED - 1) / 255,
            0 * (FL_NUM_GREEN - 1) / 255,
            128 * (FL_NUM_BLUE - 1) / 255)); // dark blue
    dkblue = "@b";
    dkblue += tempstr;
	sprintf(tempstr,"@C%d", 
			fl_color_cube(248 * (FL_NUM_RED - 1) / 255,
            248 * (FL_NUM_GREEN - 1) / 255,
            248 * (FL_NUM_BLUE - 1) / 255)); // white foreground
    white = tempstr;
    sprintf(tempstr, "@B%d", FL_SELECTION_COLOR);
    bkselect = tempstr;                      // default selection color bkgnd
	sprintf(tempstr,"@B%d", 
			fl_color_cube(255 * (FL_NUM_RED - 1) / 255,
            255 * (FL_NUM_GREEN - 1) / 255,
            255 * (FL_NUM_BLUE - 1) / 255)); // snow background
    snow = tempstr;
    bkgnd[0] = snow;
	sprintf(tempstr,"@B%d", 
			fl_color_cube(198 * (FL_NUM_RED - 1) / 255,
            226 * (FL_NUM_GREEN - 1) / 255,
            255 * (FL_NUM_BLUE - 1) / 255)); // slategray1 background
    slategray1 = tempstr;
    bkgnd[1] = slategray1;
}

static void cb_btnCloseViewer(Fl_Button*, void*) {
  dlgViewer->hide();
}

void ClearViewer() {
  	brwsViewer->clear();
  	usb = wf->USB();
  	rfc = wf->rfcarrier();
  	
  	for (int i = 0; i < progdefaults.VIEWERchannels; i++) {
  		if (pskviewer) freq = pskviewer->get_freq(progdefaults.VIEWERchannels - 1 - i);
  		else 		   freq = progdefaults.VIEWERstart + 100 * (progdefaults.VIEWERchannels - 1 - i);
  		if (rfc != 0) {
  			if (usb) dfreq = rfc + freq;
  			else     dfreq = rfc - freq;
  		} else dfreq = freq;
  			
  		brwsFreq[i] = freq;
  		if (progdefaults.VIEWERshowfreq)
			sprintf(szFreq,"@f%s%s%11.3f", bkselect.c_str(), white.c_str(), dfreq / 1.0e3);
		else
			sprintf(szFreq,"@f%s%s%3d", bkselect.c_str(), white.c_str(), progdefaults.VIEWERchannels - i);	
  		bwsrfreq = szFreq;
  		bwsrfreq += '\t';
  		bwsrfreq += bkgnd[i % 2];
  		bwsrline[i] = "";
  		ucaseline[i] = "";
  		brwsViewer->add(bwsrfreq.c_str());
  	}
	if (progdefaults.VIEWERshowfreq) {
   		brwsViewer->column_widths(cols);
	} else {
		brwsViewer->column_widths(smcols);	
	}
}

void initViewer()
{
	if (pskviewer) pskviewer->init();
	ClearViewer();
}

static void cb_btnClearViewer(Fl_Button*, void*) {
	ClearViewer();
}

static void cb_brwsViewer(Fl_Hold_Browser*, void*) {
	int sel = brwsViewer->value();
	if (sel == 0) return;
	if (sel > progdefaults.VIEWERchannels) return;
	active_modem->set_freq(brwsFreq[sel - 1]);
}

static void cb_Seek(Fl_Input *, void *)
{
	tofind = inpSeek->value();
	for (size_t i = 0; i < tofind.length(); i++)
		tofind[i] = toupper(tofind[i]);
}

Fl_Double_Window* createViewer() {
	Fl_Double_Window* w;
	Fl_Pack *p;

	make_colors();

	w = new Fl_Double_Window(450, 450, "Psk Viewer");
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

    	brwsViewer = new Fl_Hold_Browser(2, 35, 450, 400);
    	brwsViewer->callback((Fl_Callback*)cb_brwsViewer);
		if (progdefaults.VIEWERshowfreq) {
    		brwsViewer->column_widths(cols);
		} else {
			brwsViewer->column_widths(smcols);	
		}
			
		Fl_Pack *p2 = new Fl_Pack(0, 435, 450, 25);
			p2->type(1);
			bx = new Fl_Box(0,435, 10, 25);
    		btnClearViewer = new Fl_Button(10, 435, 65, 25, "Clear");
    		btnClearViewer->callback((Fl_Callback*)cb_btnClearViewer);
    		bx = new Fl_Box(75, 435, 10, 25);
	    	btnCloseViewer = new Fl_Button(85, 435, 65, 25, "Close");
    		btnCloseViewer->callback((Fl_Callback*)cb_btnCloseViewer);
    		bx = new Fl_Box(150, 435, 300, 25);
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
	  	
  	for (int i = 0; i < progdefaults.VIEWERchannels; i++) {
  		if (pskviewer) freq = pskviewer->get_freq(24 - i);
  		else 		   freq = progdefaults.VIEWERstart + 100 * (24 - i);
  		if (rfc) {
  			if (usb) dfreq = rfc + freq;
  			else     dfreq = rfc - freq;
  		} else dfreq = freq;
  			
  		brwsFreq[i] = freq;
//		sprintf(szFreq,"@f%s%s%11.3f", bkselect.c_str(), white.c_str(), dfreq / 1.0e3);
  		if (progdefaults.VIEWERshowfreq)
			sprintf(szFreq,"@f%s%s%11.3f", bkselect.c_str(), white.c_str(), dfreq / 1.0e3);
		else
			sprintf(szFreq,"@f%s%s%3d", bkselect.c_str(), white.c_str(), 25 - i);	

  		bwsrfreq = szFreq;
  		bwsrfreq += '\t';
  		bwsrfreq += bkgnd[i % 2];
  		brwsViewer->text(i + 1, bwsrfreq.c_str());
  	}
	if (progdefaults.VIEWERshowfreq) {
    	brwsViewer->column_widths(cols);
	} else {
		brwsViewer->column_widths(smcols);	
	}
}

void viewaddchr(int ch, int freq, char c) {
	if (!dlgViewer) return;

	if (rfc != wf->rfcarrier() || usb != wf->USB()) viewer_redraw();
		
	static string nuline;
	int index = progdefaults.VIEWERchannels - 1 - ch;
	if (progdefaults.VIEWERmarquee) {
		if (bwsrline[index].length() > 40 ) {
			bwsrline[index].erase(0,1); //2,1);
			ucaseline[index].erase(0,1); //2,1);
		}
		if (c >= ' ' && c <= 'z') {
			bwsrline[index] += c;
			ucaseline[index] += toupper(c);
		} else if (c == '\n') {
			bwsrline[index] += ' ';
			ucaseline[index] += ' ';
		}
	} else {
		if (c >= ' ' && c <= 'z') {
			bwsrline[index] += c;
			ucaseline[index] += toupper(c);
		} else if (c == '\n') {
			bwsrline[index] += ' ';
			ucaseline[index] += ' ';
		}
		if (bwsrline[index].length() > 42) {
			bwsrline[index] = "";
			ucaseline[index] = "";
		}
	}
	brwsFreq[index] = freq;
	if (rfc) {
  		if (usb) dfreq = rfc + freq;
  		else     dfreq = rfc - freq;
  	} else dfreq = freq;
	
//	sprintf(szFreq,"@f%s%s%11.3f", bkselect.c_str(), white.c_str(), dfreq / 1.0e3);
 	if (progdefaults.VIEWERshowfreq)
		sprintf(szFreq,"@f%s%s%11.3f", bkselect.c_str(), white.c_str(), dfreq / 1.0e3);
	else
			sprintf(szFreq,"@f%s%s%3d", bkselect.c_str(), white.c_str(), ch);	
	
  	bwsrfreq = szFreq;
  	bwsrfreq += '\t';
  	bwsrfreq += bkgnd[(index) % 2];
	nuline = bwsrfreq;
	nuline.append("@f");

	if (!tofind.empty())
		if (ucaseline[index].find(tofind) != string::npos)
			nuline.append(dkred);
	else if (!progdefaults.myCall.empty())
		if (ucaseline[index].find(progdefaults.myCall) != string::npos)
			nuline.append(dkgreen);
			
	nuline.append(bwsrline[index]);
	brwsViewer->text(1 + index, nuline.c_str());
	brwsViewer->redraw();
}
