#include "Viewer.h"
#include "main.h"
#include "viewpsk.h"
#include "configuration.h"
#include "waterfall.h"
#include <FL/Enumerations.H>

#include <string>

string bwsrfreq;
string bwsrline[MAXCHANNELS];
string ucaseline[MAXCHANNELS];
string tofind;

static int  brwsFreq[MAXCHANNELS];
static int  freq;
static double dfreq;

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
int cwidth;
int cheight;
int rfwidth;
int chwidth;

unsigned int nchars = 40;

extern viewpsk *pskviewer;

string freqformat(int i)
{
static char szLine[80];
string fline;
	if (pskviewer) freq = pskviewer->get_freq(progdefaults.VIEWERchannels - 1 - i);
	else 		   freq = progdefaults.VIEWERstart + 100 * (progdefaults.VIEWERchannels - 1 - i);
	brwsFreq[i] = freq;

	if (rfc != 0) {
		if (usb) dfreq = rfc + freq;
		else     dfreq = rfc - freq;
 	} else 
 		dfreq = freq;
	if (progdefaults.VIEWERshowfreq)
		sprintf(szLine,"%10.3f", dfreq / 1000.0);
	else
		sprintf(szLine,"%3d", progdefaults.VIEWERchannels - i);	
//	if (szLine[0] == ' ') strcpy(szLine, &szLine[1]);
	fline = "@f";
	fline += bkselect.c_str();
	fline += white.c_str();
	fline += szLine;
  	fline += '\t';
	fline += bkgnd[i % 2];
	fline += "@f";

	return fline;
}

void pskBrowser::resize(int x, int y, int w, int h) {
		unsigned int nuchars = (w - cols[0] - 20) / cwidth;
		string bline;
		if (nuchars < nchars) {
			Fl_Hold_Browser::clear();
			for (int i = 0; i < progdefaults.VIEWERchannels; i++) {
				if (bwsrline[i].length() > nuchars)
					bwsrline[i] = bwsrline[i].substr(0,nuchars);

				bline = freqformat(i);

				if (!tofind.empty())
				if (ucaseline[i].find(tofind) != string::npos)
					bline.append(dkred);
				else if (!progdefaults.myCall.empty())
					if (ucaseline[i].find(progdefaults.myCall) != string::npos)
						bline.append(dkgreen);
			
				bline.append(bwsrline[i]);
				Fl_Hold_Browser::add(bline.c_str());
			}
		}
		nchars = nuchars;
		Fl_Hold_Browser::resize(x,y,w,h);
}		

Fl_Double_Window *dlgViewer = (Fl_Double_Window *)0;

Fl_Button *btnCloseViewer=(Fl_Button *)0;
Fl_Button *btnClearViewer=(Fl_Button *)0;
pskBrowser *brwsViewer=(pskBrowser *)0;
Fl_Input  *inpSeek = (Fl_Input *)0;

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

static void evalcwidth()
{
	fl_font(FL_COURIER, 14);
	cwidth = (int)fl_width("W");
	cheight = fl_height();
	rfwidth = 11 * cwidth;
	chwidth = 4 * cwidth;
}

static void cb_btnCloseViewer(Fl_Button*, void*) {
  dlgViewer->hide();
}

void ClearViewer() {
  	brwsViewer->clear();
  	usb = wf->USB();
  	rfc = wf->rfcarrier();
	string bline;  	
  	for (int i = 0; i < progdefaults.VIEWERchannels; i++) {
  		if (pskviewer) freq = pskviewer->get_freq(progdefaults.VIEWERchannels - 1 - i);
  		else 		   freq = progdefaults.VIEWERstart + 100 * (progdefaults.VIEWERchannels - 1 - i);

		bline = freqformat(i);
  		bwsrline[i] = "";
  		ucaseline[i] = "";
  		brwsViewer->add(bline.c_str());
  	}
	if (progdefaults.VIEWERshowfreq)
		cols[0] = rfwidth;
	else
		cols[0] = chwidth;
  	brwsViewer->column_widths(cols);
	nchars = (brwsViewer->w() - cols[0] - 20) / cwidth;
}

void initViewer()
{
	if (!pskviewer) return;
	if (!dlgViewer) return;
	pskviewer->init();
	ClearViewer();
	nchars = (brwsViewer->w() - cols[0]  - 20) / cwidth;
	dlgViewer->resize(dlgViewer->x(), dlgViewer->y(),
	                  dlgViewer->w(), cheight * progdefaults.VIEWERchannels + 54);
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
	evalcwidth();
	
	int viewerwidth = 450;
	int viewerheight = 50 + cheight * progdefaults.VIEWERchannels + 4;

	w = new Fl_Double_Window(viewerwidth, viewerheight, "Psk Viewer");
	p = new Fl_Pack(0,0,viewerwidth, viewerheight);
		Fl_Pack *p1 = new Fl_Pack(0, 0, viewerwidth, 25);
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

    	brwsViewer = new pskBrowser(2, 25, viewerwidth, viewerheight - 50);
    	brwsViewer->callback((Fl_Callback*)cb_brwsViewer);

		if (progdefaults.VIEWERshowfreq)
			cols[0] = rfwidth;
		else
			cols[0] = chwidth;
    	brwsViewer->column_widths(cols);
		nchars = (brwsViewer->w() - cols[0] - 20) / cwidth;
		
		Fl_Pack *p2 = new Fl_Pack(0, viewerheight - 25, viewerwidth, 25);
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
		  	
  	for (int i = 0; i < progdefaults.VIEWERchannels; i++)
  		brwsViewer->text(i + 1, freqformat(i).c_str() );
	if (progdefaults.VIEWERshowfreq)
		cols[0] = rfwidth;
	else
		cols[0] = chwidth;
    brwsViewer->column_widths(cols);
}

void viewaddchr(int ch, int freq, char c) {
	if (!dlgViewer) return;

	if (rfc != wf->rfcarrier() || usb != wf->USB()) viewer_redraw();
		
	static string nuline;
	string bline;

	int index = progdefaults.VIEWERchannels - 1 - ch;
	if (progdefaults.VIEWERmarquee) {
		if (bwsrline[index].length() > nchars ) {
			bwsrline[index].erase(0,1);
			ucaseline[index].erase(0,1);
		}
		if (c >= ' ' && c <= 'z') {
			bwsrline[index] += c;
			ucaseline[index] += toupper(c);
		} else {
			bwsrline[index] += ' ';
			ucaseline[index] += ' ';
		}
	} else {
		if (c >= ' ' && c <= 'z') {
			bwsrline[index] += c;
			ucaseline[index] += toupper(c);
		} else {
			bwsrline[index] += ' ';
			ucaseline[index] += ' ';
		}
		if (bwsrline[index].length() > nchars) {
			bwsrline[index] = "";
			ucaseline[index] = "";
		}
	}
	nuline = freqformat(index);

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
