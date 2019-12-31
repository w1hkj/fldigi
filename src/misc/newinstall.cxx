// ----------------------------------------------------------------------------
//
// 	newinstall.cxx
//
// Copyright (C) 2007-2014 Dave Freese, W1HKJ
// Copyright (C) 2010 Stelios Bounanos, M0GLD
//
// This file is part of fldigi.
//
// Fldigi is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#include <config.h>

#include "macros.h"

#include "main.h"

#include "fl_digi.h"
#include "configuration.h"
#include "confdialog.h"
#include "record_browse.h"
#include "record_loader.h"
#include "logger.h"
#include "tabdefs.h"

#include <string>
#include <ctime>
#include <iostream>
#include <fstream>
#include <sys/stat.h>

using namespace std;

static string label[24];
static string text[24];

void newmacros()
{
label[0] = "RsID CQ";
text[0] = "<TXRSID:on><TX>\n\
CQ CQ CQ de <MYCALL> <MYCALL> <MYCALL>\n\
CQ CQ CQ de <MYCALL> <MYCALL> <MYCALL> pse k\n\
<RX><@TXRSID:off>";

label[1] = "ANS @>|";
text[1] = "<TX>\
<CALL> <CALL> de <MYCALL> <MYCALL> <MYCALL> kn\n\
<RX>";

label[2] ="QSO @>>";
text[2] = "<TX>\n\
<CALL> de <MYCALL> ";

label[3] = "KN @||";
text[3] = " btu <NAME> <CALL> de <MYCALL> k\n\
<RX>";

label[4] = "SK @||";
text[4] = "\n\
tnx fer QSO <NAME>, 73, God bless.\n\
<ZDT> <CALL> de <MYCALL> sk\n\
<RX>";

label[5] = "Me/Qth";
text[5] = "\n\
op:  <MYNAME>\n\
qth: <MYQTH>\n\
loc: <MYLOC>\n";

label[6] = "Brag";
text[6] = "\n\
<< <MYCALL>, <MYNAME> >>\n\
Age:   \n\
Rig:   \n\
Pwr:   \n\
Ant:   \n\
OS:    \n\
Soft:  <VER>\n\
Web:   \n\
Email: ";

label[7] = "";
text[7] = "";

label[8] = "T/R";
text[8] = "<TX/RX>";

label[9] = "Tx @>>";
text[9] = "<TX>";

label[10] = "Rx @||";
text[10] = "<RX>";

label[11] = "TX @>|";
text[11] = "<TX>\n\
de <MYCALL> k\n\
<RX>";

label[12] = "C Ans @>|";
text[12] = "<TX>de <MYCALL> <MYCALL><RX>";

label[13] = "C rpt @>|";
text[13] = "<TX><CNTR> <CNTR> QSL DE <MYCALL> K\n\
<RX>";

label[14] = "C Rep @>|";
text[14] = "<TX>\n\
<CALL> RR NBR <CNTR> <CNTR> TU DE <MYCALL> K\n\
<RX>";

label[15] = "C Incr";
text[15] = "<INCR>";

label[16] = "C Decr";
text[16] = "<DECR>";

label[17] = "Log QSO";
text[17] = "<LOG><INCR>";

label[18] = "CW-CQ @>|";
text[18] = "<TX>CQ CQ CQ DE <MYCALL> <MYCALL>  CQ CQ CQ DE <MYCALL> K<RX>";

label[19] = "";
text[19] = "";

label[20] = "CQ @-3+";
text[20] = "<TX>\n\
<IDLE:5>CQ CQ CQ de <MYCALL> <MYCALL>\n\
CQ CQ CQ de <MYCALL> k<RX><TIMER:15>";

label[21] = "CQ-ID @>|";
text[21] = "<TX><ID>\n\
CQ CQ CQ de <MYCALL> <MYCALL>\n\
CQ CQ CQ de <MYCALL> <MYCALL> K\n\
<RX>";

label[22] = "";
text[22] = "";

label[23] = "";
text[23] = "";

	for (int i = 0; i < 24; i++) {
		macros.text[i] = text[i];
		macros.name[i] = label[i];
	}
	for (int i = 24; i < MAXMACROS; i++) {
		macros.text[i] = "";
		macros.name[i] = "";
	}
}

struct paldata {
	const char *fname;
	const char *rgbstr0;
	const char *rgbstr1;
	const char *rgbstr2;
	const char *rgbstr3;
	const char *rgbstr4;
	const char *rgbstr5;
	const char *rgbstr6;
	const char *rgbstr7;
	const char *rgbstr8;
};

paldata palfiles[] = {
	{	"banana.pal", "  0;  0;  0",
		" 59; 59; 27","119;119; 59","179;179; 91","227;227;123",
		"235;235;151","239;239;183","247;247;219","255;255;255"
	},
	{	"blue1.pal","  0;  0;  2",
		"  0;  0; 64","  7; 11;128"," 39; 47;192"," 95;115;217",
		"151;179;231","187;203;239","219;227;247","255;255;255"
	},
	{	"blue2.pal","  3;  3; 64",
		"  7; 11;128"," 39; 47;192"," 95;115;217","151;179;231",
		"187;203;239","219;227;247","255;255;255","255;253;108"
	},
	{	"blue3.pal","  0;  0;  0",
		" 31; 31; 31"," 63; 63; 63"," 91; 91;167","119;119;191",
		"155;155;219","191;191;191","223;223;223","255;255;255"
	},
	{	"brown.pal","  0;  0;  0",
		"107; 63; 11","175; 95; 31","199;119; 43","215;163; 63",
		"231;211; 87","243;247;111","247;251;179","255;255;255"
	},
	{	"cyan1.pal","  0;  0;  0",
		"  5; 10; 10"," 22; 42; 42"," 52; 99; 99"," 94;175;175",
		"131;209;209","162;224;224","202;239;239","255;255;255"
	},
	{	"cyan2.pal","  0;  0;  0",
		" 35; 51; 51"," 75;103;103","115;159;159","155;211;211",
		"183;231;231","203;239;239","227;247;247","255;255;255"
	},
	{	"cyan3.pal","  0;  0;  0",
		" 94;114;114","138;162;162","171;201;201","199;232;232",
		"216;243;243","228;247;247","241;251;251","255;255;255"
	},
	{   "default.pal", "  0;  0;  0",
        "  0;  6;136","  0; 19;198","  0; 32;239","172;167;105",
        "194;198; 49","225;228;107","255;255;  0","255; 51;  0"
    },
	{	"digipan.pal","  0;  0;  0",
        "  0;  0; 64","  0;  0;128","  0;  0;217","150;147; 92",
        "183;186; 46","225;228;107","255;255;  0","255; 51;  0"
	},
	{	"fldigi.pal","  0;  0;  0",
        "  0;  0;177","  3;110;227","  0;204;204","223;223;223",
        "  0;234;  0","244;244;  0","250;126;  0","244;  0;  0"
	},
	{	"gmfsk.pal","  0;  0;256",
		"  0; 62;194","  0;126;130","  0;190; 66","  0;254;  2",
		" 62;194;  0","126;130;  0","190; 66;  0","254;  2;  0"
	},
	{	"gray1.pal","  0;  0;  0",
		" 69; 69; 69"," 99; 99; 99","121;121;121","140;140;140",
		"157;157;157","172;172;172","186;186;186","199;199;199"
	},
	{	"gray2.pal","  0;  0;  0",
		" 88; 88; 88","126;126;126","155;155;155","179;179;179",
		"200;200;200","220;220;220","237;237;237","254;254;254"
	},
	{	"green1.pal","  0;  0;  0",
		"  0; 32;  0","  0; 64;  0","  0; 96;  0","  0;128;  0",
		"  0;160;  0","  0;192;  0","  0;224;  0","255;255;255"
	},
	{	"green2.pal","  0;  0;  0",
		"  0; 60;  0","  0;102;  0","  0;151;  0","  0;242;  0",
		"255;255; 89","240;120;  0","255;148; 40","255;  0;  0"
	},
	{	"jungle.pal","  0;  0;  0",
		"107; 67;  0","223;143;  0","255;123; 27","255; 91; 71",
		"255;195; 95","195;255;111","151;255;151","255;255;255"
	},
	{	"negative.pal","255;255;255",
		"223;223;223","191;191;191","159;159;159","127;127;127",
		" 95; 95; 95"," 63; 63; 63"," 31; 31; 31","  0;  0;  0"
	},
	{	"orange.pal","  0;  0;  0",
		" 63; 27;  0","131; 63;  0","199; 95;  0","251;127; 11",
		"251;155; 71","251;187;131","251;219;191","255;255;255"
	},
	{	"pink.pal","  0;  0;  0",
		" 63; 35; 35","135; 75; 75","203;111;111","255;147;147",
		"255;175;175","255;199;199","255;227;227","255;255;255"
	},
	{	"rainbow.pal","  0;  0;163",
		"  0; 87;191","  0;207;219","  0;247;139","  0;255; 23",
		" 95;255;  0","219;255;  0","255;171;155","255;255;255"
	},
	{	"scope.pal","  0;  0;  0",
		"  0;  0;167","  0; 79;255","  0;239;255","  0;255; 75",
		" 95;255;  0","255;255;  0","255;127;  0","255;  0;  0"
	},
	{	"sunburst.pal","  0;  0;  0",
		"  0;  0; 59","  0;  0;123","131;  0;179","235;  0; 75",
		"255; 43; 43","255;215;111","255;255;183","255;255;255"
	},
	{	"vk4bdj.pal","  0;  0;  0",
		"  0; 32;  0","  0;154;  0","  0;161;  0","  0;177;  0",
		"156;209;144","192;185;183","214;222;224","255;255;255"
	},
	{	"yellow1.pal", "  0;  0;  0",
		" 31; 31;  0"," 63; 63;  0"," 95; 95;  0","127;127;  0",
		"159;159;  0","191;191;  0","223;223;  0","255;255;  0"
	},
	{	"yellow2.pal", "  0;  0;  0",
		" 39; 39;  0"," 75; 75;  0","111;111;  0","147;147;  0",
		"183;183;  0","219;219;  0","255;255;  0","255;255;255"
	},
	{	"yl2kf.pal","  0;  0;  0",
		"  0;  0;119","  7; 11;195"," 39; 47;159"," 95;115;203",
		"151;179;255","187;203;255","219;227;255","255;255;  5"
	},
	{	0,0,
		0,0,0,0,
		0,0,0,0
	}
};

void create_new_palettes()
{
	paldata *pd = palfiles;
	string Filename;
	while (pd->fname) {
		Filename = PalettesDir;
		Filename.append(pd->fname);
		ofstream pfile(Filename.c_str());
		pfile << pd->rgbstr0 << endl;
		pfile << pd->rgbstr1 << endl;
		pfile << pd->rgbstr2 << endl;
		pfile << pd->rgbstr3 << endl;
		pfile << pd->rgbstr4 << endl;
		pfile << pd->rgbstr5 << endl;
		pfile << pd->rgbstr6 << endl;
		pfile << pd->rgbstr7 << endl;
		pfile << pd->rgbstr8 << endl;
		pfile.close();
		pd++;
	}
}

void create_new_macros()
{
	string Filename = MacrosDir;
	Filename.append("macros.mdf");
	newmacros();
	macros.saveMacros(Filename);
}

// =============================================================================

#include <vector>
#include <sstream>

#include <FL/Fl_Window.H>
#include <FL/Fl_Wizard.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <FL/fl_draw.H>

#include "configuration.h"
#include "confdialog.h"
#include "icons.h"
#include "gettext.h"

class Wizard : public Fl_Window
{
public:
	struct wizard_tab {
		Fl_Group* tab;
		Fl_Group* parent;
		int position;
		int x, y, w, h;
	};

	Wizard(int w, int h, const char* l = 0)
		: Fl_Window(w, h, l)
	{ create_wizard(); }

	~Wizard() { destroy_wizard(); }

private:
	void create_wizard(void);
	Fl_Group* make_intro(void);
	void destroy_wizard(void);

	static void wizard_cb(Fl_Widget* w, void* arg);

	Fl_Wizard *wizard;
	Fl_Button *prev, *next, *done;
	typedef vector<wizard_tab> tab_t;
	tab_t tabs;
};

static int btn_h = 22;
static int pad = 4;

void Wizard::create_wizard(void)
{
	createRecordLoader();

	callback(wizard_cb, this);
	xclass(PACKAGE_TARNAME);

	wizard = new Fl_Wizard(0, 0, w(), h());
	wizard->end();

	// create the buttons
	fl_font(FL_HELVETICA, FL_NORMAL_SIZE);
	struct {
		Fl_Button** button;
		const char* label;
	} buttons[] = {
		{ &done, _("Finish") },
		{ &next, _("Next") },
		{ &prev, _("Back") },
	};
	int x = w() - 10;
	int bw = 0;
	for (size_t i = 0; i < sizeof(buttons)/sizeof(*buttons); i++) {
		bw = fl_width(buttons[i].label) + 10;
		Fl_Button* b = *buttons[i].button = new Fl_Button(
						(x = x - bw - 10), wizard->y() + grpOperator->h() + pad,
						bw, btn_h,
						buttons[i].label);
		b->callback(wizard_cb, this);
	}
	done->activate();
	prev->deactivate();
	next->activate();

	end();
	position(MAX(0, fl_digi_main->x() + (fl_digi_main->w() - w()) / 2),
		 MAX(0, fl_digi_main->y() + (fl_digi_main->h() - h()) / 2));

	// populate the Fl_Wizard group
	struct wizard_tab tabs_[] = {
		{ NULL },
		{ grpOperator },
		{ grpSoundDevices },
		{ grpRigFlrig },
		{ grpRigCat },
		{ grpRigHamlib },
		{ tabDataFiles }
	};

	tabs.resize(sizeof(tabs_)/sizeof(*tabs_));
	memcpy(&tabs[0], tabs_, sizeof(tabs_));

	for (tab_t::iterator i = tabs.begin() + 1; i != tabs.end(); ++i) {
		i->parent = i->tab->parent();
		i->position = i->parent->find(i->tab);
		i->x = i->tab->x();
		i->y = i->tab->y();
		i->w = i->tab->w();
		i->h = i->tab->h();
	}

	tabs[0].tab = make_intro();
	tabs[0].w = grpOperator->w();
	tabs[0].h = grpOperator->h();

	for (tab_t::iterator i = tabs.begin(); i != tabs.end(); ++i) {
		i->tab->resize(0, 0, grpOperator->w(), grpOperator->h());
		wizard->add(i->tab);
	}
	wizard->value(tabs[0].tab);
}

void Wizard::destroy_wizard(void)
{
	// re-parent tabs
	for (tab_t::const_iterator i = tabs.begin() + 1; i != tabs.end(); ++i) {
		i->parent->insert(*i->tab, i->position);
		if (i < tabs.end() - 1)
			i->tab->hide();
		i->tab->resize(i->x, i->y, i->w, i->h);
		i->parent->init_sizes();
	}

	Fl_Button* b[] = { prev, next, done };
	for (size_t i = 0; i < sizeof(b)/sizeof(*b); i++)
		delete b[i];

}

void Wizard::wizard_cb(Fl_Widget* w, void* arg)
{
	Wizard* wiz = static_cast<Wizard*>(arg);

	if (w == wiz || w == wiz->done) {
		delete wiz;
		return;
	}

	if (w == wiz->prev)
		wiz->wizard->prev();
	else if (w == wiz->next)
		wiz->wizard->next();

	Fl_Group* cur = static_cast<Fl_Group*>(wiz->wizard->value());

// modify buttons
	if (cur == wiz->tabs[0].tab) {
		wiz->prev->deactivate();
		wiz->next->activate();
		wiz->done->activate();
	} else if (cur == wiz->tabs.back().tab) {
		wiz->prev->activate();
		wiz->next->deactivate();
		wiz->done->activate();
	} else {
		wiz->prev->activate();
		wiz->next->activate();
		wiz->done->activate();
	}
	wiz->prev->show();
	wiz->next->show();
	wiz->done->show();
	wiz->prev->redraw();
	wiz->next->redraw();
	wiz->done->redraw();
}

Fl_Group* Wizard::make_intro(void)
{
	Fl_Group* intro = new Fl_Group(
					0, 0,
					grpOperator->w(), grpOperator->h(),
					"");

	ostringstream help_;
	help_ << "\n" << 
_("The wizard will guide you through the basic fldigi settings:") << "\n\n\t- " <<
_("Operator")                 << "\n\t- " <<
_("Sound Card Interface")     << "\n\t- " <<
_("Transceiver control, flrig/rigcat/hamlib")      << "\n\n" <<
_("Feel free to skip any pages or exit the wizard at any time") << ". " <<
_("All settings shown here can be changed later via the Configure menu") << '.';

	Fl_Box* help = new Fl_Box(20, intro->y() + intro->h() / 4,
				  intro->w() - 40, 3 * intro->h()/ 4);
	help->align(FL_ALIGN_TOP_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_WRAP);
	help->copy_label(help_.str().c_str());

	intro->end();

	return intro;
}


void show_wizard(int argc, char** argv)
{
	Wizard* w = new Wizard(grpOperator->w(), grpOperator->h() + (btn_h + 2 * pad), _("Fldigi configuration wizard"));

	if (argc && argv)
		w->show(argc, argv);
	else {
		w->show();
		w->set_modal();
	}
	first_use = true;
}
