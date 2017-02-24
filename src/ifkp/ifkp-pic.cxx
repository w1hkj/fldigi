// ----------------------------------------------------------------------------
// ifkppic.cxx  --  ifkp image support functions
//
// Copyright (C) 2015
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.  Adapted from code contained in gifkp source code
// distribution.
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
#include <FL/Fl_Counter.H>
#include <FL/Fl_Choice.H>

#include "gettext.h"
#include "fileselect.h"
#include "timeops.h"

Fl_Double_Window	*ifkppicRxWin = (Fl_Double_Window *)0;
picture				*ifkppicRx = (picture *)0;
Fl_Button			*btnifkpRxReset = (Fl_Button *)0;
Fl_Button			*btnifkpRxSave = (Fl_Button *)0;
Fl_Button			*btnifkpRxClose = (Fl_Button *)0;
Fl_Counter			*ifkpcnt_phase = (Fl_Counter *)0;
Fl_Counter			*ifkpcnt_slant = (Fl_Counter *)0;

Fl_Double_Window	*ifkppicTxWin = (Fl_Double_Window *)0;
picture				*ifkppicTx = (picture *)0;
Fl_Button			*btnifkppicTransmit = (Fl_Button *)0;
Fl_Button			*btnifkppicTxSendAbort = (Fl_Button *)0;
Fl_Button			*btnifkppicTxLoad = (Fl_Button *)0;
Fl_Button			*btnifkppicTxClose = (Fl_Button *)0;
Fl_Choice			*selifkppicSize = (Fl_Choice *)0;

void ifkp_showRxViewer(char c);
void ifkp_createRxViewer();

Fl_Shared_Image	*ifkpTxImg = (Fl_Shared_Image *)0;
unsigned char *ifkpxmtimg = (unsigned char *)0;
unsigned char *ifkpxmtpicbuff = (unsigned char *)0;

#define RAWSIZE 640*(480 + 8)*3*ifkp::IMAGEspp

#define RAWSTART 640*4*3*ifkp::IMAGEspp

unsigned char *ifkp_rawvideo = 0;//[RAWSIZE + 1];

int ifkp_numpixels;
int ifkp_pixelptr;
int ifkp_rawcol;
int ifkp_rawrow;
int ifkp_rawrgb;
char ifkp_image_type = 'S';

char ifkp_txclr_tooltip[24];
char ifkp_txgry_tooltip[24];

static int translate = 0;
static bool enabled = false;

void ifkp_correct_video()
{
	int W = ifkppicRx->w();
	int H = ifkppicRx->h();
	int slant = ifkpcnt_slant->value();
	int vidsize = W * H;
	int index, rowptr, colptr;
	float ratio = (((float)vidsize - (float)slant)/(float)vidsize);
	unsigned char vid[W * H * 3];
	for (int row = 0; row < H; row++) {
		rowptr = W * 3 * row * ifkp::IMAGEspp;
		for (int col = 0; col < W; col++) {
			colptr = ifkp::IMAGEspp*col;
			for (int rgb = 0; rgb < 3; rgb++) {
				index = ratio*(rowptr + colptr + ifkp::IMAGEspp*W*rgb);
				index += RAWSTART - ifkp::IMAGEspp*ifkp_pixelptr;
				if (index < 2) index = 2;
				if (index > RAWSIZE - 2) index = RAWSIZE - 2;
				vid[rgb + 3 * (col + row * W)] = ifkp_rawvideo[index];
			}
		}
	}
	ifkppicRx->video(vid, W*H*3);
}

void ifkp_updateRxPic(unsigned char data, int pos)
{
	if (!ifkppicRxWin->shown()) ifkppicRx->show();

	ifkppicRx->pixel(data, pos);

	int W = ifkppicRx->w();
	if (ifkp_image_type == 'F' || ifkp_image_type == 'p' || ifkp_image_type == 'm') {
		int n = RAWSTART + ifkp::IMAGEspp*(ifkp_rawcol + W * (ifkp_rawrgb + 3 * ifkp_rawrow));
		if (n < RAWSIZE)
			for (int i = 0; i < ifkp::IMAGEspp; i++) ifkp_rawvideo[n + i] = data;
		ifkp_rawrgb++;
		if (ifkp_rawrgb == 3) {
			ifkp_rawrgb = 0;
			ifkp_rawcol++;
			if (ifkp_rawcol == W) {
				ifkp_rawcol = 0;
				ifkp_rawrow++;
			}
		}
	} else
		for (int i = 0; i < ifkp::IMAGEspp; i++)
			ifkp_rawvideo[RAWSTART + ifkp::IMAGEspp*ifkp_numpixels + i] = data;
	ifkp_numpixels++;
	if (ifkp_numpixels >= (RAWSIZE - RAWSTART - ifkp::IMAGEspp))
		ifkp_numpixels = RAWSIZE - RAWSTART - ifkp::IMAGEspp;
}

void cb_btnifkpRxReset(Fl_Widget *, void *)
{
	progStatus.ifkp_rx_abort = true;
}

void cb_btnifkpRxSave(Fl_Widget *, void *)
{
	ifkppicRx->save_png(PicsDir.c_str());
}

void cb_btnifkpRxClose(Fl_Widget *, void *)
{
	ifkppicRxWin->hide();
	progStatus.ifkp_rx_abort = true;
}

void ifkp_save_raw_video()
{
	string fname = "YYYYMMDDHHMMSSz";

	time_t time_sec = time(0);
	struct tm ztime;
	(void)gmtime_r(&time_sec, &ztime);
	char sztime[fname.length()+1];

	strftime(sztime, sizeof(sztime), "%Y%m%d%H%M%Sz", &ztime);

	fname.assign(PicsDir).append("IFKP").append(sztime).append(".raw");

	FILE *raw = fl_fopen(fname.c_str(), "wb");
	fwrite(&ifkp_image_type, 1, 1, raw);
	fwrite(ifkp_rawvideo, 1, RAWSIZE, raw);
	fclose(raw);
}

void ifkp_load_raw_video()
{
// abort & close any Rx video processing
	int image_type = 0;
	string image_types = "TSLFVPpMm";

	if (!ifkppicRxWin)
		ifkp_createRxViewer();
	else
		ifkppicRxWin->hide();

	const char *p = FSEL::select(
			_("Load raw image file"), "Image\t*.raw\n", PicsDir.c_str());

	if (!p || !*p) return;

	FILE *raw = fl_fopen(p, "rb");
	int numread = fread(&image_type, 1, 1, raw);
	if (numread != 1) {
		fclose(raw);
		return;
	}

	if (image_types.find(ifkp_image_type) != string::npos) {

		ifkp_showRxViewer(image_type);

		numread = fread(ifkp_rawvideo, 1, RAWSIZE, raw);

		if (numread == RAWSIZE) {
			ifkpcnt_phase->activate();
			ifkpcnt_slant->activate();
			btnifkpRxSave->activate();

			ifkp_correct_video();
			ifkppicRxWin->redraw();
		}
	}

	fclose(raw);
}

void cb_ifkp_cnt_phase(Fl_Widget *, void *data)
{
	ifkp_pixelptr = ifkpcnt_phase->value();
	if (ifkp_pixelptr >= RAWSTART/ifkp::IMAGEspp) {
		ifkp_pixelptr = RAWSTART/ifkp::IMAGEspp - 1;
		ifkpcnt_phase->value(ifkp_pixelptr);
	}
	if (ifkp_pixelptr < -RAWSTART/ifkp::IMAGEspp) {
		ifkp_pixelptr = -RAWSTART/ifkp::IMAGEspp;
		ifkpcnt_phase->value(ifkp_pixelptr);
	}
	ifkp_correct_video();
}

void cb_ifkp_cnt_slant(Fl_Widget *, void *)
{
	ifkp_correct_video();
}

void ifkp_disableshift()
{
	if (!ifkppicRxWin) return;
	ifkpcnt_phase->deactivate();
	ifkpcnt_slant->deactivate();
	btnifkpRxSave->deactivate();
	ifkppicRxWin->redraw();
}

void ifkp_enableshift()
{
	if (!ifkppicRxWin) return;
	ifkpcnt_phase->activate();
	ifkpcnt_slant->activate();
	btnifkpRxSave->activate();

	ifkp_save_raw_video();

	ifkppicRxWin->redraw();
}

void ifkp_createRxViewer()
{

	ifkppicRxWin = new Fl_Double_Window(324, 274, _("IFKP Rx Image"));
	ifkppicRxWin->xclass(PACKAGE_NAME);
	ifkppicRxWin->begin();

	ifkppicRx = new picture(2, 2, 320, 240);
	ifkppicRx->noslant();

	Fl_Group *buttons = new Fl_Group(0, ifkppicRxWin->h() - 26, ifkppicRxWin->w(), 26, "");
	buttons->box(FL_FLAT_BOX);

	btnifkpRxReset = new Fl_Button(2, ifkppicRxWin->h() - 26, 40, 24, "Reset");
	btnifkpRxReset->callback(cb_btnifkpRxReset, 0);

	ifkpcnt_phase = new Fl_Counter(46, ifkppicRxWin->h() - 24, 80, 20, "");
	ifkpcnt_phase->step(1);
	ifkpcnt_phase->lstep(10);
	ifkpcnt_phase->minimum(-RAWSTART + 1);
	ifkpcnt_phase->maximum(RAWSTART - 1);
	ifkpcnt_phase->value(0);
	ifkpcnt_phase->callback(cb_ifkp_cnt_phase, 0);
	ifkpcnt_phase->tooltip(_("Phase correction"));

	ifkpcnt_slant = new Fl_Counter(140, ifkppicRxWin->h() - 24, 80, 20, "");
	ifkpcnt_slant->step(1);
	ifkpcnt_slant->lstep(10);
	ifkpcnt_slant->minimum(-200);
	ifkpcnt_slant->maximum(200);
	ifkpcnt_slant->value(0);
	ifkpcnt_slant->callback(cb_ifkp_cnt_slant, 0);
	ifkpcnt_slant->tooltip(_("Slant correction"));

	btnifkpRxSave = new Fl_Button(226, ifkppicRxWin->h() - 26, 45, 24, _("Save"));
	btnifkpRxSave->callback(cb_btnifkpRxSave, 0);

	btnifkpRxClose = new Fl_Button(273, ifkppicRxWin->h() - 26, 45, 24, _("Close"));
	btnifkpRxClose->callback(cb_btnifkpRxClose, 0);
	buttons->end();

	ifkppicRxWin->end();
	ifkppicRxWin->resizable(ifkppicRx);

	ifkp_numpixels = 0;
}

void ifkp_showRxViewer(char itype)
{
	int W = 320;
	int H = 240;
	switch (itype) {
		case 'L' : W = 320; H = 240; break;
		case 'S' : W = 160; H = 120; break;
		case 'F' : W = 640; H = 480; break;
		case 'V' : W = 640; H = 480; break;
		case 'P' : W = 240; H = 300; break;
		case 'p' : W = 240; H = 300; break;
		case 'M' : W = 120; H = 150; break;
		case 'm' : W = 120; H = 150; break;
		case 'T' : W = 59; H = 74; break;
	}

	if (!ifkppicRxWin) ifkp_createRxViewer();
	int winW, winH;
	int ifkppicX, ifkppicY;
	winW = W < 320 ? 324 : W + 4;
	winH = H < 240 ? 274 : H + 34;
	ifkppicX = (winW - W) / 2;
	ifkppicY = (winH - 30 - H) / 2;
	ifkppicRxWin->size(winW, winH);
	ifkppicRx->resize(ifkppicX, ifkppicY, W, H);
	ifkppicRxWin->init_sizes();

	ifkppicRx->clear();
	ifkppicRxWin->show();
	ifkp_disableshift();

	if (ifkp_rawvideo == 0) ifkp_rawvideo = new unsigned char [RAWSIZE + 1];
	memset(ifkp_rawvideo, 0, RAWSIZE);
	ifkp_numpixels = 0;
	ifkp_pixelptr = 0;
	ifkp_rawrow = ifkp_rawrgb = ifkp_rawcol = 0;
	ifkp_image_type = itype;
}

void ifkp_clear_rximage()
{
	ifkppicRx->clear();
	ifkp_disableshift();
	translate = 0;
	enabled = false;
	ifkp_numpixels = 0;
	ifkp_pixelptr = 0;
	ifkpcnt_phase->value(0);
	ifkpcnt_slant->value(0);
	ifkp_rawrow = ifkp_rawrgb = ifkp_rawcol = 0;
}

//------------------------------------------------------------------------------
// image transmit functions
//------------------------------------------------------------------------------

int ifkp_load_image(const char *n) {

	int D = 0;
	unsigned char *img_data;
	int W = 640;
	int H = 480;

	switch (selifkppicSize->value()) {
		case 0 : W = 59; H = 74; break;
		case 1 : W = 160; H = 120; break;
		case 2 : W = 320; H = 240; break;
		case 3 : W = 640; H = 480; break;
		case 4 : W = 640; H = 480; break;
		case 5 : W = 240; H = 300; break;
		case 6 : W = 240; H = 300; break;
		case 7 : W = 120; H = 150; break;
		case 8 : W = 120; H = 150; break;
	}

	if (ifkpTxImg) {
		ifkpTxImg->release();
		ifkpTxImg = 0;
	}
	ifkpTxImg = Fl_Shared_Image::get(n, W, H);

	if (!ifkpTxImg)
		return 0;

	if (ifkpTxImg->count() > 1) {
		ifkpTxImg->release();
		ifkpTxImg = 0;
		return 0;
	}

	ifkppicTx->hide();
	ifkppicTx->clear();

	img_data = (unsigned char *)ifkpTxImg->data()[0];

	D = ifkpTxImg->d();

	if (ifkpxmtimg) delete [] ifkpxmtimg;

	ifkpxmtimg = new unsigned char [W * H * 3];
	if (D == 3)
		memcpy(ifkpxmtimg, img_data, W*H*3);
	else if (D == 4) {
		int i, j, k;
		for (i = 0; i < W*H; i++) {
			j = i*3; k = i*4;
			ifkpxmtimg[j] = img_data[k];
			ifkpxmtimg[j+1] = img_data[k+1];
			ifkpxmtimg[j+2] = img_data[k+2];
		}
	} else if (D == 1) {
		int i, j;
		for (i = 0; i < W*H; i++) {
			j = i * 3;
			ifkpxmtimg[j] = ifkpxmtimg[j+1] = ifkpxmtimg[j+2] = img_data[i];
		}
	} else
		return 0;

//	ifkp_showTxViewer(W, H);
	char* label = strdup(n);
	ifkppicTxWin->copy_label(basename(label));
	free(label);
// load the ifkppicture widget with the rgb image

	ifkppicTx->show();
	ifkppicTxWin->redraw();
	ifkppicTx->video(ifkpxmtimg, W * H * 3);

	btnifkppicTransmit->activate();

	return 1;
}

void ifkp_updateTxPic(unsigned char data, int pos)
{
	if (!ifkppicTxWin->shown()) ifkppicTx->show();
	ifkppicTx->pixel(data, pos);
}

void cb_ifkppicTxLoad(Fl_Widget *, void *)
{
	const char *fn =
		FSEL::select(_("Load image file"), "Image\t*.{png,,gif,jpg,jpeg}\n", PicsDir.c_str());
	if (!fn) return;
	if (!*fn) return;
	ifkp_load_image(fn);
}

void ifkp_clear_tximage()
{
	ifkppicTx->clear();
}

void cb_ifkppicTxClose( Fl_Widget *w, void *)
{
	ifkppicTxWin->hide();
}

int ifkppic_TxGetPixel(int pos, int color)
{
	return ifkpxmtimg[3*pos + color]; // color = {RED, GREEN, BLUE}
}

string ifkp_image_header;

void cb_ifkppicTransmit( Fl_Widget *w, void *)
{
	string picmode = " pic%";
	switch (selifkppicSize->value()) {
		case 0: picmode += 'T'; break;
		case 1: picmode += 'S'; break;
		case 2: picmode += 'L'; break;
		case 3: picmode += 'F'; break;
		case 4: picmode += 'V'; break;
		case 5: picmode += 'P'; break;
		case 6: picmode += 'p'; break;
		case 7: picmode += 'M'; break;
		case 8: picmode += 'm'; break;
	}
	ifkp_image_header = picmode;
	active_modem->ifkp_send_image();
}

void cb_ifkppicTxSendAbort( Fl_Widget *w, void *)
{
}


void cb_selifkppicSize( Fl_Widget *w, void *)
{
	switch (selifkppicSize->value()) {
		case 0 : ifkp_showTxViewer('T'); break;
		case 1 : ifkp_showTxViewer('S'); break;
		case 2 : ifkp_showTxViewer('L'); break;
		case 3 : ifkp_showTxViewer('F'); break;
		case 4 : ifkp_showTxViewer('V'); break;
		case 5 : ifkp_showTxViewer('P'); break;
		case 6 : ifkp_showTxViewer('p'); break;
		case 7 : ifkp_showTxViewer('M'); break;
		case 8 : ifkp_showTxViewer('m'); break;
	}
}

void ifkp_createTxViewer()
{

	ifkppicTxWin = new Fl_Double_Window(324, 270, _("IFKP Send image"));
	ifkppicTxWin->xclass(PACKAGE_NAME);
	ifkppicTxWin->begin();

	ifkppicTx = new picture (2, 2, 320, 240);
	ifkppicTx->noslant();
	ifkppicTx->hide();

	selifkppicSize = new Fl_Choice(5, 244, 110, 24);
	selifkppicSize->add("59 x 74 clr");	// case 0
	selifkppicSize->add("160x120 clr");	// case 1
	selifkppicSize->add("320x240 clr");	// case 2
	selifkppicSize->add("640x480 gry");	// case 3
	selifkppicSize->add("640x480 clr");	// case 4
	selifkppicSize->add("240x300 clr");	// case 5
	selifkppicSize->add("240x300 gry");	// case 6
	selifkppicSize->add("120x150 clr");	// case 7
	selifkppicSize->add("120x150 gry");	// case 8
	selifkppicSize->value(0);
	selifkppicSize->callback(cb_selifkppicSize, 0);

	btnifkppicTxLoad = new Fl_Button(120, 244, 60, 24, _("Load"));
	btnifkppicTxLoad->callback(cb_ifkppicTxLoad, 0);

	btnifkppicTransmit = new Fl_Button(ifkppicTxWin->w() - 130, 244, 60, 24, "Xmt");
	btnifkppicTransmit->callback(cb_ifkppicTransmit, 0);

	btnifkppicTxSendAbort = new Fl_Button(ifkppicTxWin->w() - 130, 244, 60, 24, "Abort Xmt");
	btnifkppicTxSendAbort->callback(cb_ifkppicTxSendAbort, 0);

	btnifkppicTxClose = new Fl_Button(ifkppicTxWin->w() - 65, 244, 60, 24, _("Close"));
	btnifkppicTxClose->callback(cb_ifkppicTxClose, 0);

	btnifkppicTxSendAbort->hide();
	btnifkppicTransmit->deactivate();

	ifkppicTxWin->end();

}


void ifkp_load_scaled_image(std::string fname)
{

	if (!ifkppicTxWin) ifkp_createTxViewer();

	int D = 0;
	unsigned char *img_data;
	int W = 160;
	int H = 120;
	int winW = 644;
	int winH = 512;
	int ifkppicX = 0;
	int ifkppicY = 0;
	string picmode = "pic% \n";

	if (ifkpTxImg) {
		ifkpTxImg->release();
		ifkpTxImg = 0;
	}

	ifkpTxImg = Fl_Shared_Image::get(fname.c_str());
	if (!ifkpTxImg)
		return;

	int iW = ifkpTxImg->w();
	int iH = ifkpTxImg->h();
	int aspect = 0;

	if (iW > iH ) {
		if (iW >= 640) {
			W = 640; H = 480;
			winW = 644; winH = 484;
			aspect = 4;
			picmode[4] = 'V';
		}
		else if (iW >= 320) {
			W = 320; H = 240;
			winW = 324; winH = 244;
			aspect = 2;
			picmode[4] = 'L';
		}
		else {
			W = 160; H = 120;
			winW = 164; winH = 124;
			aspect = 1;
			picmode[4] = 'S';
		}
	} else {
		if (iH >= 300) {
			W = 240; H = 300;
			winW = 244; winH = 304;
			aspect = 5;
			picmode[4] = 'P';
		}
		else if (iH >= 150) {
			W = 120; H = 150;
			winW = 124; winH = 154;
			aspect = 7;
			picmode[4] = 'M';
		}
		else {
			W = 59; H = 74;
			winW = 67; winH = 82;
			aspect = 0;
			picmode[4] = 'T';
		}
	}

	{
		Fl_Image *temp;
		selifkppicSize->value(aspect);
		temp = ifkpTxImg->copy(W, H);
		ifkpTxImg->release();
		ifkpTxImg = (Fl_Shared_Image *)temp;
	}

	if (ifkpTxImg->count() > 1) {
		ifkpTxImg->release();
		ifkpTxImg = 0;
		return;
	}

	ifkppicTx->hide();
	ifkppicTx->clear();

	img_data = (unsigned char *)ifkpTxImg->data()[0];

	D = ifkpTxImg->d();

	if (ifkpxmtimg) delete [] ifkpxmtimg;

	ifkpxmtimg = new unsigned char [W * H * 3];
	if (D == 3)
		memcpy(ifkpxmtimg, img_data, W*H*3);
	else if (D == 4) {
		int i, j, k;
		for (i = 0; i < W*H; i++) {
			j = i*3; k = i*4;
			ifkpxmtimg[j] = img_data[k];
			ifkpxmtimg[j+1] = img_data[k+1];
			ifkpxmtimg[j+2] = img_data[k+2];
		}
	} else if (D == 1) {
		int i, j;
		for (i = 0; i < W*H; i++) {
			j = i * 3;
			ifkpxmtimg[j] = ifkpxmtimg[j+1] = ifkpxmtimg[j+2] = img_data[i];
		}
	} else
		return;

	char* label = strdup(fname.c_str());
	ifkppicTxWin->copy_label(basename(label));
	free(label);

// load the ifkppicture widget with the rgb image

	ifkppicTxWin->size(winW, winH);
	ifkppicX = (winW - W) / 2;
	ifkppicY = (winH - H) / 2;
	ifkppicTx->resize(ifkppicX, ifkppicY, W, H);

	selifkppicSize->hide();
	btnifkppicTransmit->hide();
	btnifkppicTxLoad->hide();
	btnifkppicTxClose->hide();
	btnifkppicTxSendAbort->hide();

	ifkppicTx->video(ifkpxmtimg, W * H * 3);
	ifkppicTx->show();

	ifkppicTxWin->show();

	active_modem->ifkp_send_image(picmode);

	return;
}

void ifkp_showTxViewer(char c)
{
	if (!ifkppicTxWin) ifkp_createTxViewer();

	int winW = 644, winH = 512, W = 480, H = 320;
	int ifkppicX, ifkppicY;

	ifkppicTx->clear();

	switch (c) {
		case 'T' :
			W = 59; H = 74; winW = 324; winH = 184;
			selifkppicSize->value(0);
			break;
		case 'S' :
		case 's' :
			W = 160; H = 120; winW = 324; winH = 154;
			selifkppicSize->value(1);
			break;
		case 'L' :
		case 'l' :
			W = 320; H = 240; winW = 324; winH = 274;
			selifkppicSize->value(2);
			break;
		case 'F' :
			W = 640; H = 480; winW = 644; winH = 514;
			selifkppicSize->value(3);
			break;
		case 'V' :
			W = 640; H = 480; winW = 644; winH = 514;
			selifkppicSize->value(4);
			break;
		case 'P' :
			W = 240; H = 300; winW = 324; winH = 334;
			selifkppicSize->value(5);
			break;
		case 'p' :
			W = 240; H = 300; winW = 324; winH = 334;
			selifkppicSize->value(6);
			break;
		case 'M' :
			W = 120; H = 150; winW = 324; winH = 184;
			selifkppicSize->value(7);
			break;
		case 'm' :
			W = 120; H = 150; winW = 324; winH = 184;
			selifkppicSize->value(8);
			break;
	}

	ifkppicTxWin->size(winW, winH);
	ifkppicX = (winW - W) / 2;
	ifkppicY = (winH - 26 - H) / 2;
	ifkppicTx->resize(ifkppicX, ifkppicY, W, H);

	selifkppicSize->resize(5, winH - 26, 110, 24);

	btnifkppicTxLoad->resize(120, winH - 26, 60, 24);

	btnifkppicTransmit->resize(winW - 130, winH - 26, 60, 24);
	btnifkppicTxSendAbort->resize(winW - 130, winH - 26, 60, 24);

	btnifkppicTxClose->resize(winW -65, winH - 26, 60, 24);

	selifkppicSize->show();
	btnifkppicTransmit->show();
	btnifkppicTxLoad->show();
	btnifkppicTxClose->show();
	btnifkppicTxSendAbort->hide();

	ifkppicTxWin->show();

}

void ifkp_deleteTxViewer()
{
	if (ifkppicTxWin) ifkppicTxWin->hide();
	if (ifkppicTx) {
		delete ifkppicTx;
		ifkppicTx = 0;
	}
	delete [] ifkpxmtimg;
	ifkpxmtimg = 0;
	delete [] ifkpxmtpicbuff;
	ifkpxmtpicbuff = 0;
	if (ifkppicTxWin) delete ifkppicTxWin;
	ifkppicTxWin = 0;
}

void ifkp_deleteRxViewer()
{
	if (ifkppicRxWin) ifkppicRxWin->hide();
	if (ifkppicRx) {
		delete ifkppicRx;
		ifkppicRx = 0;
	}
	if (ifkppicRxWin) {
		delete ifkppicRxWin;
		ifkppicRxWin = 0;
	}
}

int ifkp_print_time_left(float time_sec, char *str, size_t len,
			  const char *prefix, const char *suffix)
{
	int time_min = (int)(time_sec / 60);
	time_sec -= time_min * 60;

	if (time_min)
		return snprintf(str, len, "%s %02dm %2.1fs%s",
				prefix, time_min, time_sec, suffix);
	else
		return snprintf(str, len, "%s %2.1fs%s", prefix, time_sec, suffix);
}

// -----------------------------------------------------------------------------
// avatar send/recv
// -----------------------------------------------------------------------------

static Fl_Shared_Image	*shared_avatar_img = (Fl_Shared_Image *)0;
static unsigned char *avatar_img = (unsigned char *)0;
static Fl_Shared_Image	*my_avatar_img = (Fl_Shared_Image *)0;
static int avatar_phase_correction = 0;
static unsigned char avatar[59 * 74 * 3];

void ifkp_clear_avatar()
{
	ifkp_avatar->clear();
	avatar_phase_correction = 0;
	ifkp_numpixels = 0;
	ifkp_rawrow = ifkp_rawrgb = ifkp_rawcol = 0;
	ifkp_avatar->video(tux_img, sizeof(tux_img));
}


// W always 59, H always 74
int ifkp_load_avatar(std::string image_fname, int W, int H)
{
	W = 59; H = 74;

	if (image_fname.empty()) {
		ifkp_clear_avatar();
		return 1;
	}

	int D = 0;
	unsigned char *img_data;

	if (shared_avatar_img) {
		shared_avatar_img->release();
		shared_avatar_img = 0;
	}

	for (size_t n = 0; n < image_fname.length(); n++)
		image_fname[n] = tolower(image_fname[n]);
	std::string fname = AvatarDir;
	fname.append(image_fname).append(".png");

	FILE *temp = fl_fopen(fname.c_str(), "rb");
	if (temp) {
		fseek(temp, 0L, SEEK_SET);
		fclose(temp);
	} else {
		ifkp_avatar->video(tux_img, 59 * 74 * 3);
		return 1;
	}

	shared_avatar_img = Fl_Shared_Image::get(fname.c_str(), W, H);

// force image to be retrieved from hard drive vice shared image memory
	shared_avatar_img->reload();

	if (!shared_avatar_img) {
		ifkp_avatar->video(tux_img, 59 * 74 * 3);
		return 1;
	}

	if (shared_avatar_img->count() > 1) {
		shared_avatar_img->release();
		shared_avatar_img = 0;
		ifkp_avatar->video(tux_img, 59 * 74 * 3);
		return 0;
	}

	img_data = (unsigned char *)shared_avatar_img->data()[0];

	D = shared_avatar_img->d();

	if (avatar_img) delete [] avatar_img;

	avatar_img = new unsigned char [W * H * 3];
	if (D == 3)
		memcpy(avatar_img, img_data, W*H*3);
	else if (D == 4) {
		int i, j, k;
		for (i = 0; i < W*H; i++) {
			j = i*3; k = i*4;
			avatar_img[j] = img_data[k];
			avatar_img[j+1] = img_data[k+1];
			avatar_img[j+2] = img_data[k+2];
		}
	} else if (D == 1) {
		int i, j;
		for (i = 0; i < W*H; i++) {
			j = i * 3;
			avatar_img[j] = avatar_img[j+1] = avatar_img[j+2] = img_data[i];
		}
	} else {
		ifkp_avatar->video(tux_img, W * H * 3);
		return 0;
	}
	ifkp_avatar->video(avatar_img, W * H * 3);

	shared_avatar_img->release();
	shared_avatar_img = 0;

	return 1;
}

void correct_avatar()
{
	int W = 59;
	int H = 74;
	int index, rowptr, colptr;
	unsigned char vid[W * H * 3];

	if (avatar_phase_correction >= RAWSTART/ifkp::IMAGEspp) {
		avatar_phase_correction = RAWSTART/ifkp::IMAGEspp - 1;
	}
	if (avatar_phase_correction < -RAWSTART/ifkp::IMAGEspp) {
		avatar_phase_correction = -RAWSTART/ifkp::IMAGEspp;
	}

	for (int row = 0; row < H; row++) {
		rowptr = W * 3 * row * ifkp::IMAGEspp;
		for (int col = 0; col < W; col++) {
			colptr = ifkp::IMAGEspp*col;
			for (int rgb = 0; rgb < 3; rgb++) {
				index = rowptr + colptr + W*rgb*ifkp::IMAGEspp;
				index += RAWSTART - ifkp::IMAGEspp * avatar_phase_correction;
				if (index < 2) index = 2;
				if (index > RAWSIZE - 2) index = RAWSIZE - 2;
				vid[rgb + 3 * (col + row * W)] = ifkp_rawvideo[index];
			}
		}
	}
	ifkp_avatar->video(vid, W*H*3);
}

void ifkp_update_avatar(unsigned char data, int pos)
{
	if (ifkp_rawvideo == 0) {
		ifkp_rawvideo = new unsigned char [RAWSIZE + 1];
		memset(ifkp_rawvideo, 0, RAWSIZE);
	}

	ifkp_avatar->pixel(data, pos);
	for (int i = 0; i < ifkp::IMAGEspp; i++)
		ifkp_rawvideo[RAWSTART + ifkp::IMAGEspp*ifkp_numpixels + i] = data;

	ifkp_numpixels++;

	if (ifkp_numpixels >= (RAWSIZE - RAWSTART - ifkp::IMAGEspp))
		ifkp_numpixels = RAWSIZE - RAWSTART - ifkp::IMAGEspp;

}

int ifkp_get_avatar_pixel(int pos, int color)
{
// color = {RED, GREEN, BLUE}
	return (int)avatar[3*pos + color];

}

// ADD CALLBACK HANDLING OF PHASE CORRECTIONS

void cb_ifkp_send_avatar( Fl_Widget *w, void *)
{
	if (Fl::event_button() == FL_RIGHT_MOUSE) {
		if (Fl::get_key	(FL_Shift_L) || Fl::get_key(FL_Shift_R)) {
			if (ifkp_numpixels == 0) return;
			avatar_phase_correction += 5;
			correct_avatar();
			return;
		}
		if (Fl::get_key	(FL_Control_L) || Fl::get_key(FL_Control_R)) {
			if (ifkp_numpixels == 0) return;
			avatar_phase_correction++;
			correct_avatar();
			return;
		}
		std::string mycall = progdefaults.myCall;
		for (size_t n = 0; n < mycall.length(); n++)
			mycall[n] = tolower(mycall[n]);
		std::string fname = AvatarDir;
		fname.append(mycall).append(".png");

		my_avatar_img = Fl_Shared_Image::get(fname.c_str(), 59, 74);
		if (!my_avatar_img) return;
		unsigned char *img_data = (unsigned char *)my_avatar_img->data()[0];
		memset(avatar, 0, sizeof(avatar));
		int D = my_avatar_img->d();

		if (D == 3)
			memcpy(avatar, img_data, 59*74*3);
		else if (D == 4) {
			int i, j, k;
			for (i = 0; i < 59*74; i++) {
				j = i*3; k = i*4;
				avatar[j] = img_data[k];
				avatar[j+1] = img_data[k+1];
				avatar[j+2] = img_data[k+2];
			}
		} else if (D == 1) {
			int i, j;
			for (i = 0; i < 59*74; i++) {
				j = i * 3;
				avatar[j] = avatar[j+1] = avatar[j+2] = img_data[i];
			}
		} else
			return;

		string picmode = "\npic%A\n^r";
		ifkp_tx_text->add(picmode.c_str());
		active_modem->ifkp_send_avatar();
		return;
	}

	if (Fl::event_button() == FL_LEFT_MOUSE) {
		if (Fl::get_key	(FL_Shift_L) || Fl::get_key(FL_Shift_R)) {
			if (ifkp_numpixels == 0) return;
			avatar_phase_correction -= 5;
			correct_avatar();
			return;
		}
		if (Fl::get_key	(FL_Control_L) || Fl::get_key(FL_Control_R)) {
			if (ifkp_numpixels == 0) return;
			avatar_phase_correction--;
			correct_avatar();
			return;
		}
		std::string mycall = inpCall->value();
		if (mycall.empty()) return;
		for (size_t n = 0; n < mycall.length(); n++)
			mycall[n] = tolower(mycall[n]);
		std::string fname = AvatarDir;
		fname.append(mycall).append(".png");
		ifkp_avatar->save_png(fname.c_str());
	}
}
