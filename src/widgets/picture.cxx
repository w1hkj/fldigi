// ----------------------------------------------------------------------------
// picture.cxx rgb picture viewer
//
// Copyright (C) 2006
//		Dave Freese, W1HKJ
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
#include <iostream>

#include <sys/stat.h>
#include <sys/types.h>

#include <FL/fl_ask.H>

extern "C" {
// This is needed to avoid a boolean conflict with jmorecfg.h
#ifdef __CYGWIN__
#  include <w32api/wtypes.h>
#  define HAVE_BOOLEAN 1
#endif

// This is needed to avoid a conflict between a
// system-defined INT32 typedef, and a macro defined in
// jmorecfg.h, included by jpeglib.h. If ADDRESS_TAG_BIT
// is defined, basestd.h has been included and we will have
// the typedef from cygwin, so we define XMD_H to avoid
// defining the macro in jmorecfg.h
#if defined(__CYGWIN__) && !defined(XMD_H)
#  define XMD_H
#  define FLDIGI_JPEG_XMD_H
#endif

#include <jpeglib.h>

#ifdef FLDIGI_JPEG_XMD_H
#  undef FLDIGI_JPEG_XMD_H
#  undef XMD_H
#endif

//#  include "transupp.h"
};

#include <png.h>

#include "picture.h"

using namespace std;

picture::picture (int X, int Y, int W, int H) :
	Fl_Widget (X, Y, W, H) 
{
	width = W;
	height = H;
	depth = 3;
	quality = 80; // for jpeg export
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
//			std::cout << "#2 " << xpos << ", " << ypos << std::endl; fflush(stdout);
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
		if ((sz = strftime(newfn + flen, newlen - flen, "pic_%F_%H%M%Sz", &ztime)) > 0) {
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

//
// save_jpeg - Write a captured picture to a JPEG format file.
//

int picture::save_jpeg(const char *filename)
{
	unsigned char		*ptr;		// Pointer to image data
	FILE				*fp = 0;	// File pointer
	struct jpeg_compress_struct	info;		// Compressor info
	struct jpeg_error_mgr		err;		// Error handler info

	if ((fp = open_file(filename, ".jpg")) == NULL)
		return -1;

// Setup the JPEG compression stuff...
	info.err = jpeg_std_error(&err);
	jpeg_create_compress(&info);
	jpeg_stdio_dest(&info, fp);

	info.image_width      = width;
	info.image_height     = height;
	info.input_components = depth;
	info.in_color_space   = JCS_RGB;//simg->d() == 1 ? JCS_GRAYSCALE : JCS_RGB;

	jpeg_set_defaults(&info);
	jpeg_set_quality(&info, quality, 1);
	jpeg_simple_progression(&info);

	info.optimize_coding = 1;

// Save the image...
	jpeg_start_compress(&info, 1);
	while (info.next_scanline < info.image_height) {
		ptr = &vidbuf[info.next_scanline * width * depth];
		jpeg_write_scanlines(&info, &ptr, 1);
	}
	jpeg_finish_compress(&info);
	jpeg_destroy_compress(&info);

	fclose(fp);

	return 0;
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
		png_destroy_write_struct(&png, png_infopp_NULL);
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

	// TODO: write more useful image comments
	png_text text;
	text.key = strdup("Comment");
	text.text = strdup("MFSK-16 image decoded by " PACKAGE_STRING);
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
