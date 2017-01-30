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
#include <string.h>

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
#include "waterfall.h"
#include "util.h"

#include "script_parsing.h"
#include "run_script.h"


void cb_create_default_script(void);

static int create_default_script(char *file_name);
static int add_command(FILE *fd, char *cmd, int    param, int indent_level);
static int add_command(FILE *fd, char *cmd, char * param, int indent_level);
static int add_command(FILE *fd, char *cmd, int indent_level);
static int add_command(FILE *fd, char *cmd, bool   param, int indent_level);
static int add_command(FILE *fd, char *cmd, double param, int indent_level);
static int add_string(FILE *fd, char *cmd, int indent_level);
static void write_macro_list(FILE *fd);

extern pthread_mutex_t mutex_script_io;

/** ********************************************************
 * \brief Menu callback. Create default script based on the
 * current settings.
 * \param none
 * \return void
 ***********************************************************/
void cb_create_default_script(void)
{
	pthread_mutex_lock(&mutex_script_io);

	static bool first_time = true;
	static char script_filename[FL_PATH_MAX + 1];
	std::string new_path = "";

	if(first_time) {
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

		strncat(script_filename, "default_script.txt", FL_PATH_MAX);

		first_time = false;
	}

	const char *p = FSEL::saveas((char *)_("Script Files"), (char *)_("*.txt"), \
								 script_filename);

	if(p) {
		memset(script_filename, 0, sizeof(script_filename));
		strncpy(script_filename, p, FL_PATH_MAX);

		Fl::lock();
		create_default_script(script_filename);
		Fl::unlock();

	}

	pthread_mutex_unlock(&mutex_script_io);
}

/** ********************************************************
 * \brief Create default script based on current settings.
 * \param file_name Pointer to the file name and path.
 * \return 0 OK, other Error
 ***********************************************************/
static int create_default_script(char *file_name)
{
	FILE *fd = (FILE *)0;
	static char buffer[FL_PATH_MAX];
	std::string temp = "";

	if(!file_name) {
		LOG_INFO(_("Invalid File Name Pointer (NULL) in function %s:%d"), __FILE__, __LINE__);
		return -1;
	}

	fd = fl_fopen(file_name, "w");

	if(!fd) {
		LOG_INFO(_("Unable to create file %s (Error No=%d) func %s:%d"), file_name, errno, __FILE__, __LINE__);
		return -1;
	}

	memset(buffer, 0, sizeof(buffer));

	// Tag the text file as a FLDIGI script file

	fprintf(fd, _("%s\n# Fldigi Generated Config Script\n"), SCRIPT_FILE_TAG);
	time_t thetime = time(0);
	fprintf(fd, _("# Created: %s\n"), ctime(&thetime));

	// FLDIGI Main Window
	if(add_command(fd, (char *)CMD_FLDIGI,         0)) return fclose(fd);
	if(add_command(fd, (char *)CMD_FLDIGI_FREQ,    (double) qsoFreqDisp->value(),          1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_FLDIGI_MODE,    (char *) qso_opMODE->value(),           1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_FLDIGI_WFHZ,    (int)    wf->Carrier(),                 1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_FLDIGI_RXID,    (bool)   btnRSID->value(),              1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_FLDIGI_TXID,    (bool)   btnTxRSID->value(),            1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_FLDIGI_SPOT,    (bool)   btnAutoSpot->value(),          1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_FLDIGI_REV,     (bool)   wf->btnRev->value(),           1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_FLDIGI_AFC,     (bool)   btnAFC->value(),               1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_FLDIGI_LOCK,    (bool)   wf->xmtlock->value(),          1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_FLDIGI_SQL,     (bool)   btnSQL->value(),               1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_FLDIGI_KPSQL,   (bool)   btnPSQL->value(),              1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_FLDIGI_MODEM,   (char *) active_modem->get_mode_name(), 1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_END_CMD,        0)) return fclose(fd);


	// OPERATOR
	if(add_command(fd, (char *)CMD_OPERATOR,  0)) return fclose(fd);
	if(add_command(fd, (char *)CMD_CALLSIGN,  (char *) inpMyCallsign->value(), 1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_QTH,       (char *) inpMyName->value(),     1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_NAME,      (char *) inpMyQth->value(),      1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_LOCATOR,   (char *) inpMyLocator->value(),  1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_ANTENNA,   (char *) inpMyAntenna->value(),  1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_END_CMD,   0)) return fclose(fd);

	// AUDIO DEVICE
	if(add_command(fd, (char *)CMD_AUDIO_DEVICE,       0)) return fclose(fd);
#if USE_OSS
	// OSS
	if(add_command(fd, (char *)CMD_OSS_AUDIO,          (bool)   btnAudioIO[0]->value(), 1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_OSS_AUDIO_DEV_PATH, (char *) menuOSSDev->value(),    1)) return fclose(fd);
#endif // USE_OSS

#if USE_PORTAUDIO
	// PORT AUDIO
	if(add_command(fd, (char *)CMD_PORT_AUDIO,         (bool)   btnAudioIO[1]->value(), 1)) return fclose(fd);

	if(menuPortInDev->value() > -1) {
		memset(buffer, 0, sizeof(buffer));
		// Invalid warning issued (clang-600.0.56 [3.5] MacOSX) if index is replaced with reinterpret_cast<intptr_t>(x) in snprintf() routine.
		int index = reinterpret_cast<intptr_t>(menuPortInDev->mvalue()->user_data());
		snprintf(buffer, sizeof(buffer)-1, "%s:%d,\"%s\"", CMD_PORTA_CAP, index, menuPortInDev->text());
		if(add_string(fd, (char *)buffer,  1)) return fclose(fd);
	}

	if(menuPortOutDev->value() > -1) {
		memset(buffer, 0, sizeof(buffer));
		// Invalid warning issued (clang-600.0.56 [3.5] MacOSX) if index is replaced with reinterpret_cast<intptr_t>(x) in snprintf() routine.
		int index = reinterpret_cast<intptr_t>(menuPortOutDev->mvalue()->user_data());
		snprintf(buffer, sizeof(buffer)-1, "%s:%d,\"%s\"", CMD_PORTA_PLAY, index, menuPortOutDev->text());
		if(add_string(fd, (char *)buffer, 1)) return fclose(fd);
	}
#endif // USE_PORTAUDIO

#if USE_PULSEAUDIO
	// PULSE AUDIO
	if(add_command(fd, (char *)CMD_PULSEA,         (bool)   btnAudioIO[2]->value(),  1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_PULSEA_SERVER,  (char *) inpPulseServer->value(), 1)) return fclose(fd);
#endif // USE_PULSEAUDIO

	if(add_command(fd, (char *)CMD_END_CMD,        0)) return fclose(fd);


	// AUDIO SETTINGS
	if(add_command(fd, (char *)CMD_AUDIO_SETTINGS,       0)) return fclose(fd);
	if(add_command(fd, (char *)CMD_CAPTURE_SAMPLE_RATE,  (char *) menuInSampleRate->value(),    1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_PLAYBACK_SAMPLE_RATE, (char *) menuOutSampleRate->value(),   1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_AUDIO_CONVERTER,      (char *) menuSampleConverter->value(), 1))	return fclose(fd);
	if(add_command(fd, (char *)CMD_RX_PPM,               (int)    cntRxRateCorr->value(),       1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_TX_PPM,               (int)    cntTxRateCorr->value(),       1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_TX_OFFSET,            (int)    cntTxOffset->value(),         1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_END_CMD,              0)) return fclose(fd);

	// AUDIO RIGHT CHANNEL
	if(add_command(fd, (char *)CMD_AUDIO_RT_CHANNEL,     0)) return fclose(fd);
	if(add_command(fd, (char *)CMD_AUDIO_L_R,            (bool)   chkAudioStereoOut->value(),   1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_AUDIO_REV_L_R,        (bool)   chkReverseAudio->value(),     1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_PTT_RIGHT_CHAN,       (bool)   btnPTTrightchannel2->value(), 1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_CW_QSK_RT_CHAN,       (bool)   btnQSK2->value(),             1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_PSEUDO_FSK_RT_CHAN,   (bool)   chkPseudoFSK2->value(),       1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_END_CMD,              0)) return fclose(fd);

	// AUDIO WAVE
	if(add_command(fd, (char *)CMD_AUDIO_WAVE,           0)) return fclose(fd);
	if(add_command(fd, (char *)CMD_WAVE_SR,              (char *) listbox_wav_samplerate->value(), 1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_END_CMD,              0)) return fclose(fd);

	// RIG HRDWR PTT
	if(add_command(fd, (char *)CMD_HRDWR_PTT,                0)) return fclose(fd);
	if(add_command(fd, (char *)CMD_HPPT_PTT_RT,              (bool)   btnPTTrightchannel->value(), 1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_HPTT_SP2,                 (bool)   btnTTYptt->value(),          1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_HPTT_SP2_PATH,            (char *) inpTTYdev->value(),          1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_HPTT_SP2_RTS,             (bool)   btnRTSptt->value(),          1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_HPTT_SP2_RTS_V,           (bool)   btnRTSplusV->value(),        1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_HPTT_SP2_DTR,             (bool)   btnDTRptt->value(),          1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_HPTT_SP2_DTR_V,           (bool)   btnDTRplusV->value(),        1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_HPTT_PARALLEL,            (bool)   btnUsePPortPTT->value(),     1)) return fclose(fd);
#if HAVE_UHROUTER
	if(add_command(fd, (char *)CMD_HPTT_UHROUTER,            (bool)   btnUseUHrouterPTT->value(),  1)) return fclose(fd);
#endif
	if(add_command(fd, (char *)CMD_HPTT_SP2_START_DELAY,     (int)    cntPTT_on_delay->value(),    1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_HPTT_SP2_END_DELAY,       (int)    cntPTT_off_delay->value(),   1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_HPTT_SP2_INITIALIZE,      1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_END_CMD,                  0)) return fclose(fd);

	// RIG CAT
	if(add_command(fd, (char *)CMD_RIGCAT,                   0)) return fclose(fd);
	if(add_command(fd, (char *)CMD_RIGCAT_STATE,             (bool)   chkUSERIGCAT->value(),               1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_RIGCAT_DEV_PATH,          (char *) inpXmlRigDevice->value(),            1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_RIGCAT_DESC_FILE,         (char *) progdefaults.XmlRigFilename.c_str(), 1))	return fclose(fd);
	if(add_command(fd, (char *)CMD_RIGCAT_RETRIES,           (int)    cntRigCatRetries->value(),           1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_RIGCAT_RETRY_INTERVAL,    (int)    cntRigCatTimeout->value(),           1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_RIGCAT_WRITE_DELAY,       (int)    cntRigCatWait->value(),              1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_RIGCAT_INTIAL_DELAY,      (int)    cntRigCatInitDelay->value(),         1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_RIGCAT_BAUD_RATE,         (char *) listbox_xml_rig_baudrate->value(),   1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_RIGCAT_STOP_BITS,         (int)    valRigCatStopbits->value(),          1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_RIGCAT_ECHO,              (bool)   btnRigCatEcho->value(),              1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_RIGCAT_TOGGLE_RTS_PTT,    (bool)   btnRigCatRTSptt->value(),            1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_RIGCAT_RESTORE,           (bool)   chk_restore_tio->value(),            1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_RIGCAT_PTT_COMMAND,       (bool)   btnRigCatCMDptt->value(),            1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_RIGCAT_TOGGLE_DTR_PTT,    (bool)   btnRigCatDTRptt->value(),            1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_RIGCAT_TOGGLE_RTS_PTT,    (bool)   btnRigCatRTSptt->value(),            1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_RIGCAT_RTS_12V,           (bool)   btnRigCatRTSplus->value(),           1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_RIGCAT_DTR_12V,           (bool)   btnRigCatDTRplus->value(),           1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_RIGCAT_HRDWR_FLOW,        (bool)   chkRigCatRTSCTSflow->value(),        1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_RIGCAT_VSP,               (bool)   chkRigCatVSP->value(),               1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_RIGCAT_INITIALIZE,        1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_END_CMD,                  0)) return fclose(fd);
#if USE_HAMLIB
	// HAMLIB
	if(add_command(fd, (char *)CMD_HAMLIB,                   0)) return fclose(fd);
	if(add_command(fd, (char *)CMD_HAMLIB_STATE,             (bool)   chkUSEHAMLIB->value(),         1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_HAMLIB_RIG,               (char *) cboHamlibRig->value(),         1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_HAMLIB_DEV_PATH,          (char *) inpRIGdev->value(),            1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_HAMLIB_RETRIES,           (int)    cntHamlibRetries->value(),     1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_HAMLIB_RETRY_INTERVAL,    (int)    cntHamlibTimeout->value(),     1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_HAMLIB_WRITE_DELAY,       (int)    cntHamlibWriteDelay->value(),  1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_HAMLIB_POST_WRITE_DELAY,  (int)    cntHamlibWait->value(),        1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_HAMLIB_BAUD_RATE,         (char *) listbox_baudrate->value(),     1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_HAMLIB_STOP_BITS,         (int)    valHamRigStopbits->value(),    1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_HAMLIB_POLL_RATE,         (int)    valHamRigPollrate->value(),    1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_HAMLIB_SIDE_BAND,         (char *) listbox_sideband->value(),     1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_HAMLIB_PTT_COMMAND,       (bool)   btnHamlibCMDptt->value(),      1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_HAMLIB_DTR_12V,           (bool)   btnHamlibDTRplus->value(),     1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_HAMLIB_RTS_12V,           (bool)   btnHamlibDTRplus->value(),     1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_HAMLIB_HRDWR_FLOW,        (bool)   chkHamlibRTSCTSflow->value(),  1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_HAMLIB_SFTWR_FLOW,        (bool)   chkHamlibXONXOFFflow->value(), 1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_HAMLIB_ADV_CONFIG,        (char *) inpHamlibConfig->value(),      1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_HAMLIB_INITIALIZE,        1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_END_CMD,                  0)) return fclose(fd);
#endif //#if USE_HAMLIB

	// XMLRPC RC
	if(add_command(fd, (char *)CMD_RC_XMLRPC,            0)) return fclose(fd);
	if(add_command(fd, (char *)CMD_RC_XMLRPC_STATE,      (bool)   chkUSEXMLRPC->value(), 1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_RC_XMLRPC_BW_DELAY,   (double) mbw_delay->value(),    1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_RC_XMLRPC_INITIALIZE, 1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_END_CMD,              0)) return fclose(fd);

	// IO Config Panel
	if(add_command(fd, (char *)CMD_IO,               0)) return fclose(fd);
	if(add_command(fd, (char *)CMD_IO_LOCK,          (bool)   btnDisable_p2p_io_widgets->value(), 1)) return fclose(fd);

	if(btnEnable_arq->value()) {
		if(add_command(fd, (char *)CMD_IO_ACT_PORT,  (char *) PARM_ARQ,  1)) return fclose(fd);
	} else if(btnEnable_kiss->value()) {
		if(add_command(fd, (char *)CMD_IO_ACT_PORT,  (char *) PARM_KISS, 1)) return fclose(fd);
	}

	if(add_command(fd, (char *)CMD_IO_AX25_DECODE,   (bool)   btnEnable_ax25_decode->value(),  1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_IO_CSMA,          (bool)   btnEnable_csma->value(),         1)) return fclose(fd);

	if(add_command(fd, (char *)CMD_IO_KISS,          1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_IO_KISS_IPA,      (char *) txtKiss_ip_address->value(),     2)) return fclose(fd);
	if(add_command(fd, (char *)CMD_IO_KISS_IOPN,     (char *) txtKiss_ip_io_port_no->value(),  2)) return fclose(fd);
	if(add_command(fd, (char *)CMD_IO_KISS_OPN,      (char *) txtKiss_ip_out_port_no->value(), 2)) return fclose(fd);
	if(add_command(fd, (char *)CMD_IO_KISS_DP,       (bool)   btnEnable_dual_port->value(),    2)) return fclose(fd);
	if(add_command(fd, (char *)CMD_IO_KISS_BUSY,     (bool)   btnEnableBusyChannel->value(),   2)) return fclose(fd);
	if(add_command(fd, (char *)CMD_IO_KISS_CONT,     (int)    cntBusyChannelSeconds->value(),  2)) return fclose(fd);
	if(add_command(fd, (char *)CMD_IO_KISS_ATTEN,    (int)    cntKPSQLAttenuation->value(),    2)) return fclose(fd);
	if(add_command(fd, (char *)CMD_END_CMD,          1)) return fclose(fd);

	if(add_command(fd, (char *)CMD_IO_ARQ,           1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_IO_ARQ_IPA,       (char *) txtArq_ip_address->value(),      2)) return fclose(fd);
	if(add_command(fd, (char *)CMD_IO_ARQ_IOPN,      (char *) txtArq_ip_port_no->value(),      2)) return fclose(fd);
	if(add_command(fd, (char *)CMD_END_CMD,          1)) return fclose(fd);

	if(add_command(fd, (char *)CMD_IO_XMLRPC,        1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_IO_XMLRPC_IPA,    (char *) txtXmlrpc_ip_address->value(),   2)) return fclose(fd);
	if(add_command(fd, (char *)CMD_IO_XMLRPC_IOPN,   (char *) txtXmlrpc_ip_port_no->value(),   2)) return fclose(fd);
	if(add_command(fd, (char *)CMD_END_CMD,          1)) return fclose(fd);

	if(add_command(fd, (char *)CMD_END_CMD,          0)) return fclose(fd);

	// NBEMS
	if(add_command(fd, (char *)CMD_NBEMS,            0)) return fclose(fd);
	if(add_command(fd, (char *)CMD_NBEMS_STATE,      (bool)   chkAutoExtract->value(),       1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_NBEMS_MSG,        (bool)   chk_open_wrap_folder->value(), 1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_NBEMS_FLMSG,      (bool)   chk_open_flmsg->value(),       1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_NBEMS_FLMSG_PATH, (char *) txt_flmsg_pathname->value(),   1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_NBEMS_BRWSR,      (bool)   chk_open_flmsg_print->value(), 1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_NBEMS_TIMEOUT,    (double) sldr_extract_timeout->value(), 1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_END_CMD,          0)) return fclose(fd);

	// ID/RSID/VIDEO/CW
	if(add_command(fd, (char *)CMD_ID,                    0)) return fclose(fd);
	if(add_command(fd, (char *)CMD_ID_RSID,               1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_ID_RSID_NOTIFY,        (bool)   chkRSidNotifyOnly->value(),    2)) return fclose(fd);
	if(add_command(fd, (char *)CMD_ID_RSID_SRCH_BP,       (bool)   chkRSidWideSearch->value(),    2)) return fclose(fd);
	if(add_command(fd, (char *)CMD_ID_RSID_MARK_PREV,     (bool)   chkRSidMark->value(),          2)) return fclose(fd);
	if(add_command(fd, (char *)CMD_ID_RSID_DETECTOR,      (bool)   chkRSidAutoDisable->value(),   2)) return fclose(fd);
	if(add_command(fd, (char *)CMD_ID_RSID_ALRT_DIALOG,   (bool)   chkRSidShowAlert->value(),     2)) return fclose(fd);
	if(add_command(fd, (char *)CMD_ID_RSID_TX_FREQ_LOCK,  (bool)   chkRetainFreqLock->value(),    2)) return fclose(fd);
	if(add_command(fd, (char *)CMD_ID_RSID_FREQ_CHANGE,   (bool)   chkDisableFreqChange->value(), 2)) return fclose(fd);
	if(add_command(fd, (char *)CMD_ID_RSID_ALLOW_ERRORS,  (char *) listbox_rsid_errors->value(),  2)) return fclose(fd);
	if(add_command(fd, (char *)CMD_ID_RSID_SQL_OPEN,      (int)    sldrRSIDsquelch->value(),      2)) return fclose(fd);
	if(add_command(fd, (char *)CMD_ID_RSID_PRETONE,       (double) val_pretone->value(),          2)) return fclose(fd);
	if(add_command(fd, (char *)CMD_ID_RSID_END_XMT_ID,    (bool)   btn_post_rsid->value(),        2)) return fclose(fd);
	if(add_command(fd, (char *)CMD_END_CMD,               1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_ID_VIDEO,              1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_ID_VIDEO_TX_ID_MODE,   (bool)   btnsendid->value(),            2)) return fclose(fd);
	if(add_command(fd, (char *)CMD_ID_VIDEO_TX_VIDEO_TXT, (bool)   btnsendvideotext->value(),     2)) return fclose(fd);
	if(add_command(fd, (char *)CMD_ID_VIDEO_TX_TXT_INP,   (char *) valVideotext->value(),         2)) return fclose(fd);
	if(add_command(fd, (char *)CMD_ID_VIDEO_SMALL_FONT,   (bool)   chkID_SMALL->value(),          2)) return fclose(fd);
	if(add_command(fd, (char *)CMD_ID_VIDEO_500_HZ,       (bool)   btn_vidlimit->value(),         2)) return fclose(fd);
	if(add_command(fd, (char *)CMD_ID_VIDEO_WIDTH_LIMIT,  (bool)   btn_vidmodelimit->value(),     2)) return fclose(fd);
	if(add_command(fd, (char *)CMD_ID_VIDEO_CHAR_ROW,     (int)    sldrVideowidth->value(),       2)) return fclose(fd);
	if(add_command(fd, (char *)CMD_END_CMD,               1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_ID_CW,                 1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_ID_CW_TX_CALLSIGN,     (bool)   btnCWID->value(),              2)) return fclose(fd);
	if(add_command(fd, (char *)CMD_ID_CW_SPEED,           (int)    sldrCWIDwpm->value(),          2)) return fclose(fd);
	if(add_command(fd, (char *)CMD_END_CMD,               1)) return fclose(fd);
	if(add_command(fd, (char *)CMD_END_CMD,               0)) return fclose(fd);

	// MACROS
	write_macro_list(fd);

	return fclose(fd);
}

/** ********************************************************
 * \brief Write the current macro list in script format
 * \param fd File descriptor
 * \param cmd Pointer to the file name and path.
 * \return 0 OK, other Error
 ***********************************************************/
static void write_macro_list(FILE *fd)
{
	if(!fd) return;

	int index = 0;
	int row = 0;
	int col = 0;
	bool save_flag = false;
	int count = 0;
	char * cPtr = (char *)0;

	for(index = 0; index < MAXMACROS; index++) {

		// Do not save empty macros
		count = macros.text[index].size();
		if(count < 1) continue;
		cPtr = (char *) macros.text[index].c_str();
		while(count-- > 0) {
			if(*cPtr++ > ' ') {
				save_flag = true;
				break;
			}
		}

		if(save_flag) {
			col = (index / NUMMACKEYS) + 1;
			row = (index % NUMMACKEYS) + 1;
			fprintf(fd, "\n%s:%d,%d,\"%s\"\n", (char *) CMD_LOAD_MACRO, col, row, macros.name[index].c_str());
			fprintf(fd, "%s\n", macros.text[index].c_str());
			fprintf(fd, "%s:\n",(char *)  CMD_END_CMD);
			save_flag = false;
		}
	}
}

/** ********************************************************
 * \brief Add command and paramter to script file
 * \param fd File descriptor
 * \param cmd Pointer to the file name and path.
 * \return 0 OK, other Error
 ***********************************************************/
static int add_command(FILE *fd, char *cmd, double param, int indent_level)
{
	if(!fd || !cmd)
		return -1;

	char buffer[32];
	memset(buffer, 0, sizeof(buffer));

	if((param > -1000.0) && (param < 1000.0))
		snprintf(buffer, sizeof(buffer) - 1, "%1.3f", param);
	else
		snprintf(buffer, sizeof(buffer) - 1, "%1.9e", param);

	return add_command(fd, cmd, buffer, indent_level);
}

/** ********************************************************
 * \brief Add command and paramter to script file
 * \param fd File descriptor
 * \param cmd Pointer to the file name and path.
 * \return 0 OK, other Error
 ***********************************************************/
static int add_command(FILE *fd, char *cmd, bool param, int indent_level)
{
	if(!fd || !cmd)
		return -1;

	char buffer[32];
	memset(buffer, 0, sizeof(buffer));

	if(param)
		strncpy(buffer, PARM_ENABLE, sizeof(buffer)-1);
	else
		strncpy(buffer, PARM_DISABLE, sizeof(buffer)-1);

	return add_command(fd, cmd, buffer, indent_level);
}

/** ********************************************************
 * \brief Add command and paramter to script file
 * \param fd File descriptor
 * \param cmd Pointer to the file name and path.
 * \return 0 OK, other Error
 ***********************************************************/
static int add_command(FILE *fd, char *cmd, int param, int indent_level)
{
	if(!fd || !cmd)
		return -1;

	char buffer[32];
	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer) - 1, "%d", param);

	return add_command(fd, cmd, buffer, indent_level);
}

/** ********************************************************
 * \brief Add command and paramter to script file
 * \param fd File descriptor
 * \param cmd Pointer to the file name and path.
 * \return 0 OK, other Error
 ***********************************************************/
static int add_command(FILE *fd, char *cmd, char *param, int indent_level)
{
	if(!fd || !cmd || !param)
		return -1;

	bool q_flag = false;
	int size = strnlen(param, FILENAME_MAX);

	if(size < 1) return 0;

	for(int i = 0; i < size; i++) {
		if(param[i] == 0) break;
		if((param[i] == ',') || (param[i] <= ' ')) {
			q_flag = true;
		}
	}

	std::string indent = "";
	for(int i = 0; i < indent_level; i++)
		indent.append("   ");

	if(q_flag)
		fprintf(fd, "%s%s:\"%s\"\n", indent.c_str(), cmd, param);
	else
		fprintf(fd, "%s%s:%s\n", indent.c_str(), cmd, param);

	return ferror(fd);
}

/** ********************************************************
 * \brief Add command and paramter to script file
 * \param fd File descriptor
 * \param cmd Pointer to the file name and path.
 * \return 0 OK, other Error
 ***********************************************************/
static int add_command(FILE *fd, char *cmd, int indent_level)
{
	if(!fd || !cmd)
		return -1;

	std::string indent = "";
	for(int i = 0; i < indent_level; i++)
		indent.append("   ");

	fprintf(fd, "%s%s:\n", indent.c_str(), cmd);

	return ferror(fd);
}

/** ********************************************************
 * \brief Add command and paramter to script file
 * \param fd File descriptor
 * \param cmd Pointer to the file name and path.
 * \return 0 OK, other Error
 ***********************************************************/
static int add_string(FILE *fd, char *cmd, int indent_level)
{
	if(!fd || !cmd)
		return -1;

	std::string indent = "";
	for(int i = 0; i < indent_level; i++)
		indent.append("   ");

	fprintf(fd, "%s%s\n", indent.c_str(), cmd);

	return ferror(fd);
}

