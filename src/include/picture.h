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
	int numcol;
	int slantdir;
	void slant_corr(int x, int y);
	void slant_undo();
	int zoom ;
	int background ;
	bool binary ;
	unsigned char binary_threshold ;

	inline unsigned char pix2bin( unsigned char x ) const {
		return x < binary_threshold ? 0 : 255 ;
	}

	static void draw_cb(void *data, int x, int y, int w, uchar *buf);
	void	resize_zoom(int, int, int, int);
public:
	picture(int, int, int, int, int bg_col = 0);
	~picture();
	void	video(unsigned char const *, int);
	void	pixel(unsigned char data, int pos) {
		if (pos < 0 || pos >= bufsize) {
			return ;
		}
		FL_LOCK_D();
		vidbuf[pos] = data;
		if (pos % (width * depth) == 0)
			redraw();
		FL_UNLOCK_D();
	}
	unsigned char	pixel(int);
	int		handle(int);
	void	draw();
	void	clear();
	void	image(Fl_Image *img) {Fl_Widget::image(img);}
	void	resize(int, int, int, int);
	void    resize_height(int new_height, bool clear_img);
	void    shift_horizontal_center(int hShift);
	void    stretch(double the_ratio);
	int	save_png(const char * filename, bool monochrome = false, const char * extra_comments = NULL);
	void    set_zoom(int the_zoom);
	void    set_binary(bool bin_mode) { binary = bin_mode ;}
	int     pix_width(void) const {
		return width ;
	}
	int     pix_height(void) const {
		return height ;
	}
	const unsigned char * buffer(void) const {
		return vidbuf;
	}
	/// Sometimes the row number goes back of one due to rounding error.
	/// If this happens, noise removal does not work.
	static const int noise_height_margin = 5 ;
	void remove_noise( int row, int half_len, int noise_margin );
	static const int depth = 3;

private:
	bool restore( int row, int margin );
public:
	void dilatation( int row );
	void erosion( int row );
	void set_binary_threshold(unsigned char thres) {
		binary_threshold = thres ;
	}
	unsigned char get_binary_threshold() const {
		return binary_threshold ;
	}
};

class picbox : public Fl_Box
{
public:
	picbox(int x, int y, int w, int h, const char* l = 0)
		: Fl_Box(x, y, w, h, l) { }
	int handle(int event);
};

#endif
