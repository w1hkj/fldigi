//
//    wefax-pic.h  --  Weather Fax modem

#ifndef _WEFAX_PIC_H
#define _WEFAX_PIC_H

class Fl_Menu_ ;

class wefax ;

class wefax_pic
{
	static void create_rx_viewer(int wid_x, int wid_y,int wid_win, int hei_win);
	static void create_tx_viewer(int wid_x, int wid_y,int wid_win, int hei_win);
public:
	static void set_tx_pic(unsigned char data, int col, int row, int tx_img_col, bool is_color );
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

	static void create_both( bool called_from_fl_digi );
	static void show_both();
	static void hide_both();
};

#endif // _WEFAX_PIC_H
