// special test version of Viewer.cxx

#include <config.h>

#include "Viewer.h"
#include "main.h"
#include "viewpsk.h"
#include "configuration.h"
#include "status.h"
#include "waterfall.h"
#include <FL/Enumerations.H>
#include <FL/Fl_Slider.H>
#include <FL/fl_ask.H>

#include <string>

// To disable REGULAR EXPRESSION EVALUATION FOR THE FIND STRING
// uncomment the following two lines
// #undef HAVE_REGEX_H
// #define HAVE_REGEX_H 0

#if HAVE_REGEX_H
#    if HAVE_SYS_TYPES_H
#        include <sys/types.h>
#    endif
#    include <regex.h>
#endif


string bwsrfreq;
string bwsrline[MAXCHANNELS];
#if !HAVE_REGEX_H
string ucaseline[MAXCHANNELS];
string tofind;
#endif

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
string bkgnd[2];

int cols[] = {100, 0};
int cwidth;
int cheight;
int rfwidth;
int chwidth;
int sbarwidth = 16;
int border = 4;

//unsigned int nchars = 80;

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
 		snprintf(szLine, sizeof(szLine), "%10.3f", dfreq / 1000.0);
  	else
 		snprintf(szLine, sizeof(szLine), "%3d", progdefaults.VIEWERchannels - i);	

	fline = "@f";
	fline += bkselect.c_str();
	fline += white.c_str();
	fline += szLine;
  	fline += '\t';
	fline += bkgnd[i % 2];
	fline += "@f";

	return fline;
}

#if HAVE_REGEX_H
regex_t* seek_re = 0;
void re_comp(const char* needle)
{
        if (seek_re)
                regfree(seek_re);
        else
                seek_re = new regex_t;
        if (!(needle && *needle && regcomp(seek_re, needle, REG_EXTENDED | REG_ICASE | REG_NOSUB) == 0)) {
                delete seek_re;
                seek_re = 0;
        }
}
// match haystack against seek_re
bool re_find(const char* haystack)
{
        return seek_re && !regexec(seek_re, haystack, 0, 0, REG_NOTBOL | REG_NOTEOL);
}
#endif // HAVE_REGEX_H

void pskBrowser::resize(int x, int y, int w, int h) {
	unsigned int nuchars = (w - cols[0] - (sbarwidth + border)) / cwidth;
	string bline;
	if (nuchars < progStatus.VIEWERnchars) {
		Fl_Hold_Browser::clear();
		for (int i = 0; i < progdefaults.VIEWERchannels; i++) {
			size_t len = bwsrline[i].length();
			if (len > nuchars)
				bwsrline[i] = bwsrline[i].substr(len - nuchars);
			bline = freqformat(i);
#if !HAVE_REGEX_H
			if (!tofind.empty()) {
				if (ucaseline[i].find(tofind) != string::npos)
					bline.append(dkred);
			}
			else if (!progdefaults.myCall.empty())
				if (ucaseline[i].find(progdefaults.myCall) != string::npos)
					bline.append(dkgreen);
#else
			if (re_find(bwsrline[i].c_str()))
				bline.append(dkred);
			else if (!progdefaults.myCall.empty() &&
					strcasestr(bwsrline[i].c_str(), progdefaults.myCall.c_str()))
				bline.append(dkgreen);
#endif
			bline.append("@.").append(bwsrline[i]);
			Fl_Hold_Browser::add(bline.c_str());
		}
	}
	progStatus.VIEWERnchars = nuchars;
	progStatus.VIEWERxpos = dlgViewer->x();
	progStatus.VIEWERypos = dlgViewer->y();
	Fl_Hold_Browser::resize(x,y,w,h);
}		

Fl_Double_Window *dlgViewer = (Fl_Double_Window *)0;

Fl_Button *btnCloseViewer=(Fl_Button *)0;
Fl_Button *btnClearViewer=(Fl_Button *)0;
pskBrowser *brwsViewer=(pskBrowser *)0;
Fl_Input  *inpSeek = (Fl_Input *)0;
Fl_Slider *sldrViewerSquelch = (Fl_Slider *)0;
//Fl_Light_Button *chkBeep = 0;

// Adjust and return fg color to ensure good contrast with bg
static Fl_Color adjust_color(Fl_Color fg, Fl_Color bg)
{
	Fl_Color adj;
	unsigned max = 24;
	while ((adj = fl_contrast(fg, bg)) != fg  &&  max--)
		fg = (adj == FL_WHITE) ? fl_color_average(fg, FL_WHITE, .9)
				       : fl_color_average(fg, FL_BLACK, .9);
	return fg;
}
static void make_colors()
{
	char tempstr[20];

	snprintf(tempstr, sizeof(tempstr), "@b@C%d",
		 adjust_color(fl_color_cube(128 * (FL_NUM_RED - 1) / 255,
					    0 * (FL_NUM_GREEN - 1) / 255,
					    0 * (FL_NUM_BLUE - 1) / 255),
			      FL_BACKGROUND2_COLOR)); // dark red
	dkred = tempstr;

	snprintf(tempstr, sizeof(tempstr), "@b@C%d",
		 adjust_color(fl_color_cube(0 * (FL_NUM_RED - 1) / 255,
					    128 * (FL_NUM_GREEN - 1) / 255,
					    0 * (FL_NUM_BLUE - 1) / 255),
			      FL_BACKGROUND2_COLOR)); // dark green
	dkgreen = tempstr;

	snprintf(tempstr, sizeof(tempstr), "@b@C%d",
		 adjust_color(fl_color_cube(0 * (FL_NUM_RED - 1) / 255,
					    0 * (FL_NUM_GREEN - 1) / 255,
					    128 * (FL_NUM_BLUE - 1) / 255),
			      FL_BACKGROUND2_COLOR)); // dark blue
	dkblue = tempstr;

	snprintf(tempstr, sizeof(tempstr), "@C%d", FL_FOREGROUND_COLOR); // foreground
	white = tempstr;

	snprintf(tempstr, sizeof(tempstr), "@B%d",
		 adjust_color(FL_SELECTION_COLOR, FL_FOREGROUND_COLOR)); // default selection color bkgnd
	bkselect = tempstr;

	snprintf(tempstr, sizeof(tempstr), "@B%d", FL_BACKGROUND2_COLOR); // background for odd rows
	bkgnd[0] = tempstr;

	Fl_Color bg2 = fl_color_average(FL_BACKGROUND2_COLOR, FL_BLACK, .9);
	if (bg2 == FL_BLACK)
		bg2 = fl_color_average(FL_BACKGROUND2_COLOR, FL_WHITE, .9);
	snprintf(tempstr, sizeof(tempstr), "@B%d", adjust_color(bg2, FL_FOREGROUND_COLOR)); // even rows
	bkgnd[1] = tempstr;
}

static void evalcwidth()
{
	fl_font(FL_COURIER, FL_NORMAL_SIZE);
	cwidth = (int)fl_width("W");
	cheight = fl_height();
	rfwidth = 11 * cwidth;
	chwidth = 4 * cwidth;
}

static void cb_btnCloseViewer(Fl_Button*, void*) {
	progStatus.VIEWERxpos = dlgViewer->x();
	progStatus.VIEWERypos = dlgViewer->y();
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
  		bwsrline[i].clear();
#if !HAVE_REGEX_H
  		ucaseline[i].clear();
#endif
  		brwsViewer->add(bline.c_str());
  	}
	if (progdefaults.VIEWERshowfreq)
		cols[0] = rfwidth;
	else
		cols[0] = chwidth;
  	brwsViewer->column_widths(cols);
}

void initViewer()
{
	if (!pskviewer) return;
	if (!dlgViewer) return;
	pskviewer->init();
	ClearViewer();
	dlgViewer->resize(dlgViewer->x(), dlgViewer->y(),
	                  progStatus.VIEWERnchars * cwidth + cols[0] + (sbarwidth + border),
	                  cheight * progdefaults.VIEWERchannels + 50 + border);
}

// i in [1, progdefaults.VIEWERchannels]
static void set_freq(int i, int freq)
{
	string new_line;

	if (freq == 0) // reset
		freq = progdefaults.VIEWERstart + 100 * (progdefaults.VIEWERchannels - i);

	pskviewer->set_freq(progdefaults.VIEWERchannels - i, freq);
	new_line.append(freqformat(i - 1)).append("@.").append(bwsrline[i - 1]);
	brwsViewer->replace(i, new_line.c_str());
}

static void cb_btnClearViewer(Fl_Button*, void*) {
	if (Fl::event_button() == FL_LEFT_MOUSE)
		ClearViewer();
	else
		for (int i = 1; i <= progdefaults.VIEWERchannels; i++)
			set_freq(i, 0);
}

static void cb_brwsViewer(Fl_Hold_Browser*, void*) {
	int sel = brwsViewer->value();
	if (sel == 0 || sel > progdefaults.VIEWERchannels)
		return;

	switch (Fl::event_button()) {
	case FL_LEFT_MOUSE:
		ReceiveText->addchr('\n', FTextBase::ALTR);
		ReceiveText->addstr(bwsrline[sel - 1].c_str(), FTextBase::ALTR);
		active_modem->set_freq(brwsFreq[sel - 1]);
		break;
	case FL_MIDDLE_MOUSE: // copy from modem
		set_freq(sel, active_modem->get_freq());
		break;
	case FL_RIGHT_MOUSE: // reset
		set_freq(sel, 0);
		break;
	default:
		break;
	}
}

static void cb_Seek(Fl_Input *, void *)
{
#if !HAVE_REGEX_H
	tofind = inpSeek->value();
	for (size_t i = 0; i < tofind.length(); i++)
		tofind[i] = toupper(tofind[i]);
#else
	re_comp(inpSeek->value());
#endif
}

static void cb_Squelch(Fl_Slider *, void *)
{
	progdefaults.VIEWERsquelch = sldrViewerSquelch->value();
	progdefaults.changed = true;
}

Fl_Double_Window* createViewer() {
	Fl_Double_Window* w;
	Fl_Pack *p;

	make_colors();
	evalcwidth();
	if (progdefaults.VIEWERshowfreq)
		cols[0] = rfwidth;
	else
		cols[0] = chwidth;
	
	static int viewerwidth = (progStatus.VIEWERnchars * cwidth) + cols[0] + sbarwidth + border;
	static int viewerheight = 50 + cheight * progdefaults.VIEWERchannels + border;

	w = new Fl_Double_Window(progStatus.VIEWERxpos, progStatus.VIEWERypos, viewerwidth, viewerheight, "Psk Viewer");
	w->xclass(PACKAGE_NAME);
	p = new Fl_Pack(0,0,viewerwidth, viewerheight);
		Fl_Pack *p1 = new Fl_Pack(0, 0, viewerwidth, 25);
			p1->type(1);
			Fl_Box *bx = new Fl_Box(0,0,50, 25);
	    	inpSeek = new Fl_Input(50, 5, 200, 25, "Find: "); 
    		inpSeek->callback((Fl_Callback*)cb_Seek);
    		inpSeek->when(FL_WHEN_CHANGED);
			inpSeek->textfont(FL_SCREEN);
    		inpSeek->value("CQ");
//			chkBeep = new Fl_Light_Button(inpSeek->x() + border, inpSeek->y(), 60, inpSeek->h(), "Beep");
    		bx = new Fl_Box(250, 5, 200, 25);
    		p1->resizable(bx);
    	p1->end();
    	inpSeek->do_callback();

    	brwsViewer = new pskBrowser(2, 25, viewerwidth, viewerheight - 50);
    	brwsViewer->callback((Fl_Callback*)cb_brwsViewer);
    	brwsViewer->column_widths(cols);
		
		Fl_Pack *p2 = new Fl_Pack(0, viewerheight - 25, viewerwidth, 25);
			p2->type(1);
			bx = new Fl_Box(0,viewerheight - 25, 10, 25);
    		btnClearViewer = new Fl_Button(10, viewerheight - 25, 65, 25, "Clear");
    		btnClearViewer->callback((Fl_Callback*)cb_btnClearViewer);
		btnClearViewer->tooltip("Left click to clear text\nRight click to reset frequencies");
    		bx = new Fl_Box(75, viewerheight - 25, 10, 25);
	    	btnCloseViewer = new Fl_Button(85, viewerheight - 25, 65, 25, "Close");
    		btnCloseViewer->callback((Fl_Callback*)cb_btnCloseViewer);
    		bx = new Fl_Box(140, viewerheight - 25, 5, 25);
    		sldrViewerSquelch = new Fl_Slider(145, viewerheight - 25, 200, 25);
    		sldrViewerSquelch->tooltip("Set Viewer Squelch");
    		sldrViewerSquelch->type(FL_HOR_NICE_SLIDER);
    		sldrViewerSquelch->range(0.0, 100.0);
    		sldrViewerSquelch->value(progdefaults.VIEWERsquelch);
    		sldrViewerSquelch->callback((Fl_Callback*)cb_Squelch);
    		bx = new Fl_Box(345, viewerheight - 25, 25, 25);
    		p2->resizable(bx);
		p2->end();
		p->resizable(brwsViewer);
	p->end();
	w->resizable(p);
    w->end();
    w->callback((Fl_Callback*)cb_btnCloseViewer);
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
		if (bwsrline[index].length() > progStatus.VIEWERnchars ) {
			bwsrline[index].erase(0,1);
#if !HAVE_REGEX_H
			ucaseline[index].erase(0,1);
#endif
		}
		if (c >= ' ' && c <= '~') {
			bwsrline[index] += c;
#if !HAVE_REGEX_H
			ucaseline[index] += toupper(c);
#endif
		} else {
			bwsrline[index] += ' ';
#if !HAVE_REGEX_H
			ucaseline[index] += ' ';
#endif
		}
	} else {
		if (c >= ' ' && c <= '~') {
			bwsrline[index] += c;
#if !HAVE_REGEX_H
			ucaseline[index] += toupper(c);
#endif
		} else {
			bwsrline[index] += ' ';
#if !HAVE_REGEX_H
			ucaseline[index] += ' ';
#endif
		}
		if (bwsrline[index].length() > progStatus.VIEWERnchars)
			bwsrline[index].clear();
	}
	nuline = freqformat(index);
#if !HAVE_REGEX_H
	if (!tofind.empty()) {
		if (ucaseline[index].find(tofind) != string::npos)
			nuline.append(dkred);
	}
	else if (!progdefaults.myCall.empty())
		if (ucaseline[index].find(progdefaults.myCall) != string::npos)
			nuline.append(dkgreen);
#else
	if (re_find(bwsrline[index].c_str())) {
		nuline.append(dkred);
//		if (chkBeep->value())
//			fl_beep();
	}
	else if (!progdefaults.myCall.empty() &&
		 strcasestr(bwsrline[index].c_str(), progdefaults.myCall.c_str())) {
		nuline.append(dkgreen);
//		if (chkBeep->value())
//			fl_beep();
	}
#endif
	nuline.append("@.").append(bwsrline[index]);
	brwsViewer->text(1 + index, nuline.c_str());
	brwsViewer->redraw();
}

void viewclearchannel(int ch)
{
	int index = progdefaults.VIEWERchannels - 1 - ch;
	string nuline = freqformat(index);
	bwsrline[index] = "";
#if !HAVE_REGEX_H
	ucaseline[index] = "";
#endif
	brwsViewer->text( 1 + index, nuline.c_str());
	brwsViewer->redraw();
}

void viewer_paste_freq(int freq)
{
	if (!dlgViewer)
		return;

	int sel = 1, n = brwsViewer->size();

	for (int i = 0; i < n; i++) {
		if (brwsViewer->selected(i)) {
			brwsViewer->select(i, false);
			sel = i;
			break;
		}
	}

	set_freq(sel, freq);
	brwsViewer->select(WCLAMP(sel+1, 1, n));
}
