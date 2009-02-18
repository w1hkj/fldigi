#include <config.h>

#include "macros.h"

#include "main.h"

#include "fl_digi.h"
#include "configuration.h"
#include "confdialog.h"
#include "logger.h"

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
label[0] = "CQ @>|";
text[0] = "<TX>\n\
CQ CQ CQ de <MYCALL> <MYCALL> <MYCALL>\n\
CQ CQ CQ de <MYCALL> <MYCALL> <MYCALL> pse k\n\
<RX>";

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

label[5] = "Me";
text[5] = " my name is <MYNAME> <MYNAME> ";

label[6] = "QTH";
text[6] = " my QTH is <MYQTH>, loc: <MYLOC> ";

label[7] = "Brag";
text[7] = "\n\
<< <MYCALL>, <MYNAME> >>\n\
Age:   \n\
Rig:   \n\
Pwr:   \n\
Ant:   \n\
OS:    Linux\n\
Soft:  <VER>\n\
Web:   \n\
Email: ";

label[8] = "Tx @>>";
text[8] = "<TX>";

label[9] = "Rx @||";
text[9] = "<RX>";

label[10] = "";
text[10] = "";

label[11] = "";
text[11] = "";

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
text[18] = "<TX>CQ CQ CQ DE <MYCALL> <MYCALL> <MYCALL>  CQ CQ CQ DE <MYCALL> K<RX>";

label[19] = "";
text[19] = "";

label[20] = "CQ @-3+";
text[20] = "<TX>\n\
<IDLE:5>CQ CQ CQ de <MYCALL> <MYCALL> <MYCALL>\n\
CQ CQ CQ de <MYCALL> <MYCALL> <MYCALL> k<RX><TIMER:15>";

label[21] = "CQ-ID @>|";
text[21] = "<TX><ID>\n\
CQ CQ CQ de <MYCALL> <MYCALL> <MYCALL>\n\
CQ CQ CQ de <MYCALL> <MYCALL> <MYCALL> pse k\n\
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
	{	"digipan.pal","  0;  0;  0",
        "  0;  0;164","  3;106;220","  0;215;215","229;229;229",
        "  0;255;  0","255;255;  0","255;128;  0","255;  0;  0"
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

