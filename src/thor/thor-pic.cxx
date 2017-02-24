// ----------------------------------------------------------------------------
// thorpic.cxx  --  thor image support functions
//
// Copyright (C) 2015
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.  Adapted from code contained in gthor source code
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
#include "qrunner.h"
#include "timeops.h"

void thor_createTxViewer();
void thor_showRxViewer(char);

Fl_Double_Window	*thorpicRxWin = (Fl_Double_Window *)0;
picture				*thorpicRx = (picture *)0;
Fl_Button			*btnthorRxReset = (Fl_Button *)0;
Fl_Button			*btnthorRxSave = (Fl_Button *)0;
Fl_Button			*btnthorRxClose = (Fl_Button *)0;
Fl_Counter			*thorcnt_phase = (Fl_Counter *)0;
Fl_Counter			*thorcnt_slant = (Fl_Counter *)0;

Fl_Double_Window	*thorpicTxWin = (Fl_Double_Window *)0;
picture				*thorpicTx = (picture *)0;
Fl_Button			*btnthorpicTransmit = (Fl_Button *)0;
Fl_Button			*btnthorpicTxSendAbort = (Fl_Button *)0;
Fl_Button			*btnthorpicTxLoad = (Fl_Button *)0;
Fl_Button			*btnthorpicTxClose = (Fl_Button *)0;
Fl_Choice			*selthorpicSize = (Fl_Choice *)0;

void thor_showRxViewer(char c);
void thor_createRxViewer();

Fl_Shared_Image	*thorTxImg = (Fl_Shared_Image *)0;
unsigned char *thorxmtimg = (unsigned char *)0;
unsigned char *thorxmtpicbuff = (unsigned char *)0;

#define RAWSIZE 640*(480 + 8)*3*thor::IMAGEspp

#define RAWSTART 640*4*3*thor::IMAGEspp

unsigned char *thor_rawvideo = 0;//[RAWSIZE + 1];

int thor_numpixels;
int thor_pixelptr;
int thor_rawcol;
int thor_rawrow;
int thor_rawrgb;
char thor_image_type = 'S';

char thor_txclr_tooltip[24];
char thor_txgry_tooltip[24];

static int translate = 0;
static bool enabled = false;

std::string thor::imageheader;
std::string thor::avatarheader;
int thor::IMAGEspp = THOR_IMAGESPP;

void thor_correct_video()
{
	int W = thorpicRx->w();
	int H = thorpicRx->h();
	int slant = thorcnt_slant->value();
	int vidsize = W * H;
	int index, rowptr, colptr;
	float ratio = (((float)vidsize - (float)slant)/(float)vidsize);
	unsigned char vid[W * H * 3];
	for (int row = 0; row < H; row++) {
		rowptr = W * 3 * row * thor::IMAGEspp;
		for (int col = 0; col < W; col++) {
			colptr = thor::IMAGEspp*col;
			for (int rgb = 0; rgb < 3; rgb++) {
				index = ratio*(rowptr + colptr + thor::IMAGEspp*W*rgb);
				index += RAWSTART - thor::IMAGEspp*thor_pixelptr;
				if (index < 2) index = 2;
				if (index > RAWSIZE - 2) index = RAWSIZE - 2;
				vid[rgb + 3 * (col + row * W)] = thor_rawvideo[index];
			}
		}
	}
	thorpicRx->video(vid, W*H*3);
}

void thor_updateRxPic(unsigned char data, int pos)
{
	if (!thorpicRxWin->shown()) thorpicRx->show();

	thorpicRx->pixel(data, pos);

	int W = thorpicRx->w();
	if (thor_image_type == 'F' || thor_image_type == 'p' || thor_image_type == 'm') {
		int n = RAWSTART + thor::IMAGEspp*(thor_rawcol + W * (thor_rawrgb + 3 * thor_rawrow));
		if (n < RAWSIZE)
			for (int i = 0; i < thor::IMAGEspp; i++) thor_rawvideo[n + i] = data;
		thor_rawrgb++;
		if (thor_rawrgb == 3) {
			thor_rawrgb = 0;
			thor_rawcol++;
			if (thor_rawcol == W) {
				thor_rawcol = 0;
				thor_rawrow++;
			}
		}
	} else
		for (int i = 0; i < thor::IMAGEspp; i++)
			thor_rawvideo[RAWSTART + thor::IMAGEspp*thor_numpixels + i] = data;
	thor_numpixels++;
	if (thor_numpixels >= (RAWSIZE - RAWSTART - thor::IMAGEspp)) 
		thor_numpixels = RAWSIZE - RAWSTART - thor::IMAGEspp;
}

void cb_btnthorRxReset(Fl_Widget *, void *)
{
//	progStatus.thor_rx_abort = true;
}

void thor_save_raw_video()
{
	string fname = "YYYYMMDDHHMMSSz";

	time_t time_sec = time(0);
	struct tm ztime;
	(void)gmtime_r(&time_sec, &ztime);
	char sztime[fname.length()+1];

	strftime(sztime, sizeof(sztime), "%Y%m%d%H%M%Sz", &ztime);

	fname.assign(PicsDir).append("THOR").append(sztime).append(".raw");

	FILE *raw = fl_fopen(fname.c_str(), "wb");
	fwrite(&thor_image_type, 1, 1, raw);
	fwrite(thor_rawvideo, 1, RAWSIZE, raw);
	fclose(raw);
}

void thor_load_raw_video()
{
// abort & close any Rx video processing
	int image_type = 0;
	string image_types = "TSLFVPpMm";

	if (!thorpicRxWin)
		thor_createRxViewer();
	else
		thorpicRxWin->hide();

	const char *p = FSEL::select(
			_("Load raw image file"), "Image\t*.raw\n", PicsDir.c_str());

	if (!p || !*p) return;

	FILE *raw = fl_fopen(p, "rb");
	int numread = fread(&image_type, 1, 1, raw);
	if (numread != 1) {
		fclose(raw);
		return;
	}

	if (image_types.find(thor_image_type) != string::npos) {

		thor_showRxViewer(image_type);

		numread = fread(thor_rawvideo, 1, RAWSIZE, raw);

		if (numread == RAWSIZE) {
			thorcnt_phase->activate();
			thorcnt_slant->activate();
			btnthorRxSave->activate();

			thor_correct_video();
			thorpicRxWin->redraw();
		}
	}

	fclose(raw);
}

void cb_btnthorRxSave(Fl_Widget *, void *)
{
	thorpicRx->save_png(PicsDir.c_str());
}

void cb_btnthorRxClose(Fl_Widget *, void *)
{
	thorpicRxWin->hide();
}

void cb_thor_cnt_phase(Fl_Widget *, void *data)
{
	thor_pixelptr = thorcnt_phase->value();
	if (thor_pixelptr >= RAWSTART/thor::IMAGEspp) {
		thor_pixelptr = RAWSTART/thor::IMAGEspp - 1;
		thorcnt_phase->value(thor_pixelptr);
	}
	if (thor_pixelptr < -RAWSTART/thor::IMAGEspp) {
		thor_pixelptr = -RAWSTART/thor::IMAGEspp;
		thorcnt_phase->value(thor_pixelptr);
	}
	thor_correct_video();
}

void cb_thor_cnt_slant(Fl_Widget *, void *)
{
	thor_correct_video();
}

void thor_disableshift()
{
	if (!thorpicRxWin) return;
	thorcnt_phase->deactivate();
	thorcnt_slant->deactivate();
	btnthorRxSave->deactivate();
	thorpicRxWin->redraw();
}

void thor_enableshift()
{
	if (!thorpicRxWin) return;

	thorcnt_phase->activate();
	thorcnt_slant->activate();
	btnthorRxSave->activate();

	thor_save_raw_video();

	thorpicRxWin->redraw();
}

void thor_createRxViewer()
{

	thorpicRxWin = new Fl_Double_Window(324, 274, _("thor Rx Image"));
	thorpicRxWin->xclass(PACKAGE_NAME);
	thorpicRxWin->begin();

	thorpicRx = new picture(2, 2, 320, 240);
	thorpicRx->noslant();

	Fl_Group *buttons = new Fl_Group(0, thorpicRxWin->h() - 26, thorpicRxWin->w(), 26, "");
	buttons->box(FL_FLAT_BOX);

	btnthorRxReset = new Fl_Button(2, thorpicRxWin->h() - 26, 40, 24, "Reset");
	btnthorRxReset->callback(cb_btnthorRxReset, 0);

	thorcnt_phase = new Fl_Counter(46, thorpicRxWin->h() - 24, 80, 20, "");
	thorcnt_phase->step(1);
	thorcnt_phase->lstep(10);
	thorcnt_phase->minimum(-RAWSTART + 1);
	thorcnt_phase->maximum(RAWSTART - 1);
	thorcnt_phase->value(0);
	thorcnt_phase->callback(cb_thor_cnt_phase, 0);
	thorcnt_phase->tooltip(_("Phase correction"));

	thorcnt_slant = new Fl_Counter(140, thorpicRxWin->h() - 24, 80, 20, "");
	thorcnt_slant->step(1);
	thorcnt_slant->lstep(10);
	thorcnt_slant->minimum(-200);
	thorcnt_slant->maximum(200);
	thorcnt_slant->value(0);
	thorcnt_slant->callback(cb_thor_cnt_slant, 0);
	thorcnt_slant->tooltip(_("Slant correction"));

	btnthorRxSave = new Fl_Button(226, thorpicRxWin->h() - 26, 45, 24, _("Save"));
	btnthorRxSave->callback(cb_btnthorRxSave, 0);

	btnthorRxClose = new Fl_Button(273, thorpicRxWin->h() - 26, 45, 24, _("Close"));
	btnthorRxClose->callback(cb_btnthorRxClose, 0);
	buttons->end();

	thorpicRxWin->end();
	thorpicRxWin->resizable(thorpicRx);

	thor_numpixels = 0;
}

void thor_showRxViewer(char itype)
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

	if (!thorpicRxWin) thor_createRxViewer();
	int winW, winH;
	int thorpicX, thorpicY;
	winW = W < 320 ? 324 : W + 4;
	winH = H < 240 ? 274 : H + 34;
	thorpicX = (winW - W) / 2;
	thorpicY = (winH - 30 - H) / 2;
	thorpicRxWin->size(winW, winH);
	thorpicRx->resize(thorpicX, thorpicY, W, H);
	thorpicRxWin->init_sizes();

	thorpicRx->clear();
	thorpicRxWin->show();
	thor_disableshift();

	if (thor_rawvideo == 0) thor_rawvideo = new unsigned char [RAWSIZE + 1];
	memset(thor_rawvideo, 0, RAWSIZE);
	thor_numpixels = 0;
	thor_pixelptr = 0;
	thor_rawrow = thor_rawrgb = thor_rawcol = 0;
	thor_image_type = itype;
}

void thor_clear_rximage()
{
	thorpicRx->clear();
	thor_disableshift();
	translate = 0;
	enabled = false;
	thor_numpixels = 0;
	thor_pixelptr = 0;
	thorcnt_phase->value(0);
	thorcnt_slant->value(0);
	thor_rawrow = thor_rawrgb = thor_rawcol = 0;
}

//------------------------------------------------------------------------------
// image transmit functions
//------------------------------------------------------------------------------

void thor_load_scaled_image(std::string fname)
{

	if (!thorpicTxWin) thor_createTxViewer();

	int D = 0;
	unsigned char *img_data;
	int W = 160;
	int H = 120;
	int winW = 644;
	int winH = 512;
	int thorpicX = 0;
	int thorpicY = 0;
	string picmode = "pic% \n";

	if (thorTxImg) {
		thorTxImg->release();
		thorTxImg = 0;
	}

	thorTxImg = Fl_Shared_Image::get(fname.c_str());
	if (!thorTxImg)
		return;

	int iW = thorTxImg->w();
	int iH = thorTxImg->h();
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
		selthorpicSize->value(aspect);
		temp = thorTxImg->copy(W, H);
		thorTxImg->release();
		thorTxImg = (Fl_Shared_Image *)temp;
	}

	if (thorTxImg->count() > 1) {
		thorTxImg->release();
		thorTxImg = 0;
		return;
	}

	thorpicTx->hide();
	thorpicTx->clear();

	img_data = (unsigned char *)thorTxImg->data()[0];

	D = thorTxImg->d();

	if (thorxmtimg) delete [] thorxmtimg;

	thorxmtimg = new unsigned char [W * H * 3];
	if (D == 3)
		memcpy(thorxmtimg, img_data, W*H*3);
	else if (D == 4) {
		int i, j, k;
		for (i = 0; i < W*H; i++) {
			j = i*3; k = i*4;
			thorxmtimg[j] = img_data[k];
			thorxmtimg[j+1] = img_data[k+1];
			thorxmtimg[j+2] = img_data[k+2];
		}
	} else if (D == 1) {
		int i, j;
		for (i = 0; i < W*H; i++) {
			j = i * 3;
			thorxmtimg[j] = thorxmtimg[j+1] = thorxmtimg[j+2] = img_data[i];
		}
	} else
		return;

	char* label = strdup(fname.c_str());
	thorpicTxWin->copy_label(basename(label));
	free(label);

// load the thorpicture widget with the rgb image

	thorpicTxWin->size(winW, winH);
	thorpicX = (winW - W) / 2;
	thorpicY = (winH - H) / 2;
	thorpicTx->resize(thorpicX, thorpicY, W, H);

	selthorpicSize->hide();
	btnthorpicTransmit->hide();
	btnthorpicTxLoad->hide();
	btnthorpicTxClose->hide();
	btnthorpicTxSendAbort->hide();

	thorpicTx->video(thorxmtimg, W * H * 3);
	thorpicTx->show();

	thorpicTxWin->show();

	active_modem->thor_send_image(picmode);

	return;
}


int thor_load_image(const char *n) {

	int D = 0;
	unsigned char *img_data;
	int W = 640;
	int H = 480;

	switch (selthorpicSize->value()) {
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

	if (thorTxImg) {
		thorTxImg->release();
		thorTxImg = 0;
	}
	thorTxImg = Fl_Shared_Image::get(n, W, H);

	if (!thorTxImg)
		return 0;

	if (thorTxImg->count() > 1) {
		thorTxImg->release();
		thorTxImg = 0;
		return 0;
	}

	thorpicTx->hide();
	thorpicTx->clear();

	img_data = (unsigned char *)thorTxImg->data()[0];

	D = thorTxImg->d();

	if (thorxmtimg) delete [] thorxmtimg;

	thorxmtimg = new unsigned char [W * H * 3];
	if (D == 3)
		memcpy(thorxmtimg, img_data, W*H*3);
	else if (D == 4) {
		int i, j, k;
		for (i = 0; i < W*H; i++) {
			j = i*3; k = i*4;
			thorxmtimg[j] = img_data[k];
			thorxmtimg[j+1] = img_data[k+1];
			thorxmtimg[j+2] = img_data[k+2];
		}
	} else if (D == 1) {
		int i, j;
		for (i = 0; i < W*H; i++) {
			j = i * 3;
			thorxmtimg[j] = thorxmtimg[j+1] = thorxmtimg[j+2] = img_data[i];
		}
	} else
		return 0;

	char* label = strdup(n);
	thorpicTxWin->copy_label(basename(label));
	free(label);
// load the thorpicture widget with the rgb image

	thorpicTx->show();
	thorpicTxWin->redraw();
	thorpicTx->video(thorxmtimg, W * H * 3);

	btnthorpicTransmit->activate();

	return 1;
}

void thor_updateTxPic(unsigned char data, int pos)
{
	if (!thorpicTxWin->shown()) thorpicTx->show();
	thorpicTx->pixel(data, pos);
}

void cb_thorpicTxLoad(Fl_Widget *, void *)
{
	const char *fn =
		FSEL::select(_("Load image file"), "Image\t*.{png,,gif,jpg,jpeg}\n", PicsDir.c_str());
	if (!fn) return;
	if (!*fn) return;

	thor_load_image(fn);
}

void thor_clear_tximage()
{
	thorpicTx->clear();
}

void cb_thorpicTxClose( Fl_Widget *w, void *)
{
	thorpicTxWin->hide();
}

int thorpic_TxGetPixel(int pos, int color)
{
	return thorxmtimg[3*pos + color]; // color = {RED, GREEN, BLUE}
}

void cb_thorpicTransmit( Fl_Widget *w, void *)
{
	std::string header = "\npic%";
	switch (selthorpicSize->value()) {
		case 0: header += 'T'; break;
		case 1: header += 'S'; break;
		case 2: header += 'L'; break;
		case 3: header += 'F'; break;
		case 4: header += 'V'; break;
		case 5: header += 'P'; break;
		case 6: header += 'p'; break;
		case 7: header += 'M'; break;
		case 8: header += 'm'; break;
	}
	thor::imageheader = header;
	active_modem->thor_send_image();
}

void cb_thorpicTxSendAbort( Fl_Widget *w, void *)
{
}


void cb_selthorpicSize( Fl_Widget *w, void *)
{
	switch (selthorpicSize->value()) {
		case 0 : thor_showTxViewer('T'); break;
		case 1 : thor_showTxViewer('S'); break;
		case 2 : thor_showTxViewer('L'); break;
		case 3 : thor_showTxViewer('F'); break;
		case 4 : thor_showTxViewer('V'); break;
		case 5 : thor_showTxViewer('P'); break;
		case 6 : thor_showTxViewer('p'); break;
		case 7 : thor_showTxViewer('M'); break;
		case 8 : thor_showTxViewer('m'); break;
	}
}

void thor_createTxViewer()
{

	thorpicTxWin = new Fl_Double_Window(324, 270, _("thor Send image"));
	thorpicTxWin->xclass(PACKAGE_NAME);
	thorpicTxWin->begin();

	thorpicTx = new picture (2, 2, 320, 240);
	thorpicTx->noslant();
	thorpicTx->hide();

	selthorpicSize = new Fl_Choice(5, 244, 110, 24);
	selthorpicSize->add("59 x 74 clr");	// case 0
	selthorpicSize->add("160x120 clr");	// case 1
	selthorpicSize->add("320x240 clr");	// case 2
	selthorpicSize->add("640x480 gry");	// case 3
	selthorpicSize->add("640x480 clr");	// case 4
	selthorpicSize->add("240x300 clr");	// case 5
	selthorpicSize->add("240x300 gry");	// case 6
	selthorpicSize->add("120x150 clr");	// case 7
	selthorpicSize->add("120x150 gry");	// case 8
	selthorpicSize->value(0);
	selthorpicSize->callback(cb_selthorpicSize, 0);

	btnthorpicTxLoad = new Fl_Button(120, 244, 60, 24, _("Load"));
	btnthorpicTxLoad->callback(cb_thorpicTxLoad, 0);

	btnthorpicTransmit = new Fl_Button(thorpicTxWin->w() - 130, 244, 60, 24, "Xmt");
	btnthorpicTransmit->callback(cb_thorpicTransmit, 0);

	btnthorpicTxSendAbort = new Fl_Button(thorpicTxWin->w() - 130, 244, 60, 24, "Abort Xmt");
	btnthorpicTxSendAbort->callback(cb_thorpicTxSendAbort, 0);

	btnthorpicTxClose = new Fl_Button(thorpicTxWin->w() - 65, 244, 60, 24, _("Close"));
	btnthorpicTxClose->callback(cb_thorpicTxClose, 0);

	btnthorpicTxSendAbort->hide();
	btnthorpicTransmit->deactivate();

	thorpicTxWin->end();

}

void thor_showTxViewer(char c)
{
	if (!thorpicTxWin) thor_createTxViewer();

	int winW = 644, winH = 512, W = 480, H = 320;
	int thorpicX, thorpicY;

	thorpicTx->clear();

	switch (c) {
		case 'T' :
			W = 59; H = 74; winW = 324; winH = 184;
			selthorpicSize->value(0);
			break;
		case 'S' :
		case 's' :
			W = 160; H = 120; winW = 324; winH = 154;
			selthorpicSize->value(1);
			break;
		case 'L' :
		case 'l' :
			W = 320; H = 240; winW = 324; winH = 274; 
			selthorpicSize->value(2);
			break;
		case 'F' :
			W = 640; H = 480; winW = 644; winH = 514;
			selthorpicSize->value(3);
			break;
		case 'V' :
			W = 640; H = 480; winW = 644; winH = 514;
			selthorpicSize->value(4);
			break;
		case 'P' :
			W = 240; H = 300; winW = 324; winH = 334;
			selthorpicSize->value(5);
			break;
		case 'p' :
			W = 240; H = 300; winW = 324; winH = 334;
			selthorpicSize->value(6);
			break;
		case 'M' :
			W = 120; H = 150; winW = 324; winH = 184;
			selthorpicSize->value(7);
			break;
		case 'm' :
			W = 120; H = 150; winW = 324; winH = 184;
			selthorpicSize->value(8);
			break;
	}

	thorpicTxWin->size(winW, winH);
	thorpicX = (winW - W) / 2;
	thorpicY = (winH - 26 - H) / 2;
	thorpicTx->resize(thorpicX, thorpicY, W, H);

	selthorpicSize->resize(5, winH - 26, 110, 24);

	btnthorpicTxLoad->resize(120, winH - 26, 60, 24);

	btnthorpicTransmit->resize(winW - 130, winH - 26, 60, 24);
	btnthorpicTxSendAbort->resize(winW - 130, winH - 26, 60, 24);

	btnthorpicTxClose->resize(winW -65, winH - 26, 60, 24);

	selthorpicSize->show();
	btnthorpicTransmit->show();
	btnthorpicTxLoad->show();
	btnthorpicTxClose->show();
	btnthorpicTxSendAbort->hide();

	thorpicTxWin->show();

}

void thor_deleteTxViewer()
{
	if (thorpicTxWin) thorpicTxWin->hide();
	if (thorpicTx) {
		delete thorpicTx;
		thorpicTx = 0;
	}
	delete [] thorxmtimg;
	thorxmtimg = 0;
	delete [] thorxmtpicbuff;
	thorxmtpicbuff = 0;
	if (thorpicTxWin) delete thorpicTxWin;
	thorpicTxWin = 0;
}

void thor_deleteRxViewer()
{
	if (thorpicRxWin) thorpicRxWin->hide();
	if (thorpicRx) {
		delete thorpicRx;
		thorpicRx = 0;
	}
	if (thorpicRxWin) {
		delete thorpicRxWin;
		thorpicRxWin = 0;
	}
}

int thor_print_time_left(float time_sec, char *str, size_t len,
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

void thor_clear_avatar()
{
	thor_avatar->clear();
	avatar_phase_correction = 0;
	thor_numpixels = 0;
	thor_rawrow = thor_rawrgb = thor_rawcol = 0;
	thor_avatar->video(tux_img, 59 * 74 * 3);
}


// W always 59, H always 74
int thor_load_avatar(std::string image_fname, int W, int H)
{
	W = 59; H = 74;
	if (image_fname.empty()) {
		thor_clear_avatar();
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
		thor_avatar->video(tux_img, W * H * 3);
		return 1;
	}

	shared_avatar_img = Fl_Shared_Image::get(fname.c_str(), W, H);

// force image to be retrieved from hard drive vice shared image memory
	shared_avatar_img->reload(); 

	if (!shared_avatar_img) {
		thor_avatar->video(tux_img, W * H * 3);
		return 1;
	}

	if (shared_avatar_img->count() > 1) {
		shared_avatar_img->release();
		shared_avatar_img = 0;
		thor_avatar->video(tux_img, W * H * 3);
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
		thor_avatar->video(tux_img, W * H * 3);
		return 0;
	}
	thor_avatar->video(avatar_img, W * H * 3);

	shared_avatar_img->release();
	shared_avatar_img = 0;

	return 1;
}

void thor_correct_avatar()
{
	int W = 59;
	int H = 74;
	int index, rowptr, colptr;
	unsigned char vid[W * H * 3];

	if (avatar_phase_correction >= RAWSTART/thor::IMAGEspp) {
		avatar_phase_correction = RAWSTART/thor::IMAGEspp - 1;
	}
	if (avatar_phase_correction < -RAWSTART/thor::IMAGEspp) {
		avatar_phase_correction = -RAWSTART/thor::IMAGEspp;
	}

	for (int row = 0; row < H; row++) {
		rowptr = W * 3 * row * thor::IMAGEspp;
		for (int col = 0; col < W; col++) {
			colptr = thor::IMAGEspp*col;
			for (int rgb = 0; rgb < 3; rgb++) {
				index = rowptr + colptr + W*rgb*thor::IMAGEspp;
				index += RAWSTART - thor::IMAGEspp * avatar_phase_correction;
				if (index < 2) index = 2;
				if (index > RAWSIZE - 2) index = RAWSIZE - 2;
				vid[rgb + 3 * (col + row * W)] = thor_rawvideo[index];
			}
		}
	}
	thor_avatar->video(vid, W*H*3);
}

void thor_update_avatar(unsigned char data, int pos)
{
	if (thor_rawvideo == 0) {
		thor_rawvideo = new unsigned char [RAWSIZE + 1];
		memset(thor_rawvideo, 0, RAWSIZE);
	}

	thor_avatar->pixel(data, pos);
	for (int i = 0; i < thor::IMAGEspp; i++)
		thor_rawvideo[RAWSTART + thor::IMAGEspp*thor_numpixels + i] = data;

	thor_numpixels++;

	if (thor_numpixels >= (RAWSIZE - RAWSTART - thor::IMAGEspp)) 
		thor_numpixels = RAWSIZE - RAWSTART - thor::IMAGEspp;

}

int thor_get_avatar_pixel(int pos, int color)
{
// color = {RED, GREEN, BLUE}
	return (int)avatar[3*pos + color];

}

// ADD CALLBACK HANDLING OF PHASE CORRECTIONS

void cb_thor_send_avatar( Fl_Widget *w, void *)
{
	if (Fl::event_button() == FL_RIGHT_MOUSE) {
		if (Fl::get_key	(FL_Shift_L) || Fl::get_key(FL_Shift_R)) {
			if (thor_numpixels == 0) return;
			avatar_phase_correction += 5;
			thor_correct_avatar();
			return;
		}
		if (Fl::get_key	(FL_Control_L) || Fl::get_key(FL_Control_R)) {
			if (thor_numpixels == 0) return;
			avatar_phase_correction++;
			thor_correct_avatar();
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

		thor::avatarheader = "\npic%A";
		active_modem->thor_send_avatar();

		return;
	}

	if (Fl::event_button() == FL_LEFT_MOUSE) {
		if (Fl::get_key	(FL_Shift_L) || Fl::get_key(FL_Shift_R)) {
			if (thor_numpixels == 0) return;
			avatar_phase_correction -= 5;
			thor_correct_avatar();
			return;
		}
		if (Fl::get_key	(FL_Control_L) || Fl::get_key(FL_Control_R)) {
			if (thor_numpixels == 0) return;
			avatar_phase_correction--;
			thor_correct_avatar();
			return;
		}
		std::string mycall = inpCall->value();
		if (mycall.empty()) return;
		for (size_t n = 0; n < mycall.length(); n++)
			mycall[n] = tolower(mycall[n]);
		std::string fname = AvatarDir;
		fname.append(mycall).append(".png");
		thor_avatar->save_png(fname.c_str());
	}
}


