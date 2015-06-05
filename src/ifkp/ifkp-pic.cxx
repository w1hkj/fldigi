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

void ifkp_showTxViewer(char c);

Fl_Shared_Image	*ifkpTxImg = (Fl_Shared_Image *)0;
unsigned char *ifkpxmtimg = (unsigned char *)0;
unsigned char *ifkpxmtpicbuff = (unsigned char *)0;

#define RAWSIZE 640*(480 + 8)*3*10
#define RAWSTART 640*4*3*10
unsigned char ifkp_rawvideo[RAWSIZE + 1];
int ifkp_numpixels;
int ifkp_pixelptr;
int ifkp_rawcol;
int ifkp_rawrow;
int ifkp_rawrgb;
char ifkp_image_type = 'S';

int	ifkp_txSPP = 8;

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
		rowptr = W * 3 * row * 10;
		for (int col = 0; col < W; col++) {
			colptr = 10*col;
			for (int rgb = 0; rgb < 3; rgb++) {
				index = ratio*(rowptr + colptr + 10*W*rgb);
				index += RAWSTART - 10*ifkp_pixelptr;
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
		int n = RAWSTART + 10*(ifkp_rawcol + W * (ifkp_rawrgb + 3 * ifkp_rawrow));
		if (n < RAWSIZE)
			for (int i = 0; i < 10; i++) ifkp_rawvideo[n + i] = data;
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
		for (int i = 0; i < 10; i++) ifkp_rawvideo[RAWSTART + 10*ifkp_numpixels + i] = data;
	ifkp_numpixels++;
	if (ifkp_numpixels >= (RAWSIZE - RAWSTART - 10)) 
		ifkp_numpixels = RAWSIZE - RAWSTART - 10;
}

void cb_btnifkpRxReset(Fl_Widget *, void *)
{
	progStatus.ifkp_rx_abort = true;
}

void cb_btnifkpRxSave(Fl_Widget *, void *)
{
	ifkppicRx->save_png(PicsDir.c_str());
//	FILE *raw = fopen("image.raw", "wb");
//	std::cout << "wrote " << fwrite(ifkp_rawvideo, 1, RAWSIZE, raw) << "\n";
//	fclose(raw);
}

void cb_btnifkpRxClose(Fl_Widget *, void *)
{
	ifkppicRxWin->hide();
	progStatus.ifkp_rx_abort = true;
//	ifkppicRxWin->hide();
//	FILE *raw = fopen("image.raw", "rb");
//	std::cout << "read " << fread(ifkp_rawvideo, 1, RAWSIZE, raw) << "\n";
//	fclose(raw);
//	ifkp_correct_video();
}

void cb_ifkp_cnt_phase(Fl_Widget *, void *data)
{
	ifkp_pixelptr = ifkpcnt_phase->value();
	if (ifkp_pixelptr >= RAWSTART/10) {
		ifkp_pixelptr = RAWSTART/10 - 1;
		ifkpcnt_phase->value(ifkp_pixelptr);
	}
	if (ifkp_pixelptr < -RAWSTART/10) {
		ifkp_pixelptr = -RAWSTART/10;
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
		case 0 : W = 160; H = 120; break;
		case 1 : W = 320; H = 240; break;
		case 2 : W = 640; H = 480; break;
		case 3 : W = 640; H = 480; break;
		case 4 : W = 240; H = 300; break;
		case 5 : W = 240; H = 300; break;
		case 6 : W = 120; H = 150; break;
		case 7 : W = 120; H = 150; break;
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
	if (fn)
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

void cb_ifkppicTransmit( Fl_Widget *w, void *)
{
	string picmode = " pic%";
	switch (selifkppicSize->value()) {
		case 0: picmode += 'S'; break;
		case 1: picmode += 'L'; break;
		case 2: picmode += 'F'; break;
		case 3: picmode += 'V'; break;
		case 4: picmode += 'P'; break;
		case 5: picmode += 'p'; break;
		case 6: picmode += 'M'; break;
		case 7: picmode += 'm'; break;
	}
	picmode.append("^r");
	TransmitText->add(picmode.c_str());
	active_modem->ifkp_send_image();
}

void cb_ifkppicTxSendAbort( Fl_Widget *w, void *)
{
}


void cb_selifkppicSize( Fl_Widget *w, void *)
{
	switch (selifkppicSize->value()) {
		case 0 : ifkp_showTxViewer('S'); break;
		case 1 : ifkp_showTxViewer('L'); break;
		case 2 : ifkp_showTxViewer('F'); break;
		case 3 : ifkp_showTxViewer('V'); break;
		case 4 : ifkp_showTxViewer('P'); break;
		case 5 : ifkp_showTxViewer('p'); break;
		case 6 : ifkp_showTxViewer('M'); break;
		case 7 : ifkp_showTxViewer('m'); break;
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
	selifkppicSize->add("160x120 clr");	// case 0
	selifkppicSize->add("320x240 clr");	// case 1
	selifkppicSize->add("640x480 gry");	// case 2
	selifkppicSize->add("640x480 clr");	// case 3
	selifkppicSize->add("240x300 clr");	// case 4
	selifkppicSize->add("240x300 gry");	// case 5
	selifkppicSize->add("120x150 clr");	// case 6
	selifkppicSize->add("120x150 gry");	// case 7
	selifkppicSize->value(1);
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

void ifkp_showTxViewer(char c)
{
	if (!ifkppicTxWin) ifkp_createTxViewer();

	int winW = 644, winH = 512, W = 480, H = 320;
	int ifkppicX, ifkppicY;

	ifkppicTx->clear();

	switch (c) {
		case 'S' :
		case 's' :
			W = 160; H = 120; winW = 324; winH = 154;
			selifkppicSize->value(0);
			break;
		case 'L' :
		case 'l' :
			W = 320; H = 240; winW = 324; winH = 274; 
			selifkppicSize->value(1);
			break;
		case 'F' :
			W = 640; H = 480; winW = 644; winH = 514;
			selifkppicSize->value(2);
			break;
		case 'V' :
			W = 640; H = 480; winW = 644; winH = 514;
			selifkppicSize->value(3);
			break;
		case 'P' :
			W = 240; H = 300; winW = 324; winH = 334;
			selifkppicSize->value(4);
			break;
		case 'p' :
			W = 240; H = 300; winW = 324; winH = 334;
			selifkppicSize->value(5);
			break;
		case 'M' :
			W = 120; H = 150; winW = 324; winH = 184;
			selifkppicSize->value(6);
			break;
		case 'm' :
			W = 120; H = 150; winW = 324; winH = 184;
			selifkppicSize->value(7);
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

