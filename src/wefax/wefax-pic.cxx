// ----------------------------------------------------------------------------
// mfsk-pic.cxx  --  mfsk support functions
//
// Copyright (C) 2010
//		Remi Chateauneu, F4ECW
//
// This file is part of fldigi.  Adapted from code contained in HAMFAX source code 
// distribution.
//  Hamfax Copyright (C) Christof Schmitt
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

#include <config.h>
#include <libgen.h>
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>


#include <time.h>
#include <unistd.h>

#include <FL/Fl_Widget.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Light_Button.H>
#include <FL/Fl_Spinner.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Int_Input.H>
#include <FL/Fl_Counter.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Slider.H>
#include <FL/Fl_Select_Browser.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Chart.H>
#include <FL/Fl_Shared_Image.H>
#include <FL/Enumerations.H>

#include "wefax-pic.h"

#include "debug.h"
#include "configuration.h"
#include "wefax.h"
#include "trx.h"
#include "fl_digi.h"
#include "main.h"
#include "fileselect.h"
#include "picture.h"
#include "gettext.h"

Fl_Double_Window	*wefax_pic_rx_win = (Fl_Double_Window *)0;

static Fl_Scroll       *wefax_pic_rx_scroll          = (Fl_Scroll *)0 ;
static picture	       *wefax_pic_rx_picture         = (picture *)0;
static Fl_Button       *wefax_btn_rx_save            = (Fl_Button *)0;
static Fl_Button       *wefax_btn_rx_abort           = (Fl_Button *)0;
static Fl_Button       *wefax_btn_rx_pause           = (Fl_Button *)0;
static Fl_Button       *wefax_btn_rx_resume          = (Fl_Button *)0;
static Fl_Choice       *wefax_choice_rx_zoom         = (Fl_Choice *)0;
static Fl_Browser      *wefax_browse_rx_events       = (Fl_Browser *)0;
static Fl_Button       *wefax_btn_rx_skip_apt        = (Fl_Button *)0;
static Fl_Button       *wefax_btn_rx_skip_phasing    = (Fl_Button *)0;
static Fl_Light_Button *wefax_round_rx_noise_removal = (Fl_Light_Button *)0;
static Fl_Light_Button *wefax_round_rx_binary        = (Fl_Light_Button *)0;
static Fl_Spinner      *wefax_spinner_rx_binary      = (Fl_Spinner *)0;
static Fl_Light_Button *wefax_round_rx_non_stop      = (Fl_Light_Button *)0;
static Fl_Output       *wefax_out_rx_row_num         = (Fl_Output *)0;
static Fl_Output       *wefax_out_rx_width           = (Fl_Output *)0;
static Fl_Choice       *wefax_choice_rx_lpm          = (Fl_Choice *)0;
static Fl_Counter      *wefax_cnt_rx_ratio           = (Fl_Counter *)0;
static Fl_Slider       *wefax_slider_rx_center       = (Fl_Slider *)0;
static Fl_Light_Button *wefax_round_rx_auto_center   = (Fl_Light_Button *)0;
static Fl_Chart        *wefax_chart_rx_power         = (Fl_Chart *)0;
static Fl_Choice       *wefax_choice_rx_filter       = (Fl_Choice *)0;

Fl_Double_Window	*wefax_pic_tx_win = (Fl_Double_Window *)0;

static Fl_Scroll *wefax_pic_tx_scroll           = (Fl_Scroll *)0 ;
static picture   *wefax_pic_tx_picture          = (picture *)0;
static picbox    *wefax_pic_tx_box              = (picbox *)0;
static Fl_Choice *wefax_choice_tx_zoom          = (Fl_Choice *)0;
static Fl_Choice *wefax_choice_tx_lpm           = (Fl_Choice *)0;
static Fl_Button *wefax_btn_tx_send_color       = (Fl_Button *)0;
static Fl_Button *wefax_btn_tx_send_grey        = (Fl_Button *)0;
static Fl_Output *wefax_out_tx_row_num          = (Fl_Output *)0;
static Fl_Output *wefax_out_tx_col_num          = (Fl_Output *)0;
static Fl_Button *wefax_btn_tx_send_abort       = (Fl_Button *)0;
static Fl_Button *wefax_btn_tx_load             = (Fl_Button *)0;
static Fl_Button *wefax_btn_tx_clear            = (Fl_Button *)0;
static Fl_Button *wefax_btn_tx_close            = (Fl_Button *)0;

/// The image to send.
static Fl_Shared_Image *wefax_shared_tx_img = (Fl_Shared_Image *)0;

/// This contains the original content of the image to send,
/// converted into three bytes per pixel.
static unsigned char *wefax_xmtimg = (unsigned char *)0;

/// This indicates whether an image to send is loaded in the GUI.
/// It allows to acquire twice when re-loading an image without sending.
static bool wefax_image_loaded_in_gui = false ;

/// Used for shifting the received image left and right.
static volatile int center_val_prev = 0 ;

static volatile bool global_auto_center = false ;

void wefax_pic::update_auto_center(bool is_auto_center)
{
	wefax_round_rx_auto_center->value(is_auto_center ? 1 : 0);
	global_auto_center = is_auto_center;
}

/// Global pointer to the current wefax modem.
static wefax *wefax_serviceme = 0;

/// TODO: This should be hidden in the class picture. It is in wefax too.
static const int bytes_per_pix = 3 ;

/// Initial size of the reception image. A typical fax has about 1300 lines.
static const int curr_pix_h_default = 300 ;

/// Always reset before loading a new image.
static volatile int curr_pix_height = curr_pix_h_default ;

/// The antepenultimate line of the rx image is filtered to remove noise.
static int rx_last_filtered_row = 0 ;

static bool noise_removal = false ;

/// Alters the slanting of the image based on LPM adjustments.
static volatile double rx_slant_ratio = 0.0 ;

/// This transforms the user slant ratio (Small number around 0.0)
/// into a ratio used to stretch the image (Very very small mantissa added to 1.0).
static double slant_factor_default(void)
{
	return 100.0 / ( rx_slant_ratio + 100.0 );
}

static double slant_factor_with_ratio( double ratio_percent )
{
	return ( ratio_percent + 100.0 ) / ( rx_slant_ratio + 100.0 );
}

/// Minimum fax number of rows should be at least this size.
static const int mini_max_fax_lines = 1000 ;

/// When set by the user, no new pixel is added or printed.
/// However, when the printing resumes, the position is not altered.
static volatile bool reception_paused = false ;

/// Sets the label of the received or sent image.
static void set_win_label( Fl_Window * wefax_pic, const std::string & lab)
{
	/// If the wefax window (rx or tx) is embedded in the main window.
	// then we would set wefax_pic = fl_digi_main;
	if( progdefaults.WEFAX_EmbeddedGui ) return ; 
	char* label = strdup(lab.c_str());
	wefax_pic->copy_label(label);
	free(label);
	wefax_pic->redraw();
}

/// Called when clearing the image to send.
static void clear_image(void)
{
	ENSURE_THREAD(FLMAIN_TID);
	if (wefax_xmtimg)
	{
		delete [] wefax_xmtimg;
		wefax_xmtimg = NULL ;
	}
	if (wefax_shared_tx_img) {
		wefax_shared_tx_img->release();
		wefax_shared_tx_img = 0;
	}
	set_win_label(wefax_pic_tx_win,"");
}

/// Clears the loaded image. It allows XML-RPC clients to send an image.
static void wefax_cb_pic_tx_clear( Fl_Widget *, void *)
{
	ENSURE_THREAD(FLMAIN_TID);
	FL_LOCK_D();
	wefax_image_loaded_in_gui = false ;
	clear_image();
	wefax_pic_tx_picture->clear();
	wefax_pic_tx_picture->resize(0,0,0,0);
	wefax_pic::restart_tx_viewer();
	/// Now the lock can be acquired by XML-RPC.
	wefax_serviceme->transmit_lock_release( "Cleared" );
	FL_UNLOCK_D();
}

/// According to config flags, shows or hides the transmission window, and resizes both windows if needed.
static void wefax_pic_show_tx()
{
	ENSURE_THREAD(FLMAIN_TID);
	if( progdefaults.WEFAX_HideTx )
	{
		wefax_pic_tx_win->hide();
		if( progdefaults.WEFAX_EmbeddedGui )
		{
			wefax_pic_rx_win->resize( 
				text_panel->x() + mvgroup->w(), text_panel->y(),
				text_panel->w() - mvgroup->w(),
				text_panel->h() );
		}
	}
	else
	{
		if( progdefaults.WEFAX_EmbeddedGui )
		{
			wefax_pic_rx_win->resize( 
				text_panel->x() + mvgroup->w(), text_panel->y(),
				text_panel->w() - mvgroup->w(), text_panel->h()/2);
			wefax_pic_tx_win->resize( 
				text_panel->x() + mvgroup->w(), wefax_pic_rx_win->y() + wefax_pic_rx_win->h(),
				text_panel->w() - mvgroup->w(), text_panel->h() - wefax_pic_rx_win->h());
		}
		wefax_pic_tx_win->show();
	}
}

static void wefax_cb_pic_tx_close( Fl_Widget *, void *)
{
	ENSURE_THREAD(FLMAIN_TID);
	FL_LOCK_D();
	/// TODO: Small inconvenience: When coming back in wefax mode, the tx window is always closed.
	progdefaults.WEFAX_HideTx = true ;
	wefax_pic_show_tx();
	FL_UNLOCK_D();
}

/// Usual LPM values.
static const struct {
	int          m_value ;
	const char * m_label ;
} all_lpm_values[] = {
	{ 240, "240" },
	{ 120, "120" },
	{  90,  "90" },
	{  60,  "60" }
};

static const int nb_lpm_values = sizeof(all_lpm_values) / sizeof(all_lpm_values[0]);

/// Returns the LPM value choosed on the TX or RX window.
static int get_choice_lpm_value( Fl_Choice * the_choice_lpm )
{
	int idx_lpm = the_choice_lpm->value();
	if( ( idx_lpm < 0 ) || ( idx_lpm >= nb_lpm_values ) ) {
		LOG_WARN( "Invalid LPM index=%d", idx_lpm );
		idx_lpm = 0 ;
	}
	return all_lpm_values[ idx_lpm ].m_value ;
}

/// Lpm=120 is by far the most common value, therefore used by default if nothing else works.
// wefax.cxx will anyway try 120 for wefax576 or 60 for wefax288.
static const int lpm_default_idx = 1 ;

static const char * title_choice_lpm = "LPM" ;

/// Fills a FLTK widget with LPM vpossible values. Used for transmission and reception.
static Fl_Choice * make_lpm_choice( int width_offset, int y_btn, int width_btn, int hei_tx_btn )
{
	Fl_Choice * choice_lpm = new Fl_Choice(width_offset, y_btn, width_btn, hei_tx_btn, title_choice_lpm );
	for( int ix_lpm = 0 ; ix_lpm < nb_lpm_values ; ++ix_lpm ) {
		choice_lpm->add( all_lpm_values[ ix_lpm ].m_label );
	};
	choice_lpm->value(lpm_default_idx);
	choice_lpm->tooltip(_("Set the LPM value"));
	return choice_lpm ;
}

/// Sometimes the LPM can be calculated to 122.0 when it should be 120.0.
int wefax_pic::normalize_lpm( double the_lpm )
{
	for( int ix_lpm = 0 ; ix_lpm < nb_lpm_values ; ++ix_lpm ) {
		int curr_lpm = all_lpm_values[ ix_lpm ].m_value ;
		if( std::fabs( the_lpm - curr_lpm ) < 3.0 ) {
			return curr_lpm ;
		}
	};
	int dflt_lpm = all_lpm_values[ lpm_default_idx ].m_value ;
	LOG_INFO("Out of bounds LPM=%f. Setting to default:%d", the_lpm, dflt_lpm );
	return dflt_lpm ;
}

/// At this place, the LPM should have been normalized.
static void set_nearest_lpm( int the_lpm )
{
	for( int ix_lpm = 0 ; ix_lpm < nb_lpm_values ; ++ix_lpm ) {
		if( the_lpm == all_lpm_values[ ix_lpm ].m_value ) {
			wefax_choice_rx_lpm->value( ix_lpm );
			return ;
		}
	};
	LOG_INFO("Unknown LPM=%d. Reset to default:%d",
			the_lpm, all_lpm_values[ lpm_default_idx ].m_value );
	wefax_choice_rx_lpm->value( lpm_default_idx );
}

/// Called just before starting to receive the image. The LPM should be normalized.
void wefax_pic::update_rx_lpm( int the_lpm )
{
	ENSURE_THREAD(FLMAIN_TID);
	set_nearest_lpm( the_lpm );

	/// We keep the previous value of the slant ratio.
	wefax_cnt_rx_ratio->value( rx_slant_ratio );

	/// The increment must be very small otherwise the image is too slanted.
	wefax_cnt_rx_ratio->step( 0.001 );
	wefax_cnt_rx_ratio->precision( 3 );

	/// TODO: Not sure about the difference between range() and bounds().
	wefax_cnt_rx_ratio->range( -1.0, 1.0 );
	wefax_cnt_rx_ratio->bounds( -1.0, 1.0 );
	wefax_cnt_rx_ratio->redraw();
}

/// Called for each new color component.
int wefax_pic::update_rx_pic_col(unsigned char data, int pix_pos )
{
	/// Each time the received image becomes too high, we increase its height.
	static const int curr_pix_incr_height = 100 ;

	/// Three ints per pixel. It is safer to recalculate the 
	/// row index to avoid any buffer overflow, because the given
	/// row is invalid if the image was horizontally moved.
	int row_number = 1 + ( pix_pos / ( wefax_pic_rx_picture->pix_width() * bytes_per_pix ) );

	/// Maybe we must increase the image height.
	if( curr_pix_height <= row_number )
	{
		FL_LOCK_D();
		curr_pix_height = row_number + curr_pix_incr_height ;
		wefax_pic_rx_picture->resize_height( curr_pix_height, false );

		int y_pos = wefax_pic_rx_picture->h() - wefax_pic_rx_scroll->h() ;
		if( y_pos < 0 )
		{
			y_pos = 0 ;
		}
		else
		{
			// Small margin at the bottom, so we can immediately see new lines.
			y_pos += 20 ;
		}
		wefax_pic_rx_picture->position( wefax_pic_rx_picture->x(), -y_pos );
		wefax_pic_rx_scroll->redraw();
		FL_UNLOCK_D();
	}
	wefax_pic_rx_picture->pixel(data, pix_pos);
	return row_number ;
}

/// This estimates the colum of where the horizontal center of the image is,
/// or rather, the beginning of the left margin. The estimation is done on
/// a range of rows. It looks for a vertical band of X pixels, where the image
/// derivative is the lowest. It works well with faxes because they always have
/// a wide blank margin.
static int estimate_rx_image_center( int row_end )
{
	/// This works as well with color images.
	int img_wid = wefax_pic_rx_picture->pix_width() * bytes_per_pix ;

	/// Much bigger that a char, so we can add values without overflow.
	int img_buf[ img_wid ];
	for( int col_ix = 0; col_ix < img_wid ; ++col_ix ) {
		img_buf[ col_ix ] = 0 ;
	}

	unsigned const char * img_start = wefax_pic_rx_picture->buffer();

	/// This computes the absolute value of the horizontal derivative.
	for( int row_ix = 1; row_ix < row_end ; ++row_ix ) {
		int col_offset = row_ix * img_wid ;
		/// Pixels of the current row.
		const unsigned char * row_start = img_start + col_offset ;
		/// This is the previous colum of the next row, does not matter.
		int pix_prev = row_start[img_wid - 1];
		/// This is an estimation of how the image has information or is just uniform.
		for( int col_ix = 0; col_ix < img_wid ; ++col_ix ) {
			int pix_next = row_start[ col_ix ];
			/// Absolute value of the horizontal derivative.
			int deriv = pix_next - pix_prev;
			if( deriv < 0 ) {
				deriv = -deriv ;
			}
			img_buf[ col_ix ] += deriv ;
			/// Used at next iteration so that the memory is read once only.
			pix_prev = pix_next ;
		}
	}

	/// Much bigger that a char, so we can add values without overflow.
	int img_avg[ img_wid ];

	/// The width of the image band on which we compute the average
	/// of the absolute value of the horizontal derivate.
	static const int avg_wid = 150 ;

	int curr_avg = 0 ;
	/// Average of the first column.
	for( int col_ix = 0; col_ix < avg_wid ; ++col_ix ) {
		curr_avg += img_buf[ col_ix ] ;
	}
	img_avg[ 0 ] = curr_avg ;

	/// Rest of the average in a single pass.
	for( int col_ix = 1; col_ix < img_wid - avg_wid ; ++col_ix ) {
		curr_avg += img_buf[ col_ix + avg_wid ] - img_buf[ col_ix - 1 ];
		img_avg[ col_ix ] = curr_avg ;
	}
	/// Finishes the last bit which wraps at the beginning of the row.
	for( int col_ix = img_wid - avg_wid; col_ix < img_wid ; ++col_ix ) {
		curr_avg += img_buf[ col_ix + avg_wid - img_wid ] - img_buf[ col_ix - 1 ];
		img_avg[ col_ix ] = curr_avg ;
	}

	/// Find the minimum of the derivative on an averaged segment. Last col is zero.
	int min_idx = -1 ;
	int min_val = INT_MAX ;
	for( int col_ix = 0; col_ix < img_wid - 1 ; ++col_ix ) {
		int avg_val = img_avg[ col_ix ] ;
		if( avg_val < min_val ) {
			min_idx = col_ix ;
			min_val = avg_val ;
		}
	}

	if( (min_idx < 0) || (min_idx >= img_wid) ) {
		LOG_ERROR("ERROR MINIMUM:%d",min_idx);
		return 0 ;
	}

	/// Better shift left than shift right too much.
	if( min_idx > img_wid / 2 ) {
		min_idx = min_idx - img_wid ;
	}
	/// Three chars per pixel.
	min_idx /= bytes_per_pix ;

	return min_idx ;
}

/// Called for each bw pixel.
void wefax_pic::update_rx_pic_bw(unsigned char data, int pix_pos )
{
	/// No pixel is added nor printed until this flag is reset to false.
	if( reception_paused  ) {
		return ;
	};
	/// The image must be horizontally shifted.
	pix_pos += center_val_prev * bytes_per_pix ;
	if( pix_pos < 0 ) {
		pix_pos = 0 ;
	}

	/// Maybe there is a slant.
	pix_pos = ( double )pix_pos * slant_factor_default() + 0.5 ;

	/// Must be a multiple of the number of bytes per pixel.
	pix_pos = ( pix_pos / bytes_per_pix ) * bytes_per_pix ;

	static int last_row_number = 0 ;

	                 update_rx_pic_col(data, pix_pos);
	                 update_rx_pic_col(data, pix_pos + 1);
	int row_number = update_rx_pic_col(data, pix_pos + 2);

	/// Prints the row number sometimes only, to save CPU.
	if( ( pix_pos % 5000 ) == 0 ) {
		char row_num_buffer[20];
		snprintf( row_num_buffer, sizeof(row_num_buffer), "%d", row_number );
		wefax_out_rx_row_num->value( row_num_buffer );
	}

	/// Maybe we restarted an image or maybe went back because of recentering.
	if( last_row_number > row_number ) {
		last_row_number = row_number ;
	}

	/// Eliminate the noise from the ante-antepenultimate line
	if( noise_removal )
	{
		if( ( row_number > picture::noise_height_margin - 2 ) && ( row_number != rx_last_filtered_row ) )
		{
			static bool math_morph_enhancement = false ;
			/* Not used yet because this depends on the image features.
			 * Lines are properly enhancement, but not letters.
			 * Maybe row_number > 2 * picture::noise_height_margin
		 	 * Also decrease picture::noise_height_margin and ensure image is repaired
		 	 * when switching on/off noise removal. */
			if( math_morph_enhancement ) {
				wefax_pic_rx_picture->dilatation(
					row_number - 2 * picture::noise_height_margin );
				wefax_pic_rx_picture->erosion(
					row_number - 1 * picture::noise_height_margin );
			}
			wefax_pic_rx_picture->remove_noise(
				row_number,
				progdefaults.WEFAX_NoiseMargin,
				progdefaults.WEFAX_NoiseThreshold );
			rx_last_filtered_row = row_number ;
		}
	}

	/// We start to recenter only if the row number is bigger than that.
	static const int row_margin = 100 ;

	/// We try to recenter every X rows.
	static const int row_recenter_period = 10 ;

	/// Every new X-th row, maybe automatic recenter.
	if( ( global_auto_center ) &&
		( (row_number % row_recenter_period) == 0 ) &&
		( row_number > row_margin ) &&
		( row_number >= last_row_number + row_recenter_period ) ) {
		int delta_center = estimate_rx_image_center( row_number );

		static const int neglect_center = 3 ;
		if( (delta_center > -neglect_center) && (delta_center < neglect_center) ) {
			/// We may stop the centering algorithm but it is not stable at the beginning.
			// update_auto_center(false);
		} else {
			center_val_prev -= delta_center ;
			wefax_pic_rx_picture->shift_horizontal_center( -delta_center );
		}
		/// Beware that the line number may go backward if the image is left-shifted,
		/// that is, shortened. Is this assignment REALLY necessary ?
		last_row_number = row_number ;
	}
}

static void wefax_cb_pic_rx_pause( Fl_Widget *, void *)
{
	wefax_btn_rx_pause->hide();
	wefax_btn_rx_resume->show();
	reception_paused = true ;
	wefax_serviceme->update_rx_label();
}

static void wefax_cb_pic_rx_resume( Fl_Widget *, void *)
{
	wefax_btn_rx_pause->show();
	wefax_btn_rx_resume->hide();
	reception_paused = false ;
	wefax_serviceme->update_rx_label();
}

static void LocalSleep( int seconds )
{
#ifdef __MINGW32__
		MilliSleep(seconds);
#else
		usleep(100000*seconds);
#endif
}

/// Displays the latest image file saved.
static void add_to_files_list( const std::string & the_fil_nam )
{
	ENSURE_THREAD(FLMAIN_TID);
	/// If the beginning of the file is the default output directory, do not print it.
	std::string dst_file( the_fil_nam );
	int pos_dflt = dst_file.find( progdefaults.wefax_save_dir );
	if( pos_dflt == 0 ) {
		dst_file.erase( 0, progdefaults.wefax_save_dir.size() );
	}

	int curr_siz = wefax_browse_rx_events->size();
	std::stringstream tmp_strm ;
	/// 999 images seem a reasonable sizing (3 digits).
	tmp_strm << std::setw(3) << ( curr_siz + 1 ) << " " << dst_file;

	/// The complete file name is stored. It needs a private copy.
	wefax_browse_rx_events->add(
            tmp_strm.str().c_str(),
            strdup( the_fil_nam.c_str() ) );
	wefax_browse_rx_events->bottomline( curr_siz + 1 );

	/// This window is hidden/shown to signal that a file was added.
	static const int nb_blink = 5 ;
	for( int ix_blink = 0 ; ix_blink < nb_blink ; ++ix_blink ) {
		wefax_browse_rx_events->hide();
		wefax_browse_rx_events->redraw();
		LocalSleep(1);
		wefax_browse_rx_events->show();
		wefax_browse_rx_events->redraw();
		LocalSleep(1);
	}

	/// If there is not directory specification, adds the default dir.
	if( the_fil_nam.empty() )
		LOG_WARN("Empty file name");
	else if( the_fil_nam[0] != '/' )
		wefax_serviceme->put_received_file( progdefaults.wefax_save_dir + '/' + the_fil_nam );
	else
		wefax_serviceme->put_received_file( the_fil_nam );
};

static void wefax_cb_pic_rx_abort( Fl_Widget *, void *)
{
	ENSURE_THREAD(FLMAIN_TID);
	if (wefax_serviceme != active_modem) return;

	/// This will call wefax_pic::abort_rx_viewer
	wefax_serviceme->end_reception();
	wefax_round_rx_non_stop->value(false);
	wefax_serviceme->set_rx_manual_mode(false);
	wefax_round_rx_non_stop->redraw();
	wefax_btn_rx_pause->show();
	wefax_btn_rx_resume->hide();
}

void wefax_pic::set_manual( bool manual )
{
	wefax_round_rx_non_stop->value(manual);
}

static void wefax_cb_pic_rx_manual( Fl_Widget *, void *)
{
	if (wefax_serviceme != active_modem) return;

	if( wefax_round_rx_non_stop->value() )
	{
		wefax_serviceme->set_rx_manual_mode(true);
		wefax_serviceme->skip_apt();
		wefax_serviceme->skip_phasing(true);
	}
	else
	{
		wefax_serviceme->set_rx_manual_mode(false);
	}
}

static void wefax_cb_pic_rx_center( Fl_Widget *, void *)
{
	if (wefax_serviceme != active_modem) return;
	ENSURE_THREAD(FLMAIN_TID);

	int center_new_val = wefax_slider_rx_center->value();
	int center_delta = center_new_val - center_val_prev ;
	center_val_prev = center_new_val ;

	/// Not sure whether lock/unlock is necessary.
	FL_LOCK_D();
	wefax_pic_rx_picture->shift_horizontal_center( center_delta );

	FL_UNLOCK_D();
}

static void wefax_cb_pic_rx_auto_center( Fl_Widget *, void *)
{
	if (wefax_serviceme != active_modem) return;
	ENSURE_THREAD(FLMAIN_TID);

	char rndVal = wefax_round_rx_auto_center->value();
	wefax_pic::update_auto_center(rndVal ? true : false);
}

/// This gets the directory where images are accessed by default.
static std::string default_dir_get( const std::string & config_dir )
{
	std::string tmp_dir = config_dir.empty() ? PicsDir : config_dir ;
	/// Valid dir names must end with a slash.
	if( ! tmp_dir.empty() ) {
		char termin = tmp_dir[ tmp_dir.size() - 1 ];
		if( ( termin != '/' ) && ( termin != '\\' ) ) tmp_dir += '/';
	}
	return tmp_dir ;
}

/// This sets the directory where images are accessed by default.
/// Receives a file name, not a directory name.
static void default_dir_set( std::string & config_dir, const std::string & fil_name )
{
	char * fil_nam_copy = strdup( fil_name.c_str() );
	/// dirname() is a POSIX function.
	const char * dir_nam = dirname( fil_nam_copy );
	config_dir = dir_nam + std::string("/");
	LOG_INFO("Setting default dir to %s", dir_nam );
	free( fil_nam_copy );
}

/// Adds the file name to log to the adif file.
static void qso_notes( const char * direction, const std::string & file_name )
{
	if( progdefaults.WEFAX_AdifLog == false ) {
		return ;
	}
	const std::string tmp_notes = direction + file_name ;
	wefax_serviceme->qso_rec().putField( NOTES, tmp_notes.c_str() );
}

static void wefax_cb_pic_rx_save( Fl_Widget *, void *)
{
	ENSURE_THREAD(FLMAIN_TID);
	const char ffilter[] = "Portable Network Graphics\t*.png\n";
	std::string dfname = default_dir_get( progdefaults.wefax_save_dir );
	dfname.append( wefax_serviceme->suggested_filename()  );

	const char *file_name = FSEL::saveas(_("Save image as:"), ffilter, dfname.c_str(), NULL);
	/// Beware that no extra comments are saved here.
	if (file_name) {
		wefax_pic_rx_picture->save_png(file_name,progdefaults.WEFAX_SaveMonochrome);
		qso_notes( "RX:", file_name );
		wefax_serviceme->qso_rec_save();
		/// Next time, image will be saved at the same place.
		default_dir_set( progdefaults.wefax_save_dir, file_name );
		add_to_files_list( file_name );
	}
}

/// Beware, might be called by another thread. Called by the GUI
/// or when APT start is detected.
void wefax_pic::skip_rx_apt(void)
{
	ENSURE_THREAD(FLMAIN_TID);

	FL_LOCK_D();
	wefax_btn_rx_abort->hide();
	wefax_btn_rx_skip_apt->hide();
	wefax_btn_rx_skip_phasing->show();

	wefax_round_rx_noise_removal->hide();
	wefax_round_rx_binary->hide();
	wefax_spinner_rx_binary->hide();
	FL_UNLOCK_D();
}

/// Called when the user clicks "Skip APT"
static void wefax_cb_pic_rx_skip_apt( Fl_Widget *, void *)
{
	if (wefax_serviceme != active_modem) return;
	ENSURE_THREAD(FLMAIN_TID);
	wefax_serviceme->skip_apt();
}

/// Called when clicking "Skip phasing" or by wefax.cxx
/// when end of phasing is detected. Beware, might be called by another thread.
void wefax_pic::skip_rx_phasing(bool auto_center)
{
	ENSURE_THREAD(FLMAIN_TID);
	FL_LOCK_D();
	/// Theoretically, this widget should already be hidden, but sometimes
	/// it seems that a call to skip_apt is lost... ?
	wefax_btn_rx_abort->show();
	wefax_btn_rx_skip_apt->hide();
	wefax_btn_rx_skip_phasing->hide();

	wefax_round_rx_noise_removal->show();
	if( progdefaults.WEFAX_SaveMonochrome ) {
		wefax_round_rx_binary->show();
		wefax_spinner_rx_binary->show();
	}
	wefax_out_rx_row_num->show();
	wefax_out_rx_width->show();
	wefax_cnt_rx_ratio->show();
	wefax_slider_rx_center->show();
	wefax_round_rx_auto_center->show();

	update_auto_center(auto_center);

	FL_UNLOCK_D();
}

/// Called when clicking "Skip phasing".
static void wefax_cb_pic_rx_skip_phasing( Fl_Widget *w, void *)
{
	if (wefax_serviceme != active_modem) return;
	wefax_serviceme->skip_phasing(true);
}

static void wefax_cb_pic_rx_noise_removal( Fl_Widget *, void *)
{
	if (wefax_serviceme != active_modem) return;
	ENSURE_THREAD(FLMAIN_TID);

	char rndVal = wefax_round_rx_noise_removal->value();
	noise_removal = rndVal ? true : false;
}

static void wefax_cb_pic_rx_binary( Fl_Widget *, void *)
{
	if (wefax_serviceme != active_modem) return;
	ENSURE_THREAD(FLMAIN_TID);

	char rndVal = wefax_round_rx_binary->value();
	if( rndVal ) {
		wefax_pic_rx_picture->set_binary( true );
		wefax_spinner_rx_binary->activate();
	} else {
		wefax_pic_rx_picture->set_binary( false );
		wefax_spinner_rx_binary->deactivate();
	}
}

static void wefax_cb_pic_rx_bin_threshold( Fl_Widget *, void *)
{
	if (wefax_serviceme != active_modem) return;
	ENSURE_THREAD(FLMAIN_TID);

	int rndVal = wefax_spinner_rx_binary->value();
	wefax_pic_rx_picture->set_binary_threshold( rndVal );
}

/// Sets the reception filter: The change should be visible.
static void wefax_cb_rx_set_filter( Fl_Widget *, void * )
{
	if (wefax_serviceme != active_modem) return;
	int ix_filter = wefax_choice_rx_filter->value();
	/// Saved in the configuration.
	progdefaults.wefax_filter = ix_filter ;
	wefax_serviceme->set_rx_filter(ix_filter);
}

static void wefax_cb_choice_rx_lpm( Fl_Widget *, void * )
{
	if (wefax_serviceme != active_modem) return;
	int the_new_lpm = get_choice_lpm_value( wefax_choice_rx_lpm );

	wefax_serviceme->set_lpm( the_new_lpm );
}

static void wefax_cb_pic_ratio( Fl_Widget *, void * )
{
	if (wefax_serviceme != active_modem) return;
	ENSURE_THREAD(FLMAIN_TID);
	double ratio_percent = wefax_cnt_rx_ratio->value();
	double current_ratio = slant_factor_with_ratio( ratio_percent );
	wefax_pic_rx_picture->stretch( current_ratio );
	rx_slant_ratio = ratio_percent ;

	/// And we update the configuration structure.
	progdefaults.wefax_slant = rx_slant_ratio ;

	/// Will prompt for saving configuration when exiting.
	progdefaults.changed = true;
}

/// Possible zooms. The value is processed by class picture.
static const struct {
	int          m_value ;
	const char * m_label ;
} all_zooms[] = {
	{ -3,  "25%" },
	{ -2,  "33%" },
	{ -1,  "50%" },
	{  0, "100%" },
	{  1, "200%" },
	{  2, "300%" },
	{  3, "400%" },
};

// Index in all_zooms.
static const int idx_default_zoom = 2 ;

static int zoom_nb = sizeof(all_zooms) / sizeof(all_zooms[0]);

static void wefax_cb_pic_zoom( Fl_Widget * wefax_choice_zoom, void * )
{
	if (wefax_serviceme != active_modem) return;
	ENSURE_THREAD(FLMAIN_TID);
	int idx_zoom = dynamic_cast<Fl_Choice *>(wefax_choice_zoom)->value();
	if( ( idx_zoom < 0 ) || ( idx_zoom >= zoom_nb ) ) {
		LOG_WARN( "Invalid zoom index=%d", idx_zoom );
		idx_zoom = idx_default_zoom ;
	}

	/// Not very elegant but OK if two possibilities only.
	if( wefax_choice_zoom == wefax_choice_rx_zoom )
	{
		wefax_pic_rx_picture->set_zoom( all_zooms[ idx_zoom ].m_value );
		wefax_pic_rx_win->redraw();
	}
	else if( wefax_choice_zoom == wefax_choice_tx_zoom )
	{
		wefax_pic_tx_picture->set_zoom( all_zooms[ idx_zoom ].m_value );
		wefax_pic_tx_win->redraw();
	}
	else
	{
		LOG_ERROR("Inconsistent possibility");
	}
}

static Fl_Choice * wefax_create_zoom(int wid_off_up, int hei_off_up, int wid_btn_curr, int height_btn)
{
	ENSURE_THREAD(FLMAIN_TID);
	static const char * title_zoom = "Zoom" ;
	Fl_Choice * wefax_choice_zoom = new Fl_Choice(wid_off_up, hei_off_up, wid_btn_curr, height_btn, _(title_zoom));
	wefax_choice_zoom->callback(wefax_cb_pic_zoom, 0);
	for( int ix_zoom = 0; ix_zoom < zoom_nb ; ++ix_zoom ) {
		wefax_choice_zoom->add( all_zooms[ ix_zoom ].m_label );
	};
	wefax_choice_zoom->value(idx_default_zoom);
	wefax_choice_zoom->tooltip(_("Window zoom"));
	wefax_choice_zoom->align(FL_ALIGN_LEFT);
	return wefax_choice_zoom ;
}

void wefax_pic::abort_rx_viewer(void)
{
	if (wefax_serviceme != active_modem) return;
	ENSURE_THREAD(FLMAIN_TID);
	put_status("");

	FL_LOCK_D();
	/// Maybe the image is too high, we make it shorter.
	wefax_pic_rx_picture->resize_height( curr_pix_h_default, true );

	/// Now returns to the top of the image, and refresh the scrolling.
	wefax_pic_rx_picture->position( wefax_pic_rx_picture->x(), 0 );
	wefax_pic_rx_scroll->redraw();

	curr_pix_height = curr_pix_h_default ;
	rx_last_filtered_row = 0;
	center_val_prev = 0 ;
	update_auto_center(false);
	wefax_slider_rx_center->value(0.0);

	wefax_btn_rx_abort->hide();
	wefax_btn_rx_skip_apt->show();
	wefax_btn_rx_skip_phasing->hide();

	wefax_round_rx_noise_removal->hide();
	wefax_round_rx_binary->hide();
	wefax_spinner_rx_binary->hide();

	wefax_out_rx_row_num->hide();
	wefax_out_rx_row_num->value("");
	wefax_out_rx_width->hide();
	wefax_cnt_rx_ratio->hide();
	wefax_cnt_rx_ratio->value();
	wefax_slider_rx_center->hide();
	wefax_round_rx_auto_center->hide();

	/// Back to the first line before reading next image.
	wefax_pic_rx_scroll->position( 0, 0 );
	FL_UNLOCK_D();
}

/// This must be called within REQ or REQ_SYNC to avoid a segfault.
void wefax_pic::power( double start, double phase, double image, double black, double stop )
{
	if (wefax_serviceme != active_modem) return;
	ENSURE_THREAD(FLMAIN_TID);

	static bool init_done = false ;

	/// This reduces memory reallocation.
	if( init_done ) {
		/// TODO: Keep the latest value and update only if they really changed.
		/// This should save CPU.
		wefax_chart_rx_power->replace(1,start, "start", FL_BLUE);
		wefax_chart_rx_power->replace(2,phase, "phase", FL_RED);
		wefax_chart_rx_power->replace(3,image, "image", FL_GREEN);
		wefax_chart_rx_power->replace(4,black, "black", FL_YELLOW);
		wefax_chart_rx_power->replace(5,stop,  "stop",  FL_MAGENTA);
	} else {
		// First time only.
		init_done = true ;
		wefax_chart_rx_power->autosize(1);
		wefax_chart_rx_power->add(start, "start", FL_BLUE);
		wefax_chart_rx_power->add(phase, "phase", FL_RED);
		wefax_chart_rx_power->add(image, "image", FL_GREEN);
		wefax_chart_rx_power->add(black, "black", FL_YELLOW);
		wefax_chart_rx_power->add(stop,  "stop",  FL_MAGENTA);
	}
}

/// The resizing is different from the base class.
class picture_scroll : public picture
{
	public:
	/// Background color is gray.
	picture_scroll(int X, int Y, int W, int H) :
		picture (X, Y, W, H, 255)  {};

	virtual ~picture_scroll() {};

	/// picture::resize destroys the image, we do not want that when displaying.
	virtual void resize(int x, int y, int w, int h)
	{
		FL_LOCK_D();
		LOG_DEBUG("resize: %d %d %d %d", x, y, w, h );
		Fl_Widget::resize( x, y, w, h );
		FL_UNLOCK_D();
	}

	/// This must not process slant this way, so inhibits picture::handle.
	virtual int handle(int event)
	{
		return 0 ;
	}
};

static void wefax_load_image(const char * fil_name);

static void wefax_cb_browse_rx_events( Fl_Widget *, void * )
{
	if (wefax_serviceme != active_modem) return;
	ENSURE_THREAD(FLMAIN_TID);

	int idx_file = wefax_browse_rx_events->value();
	if( idx_file != 0 )
	{
		/// The file name with its directory is stored as data.
		const char * filnam = (const char *)wefax_browse_rx_events->data( idx_file );
		if(filnam == NULL)
		{
			LOG_INFO("Cannot find index=%d nb=%d", idx_file, wefax_browse_rx_events->size() );
			return ;
		}
		/// The tx window might be hidden.
		progdefaults.WEFAX_HideTx = false ;
		wefax_pic_show_tx();
		wefax_load_image(filnam);
	}
}

static const int extra_win_wid = 800 ;

void wefax_pic::create_rx_viewer(int pos_x, int pos_y,int win_wid, int hei_win)
{
	rx_slant_ratio = progdefaults.wefax_slant ;

	FL_LOCK_D();

	int wid_btn_margin = 1 ;

	if( progdefaults.WEFAX_EmbeddedGui )
	{
		wefax_pic_rx_win = new Fl_Double_Window(pos_x, pos_y, win_wid, hei_win);
	}
	else
	{
		Fl_Double_Window * tmpWin = new Fl_Double_Window(win_wid, hei_win, "Fax reception" );
		wefax_pic_rx_win = tmpWin ;
	}
	wefax_pic_rx_win->color(
			fl_rgb_color(
				progdefaults.RxColor.R,
				progdefaults.RxColor.G,
				progdefaults.RxColor.B),
			progdefaults.RxTxSelectcolor);
	wefax_pic_rx_win->align(FL_ALIGN_CLIP);

	int wid_img = win_wid - 15 ;

	/// A bit wider so that it does not scroll at the beginning.
	int height_btn = 20 ;
	int height_margin = 6 ;
	int margin_top_bottom = 2 ;
	int hei_scroll = hei_win - height_margin - 2 * height_btn - 2 * margin_top_bottom -1 ;

	wefax_pic_rx_win->begin();

	wefax_pic_rx_scroll = new Fl_Scroll( 1, 2, win_wid-2, hei_scroll );
	wefax_pic_rx_scroll->type(Fl_Scroll::HORIZONTAL | Fl_Scroll::VERTICAL);
	wefax_pic_rx_scroll->color(
				fl_rgb_color(
					255,
					255,
					255),
				progdefaults.RxTxSelectcolor);
	wefax_pic_rx_scroll->box(FL_ENGRAVED_FRAME);
	wefax_pic_rx_scroll->begin();

	/// It will be resized immediately.
	wefax_pic_rx_picture = new picture_scroll( 0, 0, wid_img, curr_pix_height);
	wefax_pic_rx_picture->align(FL_ALIGN_TOP);
	wefax_pic_rx_picture->set_zoom( all_zooms[ idx_default_zoom ].m_value );

	wefax_pic_rx_scroll->end();

	wefax_pic_rx_win->resizable( wefax_pic_rx_scroll );

	int hei_off_up = hei_win - height_margin - 2 * height_btn - margin_top_bottom ;
	int wid_margin = 2;
	int wid_off_up = wid_margin ;
	int wid_btn_curr = 0 ;

	/// Sets the group at a big size, we will resize the width at the end.
	Fl_Group * tmpGroup = new Fl_Group( 0, hei_off_up, extra_win_wid, height_margin + 2 * height_btn + margin_top_bottom );
	tmpGroup->begin();

	wid_btn_curr = 90 ;
	wefax_btn_rx_save = new Fl_Button(wid_off_up, hei_off_up, wid_btn_curr, height_btn,_("Save..."));
	wefax_btn_rx_save->callback(wefax_cb_pic_rx_save, 0);
	wefax_btn_rx_save->tooltip(_("Save current image in a file."));

	/// Clear, Skipt APT and Skip phasing are at the same place	
	wid_off_up += wid_btn_margin + wid_btn_curr ;
	wid_btn_curr = 75 ;
	wefax_btn_rx_abort = new Fl_Button(wid_off_up, hei_off_up, wid_btn_curr, height_btn, _("Clear"));
	wefax_btn_rx_abort->callback(wefax_cb_pic_rx_abort, 0);
	wefax_btn_rx_abort->tooltip(_("End and clear current image reception."));

	wefax_btn_rx_skip_apt = new Fl_Button(wid_off_up, hei_off_up, wid_btn_curr, height_btn, _("Skip APT"));
	wefax_btn_rx_skip_apt->callback(wefax_cb_pic_rx_skip_apt, 0);
	wefax_btn_rx_skip_apt->tooltip(_("Skip Automatic Picture Transmission step"));
	
	wefax_btn_rx_skip_phasing = new Fl_Button(wid_off_up, hei_off_up, wid_btn_curr, height_btn, _("Skip Phase"));
	wefax_btn_rx_skip_phasing->callback(wefax_cb_pic_rx_skip_phasing, 0);
	wefax_btn_rx_skip_phasing->tooltip(_("Skip phasing step"));

	wid_off_up += wid_btn_margin + wid_btn_curr ;
	wid_btn_curr = 45 ;
	wefax_btn_rx_pause = new Fl_Button(wid_off_up, hei_off_up, wid_btn_curr, height_btn, _("Pause"));
	wefax_btn_rx_pause->callback(wefax_cb_pic_rx_pause, 0);
	wefax_btn_rx_pause->tooltip(_("Pause reception."));

	wefax_btn_rx_resume = new Fl_Button(wid_off_up, hei_off_up, wid_btn_curr, height_btn, _("Resume"));
	wefax_btn_rx_resume->callback(wefax_cb_pic_rx_resume, 0);
	wefax_btn_rx_resume->tooltip(_("Resume reception."));
	wefax_btn_rx_resume->hide();

	wid_off_up += wid_btn_margin + wid_btn_curr ;
	wid_btn_curr = 75 ;
	wefax_round_rx_non_stop = new Fl_Light_Button(wid_off_up, hei_off_up, wid_btn_curr, height_btn, _("Non stop"));
	wefax_round_rx_non_stop->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	wefax_round_rx_non_stop->selection_color(FL_RED);
	wefax_round_rx_non_stop->callback(wefax_cb_pic_rx_manual, 0);
	wefax_round_rx_non_stop->tooltip(_("Continuous reception mode"));

	wid_off_up += wid_btn_margin + wid_btn_curr + 40;
	wid_btn_curr = 58 ;
	wefax_choice_rx_zoom = wefax_create_zoom( wid_off_up, hei_off_up, wid_btn_curr, height_btn );

	static const char * title_filter = "FIR" ;
	wid_off_up += wid_btn_margin + wid_btn_curr + 25;
	wid_btn_curr = 88 ;
	wefax_choice_rx_filter = new Fl_Choice(wid_off_up, hei_off_up, wid_btn_curr, height_btn, title_filter );
	wefax_choice_rx_filter->callback( wefax_cb_rx_set_filter, 0 );
	wefax_choice_rx_filter->align(FL_ALIGN_LEFT);

	int nb_filters = 0 ;
	for( const char ** filter_names = wefax::rx_filters(); *filter_names; ++filter_names, ++nb_filters ) {
		wefax_choice_rx_filter->add( *filter_names );
	};

	/// Sets the default filter based on the configuration.
	int init_filter_idx = progdefaults.wefax_filter ;
	if( ( init_filter_idx < 0 ) || ( init_filter_idx >= nb_filters ) ) {
		LOG_WARN("Invalid config filter index:%d", init_filter_idx );
		/// It is impossible to have no filters at all.
		init_filter_idx = 0 ;
	}
	wefax_choice_rx_filter->value(init_filter_idx);
	wefax_choice_rx_filter->tooltip(_("Set the reception filter."));

	wid_off_up += wid_btn_margin + wid_btn_curr ;

	int hei_off_low = hei_off_up + height_btn + height_margin ;
	int wid_off_low = wid_margin ;

	/// fl_width does not take into account the different size of letters.
	wid_off_low += 28 ;
	wid_btn_curr = 53 ;
	wefax_choice_rx_lpm = make_lpm_choice( wid_off_low, hei_off_low, wid_btn_curr, height_btn );
	wefax_choice_rx_lpm->callback(wefax_cb_choice_rx_lpm);

	/// SkipApt and SkipPhasing are never displayed at the same time then the following widgets.
	/// Therefore we go back to the previous offset.
	wid_off_low += 4 * wid_btn_margin + wid_btn_curr ;
	wid_btn_curr = 35 ;
	wefax_out_rx_row_num = new Fl_Output(wid_off_low, hei_off_low, wid_btn_curr, height_btn);
	wefax_out_rx_row_num->align(FL_ALIGN_LEFT);
	wefax_out_rx_row_num->tooltip(_("Fax line number being read. Image is saved when reaching max lines."));

	wid_off_low += wid_btn_margin + wid_btn_curr + 10;
	wid_btn_curr = 37 ;
	wefax_out_rx_width = new Fl_Output(wid_off_low, hei_off_low, wid_btn_curr, height_btn, "x");
	wefax_out_rx_width->align(FL_ALIGN_LEFT);
	wefax_out_rx_width->value("n/a");
	wefax_out_rx_width->tooltip(_("Image width, in pixels."));

	static const char * title_lpm = _("Slant");
	wid_off_low += wid_btn_margin + wid_btn_curr + 37;
	wid_btn_curr = 62 ;
	wefax_cnt_rx_ratio = new Fl_Counter(wid_off_low, hei_off_low, wid_btn_curr, height_btn, title_lpm);
	wefax_cnt_rx_ratio->align(FL_ALIGN_LEFT);
	wefax_cnt_rx_ratio->type(FL_SIMPLE_COUNTER);
	wefax_cnt_rx_ratio->callback(wefax_cb_pic_ratio, 0);
	wefax_cnt_rx_ratio->tooltip(_("Adjust image slant to correct soundcard clock error."));

	static const char * title_center = _("Align");
	wid_off_low += wid_btn_curr + 37;
	wid_btn_curr = 45 ;
	wefax_slider_rx_center = new Fl_Slider(wid_off_low, hei_off_low, wid_btn_curr, height_btn, title_center);
	wefax_slider_rx_center->type(FL_HORIZONTAL);
	wefax_slider_rx_center->align(FL_ALIGN_LEFT);
	/// The range is set when the image size in pixels is known.
	wefax_slider_rx_center->step(1.0);
	wefax_slider_rx_center->callback(wefax_cb_pic_rx_center, 0);
	wefax_slider_rx_center->tooltip(_("Align image horizontally."));
	center_val_prev = 0 ;

	wid_off_low += wid_btn_curr ;
	wid_btn_curr = 16 ;
	wefax_round_rx_auto_center = new Fl_Light_Button(wid_off_low, hei_off_low, wid_btn_curr, height_btn );
	wefax_round_rx_auto_center->selection_color(FL_GREEN);
	wefax_round_rx_auto_center->callback(wefax_cb_pic_rx_auto_center, 0);
	wefax_round_rx_auto_center->tooltip(_("Enable automatic image centering"));
	update_auto_center(false);

	wid_off_low += 2 * wid_btn_margin + wid_btn_curr ;
	wid_btn_curr = 53 ;
	wefax_round_rx_noise_removal = new Fl_Light_Button(wid_off_low, hei_off_low, wid_btn_curr, height_btn, _("Noise"));
	wefax_round_rx_noise_removal->callback(wefax_cb_pic_rx_noise_removal, 0);
	wefax_round_rx_noise_removal->tooltip(_("Removes noise when ticked"));
	wefax_round_rx_noise_removal->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	wefax_round_rx_noise_removal->selection_color(FL_RED);

	wid_off_low += 2 * wid_btn_margin + wid_btn_curr ;
	wid_btn_curr = 38 ;
	wefax_round_rx_binary = new Fl_Light_Button(wid_off_low, hei_off_low, wid_btn_curr, height_btn, _("Bin"));
	wefax_round_rx_binary->callback(wefax_cb_pic_rx_binary, 0);
	wefax_round_rx_binary->tooltip(_("Binary image when ticked"));
	wefax_round_rx_binary->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	wefax_round_rx_binary->selection_color(FL_RED);
	wefax_round_rx_binary->value(0);
	if( false == progdefaults.WEFAX_SaveMonochrome ) {
		wefax_round_rx_binary->hide();
	}

	wid_off_low += wid_btn_curr ;
	wid_btn_curr = 40 ;
	wefax_spinner_rx_binary = new Fl_Spinner( wid_off_low, hei_off_low, wid_btn_curr, height_btn );
	wefax_spinner_rx_binary->callback(wefax_cb_pic_rx_bin_threshold, 0);
	wefax_spinner_rx_binary->tooltip(_("Set binarization level"));
	wefax_spinner_rx_binary->format("%d");
	wefax_spinner_rx_binary->type(FL_INT_INPUT);
	wefax_spinner_rx_binary->range(0.0,255.0);
	wefax_spinner_rx_binary->step(1.0);
	wefax_spinner_rx_binary->value(wefax_pic_rx_picture->get_binary_threshold());
	if( false == progdefaults.WEFAX_SaveMonochrome ) {
		wefax_spinner_rx_binary->hide();
	}
	wefax_spinner_rx_binary->deactivate();


	wid_off_low += wid_btn_curr ;

	int wid_hei_two = height_margin + 2 * height_btn ;

	/// Starts from the longest of the two lines.
	int wid_off_two = std::max( wid_off_low, wid_off_up );

	wid_btn_curr = 78 ;
	wefax_chart_rx_power = new Fl_Chart( wid_off_two, hei_off_up, wid_btn_curr, wid_hei_two );
	wefax_chart_rx_power->tooltip(_("Power/noise for significant frequencies"));
	wefax_chart_rx_power->type(FL_HORBAR_CHART);

	wid_off_two += wid_btn_margin + wid_btn_curr ;
	wid_btn_curr = std::max( extra_win_wid - wid_off_two - wid_margin, 200 ) ;

	wefax_browse_rx_events = new Fl_Select_Browser(wid_off_two, hei_off_up, wid_btn_curr+2, wid_hei_two );
	wefax_browse_rx_events->callback(wefax_cb_browse_rx_events, 0);
	// static std::string tooltip_rx_events ;
	std::string tooltip_rx_events = _("Files saved in ") + default_dir_get( progdefaults.wefax_save_dir );
	wefax_browse_rx_events->tooltip( tooltip_rx_events.c_str() );
	/// TODO: The horizontal slider should not be always displayed.
	wefax_browse_rx_events->has_scrollbar(Fl_Browser::VERTICAL_ALWAYS | Fl_Browser::HORIZONTAL);

	tmpGroup->resizable(wefax_browse_rx_events);
	tmpGroup->end();
	tmpGroup->resize( 0, hei_off_up, win_wid-2, height_margin + 2 * height_btn + margin_top_bottom );

	wefax_pic_rx_win->end();
	wefax_pic_rx_win->init_sizes();
	wefax_pic_rx_win->redraw();

	FL_UNLOCK_D();
}

void wefax_pic::resize_rx_viewer(int wid_img)
{
	ENSURE_THREAD(FLMAIN_TID);
	FL_LOCK_D();

	abort_rx_viewer();

	/// We want to update the picture data structure.
	wefax_pic_rx_picture->picture::resize( 0, 0, wid_img, curr_pix_h_default );

	char buffer_width[20];
	snprintf( buffer_width, sizeof(buffer_width), "%d", wid_img );
	wefax_out_rx_width->value(buffer_width);

	/// This is a number of pixels.
	double range_rx_center = wid_img / 2.0 ;
	wefax_slider_rx_center->range(-range_rx_center, range_rx_center);

	wefax_pic_rx_win->redraw();
	FL_UNLOCK_D();
}

void wefax_pic::set_rx_label(const std::string & win_label)
{
	FL_LOCK_D();

	std::string tmp_label( win_label );
	if( reception_paused ) {
		tmp_label += " paused" ;
	}

	/// This copy seems strange, but otherwise the label is not updated.
	set_win_label(wefax_pic_rx_win, tmp_label);
	FL_UNLOCK_D();
}

void wefax_pic::save_image(const std::string & fil_name, const std::string & extra_comments )
{
	std::string dfname = default_dir_get( progdefaults.wefax_save_dir ) + fil_name ;

	std::stringstream local_comments;
	local_comments << extra_comments ;
	local_comments << "Slant:" << rx_slant_ratio << "\n" ;
	local_comments << "Auto-Center:" << ( global_auto_center ? "On" : "Off" ) << "\n" ;
	wefax_pic_rx_picture->save_png(dfname.c_str(),progdefaults.WEFAX_SaveMonochrome, local_comments.str().c_str());
	add_to_files_list( dfname );
	qso_notes( "RX:", fil_name );
	wefax_serviceme->qso_rec_save();
}

/// Protected by an exclusive mutex.
static std::string wefax_load_image_after_acquire(const char * fil_name)
{
	if (wefax_serviceme != active_modem) return "Not in WEFAX mode";
	ENSURE_THREAD(FLMAIN_TID);

	wefax_serviceme->qso_rec_init();
	qso_notes( "TX:", fil_name );

	clear_image();
	wefax_shared_tx_img = Fl_Shared_Image::get(fil_name);
	if (!wefax_shared_tx_img) {
		std::string err_msg("Cannot call Fl_Shared_Image::get on file:" + std::string(fil_name) );
		LOG_ERROR("%s",err_msg.c_str());
		return err_msg;
	}
	if (wefax_shared_tx_img->count() > 1) { // we only handle rgb images
		std::string err_msg("Handle only RGB images: " + std::string(fil_name)  );
		LOG_ERROR("%s",err_msg.c_str());
		clear_image();
		return err_msg;
	}
	unsigned char * img_data = (unsigned char *)wefax_shared_tx_img->data()[0];
	int img_wid = wefax_shared_tx_img->w();
	int img_hei = wefax_shared_tx_img->h();
	int img_depth = wefax_shared_tx_img->d();
	wefax_xmtimg = new unsigned char [img_wid * img_hei * bytes_per_pix];
	if (img_depth == bytes_per_pix)
		memcpy(wefax_xmtimg, img_data, img_wid*img_hei*bytes_per_pix);
	else if (img_depth == 4) {
		int i, j, k;
		for (i = 0; i < img_wid*img_hei; i++) {
			j = i*bytes_per_pix; k = i*4;
			wefax_xmtimg[j] = img_data[k];
			wefax_xmtimg[j+1] = img_data[k+1];
			wefax_xmtimg[j+2] = img_data[k+2];
		}
	} else if (img_depth == 1) {
		int i, j;
		for (i = 0; i < img_wid*img_hei; i++) {
			j = i * bytes_per_pix;
			wefax_xmtimg[j] = wefax_xmtimg[j+1] = wefax_xmtimg[j+2] = img_data[i];
		}
	} else {
		std::stringstream err_strm ;
		err_strm << "Inconsistent img_depth=" << img_depth << " for " << fil_name ;
		std::string err_msg = err_strm.str();
		LOG_ERROR("%s",err_msg.c_str());
		return err_msg ;
	};

	wefax_pic::tx_viewer_resize(img_wid, img_hei);

	set_win_label(wefax_pic_tx_win, fil_name);

	wefax_pic_tx_box->label(0);

	// load the picture widget with the rgb image
	FL_LOCK_D();
	wefax_pic_tx_picture->show();
	wefax_pic_tx_picture->clear();
	wefax_pic_tx_picture->video(wefax_xmtimg, img_wid * img_hei * bytes_per_pix);

	int tim_color = wefax_serviceme->tx_time( img_wid * img_hei * bytes_per_pix );
	static char wefax_txclr_tooltip[24];
	snprintf( wefax_txclr_tooltip, sizeof(wefax_txclr_tooltip),
		_("Time needed: %dm %ds (Color)"), tim_color/60, tim_color % 60 );
	wefax_btn_tx_send_color->tooltip(wefax_txclr_tooltip);
	if( false ) {
		// No color transmission now because no information this format.
		wefax_btn_tx_send_color->activate();
	}

	int tim_grey = wefax_serviceme->tx_time( img_wid * img_hei );
	static char wefax_txgry_tooltip[24];
	snprintf( wefax_txgry_tooltip, sizeof(wefax_txgry_tooltip),
		_("Time needed: %dm %ds (B/W)"), tim_grey/60, tim_grey % 60 );
	wefax_btn_tx_send_grey->tooltip(wefax_txgry_tooltip);
	wefax_btn_tx_send_grey->activate();

	wefax_choice_tx_zoom->activate();
	wefax_btn_tx_clear->activate();

	FL_UNLOCK_D();
	return std::string();
}

static void wefax_load_image(const char * fil_name)
{
	if (wefax_serviceme != active_modem) return;
	if( false == wefax_image_loaded_in_gui )
	{
		/// So we do not re-acquire the exclusive access to wefax transmission.
		wefax_serviceme->transmit_lock_acquire(fil_name);
		wefax_image_loaded_in_gui = true ;
	}
	wefax_load_image_after_acquire(fil_name);
}

void wefax_pic::set_tx_pic(unsigned char data, int col, int row, bool is_color )
{
	if (wefax_serviceme != active_modem) return;
	ENSURE_THREAD(FLMAIN_TID);
	if( ( col >= wefax_shared_tx_img->w() )
	 || ( row >= wefax_shared_tx_img->h() ) ) {
		LOG_ERROR("invalid col=%d row=%d w=%d h=%d", col, row, wefax_shared_tx_img->w(), wefax_shared_tx_img->h() );
		exit(EXIT_FAILURE);
	}

	int offset = row * wefax_shared_tx_img->w() + col ;

	if (is_color) {
		wefax_pic_tx_picture->pixel( data, offset );
	} else {
		int tripleOffset = bytes_per_pix * offset ;
		wefax_pic_tx_picture->pixel( data, tripleOffset );
		wefax_pic_tx_picture->pixel( data, tripleOffset + 1 );
		wefax_pic_tx_picture->pixel( data, tripleOffset + 2 );
	}

	static int previous_row = -1 ;
	if( row != previous_row )
	{
		previous_row = row ;
		char num_buffer[20];
		snprintf( num_buffer, sizeof(num_buffer), "%d", row );
		wefax_out_tx_row_num->value( num_buffer );
		snprintf( num_buffer, sizeof(num_buffer), "%d", wefax_shared_tx_img->w() );
		wefax_out_tx_col_num->value( num_buffer );
	}
}

static void wefax_cb_pic_tx_load(Fl_Widget *, void *)
{
	const char *fil_name =
		FSEL::select(_("Load image file"), "Portable Network Graphics\t*.png\n"
			"Independent JPEG Group\t*.{jpg,jif,jpeg,jpe}\n"
			"Graphics Interchange Format\t*.gif", 
			default_dir_get( progdefaults.wefax_load_dir ).c_str() );
	if (!fil_name) {
		LOG_WARN( " Cannot FSEL::select" );
		return ;
	};
	/// Next time, image will be saved at the same place.
	default_dir_set( progdefaults.wefax_load_dir, fil_name );
	wefax_load_image(fil_name);
}

/// Called whether color or b/w image.
static void wefax_pic_tx_send_common(
		bool is_color )
{
	ENSURE_THREAD(FLMAIN_TID);
	FL_LOCK_D();

	if (wefax_serviceme != active_modem) return;

	int img_w = wefax_shared_tx_img->w();
	int img_h = wefax_shared_tx_img->h();

	wefax_choice_tx_lpm->hide();
	wefax_btn_tx_send_color->hide();
	wefax_btn_tx_send_grey->hide();
	wefax_btn_tx_load->hide();
	wefax_choice_tx_zoom->hide();
	wefax_btn_tx_clear->hide();
	wefax_btn_tx_close->hide();
	wefax_out_tx_row_num->show();
	wefax_out_tx_col_num->show();
	wefax_btn_tx_send_abort->show();
	wefax_pic_tx_picture->clear();

	wefax_out_tx_row_num->value( "Init" );
	wefax_out_tx_col_num->value( "" );

	wefax_serviceme->set_tx_parameters(
			get_choice_lpm_value( wefax_choice_tx_lpm ),
			wefax_xmtimg,
			is_color,
			img_w,
			img_h );

	FL_UNLOCK_D();
	start_tx();
}

static void wefax_cb_pic_tx_send_color( Fl_Widget * , void *)
{
	wefax_pic_tx_send_common(true);
}

static void wefax_cb_pic_tx_send_grey( Fl_Widget *, void *)
{
	wefax_pic_tx_send_common(false);
}


static void wefax_cb_pic_tx_send_abort( Fl_Widget *, void *)
{
	if (wefax_serviceme != active_modem) return;

	/// Maybe we are not sending an image.
	if( wefax_shared_tx_img ) {
		wefax_serviceme->set_tx_abort_flag();
		// reload the picture widget with the rgb image
		FL_LOCK_D();
		wefax_pic_tx_picture->video(wefax_xmtimg, wefax_shared_tx_img->w() * wefax_shared_tx_img->h() * bytes_per_pix);
		FL_UNLOCK_D();
	}
}

void wefax_pic::restart_tx_viewer(void)
{
	ENSURE_THREAD(FLMAIN_TID);
	wefax_out_tx_row_num->hide();
	wefax_out_tx_col_num->hide();
	wefax_btn_tx_send_abort->hide();
	wefax_btn_tx_load->show();
	wefax_btn_tx_close->show();
	if( wefax_image_loaded_in_gui )
	{
		// If the image was loaded from the GUI.
		wefax_choice_tx_lpm->show();
		wefax_btn_tx_send_color->show();
		wefax_btn_tx_send_grey->show();
		wefax_choice_tx_zoom->show();
		wefax_btn_tx_clear->show();
	}
	else
	{
		/// If the image was loaded and sent from XML-RPC, or no image present.
		wefax_choice_tx_lpm->deactivate();
		wefax_btn_tx_send_color->deactivate();
		wefax_btn_tx_send_grey->deactivate();
		wefax_choice_tx_zoom->deactivate();
		wefax_btn_tx_clear->deactivate();
	}
}

void wefax_pic::create_tx_viewer(int pos_x, int pos_y,int win_wid, int hei_win)
{
	ENSURE_THREAD(FLMAIN_TID);
	FL_LOCK_D();

	int wid_btn_margin = 5 ;
	if( progdefaults.WEFAX_EmbeddedGui )
	{
		wefax_pic_tx_win = new Fl_Double_Window(pos_x, pos_y, win_wid, hei_win );
	}
	else
	{
		Fl_Double_Window * tmpWin = new Fl_Double_Window(win_wid, hei_win, _("Send image"));
		wefax_pic_tx_win = tmpWin;
	}
	wefax_pic_tx_win->color(
			fl_rgb_color(
				progdefaults.TxColor.R,
				progdefaults.TxColor.G,
				progdefaults.TxColor.B),
			progdefaults.RxTxSelectcolor);

	wefax_pic_tx_win->begin();

	wefax_pic_tx_scroll = new Fl_Scroll( 1, 1, win_wid-2, hei_win - 23 );
	wefax_pic_tx_scroll->type(Fl_Scroll::HORIZONTAL | Fl_Scroll::VERTICAL);
	wefax_pic_tx_scroll->color(
				fl_rgb_color(
					255,
					255,
					255),
				progdefaults.RxTxSelectcolor);
	wefax_pic_tx_scroll->box(FL_ENGRAVED_FRAME);
	wefax_pic_tx_scroll->begin();

	/// It will be resized immediately when an image is loaded.
	wefax_pic_tx_picture = new picture_scroll( 0,0,0,0);
	wefax_pic_tx_picture->align(FL_ALIGN_TOP);
	wefax_pic_tx_picture->set_zoom( all_zooms[ idx_default_zoom ].m_value );

	wefax_pic_tx_scroll->end();

	wefax_pic_tx_win->resizable( wefax_pic_tx_scroll );
	wefax_pic_tx_box = new picbox(
			wefax_pic_tx_scroll->x(),
			wefax_pic_tx_scroll->y(),
			wefax_pic_tx_scroll->w(),
			wefax_pic_tx_scroll->h(),
		      _("Loads an image file\nSupported types: PNG, JPEG, BMP"));
	wefax_pic_tx_box->labelfont(FL_HELVETICA_ITALIC);

	static const int last_margin = 21 ;
	static const int y_btn = hei_win - last_margin ;
	static const int hei_tx_btn = 20 ;

	Fl_Group * tmpGroup = new Fl_Group( 0, y_btn, extra_win_wid, last_margin );
	tmpGroup->begin();
	int width_btn = 30;
	int width_offset = 30;

	width_btn = 50 ;
	wefax_choice_tx_lpm = make_lpm_choice( width_offset, y_btn, width_btn, hei_tx_btn );

	width_offset += width_btn + wid_btn_margin ;
	width_btn = 55 ;
	wefax_btn_tx_send_color = new Fl_Button(width_offset, y_btn, width_btn, hei_tx_btn, "Tx Color");
	wefax_btn_tx_send_color->callback(wefax_cb_pic_tx_send_color, 0);
	wefax_btn_tx_send_color->tooltip(_("Starts transmit in color mode"));

	width_offset += width_btn + wid_btn_margin ;
	width_btn = 45 ;
	wefax_btn_tx_send_grey = new Fl_Button(width_offset, y_btn, width_btn, hei_tx_btn, "Tx B/W");
	wefax_btn_tx_send_grey->callback( wefax_cb_pic_tx_send_grey, 0);
	wefax_btn_tx_send_grey->tooltip(_("Starts transmit in gray level mode"));

	width_offset += width_btn + wid_btn_margin ;
	width_btn = 55 ;
	wefax_btn_tx_load = new Fl_Button(width_offset, y_btn, width_btn, hei_tx_btn, _("Load..."));
	wefax_btn_tx_load->callback(wefax_cb_pic_tx_load, 0);
	wefax_btn_tx_load->tooltip(_("Load image to send"));

	width_offset += width_btn + wid_btn_margin + 40 ;
	width_btn = 58 ;
	wefax_choice_tx_zoom = wefax_create_zoom( width_offset, y_btn, width_btn, hei_tx_btn );

	width_offset += width_btn + wid_btn_margin ;
	width_btn = 45 ;
	wefax_btn_tx_clear = new Fl_Button(width_offset, y_btn, width_btn, hei_tx_btn, _("Clear"));
	wefax_btn_tx_clear->callback(wefax_cb_pic_tx_clear, 0);
	wefax_btn_tx_clear->tooltip(_("Clear image to transmit"));

	width_offset += width_btn + wid_btn_margin ;
	width_btn = 45 ;
	wefax_btn_tx_close = new Fl_Button(width_offset, y_btn, width_btn, hei_tx_btn, _("Close"));
	wefax_btn_tx_close->callback(wefax_cb_pic_tx_close, 0);
	wefax_btn_tx_close->tooltip(_("Close transmit window"));

	wefax_out_tx_row_num = new Fl_Output(20, y_btn, 50, hei_tx_btn );
	wefax_out_tx_row_num->align(FL_ALIGN_LEFT);
	wefax_out_tx_row_num->tooltip(_("Fax line number being sent."));

	wefax_out_tx_col_num = new Fl_Output(80, y_btn, 50, hei_tx_btn, "x" );
	wefax_out_tx_col_num->align(FL_ALIGN_LEFT);
	wefax_out_tx_col_num->tooltip(_("Fax column number."));

	wefax_btn_tx_send_abort = new Fl_Button(180, y_btn, 100, hei_tx_btn, _("Abort Transmit") );
	wefax_btn_tx_send_abort->callback(wefax_cb_pic_tx_send_abort, 0);
	wefax_btn_tx_send_abort->tooltip(_("Abort transmission"));

	wefax_out_tx_row_num->hide();
	wefax_out_tx_col_num->hide();
	wefax_btn_tx_send_abort->hide();
	wefax_btn_tx_send_color->deactivate();
	wefax_btn_tx_send_grey->deactivate();
	wefax_choice_tx_zoom->deactivate();
	wefax_btn_tx_clear->deactivate();

	tmpGroup->end();
	wefax_pic_tx_win->end();
	wefax_pic_rx_win->init_sizes();
	wefax_pic_rx_win->redraw();
	FL_UNLOCK_D();
}

void wefax_pic::abort_tx_viewer(void)
{
	wefax_cb_pic_tx_send_abort(NULL,NULL);
	wefax_cb_pic_tx_close(NULL,NULL);
}

void wefax_pic::tx_viewer_resize(int the_width, int the_height)
{
	ENSURE_THREAD(FLMAIN_TID);
	LOG_DEBUG("the_width=%d the_height=%d", the_width, the_height );

	int win_width = the_width < 288 ? 290 : the_width + 4;
	int win_height = the_height < 180 ? 180 : the_height + 30;
	int pic_x = (win_width - the_width) / 2;
	int pic_y =  (win_height - 30 - the_height)/2;
	FL_LOCK_D();

	/// This because it is a picture_scroll object.
	wefax_pic_tx_picture->picture::resize(pic_x, pic_y, the_width, the_height);

	wefax_pic_tx_picture->clear();
	wefax_pic_tx_box->size(win_width, win_height);
	FL_UNLOCK_D();
}

/// TODO: This crashes but should be called.
void wefax_pic::delete_tx_viewer()
{
	ENSURE_THREAD(FLMAIN_TID);
	FL_LOCK_D();
	wefax_pic_tx_win->hide();
	wefax_serviceme = 0;

	/// First delete the Fl_Widget.
	delete wefax_pic_tx_win;
	wefax_pic_tx_win = 0;

	delete [] wefax_xmtimg;
	wefax_xmtimg = 0;
	FL_UNLOCK_D();
}

/// TODO: This crashes.
void wefax_pic::delete_rx_viewer()
{
	ENSURE_THREAD(FLMAIN_TID);
	FL_LOCK_D();
	wefax_pic_rx_win->hide();
	wefax_serviceme = 0;

	/// These objects are deleted with the main window.
	wefax_pic_tx_win = 0;
	wefax_pic_rx_win = 0;
	FL_UNLOCK_D();
}

void wefax_pic::setpicture_link(wefax *me)
{
	wefax_serviceme = me;
}

/// Called by the main menu bar to open explicitely a wefax reception window.
void wefax_pic::cb_mnu_pic_viewer_rx(Fl_Menu_ *, void * ) {
	ENSURE_THREAD(FLMAIN_TID);
	if ( ! wefax_pic_rx_win) {
		LOG_ERROR("wefax_rx_win should be created");
		return ;
	}
	wefax_pic_rx_win->show();
}

/// Called by the main menu bar to open explicitely a wefax transmission window.
void wefax_pic::cb_mnu_pic_viewer_tx(Fl_Menu_ *, void * ) {
	if ( ! wefax_pic_tx_win) {
		LOG_ERROR("wefax_tx_win should be created");
		return ;
	}
	progdefaults.WEFAX_HideTx = ! progdefaults.WEFAX_HideTx ;
	wefax_pic_show_tx();
}

/// Called from XML-RPC thread.
void wefax_pic::send_image( const std::string & fil_nam )
{
	LOG_INFO("Sending %s", fil_nam.c_str() );
	/// Here, transmit_lock_acquire is called by the XML-RPC client.
	std::string err_msg = wefax_load_image_after_acquire( fil_nam.c_str() );
	if( ! err_msg.empty() )
	{
		if (wefax_serviceme == active_modem)
		{
			/// Allows another XML-RPC client or the GUI to send an image.
			wefax_serviceme->transmit_lock_release( err_msg );
		}
		return ;
	}
	wefax_cb_pic_tx_send_grey( NULL, NULL );
	LOG_INFO("Sent %s", fil_nam.c_str() );
}

/// This function is called at two places:
// - When creating the main window.
// - When initializing the fax modem.
void wefax_pic::create_both(bool called_from_fl_digi)
{
	if( wefax_pic_rx_win ) return ;
	ENSURE_THREAD(FLMAIN_TID);

	/// This function can be called from fl_digi.cxx or wefax.cxx.
	if( called_from_fl_digi && progdefaults.WEFAX_EmbeddedGui )
	{
		int rxWinHeight = progdefaults.WEFAX_HideTx ? text_panel->h() : text_panel->h()/2 ;
		wefax_pic::create_rx_viewer(
			text_panel->x() + mvgroup->w(), text_panel->y(),
			text_panel->w() - mvgroup->w(), rxWinHeight );
		wefax_pic::create_tx_viewer(
			text_panel->x() + mvgroup->w(), text_panel->y() + ReceiveText->h(),
			text_panel->w() - mvgroup->w(), text_panel->h() - ReceiveText->h());
	}
	else
	if( ! called_from_fl_digi && ! progdefaults.WEFAX_EmbeddedGui )
	{
		wefax_pic::create_rx_viewer(
			0, 0, extra_win_wid, 400 );
		wefax_pic::create_tx_viewer(
			0, 0, extra_win_wid, 400 );
	}
	/// Explicitely does nothing here if conditions are not met.
}

void wefax_pic::show_both()
{
	ENSURE_THREAD(FLMAIN_TID);
	if( ( NULL == wefax_pic_rx_win ) || ( NULL == wefax_pic_tx_win ) ) {
		return;
	}
	wefax_pic_show_tx();
	wefax_pic_rx_win->show();
}

void wefax_pic::hide_both()
{
	ENSURE_THREAD(FLMAIN_TID);
	if( ( NULL == wefax_pic_rx_win ) || ( NULL == wefax_pic_tx_win ) ) {
		return;
	}
	wefax_pic_rx_win->hide();
	wefax_pic_tx_win->hide();
}
