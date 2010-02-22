// ----------------------------------------------------------------------------
// picture.cxx rgb picture viewer
//
// Copyright (C) 2006-2008
//		Dave Freese, W1HKJ
// Copyright (C) 2008
//		Stelios Bounanos, M0GLD
//
// This file is part of fldigi.  

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

#ifdef __MINGW32__
#  include "compat.h"
#endif

#include <string>
#include <sstream>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <FL/Fl.H>
#include <FL/fl_draw.H>

#include <png.h>

#include "fl_digi.h"
#include "trx.h"
#include "fl_lock.h"
#include "picture.h"
#include "debug.h"

using namespace std;

picture::picture (int X, int Y, int W, int H) :
	Fl_Widget (X, Y, W, H) 
{
	width = W;
	height = H;
	depth = 3;
	bufsize = W * H * depth;
	numcol = 0;
	slantdir = 0;
	vidbuf = new unsigned char[bufsize];
	memset( vidbuf, 0, bufsize );	
}

picture::~picture()
{
	if (vidbuf) delete [] vidbuf;
}

void picture::video(unsigned char *data, int len )
{
	if (len > bufsize) return;
	FL_LOCK_D();
	memmove( vidbuf, data, len );
	redraw();
	FL_UNLOCK_D();
}

void picture::pixel(unsigned char data, int pos)
{
	if (pos < 0 || pos >= bufsize) return;
	FL_LOCK_D();
	vidbuf[pos] = data;
	if (pos % (width * 3) == 0)
		redraw();
	FL_UNLOCK_D();
}

unsigned char picture::pixel(int pos)
{
	if (pos < 0 || pos >= bufsize) return 0;
	return vidbuf[pos];
}

void picture::clear()
{
	FL_LOCK_D();
	memset(vidbuf, 0, bufsize);
	redraw();
	FL_UNLOCK_D();
}


void picture::resize(int x, int y, int w, int h)
{
	FL_LOCK_D();
	width = w;
	height = h;
	delete [] vidbuf;
	bufsize = depth * w * h;
	vidbuf = new unsigned char[bufsize];
	memset( vidbuf, 0, bufsize );
	Fl_Widget::resize(x,y,w,h);
	FL_UNLOCK_D();
}

void picture::draw()
{
//	draw_box();
	fl_draw_image( vidbuf, x(), y(), w(), h() );
}

void picture::slant_undo()
{
	int row, col;
	unsigned char temp[width * depth];
	if (height == 0 || width == 0 || slantdir == 0) return;
	if (slantdir == -1) { // undo from left
		for (row = 0; row < height; row++) {
			col = numcol * row / (height - 1);
			if (col > 0) {
				memmove(	temp, 
							&vidbuf[(row * width + width - col) * depth], 
							(width - col) * depth );
				memmove(	&vidbuf[(row * width + col)*depth], 
							&vidbuf[row * width *depth], 
							(width - col) * depth );
				memmove(	&vidbuf[row * width * depth], 
							temp, 
							col * depth );
 			}
		}
	} else if (slantdir == 1) { // undo from right
		for (row = 0; row < height; row++) {
			col = numcol * row / (height - 1);
			if (col > 0) {
				memmove(	temp, 
							&vidbuf[row * width * depth], 
							col * depth );
				memmove(	&vidbuf[row * width * depth], 
							&vidbuf[(row * width + col) * depth], 
							(width - col) * depth );
				memmove(	&vidbuf[(row * width + width - col) * depth], 
							temp, 
							col *depth );
			}
		}
	}
	slantdir = 0;
	redraw();
}

void picture::slant_corr(int x, int y)
{
	int row, col;
	unsigned char temp[width * depth];
	if (height == 0 || width == 0) return;
	if (x > width / 2) { // unwrap from right
		numcol = (width - x) * height / y;
		if (numcol > width / 2) numcol = width / 2;
		for (row = 0; row < height; row++) {
			col = numcol * row / (height - 1);
			if (col > 0) {
				memmove(	temp, 
							&vidbuf[(row * width + width - col) * depth], 
							(width - col) * depth );
				memmove(	&vidbuf[(row * width + col)*depth], 
							&vidbuf[row * width *depth], 
							(width - col) * depth );
				memmove(	&vidbuf[row * width * depth], 
							temp, 
							col * depth );
 			}
		}
		slantdir = 1;
	} else { // unwrap from left
		numcol = x * height / y;
		if (numcol > width / 2) numcol = width / 2;
		for (row = 0; row < height; row++) {
			col = numcol * row / (height - 1);
			if (col > 0) {
				memmove(	temp, 
							&vidbuf[row * width * depth], 
							col * depth );
				memmove(	&vidbuf[row * width * depth], 
							&vidbuf[(row * width + col) * depth], 
							(width - col) * depth );
				memmove(	&vidbuf[(row * width + width - col) * depth], 
							temp, 
							col *depth );
			}
		}
		slantdir = -1;
	}
	redraw();
}

int picture::handle(int event)
{
	if (Fl::event_inside( this )) {
		if (event == FL_RELEASE) {
			int xpos = Fl::event_x() - x();
			int ypos = Fl::event_y() - y();
			int evb = Fl::event_button();
			if (evb == 1)
				slant_corr(xpos, ypos);
			else if (evb == 3)
				slant_undo();
			LOG_DEBUG("#2 %d, %d", xpos, ypos);
			return 1;
		}
		return 1;
	}
	return 0;
}

static FILE* open_file(const char* name, const char* suffix)
{
	FILE* fp;

	size_t flen = strlen(name);
	if (name[flen - 1] == '/') {
		// if the name ends in a slash we will generate
		// a timestamped name in the following  format:
		const char t[] = "pic_YYYY-MM-DD_HHMMSSz";

		size_t newlen = flen + sizeof(t);
		if (suffix)
			newlen += 5;
		char* newfn = new char[newlen];
		memcpy(newfn, name, flen);

		time_t time_sec = time(0);
		struct tm ztime;
		(void)gmtime_r(&time_sec, &ztime);

		size_t sz;
		if ((sz = strftime(newfn + flen, newlen - flen, "pic_%Y-%m-%d_%H%M%Sz", &ztime)) > 0) {
			strncpy(newfn + flen + sz, suffix, newlen - flen - sz);
			newfn[newlen - 1] = '\0';
			mkdir(name, 0777);
			fp = fopen(newfn, "wb");
		}
		else
			fp = NULL;
		delete [] newfn;
	}
	else
		fp = fopen(name, "wb");

	return fp;
}

int picture::save_png(const char* filename)
{
	FILE* fp;
	if ((fp = open_file(filename, ".png")) == NULL)
		return -1;

	// set up the png structures
	png_structp png;
	png_infop info;
	if ((png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL)) == NULL) {
		fclose(fp);
		return -1;
	}
	if ((info = png_create_info_struct(png)) == NULL) {
		png_destroy_write_struct(&png, NULL);
		fclose(fp);
		return -1;
	}
	if (setjmp(png_jmpbuf(png))) {
		png_destroy_write_struct(&png, &info);
		fclose(fp);
		return -1;
	}

	// use an stdio stream
	png_init_io(png, fp);

	// set png header
	png_set_IHDR(png, info, width, height, 1 << depth,
		     PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
		     PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	// write text comments
	struct tm tm;
	time_t t = time(NULL);
	gmtime_r(&t, &tm);
	char z[20 + 1];
	strftime(z, sizeof(z), "%Y-%m-%dT%H:%M:%SZ", &tm);
	z[sizeof(z) - 1] = '\0';

	ostringstream comment;
	comment << "Program: " PACKAGE_STRING << '\n'
		<< "Received: " << z << '\n'
		<< "Modem: " << mode_info[active_modem->get_mode()].name << '\n'
		<< "Frequency: " << inpFreq->value() << '\n';
	if (inpCall->size())
		comment << "Log call: " << inpCall->value() << '\n';

	// set text
	png_text text;
	text.key = strdup("Comment");
	text.text = strdup(comment.str().c_str());
	text.compression = PNG_TEXT_COMPRESSION_NONE;
	png_set_text(png, info, &text, 1);

	// write header
	png_write_info(png, info);

	// write image
	png_bytep row;
	for (int i = 0; i < height; i++) {
		row = &vidbuf[i * width * depth];
		png_write_rows(png, &row, 1);
	}
	png_write_end(png, info);

	// clean up
	free(text.key);
	free(text.text);
	png_destroy_write_struct(&png, &info);
	fclose(fp);

	return 0;
}


int picbox::handle(int event)
{
	if (!Fl::event_inside(this))
		return 0;

	switch (event) {
	case FL_DND_ENTER: case FL_DND_LEAVE:
	case FL_DND_DRAG: case FL_DND_RELEASE:
		return 1;
	case FL_PASTE:
		break;
	default:
		return Fl_Box::handle(event);
	}

	// handle FL_PASTE
	string text = Fl::event_text();
	string::size_type p;
	if ((p = text.find("file://")) != string::npos)
		text.erase(0, p + strlen("file://"));
	if ((p = text.find('\r')) != string::npos)
		text.erase(p);

	struct stat st;
	if (stat(text.c_str(), &st) == -1 || !S_ISREG(st.st_mode))
		return 0;
	extern void load_image(const char*);
	load_image(text.c_str());

	return 1;
}
