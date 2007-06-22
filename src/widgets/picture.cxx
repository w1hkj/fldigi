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

#include <FL/fl_ask.H>

#include "picture.h"

#include <iostream>

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
	Fl::lock();
	memmove( vidbuf, data, len );
	redraw();
	Fl::unlock();
}

void picture::pixel(unsigned char data, int pos)
{
	if (pos < 0 || pos >= bufsize) return;
	Fl::lock();
	vidbuf[pos] = data;
	redraw();
	Fl::unlock();
}

unsigned char picture::pixel(int pos)
{
	if (pos < 0 || pos >= bufsize) return 0;
	return vidbuf[pos];
}

void picture::clear()
{
	Fl::lock();
	memset(vidbuf, 0, bufsize);
	redraw();
	Fl::unlock();
}


void picture::resize(int x, int y, int w, int h)
{
	Fl::lock();
	width = w;
	height = h;
	delete [] vidbuf;
	bufsize = depth * w * h;
	vidbuf = new unsigned char[bufsize];
	memset( vidbuf, 0, bufsize );
	Fl_Widget::resize(x,y,w,h);
	Fl::unlock();
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

//
// save_jpeg - Write a captured picture to a JPEG format file.
//

int picture::save_jpeg(char *filename)
{
	unsigned char		*ptr;		// Pointer to image data
	FILE				*fp;		// File pointer
	struct jpeg_compress_struct	info;		// Compressor info
	struct jpeg_error_mgr		err;		// Error handler info

// Create the output file...
	if ((fp = fopen(filename, "wb")) == NULL)
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

