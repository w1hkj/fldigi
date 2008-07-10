// ----------------------------------------------------------------------------
// picture.h
//
// Copyright (C) 2006-2008
//		Dave Freese, W1HKJ
// Copyright (C) 2008
//		Stelios Bounanos, M0GLD
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

#ifndef picture_H
#define picture_H

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Widget.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Box.H>

#include "threads.h"

class picture : public Fl_Widget { 
private:
	unsigned char *vidbuf;
	int bufsize;
	int width;
	int height;
	int depth;
	int quality;
	int numcol;
	int slantdir;
	void slant_corr(int x, int y);
	void slant_undo();

public:
	picture(int, int, int, int);
	~picture();
	void	video(unsigned char *, int);
	void	pixel(unsigned char, int);
	unsigned char	pixel(int);
	int		handle(int);
	void	draw();
	void	clear();
	void	image(Fl_Image *img) {Fl_Widget::image(img);}
	void	resize(int, int, int, int);
#if USE_LIBJPEG
	int	save_jpeg(const char *);
#endif
#if USE_LIBPNG
	int	save_png(const char *);
#endif
};

class picbox : public Fl_Box
{
public:
	picbox(int x, int y, int w, int h, const char* l = 0)
		: Fl_Box(x, y, w, h, l) { }
	int handle(int event);
};

#endif
