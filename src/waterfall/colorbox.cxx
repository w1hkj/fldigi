// ----------------------------------------------------------------------------
// colorbox.cxx
//
// Copyright (C) 2007-2008
//		Dave Freese, W1HKJ
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

#include <string>
#include <FL/Fl_Color_Chooser.H>

#include "gettext.h"
#include "colorbox.h"
#include "confdialog.h"
#include "main.h"
#include "fl_digi.h"
#include "fileselect.h"
#include "debug.h"

void colorbox::draw() {
	int ypos = y() + 2;
	int xpos;
	int ht = h() - 4;
	int wd = w() - 4;
	draw_box();
	for(int i = 0; i < wd; i++){
		xpos = x() + 2 + i;
		int xc = i * 256 / wd;
		if (xc > 255) xc = 255;
		fl_rectf (xpos, ypos, 1, ht, mag[xc].R, mag[xc].G, mag[xc].B);
	}
}

void colorbox::mag_RGBcolors(RGB *rgb) {
	for (int i = 0; i < 256; i++)
		mag[i] = rgb[i];
	redraw();
}

void colorbox::mag_RGBIcolors(RGBI *rgbi) {
	for (int i = 0; i < 256; i++) {
		mag[i].R = rgbi[i].R;
		mag[i].G = rgbi[i].G;
		mag[i].B = rgbi[i].B;
	}
	redraw();
}

void colorbox::palette_to_mag(RGB *pal) {
	for (int n = 0; n < 8; n++) {
		for (int i = 0; i < 32; i++) {
			mag[i + 32*n].R = pal[n].R + (int)(1.0 * i * (pal[n+1].R - pal[n].R) / 32.0);
			mag[i + 32*n].G = pal[n].G + (int)(1.0 * i * (pal[n+1].G - pal[n].G) / 32.0);
			mag[i + 32*n].B = pal[n].B + (int)(1.0 * i * (pal[n+1].B - pal[n].B) / 32.0);
		}
	}
	redraw();
}

void setColorButtons()
{
	for (int i = 0; i < 9; i++) {
		btnColor[i]->color( fl_rgb_color( palette[i].R, palette[i].G, palette[i].B ) );
		btnColor[i]->redraw();
	}
}

void selectColor(int n)
{
	uchar r, g, b;
	r = palette[n].R;
	g = palette[n].G;
	b = palette[n].B;
	fl_color_chooser("Spectrum", r, g, b);

	palette[n].R = r;
	palette[n].G = g;
	palette[n].B = b;

	btnColor[n]->color( fl_rgb_color( palette[n].R, palette[n].G, palette[n].B ) );
	btnColor[n]->redraw();
	wf->setcolors();
	WF_Palette->redraw();
}

static std::string palfilename = "";
static std::string palLabelStr;

void loadPalette()
{
	int r,g,b;
	FILE *clrfile = NULL;
	if (palfilename.size() == 0) {
		palfilename = PalettesDir;
		palfilename.append ("fldigi.pal");
	}
    const char *p = FSEL::select(_("Open palette"), _("Fldigi palette\t*.pal"), palfilename.c_str());
	if (!p) return;
	if (!*p) return;
	if ((clrfile = fl_fopen(p, "r")) != NULL) {
		for (int i = 0; i < 9; i++) {
			if (fscanf(clrfile, "%d;%d;%d\n", &r, &g, &b) == EOF) {
				if (ferror(clrfile))
					LOG_PERROR("fscanf");
				else
					LOG_ERROR("unexpected EOF");
				fclose(clrfile);
				return;
			}
			palette[i].R = r;
			palette[i].G = g;
			palette[i].B = b;
		}
   		fclose(clrfile);
		wf->setcolors();
   		setColorButtons();
		palfilename = p;
		palLabelStr = p;
		size_t pos = palLabelStr.find_last_of('/');
		if (pos != std::string::npos) palLabelStr.erase(0, pos+1);
		WF_Palette->label("                                          ");
		WF_Palette->redraw_label();
		palLabelStr = _("Palette: ") + palLabelStr;
		WF_Palette->label(palLabelStr.c_str());
		WF_Palette->redraw_label();
		WF_Palette->redraw();
		progdefaults.PaletteName = palLabelStr;
	}
}

void savePalette()
{
	FILE *clrfile = NULL;
	if (palfilename.size() == 0) {
		palfilename = PalettesDir;
		palfilename.append ("fldigi.pal");
	}
	const char *p = FSEL::saveas(_("Save palette"), _("Fldigi palette\t*.pal"), palfilename.c_str());
	if (!p) return;
	if ((clrfile = fl_fopen(p, "w")) != NULL) {
		for (int i = 0; i < 9; i++) {
			fprintf(clrfile, "%3d;%3d;%3d\n", palette[i].R, palette[i].G, palette[i].B );
		}
   		fclose(clrfile);
		palfilename = p;
		palLabelStr = p;
		size_t pos = palLabelStr.find_last_of('/');
		if (pos != std::string::npos) palLabelStr.erase(0, pos+1);
		palLabelStr = _("Palette: ") + palLabelStr;
		WF_Palette->label("                                          ");
		WF_Palette->redraw_label();
		WF_Palette->label(palLabelStr.c_str());
		WF_Palette->redraw_label();
		WF_Palette->redraw();
		progdefaults.PaletteName = palLabelStr;
   	}
}

RGB FHpalette[9];
static std::string FHpalfilename = "";
static std::string FHpalLabelStr;

void selectFHColor(int n)
{
	uchar r, g, b;
	r = FHpalette[n].R;
	g = FHpalette[n].G;
	b = FHpalette[n].B;
	fl_color_chooser("Spectrum", r, g, b);

	FHpalette[n].R = r;
	FHpalette[n].G = g;
	FHpalette[n].B = b;

	btn_FH_Color[n]->color( fl_rgb_color( FHpalette[n].R, FHpalette[n].G, FHpalette[n].B ) );
	btn_FH_Color[n]->redraw();
	FHdisp->set_colors(FHpalette);
	FHdisp->redraw();

	FH_Palette->palette_to_mag(FHpalette);
}

void setFH_ColorButtons()
{
	for (int i = 0; i < 9; i++) {
		btn_FH_Color[i]->color( fl_rgb_color( FHpalette[i].R, FHpalette[i].G, FHpalette[i].B ) );
		btn_FH_Color[i]->redraw();
	}
}

void loadFHPalette()
{
	int r,g,b;
	FILE *clrfile = NULL;
	if (FHpalfilename.size() == 0) {
		FHpalfilename = PalettesDir;
		FHpalfilename.append ("fldigi.pal");
	}
    const char *p = FSEL::select(_("Open FHPalette"), _("Fldigi FHPalette\t*.pal"), FHpalfilename.c_str());
	if (!p) return;
	if (!*p) return;
	if ((clrfile = fl_fopen(p, "r")) != NULL) {
		for (int i = 0; i < 9; i++) {
			if (fscanf(clrfile, "%d;%d;%d\n", &r, &g, &b) == EOF) {
				if (ferror(clrfile))
					LOG_PERROR("fscanf");
				else
					LOG_ERROR("unexpected EOF");
				fclose(clrfile);
				return;
			}
			FHpalette[i].R = r;
			FHpalette[i].G = g;
			FHpalette[i].B = b;
		}
		fclose(clrfile);
		setFH_ColorButtons();
		FHpalfilename = p;
		FHpalLabelStr = p;

		size_t pos = FHpalLabelStr.find_last_of('/');
		if (pos != std::string::npos) FHpalLabelStr.erase(0, pos+1);
		pos = FHpalLabelStr.find(".pal");
		if (pos != std::string::npos) FHpalLabelStr.erase(pos);
		FHpalLabelStr = _("Palette: ") + FHpalLabelStr;

		FH_Palette->label("                                          ");
		FH_Palette->redraw_label();
		FH_Palette->label(FHpalLabelStr.c_str());
		FH_Palette->redraw_label();

		progdefaults.FHPaletteName = FHpalLabelStr;

		FH_Palette->palette_to_mag(FHpalette);

		FHdisp->set_colors(FHpalette);

	}

}

void saveFHPalette()
{
	FILE *clrfile = NULL;
	if (FHpalfilename.size() == 0) {
		FHpalfilename = PalettesDir;
		FHpalfilename.append ("fldigi.pal");
	}
	const char *p = FSEL::saveas(_("Save FHPalette"), _("Fldigi FHPalette\t*.pal"), FHpalfilename.c_str());
	if (!p) return;
	if ((clrfile = fl_fopen(p, "w")) != NULL) {
		for (int i = 0; i < 9; i++) {
			fprintf(clrfile, "%3d;%3d;%3d\n", FHpalette[i].R, FHpalette[i].G, FHpalette[i].B );
		}
   		fclose(clrfile);
		palfilename = p;
		palLabelStr = p;

		size_t pos = FHpalLabelStr.find_last_of('/');
		if (pos != std::string::npos) FHpalLabelStr.erase(0, pos+1);
		pos = FHpalLabelStr.find(".pal");
		if (pos != std::string::npos) FHpalLabelStr.erase(pos);
		FHpalLabelStr = _("Palette: ") + FHpalLabelStr;

		FH_Palette->label("                                          ");
		FH_Palette->redraw_label();
		FH_Palette->label(FHpalLabelStr.c_str());
		FH_Palette->redraw_label();

		progdefaults.FHPaletteName = FHpalLabelStr;
   	}
}


