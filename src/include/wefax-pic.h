// ----------------------------------------------------------------------------
// Copyright (C) 2014
//              David Freese, W1HKJ
//              Remi Chateauneu
//
// This file is part of fldigi
//
// fldigi is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#ifndef _WEFAX_PIC_H
#define _WEFAX_PIC_H

#include <FL/Fl_Double_Window.H>

class Fl_Menu_ ;

class wefax ;

class wefax_pic
{
	static void create_rx_viewer(int wid_x, int wid_y,int wid_win, int hei_win);
	static void create_tx_viewer(int wid_x, int wid_y,int wid_win, int hei_win);
public:
	static void set_tx_pic(unsigned char data, int col, int row, bool is_color );
	static int  normalize_lpm( double the_lpm_value );
	static void update_rx_lpm(int lpm);
	static int update_rx_pic_col(unsigned char data, int pos);
	static void update_rx_pic_bw(unsigned char data, int pos);
	static void tx_viewer_resize(int the_width, int the_height);
	static void restart_tx_viewer(void);
	static void abort_rx_viewer(void);
	static void abort_tx_viewer(void);
	static void resize_rx_viewer(int width_img);
	static void set_rx_label(const std::string & win_label);
	static void delete_rx_viewer(void);
	static void delete_tx_viewer(void);
	static void skip_rx_apt(void);
	static void skip_rx_phasing(bool auto_center);
	static void cb_mnu_pic_viewer_rx(Fl_Menu_ *, void *);
	static void cb_mnu_pic_viewer_tx(Fl_Menu_ *, void *);
	static void setpicture_link(wefax *me);
	static void save_image(const std::string & fil_name, const std::string & extra_comments);
	static void power( double start, double phase, double image, double black, double stop );
	static void send_image( const std::string & fil_name );
	static void set_manual( bool manual_mode );
	static void update_auto_center( bool is_auto_center );

	static void create_both( bool called_from_fl_digi );
	static void show_both();
	static void hide_both();
};

extern Fl_Double_Window *wefax_pic_tx_win;
extern Fl_Double_Window *wefax_pic_rx_win;

#endif // _WEFAX_PIC_H
