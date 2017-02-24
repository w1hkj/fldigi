// ----------------------------------------------------------------------------
// fsqpic.cxx  --  fsq image support functions
//
// Copyright (C) 2015
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.  Adapted from code contained in gfsq source code
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

Fl_Double_Window	*fsqpicRxWin = (Fl_Double_Window *)0;
picture				*fsqpicRx = (picture *)0;
Fl_Button			*btnfsqRxReset = (Fl_Button *)0;
Fl_Button			*btnfsqRxSave = (Fl_Button *)0;
Fl_Button			*btnfsqRxClose = (Fl_Button *)0;
Fl_Counter			*cnt_phase = (Fl_Counter *)0;
Fl_Counter			*cnt_slant = (Fl_Counter *)0;

Fl_Double_Window	*fsqpicTxWin = (Fl_Double_Window *)0;
picture				*fsqpicTx = (picture *)0;
Fl_Button			*btnfsqpicTransmit = (Fl_Button *)0;
Fl_Button			*btnfsqpicTxSendAbort = (Fl_Button *)0;
Fl_Button			*btnfsqpicTxLoad = (Fl_Button *)0;
Fl_Button			*btnfsqpicTxClose = (Fl_Button *)0;
Fl_Choice			*selfsqpicSize = (Fl_Choice *)0;

Fl_Shared_Image	*fsqTxImg = (Fl_Shared_Image *)0;
unsigned char *fsqxmtimg = (unsigned char *)0;
unsigned char *fsqxmtpicbuff = (unsigned char *)0;

#define RAWSIZE 640*(480 + 8)*3*10
#define RAWSTART 640*4*3*10
unsigned char rawvideo[RAWSIZE + 1];
int numpixels;
int pixelptr;
int rawcol;
int rawrow;
int rawrgb;
char image_type = 'S';

int	fsq_txSPP = 8;

char fsq_txclr_tooltip[24];
char fsq_txgry_tooltip[24];

static int translate = 0;
static bool enabled = false;

void correct_video()
{
	int W = fsqpicRx->w();
	int H = fsqpicRx->h();
	int slant = cnt_slant->value();
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
				index += RAWSTART - 10*pixelptr;
				if (index < 2) index = 2;
				if (index > RAWSIZE - 2) index = RAWSIZE - 2;
				vid[2 - rgb + 3 * (col + row * W)] = rawvideo[index];
			}
		}
	}
	fsqpicRx->video(vid, W*H*3);
}

void fsq_updateRxPic(unsigned char data, int pos)
{
	if (!fsqpicRxWin->shown()) fsqpicRx->show();

	fsqpicRx->pixel(data, pos);

	int W = fsqpicRx->w();
	if (image_type == 'F' || image_type == 'p' || image_type == 'm') {
		int n = RAWSTART + 10*(rawcol + W * (rawrgb + 3 * rawrow));
		if (n < RAWSIZE)
			for (int i = 0; i < 10; i++) rawvideo[n + i] = data;
		rawrgb++;
		if (rawrgb == 3) {
			rawrgb = 0;
			rawcol++;
			if (rawcol == W) {
				rawcol = 0;
				rawrow++;
			}
		}
	} else
		for (int i = 0; i < 10; i++) rawvideo[RAWSTART + 10*numpixels + i] = data;
	numpixels++;
	if (numpixels >= (RAWSIZE - RAWSTART - 10)) 
		numpixels = RAWSIZE - RAWSTART - 10;
}

void cb_btnfsqRxReset(Fl_Widget *, void *)
{
	progStatus.fsq_rx_abort = true;
}

void cb_btnfsqRxSave(Fl_Widget *, void *)
{
	fsqpicRx->save_png(PicsDir.c_str());
}

void cb_btnfsqRxClose(Fl_Widget *, void *)
{
	fsqpicRxWin->hide();
	progStatus.fsq_rx_abort = true;
}

void cb_cnt_phase(Fl_Widget *, void *data)
{
	pixelptr = cnt_phase->value();
	if (pixelptr >= RAWSTART/10) {
		pixelptr = RAWSTART/10 - 1;
		cnt_phase->value(pixelptr);
	}
	if (pixelptr < -RAWSTART/10) {
		pixelptr = -RAWSTART/10;
		cnt_phase->value(pixelptr);
	}
	correct_video();
}

void cb_cnt_slant(Fl_Widget *, void *)
{
	correct_video();
}

void fsq_disableshift()
{
	if (!fsqpicRxWin) return;
	cnt_phase->deactivate();
	cnt_slant->deactivate();
	btnfsqRxSave->deactivate();
	fsqpicRxWin->redraw();
}

void fsq_enableshift()
{
	if (!fsqpicRxWin) return;
	cnt_phase->activate();
	cnt_slant->activate();
	btnfsqRxSave->activate();
	fsqpicRxWin->redraw();
}

void fsq_createRxViewer()
{

	fsqpicRxWin = new Fl_Double_Window(324, 274, _("FSQ Rx Image"));
	fsqpicRxWin->xclass(PACKAGE_NAME);
	fsqpicRxWin->begin();

	fsqpicRx = new picture(2, 2, 320, 240);
	fsqpicRx->noslant();

	Fl_Group *buttons = new Fl_Group(0, fsqpicRxWin->h() - 26, fsqpicRxWin->w(), 26, "");
	buttons->box(FL_FLAT_BOX);

	btnfsqRxReset = new Fl_Button(2, fsqpicRxWin->h() - 26, 40, 24, "Reset");
	btnfsqRxReset->callback(cb_btnfsqRxReset, 0);

	cnt_phase = new Fl_Counter(46, fsqpicRxWin->h() - 24, 80, 20, "");
	cnt_phase->step(1);
	cnt_phase->lstep(10);
	cnt_phase->minimum(-RAWSTART + 1);
	cnt_phase->maximum(RAWSTART - 1);
	cnt_phase->value(0);
	cnt_phase->callback(cb_cnt_phase, 0);
	cnt_phase->tooltip(_("Phase correction"));

	cnt_slant = new Fl_Counter(140, fsqpicRxWin->h() - 24, 80, 20, "");
	cnt_slant->step(1);
	cnt_slant->lstep(10);
	cnt_slant->minimum(-200);
	cnt_slant->maximum(200);
	cnt_slant->value(0);
	cnt_slant->callback(cb_cnt_slant, 0);
	cnt_slant->tooltip(_("Slant correction"));

	btnfsqRxSave = new Fl_Button(226, fsqpicRxWin->h() - 26, 45, 24, _("Save"));
	btnfsqRxSave->callback(cb_btnfsqRxSave, 0);

	btnfsqRxClose = new Fl_Button(273, fsqpicRxWin->h() - 26, 45, 24, _("Close"));
	btnfsqRxClose->callback(cb_btnfsqRxClose, 0);
	buttons->end();

	fsqpicRxWin->end();
	fsqpicRxWin->resizable(fsqpicRx);

	numpixels = 0;
}

void fsq_showRxViewer(int W, int H, char itype)
{
	if (!fsqpicRxWin) fsq_createRxViewer();
	int winW, winH;
	int fsqpicX, fsqpicY;
	winW = W < 320 ? 324 : W + 4;
	winH = H < 240 ? 274 : H + 34;
	fsqpicX = (winW - W) / 2;
	fsqpicY = (winH - 30 - H) / 2;
	fsqpicRxWin->size(winW, winH);
	fsqpicRx->resize(fsqpicX, fsqpicY, W, H);
	fsqpicRxWin->init_sizes();

	fsqpicRx->clear();
	fsqpicRxWin->show();
	fsq_disableshift();

	memset(rawvideo, 0, RAWSIZE);
	numpixels = 0;
	pixelptr = 0;
	rawrow = rawrgb = rawcol = 0;
	image_type = itype;
}

void fsq_clear_rximage()
{
	fsqpicRx->clear();
	fsq_disableshift();
	translate = 0;
	enabled = false;
	numpixels = 0;
	pixelptr = 0;
	cnt_phase->value(0);
	cnt_slant->value(0);
	rawrow = rawrgb = rawcol = 0;
}

//------------------------------------------------------------------------------
// image transmit functions
//------------------------------------------------------------------------------

int fsq_load_image(const char *n, int W, int H) {

	int D = 0;
	unsigned char *img_data;

	switch (selfsqpicSize->value()) {
		case 0 : W = 160; H = 120; break;
		case 1 : W = 320; H = 240; break;
		case 2 : W = 640; H = 480; break;
		case 3 : W = 640; H = 480; break;
		case 4 : W = 240; H = 300; break;
		case 5 : W = 240; H = 300; break;
		case 6 : W = 120; H = 150; break;
		case 7 : W = 120; H = 150; break;
	}

	if (fsqTxImg) {
		fsqTxImg->release();
		fsqTxImg = 0;
	}
	fsqTxImg = Fl_Shared_Image::get(n, W, H);

	if (!fsqTxImg)
		return 0;

	if (fsqTxImg->count() > 1) {
		fsqTxImg->release();
		fsqTxImg = 0;
		return 0;
	}

	fsqpicTx->hide();
	fsqpicTx->clear();

	img_data = (unsigned char *)fsqTxImg->data()[0];

	D = fsqTxImg->d();

	if (fsqxmtimg) delete [] fsqxmtimg;

	fsqxmtimg = new unsigned char [W * H * 3];
	if (D == 3)
		memcpy(fsqxmtimg, img_data, W*H*3);
	else if (D == 4) {
		int i, j, k;
		for (i = 0; i < W*H; i++) {
			j = i*3; k = i*4;
			fsqxmtimg[j] = img_data[k];
			fsqxmtimg[j+1] = img_data[k+1];
			fsqxmtimg[j+2] = img_data[k+2];
		}
	} else if (D == 1) {
		int i, j;
		for (i = 0; i < W*H; i++) {
			j = i * 3;
			fsqxmtimg[j] = fsqxmtimg[j+1] = fsqxmtimg[j+2] = img_data[i];
		}
	} else
		return 0;

//	fsq_showTxViewer(W, H);
	char* label = strdup(n);
	fsqpicTxWin->copy_label(basename(label));
	free(label);
// load the fsqpicture widget with the rgb image

	fsqpicTx->show();
	fsqpicTxWin->redraw();
	fsqpicTx->video(fsqxmtimg, W * H * 3);

	btnfsqpicTransmit->activate();

	return 1;
}

void fsq_updateTxPic(unsigned char data, int pos)
{
	if (!fsqpicTxWin->shown()) fsqpicTx->show();
	fsqpicTx->pixel(data, pos);
}

void cb_fsqpicTxLoad(Fl_Widget *, void *)
{
	const char *fn =
		FSEL::select(_("Load image file"), "Image\t*.{png,,gif,jpg,jpeg}\n", PicsDir.c_str());
	if (!fn) return;
	if (!*fn) return;
	fsq_load_image(fn);
}

void fsq_clear_tximage()
{
	fsqpicTx->clear();
}

void cb_fsqpicTxClose( Fl_Widget *w, void *)
{
	fsqpicTxWin->hide();
}

int fsqpic_TxGetPixel(int pos, int color)
{
	return fsqxmtimg[3*pos + color]; // color = {RED, GREEN, BLUE}
}

void cb_fsqpicTransmit( Fl_Widget *w, void *)
{
	std::string image_txt;
	image_txt.assign(fsq_selected_call.c_str());
	switch (selfsqpicSize->value()) {
		case 0: image_txt.append("% S"); break;
		case 1: image_txt.append("% L"); break;
		case 2: image_txt.append("% F"); break;
		case 3: image_txt.append("% V"); break;
		case 4: image_txt.append("% P"); break;
		case 5: image_txt.append("% p"); break;
		case 6: image_txt.append("% M"); break;
		case 7: image_txt.append("% m"); break;
	}
	active_modem->fsq_send_image(image_txt);
}

void cb_fsqpicTxSendAbort( Fl_Widget *w, void *)
{
}

void cb_selfsqpicSize( Fl_Widget *w, void *)
{
	switch (selfsqpicSize->value()) {
		case 0 : fsq_showTxViewer('S'); break;
		case 1 : fsq_showTxViewer('L'); break;
		case 2 : fsq_showTxViewer('F'); break;
		case 3 : fsq_showTxViewer('V'); break;
		case 4 : fsq_showTxViewer('P'); break;
		case 5 : fsq_showTxViewer('p'); break;
		case 6 : fsq_showTxViewer('M'); break;
		case 7 : fsq_showTxViewer('m'); break;
	}
}

void fsq_createTxViewer()
{

	fsqpicTxWin = new Fl_Double_Window(324, 270, _("Send image"));
	fsqpicTxWin->xclass(PACKAGE_NAME);
	fsqpicTxWin->begin();

	fsqpicTx = new picture (2, 2, 320, 240);
	fsqpicTx->noslant();
	fsqpicTx->hide();

	selfsqpicSize = new Fl_Choice(5, 244, 110, 24);
	selfsqpicSize->add("160x120 clr");	// case 0
	selfsqpicSize->add("320x240 clr");	// case 1
	selfsqpicSize->add("640x480 gry");	// case 2
	selfsqpicSize->add("640x480 clr");	// case 3
	selfsqpicSize->add("240x300 clr");	// case 4
	selfsqpicSize->add("240x300 gry");	// case 5
	selfsqpicSize->add("120x150 clr");	// case 6
	selfsqpicSize->add("120x150 gry");	// case 7
	selfsqpicSize->value(1);
	selfsqpicSize->callback(cb_selfsqpicSize, 0);

	btnfsqpicTxLoad = new Fl_Button(120, 244, 60, 24, _("Load"));
	btnfsqpicTxLoad->callback(cb_fsqpicTxLoad, 0);

	btnfsqpicTransmit = new Fl_Button(fsqpicTxWin->w() - 130, 244, 60, 24, "Xmt");
	btnfsqpicTransmit->callback(cb_fsqpicTransmit, 0);

	btnfsqpicTxSendAbort = new Fl_Button(fsqpicTxWin->w() - 130, 244, 60, 24, "Abort Xmt");
	btnfsqpicTxSendAbort->callback(cb_fsqpicTxSendAbort, 0);

	btnfsqpicTxClose = new Fl_Button(fsqpicTxWin->w() - 65, 244, 60, 24, _("Close"));
	btnfsqpicTxClose->callback(cb_fsqpicTxClose, 0);

	btnfsqpicTxSendAbort->hide();
	btnfsqpicTransmit->deactivate();

	fsqpicTxWin->end();

}

void fsq_showTxViewer(char c)
{
	if (!fsqpicTxWin) fsq_createTxViewer();

	int winW = 644, winH = 512, W = 480, H = 320;
	int fsqpicX, fsqpicY;

	fsqpicTx->clear();

	switch (c) {
		case 'S' :
		case 's' :
			W = 160; H = 120; winW = 324; winH = 154;
			selfsqpicSize->value(0);
			break;
		case 'L' :
		case 'l' :
			W = 320; H = 240; winW = 324; winH = 274; 
			selfsqpicSize->value(1);
			break;
		case 'F' :
			W = 640; H = 480; winW = 644; winH = 514;
			selfsqpicSize->value(2);
			break;
		case 'V' :
			W = 640; H = 480; winW = 644; winH = 514;
			selfsqpicSize->value(3);
			break;
		case 'P' :
			W = 240; H = 300; winW = 324; winH = 334;
			selfsqpicSize->value(4);
			break;
		case 'p' :
			W = 240; H = 300; winW = 324; winH = 334;
			selfsqpicSize->value(5);
			break;
		case 'M' :
			W = 120; H = 150; winW = 324; winH = 184;
			selfsqpicSize->value(6);
			break;
		case 'm' :
			W = 120; H = 150; winW = 324; winH = 184;
			selfsqpicSize->value(7);
	}

	fsqpicTxWin->size(winW, winH);
	fsqpicX = (winW - W) / 2;
	fsqpicY = (winH - 26 - H) / 2;
	fsqpicTx->resize(fsqpicX, fsqpicY, W, H);

	selfsqpicSize->resize(5, winH - 26, 110, 24);

	btnfsqpicTxLoad->resize(120, winH - 26, 60, 24);

	btnfsqpicTransmit->resize(winW - 130, winH - 26, 60, 24);
	btnfsqpicTxSendAbort->resize(winW - 130, winH - 26, 60, 24);

	btnfsqpicTxClose->resize(winW -65, winH - 26, 60, 24);

	selfsqpicSize->show();
	btnfsqpicTransmit->show();
	btnfsqpicTxLoad->show();
	btnfsqpicTxClose->show();
	btnfsqpicTxSendAbort->hide();

	fsqpicTxWin->show();

}

void fsq_deleteTxViewer()
{
	fsqpicTxWin->hide();
	if (fsqpicTx) delete fsqpicTx;
	delete [] fsqxmtimg;
	fsqxmtimg = 0;
	delete [] fsqxmtpicbuff;
	fsqxmtpicbuff = 0;
	delete fsqpicTxWin;
	fsqpicTxWin = 0;
}

void fsq_deleteRxViewer()
{
	fsqpicRxWin->hide();
	if (fsqpicRx) {
		delete fsqpicRx;
		fsqpicRx = 0;
	}
	delete fsqpicRxWin;
	fsqpicRxWin = 0;
}

int fsq_print_time_left(float time_sec, char *str, size_t len,
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

