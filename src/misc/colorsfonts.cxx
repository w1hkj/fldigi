// ----------------------------------------------------------------------------
//
//	colorsfonts.cxx
//
// Copyright (C) 2006
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.
//
// fldigi is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// ----------------------------------------------------------------------------

#include <config.h>

#include "colorsfonts.h"

Fl_Double_Window	*dlgColorFont = (Fl_Double_Window *)0;
Fl_Check_Button		*btnUseGroupColors=(Fl_Check_Button *)0;
Fl_Button			*btnGroup1=(Fl_Button *)0;
Fl_Button			*btnGroup2=(Fl_Button *)0;
Fl_Button			*btnGroup3=(Fl_Button *)0;
Fl_Button			*btnFkeyTextColor=(Fl_Button *)0;
Fl_Button			*btnFkeyDefaults=(Fl_Button *)0;
Fl_Multiline_Output	*RxText=(Fl_Multiline_Output *)0;
Fl_Multiline_Output	*TxText=(Fl_Multiline_Output *)0;
Fl_Button			*btnRxColor=(Fl_Button *)0;
Fl_Button			*btnRxFont=(Fl_Button *)0;
Fl_Button			*btnTxColor=(Fl_Button *)0;
Fl_Button			*btnTxFont=(Fl_Button *)0;
Fl_Button			*btnTextDefaults=(Fl_Button *)0;
Fl_Button			*btnNoTextColor=(Fl_Button *)0;
Fl_Button			*btnClrFntOK=(Fl_Button *)0;

void selectColorsFonts() 
{
	if (!dlgColorFont)
		make_colorsfonts();
	dlgColorFont->show();
}

void cb_ColorFontOK() 
{
	dlgColorFont->hide();
}

static void cb_btnUseGroupColors(Fl_Check_Button* o, void*) 
{
	progdefaults.useGroupColors = o->value();
	colorize_macros();
	progdefaults.changed = true;
}

static void cb_btnGroup1(Fl_Button* o, void*) 
{
	uchar r, g, b;
	r = progdefaults.btnGroup1.R;
	g = progdefaults.btnGroup1.G;
	b = progdefaults.btnGroup1.B;
	int res = fl_color_chooser("Group 1", r, g, b);
	if (res) {
		progdefaults.btnGroup1.R = r;
		progdefaults.btnGroup1.G = g;
		progdefaults.btnGroup1.B = b;
		o->color(fl_rgb_color(r,g,b));
		progdefaults.changed = true;
		colorize_macros();
	}
}

static void cb_btnGroup2(Fl_Button* o, void*) 
{
	uchar r, g, b;
	r = progdefaults.btnGroup2.R;
	g = progdefaults.btnGroup2.G;
	b = progdefaults.btnGroup2.B;
	int res = fl_color_chooser("Group 1", r, g, b);
	if (res) {
		progdefaults.btnGroup2.R = r;
		progdefaults.btnGroup2.G = g;
		progdefaults.btnGroup2.B = b;
		o->color(fl_rgb_color(r,g,b));
		progdefaults.changed = true;
		colorize_macros();
	}
}

static void cb_btnGroup3(Fl_Button* o, void*) 
{
	uchar r, g, b;
	r = progdefaults.btnGroup3.R;
	g = progdefaults.btnGroup3.G;
	b = progdefaults.btnGroup3.B;
	int res = fl_color_chooser("Group 1", r, g, b);
	if (res) {
		progdefaults.btnGroup3.R = r;
		progdefaults.btnGroup3.G = g;
		progdefaults.btnGroup3.B = b;
		o->color(fl_rgb_color(r,g,b));
		progdefaults.changed = true;
		colorize_macros();
	}
}

static void cb_btnFkeyTextColor(Fl_Button* o, void*) 
{
	uchar r, g, b;
	r = progdefaults.btnFkeyTextColor.R;
	g = progdefaults.btnFkeyTextColor.G;
	b = progdefaults.btnFkeyTextColor.B;
	int res = fl_color_chooser("Fkey Text", r, g, b);
	if (res) {
		progdefaults.btnFkeyTextColor.R = r;
		progdefaults.btnFkeyTextColor.G = g;
		progdefaults.btnFkeyTextColor.B = b;
		o->color(fl_rgb_color(r,g,b));
		btnGroup1->labelcolor(fl_rgb_color(r,g,b));
		btnGroup2->labelcolor(fl_rgb_color(r,g,b));
		btnGroup3->labelcolor(fl_rgb_color(r,g,b));
		btnGroup1->redraw_label();
		btnGroup2->redraw_label();
		btnGroup3->redraw_label();
		progdefaults.changed = true;
		colorize_macros();
	}
}

static void cb_btnFkeyDefaults(Fl_Button*, void*) 
{
	uchar r, g, b;
	Fl_Color clr;

	r = 80; g = 144; b = 144;
	clr = fl_rgb_color(r,g,b);
	btnGroup1->color(clr);
	progdefaults.btnGroup1.R = r;
	progdefaults.btnGroup1.G = g;
	progdefaults.btnGroup1.B = b;

	r = 144; g = 80; b = 80;
	clr = fl_rgb_color(r,g,b);
	btnGroup2->color(clr);
	progdefaults.btnGroup2.R = r;
	progdefaults.btnGroup2.G = g;
	progdefaults.btnGroup2.B = b;
	
	r = 80; g = 80; b = 144;
	clr = fl_rgb_color(r,g,b);
	btnGroup3->color(clr);
	progdefaults.btnGroup3.R = r;
	progdefaults.btnGroup3.G = g;
	progdefaults.btnGroup3.B = b;
	
	r = 255; g = 255; b = 255;
	clr = fl_rgb_color(r,g,b);
	btnFkeyTextColor->color(clr);
	btnFkeyTextColor->redraw_label();
	progdefaults.btnFkeyTextColor.R = r;
	progdefaults.btnFkeyTextColor.G = g;
	progdefaults.btnFkeyTextColor.B = b;
	
	btnGroup1->labelcolor(clr);
	btnGroup1->redraw_label();

	btnGroup2->labelcolor(clr);
	btnGroup2->redraw_label();

	btnGroup3->labelcolor(clr);
	btnGroup3->redraw_label();
	progdefaults.changed = true;
	colorize_macros();
}

static void cb_btnRxColor(Fl_Button*, void*) 
{
	uchar r, g, b;
	r = progdefaults.RxColor.R;
	g = progdefaults.RxColor.G;
	b = progdefaults.RxColor.B;
	int res = fl_color_chooser("Rx Color", r, g, b);
	if (res) {
		progdefaults.RxColor.R = r;
		progdefaults.RxColor.G = g;
		progdefaults.RxColor.B = b;
		ReceiveText->color(fl_rgb_color(r,g,b));
		ReceiveText->damage();
		RxText->color(fl_rgb_color(r,g,b));
		RxText->redraw();
		progdefaults.changed = true;
		ReceiveText->color(
			fl_rgb_color(
				progdefaults.RxColor.R,
				progdefaults.RxColor.G,
				progdefaults.RxColor.B));
		ReceiveText->redraw();
	}
}

void cbRxFontBrowser(Font_Browser*, void* v) 
{
	Font_Browser *ft= (Font_Browser*)v;
	Fl_Font fnt = ft->fontNumber();
	int size = ft->fontSize();

	RxText->textfont(fnt);
	RxText->textsize(size);
	RxText->redraw();
	
	progdefaults.RxFontnbr = (int)(fnt);
	progdefaults.RxFontsize = size;
	progdefaults.changed = true;
	
	ReceiveText->setFont(fnt);
	ReceiveText->setFontSize(size);
	ReceiveText->redraw();
	
	ft->hide();
}

static void cb_btnRxFont(Fl_Button*, void*) 
{
	static Font_Browser *b = (Font_Browser *)0;
	if (!b) {
		b = new Font_Browser;
		b->fontNumber((Fl_Font)progdefaults.RxFontnbr);
		b->fontSize(progdefaults.RxFontsize);
	}
	b->callback((Fl_Callback*)cbRxFontBrowser, (void*)(b));
	b->show();
}

static void cb_btnTxColor(Fl_Button*, void*) 
{
	uchar r, g, b;
	r = progdefaults.TxColor.R;
	g = progdefaults.TxColor.G;
	b = progdefaults.TxColor.B;
	int res = fl_color_chooser("Rx Color", r, g, b);
	if (res) {
		progdefaults.TxColor.R = r;
		progdefaults.TxColor.G = g;
		progdefaults.TxColor.B = b;
		TransmitText->color(fl_rgb_color(r,g,b));
		TransmitText->damage();
		TxText->color(fl_rgb_color(r,g,b));
		TxText->redraw();
		progdefaults.changed = true;
		TransmitText->color(
			fl_rgb_color(
				progdefaults.TxColor.R,
				progdefaults.TxColor.G,
				progdefaults.TxColor.B));		
		TransmitText->redraw();
	}
}

void cbTxFontBrowser(Font_Browser*, void* v) 
{
	Font_Browser *ft= (Font_Browser*)v;
	Fl_Font fnt = ft->fontNumber();
	int size = ft->fontSize();

	TxText->textfont(fnt);
	TxText->textsize(size);
	TxText->redraw();
	
	progdefaults.TxFontnbr = (int)(fnt);
	progdefaults.TxFontsize = size;
	progdefaults.changed = true;
	
	TransmitText->setFont(fnt);
	TransmitText->setFontSize(size);
	TransmitText->redraw();
	
	ft->hide();
}

static void cb_btnTxFont(Fl_Button*, void*) 
{
	static Font_Browser *b = (Font_Browser *)0;
	if (!b) {
		b = new Font_Browser;
		b->fontNumber((Fl_Font)progdefaults.TxFontnbr);
		b->fontSize(progdefaults.TxFontsize);
	}
	b->callback((Fl_Callback*)cbTxFontBrowser, (void*)(b));
	b->show();
}

static void cb_btnNoTextColor(Fl_Button*, void*) 
{
	uchar r, g, b;
	Fl_Color clr;

	r = 255; g = 255; b = 255;
	clr = fl_rgb_color(r,g,b);
	RxText->color(clr);
	RxText->redraw();
	ReceiveText->color(clr);
	ReceiveText->redraw();

	TxText->color(clr);
	TxText->redraw();
	TransmitText->color(clr);
	TransmitText->redraw();
	
	progdefaults.RxColor.R = r;
	progdefaults.RxColor.G = g;
	progdefaults.RxColor.B = b;
	progdefaults.TxColor.R = r;
	progdefaults.TxColor.G = g;
	progdefaults.TxColor.B = b;

	progdefaults.changed = true;
}

static void cb_btnTextDefaults(Fl_Button*, void*) 
{
	uchar r, g, b;
	Fl_Color clr;

	r = 255; g = 242; b = 190;
	clr = fl_rgb_color(r,g,b);
	RxText->color(clr);
	RxText->redraw();
	ReceiveText->color(clr);
	ReceiveText->redraw();
	progdefaults.RxColor.R = r;
	progdefaults.RxColor.G = g;
	progdefaults.RxColor.B = b;
	
	r = 200; g = 235; b = 255;
	clr = fl_rgb_color(r,g,b);
	TxText->color(clr);
	TxText->redraw();
	TransmitText->color(clr);
	TransmitText->redraw();
	progdefaults.TxColor.R = r;
	progdefaults.TxColor.G = g;
	progdefaults.TxColor.B = b;
	
	progdefaults.changed = true;
}

static void cb_btnClrFntOK(Fl_Button*, void*) 
{
	cb_ColorFontOK();
}

void make_colorsfonts() 
{
	Fl_Double_Window* w;
	{ Fl_Double_Window* o = new Fl_Double_Window(370, 235, "Fldigi - Colors / Fonts");
		w = o;
		{ Fl_Group* o = new Fl_Group(0, 5, 185, 200, "Function Keys");
			o->box(FL_ENGRAVED_FRAME);
			o->align(FL_ALIGN_TOP_LEFT|FL_ALIGN_INSIDE);

			btnUseGroupColors = new Fl_Check_Button(10, 30, 70, 15, "use colored buttons");
			btnUseGroupColors->down_box(FL_DOWN_BOX);
			btnUseGroupColors->callback((Fl_Callback*)cb_btnUseGroupColors);
			btnUseGroupColors->value(progdefaults.useGroupColors);
			
			btnGroup1 = new Fl_Button(90, 55, 75, 20, "Group 1");
			btnGroup1->callback((Fl_Callback*)cb_btnGroup1);
			btnGroup1->color(
				fl_rgb_color(
					progdefaults.btnGroup1.R,
					progdefaults.btnGroup1.G,
					progdefaults.btnGroup1.B));
			btnGroup1->labelcolor(
				fl_rgb_color(
					progdefaults.btnFkeyTextColor.R,
					progdefaults.btnFkeyTextColor.G,
					progdefaults.btnFkeyTextColor.B));		
			btnGroup1->tooltip("Background color for Fkey group");				
			new Fl_Box(15, 56, 75, 20, "Bkgnd");
			
			btnGroup2 = new Fl_Button(90, 84, 75, 20, "Group 2");
			btnGroup2->callback((Fl_Callback*)cb_btnGroup2);
			btnGroup2->color(
				fl_rgb_color(
					progdefaults.btnGroup2.R,
					progdefaults.btnGroup2.G,
					progdefaults.btnGroup2.B));
			btnGroup2->labelcolor(
				fl_rgb_color(
					progdefaults.btnFkeyTextColor.R,
					progdefaults.btnFkeyTextColor.G,
					progdefaults.btnFkeyTextColor.B));			
			btnGroup2->tooltip("Background color for Fkey group");				
			new Fl_Box(15, 85, 75, 20, "Bkgnd");
			
			btnGroup3 = new Fl_Button(90, 114, 75, 20, "Group 3");
			btnGroup3->callback((Fl_Callback*)cb_btnGroup3);
			btnGroup3->color(
				fl_rgb_color(
					progdefaults.btnGroup3.R,
					progdefaults.btnGroup3.G,
					progdefaults.btnGroup3.B));
			btnGroup3->labelcolor(
				fl_rgb_color(
					progdefaults.btnFkeyTextColor.R,
					progdefaults.btnFkeyTextColor.G,
					progdefaults.btnFkeyTextColor.B));			
			btnGroup3->tooltip("Background color for Fkey group");				
			new Fl_Box(15, 115, 75, 20, "Bkgnd");
			
			btnFkeyTextColor = new Fl_Button(90, 145, 75, 20);
			btnFkeyTextColor->callback((Fl_Callback*)cb_btnFkeyTextColor);
			btnFkeyTextColor->color(
				fl_rgb_color(
					progdefaults.btnFkeyTextColor.R,
					progdefaults.btnFkeyTextColor.G,
					progdefaults.btnFkeyTextColor.B));
			new Fl_Box(15, 145, 75, 20, "Label Txt");
			
			btnFkeyDefaults = new Fl_Button(90, 175, 75, 20, "Defaults");
			btnFkeyDefaults->callback((Fl_Callback*)cb_btnFkeyDefaults);
			
			o->end();
		}
		{ Fl_Group* o = new Fl_Group(185, 5, 185, 200, "Text Controls");
			o->box(FL_ENGRAVED_FRAME);
			o->align(FL_ALIGN_TOP_LEFT|FL_ALIGN_INSIDE);
			
			RxText = new Fl_Multiline_Output(195, 30, 165, 37, "");
			RxText->value("Receive Text");
			RxText->color(
				fl_rgb_color(
					progdefaults.RxColor.R,
					progdefaults.RxColor.G,
					progdefaults.RxColor.B));
			RxText->textfont(progdefaults.RxFontnbr);
			RxText->textsize(progdefaults.RxFontsize);
			
			TxText = new Fl_Multiline_Output(195, 103, 165, 37, "");
			TxText->value("Transmit Text");
			TxText->color(
				fl_rgb_color(
					progdefaults.TxColor.R,
					progdefaults.TxColor.G,
					progdefaults.TxColor.B));
			TxText->textfont(progdefaults.TxFontnbr);
			TxText->textsize(progdefaults.TxFontsize);
			
			btnRxColor = new Fl_Button(200, 75, 70, 20, "Rx Bkgnd");
			btnRxColor->callback((Fl_Callback*)cb_btnRxColor);
			
			btnRxFont = new Fl_Button(285, 75, 70, 20, "Rx Font");
			btnRxFont->callback((Fl_Callback*)cb_btnRxFont);
			
			btnTxColor = new Fl_Button(200, 145, 70, 20, "Tx Bkgnd");
			btnTxColor->callback((Fl_Callback*)cb_btnTxColor);
			
			btnTxFont = new Fl_Button(285, 145, 70, 20, "Tx Font");
			btnTxFont->callback((Fl_Callback*)cb_btnTxFont);

			btnNoTextColor  = new Fl_Button(200, 175, 70, 20, "No Color");
			btnNoTextColor->callback((Fl_Callback*)cb_btnNoTextColor);
						
			btnTextDefaults = new Fl_Button(285, 175, 70, 20, "Defaults");
			btnTextDefaults->callback((Fl_Callback*)cb_btnTextDefaults);

			o->end();
		}

		btnClrFntOK = new Fl_Button(295, 209, 72, 20, "OK");
		btnClrFntOK->callback((Fl_Callback*)cb_btnClrFntOK);
	    
    	o->end();
	}
	dlgColorFont = w;
        dlgColorFont->xclass(PACKAGE);
}
