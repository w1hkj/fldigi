#include <config.h>

#include "colorbox.h"
#include "waterfall.h"
#include "confdialog.h"
#include <string>
#include <FL/Fl_Color_Chooser.H>

using namespace std;

#include "File_Selector.h"

void colorbox::draw() {
	int ypos = y() + 2;
	int xpos;
	int ht = h() - 4;
	draw_box();
	for(int i = 0; i < 256; i++){
		xpos = x() + 2 + i;
		fl_rectf (xpos, ypos, 1, ht, mag2RGBI[i].R, mag2RGBI[i].G, mag2RGBI[i].B);
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
		palfilename = HomeDir;
		palfilename.append ("/fldigi.pal");
	}
    char *p = File_Select("Open palette", "*.pal", palfilename.c_str(), 0);
	if (!p) return;
	if ((clrfile = fopen(p, "r")) != NULL) {
		for (int i = 0; i < 9; i++) {
			if (fscanf(clrfile, "%d;%d;%d\n", &r, &g, &b) == EOF) {
				if (ferror(clrfile))
					perror("fscanf");
				else
					cerr << p << ": unexpected EOF\n";
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
		palLabelStr = "Palette: " + palLabelStr;
		WF_Palette->label(palLabelStr.c_str());
		WF_Palette->redraw();
	}
}

void savePalette()
{
	FILE *clrfile = NULL;
	if (palfilename.size() == 0) {
		palfilename = HomeDir;
		palfilename.append ("/fldigi.pal");
	}
	char *p = File_Select("Save palette", "*.pal", palfilename.c_str(), 0);
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
		palLabelStr = "Palette: " + palLabelStr;
		WF_Palette->label(palLabelStr.c_str());
   	}
}
