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
#include <FL/Fl_Round_Button.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Int_Input.H>
#include <FL/Fl_Counter.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Slider.H>
#include <FL/Fl_Browser.H>
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

static Fl_Double_Window	*wefax_pic_rx_win = (Fl_Double_Window *)0;

static Fl_Scroll       *wefax_pic_rx_scroll          = (Fl_Scroll *)0 ;
static picture	       *wefax_pic_rx_picture         = (picture *)0;
static Fl_Button       *wefax_btn_rx_save            = (Fl_Button *)0;
static Fl_Button       *wefax_btn_rx_abort           = (Fl_Button *)0;
static Fl_Button       *wefax_btn_rx_manual          = (Fl_Button *)0;
static Fl_Button       *wefax_btn_rx_pause           = (Fl_Button *)0;
static Fl_Button       *wefax_btn_rx_resume          = (Fl_Button *)0;
static Fl_Round_Button *wefax_round_rx_adif_log      = (Fl_Round_Button *)0;
static Fl_Choice       *wefax_choice_rx_zoom         = (Fl_Choice *)0;
static Fl_Browser      *wefax_browse_rx_events       = (Fl_Browser *)0;
static Fl_Button       *wefax_btn_rx_skip_apt        = (Fl_Button *)0;
static Fl_Button       *wefax_btn_rx_skip_phasing    = (Fl_Button *)0;
static Fl_Int_Input    *wefax_int_rx_max             = (Fl_Int_Input *)0;
static Fl_Output       *wefax_out_rx_row_num         = (Fl_Output *)0;
static Fl_Output       *wefax_out_rx_width           = (Fl_Output *)0;
static Fl_Choice       *wefax_choice_rx_lpm          = (Fl_Choice *)0;
static Fl_Counter      *wefax_cnt_rx_ratio           = (Fl_Counter *)0;
static Fl_Slider       *wefax_slider_rx_center       = (Fl_Slider *)0;
static Fl_Round_Button *wefax_round_rx_auto_center   = (Fl_Round_Button *)0;
static Fl_Chart        *wefax_chart_rx_power         = (Fl_Chart *)0;
static Fl_Choice       *wefax_choice_rx_filter       = (Fl_Choice *)0;

static Fl_Double_Window	*wefax_pic_tx_win = (Fl_Double_Window *)0;

static Fl_Scroll *wefax_pic_tx_scroll           = (Fl_Scroll *)0 ;
static picture   *wefax_pic_tx_picture          = (picture *)0;
static picbox    *wefax_pic_tx_box              = (picbox *)0;
static Fl_Choice *wefax_choice_tx_lpm           = (Fl_Choice *)0;
static Fl_Round_Button *wefax_round_tx_adif_log = (Fl_Round_Button *)0;
static Fl_Button *wefax_btn_tx_send_color       = (Fl_Button *)0;
static Fl_Button *wefax_btn_tx_send_grey        = (Fl_Button *)0;
static Fl_Output *wefax_out_tx_row_num          = (Fl_Output *)0;
static Fl_Button *wefax_btn_tx_send_abort       = (Fl_Button *)0;
static Fl_Button *wefax_btn_tx_load             = (Fl_Button *)0;
static Fl_Button *wefax_btn_tx_clear            = (Fl_Button *)0;
static Fl_Button *wefax_btn_tx_close            = (Fl_Button *)0;

static Fl_Shared_Image *wefax_shared_tx_img = (Fl_Shared_Image *)0;
static unsigned char *wefax_xmtimg = (unsigned char *)0;
static unsigned char *wefax_xmtpicbuff = (unsigned char *)0;

/// This indicates whether an image to send is loaded in the GUI.
/// It allows to acquire twice when re-loading an image without sending.
static bool wefax_image_loaded_in_gui = false ;

/// Used for shifting the received image left and right.
static volatile int center_val_prev = 0 ;

static volatile bool global_auto_center = false ;

static void update_auto_center(bool is_auto_center)
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
static void set_win_label( Fl_Double_Window * wefax_pic, const std::string & lab)
{
	char* label = strdup(lab.c_str());
	wefax_pic->copy_label(label);
	free(label);
	wefax_pic->redraw();
}

/// Called when clearing the image to send.
static void clear_image(void)
{
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
	FL_LOCK_D();
	wefax_image_loaded_in_gui = false ;
	clear_image();
	wefax_pic_tx_picture->clear();
	wefax_pic::restart_tx_viewer();
	/// Now the lock can be acquired by XML-RPC.
	wefax_serviceme->transmit_lock_release( "Cleared" );
	FL_UNLOCK_D();
}

static void wefax_cb_pic_tx_close( Fl_Widget *, void *)
{
	FL_LOCK_D();
	wefax_pic_tx_win->hide();
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

/// Lpm=120 is by far the most common value, therefore used by default.
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
	choice_lpm->tooltip("Set the LPM value");
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

	/// Maybe there is a slant.
	pix_pos = ( double )pix_pos * slant_factor_default() + 0.5 ;

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
		/// NOTE: Might be better to redraw here and not in resize_height.
		wefax_pic_rx_scroll->position( wefax_pic_rx_scroll->xposition(), y_pos );
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

	/// Ths computes the absolute value of the horizontal derivative.
	for( int row_ix = 1; row_ix < row_end ; ++row_ix ) {
		int col_offset = row_ix * img_wid ;
		const unsigned char * row_start = img_start + col_offset ;
		/// This is an estimation of how the image has information or is just uniform.
		int pix_prev = row_start[img_wid - 1];
		for( int col_ix = 0; col_ix < img_wid ; ++col_ix ) {
			int pix_next = row_start[ col_ix ];
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
	pix_pos += center_val_prev ;
	if( pix_pos < 0 ) {
		pix_pos = 0 ;
	}

	static int last_row_number = 0 ;

	                 update_rx_pic_col(data, pix_pos);
	                 update_rx_pic_col(data, pix_pos + 1);
	int row_number = update_rx_pic_col(data, pix_pos + 2);

	/// Prints the row number sometimes only, to save CPU.
	if( ( pix_pos % 1000 ) == 0 ) {
		char row_num_buffer[20];
		snprintf( row_num_buffer, sizeof(row_num_buffer), "%d", row_number );
		wefax_out_rx_row_num->value( row_num_buffer );
	}

	/// Maybe we restarted an image or maybe went back because of recentering.
	if( last_row_number > row_number ) {
		last_row_number = row_number ;
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
		/// that is, shortened.
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
	wefax_browse_rx_events->add( tmp_strm.str().c_str() );
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
	if (wefax_serviceme != active_modem) return;

	/// This will call wefax_pic::abort_rx_viewer
	wefax_serviceme->end_reception();
	wefax_serviceme->set_rx_manual_mode(false);
	wefax_btn_rx_pause->show();
	wefax_btn_rx_resume->hide();
}

static void wefax_cb_pic_rx_manual( Fl_Widget *, void *)
{
	if (wefax_serviceme != active_modem) return;

	wefax_serviceme->set_rx_manual_mode(true);
	wefax_serviceme->skip_apt();
	wefax_serviceme->skip_phasing(true);
}

static void wefax_cb_pic_rx_center( Fl_Widget *, void *)
{
	if (wefax_serviceme != active_modem) return;

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

	char rndVal = wefax_round_rx_auto_center->value();
	update_auto_center(rndVal ? true : false);
}

/// This gets the directory where images are accessed by default.
static const std::string & default_dir_get( const std::string & config_dir )
{
	if( config_dir.empty() ) {
		return PicsDir ;
	} else {
		return config_dir ;
	}
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
	if( wefax_serviceme->get_adif_log() == false ) {
		return ;
	}
	const std::string tmp_notes = direction + file_name ;
	wefax_serviceme->qso_rec().putField( NOTES, tmp_notes.c_str() );
}

static void wefax_cb_pic_rx_save( Fl_Widget *, void *)
{
	const char ffilter[] = "Portable Network Graphics\t*.png\n";
	std::string dfname = default_dir_get( progdefaults.wefax_save_dir );
	dfname.append( wefax_serviceme->suggested_filename()  );

	const char *file_name = FSEL::saveas(
						_("Save image as:"),
						ffilter,
						dfname.c_str());
	/// Beware that no extra comments are saved here.
	if (file_name) {
		wefax_pic_rx_picture->save_png(file_name);
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
	FL_LOCK_D();
	wefax_btn_rx_skip_apt->hide();
	wefax_btn_rx_abort->show();
	wefax_btn_rx_manual->hide();
	wefax_btn_rx_skip_phasing->show();
	FL_UNLOCK_D();
}

/// Called when the user clicks "Skip APT"
static void wefax_cb_pic_rx_skip_apt( Fl_Widget *, void *)
{
	if (wefax_serviceme != active_modem) return;
	wefax_serviceme->skip_apt();
}

/// Called when clicking "Skip phasing" or by wefax.cxx
/// when end of phasing is detected. Beware, might be called by another thread.
void wefax_pic::skip_rx_phasing(bool auto_center)
{
	FL_LOCK_D();
	/// Theoretically, this widget should already be hidden, but sometimes
	/// it seems that a call to skip_apt is lost... ?
	wefax_btn_rx_skip_apt->hide();
	wefax_btn_rx_skip_phasing->hide();
	wefax_int_rx_max->show();
	wefax_out_rx_row_num->show();
	wefax_out_rx_width->show();
	wefax_choice_rx_lpm->show();
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

/// Sets the reception filter: The change should be visible.
static void wefax_cb_rx_set_filter( Fl_Widget *, void * )
{
	if (wefax_serviceme != active_modem) return;
	int ix_filter = wefax_choice_rx_filter->value();
	/// Saved in the configuration.
	progdefaults.wefax_filter = ix_filter ;
	wefax_serviceme->set_rx_filter(ix_filter);
}

void wefax_pic::restore_max_lines(void)
{
	char buf_max_lines[20];
	snprintf( buf_max_lines, sizeof(buf_max_lines), "%d", wefax_serviceme->get_max_lines() );
	wefax_int_rx_max->value( buf_max_lines );
}

static void wefax_cb_pic_max_lines( Fl_Widget *, void * )
{
	if (wefax_serviceme != active_modem) return;
	const char * ptr_val_gui = wefax_int_rx_max->value();
	int max_val_gui ;

	/// The value given by FLTK should be an integer, but better to be sure.
	if( 1 != sscanf( ptr_val_gui, "%d", &max_val_gui ) ) {
		LOG_ERROR( _("Cannot parse: %s"), ptr_val_gui ) ;
		wefax_pic::restore_max_lines();
		return ;
	}

	/// Faxes should not be too small otherwise we will spend all our time
	/// saving automatic image files.
	if( max_val_gui < mini_max_fax_lines ) {
		wefax_pic::restore_max_lines();
		return ;
	}

	wefax_serviceme->set_max_lines( max_val_gui );
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

static void wefax_cb_pic_rx_zoom( Fl_Widget *, void * )
{
	if (wefax_serviceme != active_modem) return;
	int idx_zoom = wefax_choice_rx_zoom->value();
	if( ( idx_zoom < 0 ) || ( idx_zoom >= zoom_nb ) ) {
		LOG_WARN( "Invalid zoom index=%d", idx_zoom );
		idx_zoom = idx_default_zoom ;
	}
	wefax_pic_rx_picture->set_zoom( all_zooms[ idx_zoom ].m_value );
	wefax_pic_rx_win->redraw();
}

/// Called when ticking whether received images are logged to Adif file.
static void wefax_cb_pic_rx_adif_log( Fl_Widget *, void * )
{
	if (wefax_serviceme != active_modem) return;
	char log_rx = wefax_round_rx_adif_log->value();
	wefax_serviceme->set_adif_log( log_rx );
	wefax_round_tx_adif_log->value(log_rx);
}

void wefax_pic::abort_rx_viewer(void)
{
	if (wefax_serviceme != active_modem) return;
	put_status("");

	FL_LOCK_D();
	/// Maybe the image is too high, we make it shorter.
	wefax_pic_rx_picture->resize_height( curr_pix_h_default, true );

	curr_pix_height = curr_pix_h_default ;
	center_val_prev = 0 ;
	update_auto_center(false);
	wefax_slider_rx_center->value(0.0);

	/// Not sure whether lock/unlock is necessary.
	wefax_btn_rx_abort->hide();
	wefax_btn_rx_manual->show();
	wefax_btn_rx_skip_apt->show();
	wefax_btn_rx_skip_phasing->hide();

	wefax_int_rx_max->hide();
	restore_max_lines();

	wefax_out_rx_row_num->hide();
	wefax_out_rx_row_num->value("");
	wefax_out_rx_width->hide();
	wefax_choice_rx_lpm->hide();
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

void wefax_pic::create_rx_viewer(void)
{
	if( wefax_pic_rx_win )
	{
		return ;
	}

	rx_slant_ratio = progdefaults.wefax_slant ;

	int wid_img = 904 ; // This is convenient for a typical screen, and matches half the size of a fax.

	int hei_img = curr_pix_height ;

	/// A bit wider so that it does not scroll at the beginning.
	int wid_scroll = wid_img + 15 ;
	int hei_scroll = 200 ;
	int height_btn = 24 ;
	int height_margin = 5 ;
	int hei_win = hei_scroll + 12 + height_margin + 2 * height_btn ;

	int wid_win = wid_scroll + 0 ;

	FL_LOCK_D();

	wefax_pic_rx_win = new Fl_Double_Window(wid_win, hei_win, "Fax reception" );

	/// Approximate value for the minimum sizes.
	wefax_pic_rx_win->size_range(500, 100, 0, 0 );

	wefax_pic_rx_win->xclass(PACKAGE_NAME);
	wefax_pic_rx_win->begin();

	wefax_pic_rx_scroll = new Fl_Scroll( 0, 0, wid_scroll, hei_scroll );
	wefax_pic_rx_scroll->type(Fl_Scroll::HORIZONTAL | Fl_Scroll::VERTICAL);
	wefax_pic_rx_scroll->begin();

	/// It will be resized immediately.
	wefax_pic_rx_picture = new picture_scroll( 0, 0, wid_img, hei_img);
	wefax_pic_rx_picture->align(FL_ALIGN_TOP);
	wefax_pic_rx_picture->set_zoom( all_zooms[ idx_default_zoom ].m_value );

	wefax_pic_rx_scroll->end();

	wefax_pic_rx_win->resizable( wefax_pic_rx_scroll );

	static const int wid_btn_margin = 3 ;

	int hei_off_up = hei_win - 2 * height_btn - 2 * height_margin ;
	int wid_offset_up = 5 ;
	int wid_btn_curr = 0 ;

	wid_btn_curr = 50 ;
	wefax_btn_rx_save = new Fl_Button(wid_offset_up, hei_off_up, wid_btn_curr, height_btn,_("Save..."));
	wefax_btn_rx_save->callback(wefax_cb_pic_rx_save, 0);
	wefax_btn_rx_save->tooltip("Save current image in a file.");

	wid_offset_up += wid_btn_margin + wid_btn_curr ;
	wid_btn_curr = 50 ;
	wefax_btn_rx_abort = new Fl_Button(wid_offset_up, hei_off_up, wid_btn_curr, height_btn, _("Abort"));
	wefax_btn_rx_abort->callback(wefax_cb_pic_rx_abort, 0);
	wefax_btn_rx_abort->tooltip("End and clear current image reception.");
	wefax_btn_rx_abort->hide();

	/// At the same place and size as abort.
	wefax_btn_rx_manual = new Fl_Button(wid_offset_up, hei_off_up, wid_btn_curr, height_btn, _("Manual"));
	wefax_btn_rx_manual->callback(wefax_cb_pic_rx_manual, 0);
	wefax_btn_rx_manual->tooltip("Fully manual mode. Press abort to quit.");

	wid_offset_up += wid_btn_margin + wid_btn_curr ;
	wid_btn_curr = 50 ;
	wefax_btn_rx_pause = new Fl_Button(wid_offset_up, hei_off_up, wid_btn_curr, height_btn, _("Pause"));
	wefax_btn_rx_pause->callback(wefax_cb_pic_rx_pause, 0);
	wefax_btn_rx_pause->tooltip("Pause reception.");

	wefax_btn_rx_resume = new Fl_Button(wid_offset_up, hei_off_up, wid_btn_curr, height_btn, _("Resume"));
	wefax_btn_rx_resume->callback(wefax_cb_pic_rx_resume, 0);
	wefax_btn_rx_resume->tooltip("Resume reception.");
	wefax_btn_rx_resume->hide();

	static const char * title_adif_log = "Log";
	wid_offset_up += 10 + wid_btn_curr + fl_width( title_adif_log ) ;
	wid_btn_curr = 20 ;
	wefax_round_rx_adif_log = new Fl_Round_Button(wid_offset_up, hei_off_up, wid_btn_curr, height_btn, _(title_adif_log));
	wefax_round_rx_adif_log->callback(wefax_cb_pic_rx_adif_log, 0);
	wefax_round_rx_adif_log->value( wefax_serviceme->get_adif_log() );
	wefax_round_rx_adif_log->tooltip("Log to Adif file");
	wefax_round_rx_adif_log->align(FL_ALIGN_LEFT);

	static const char * title_zoom = "Zoom" ;
	wid_offset_up += 5 + wid_btn_curr + fl_width( title_zoom ) ;
	wid_btn_curr = 60 ;
	wefax_choice_rx_zoom = new Fl_Choice(wid_offset_up, hei_off_up, wid_btn_curr, height_btn, _(title_zoom));
	wefax_choice_rx_zoom->callback(wefax_cb_pic_rx_zoom, 0);
	for( int ix_zoom = 0; ix_zoom < zoom_nb ; ++ix_zoom ) {
		wefax_choice_rx_zoom->add( all_zooms[ ix_zoom ].m_label );
	};
	wefax_choice_rx_zoom->value(idx_default_zoom);
	wefax_choice_rx_zoom->tooltip("Window zoom");
	wefax_choice_rx_zoom->align(FL_ALIGN_LEFT);

	static const char * title_filter = "FIR" ;
	wid_offset_up += wid_btn_margin + wid_btn_curr + fl_width( title_filter ) ;
	wid_btn_curr = 80 ;
	wefax_choice_rx_filter = new Fl_Choice(wid_offset_up, hei_off_up, wid_btn_curr, height_btn, title_filter );
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
	wefax_choice_rx_filter->tooltip("Set the reception filter.");

	/// Skipt APT and Skip phasing are at the same place	
	wid_offset_up += wid_btn_margin + wid_btn_curr ;
	wid_btn_curr = 80 ;
	wefax_btn_rx_skip_apt = new Fl_Button(wid_offset_up, hei_off_up, wid_btn_curr, height_btn, _("Skip APT"));
	wefax_btn_rx_skip_apt->callback(wefax_cb_pic_rx_skip_apt, 0);
	wefax_btn_rx_skip_apt->tooltip("Skip Automatic Picture Transmission step");
	
	// This goes at the same place as "Skip Apt"
	wefax_btn_rx_skip_phasing = new Fl_Button(wid_offset_up, hei_off_up, wid_btn_curr, height_btn, _("Skip Phasing"));
	wefax_btn_rx_skip_phasing->callback(wefax_cb_pic_rx_skip_phasing, 0);
	wefax_btn_rx_skip_phasing->tooltip("Skip phasing step");

	static const char * title_max = "Max row" ;
	wid_offset_up += fl_width( title_max );
	wid_btn_curr = 50 ;
	wefax_int_rx_max = new Fl_Int_Input(wid_offset_up, hei_off_up, wid_btn_curr, height_btn, _(title_max));
	wefax_int_rx_max->align(FL_ALIGN_LEFT);
	wefax_int_rx_max->callback(wefax_cb_pic_max_lines, 0);
	/// This buffer will never change so it does not matter if it is static.
	static char tooltip_max_lines[256];
	snprintf(tooltip_max_lines, sizeof(tooltip_max_lines),
			"Maximum number of lines per image. Minimum value is %d",
			mini_max_fax_lines );
	wefax_int_rx_max->tooltip(tooltip_max_lines);

	wid_offset_up += wid_btn_margin + wid_btn_curr ;

	int hei_off_low = hei_off_up + height_btn + height_margin ;
	int wid_off_low = 5 ;

	/// SkipApt and SkipPhasing are never displayed at the same time then the following widgets.
	/// Therefore we go back to the previous offset.
	static const char * title_row_num = "" ;
	wid_off_low += fl_width( title_row_num );
	wid_btn_curr = 40 ;
	wefax_out_rx_row_num = new Fl_Output(wid_off_low, hei_off_low, wid_btn_curr, height_btn, _(title_row_num));
	wefax_out_rx_row_num->align(FL_ALIGN_LEFT);
	wefax_out_rx_row_num->tooltip("Fax line number being read. Image is saved when reaching max lines.");

	static const char * title_width = "Width" ;
	wid_off_low += wid_btn_margin + wid_btn_curr + fl_width( title_width ) ;
	wid_btn_curr = 35 ;
	wefax_out_rx_width = new Fl_Output(wid_off_low, hei_off_low, wid_btn_curr, height_btn, _(title_width));
	wefax_out_rx_width->align(FL_ALIGN_LEFT);
	wefax_out_rx_width->value("n/a");
	wefax_out_rx_width->tooltip("Image width, in pixels.");

	/// fl_width does not take into account the different size of letters.
	wid_off_low += 3 * wid_btn_margin + wid_btn_curr + fl_width( title_choice_lpm );
	wid_btn_curr = 60 ;
	wefax_choice_rx_lpm = make_lpm_choice( wid_off_low, hei_off_low, wid_btn_curr, height_btn );
	wefax_choice_rx_lpm->callback(wefax_cb_choice_rx_lpm);

	static const char * title_lpm = "Slant" ;
	wid_off_low += wid_btn_margin + wid_btn_curr + fl_width( title_lpm );
	wid_btn_curr = 80 ;
	wefax_cnt_rx_ratio = new Fl_Counter(wid_off_low, hei_off_low, wid_btn_curr, height_btn, _(title_lpm));
	wefax_cnt_rx_ratio->align(FL_ALIGN_LEFT);
	wefax_cnt_rx_ratio->type(FL_SIMPLE_COUNTER);
	wefax_cnt_rx_ratio->callback(wefax_cb_pic_ratio, 0);
	wefax_cnt_rx_ratio->tooltip("Adjust image slant to correct soundcard clock error.");

	static const char * title_center = "Center" ;
	wid_off_low += wid_btn_curr + fl_width( title_center );
	wid_btn_curr = 85 ;
	wefax_slider_rx_center = new Fl_Slider(wid_off_low, hei_off_low, wid_btn_curr, height_btn, _(title_center));
	wefax_slider_rx_center->type(FL_HORIZONTAL);
	wefax_slider_rx_center->align(FL_ALIGN_LEFT);
	/// Multiplied by three because of color image.
	double range_rx_center = bytes_per_pix * (double)wid_img ;
	wefax_slider_rx_center->range(-range_rx_center, range_rx_center);
	wefax_slider_rx_center->step(1.0);
	wefax_slider_rx_center->callback(wefax_cb_pic_rx_center, 0);
	wefax_slider_rx_center->tooltip("Align image horizontally.");
	center_val_prev = 0 ;

	static const char * title_auto_center = "Auto" ;
	wid_off_low += wid_btn_margin + wid_btn_curr + fl_width( title_auto_center );
	wid_btn_curr = 15 ;
	wefax_round_rx_auto_center = new Fl_Round_Button(wid_off_low, hei_off_low, wid_btn_curr, height_btn, _(title_auto_center) );
	wefax_round_rx_auto_center->align(FL_ALIGN_LEFT);
	wefax_round_rx_auto_center->callback(wefax_cb_pic_rx_auto_center, 0);
	wefax_round_rx_auto_center->tooltip("Enable automatic image centering");
	update_auto_center(false);

	wid_off_low += wid_btn_margin + wid_btn_curr ;

	int wid_hei_two = wid_btn_margin + 2 * height_btn ;

	/// Starts from the longest of the two lines.
	int wid_off_two = ( wid_off_low > wid_offset_up ) ? wid_off_low : wid_offset_up ;

	wid_btn_curr = 80 ;
	wefax_chart_rx_power = new Fl_Chart( wid_off_two, hei_off_up, wid_btn_curr, wid_hei_two );
	wefax_chart_rx_power->tooltip("Power/noise for significant frequencies");
	wefax_chart_rx_power->type(FL_HORBAR_CHART);

	wid_off_two += wid_btn_margin + wid_btn_curr ;
	wid_btn_curr = 315;
	wefax_browse_rx_events = new Fl_Browser(wid_off_two, hei_off_up, wid_btn_curr, wid_hei_two );
	static std::string tooltip_rx_events ;
	tooltip_rx_events = "Files saved in " + default_dir_get( progdefaults.wefax_save_dir );
	wefax_browse_rx_events->tooltip( tooltip_rx_events.c_str() );
	wefax_browse_rx_events->has_scrollbar(Fl_Browser::VERTICAL | Fl_Browser::HORIZONTAL);

	wefax_pic_rx_win->end();

	FL_UNLOCK_D();
}

void wefax_pic::resize_rx_viewer(int wid_img)
{
	FL_LOCK_D();

	abort_rx_viewer();

	/// We want to update the picture data structure.
	wefax_pic_rx_picture->picture::resize( 0, 0, wid_img, curr_pix_h_default );

	char buffer_width[20];
	snprintf( buffer_width, sizeof(buffer_width), "%d", wid_img );
	wefax_out_rx_width->value(buffer_width);

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
	local_comments << "Center:" << global_auto_center << "\n" ;
	wefax_pic_rx_picture->save_png(dfname.c_str(), local_comments.str().c_str());
	add_to_files_list( fil_name );
	qso_notes( "RX:", fil_name );
	wefax_serviceme->qso_rec_save();
}

/// Protected by an exclusive mutex.
static std::string wefax_load_image_after_acquire(const char * fil_name)
{
	if (wefax_serviceme != active_modem) return "Not in WEFAX mode";

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
		"Time needed: %dm %ds (Color)", tim_color/60, tim_color % 60 );
	wefax_btn_tx_send_color->tooltip(wefax_txclr_tooltip);
	wefax_btn_tx_send_color->activate();

	int tim_grey = wefax_serviceme->tx_time( img_wid * img_hei );
	static char wefax_txgry_tooltip[24];
	snprintf( wefax_txgry_tooltip, sizeof(wefax_txgry_tooltip),
		"Time needed: %dm %ds (B/W)", tim_grey/60, tim_grey % 60 );
	wefax_btn_tx_send_grey->tooltip(wefax_txgry_tooltip);
	wefax_btn_tx_send_grey->activate();

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

void wefax_pic::set_tx_pic(unsigned char data, int col, int row, int tx_img_col, bool is_color )
{
	if (wefax_serviceme != active_modem) return;
	if( ( col >= tx_img_col )
	 || ( col >= wefax_shared_tx_img->w() )
	 || ( row >= wefax_shared_tx_img->h() ) ) {
		LOG_ERROR("invalid col=%d tx_img_col=%d row=%d", col, tx_img_col, row );
		exit(EXIT_FAILURE);
	}

	int offset = row * tx_img_col + col ;

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
		char row_num_buffer[20];
		snprintf( row_num_buffer, sizeof(row_num_buffer), "%d", row );
		wefax_out_tx_row_num->value( row_num_buffer );
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
		bool is_color,
		int img_w,
		int img_h,
		int xmt_bytes )
{
	FL_LOCK_D();

	wefax_choice_tx_lpm->hide();
	wefax_round_tx_adif_log->hide();
	wefax_btn_tx_send_color->hide();
	wefax_btn_tx_send_grey->hide();
	wefax_btn_tx_load->hide();
	wefax_btn_tx_clear->hide();
	wefax_btn_tx_close->hide();
	wefax_out_tx_row_num->show();
	wefax_btn_tx_send_abort->show();
	wefax_pic_tx_picture->clear();

	wefax_out_tx_row_num->value( "Init" );

	wefax_serviceme->set_tx_parameters(
			get_choice_lpm_value( wefax_choice_tx_lpm ),
			wefax_xmtpicbuff,
			is_color,
			img_w,
			img_h,
			xmt_bytes );

	FL_UNLOCK_D();
	start_tx();
}

static void wefax_cb_pic_tx_send_color( Fl_Widget * , void *)
{
	LOG_DEBUG("Entering" );
	if (wefax_serviceme != active_modem) return;

	int img_wid = wefax_shared_tx_img->w();
	int img_hei = wefax_shared_tx_img->h();
	if (wefax_xmtpicbuff) delete [] wefax_xmtpicbuff;
	wefax_xmtpicbuff = new unsigned char [img_wid*img_hei*bytes_per_pix];

	for (int iy = 0; iy < img_hei; iy++) {
		int rowstart = iy * img_wid * bytes_per_pix;
		for (int rgb = 0; rgb < bytes_per_pix; rgb++) {
			for (int ix = 0; ix < img_wid; ix++) {
				wefax_xmtpicbuff[rowstart + rgb*img_wid + ix]
					= wefax_xmtimg[rowstart + rgb + ix*bytes_per_pix];
			}
		}
	}

	wefax_pic_tx_send_common(true,img_wid,img_hei,img_wid * img_hei * bytes_per_pix);
}

static void wefax_cb_pic_tx_send_grey( Fl_Widget *, void *)
{
	LOG_DEBUG("Entering" );
	if (wefax_serviceme != active_modem) return;

	int img_wid = wefax_shared_tx_img->w();
	int img_hei = wefax_shared_tx_img->h();
	if (wefax_xmtpicbuff) delete [] wefax_xmtpicbuff;
	wefax_xmtpicbuff = new unsigned char [img_wid*img_hei];

	for (int i = 0; i < img_wid*img_hei; i++) {
		wefax_xmtpicbuff[i]
			= ( 31 * wefax_xmtimg[i*bytes_per_pix]
				+ 61 * wefax_xmtimg[i*bytes_per_pix + 1]
				+ 8 * wefax_xmtimg[i*bytes_per_pix + 2])/100;
	}

	wefax_pic_tx_send_common(false,img_wid,img_hei,img_wid * img_hei);
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

/// Called when ticking "log" check box in the transmit window.
static void wefax_cb_pic_tx_adif_log( Fl_Widget *, void *)
{
	if (wefax_serviceme != active_modem) return;
	char log_tx = wefax_round_tx_adif_log->value();
	wefax_serviceme->set_adif_log( log_tx );
	wefax_round_rx_adif_log->value(log_tx);
}

void wefax_pic::restart_tx_viewer(void)
{
	wefax_out_tx_row_num->hide();
	wefax_btn_tx_send_abort->hide();
	wefax_round_tx_adif_log->show();
	wefax_btn_tx_load->show();
	wefax_btn_tx_close->show();
	if( wefax_image_loaded_in_gui )
	{
		// If the image was loaded from the GUI.
		wefax_choice_tx_lpm->show();
		wefax_btn_tx_send_color->show();
		wefax_btn_tx_send_grey->show();
		wefax_btn_tx_clear->show();
	}
	else
	{
		/// If the image was loaded and sent from XML-RPC, or no image present.
		wefax_choice_tx_lpm->deactivate();
		wefax_btn_tx_send_color->deactivate();
		wefax_btn_tx_send_grey->deactivate();
		wefax_btn_tx_clear->deactivate();
	}
}

void wefax_pic::create_tx_viewer(void)
{
	if( wefax_pic_tx_win ) {
		return ;
	}
	FL_LOCK_D();

	int dbl_win_wid = 370 ;
	wefax_pic_tx_win = new Fl_Double_Window(dbl_win_wid, 180, _("Send image"));
	wefax_pic_tx_win->xclass(PACKAGE_NAME);
	wefax_pic_tx_win->begin();

	wefax_pic_tx_scroll = new Fl_Scroll( 2, 2, dbl_win_wid - 10, 150 );
	wefax_pic_tx_scroll->type(Fl_Scroll::HORIZONTAL | Fl_Scroll::VERTICAL);
	wefax_pic_tx_scroll->begin();

	/// It will be resized immediately.
	wefax_pic_tx_picture = new picture_scroll( 0, 0, 100, 100);

	wefax_pic_tx_picture->align(FL_ALIGN_TOP);

	wefax_pic_tx_scroll->end();

	wefax_pic_tx_win->resizable( wefax_pic_tx_scroll );
	wefax_pic_tx_picture->hide();
	wefax_pic_tx_box = new picbox(
			wefax_pic_tx_win->x(),
			wefax_pic_tx_win->y(),
			wefax_pic_tx_win->w(),
			wefax_pic_tx_win->h(),
		      _("Loads an image file\nSupported types: PNG, JPEG, BMP"));
	wefax_pic_tx_box->labelfont(FL_HELVETICA_ITALIC);

	static const int y_btn = 155 ;
	static const int hei_tx_btn = 24 ;

	int width_btn = 0;
	int width_offset = 30;

	width_btn = 70 ;
	wefax_choice_tx_lpm = make_lpm_choice( width_offset, y_btn, width_btn, hei_tx_btn );

	static const char * title_adif_log = "Log";
	width_offset += 5 + width_btn + fl_width( title_adif_log ) ;
	width_btn = 25 ;
	wefax_round_tx_adif_log = new Fl_Round_Button(width_offset, y_btn, width_btn, hei_tx_btn, _(title_adif_log));
	wefax_round_tx_adif_log->callback(wefax_cb_pic_tx_adif_log, 0);
	wefax_round_tx_adif_log->value( wefax_serviceme->get_adif_log() );
	wefax_round_tx_adif_log->tooltip("Log to Adif file");
	wefax_round_tx_adif_log->align(FL_ALIGN_LEFT);

	width_offset += width_btn ;
	width_btn = 50 ;
	wefax_btn_tx_send_color = new Fl_Button(width_offset, y_btn, width_btn, hei_tx_btn, "Tx Color");
	wefax_btn_tx_send_color->callback(wefax_cb_pic_tx_send_color, 0);

	width_offset += width_btn ;
	width_btn = 50 ;
	wefax_btn_tx_send_grey = new Fl_Button(width_offset, y_btn, width_btn, hei_tx_btn, "Tx B/W");
	wefax_btn_tx_send_grey->callback( wefax_cb_pic_tx_send_grey, 0);

	width_offset += width_btn ;
	width_btn = 35 ;
	wefax_btn_tx_load = new Fl_Button(width_offset, y_btn, width_btn, hei_tx_btn, _("Load"));
	wefax_btn_tx_load->callback(wefax_cb_pic_tx_load, 0);
	wefax_btn_tx_load->tooltip("Load image to send");

	width_offset += width_btn ;
	width_btn = 35 ;
	wefax_btn_tx_clear = new Fl_Button(width_offset, y_btn, width_btn, hei_tx_btn, _("Clear"));
	wefax_btn_tx_clear->callback(wefax_cb_pic_tx_clear, 0);

	width_offset += width_btn ;
	width_btn = 35 ;
	wefax_btn_tx_close = new Fl_Button(width_offset, y_btn, width_btn, hei_tx_btn, _("Close"));
	wefax_btn_tx_close->callback(wefax_cb_pic_tx_close, 0);

	static const char * title_row_num = "" ;
	width_offset += fl_width( title_row_num );
	wefax_out_tx_row_num = new Fl_Output(20, y_btn, 100, hei_tx_btn, _(title_row_num));
	wefax_out_tx_row_num->align(FL_ALIGN_LEFT);
	wefax_out_tx_row_num->tooltip("Fax line number being sent.");

	wefax_btn_tx_send_abort = new Fl_Button(180, y_btn, 122, hei_tx_btn, "Abort Xmt");
	wefax_btn_tx_send_abort->callback(wefax_cb_pic_tx_send_abort, 0);
	wefax_btn_tx_send_abort->tooltip("Abort transmission");

	wefax_out_tx_row_num->hide();
	wefax_btn_tx_send_abort->hide();
	wefax_btn_tx_send_color->deactivate();
	wefax_btn_tx_send_grey->deactivate();
	wefax_btn_tx_clear->deactivate();

	wefax_pic_tx_win->end();
	FL_UNLOCK_D();
}

void wefax_pic::abort_tx_viewer(void)
{
	wefax_cb_pic_tx_send_abort(NULL,NULL);
	wefax_cb_pic_tx_close(NULL,NULL);
}

void wefax_pic::tx_viewer_resize(int the_width, int the_height)
{
	LOG_DEBUG("the_width=%d the_height=%d", the_width, the_height );

	int win_width = the_width < 288 ? 290 : the_width + 4;
	int win_height = the_height < 180 ? 180 : the_height + 30;
	int pic_x = (win_width - the_width) / 2;
	int pic_y =  (win_height - 30 - the_height)/2;
	FL_LOCK_D();
	wefax_pic_tx_win->size(win_width, win_height);

	/// This because it is a picture_scroll object.
	wefax_pic_tx_picture->picture::resize(pic_x, pic_y, the_width, the_height);

	wefax_pic_tx_picture->clear();
	wefax_pic_tx_box->size(win_width, win_height);
	FL_UNLOCK_D();
}

void wefax_pic::show_tx_viewer(int the_width, int the_height)
{
	LOG_DEBUG("the_width=%d the_height=%d", the_width, the_height );
	if (wefax_pic_tx_win) {
		wefax_pic_tx_win->show();
		return;
	}

	int win_width = the_width < 288 ? 290 : the_width + 4;
	int win_height = the_height < 180 ? 180 : the_height + 30;
	int pic_x = (win_width - the_width) / 2;
	int pic_y =  2;
	FL_LOCK_D();
	wefax_pic_tx_win->size(win_width, win_height);
	wefax_pic_tx_picture->resize(pic_x, pic_y, the_width, the_height);

	wefax_pic::restart_tx_viewer();
	wefax_pic_tx_win->show();

	FL_UNLOCK_D();
}

/// TODO: This crashes.
void wefax_pic::delete_tx_viewer()
{
	LOG_DEBUG("Entering");
	FL_LOCK_D();
	wefax_pic_tx_win->hide();
	wefax_serviceme = 0;

	/// First delete the Fl_Widget.
	delete wefax_pic_tx_win;
	wefax_pic_tx_win = 0;

	delete [] wefax_xmtimg;
	wefax_xmtimg = 0;
	delete [] wefax_xmtpicbuff;
	wefax_xmtpicbuff = 0;
	FL_UNLOCK_D();
}

/// TODO: This crashes.
void wefax_pic::delete_rx_viewer()
{
	LOG_DEBUG("Entering");
	FL_LOCK_D();
	wefax_pic_rx_win->hide();
	wefax_serviceme = 0;

	/// First delete the Fl_Widget. CRASHES HERE.
	delete wefax_pic_rx_win;

	wefax_pic_rx_win = 0;
/*
	if (wefax_pic_rx_picture) {
		delete wefax_pic_rx_picture;
		wefax_pic_rx_picture = 0;
	}
*/
	FL_UNLOCK_D();
}

void wefax_pic::setpicture_link(wefax *me)
{
	wefax_serviceme = me;
}

void wefax_pic::rx_hide(void)
{
	if( wefax_pic_rx_win ) {
		wefax_pic_rx_win->hide();
	}
}

void wefax_pic::tx_hide(void)
{
	if( wefax_pic_tx_win ) {
		wefax_pic_tx_win->hide();
	}
}

void wefax_pic::cb_mnu_pic_viewer(Fl_Menu_ *, void *) {
	if (wefax_pic_rx_win) {
		wefax_pic_rx_picture->redraw();
		wefax_pic_rx_win->show();
	}
}

/// Called from XML-RPC thread.
void wefax_pic::send_image( const std::string & fil_nam )
{
	LOG_INFO("%s", fil_nam.c_str() );
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
}

