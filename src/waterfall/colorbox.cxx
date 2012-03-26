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
#include "waterfall.h"
#include "confdialog.h"
#include "main.h"
#include "fl_digi.h"
#include "fileselect.h"
#include "debug.h"

using namespace std;

void colorbox::draw() {
	int ypos = y() + 2;
	int xpos;
	int ht = h() - 4;
	int wd = w() - 4;
	draw_box();
	for(int i = 0; i < wd; i++){
		xpos = x() + 2 + i;
		int xc = i * 256 / wd;
		fl_rectf (xpos, ypos, 1, ht, mag2RGBI[xc].R, mag2RGBI[xc].G, mag2RGBI[xc].B);
	}
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

static string palfilename = "";
static string palLabelStr;

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
	if ((clrfile = fopen(p, "r")) != NULL) {
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
		if (pos != string::npos) palLabelStr.erase(0, pos+1);
		palLabelStr = _("Palette: ") + palLabelStr;
		WF_Palette->label(palLabelStr.c_str());
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
	if ((clrfile = fopen(p, "w")) != NULL) {
		for (int i = 0; i < 9; i++) {
			fprintf(clrfile, "%3d;%3d;%3d\n", palette[i].R, palette[i].G, palette[i].B );
		}
   		fclose(clrfile);
		palfilename = p;
		palLabelStr = p;
		size_t pos = palLabelStr.find_last_of('/');
		if (pos != string::npos) palLabelStr.erase(0, pos+1);
		palLabelStr = _("Palette: ") + palLabelStr;
		WF_Palette->label(palLabelStr.c_str());
		WF_Palette->redraw();
		progdefaults.PaletteName = palLabelStr;
   	}
}
