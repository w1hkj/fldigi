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

#ifndef picture_H
#define picture_H

#include <FL/Fl_Widget.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Image.H>

class picture : public Fl_Widget { 
private:
	unsigned char *vidbuf;
	int bufsize;
	int width;
	int height;
	int depth;
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
	int	save_png(const char *);
};

class picbox : public Fl_Box
{
public:
	picbox(int x, int y, int w, int h, const char* l = 0)
		: Fl_Box(x, y, w, h, l) { }
	int handle(int event);
};

#endif
