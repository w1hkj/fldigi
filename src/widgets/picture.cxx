// ----------------------------------------------------------------------------
// picture.cxx rgb picture viewer
//
// Copyright (C) 2006-2008
//		Dave Freese, W1HKJ
// Copyright (C) 2008-2009
//		Stelios Bounanos, M0GLD
// Copyright (C) 2010
//		Remi Chateauneu, F4ECW
//
// This file is part of fldigi.  
//
// fldigi is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// Fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi.  If not, write to the Free Software
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
#include <algorithm>

#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <FL/Fl.H>
#include <FL/fl_draw.H>

#include <zlib.h>
#include <png.h>

#include "fl_digi.h"
#include "trx.h"
#include "fl_lock.h"
#include "picture.h"
#include "debug.h"
#include "timeops.h"

using namespace std;

picture::picture (int X, int Y, int W, int H, int bg_col) :
	Fl_Widget (X, Y, W, H) 
{
	width = W;
	height = H;
	bufsize = W * H * depth;
	numcol = 0;
	slantdir = 0;
	vidbuf = new unsigned char[bufsize];
	background = bg_col ;
	memset( vidbuf, background, bufsize );	
	zoom = 0 ;
	binary = false ;
	binary_threshold = 128 ;
}

picture::~picture()
{
	if (vidbuf) delete [] vidbuf;
}

void picture::video(unsigned char const *data, int len )
{
	if (len > bufsize) return;
	FL_LOCK_D();
	memcpy( vidbuf, data, len );
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
	memset(vidbuf, background, bufsize);
	redraw();
	FL_UNLOCK_D();
}


void picture::resize_zoom(int x, int y, int w, int h)
{
	if( zoom < 0 ) {
		int stride = -zoom + 1 ;
		Fl_Widget::resize(x,y,w/stride,h/stride);
	} else if( zoom > 0 ) {
		int stride = zoom + 1 ;
		Fl_Widget::resize(x,y,w*stride,h*stride);
	} else {
		Fl_Widget::resize(x,y,w,h);
	}
}

void picture::resize(int x, int y, int w, int h)
{
	FL_LOCK_D();
	width = w;
	height = h;
	delete [] vidbuf;
	bufsize = depth * w * h;
	vidbuf = new unsigned char[bufsize];
	memset( vidbuf, background, bufsize );
	resize_zoom(x,y,w,h);

	FL_UNLOCK_D();
}

/// No data destruction. Used when the received image grows more than expected.
/// Beware that this is not protected by a mutex.
void picture::resize_height(int new_height, bool clear_img)
{
	int new_bufsize = width * new_height * depth;

	/// If the allocation fails, std::bad_alloc is thrown.
	unsigned char * new_vidbuf = new unsigned char[new_bufsize];
	if( clear_img )
	{
		/// Sets to zero the complete image.
		memset( new_vidbuf, background, new_bufsize );
	}
	else
	{
		if( new_height <= height )
		{
			memcpy( new_vidbuf, vidbuf, new_bufsize );
		}
		else
		{
			memcpy( new_vidbuf, vidbuf, bufsize );
			memset( new_vidbuf + bufsize, background, new_bufsize - bufsize );
		}
	}
	delete [] vidbuf ;
	vidbuf = new_vidbuf ;
	bufsize = new_bufsize ;
	height = new_height;
	resize_zoom( x(), y(), width, height );
	redraw();
}

void picture::stretch(double the_ratio)
{

	/// We do not change the width but the height
	int new_height = height * the_ratio + 0.5 ;
	int new_bufsize = width * new_height * depth;

	/// If the allocation fails, std::bad_alloc is thrown.
	unsigned char * new_vidbuf = new unsigned char[new_bufsize];

	/// No interpolation, it takes the nearest pixel.
	for( int ix_out = 0 ; ix_out < new_bufsize ; ix_out += depth ) {
		int ix_in = 0.5 + ( double )ix_out * the_ratio ;
		switch( ix_in % depth ) {
			case 1 : --ix_in ; break ;
			case 2 : ++ix_in ; break ;
			default: ;
		}

		if( ix_in >= bufsize ) {
			/// Grey value as a filler to indicate the end. For debugging.
			memset( new_vidbuf + ix_out, 128, new_bufsize - ix_out );
			break ;
		}
		for( int i = 0; i < depth ; ++i )
		{
			new_vidbuf[ ix_out + i ] = vidbuf[ ix_in + i ];
		}
	};

	delete [] vidbuf ;
	vidbuf = new_vidbuf ;
	bufsize = new_bufsize ;
	height = new_height;
	resize_zoom( x(), y(), width, height );
	redraw();
}

/// Change the horizontal center of the image by shifting the pixels.
/// Beware that it is not protected by FL_LOCK_D/FL_UNLOCK_D
void picture::shift_horizontal_center(int horizontal_shift)
{
	/// This is a number of pixels.
	horizontal_shift *= depth ;
	if( horizontal_shift < -bufsize ) {
		horizontal_shift = -bufsize ;
	}

	if( horizontal_shift > 0 ) {
		/// Here we lose a couple of pixels at the end of the buffer 
		/// if there is not a line enough. It should not be a lot.
		memmove( vidbuf + horizontal_shift, vidbuf, bufsize - horizontal_shift );
		memset( vidbuf, background, horizontal_shift );
	} else {
		/// Here, it is not necessary to reduce the buffer'size.
		memmove( vidbuf, vidbuf - horizontal_shift, bufsize + horizontal_shift );
		memset( vidbuf + bufsize + horizontal_shift, background, -horizontal_shift );
	}

	redraw();
}

void picture::set_zoom( int the_zoom )
{
	zoom = the_zoom ;
	/// The size of the displayed bitmap is changed.
	resize_zoom( x(), y(), width, height );
}

// in 	data 	                     user data passed to function
// in 	x_screen,y_screen,wid_screen position and width of scan line in image
// out 	buf 	                     buffer for generated image data.
// Must copy wid_screen pixels from scanline y_screen, starting at pixel x_screen to this buffer. 
void picture::draw_cb( void *data, int x_screen, int y_screen, int wid_screen, uchar * __restrict__ buf) 
{
	const picture * __restrict__ ptr_pic = ( const picture * ) data ;
	const int img_width = ptr_pic->width ;
	const unsigned char * __restrict__ in_ptr = ptr_pic->vidbuf ;

	/// One pixel out of (zoom+1)
	if( ptr_pic->zoom < 0 ) {
		const int stride = -ptr_pic->zoom + 1 ;
		const int in_offset = ( img_width * y_screen + x_screen ) * stride ;
		int dpth_in_offset = depth * in_offset ;

		if(ptr_pic->binary) {
			for( int ix_w = 0, max_w = wid_screen * depth; ix_w < max_w ; ix_w += depth ) {
				buf[ ix_w     ] = ptr_pic->pix2bin(in_ptr[ dpth_in_offset     ]);
				buf[ ix_w + 1 ] = ptr_pic->pix2bin(in_ptr[ dpth_in_offset + 1 ]);
				buf[ ix_w + 2 ] = ptr_pic->pix2bin(in_ptr[ dpth_in_offset + 2 ]);
				dpth_in_offset += depth * stride ;
			}
		}
		else {
			for( int ix_w = 0, max_w = wid_screen * depth; ix_w < max_w ; ix_w += depth ) {
				buf[ ix_w     ] = in_ptr[ dpth_in_offset     ];
				buf[ ix_w + 1 ] = in_ptr[ dpth_in_offset + 1 ];
				buf[ ix_w + 2 ] = in_ptr[ dpth_in_offset + 2 ];
				dpth_in_offset += depth * stride ;
			}
		}
		return ;
	}

	/// Reads each input pixel (-zoom+1) times.
	if( ptr_pic->zoom > 0 ) {
		const int stride = ptr_pic->zoom + 1 ;
		const int in_offset = img_width * ( y_screen / stride ) + x_screen / stride ;
#ifndef NDEBUG
		if( y_screen / stride >= ptr_pic->h() )
		{
			LOG_ERROR(
				"Overflow2 y_screen=%d h=%d y_screen*stride=%d height=%d stride=%d\n",
				y_screen,
				ptr_pic->h(),
				(y_screen/stride),
				ptr_pic->height,
				stride );
			return ;
		}
#endif
		if(ptr_pic->binary) {
                	for( int ix_w = 0, max_w = wid_screen * depth, dpth_in_offset = depth * in_offset ; ix_w < max_w ; )
			{
				unsigned char in_dpth_in_offset_0 = ptr_pic->pix2bin(in_ptr[ dpth_in_offset     ]);
				unsigned char in_dpth_in_offset_1 = ptr_pic->pix2bin(in_ptr[ dpth_in_offset + 1 ]);
				unsigned char in_dpth_in_offset_2 = ptr_pic->pix2bin(in_ptr[ dpth_in_offset + 2 ]);

				// Stride is less than 4 or 5.
				for( int j= 0; j < stride; j++, ix_w += depth )
				{
					buf[ ix_w     ] = in_dpth_in_offset_0;
					buf[ ix_w + 1 ] = in_dpth_in_offset_1;
					buf[ ix_w + 2 ] = in_dpth_in_offset_2;
				}
				dpth_in_offset += depth ;
			}
		} else {
                	for( int ix_w = 0, max_w = wid_screen * depth, dpth_in_offset = depth * in_offset ; ix_w < max_w ; )
			{
				unsigned char in_dpth_in_offset_0 = in_ptr[ dpth_in_offset     ];
				unsigned char in_dpth_in_offset_1 = in_ptr[ dpth_in_offset + 1 ];
				unsigned char in_dpth_in_offset_2 = in_ptr[ dpth_in_offset + 2 ];

				// Stride is less than 4 or 5.
				for( int j= 0; j < stride; j++, ix_w += depth )
				{
					buf[ ix_w     ] = in_dpth_in_offset_0;
					buf[ ix_w + 1 ] = in_dpth_in_offset_1;
					buf[ ix_w + 2 ] = in_dpth_in_offset_2;
				}
				dpth_in_offset += depth ;
			}
		}

		return ;
	}

	// zoom == 0, stride=1
	const int in_offset = img_width * y_screen + x_screen ;
	if(ptr_pic->binary) {
		int dpth_in_offset = depth * in_offset ;
		for( int ix_w = 0, max_w = wid_screen * depth; ix_w < max_w ; ix_w += depth ) {
			buf[ ix_w     ] = ptr_pic->pix2bin(in_ptr[ dpth_in_offset     ]);
			buf[ ix_w + 1 ] = ptr_pic->pix2bin(in_ptr[ dpth_in_offset + 1 ]);
			buf[ ix_w + 2 ] = ptr_pic->pix2bin(in_ptr[ dpth_in_offset + 2 ]);
			dpth_in_offset += depth ;
		}
	} else {
		abort(); // This should never be called, see optimization in picture::draw().
	}
} // picture::draw_cb

void picture::draw()
{
	if( ( zoom == 0 ) && ( binary == false ) ) {
		/// No scaling, this is faster.
		fl_draw_image( vidbuf, x(), y(), w(), h() );
	} else {
		fl_draw_image( draw_cb, this, x(), y(), w(), h() );
		// redraw();
	}
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

static inline unsigned char avg_pix( const unsigned char * vidbuf )
{
	return ( vidbuf[ 0 ] + vidbuf[ 1 ] + vidbuf[ 2 ] ) / picture::depth ;
}

int picture::save_png(const char* filename, bool monochrome, const char *extra_comments)
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
	/* png_set_compression_level() shall set the compression level to "level". 
	 * The valid values for "level" range from [0,9], corresponding directly 
	 * to compression levels for zlib. The value 0 implies no compression 
	 * and 9 implies maximal compression. Note: Tests have shown that zlib 
	 * compression levels 3-6 usually perform as well as level 9 for PNG images, 
	 * and do considerably fewer calculations. */
	png_set_compression_level(png, Z_BEST_COMPRESSION);

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
	int color_type = monochrome ? PNG_COLOR_TYPE_GRAY : PNG_COLOR_TYPE_RGB ;
	/// Color images must take eight bits per pixel.
	const int bit_depth = ( monochrome && binary ) ? 1 : 8 ;
	png_set_IHDR(png, info, width, height, bit_depth,
		     color_type, PNG_INTERLACE_NONE,
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
	if( extra_comments ) {
		comment << extra_comments ;
	}
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

	// Extra check for debugging.
	if( height * width * depth != bufsize ) {
		LOG_ERROR("Buffer inconsistency h=%d w=%d b=%d", height, width, bufsize );
	}

	// write image
	if(monochrome)
	{
		unsigned char tmp_row[width];
		png_bytep row;
		for (int i = 0; i < height; i++) {
			int row_offset = i * width * depth ;
			if( binary )
			{
				unsigned char accumPix = 0 ;
				int j_offset = 0 ;
				for(int j = 0 ; j < width; ++j )
				{
					int col_offset = row_offset + j * depth ;
					unsigned char tmpChr = avg_pix( vidbuf + col_offset );
					tmpChr = pix2bin(tmpChr) ? 1 : 0 ;
					j_offset = j & 0x07 ;
					tmpChr = tmpChr << ( 7 - j_offset );
					accumPix |= tmpChr ;

					if( j_offset == 7 ) {
						tmp_row[ j >> 3 ] = accumPix ;
						accumPix = 0 ;
					}
				}
				if( j_offset != 7 ) {
					tmp_row[ width >> 3 ] = accumPix ;
				}
			} else {
				for(int j = 0 ; j < width; ++j )
				{
					int col_offset = row_offset + j * depth ;
					tmp_row[j] = avg_pix( vidbuf + col_offset );
				}
			}
			row = tmp_row;
			png_write_rows(png, &row, 1);
		}
	}
	else
	{
		png_bytep row;
		for (int i = 0; i < height; i++) {
			row = &vidbuf[i * width * depth];
			png_write_rows(png, &row, 1);
		}
	}
	png_write_end(png, info);

	// clean up
	free(text.key);
	free(text.text);
	png_destroy_write_struct(&png, &info);
	fclose(fp);

	return 0;
}

bool picture::restore( int row, int margin )
{
	if( ( row <= noise_height_margin ) || ( row >= height ) ) return true;

	unsigned char * line_ante = vidbuf + (row - noise_height_margin) * width * depth;
	// Copy the new calculated value (at previous call) to all channels.
	// TODO: Do that when switching off noise removal, otherwise a couple of colored pixels are left.
	for( int col = margin ; col < width - margin; ++col )
	{
		int offset = col * depth ;
		line_ante[ offset ] = line_ante[ offset + 2 ] = line_ante[ offset + 1 ];
	}
	return false ;
}

void picture::erosion( int row )
{
	static const size_t margin_one = 1 ;
	if( restore( row, margin_one ) ) return ;

	const unsigned char * line_prev = vidbuf + (row - noise_height_margin + 1) * width * depth;
	      unsigned char * line_curr = vidbuf + (row - noise_height_margin + 2) * width * depth;
	const unsigned char * line_next = vidbuf + (row - noise_height_margin + 3) * width * depth;

	for( size_t col = margin_one ; col < width - margin_one; ++col )
	{
		unsigned char new_pix = 255 ;
		new_pix = std::min( new_pix, line_prev[ depth * ( col - 1 ) ] );
		new_pix = std::min( new_pix, line_prev[ depth * ( col     ) ] );
		new_pix = std::min( new_pix, line_prev[ depth * ( col + 1 ) ] );
		new_pix = std::min( new_pix, line_curr[ depth * ( col - 1 ) ] );
		new_pix = std::min( new_pix, line_curr[ depth * ( col     ) ] );
		new_pix = std::min( new_pix, line_curr[ depth * ( col + 1 ) ] );
		new_pix = std::min( new_pix, line_next[ depth * ( col - 1 ) ] );
		new_pix = std::min( new_pix, line_next[ depth * ( col     ) ] );
		new_pix = std::min( new_pix, line_next[ depth * ( col + 1 ) ] );

		/// Use this channel as a buffer. Beware that if we change the slant,
		// this component might not be restored
		// because the line position changed. Not a big problem.
		// We might forbid slanting when de-noising.
		line_curr[ col * depth + 1 ] = new_pix;
	}
}

void picture::dilatation( int row )
{
	static const size_t margin_one = 1 ;
	if( restore( row, margin_one ) ) return ;

	const unsigned char * line_prev = vidbuf + (row - noise_height_margin + 1) * width * depth;
	      unsigned char * line_curr = vidbuf + (row - noise_height_margin + 2) * width * depth;
	const unsigned char * line_next = vidbuf + (row - noise_height_margin + 3) * width * depth;

	for( size_t col = margin_one ; col < width - margin_one; ++col )
	{
		unsigned char new_pix = 0 ;
		new_pix = std::max( new_pix, line_prev[ depth * ( col - 1 ) ] );
		new_pix = std::max( new_pix, line_prev[ depth * ( col     ) ] );
		new_pix = std::max( new_pix, line_prev[ depth * ( col + 1 ) ] );
		new_pix = std::max( new_pix, line_curr[ depth * ( col - 1 ) ] );
		new_pix = std::max( new_pix, line_curr[ depth * ( col     ) ] );
		new_pix = std::max( new_pix, line_curr[ depth * ( col + 1 ) ] );
		new_pix = std::max( new_pix, line_next[ depth * ( col - 1 ) ] );
		new_pix = std::max( new_pix, line_next[ depth * ( col     ) ] );
		new_pix = std::max( new_pix, line_next[ depth * ( col + 1 ) ] );

		/// Use this channel as a buffer. Beware that if we change the slant,
		// this component might not be restored
		// because the line position changed. Not a big problem.
		// We might forbid slanting when de-noising.
		line_curr[ col * depth + 1 ] = new_pix;
	}
}

void picture::remove_noise( int row, int half_len, int noise_margin )
{
	if( restore( row, half_len ) ) return ;

	const unsigned char * line_prev = vidbuf + (row - noise_height_margin + 1) * width * depth;
	      unsigned char * line_curr = vidbuf + (row - noise_height_margin + 2) * width * depth;
	const unsigned char * line_next = vidbuf + (row - noise_height_margin + 3) * width * depth;

	const int nb_neighbours = ( 2 * ( 2 * half_len + 1 ) );
	int medians[nb_neighbours];

	/// Takes into account the first component only.
	for( int col = half_len ; col < width - half_len; ++col )
	{
		int curr_pix = line_curr[ col * depth ];
		assert( ( curr_pix >= 0 ) && ( curr_pix <= 255 ) );

		int pix_min = 255, pix_max = 0;
		for( int subcol = col - half_len, tmp, nghb_i = 0 ; subcol <= col + half_len ; ++subcol )
		{
			int offset = subcol * depth ;
			tmp = line_prev[ offset ];
			if( tmp < pix_min ) pix_min = tmp ;
			else if( tmp > pix_max ) pix_max = tmp ;
			medians[nghb_i++] = tmp;

			tmp = line_next[ offset ];
			if( tmp < pix_min ) pix_min = tmp ;
			else if( tmp > pix_max ) pix_max = tmp ;
			medians[nghb_i++] = tmp;
		}

		// Maybe the pixel is between min and max.
		int thres_min = pix_min - noise_margin;
		if(thres_min < 0) thres_min = 0;
		assert(thres_min <= pix_min);

		int thres_max = pix_max + noise_margin;
		if(thres_max > 255 ) thres_max = 255;
		if(thres_max < pix_max) abort();

		if( ( curr_pix >= thres_min ) && ( curr_pix <= thres_max ) ) continue ;
		assert( ( pix_max >= 0 ) && ( pix_max <= 255 ) );

		std::sort( medians, medians + nb_neighbours );
		int new_pix = medians[ nb_neighbours / 2 ];
		assert( new_pix >= 0 );

		/// Use this channel as a buffer. Beware that if we change the slant,
		// this component might not be restored
		// because the line position changed. Not a big problem.
		// We might forbid slanting when de-noising.
		line_curr[ col * depth + 1 ] = new_pix;
	}
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
// from dnd event "file:///home/dave/Photos/dave.jpeg"
	string::size_type p;
	if ((p = text.find("file://")) != string::npos)
		text.erase(0, p + strlen("file://"));
	if ((p = text.find('\r')) != string::npos)
		text.erase(p);
	if ((p = text.find('\n')) != string::npos)
		text.erase(p);

	struct stat st;
	if (stat(text.c_str(), &st) == -1 || !S_ISREG(st.st_mode))
		return 0;
	extern void load_image(const char*);
	load_image(text.c_str());

	return 1;
}
