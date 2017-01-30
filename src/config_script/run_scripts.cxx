// ----------------------------------------------------------------------------
// Copyright (C) 2015
//              Robert Stiles
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

#include <cstdlib>
#include <cstdarg>
#include <string>
#include <fstream>
#include <algorithm>
#include <map>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <ctime>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <libgen.h>
#include <ctype.h>
#include <sys/time.h>

#include "config.h"

#include <sys/types.h>

#ifdef __WOE32__
#  ifdef __CYGWIN__
#    include <w32api/windows.h>
#  else
#    include <windows.h>
#  endif
#endif

#include <cstdlib>
#include <cstdarg>
#include <string>
#include <fstream>
#include <algorithm>
#include <map>

#ifndef __WOE32__
#include <sys/wait.h>
#endif

#include "gettext.h"
#include "fl_digi.h"

#include <FL/Fl.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Pixmap.H>
#include <FL/Fl_Image.H>
//#include <FL/Fl_Tile.H>
#include <FL/x.H>
#include <FL/Fl_Help_Dialog.H>
#include <FL/Fl_Progress.H>
#include <FL/Fl_Tooltip.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Multiline_Input.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Pack.H>
#include <FL/filename.H>
#include <FL/fl_ask.H>

#include "waterfall.h"
#include "raster.h"
#include "progress.h"
#include "Panel.h"

#include "main.h"
#include "threads.h"
#include "trx.h"
#if USE_HAMLIB
#include "hamlib.h"
#endif
#include "timeops.h"
#include "rigio.h"
#include "nullmodem.h"
#include "psk.h"
#include "cw.h"
#include "mfsk.h"
#include "wefax.h"
#include "wefax-pic.h"
#include "navtex.h"
#include "mt63.h"
#include "view_rtty.h"
#include "olivia.h"
#include "contestia.h"
#include "thor.h"
#include "dominoex.h"
#include "feld.h"
#include "throb.h"
//#include "pkt.h"
#include "wwv.h"
#include "analysis.h"
#include "fftscan.h"
#include "ssb.h"

#include "smeter.h"
#include "pwrmeter.h"

#include "ascii.h"
#include "globals.h"
#include "misc.h"
#include "FTextRXTX.h"

#include "confdialog.h"
#include "configuration.h"
#include "status.h"

#include "macros.h"
#include "macroedit.h"
#include "logger.h"
#include "lookupcall.h"

#include "font_browser.h"

#include "icons.h"
#include "pixmaps.h"

#include "rigsupport.h"

#include "qrunner.h"

#include "Viewer.h"
#include "soundconf.h"

#include "htmlstrings.h"
#	include "xmlrpc.h"
#if BENCHMARK_MODE
#	include "benchmark.h"
#endif

#include "debug.h"
#include "re.h"
#include "network.h"
#include "spot.h"
#include "dxcc.h"
#include "locator.h"
#include "notify.h"

#include "logbook.h"

#include "rx_extract.h"
#include "speak.h"
#include "flmisc.h"

#include "arq_io.h"
#include "data_io.h"
#include "kmlserver.h"

#include "notifydialog.h"
#include "macroedit.h"
#include "rx_extract.h"
#include "wefax-pic.h"
#include "charsetdistiller.h"
#include "charsetlist.h"
#include "outputencoder.h"
#include "record_loader.h"
#include "record_browse.h"
#include "fileselect.h"

#include "script_parsing.h"
#include "run_script.h"

pthread_mutex_t mutex_script_io = PTHREAD_MUTEX_INITIALIZER;

extern std::string ScriptsDir;

void script_execute(void *);
static void script_execute(const char *filename, bool queue_flag);

/** ********************************************************
 * \brief Template for assigning bool values to various widget types.
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
template <typename widget_type>
static int assign_bool(widget_type * widget, ScriptParsing *sp, SCRIPT_COMMANDS *sc, bool &data)
{
	if(!sp || !sc)
		return script_function_parameter_error;

	bool value = 0;

	int error = sp->check_bool(sc->args[0], value);

	if(error != script_no_errors)
		return error;

	if(!widget)
		return script_no_errors;

	widget->value(value);
	widget->do_callback();

	data = value;

	return script_no_errors;
}

/** ********************************************************
 * \brief Template for assigning bool values to various widget types.
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
template <typename widget_type>
static int assign_bool(widget_type * widget, ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	bool data = false;
	return assign_bool(widget, sp, sc, data);
}

/** ********************************************************
 * \brief Template for assigning integer values to various widget types.
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
template <typename widget_type>
static int assign_integer(widget_type * widget, ScriptParsing *sp, SCRIPT_COMMANDS *sc, int &data)
{
	if(!sp || !sc)
		return script_function_parameter_error;

	std::string str_data = "";

	if(sc->argc > 0)
		str_data.assign(sc->args[0]);

	if(str_data.empty())
		return script_invalid_parameter;

	int value = 0;
	int cnt = sscanf(str_data.c_str(), "%d", &value);

	if(cnt < 1)
		return script_invalid_parameter;

	if(!widget)
		return script_no_errors;

	int min = (int) widget->minimum();
	int max = (int) widget->maximum();

	if((value < min) || (value > max))
		return script_invalid_parameter;

	widget->value(value);
	widget->do_callback();

	data = value;

	return script_no_errors;
}

/** ********************************************************
 * \brief Template for assigning integer values to various widget types.
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
template <typename widget_type>
static int assign_integer(widget_type * widget, ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	int data = 0.0;
	return assign_integer(widget, sp, sc, data);
}

/** ********************************************************
 * \brief Template for assigning double values to various widget types.
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
template <typename widget_type>
static int assign_double(widget_type * widget, ScriptParsing *sp, SCRIPT_COMMANDS *sc, double &data)
{
	if(!sp || !sc)
		return script_function_parameter_error;

	std::string str_data = "";

	if(sc->argc > 0)
		str_data.assign(sc->args[0]);

	if(str_data.empty())
		return script_invalid_parameter;

	double value = 0;
	int cnt = sscanf(str_data.c_str(), "%lf", &value);

	if(cnt < 1)
		return script_invalid_parameter;

	if(!widget)
		return script_no_errors;

	double min = (double) widget->minimum();
	double max = (double) widget->maximum();

	if((value < min) || (value > max))
		return script_invalid_parameter;

	widget->value(value);
	widget->do_callback();

	data = value;

	return script_no_errors;
}

/** ********************************************************
 * \brief Template for assigning double values to various widget types.
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
template <typename widget_type>
static int assign_double(widget_type * widget, ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	double data = 0.0;
	return assign_double(widget, sp, sc, data);
}

/** ********************************************************
 * \brief Template for assigning string values to various widget types.
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
template <typename widget_type>
static int assign_string(widget_type * widget, ScriptParsing *sp, SCRIPT_COMMANDS *sc, std::string &data)
{
	if(!sp || !sc)
		return script_function_parameter_error;

	std::string str_data = "";

	if(sc->argc > 0)
		str_data.assign(sc->args[0]);

	if(str_data.empty())
		return script_invalid_parameter;

	if(!widget)
		return script_no_errors;

	widget->value(str_data.c_str());
	widget->do_callback();

	data.assign(str_data);

	return script_no_errors;
}

/** ********************************************************
 * \brief Template for assigning string values to various widget types.
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
template <typename widget_type>
static int assign_string(widget_type * widget, ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	std::string data = "";
	return assign_string(widget, sp, sc, data);
}

/** ********************************************************
 * \brief Template for assigning index values to various widget types.
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
template <typename widget_type>
static int assign_index(widget_type * widget, ScriptParsing *sp, SCRIPT_COMMANDS *sc, int &data)
{
	if(!sp || !sc)
		return script_function_parameter_error;

	std::string str_data = "";

	if(sc->argc > 0)
		str_data.assign(sc->args[0]);

	if(str_data.empty())
		return script_invalid_parameter;

	if(!widget)
		return script_no_errors;

	int index = widget->find_index(str_data.c_str());
	if(index < 0)
		return script_invalid_parameter;

	widget->index(index);
	widget->do_callback();

	data = index;

	return script_no_errors;
}

/** ********************************************************
 * \brief Template for assigning index values to various widget types.
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
template <typename widget_type>
static int assign_index(widget_type * widget, ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	int data = 0;
	return assign_index(widget, sp, sc, data);
}


/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_rsid_notify(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_bool(chkRSidNotifyOnly, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_rsid_search_bp(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_bool(chkRSidWideSearch, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_rsid_mark_prev(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_bool(chkRSidMark, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_rsid_detector(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_bool(chkRSidAutoDisable, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_rsid_alert_dialog(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_bool(chkRSidShowAlert, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_rsid_tx_freq_lock(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_bool(chkRetainFreqLock, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_rsid_freq_change(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_bool(chkDisableFreqChange, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_rsid_allow_errors(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_index(listbox_rsid_errors, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_rsid_sql_open(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_integer(sldrRSIDsquelch, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_rsid_pretone(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_double(val_pretone, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_rsid_char_per_row(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_integer(sldrVideowidth, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_rsid_end_xmt_id(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_bool(btn_post_rsid, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_video_tx_id_mode(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_bool(btnsendid, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_video_tx_vid_txt(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_bool(btnsendvideotext, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_video_txt_input(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_string(valVideotext, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_video_small_font(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_bool(chkID_SMALL, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_video_500hz(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_bool(btn_vidlimit, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_video_width_limit(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_bool(btn_vidmodelimit, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_cw_callsign(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_bool(btnCWID, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_cw_speed(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_integer(sldrCWIDwpm, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_misc_nbems_state(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_bool(chkAutoExtract, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_misc_nbems_open_flmsg(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_bool(chk_open_flmsg, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_misc_nbems_open_msg(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_bool(chk_open_wrap_folder, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_misc_nbems_open_brwsr(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_bool(chk_open_flmsg_print, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_misc_nbems_flmsg_path(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_string(txt_flmsg_pathname, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_misc_nbems_timeout(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_integer(sldr_extract_timeout, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_rig_freq(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	// Not suitable for assign_xxxx
	if(!sp || !sc)
		return script_function_parameter_error;

	std::string str_data = "";

	if(sc->argc > 0)
		str_data.assign(sc->args[0]);

	if(str_data.empty())
		return script_invalid_parameter;

	if(!qsoFreqDisp)
		return script_no_errors;

	double value = 0;
	double max = (double) qsoFreqDisp->maximum();

	int cnt = sscanf(str_data.c_str(), "%lf", &value);

	if(cnt < 1 || value < 0.0 || value > max)
		return script_invalid_parameter;

	qsy((long int) value, active_modem ? active_modem->get_freq() : 1500);

	return script_no_errors;
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_rig_mode(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_index(qso_opMODE, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_wf_hz_offset(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	// Not suitable for assign_xxxx
	if(!sp || !sc)
		return script_function_parameter_error;

	std::string str_data = "";

	if(sc->argc > 0)
		str_data.assign(sc->args[0]);

	if(str_data.empty())
		return script_invalid_parameter;

	if(!cntrWfwidth)
		return script_no_errors;

	int value = 0;
	int cnt = sscanf(str_data.c_str(), "%d", &value);

	int min = 0;
	int max = cntrWfwidth->maximum();

	if(cnt < 1 || value < min || value > max)
		return script_invalid_parameter;

	active_modem->set_freq(value);

	return script_no_errors;
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_rx_rsid(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_bool(btnRSID, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_tx_rsid(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_bool(btnTxRSID, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_spot(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_bool(btnAutoSpot, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_rev(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	if(!wf)	return script_no_errors;
	return assign_bool(wf->btnRev, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_afc(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_bool(btnAFC, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_lock(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	if(!wf)	return script_no_errors;
	return assign_bool(wf->xmtlock, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_sql(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_bool(btnSQL, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_kpsql(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_bool(btnPSQL, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 **********************************************************/
int process_modem(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	// Not suitable for assign_xxxx
	if(!sp || !sc)
		return script_function_parameter_error;

	bool modem_found = false;
	int index = 0;

	std::string value;
	value.assign(sc->args[0]);

	if(value.empty())
		return script_parameter_error;

	sp->to_uppercase(value);

	for(index = 0; index < NUM_MODES; index++) {
		if(strncmp(value.c_str(), mode_info[index].sname, 32) == 0) {
			modem_found = true;
			break;
		}
	}

	if(modem_found == false)
		return script_invalid_parameter;

	if((data_io_enabled == KISS_IO) && (!(mode_info[index].iface_io & KISS_IO))) {
		LOG_INFO(_("Invalid Modem for KISS IO"));
		return script_invalid_parameter;
	}

	REQ_SYNC(init_modem_sync, index, 0);

	return script_no_errors;
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_io_kiss_ip_address(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	// Not suitable for assign_xxxx
	if(!sp || !sc)
		return script_function_parameter_error;

	std::string str_data = "";

	if(sc->argc > 0)
		str_data.assign(sc->args[0]);

	if(str_data.empty())
		return script_invalid_parameter;

	if(!txtKiss_ip_address)
		return script_no_errors;

	if(strncmp((const char *)str_data.c_str(), (const char *)txtKiss_ip_address->value(), 32) != 0) {
		txtKiss_ip_address->value(str_data.c_str());
		txtKiss_ip_address->do_callback();
		sp->restart_flag(true);
	}

	return script_no_errors;
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_io_kiss_io_port_no(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	// Not suitable for assign_xxxx
	if(!sp || !sc)
		return script_function_parameter_error;

	std::string str_data = "";

	if(sc->argc > 0)
		str_data.assign(sc->args[0]);

	if(str_data.empty())
		return script_invalid_parameter;

	if(!txtKiss_ip_io_port_no)
		return script_no_errors;

	if(strncmp((const char *)str_data.c_str(), (const char *)txtKiss_ip_io_port_no->value(), 32) != 0) {
		txtKiss_ip_io_port_no->value(str_data.c_str());
		txtKiss_ip_io_port_no->do_callback();
		sp->restart_flag(true);
	}

	return script_no_errors;
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_io_kiss_o_port_no(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	// Not suitable for assign_xxxx
	if(!sp || !sc)
		return script_function_parameter_error;

	std::string str_data = "";

	if(sc->argc > 0)
		str_data.assign(sc->args[0]);

	if(str_data.empty())
		return script_invalid_parameter;

	if(!txtKiss_ip_out_port_no)
		return script_no_errors;

	if(strncmp((const char *)str_data.c_str(), (const char *)txtKiss_ip_out_port_no->value(), 32) != 0) {
		txtKiss_ip_out_port_no->value(str_data.c_str());
		txtKiss_ip_out_port_no->do_callback();
		sp->restart_flag(true);
	}

	return script_no_errors;
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_io_kiss_dual_port(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_bool(btnEnable_dual_port, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_io_kiss_busy_channel(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_bool(btnEnableBusyChannel, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_io_kiss_continue_after(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_integer(cntBusyChannelSeconds, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_io_kiss_kpsql_atten(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_integer(cntKPSQLAttenuation, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_io_arq_ip_address(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	// Not suitable for assign_xxxx
	if(!sp || !sc)
		return script_function_parameter_error;

	std::string str_data = "";

	if(sc->argc > 0)
		str_data.assign(sc->args[0]);

	if(str_data.empty())
		return script_invalid_parameter;

	if(!txtArq_ip_address)
		return script_no_errors;

	if(strncmp((const char *)str_data.c_str(), (const char *)txtArq_ip_address->value(), 32) != 0) {
		txtArq_ip_address->value(str_data.c_str());
		txtArq_ip_address->do_callback();
		sp->restart_flag(true);
	}

	return script_no_errors;
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_io_arq_io_port_no(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	// Not suitable for assign_xxxx
	if(!sp || !sc)
		return script_function_parameter_error;

	std::string str_data = "";

	if(sc->argc > 0)
		str_data.assign(sc->args[0]);

	if(str_data.empty())
		return script_invalid_parameter;

	if(!txtArq_ip_address)
		return script_no_errors;

	if(strncmp((const char *)str_data.c_str(), (const char *)txtArq_ip_port_no->value(), 32) != 0) {
		txtArq_ip_port_no->value(str_data.c_str());
		txtArq_ip_port_no->do_callback();
		sp->restart_flag(true);
	}

	return script_no_errors;
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_io_xmlrpc_ip_address(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	// Not suitable for assign_xxxx
	if(!sp || !sc)
		return script_function_parameter_error;

	std::string str_data = "";

	if(sc->argc > 0)
		str_data.assign(sc->args[0]);

	if(str_data.empty())
		return script_invalid_parameter;

	if(!txtXmlrpc_ip_address)
		return script_no_errors;

	if(strncmp((const char *)str_data.c_str(), (const char *)txtXmlrpc_ip_address->value(), 32) != 0) {
		txtXmlrpc_ip_address->value(str_data.c_str());
		txtXmlrpc_ip_address->do_callback();
		sp->restart_flag(true);
	}

	return script_no_errors;
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_io_xmlrpc_io_port_no(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	// Not suitable for assign_xxxx
	if(!sp || !sc)
		return script_function_parameter_error;

	std::string str_data = "";

	if(sc->argc > 0)
		str_data.assign(sc->args[0]);

	if(str_data.empty())
		return script_invalid_parameter;

	if(!txtXmlrpc_ip_port_no)
		return script_no_errors;

	if(strncmp((const char *)str_data.c_str(), (const char *)txtXmlrpc_ip_port_no->value(), 32) != 0) {
		txtXmlrpc_ip_port_no->value(str_data.c_str());
		txtXmlrpc_ip_port_no->do_callback();
		sp->restart_flag(true);
	}

	return script_no_errors;
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_io_lock(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_bool(btnDisable_p2p_io_widgets, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_io_active_port(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	// Not suitable for assign_xxxx
	if(!sp || !sc)
		return script_function_parameter_error;

	std::string value;
	value.assign(sc->args[0]);

	if(value.empty())
		return script_parameter_error;

	sp->to_uppercase(value);

	if(!btnEnable_kiss || !btnEnable_arq)
		return script_no_errors;

	if(value.find(PARM_KISS) != std::string::npos) {
        enable_kiss();
	} else if(value.find(PARM_ARQ) != std::string::npos) {
        enable_arq();
	} else {
		return script_invalid_parameter;
	}

	return script_no_errors;
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_io_ax25_decode(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_bool(btnEnable_ax25_decode, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_io_csma(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_bool(btnEnable_csma, sp, sc);
}

/** ********************************************************
 * \brief Assign Call Sign.
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 * \par Note:
 * This string storage can be assigned to anything. User
 * should follow the limitations imposed by the rules
 * of the host country.
 ***********************************************************/
int process_callsign_info(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_string(inpMyCallsign, sp, sc);
}

/** ********************************************************
 * \brief Operator Name
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_name_info(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_string(inpMyName, sp, sc);
}

/** ********************************************************
 * \brief QTH Location of Operator
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_qth_info(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_string(inpMyQth, sp, sc);
}

/** ********************************************************
 * \brief Assign Locator
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_locator_info(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_string(inpMyLocator, sp, sc);
}

/** ********************************************************
 * \brief Assign Antenna information
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_antenna_info(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_string(inpMyAntenna, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_use_oss_audio_device(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
#if USE_OSS
	return assign_bool(btnAudioIO[0], sp, sc);
#else
	return script_no_errors;
#endif // USE_OSS
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_oss_audio_device_path(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	// Not suitable for assign_xxxx
#if USE_OSS
	if(!sp || !sc)
		return script_function_parameter_error;

	std::string str_data;
	std::string valid_data;

	str_data.assign(sc->args[0]);
	if(str_data.empty())
		return script_invalid_parameter;

	if(!menuOSSDev)
		return script_no_errors;

	if(sp->check_dev_path(str_data.c_str()))
		return script_device_path_not_found;

	int index = 0;
	bool found = false;
	int count = menuOSSDev->menubutton()->size();

	for (index = 0; index < count; index++ ) {
		const Fl_Menu_Item &item = menuOSSDev->menubutton()->menu()[index];
		valid_data.assign(item.label());
		if(!valid_data.empty()) {
			if(strncmp(valid_data.c_str(), str_data.c_str(), FL_PATH_MAX) == 0) {
				found = true;
				break;
			}
		}
	}

	if(!found)
		return script_invalid_parameter;

	menuOSSDev->value(index);
	menuOSSDev->do_callback();
	return script_no_errors;

#else
	return script_no_errors;
#endif // USE_OSS
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_use_port_audio_device(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
#if USE_PORTAUDIO
	return assign_bool(btnAudioIO[1], sp, sc);
#else
	return script_no_errors;
#endif // USE_PORTAUDIO
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_capture_path(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	// Not suitable for assign_xxxx
#if USE_PORTAUDIO
	if(!sp || !sc)
		return script_function_parameter_error;

	int value = 0;
	std::string str_data = "";

	if(sc->argc < 2)
		return script_invalid_parameter;

	int cnt = sscanf(sc->args[0], "%d", &value);

	if(cnt < 1 || value < 0)
		return script_invalid_parameter;

	str_data.assign(sc->args[1]);
	if(str_data.empty())
		return script_invalid_parameter;

	if(!menuPortInDev)
		return script_no_errors;

	cnt = pa_set_dev(menuPortInDev, str_data, value);

	if(cnt == PA_DEV_NOT_FOUND)
		return script_invalid_parameter;

#endif // USE_PORTAUDIO

	return script_no_errors;
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_playback_path(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	// Not suitable for assign_xxxx
#if USE_PORTAUDIO

	if(!sp || !sc)
		return script_function_parameter_error;

	int value = 0;
	std::string str_data = "";

	if(sc->argc < 2)
		return script_invalid_parameter;

	int cnt = sscanf(sc->args[0], "%d", &value);

	if(cnt < 1 || value < 0)
		return script_invalid_parameter;

	str_data.assign(sc->args[1]);
	if(str_data.empty())
		return script_invalid_parameter;

	if(!menuPortOutDev)
		return script_no_errors;

	cnt = pa_set_dev(menuPortOutDev, str_data, value);

	if(cnt == PA_DEV_NOT_FOUND)
		return script_invalid_parameter;

#endif

	return script_no_errors;
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_use_pulse_audio_device(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
#if USE_PULSEAUDIO
	return assign_bool(btnAudioIO[2], sp, sc);
#else
	return script_no_errors;
#endif // USE_PULSEAUDIO
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_pulse_audio_device_path(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
#if USE_PULSEAUDIO
	return assign_string(inpPulseServer, sp, sc);
#else
	return script_no_errors;
#endif // USE_PULSEAUDIO
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_audio_device_sample_rate_capture(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	// Not suitable for assign_xxxx
	if(!sp || !sc)
		return script_function_parameter_error;

	std::string str_data = "";

	if(sc->argc > 0)
		str_data.assign(sc->args[0]);

	if(str_data.empty())
		return script_invalid_parameter;

	if(!menuInSampleRate)
		return script_no_errors;

	int index = menuInSampleRate->find_index(str_data.c_str());
	if(index < 0) {
		str_data.append(" (native)");
		index = menuInSampleRate->find_index(str_data.c_str());
	}

	if(index < 0)
		return script_invalid_parameter;

	menuInSampleRate->index(index);
	menuInSampleRate->do_callback();

	return script_no_errors;
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_audio_device_sample_rate_playback(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	// Not suitable for assign_xxxx
	if(!sp || !sc)
		return script_function_parameter_error;

	std::string str_data = "";

	if(sc->argc > 0)
		str_data.assign(sc->args[0]);

	if(str_data.empty())
		return script_invalid_parameter;

	if(!menuOutSampleRate)
		return script_no_errors;

	int index = menuOutSampleRate->find_index(str_data.c_str());
	if(index < 0) {
		str_data.append(" (native)");
		index = menuOutSampleRate->find_index(str_data.c_str());
	}

	if(index < 0)
		return script_invalid_parameter;

	menuOutSampleRate->index(index);
	menuOutSampleRate->do_callback();

	return script_no_errors;

}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_audio_device_converter(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	// Not suitable for assign_xxxx
	if(!sp || !sc)
		return script_function_parameter_error;

	std::string str_data;
	std::string append_string;

	if(sc->argc > 0)
		str_data.assign(sc->args[0]);

	if(str_data.empty())
		return script_invalid_parameter;

	append_string.assign(" Sinc Interpolator");
	if(str_data.find(append_string) != std::string::npos)
		append_string.clear();

	if(append_string.empty()) {
		append_string.assign(" Interpolator");
		if(str_data.find(append_string) != std::string::npos) {
			append_string.clear();
		}
	}

	str_data.append(append_string);

	if(!menuSampleConverter)
		return script_no_errors;

	int index = menuSampleConverter->find_index(str_data.c_str());
	if(index < 0)
		return script_invalid_parameter;

	menuSampleConverter->index(index);
	menuSampleConverter->do_callback();

	return script_no_errors;
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_rx_ppm(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_integer(cntRxRateCorr, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_tx_ppm(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_integer(cntTxRateCorr, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_tx_offset(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_integer(cntTxOffset, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_modem_signal_left_right(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_bool(chkAudioStereoOut, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_reverse_left_right(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_bool(chkReverseAudio, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_ptt_tone_right_channel(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_bool(btnPTTrightchannel2, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_cw_qsk_right_channel(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_bool(btnQSK2, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_pseudo_fsk_right_channel(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_bool(chkPseudoFSK2, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_wave_file_sample_rate(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	// Not suitable for assign_xxxx
	if(!sp || !sc)
		return script_function_parameter_error;

	std::string str_data = "";
	char buffer[32];

	if(sc->argc > 0)
		str_data.assign(sc->args[0]);

	if(str_data.empty())
		return script_invalid_parameter;

	int value = 0;
	int cnt = sscanf(str_data.c_str(), "%d", &value);

	if(cnt < 1)
		return script_invalid_parameter;

	if((value < 8000) || (value > 48000))
		return script_invalid_parameter;

	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer) - 1, "%d", value);

	if(!listbox_wav_samplerate)
		return script_no_errors;

	int index = listbox_wav_samplerate->find_index(buffer);
	if(index < 0)
		return script_invalid_parameter;

	listbox_wav_samplerate->index(index);
	listbox_wav_samplerate->do_callback();

	return script_no_errors;
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_hrdw_ptt_right_audio_channel(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_bool(btnPTTrightchannel, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_hrdw_ptt_sep_serial_port(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_bool(btnTTYptt, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 * \par NOTE:
 * Fl_ComboBox (custom widget)
 * find_index() located in combo.cxx and used here returns -1
 * on non-matching.
 ***********************************************************/
int process_hrdw_ptt_sep_serial_port_path(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_index(inpTTYdev, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_hrdw_ptt_sep_serial_port_rts(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_bool(btnRTSptt, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_hrdw_ptt_sep_serial_port_dtr(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_bool(btnDTRptt, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_hrdw_ptt_sep_serial_port_rts_v(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_bool(btnRTSplusV, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_hrdw_ptt_sep_serial_port_dtr_v(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_bool(btnDTRplusV, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_hrdw_ptt_start_delay(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_integer(cntPTT_on_delay, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_hrdw_ptt_end_delay(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_integer(cntPTT_off_delay, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_hrdw_ptt_uhrouter(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
#if HAVE_UHROUTER
	return assign_bool(btnUseUHrouterPTT, sp, sc);
#else
	return script_no_errors;
#endif // HAVE_UHROUTER
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_hrdw_ptt_parallel(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_bool(btnUsePPortPTT, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_hrdw_ptt_initialize(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{

	if(!btnInitHWPTT)
		return script_no_errors;

	btnInitHWPTT->do_callback();

	return script_no_errors;
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_use_rigcat(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_bool(chkUSERIGCAT, sp, sc);
}

extern void loadRigXmlFile(void);
/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_rigcat_desc_file(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	// Not suitable for assign_xxxx
	if(!sp || !sc)
		return script_function_parameter_error;

	std::string str_data = "";

	if(sc->argc > 0)
		str_data.assign(sc->args[0]);

	if(str_data.empty())
		return script_invalid_parameter;

	if(str_data.find(DEFAULT_RIGXML_FILENAME) != std::string::npos)
		return script_no_errors;

	if(sp->check_filename((char *) str_data.c_str()))
		return script_file_not_found;

	if(!txtXmlRigFilename)
		return script_no_errors;

	progdefaults.XmlRigFilename.assign(str_data);
	txtXmlRigFilename->value(fl_filename_name(progdefaults.XmlRigFilename.c_str()));
	loadRigXmlFile();

	return script_no_errors;
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_rigcat_device_path(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_index(inpXmlRigDevice, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_rigcat_retries(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	int value = 0;
	int error = assign_integer(cntRigCatRetries, sp, sc, value);

	if(!error && cntRigCatRetries) {
		progdefaults.RigCatRetries = value;
		progdefaults.changed = true;
	}
	return error;
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_rigcat_retry_interval(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	int value = 0;
	int error = assign_integer(cntRigCatTimeout, sp, sc, value);

	if(!error && cntRigCatTimeout) {
		progdefaults.RigCatTimeout = value;
		progdefaults.changed = true;
	}

	return error;
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_rigcat_write_delay(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	int value = 0;
	int error = assign_integer(cntRigCatWait, sp, sc, value);

	if(!error && cntRigCatWait) {
		progdefaults.RigCatTimeout = value;
		progdefaults.changed = true;
	}

	return error;
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_rigcat_init_delay(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	int value = 0;
	int error = assign_integer(cntRigCatInitDelay, sp, sc, value);

	if(!error && cntRigCatInitDelay) {
		progdefaults.RigCatInitDelay = value;
		progdefaults.changed = true;
	}
	return error;
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_rigcat_baud_rate(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	int index = 0;
	int error = assign_index(listbox_xml_rig_baudrate, sp, sc, index);

	if(!error && listbox_xml_rig_baudrate) {
		progdefaults.XmlRigBaudrate = index;
		progdefaults.changed = true;
	}

	return error;
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_rigcat_cat_command_ptt(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return assign_bool(btnRigCatCMDptt, sp, sc);
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_rigcat_stop_bits(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	int value = 0;
	int error = assign_integer(valRigCatStopbits, sp, sc, value);

	if(!error && valRigCatStopbits) {
		progdefaults.RigCatStopbits = value;
		progdefaults.changed = true;
	}

	return error;
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_rigcat_commands_echoed(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	bool value = 0;
	int error = assign_bool(btnRigCatEcho, sp, sc, value);

	if(!error && btnRigCatEcho) {
		progdefaults.RigCatECHO = value;
		progdefaults.changed = true;
	}

	return error;
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_rigcat_toggle_rts_ptt(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	bool value = 0;
	int error = assign_bool(btnRigCatRTSptt, sp, sc, value);

	if(!error && btnRigCatRTSptt) {
		progdefaults.RigCatRTSptt = value;
		progdefaults.changed = true;
	}

	return error;
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_rigcat_restore_on_close(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	bool value = 0;
	int error = assign_bool(chk_restore_tio, sp, sc, value);

	if(!error && chk_restore_tio) {
		progdefaults.RigCatRestoreTIO = value;
		progdefaults.changed = true;
	}

	return error;
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_rigcat_rts_12v(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	bool value = 0;
	int error = assign_bool(btnRigCatRTSplus, sp, sc, value);

	if(!error && btnRigCatRTSplus) {
		progdefaults.RigCatRTSplus = value;
		progdefaults.changed = true;
	}

	return error;
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_rigcat_dtr_12v(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	bool value = 0;
	int error = assign_bool(btnRigCatDTRplus, sp, sc, value);

	if(!error && btnRigCatDTRplus) {
		progdefaults.RigCatDTRplus = value;
		progdefaults.changed = true;
	}

	return error;
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_rigcat_hrdwr_flow(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	bool value = 0;
	int error = assign_bool(chkRigCatRTSCTSflow, sp, sc, value);

	if(!error && chkRigCatRTSCTSflow) {
		progdefaults.RigCatRTSCTSflow = value;
		progdefaults.changed = true;
	}

	return error;
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_rigcat_vsp_enable(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	bool value = 0;
	int error = assign_bool(chkRigCatVSP, sp, sc, value);

	if(!error && chkRigCatVSP) {
		progdefaults.RigCatVSP = value;
		progdefaults.changed = true;
	}

	return error;
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_rigcat_initialize(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	if(!btnInitRIGCAT)
		return script_no_errors;

	btnInitRIGCAT->do_callback();

	return script_no_errors;
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_use_hamlib(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
#if USE_HAMLIB
	return assign_bool(chkUSEHAMLIB, sp, sc);
#endif // USE_HAMLIB

	return script_no_errors;
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_hamlib_rig(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
#if USE_HAMLIB
	return assign_index(cboHamlibRig, sp, sc);
#else
	return script_no_errors;
#endif // USE_HAMLIB
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_hamlib_retries(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
#if USE_HAMLIB
	int value = 0;
	int error = assign_integer(cntHamlibRetries, sp, sc, value);

	if(!error && cntHamlibRetries) {
		progdefaults.HamlibRetries = value;
		progdefaults.changed = true;
	}

	return error;

#else
	return script_no_errors;
#endif // USE_HAMLIB

}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_hamlib_retry_interval(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
#if USE_HAMLIB
	int value = 0;
	int error = assign_integer(cntHamlibTimeout, sp, sc, value);

	if(!error && cntHamlibTimeout) {
		progdefaults.HamlibTimeout = value;
		progdefaults.changed = true;
	}

	return error;
#else
	return script_no_errors;
#endif // USE_HAMLIB
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_hamlib_write_delay(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
#if USE_HAMLIB
	int value = 0;
	int error = assign_integer(cntHamlibWriteDelay, sp, sc, value);

	if(!error && cntHamlibWriteDelay) {
		progdefaults.HamlibWriteDelay = value;
		progdefaults.changed = true;
	}

	return error;
#else
	return script_no_errors;
#endif // USE_HAMLIB
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_hamlib_post_write_delay(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
#if USE_HAMLIB
	int value = 0;
	int error = assign_integer(cntHamlibWait, sp, sc, value);

	if(!error && cntHamlibWait) {
		progdefaults.HamlibWait = value;
		progdefaults.changed = true;
	}

	return error;
#else
	return script_no_errors;
#endif // USE_HAMLIB
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 * \par Note:
 * This string storage can be assigned to anything. User
 * should follow the limitations imposed by the rules
 * of the host country.
 ***********************************************************/
int process_hamlib_device_path(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
#if USE_HAMLIB
	// Not suitable for assign_xxxx
	if(!sp || !sc)
		return script_function_parameter_error;

	std::string str_data = "";

	if(sc->argc > 0)
		str_data.assign(sc->args[0]);

	if(str_data.empty())
		return script_invalid_parameter;

	if(!inpRIGdev)
		return script_no_errors;

	int index = inpRIGdev->find_index(str_data.c_str());
	if(index < 0)
		return script_invalid_parameter;

	progdefaults.HamRigDevice.assign(str_data);
	progdefaults.changed = true;

	inpRIGdev->value(str_data.c_str());
	inpRIGdev->do_callback();

#endif // USE_HAMLIB

	return script_no_errors;
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_hamlib_baud_rate(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
#if USE_HAMLIB
	int index = 0;
	int error = assign_index(listbox_baudrate, sp, sc, index);

	if(!error && listbox_baudrate) {
		progdefaults.HamRigBaudrate = index;
		progdefaults.changed = true;
	}
	return error;
#else
	return script_no_errors;
#endif // USE_HAMLIB
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_hamlib_stop_bits(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
#if USE_HAMLIB
	int value = 0;
	int error = assign_integer(valHamRigStopbits, sp, sc, value);

	if(!error && valHamRigStopbits) {
		progdefaults.HamRigStopbits = value;
		progdefaults.changed = true;
	}
	return error;
#else
	return script_no_errors;
#endif // USE_HAMLIB
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_hamlib_poll_rate(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
#if USE_HAMLIB
	int value = 0;
	int error = assign_integer(valHamRigPollrate, sp, sc, value);

	if(!error && valHamRigPollrate) {
		progdefaults.HamRigPollrate = value;
		progdefaults.changed = true;
	}
	return error;
#else
	return script_no_errors;
#endif // USE_HAMLIB
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_hamlib_sideband(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
#if USE_HAMLIB
	int index = 0;
	int error = assign_index(listbox_sideband, sp, sc, index);

	if(!error && listbox_sideband) {
		progdefaults.HamlibSideband = index;
		progdefaults.changed = true;
	}
	return error;
#else
	return script_no_errors;
#endif // USE_HAMLIB
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_hamlib_ptt_hl_command(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
#if USE_HAMLIB
	bool value = 0;
	int error = assign_bool(btnHamlibCMDptt, sp, sc, value);

	if(!error && btnHamlibCMDptt) {
		progdefaults.HamlibCMDptt = value;
		progdefaults.changed = true;
	}
	return error;
#else
	return script_no_errors;
#endif // USE_HAMLIB
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_hamlib_dtr_12(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
#if USE_HAMLIB
	bool value = 0;
	int error = assign_bool(btnHamlibDTRplus, sp, sc, value);

	if(!error && btnHamlibDTRplus) {
		progdefaults.HamlibDTRplus = value;
		progdefaults.changed = true;
	}
	return error;
#else
	return script_no_errors;
#endif // USE_HAMLIB
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_hamlib_rts_12(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
#if USE_HAMLIB
	// Not suitable for assign_xxxx
	if(!sp || !sc)
		return script_function_parameter_error;

	bool value = 0;
	int error = sp->check_bool(sc->args[0], value);

	if(error != script_no_errors)
		return error;

	if(!chkHamlibRTSCTSflow || !chkHamlibRTSplus)
		return script_no_errors;

	if(chkHamlibRTSCTSflow->value())
		value = false;

	progdefaults.HamlibRTSplus = value;
	progdefaults.changed = true;

	chkHamlibRTSplus->value(value);
	chkHamlibRTSplus->do_callback();

#endif // USE_HAMLIB

	return script_no_errors;
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_hamlib_rts_cts_flow(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
#if USE_HAMLIB
	bool value = 0;
	int error = assign_bool(chkHamlibRTSCTSflow, sp, sc, value);

	if(!error && chkHamlibRTSCTSflow) {
		progdefaults.HamlibRTSCTSflow = value;
		progdefaults.changed = true;
	}
	return error;
#else
	return script_no_errors;
#endif // USE_HAMLIB
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_hamlib_xon_xoff_flow(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
#if USE_HAMLIB
	bool value = 0;
	int error = assign_bool(chkHamlibXONXOFFflow, sp, sc, value);

	if(!error && chkHamlibXONXOFFflow) {
		progdefaults.HamlibXONXOFFflow = value;
		progdefaults.changed = true;
	}
	return error;
#else
	return script_no_errors;
#endif // USE_HAMLIB
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_hamlib_advanced_config(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
#if USE_HAMLIB
	std::string value = "";
	int error = assign_string(inpHamlibConfig, sp, sc, value);

	if(!error && inpHamlibConfig) {
		progdefaults.HamConfig.assign(value);
		progdefaults.changed = true;
	}
	return error;
#else
	return script_no_errors;
#endif // USE_HAMLIB
}

/** ********************************************************
 * \brief Initialize HAMLIB Parameters
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_hamlib_initialize(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
#if USE_HAMLIB

	if(!btnInitHAMLIB)
		return script_no_errors;

	btnInitHAMLIB->do_callback();

#endif // USE_HAMLIB

	return script_no_errors;
}

/** ********************************************************
 * \brief Set XMLRPC rig control to active use.
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_use_xml_rpc(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	bool value = 0;
	int error = assign_bool(chkUSEXMLRPC, sp, sc, value);

	if(!error && chkUSEXMLRPC) {
		progdefaults.chkUSEXMLRPCis = value;
		progdefaults.changed = true;
	}

	return error;
}

/** ********************************************************
 * \brief
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_xml_rpc_mode_bw_delay(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	double value = 0;
	int error = assign_double(mbw_delay, sp, sc, value);

	if(!error && mbw_delay) {
		progdefaults.mbw = value;
		progdefaults.changed = true;
	}

	return error;
}

/** ********************************************************
 * \brief Initialize Rig Control XMLRPC interface.
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_xml_rpc_initialize(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{

	if(!btnInitXMLRPC)
		return script_no_errors;

	btnInitXMLRPC->do_callback();

	return script_no_errors;
}

/** ********************************************************
 * \brief Assign Macro information to there respective
 * macro button.
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_load_macro(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	// Not suitable for assign_xxxx
	if(!sp || !sc)
		return script_function_parameter_error;

	std::string macro_data = sp->macro_command();
	std::string macro_label = "";

	int col = 0;
	int row = 0;
	int index = 0;

	if(sc->argc < 3) return script_invalid_parameter;

	if((!sc->args[0]) || (!sc->args[1]) || (!sc->args[2]))
		return script_invalid_parameter;

	index = sscanf(sc->args[0], "%d", &col);
	if(index < 1)
		return script_invalid_parameter;

	index = sscanf(sc->args[1], "%d", &row);
	if(index < 1)
		return script_invalid_parameter;

	macro_label.assign(sc->args[2]);
	if(macro_label.empty())
		return script_invalid_parameter;

	if(col > 0) col--;
	if(row > 0) row--;

	index = (col * NUMMACKEYS) + row;

	if((index < 0) || (index > MAXMACROS))
		return script_invalid_parameter;

	if((col == 0) || ((col == altMacros) && (progdefaults.mbar_scheme > MACRO_SINGLE_BAR_MAX))) {
		update_macro_button(index, macro_data.c_str(), macro_label.c_str());
	} else {
		macros.text[index].assign(macro_data);
		macros.name[index].assign(macro_label);
	}

	return script_no_errors;
}

/** ********************************************************
 * \brief Reset Configuration panel attributes
 * \param sp Access to ScritpParsing members.
 * \param sc Access to SCRIPT_COMMANDS structure variables.
 * \return 0 (no error) Other (error)
 ***********************************************************/
int process_reset(ScriptParsing *sp, SCRIPT_COMMANDS *sc)
{
	return script_no_errors;
}

/** ********************************************************
 * \brief Pass script file name to script passing class
 * for execution.
 * Called from the File->Execute Config Script menu item.
 * \param void
 ***********************************************************/
static void script_execute(const char *filename, bool queue_flag)
{

	ScriptParsing *sp = 0;
	static std::string script_filename = "";

	if(!filename) {
		LOG_INFO(_("Script file name (path) null pointer"));
		return;
	}

	script_filename.assign(filename);

	if(script_filename.empty()) {
		LOG_INFO(_("Script file name (path) invalid"));
		return;
	}

	sp = new ScriptParsing;

	if(!sp) {
		LOG_INFO(_("Script Parsing Class Allocation Fail (%s)"), script_filename.c_str());
		return;
	}

	LOG_INFO(_("Executing script file: %s"), script_filename.c_str());

	sp->parse_commands((char *) script_filename.c_str());

	if(sp->script_errors()) {
		LOG_INFO(_("Issues reported in processing script file: %s"), script_filename.c_str());
		fl_alert("%s", _("Script file contains potential issues\nSee documentation and/or Log file for details."));
	}

	if(sp->restart_flag()) {
		fl_alert("%s", _("Some changes made by the script requires program\nrestart before they become active."));
	}

	if(sp)
		delete sp;
}

/** ********************************************************
 * \brief Call back function when executing a configuration script.
 * Called from the File->Execute Config Script menu item.
 * \param void
 ***********************************************************/
void cb_scripts(bool reset_path = false)
{
	pthread_mutex_lock(&mutex_script_io);

	static bool first_time = true;
	static char script_filename[FL_PATH_MAX + 1];
	std::string new_path = "";

	if(reset_path || first_time) {
		memset(script_filename, 0, sizeof(script_filename));
		strncpy(script_filename, ScriptsDir.c_str(), FL_PATH_MAX);
		int len = strnlen(script_filename, FL_PATH_MAX);

		if(len > 0) {
			len--;
			if(script_filename[len] == PATH_CHAR_SEPERATOR);
			else strncat(script_filename, PATH_SEPERATOR, FL_PATH_MAX);
		} else {
			return;
		}

		first_time = false;
	}

	const char *p = FSEL::select((char *)_("Script Files"), (char *)_("*.txt"), \
								 script_filename);

	if(p) {
		memset(script_filename, 0, sizeof(script_filename));
		strncpy(script_filename, p, FL_PATH_MAX);

		Fl::lock();
		script_execute(script_filename, false);
		Fl::unlock();

	}

	pthread_mutex_unlock(&mutex_script_io);
}
