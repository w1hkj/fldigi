// ----------------------------------------------------------------------------
// configuration.h
//
// Copyright (C) 2006-2010
//		Dave Freese, W1HKJ
// Copyright (C) 2008-2010
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

#ifndef _CONFIGURATION_H
#define _CONFIGURATION_H

#include <string>
#include <math.h>

#include "rtty.h"
#include "waterfall.h"
#include "lookupcall.h"
#include "psk_browser.h"

#if defined(__linux__)
#  define DEFAULT_PTTDEV "/dev/ttyS0"
#  define DEFAULT_CWFSKPORT "/dev/ttyS1"
#  define DEFAULT_HAMRIGDEVICE "/dev/ttyS0"
#elif defined(__FreeBSD__)
#  define DEFAULT_PTTDEV "/dev/ttyd0"
#  define DEFAULT_CWFSKPORT "/dev/ttyd1"
#  define DEFAULT_HAMRIGDEVICE "/dev/ttyd0"
#elif defined(__NetBSD__)
#  define DEFAULT_PTTDEV "/dev/tty00"
#  define DEFAULT_CWFSKPORT "/dev/tty01"
#  define DEFAULT_HAMRIGDEVICE "/dev/tty00"
#elif defined(__OpenBSD__)
#  define DEFAULT_PTTDEV "/dev/tty00"
#  define DEFAULT_CWFSKPORT "/dev/tty01"
#  define DEFAULT_HAMRIGDEVICE "/dev/tty00"
#elif defined(__WOE32__)
#  define DEFAULT_PTTDEV "COM1"
#  define DEFAULT_CWFSKPORT "COM2"
#  define DEFAULT_HAMRIGDEVICE "COM1"
#else // not sure
#  define DEFAULT_PTTDEV "/dev/ptt"
#  define DEFAULT_CWFSKPORT "/dev/fsk"
#  define DEFAULT_HAMRIGDEVICE "/dev/rig"
#endif


// Format: ELEM_(TYPE, VARIABLE-NAME, TAG-STRING, DOC-STRING, DEFAULT-VALUE)
// Variables that are not saved to the xml file have empty TAG-STRINGs and DOC-STRINGs
//
// No preprocessor directives or C++ comments inside this macro!
// Indent with spaces only.

#ifdef ELEM_
#  error ELEM_ should not be defined at this point
#endif

#define CONFIG_LIST                                                                     \
    ELEM_(bool, confirmExit, "CONFIRMEXIT",                                             \
          "Ensure user wants to leave flgidi",                                          \
          false)                                                                        \
        ELEM_(bool, SaveConfig, "SAVECONFIG",                                           \
              "Save current configuration on exit",                                     \
              false)                                                                    \
        ELEM_(bool, noise, "NOISETEST",                                                 \
              "Noise test on/off",                                                      \
              false)                                                                    \
        ELEM_(double, s2n, "SIGNAL2NOISE",                                              \
              "Signal to Noise ratio for test",                                         \
              +20.0)                                                                    \
        ELEM_(bool, rsidWideSearch, "RSIDWIDESEARCH",                                   \
              "RSID detector searches the entire passband",                             \
              false)                                                                    \
        ELEM_(int, rsid_squelch, "RSIDSQUELCH",                                         \
              "RSID detection opens squelch for nn seconds",                            \
              5)                                                                        \
        ELEM_(bool, rsid, "RECEIVERSID",                                                \
              "Enable Reed Soloman ID decoding",                                        \
              false)                                                                    \
        ELEM_(bool, TransmitRSid, "TRANSMITRSID",                                       \
              "Send RSID at beginning and end of transmission",                         \
              false)                                                                    \
        ELEM_(bool, rsid_mark, "RSIDMARK",                                              \
              "Append marker (for the previous modem and frequency) to the RX text\n"   \
              "widget before changing to the new modem and/or frequency",               \
              true)                                                                     \
        ELEM_(bool, rsid_notify_only, "RSIDNOTIFYONLY",                                 \
              "Trigger RSID notifications but do not change modem and frequency",       \
              false)                                                                    \
        ELEM_(bool, rsid_auto_disable, "RSIDAUTODISABLE",                               \
              "Disable RSID detection when RsID signal is detected",                    \
              false)                                                                    \
        ELEM_(bool, rsid_post, "RSIDPOST",                                              \
              "Transmit an RSID signal when modem data is concluded",                   \
              false)                                                                    \
        ELEM_(mode_set_t, rsid_rx_modes, "RSIDRXMODESEXCLUDE",                          \
              "Mode names for which RSID reception is disabled",                        \
              mode_set_t())                                                             \
        ELEM_(mode_set_t, rsid_tx_modes, "RSIDTXMODESEXCLUDE",                          \
              "Mode names for which RSID transmission is disabled",                     \
              mode_set_t())                                                             \
                                                                                        \
        ELEM_(int, RsID_label_type, "RSID_ERRORS",                                      \
              "values (low, medium, high)  0, 1, 2",                                    \
              1)                                                                        \
        ELEM_(bool, disable_rsid_warning_dialog_box, "DISABLE_RSID_WARNING_DIALOG_BOX", \
              "disable displaying the rsid warning dialog box",                         \
              false)                                                                    \
        ELEM_(bool, slowcpu, "SLOWCPU",                                                 \
              "Disable expensive processing in some decoders",                          \
              true)                                                                     \
        ELEM_(bool, disable_rsid_freq_change, "DISABLERSIDFREQCHANGE",                  \
              "disable changing frequency on rsid modem change/reset",                  \
              false)                                                                    \
		ELEM_(bool, retain_freq_lock, "RETAINFREQLOCK",                                 \
			 "retain frequency lock on rsid modem change/reset",                   \
			 false)                                                                     \
        ELEM_(bool, changed, "", "",  false)                                            \
                                                                                        \
        ELEM_(double, wfRefLevel, "WFREFLEVEL",                                         \
              "Waterfall reference level (dB)",                                         \
              -20.0)                                                                    \
        ELEM_(double, wfAmpSpan, "WFAMPSPAN",                                           \
              "Waterfall amplitude span (dB)",                                          \
              70.0)                                                                     \
        ELEM_(bool, WF_UIrev, "WF_UIREV",                                               \
              "WF_UI - enable reverse button",                                          \
              false)                                                                    \
        ELEM_(bool, WF_UIx1, "WF_UIX1",                                                 \
              "WF_UI - enable scale multiplication button",                             \
              false)                                                                    \
        ELEM_(bool, WF_UIwfcarrier, "WF_UIWFCARRIER",                                   \
              "WF_UI - enable wf carrier button",                                       \
              false)                                                                    \
        ELEM_(bool, WF_UIwfshift, "WF_UIWFSHIFT",                                       \
              "WF_UI - enable wf shift buttons",                                        \
              false)                                                                    \
        ELEM_(bool, WF_UIwfreflevel, "WF_UIWFREFLEVEL",                                 \
              "WF_UI - enable rf level and range controls",                             \
              false)                                                                    \
        ELEM_(bool, WF_UIwfdrop, "WF_UIWFDROP",                                         \
              "WF_UI - enable wf drop rate control",                                    \
              false)                                                                    \
        ELEM_(bool, WF_UIwfampspan, "WF_UIWFAMPSPAN",                                   \
              "WF_UI - enable wf amp span control",                                     \
              false)                                                                    \
        ELEM_(bool, WF_UIwfstore, "WF_UIWFSTORE",                                       \
              "WF_UI - enable wf memory store button",                                  \
              false)                                                                    \
        ELEM_(bool, WF_UIwfmode, "WF_UIWFMODE",                                         \
              "WF_UI - enable wf mode control",                                         \
              false)                                                                    \
        ELEM_(bool, WF_UIqsy, "WF_UIQSY",                                               \
              "WF_UI - enable wf qsy button",                                           \
              false)                                                                    \
        ELEM_(bool, WF_UIxmtlock, "WF_UIXMTLOCK",                                       \
              "WF_UI - enable wf transmit lock button",                                 \
              false)                                                                    \
        ELEM_(int, LowFreqCutoff, "LOWFREQCUTOFF",                                      \
              "Lowest frequency shown on waterfall (Hz)",                               \
              0)                                                                        \
        ELEM_(int,  HighFreqCutoff, "HIGHFREQCUTOFF",                                   \
              "Highest frequency shown on waterfall (Hz)",                              \
              3000)                                                                     \
        ELEM_(double, CWsweetspot, "CWSWEETSPOT",                                       \
              "Default CW tracking point (Hz)",                                         \
              1000)                                                                     \
        ELEM_(double, RTTYsweetspot, "RTTYSWEETSPOT",                                   \
              "Default RTTY tracking point (Hz)",                                       \
              1000)                                                                     \
        ELEM_(double, PSKsweetspot, "PSKSWEETSPOT",                                     \
              "Default tracking point for all other modems (Hz)",                       \
              1000)                                                                     \
        ELEM_(bool, StartAtSweetSpot, "STARTATSWEETSPOT",                               \
              "Always start new modems at sweet spot frequencies",                      \
              false)                                                                    \
        ELEM_(bool, CWOffset, "CWOFFSET",                                               \
              "Select if waterfall should compensate for BFO offset in CW",             \
              false)                                                                    \
        ELEM_(bool, CWIsLSB, "CWISLSB",                                                 \
              "Select if BFO is injected as LSB instead of USB",                        \
              false)                                                                    \
        ELEM_(bool, WaterfallHistoryDefault, "WATERFALLHISTORYDEFAULT",                 \
              "Replay audio history when changing frequency by clicking on\n"           \
              "the waterfall",                                                          \
              false)                                                                    \
        ELEM_(bool, WaterfallQSY, "WATERFALLQSY",                                       \
              "Change rig frequency by dragging the mouse cursor on the waterfall\n"    \
              "frequency scale area",                                                   \
              false)                                                                    \
        ELEM_(bool, WaterfallClickInsert, "WATERFALLCLICKINSERT",                       \
              "Insert text to the RX text widget when changing frequency by left\n"     \
              "clicking on the waterfall",                                              \
              false)                                                                    \
        ELEM_(std::string, WaterfallClickText, "WATERFALLCLICKTEXT",                    \
              "Waterfall left click text for WATERFALLCLICKINSERT",                     \
              "\n<FREQ>\n")                                                             \
        ELEM_(int, WaterfallWheelAction, "WATERFALLWHEELACTION",                        \
              "Describes how waterfall mouse wheel events are handled\n"                \
              "  0: do nothing; 1: change AFC width or decoder bandwidth;\n"            \
              "  2: signal search; 3: change squelch level; 4: change modem carrier;\n" \
              "  5: change modem; 6: scroll visible area.  The default is 4.",          \
              waterfall::WF_CARRIER)                                                    \
        ELEM_(bool, rx_lowercase, "RX_LOWERCASE",                                       \
              "Print Rx in lowercase for CW, RTTY, CONTESTIA and THROB",                \
              false)                                                                    \
        ELEM_(bool, tx_lowercase, "TX_LOWERCASE",                                       \
              "Transmit all text in lowercase",                                         \
              false)                                                                    \
        /* PSK, filter can be 0, 1, 2, 3 or 4 */                                        \
        ELEM_(int, PSK_filter, "PSKFILTER",                                             \
              "Not configurable; must always be 0",                                     \
              0)                                                                        \
        ELEM_(bool, pskbrowser_on, "PSKBROWSER_ON",                                     \
              "Enable psk multi-channel detector - disable for very slow cpus",         \
              true)                                                                     \
        /* PSK / PSKmail interface */                                                   \
        ELEM_(int, SearchRange, "PSKSEARCHRANGE",                                       \
              "PSK signal acquisition search range (Hz)",                               \
              50)                                                                       \
        ELEM_(double, ACQsn, "ACQSN",                                                   \
              "PSK signal acquisition S/N (dB)",                                        \
              9.0)                                                                      \
        ELEM_(bool, Pskmails2nreport, "PSKMAILS2NREPORT",                               \
              "Send s2n report to pskmail client/server",                               \
              false)                                                                    \
        ELEM_(bool, StatusDim, "STATUSDIM",                                             \
              "Behaviour of status (S/N and IMD) fields:\n"                             \
              "  0: Clear after timeout\n"                                              \
              "  1: Dim after timeout\n",                                               \
              true)                                                                     \
        ELEM_(double, StatusTimeout, "STATUSTIMEOUT",                                   \
              "Dim or Clear timeout (seconds)",                                         \
              15.0)                                                                     \
        ELEM_(bool, PSKmailSweetSpot, "PSKMAILSWEETSPOT",                               \
              "Reset to carrier when no signal is present",                             \
              false)                                                                    \
        ELEM_(int, ServerOffset, "PSKSERVEROFFSET",                                     \
              "Listen for signals within this range (Hz)",                              \
              50)                                                                       \
        ELEM_(int, ServerCarrier, "PSKSERVERCARRIER",                                   \
              "Default PSKMail listen / transmit frequency",                            \
              1500)                                                                     \
        ELEM_(int, ServerAFCrange, "PSKSERVERAFCRANGE",                                 \
              "Limit AFC movement to this range (Hz)",                                  \
              25)                                                                       \
        ELEM_(double, ServerACQsn, "PSKSERVERACGSN",                                    \
              "Acquisition S/N (dB)",                                                   \
              9.0)                                                                      \
        /* RTTY */                                                                      \
        ELEM_(int, kahn_demod, "KAHNDEMOD",                                             \
              "1 - use Kahn power demodulator\n"                                        \
              "0 - use ATC (Kok Chen) demodulator",                                     \
              1)                                                                        \
        ELEM_(bool, true_scope, "TRUESCOPE",                                            \
              "Enabled  - XY scope displays Mark/Space channel signals\n"               \
              "Disabled - XY scope displays pseudo M/S signals",                        \
              true)                                                                     \
        ELEM_(int, rtty_shift, "RTTYSHIFT",                                             \
              "Carrier shift (Hz). Values are as follows:\n"                            \
              "  0: 23; 1: 85; 2: 160; 3: 170; 4: 182; 5: 200; 6: 240; 7: 350;\n"       \
              "  8: 425; 9: 850; -1: custom",                                           \
              3) /* 170 */                                                              \
        ELEM_(int, rtty_custom_shift, "RTTYCUSTOMSHIFT",                                \
              "Custom shift (Hz)",                                                      \
              450)                                                                      \
        ELEM_(double, RTTY_BW, "RTTYBW",                                                \
              "Receive filter bandwidth (Hz)",                                          \
              68.0)                                                                     \
        ELEM_(int, rtty_cwi, "RTTYCWI",                                                 \
              "Selective decoding of mark/space tones\n"                                \
              "0 - both\n"                                                              \
              "1 - mark only\n"                                                         \
              "2 - space only",                                                         \
              0)                                                                        \
        ELEM_(double, rtty_filter, "RTTYFILTER",                                        \
              "Rtty Rx Filter shape factor, K * (t/T)\n"                                \
              "You may alter this value using a text editor\n"                          \
              "change will be effective when restarting fldigi\n"                       \
              "K = 1.25; best for W1HKJ (default)\n"                                    \
              "K = 1.5 - best for DO2SMF",                                              \
              1.25)                                                                     \
        ELEM_(int, rtty_baud, "RTTYBAUD",                                               \
              "Carrier baud rate. Values are as follows:\n"                             \
              "  1: 45; 1: 45.45; 2: 50; 3: 56; 4: 75; 5: 100; 6: 110; 7: 150; \n"      \
              "  8: 200; 9: 300",                                                       \
              0)   /* 45 */                                                             \
        ELEM_(int, rtty_bits, "RTTYBITS",                                               \
              "Bits per character. Values are as follows:\n"                            \
              "  0: 5 (baudot); 1: 7 (ascii); 2: 8 (ascii)",                            \
              0)   /* 5 */                                                              \
        ELEM_(int, rtty_parity, "RTTYPARITY",                                           \
              "Parity. Values are as folows:\n"                                         \
              "  0: none; 1: even; 2: odd: 3: zero; 4: one",                            \
              RTTY_PARITY_NONE)                                                         \
        ELEM_(int, rtty_stop, "RTTYSTOP",                                               \
              "Stop bits. Values are as folows:\n"                                      \
              "  0: 1; 1: 1.5; 2: 2",                                                   \
              1)   /* 1.5 */                                                            \
        ELEM_(bool, rtty_reverse, "RTTYREVERSE",                                        \
              "This setting is currently unused",                                       \
              false)                                                                    \
        ELEM_(bool, rtty_msbfirst, "RTTYMSBFIRST",                                      \
              "This setting is currently unused",                                       \
              false)                                                                    \
        ELEM_(bool, rtty_crcrlf, "RTTYCRCLF",                                           \
              "Use \"CR CR LF\" for \"CR LF\"",                                         \
              false)                                                                    \
        ELEM_(bool, rtty_autocrlf, "RTTYAUTOCRLF",                                      \
              "Automatically add CRLF after `page width' characters",                   \
              true)                                                                     \
        ELEM_(int, rtty_autocount, "RTTYAUTOCOUNT",                                     \
              "Page width for RTTYAUTOCRLF",                                            \
              72)                                                                       \
        ELEM_(int, rtty_afcspeed, "RTTYAFCSPEED",                                       \
              "AFC tracking speed. Values are as follows:\n"                            \
              "  0: slow; 1: normal; 2: fast",                                          \
              1)   /* normal */                                                         \
        ELEM_(bool, useFSKkeyline, "", "",  false)                                      \
        ELEM_(bool, useFSKkeylineDTR, "", "",  false)                                   \
        ELEM_(bool, FSKisLSB, "", "",  true)                                            \
        ELEM_(bool, useUART, "", "",  false)                                            \
        ELEM_(bool, PreferXhairScope, "PREFERXHAIRSCOPE",                               \
              "Default to crosshair digiscope",                                         \
              false)                                                                    \
        ELEM_(bool, PseudoFSK, "PSEUDOFSK",                                             \
              "Generate Pseudo-FSK signal on right audio channel",                      \
              false)                                                                    \
        ELEM_(bool, SynopAdifDecoding, "SYNOPADIFDECODING",                             \
              "Decoding of Synop weather information on RTTY to ADIF log",              \
              false)                                                                    \
        ELEM_(bool, SynopKmlDecoding, "SYNOPKMLDECODING",                               \
              "Decoding of Synop weather information on RTTY to KML file",              \
              false)                                                                    \
        ELEM_(bool, SynopInterleaved, "SYNOPINTERLEAVED",                               \
              "Decoding of Synop interleaved with coded text, or replaces it",          \
              false)                                                                    \
        ELEM_(bool, UOSrx, "UOSRX",                                                     \
              "Revert to unshifted chars on a space (RX)",                              \
              true)                                                                     \
        ELEM_(bool, UOStx, "UOSTX",                                                     \
              "Revert to unshifted chars on a space (TX)",                              \
              true)                                                                     \
        ELEM_(bool, useMARKfreq, "USEMARKFREQ",                                         \
              "Use MARK frequency for logging",                                         \
              true)                                                                     \
        /* CW */                                                                        \
        ELEM_(bool, useCWkeylineRTS, "", "",  false)                                    \
        ELEM_(bool, useCWkeylineDTR, "", "",  false)                                    \
        ELEM_(int, CWweight, "CWWEIGHT",                                                \
              "Dot to dot-space ratio",                                                 \
              50)                                                                       \
        ELEM_(int, CWspeed, "CWSPEED",                                                  \
              "Transmit speed (WPM)",                                                   \
              18)                                                                       \
        ELEM_(int, CWfarnsworth, "CWFARNSWORTH",                                        \
              "Speed for Farnsworth timing (WPM)",                                      \
              18)                                                                       \
        ELEM_(bool, CWusefarnsworth, "CWUSEFARNSWORTH",                                 \
              "Use Farnsworth timing",                                                  \
              false)                                                                    \
        ELEM_(int, defCWspeed, "CWDEFSPEED",                                            \
              "Default speed (WPM)",                                                    \
              24)                                                                       \
        ELEM_(int, CWbandwidth, "CWBANDWIDTH",                                          \
              "Filter bandwidth (Hz)",                                                  \
              150)                                                                      \
        ELEM_(double, CWlower, "CWLOWER",                                               \
              "Detector hysterisis, lower threshold",                                   \
              0.4)                                                                      \
        ELEM_(double, CWupper, "CWUPPER",                                               \
              "Detector hysterisis, upper threshold",                                   \
              0.6)                                                                      \
        ELEM_(int, CWmfiltlen, "CWMFILTLEN",                                            \
              "Matched Filter length",                                                  \
              100)                                                                      \
        ELEM_(bool, CWtrack, "CWTRACK",                                                 \
              "Automatic receive speed tracking",                                       \
              true)                                                                     \
        ELEM_(bool, CWmfilt, "CWMFILT",                                                 \
              "Matched Filter in use",                                                  \
              false)                                                                    \
        ELEM_(bool, CWuse_fft_filter, "CWUSEFFTFILTER",                                 \
              "Use FFT overlap and add convolution filter",                             \
              false)                                                                    \
        ELEM_(bool, CWuseSOMdecoding, "CWUSESOMDECODING",                               \
              "Self Organizing Map decoding",                                           \
              false)                                                                    \
        ELEM_(int, CWrange, "CWRANGE",                                                  \
              "Tracking range for CWTRACK (WPM)",                                       \
              10)                                                                       \
        ELEM_(int, CWlowerlimit, "CWLOWERLIMIT",                                        \
              "Lower RX limit (WPM)",                                                   \
              5)                                                                        \
        ELEM_(int, CWupperlimit, "CWUPPERLIMIT",                                        \
              "Upper TX limit (WPM)",                                                   \
              50)                                                                       \
        ELEM_(double, CWrisetime, "CWRISETIME",                                         \
              "Leading and trailing edge rise times (milliseconds)",                    \
              4.0)                                                                      \
        ELEM_(double, CWdash2dot, "CWDASH2DOT",                                         \
              "Dash to dot ratio",                                                      \
              3.0)                                                                      \
        ELEM_(bool, QSK, "QSK",                                                         \
              "Generate QSK signal on right audio channel",                             \
              false)                                                                    \
        ELEM_(double, CWpre, "CWPRE",                                                   \
              "Pre-keydown timing (milliseconds)",                                      \
              4.0)                                                                      \
        ELEM_(double, CWpost, "CWPOST",                                                 \
              "Post-keydown timing (milliseconds)",                                     \
              4.0)                                                                      \
        ELEM_(bool, CWid, "CWID",                                                       \
              "Send callsign in CW at the end of every transmission",                   \
              false)                                                                    \
        ELEM_(int, CWIDwpm, "IDWPM",                                                    \
              "CW ID speed (WPM)",                                                      \
              18)                                                                       \
        ELEM_(mode_set_t, cwid_modes, "CWIDMODESEXCLUDE",                               \
              "Mode names for which CWID transmission is disabled",                     \
              mode_set_t())                                                             \
        ELEM_(bool, QSKadjust, "QSKADJUST",                                             \
              "Send a continuous stream of test characters as the QSK signal",          \
              false)                                                                    \
        ELEM_(int, TestChar, "TESTCHAR",                                                \
              "Test character for QSKADJUST (ASCII value)",                             \
              0)                                                                        \
        ELEM_(int, QSKshape, "QSKSHAPE",                                                \
              "QSK edge shape. Values are as follows:\n"                                \
              "  0: Hanning; 1: Blackman.\n"                                            \
              "Raised cosine = Hanning.\n",                                             \
              0)   /* Hanning */                                                        \
        ELEM_(bool, CWnarrow, "CWNARROW",                                               \
              "Weight decreases with increasing edge timing",                           \
              false)                                                                    \
        ELEM_(bool, CW_use_paren, "CWUSEPAREN",                                         \
              "Use open paren character; typically used in MARS ops",                   \
              false)                                                                    \
        ELEM_(std::string, CW_prosigns, "CWPROSIGNS",                                   \
              "CW prosigns BT AA AS AR SK KN INT HM VE",                                \
              "=~<>%+&{}")                                                              \
        /* FELD HELL */                                                                 \
        ELEM_(double, HELL_BW, "HELL_BW0", "Feld Hell working bandwidth",  245.0)       \
        ELEM_(double, HELL_BW_FH, "HELL_BW1", "FH bandwidth",  245.0)                   \
        ELEM_(double, HELL_BW_SH, "HELL_BW2", "Slow Hell bandwidth",  30.0)             \
        ELEM_(double, HELL_BW_X5, "HELL_BW3", "X5 Hell bandwidth",  1225.)              \
        ELEM_(double, HELL_BW_X9, "HELL_BW4", "X9 Hell bandwidth",  2205.0)             \
        ELEM_(double, HELL_BW_FSK, "HELL_BW5", "FSK Hell bandwidth",  180.0)            \
        ELEM_(double, HELL_BW_FSK105, "HELL_BW6", "FSK105 Hell bandwidth",  100.0)      \
        ELEM_(double, HELL_BW_HELL80, "HELL_BW7", "HELL80 bandwidth",  450.0)           \
        ELEM_(bool, HellRcvWidth, "HELLRCVWIDTH",                                       \
              "Halve receive width (compress RX in time)",                              \
              false)                                                                    \
        ELEM_(bool, HellBlackboard, "HELLBLACKBOARD",                                   \
              "Display RX in reverse video",                                            \
              false)                                                                    \
        ELEM_(int, HellXmtWidth, "HELLXMTWIDTH",                                        \
              "Transmit width (number of multiple scans per character line)",           \
              1)                                                                        \
        ELEM_(bool, HellXmtIdle, "HELLXMTIDLE",                                         \
              "Transmit periods (.) when idle",                                         \
              false)                                                                    \
        ELEM_(bool, HellPulseFast, "HELLPULSEFAST",                                     \
              "Raised cosine pulse shape factor. Values are as follows:\n"              \
              "  0: slow (4 ms); 1: fast (2 ms).",                                      \
              false)   /* slow */                                                       \
        /* OLIVIA */                                                                    \
        ELEM_(int, oliviatones, "OLIVIATONES",                                          \
              "Number of tones. Values are as follows:\n"                               \
              "  0: 2; 1: 4; 2: 8; 3: 16; 4: 32; 5: 64; 6: 128; 7: 256",                \
              2)   /* 8 */                                                              \
        ELEM_(int, oliviabw, "OLIVIABW",                                                \
              "Bandwidth (Hz). Values are as follows:\n"                                \
              "  0: 125; 1: 250; 2: 500; 3: 1000; 4: 2000.",                            \
              2)   /* 500 */                                                            \
        ELEM_(int, oliviasmargin, "OLIVIASMARGIN",                                      \
              "Tune margin (tone frequency spacing)",                                   \
              8)                                                                        \
        ELEM_(int, oliviasinteg, "OLIVIASINTEG",                                        \
              "Integration period (FEC blocks)",                                        \
              4)                                                                        \
        ELEM_(bool, olivia_reset_fec, "OLIVIARESETFEC",                                 \
              "Force Integration (FEC) depth to be reset when new BW/Tones selected",   \
              false)                                                                    \
        ELEM_(bool, olivia8bit, "OLIVIA8BIT",                                           \
              "8-bit extended characters",                                              \
              true)                                                                     \
        /* CONTESTIA */                                                                 \
        ELEM_(int, contestiatones, "CONTESTIATONES",                                    \
              "Number of tones. Values are as follows:\n"                               \
              "  0: 2; 1: 4; 2: 8; 3: 16; 4: 32; 5: 64; 6: 128; 7: 256",                \
              2)   /* 8 */                                                              \
        ELEM_(int, contestiabw, "CONTESTIABW",                                          \
              "Bandwidth (Hz). Values are as follows:\n"                                \
              "  0: 125; 1: 250; 2: 500; 3: 1000; 4: 2000.",                            \
              2)   /* 500 */                                                            \
        ELEM_(int, contestiasmargin, "CONTESTIASMARGIN",                                \
              "Tune margin (tone frequency spacing)",                                   \
              8)                                                                        \
        ELEM_(int, contestiasinteg, "CONTESTIASINTEG",                                  \
              "Integration period (FEC blocks)",                                        \
              4)                                                                        \
        ELEM_(bool, contestia8bit, "CONTESTIA8BIT",                                     \
              "8-bit extended characters",                                              \
              true)                                                                     \
		ELEM_(bool, contestia_reset_fec, "CONTESTIARESETFEC",                           \
		      "Force Integration (FEC) depth to be reset when new BW/Tones selected",   \
			  false)                                                                    \
        /* THOR */                                                                      \
        ELEM_(double, THOR_BW, "THORBW",                                                \
              "Filter bandwidth factor (bandwidth relative to signal width)",           \
              2.0)                                                                      \
        ELEM_(bool, THOR_FILTER, "THORFILTER",                                          \
              "Enable filtering before decoding",                                       \
              true)                                                                     \
        ELEM_(std::string, THORsecText, "THORSECTEXT",                                  \
              "Secondary text (sent during keyboard idle times)",                       \
              "")                                                                       \
        ELEM_(int, THOR_PATHS, "THORPATHS",                                             \
              "This setting is currently unused",                                       \
              5)                                                                        \
        ELEM_(double, ThorCWI, "THORCWI",                                               \
              "CWI threshold (CWI detection and suppression)",                          \
              0.0)                                                                      \
        ELEM_(bool, THOR_PREAMBLE, "THORPREAMBLE",                                      \
              "Detect THOR preamble (and flush Rx pipeline)",                           \
              true)                                                                     \
        ELEM_(bool, THOR_SOFTSYMBOLS, "THORSOFTSYMBOLS",                                \
              "Enable Soft-symbol decoding",                                            \
              true)                                                                     \
        ELEM_(bool, THOR_SOFTBITS, "THORSOFTBITS",                                      \
              "Enable Soft-bit decoding",                                               \
              true)                                                                     \
        /* PACKET */                                                                    \
        ELEM_(int, PKT_BAUD_SELECT, "PKTBAUDSELECT",                                    \
              "Packet baud rate. Values are as follows:\n"                              \
              "  0: 1200 (V/UHF); 1: 300 (HF); 2: 2400 (V/UHF)",                        \
              0)   /* 1200 baud (V/UHF) default. */                                     \
        ELEM_(double, PKT_LOSIG_RXGAIN, "LOSIGRXGAIN",                                  \
              "Signal gain for lower frequency (Mark) tone (in dB)",                    \
              0.0)                                                                      \
        ELEM_(double, PKT_HISIG_RXGAIN, "HISIGRXGAIN",                                  \
              "Signal gain for higher frequency (Space) tone (in dB)",                  \
              0.0)                                                                      \
        ELEM_(double, PKT_LOSIG_TXGAIN, "LOSIGTXGAIN",                                  \
              "Signal gain for Mark (lower frequency) tone (in dB)",                    \
              0.0)                                                                      \
        ELEM_(double, PKT_HISIG_TXGAIN, "HISIGTXGAIN",                                  \
              "Signal gain for Space (higher frequency) tone (in dB)",                  \
              0.0)                                                                      \
        ELEM_(bool, PKT_PreferXhairScope, "PKTPREFERXHAIRSCOPE",                        \
              "Default to syncscope (detected symbol scope)",                           \
              false)                                                                    \
        ELEM_(bool, PKT_AudioBoost, "PKTAUDIOBOOST",                                    \
              "No extra input gain (similar to Mic Boost) by default",                  \
              false)                                                                    \
                                                                                        \
        ELEM_(bool, PKT_RXTimestamp, "PKTRXTIMESTAMP",                                  \
              "No timestamps on RX packets by default",                                 \
              false)                                                                    \
                                                                                        \
        ELEM_(bool, PKT_expandMicE, "PKTEXPANDMICE",                                    \
              "decode received Mic-E data",                                             \
              false)                                                                    \
        ELEM_(bool, PKT_expandPHG, "PKTEXPANDPHG",                                      \
              "decode received PHG data",                                               \
              false)                                                                    \
        ELEM_(bool, PKT_expandCmp, "PKTEXPANDCMP",                                      \
              "decode received Compressed Position data",                               \
              false)                                                                    \
        ELEM_(bool, PKT_unitsSI, "PKTUNITSSI",                                          \
              "display decoded data in SI units",                                       \
              false)                                                                    \
        ELEM_(bool, PKT_unitsEnglish, "PKTUNITSENGLISH",                                \
              "display decoded data in English units",                                  \
              false)                                                                    \
        /* DOMINOEX */                                                                  \
        ELEM_(double, DOMINOEX_BW, "DOMINOEXBW",                                        \
              "Filter bandwidth factor (bandwidth relative to signal width)",           \
              2.0)                                                                      \
        ELEM_(std::string, secText, "SECONDARYTEXT",                                    \
              "Secondary text (sent during keyboard idle times)",                       \
              "")                                                                       \
        ELEM_(bool, DOMINOEX_FILTER, "DOMINOEXFILTER",                                  \
              "Enable filtering before decoding",                                       \
              true)                                                                     \
        ELEM_(bool, DOMINOEX_FEC, "DOMINOEXFEC",                                        \
              "Enable MultiPSK-compatible FEC",                                         \
              false)                                                                    \
        ELEM_(int, DOMINOEX_PATHS, "DOMINOEXPATHS",                                     \
              "This setting is currently unused",                                       \
              5)                                                                        \
        ELEM_(double, DomCWI, "DOMCWI",                                                 \
              "CWI threshold (CWI detection and suppression)",                          \
              0.0)                                                                      \
        /* MT63 */                                                                      \
        ELEM_(bool, mt63_8bit, "MT638BIT",                                              \
              "8-bit extended characters",                                              \
              true)                                                                     \
        ELEM_(bool, mt63_rx_integration, "MT63INTEGRATION",                             \
              "Long receive integration",                                               \
              false)                                                                    \
        ELEM_(bool, mt63_twotones, "MT63TWOTONES",                                      \
              "Also transmit upper start tone (only if MT63USETONES is enabled)",       \
              true)                                                                     \
        ELEM_(bool, mt63_usetones, "MT63USETONES",                                      \
              "Transmit lower start tone",                                              \
              true)                                                                     \
        ELEM_(int, mt63_tone_duration, "MT63TONEDURATION",                              \
              "Tone duration (seconds)",                                                \
              4)                                                                        \
        ELEM_(bool, mt63_at500, "MT63AT500",                                            \
              "Always transmit lowest tone at 500 Hz",                                  \
              false)                                                                    \
        /* Waterfall & UI */                                                            \
        ELEM_(uchar, red, "", "",  0)                                                   \
        ELEM_(uchar, green, "", "",  255)                                               \
        ELEM_(uchar, blue, "", "",  255)                                                \
        ELEM_(bool, MultiColorWF, "", "",  false)                                       \
        ELEM_(int, wfPreFilter, "WFPREFILTER",                                          \
              "Waterfal FFT prefilter window function. Values are as follows:\n"        \
              "  0: Rectangular; 1: Blackman; 2: Hamming; 3: Hanning; 4: Triangular",   \
              1)   /* Blackman */                                                       \
        ELEM_(bool, WFaveraging, "WFAVERAGING",                                         \
              "Use FFT averaging to decrease waterfall noise",                          \
              false)                                                                    \
        ELEM_(int, wf_latency, "WF_LATENCY",                                            \
              "Waterfal latency, 1...16",                                               \
              8)                                                                        \
        ELEM_(bool, UseCursorLines, "USECURSORLINES",                                   \
              "Draw cursor with vertical lines",                                        \
              true)                                                                     \
        ELEM_(bool, UseCursorCenterLine, "USECURSORCENTERLINE",                         \
              "Draw cursor center line",                                                \
              true)                                                                     \
        ELEM_(bool, UseBWTracks, "USEBWTRACKS",                                         \
              "Draw bandwidth marker with vertical lines",                              \
              true)                                                                     \
        ELEM_(bool, UseWideTracks, "USEWIDETRACKS",                                     \
              "Draw bandwidth marker with 3x vertical lines",                           \
              false)                                                                    \
        ELEM_(bool, UseWideCursor, "USEWIDECURSOR",                                     \
              "Draw cursor with 3x vertical lines",                                     \
              false)                                                                    \
        ELEM_(bool, UseWideCenter, "USEWIDECENTER",                                     \
              "Draw center line marker with 3x vertical lines",                         \
              false)                                                                    \
        ELEM_(RGBI, cursorLineRGBI, "CLCOLORS",                                         \
              "Color of cursor lines (RGBI)",                                           \
              {255, 255, 0, 255})                                                       \
        ELEM_(RGBI, cursorCenterRGBI, "CCCOLORS",                                       \
              "Color of cursor center line (RGBI)",                                     \
              {255, 255, 255, 255})                                                     \
        ELEM_(RGBI, bwTrackRGBI, "BWTCOLORS",                                           \
              "Color of bandwidth marker (RGBI)",                                       \
              {255, 0, 0, 255})                                                         \
        ELEM_(RGBI, notchRGBI, "NOTCHCOLORS",                                           \
              "Color of notch marker (RGBI)",                                           \
              {255, 255, 255, 255})                                                     \
        ELEM_(RGBI, rttymarkRGBI, "RTTYMARKRGBI",                                       \
              "Color of RTTY MARK freq marker (RGBI)",                                  \
              {255, 120, 0, 255})                                                       \
        ELEM_(int, feldfontnbr, "FELDFONTNBR",                                          \
              "Index of raster font used for transmission",                             \
              4)                                                                        \
        ELEM_(bool, viewXmtSignal, "VIEWXMTSIGNAL",                                     \
              "Show transmit signal on waterfall",                                      \
              false)                                                                    \
        ELEM_(bool, sendid, "SENDID",                                                   \
              "Send video ID containing modem name",                                    \
              false)                                                                    \
        ELEM_(bool, macroid, "MACROID",                                                 \
              "This setting is currently unused",                                       \
              false)                                                                    \
        ELEM_(bool, sendtextid, "SENDTEXTID",                                           \
              "Send video ID containing arbitrary text",                                \
              false)                                                                    \
        ELEM_(std::string, strTextid, "STRTEXTID",                                      \
              "Video ID text for SENDTEXTID (keep short!)",                             \
              "CQ")                                                                     \
        ELEM_(double, pretone, "PRETONE",                                               \
              "Single tone at center of modem BW, carrier detect for amplifiers",       \
              0.0)                                                                      \
        ELEM_(bool, macroCWid, "", "",  false)                                          \
        ELEM_(std::string, DTMFstr, "", "", "")                                         \
        ELEM_(bool, DTMFdecode, "DTMFDECODE",                                           \
              "Decode received DTMF tones",                                             \
              false)                                                                    \
        ELEM_(int, videowidth, "VIDEOWIDTH",                                            \
              "Video ID text width (characters per row)",                               \
              1)                                                                        \
        ELEM_(bool, vidlimit, "VIDLIMIT",                                               \
              "Limit video width to 500 Hz",                                            \
              true)                                                                     \
        ELEM_(bool, vidmodelimit, "VIDMODELIMIT",                                       \
              "Limit video width to mode bandwidth",                                    \
              true)                                                                     \
        ELEM_(bool, ID_SMALL, "IDSMALL",                                                \
              "Use small video ID font",                                                \
              true)                                                                     \
        ELEM_(mode_set_t, videoid_modes, "VIDEOIDMODESEXCLUDE",                         \
              "Mode names for which Video ID transmission is disabled",                 \
              mode_set_t())                                                             \
        ELEM_(bool, macrotextid, "", "",  false)                                        \
        ELEM_(bool, docked_rig_control, "DOCKEDRIGCONTROL",                             \
              "Docked rig control",                                                     \
              true)                                                                     \
        ELEM_(int,  wfheight, "WFHEIGHT",                                               \
              "Waterfall height (pixels)",                                              \
              125)                                                                      \
        ELEM_(bool, tooltips, "TOOLTIPS",                                               \
              "Show tooltips",                                                          \
              true)                                                                     \
        ELEM_(bool, NagMe, "NAGME",                                                     \
              "Prompt to save log",                                                     \
              false)                                                                    \
        ELEM_(bool, ClearOnSave, "CLEARONSAVE",                                         \
              "Clear log fields on save",                                               \
              false)                                                                    \
        ELEM_(bool, sort_date_time_off, "SORTDATEOFF",                                  \
              "Sort log by date/time off",                                              \
              true)                                                                     \
        ELEM_(bool, force_date_time, "FORCEDATETIME",                                   \
              "Force date/time ON == OFF",                                              \
              false)                                                                    \
        ELEM_(bool, menuicons, "MENUICONS",                                             \
              "Show menu icons",                                                        \
              true)                                                                     \
        ELEM_(mode_set_t, visible_modes, "VISIBLEMODES",                                \
              "Modes that are not shown in the opmodes menu",                           \
              mode_set_t())                                                             \
        ELEM_(bool, rxtext_clicks_qso_data, "RXTEXTCLICKS",                             \
              "Double-click on RX text enters QSO data",                                \
              true)                                                                     \
        ELEM_(bool, rxtext_tooltips, "RXTEXTTOOLTIPS",                                  \
              "Show callsign tooltips in received text",                                \
              false)                                                                    \
        ELEM_(bool, autofill_qso_fields, "AUTOFILLQSO",                                 \
              "Auto-fill Country and Azimuth QSO fields",                               \
              false)                                                                    \
        ELEM_(bool, calluppercase, "CALLUPPERCASE",                                     \
              "Convert callsign field to upper case",                                   \
              true)                                                                     \
        ELEM_(bool, RSTdefault, "RSTDEFAULT",                                           \
              "Default outgoing RST to 599",                                            \
              false)                                                                    \
        ELEM_(bool, RSTin_default, "RSTINDEFAULT",                                      \
              "Default incoming RST to 599",                                            \
              false)                                                                    \
        ELEM_(bool, autoextract, "AUTOEXTRACT",                                         \
              "Enable detection and extraction of \"wrapped\" text",                    \
              true)                                                                     \
        ELEM_(bool, open_flmsg, "OPEN_FLMSG",                                           \
              "Open flmsg with the autoextract file",                                   \
              true)                                                                     \
        ELEM_(bool, open_flmsg_print, "OPEN_FLMSG_PRINT",                               \
              "Open flmsg with the autoextract file\nprint to browser\nclose flmsg",    \
              true)                                                                     \
        ELEM_(bool, open_nbems_folder, "OPEN_NBEMS_FOLDER",                             \
              "Open NBEMS folder upon receipt of an autoextract file",                  \
              false)                                                                    \
        ELEM_(std::string, flmsg_pathname, "FLMSG_PATHNAME",                            \
              "Full pathname to the flmsg executable",                                  \
              "")                                                                       \
        ELEM_(double, extract_timeout, "EXTRACT_TIMEOUT",                               \
              "Abort message extraction after nn.n seconds of inactivity",              \
              2.0)                                                                      \
        ELEM_(std::string, cty_dat_pathname, "CTYDAT_PATHNAME",                         \
              "Full pathname to the cty.dat data file",                                 \
              "")                                                                       \
        ELEM_(bool, speak, "SPEAK",                                                     \
              "Capture text to file 'talk/textout.txt'",                                \
              false)                                                                    \
        ELEM_(bool, auto_talk, "AUTO_TALK",                                             \
              "Connect to Digitalk socket server during program initialization",        \
              false)                                                                    \
        /* QRZ */                                                                       \
        ELEM_(int, QRZXML, "QRZXMLTYPE",                                                \
              "Callsign xml query type.  Values are as follows:\n"                      \
              "  0: none; 1: QRZ (paid sub.); 2: QRZ cdrom; 3: HamCall (paid sub.);\n"  \
              "  4: callook free US calls xml service; 5: hamQTH free xml service.\n"   \
              "  The default is none.",                                                 \
              QRZXMLNONE)                                                               \
        ELEM_(int, QRZWEB, "QRZWEBTYPE",                                                \
              "Callsign browser query type.  Values are as follows:\n"                  \
              "  0: none; 1: QRZ web browser; 2: HamCall web browser\n"                 \
              "  3: hamQTH web browser.\n  The default is none.",                       \
              QRZWEBNONE)                                                               \
        ELEM_(std::string, QRZpathname, "QRZPATHNAME",                                  \
              "QRZ cdrom path",                                                         \
              "")                                                                       \
        ELEM_(std::string, QRZusername, "QRZUSER",                                      \
              "QRZ or HamCall subscriber username",                                     \
              "")                                                                       \
        ELEM_(std::string, QRZuserpassword, "QRZPASSWORD",                              \
              "QRZ or HamCall subscriber password",                                     \
              "")                                                                       \
        ELEM_(bool, QRZchanged, "", "",  false)                                         \
        /* eQSL */                                                                      \
        ELEM_(std::string, eqsl_id, "EQSL_ID",                                          \
              "eQSL login id",                                                          \
              "")                                                                       \
        ELEM_(std::string, eqsl_pwd, "EQSL_PASSWORD",                                   \
              "eQSL login password",                                                    \
              "")                                                                       \
        ELEM_(std::string, eqsl_nick, "EQSL_NICKNAME",                                  \
              "eQSL nickname",                                                          \
              "")                                                                       \
        ELEM_(std::string, eqsl_default_message, "EQSL_DEF_MSG",                        \
              "eQSl default message",                                                   \
              "")                                                                       \
        ELEM_(bool, eqsl_when_logged, "EQSL_WHEN_LOGGED",                               \
              "Send eQSL when other log action invoked",                                \
              false)                                                                    \
        ELEM_(bool, eqsl_datetime_off, "EQSL_DATETIME_OFF",                             \
              "Send logbook date/time off vice date on (default)",                      \
              false)                                                                    \
        /* Rig control */                                                               \
        ELEM_(bool, btnusb, "BTNUSB",                                                   \
              "This setting is currently unused",                                       \
              true)                                                                     \
        ELEM_(bool, RTSptt, "RTSPTT",                                                   \
              "RTS is PTT signal line",                                                 \
              false)                                                                    \
        ELEM_(bool, DTRptt, "DTRPTT",                                                   \
              "DTR is PTT signal line",                                                 \
              false)                                                                    \
        ELEM_(bool, RTSplus, "RTSPLUS",                                                 \
              "Initial voltage on RTS is +V",                                           \
              false)                                                                    \
        ELEM_(bool, DTRplus, "DTRPLUS",                                                 \
              "Initial voltage on DTR is +V",                                           \
              false)                                                                    \
        ELEM_(bool, PTTrightchannel, "PTTRIGHTCHANNEL",                                 \
              "Generate PTT signal on right audio channel",                             \
              false)                                                                    \
        ELEM_(int, chkUSEHAMLIBis, "CHKUSEHAMLIBIS",                                    \
              "Use Hamlib rig control",                                                 \
              0)                                                                        \
        ELEM_(int, chkUSERIGCATis, "CHKUSERIGCATIS",                                    \
              "Use RigCAT rig control",                                                 \
              0)                                                                        \
        ELEM_(int, chkUSEXMLRPCis, "CHKUSEXMLRPCIS",                                    \
              "Use XML-RPC rig control",                                                \
              0)                                                                        \
        ELEM_(std::string, PTTdev, "PTTDEV",                                            \
              "PTT device",                                                             \
              DEFAULT_PTTDEV)                                                           \
        ELEM_(std::string, CWFSKport, "", "",  DEFAULT_CWFSKPORT)                       \
        ELEM_(std::string, HamRigDevice, "HAMRIGDEVICE",                                \
              "Hamlib rig device",                                                      \
              DEFAULT_HAMRIGDEVICE)                                                     \
        ELEM_(std::string, HamRigName, "HAMRIGNAME",                                    \
              "Hamlib rig name",                                                        \
              "")                                                                       \
        ELEM_(int, HamRigModel, "HAMRIGMODEL",                                          \
              "Hamlib rig model",                                                       \
              0)                                                                        \
        ELEM_(std::string, HamConfig, "HAMCONFIG",                                      \
              "Hamlib configuration (param=val, ...)",                                  \
              "")                                                                       \
        ELEM_(int, HamRigBaudrate, "HAMRIGBAUDRATE",                                    \
              "Hamlib rig baud rate. Values are as follows:\n"                          \
              "  0: 300; 1: 600; 2: 1200; 3: 2400; 4: 4800; 5: 9600; 6: 19200;\n"       \
              "  7: 38400; 8: 57600; 9: 115200; 10: 230400; 11: 460800.",               \
              1)   /* 600 baud */                                                       \
        ELEM_(int, HamRigStopbits, "HAMRIGSTOPBITS",                                    \
              "Hamlib stopbits <1/2>.",                                                 \
              2)   /* 600 baud */                                                       \
        ELEM_(std::string, XmlRigFilename, "XMLRIGFILENAME",                            \
              "RigCAT XML file name",                                                   \
              "")                                                                       \
        ELEM_(std::string, XmlRigDevice, "XMLRIGDEVICE",                                \
              "RigCAT device",                                                          \
              DEFAULT_HAMRIGDEVICE)                                                     \
        ELEM_(int, XmlRigBaudrate, "XMLRIGBAUDRATE",                                    \
              "RigCAT rig baud rate.  See HAMRIGBAUDRATE.",                             \
              1)   /* 600 baud */                                                       \
        ELEM_(int, RigCatStopbits, "RIGCATSTOPBITS",                                    \
              "RigCAT stopbits. <1/2>",                                                 \
              2)   /* 600 baud */                                                       \
        ELEM_(bool, TTYptt, "TTYPTT",                                                   \
              "Use separate device for PTT",                                            \
              false)                                                                    \
        ELEM_(bool, HamlibCMDptt, "HAMLIBCMDPTT",                                       \
              "PTT via Hamlib command",                                                 \
              false)                                                                    \
        ELEM_(bool, RigCatCMDptt, "RIGCATCMDPTT",                                       \
              "PTT via RigCAT command",                                                 \
              false)                                                                    \
        ELEM_(bool, UseUHrouterPTT, "USEUHROUTERPTT",                                   \
              "Use uHRouter PTT (OS X only)",                                           \
              false)                                                                    \
        ELEM_(bool, UsePPortPTT, "USEPPORTPTT",                                         \
              "Use parallel port PTT",                                                  \
              false)                                                                    \
        /* RigCAT parameters */                                                         \
        ELEM_(bool, RigCatRTSplus, "RIGCATRTSPLUS",                                     \
              "Initial state of RTS",                                                   \
              false)                                                                    \
        ELEM_(bool, RigCatDTRplus, "RIGCATDTRPLUS",                                     \
              "Initial state of DTR",                                                   \
              false)                                                                    \
        ELEM_(bool, RigCatRTSptt, "RIGCATRTSPTT",                                       \
              "Toggle RTS for PTT",                                                     \
              false)                                                                    \
        ELEM_(bool, RigCatDTRptt, "RIGCATDTRPTT",                                       \
              "Toggle DTR for PTT",                                                     \
              false)                                                                    \
        ELEM_(bool, RigCatRTSCTSflow, "RIGCATRTSCTSFLOW",                               \
              "RTS/CTS flow control",                                                   \
              false)                                                                    \
        ELEM_(int, RigCatRetries, "RIGCATRETRIES",                                      \
              "Number of retries before giving up",                                     \
              2)                                                                        \
        ELEM_(int, RigCatTimeout, "RIGCATTIMEOUT",                                      \
              "Retry interval (milliseconds)",                                          \
              10)                                                                       \
        ELEM_(int, RigCatWait, "RIGCATWAIT",                                            \
              "Write delay (milliseconds)",                                             \
              50)                                                                       \
        ELEM_(bool, RigCatECHO, "RIGCATECHO",                                           \
              "Commands are echoed",                                                    \
              false)                                                                    \
        ELEM_(bool, RigCatVSP, "RIGCATVSP",                                             \
              "VSP support enabled",                                                    \
              false)                                                                    \
        ELEM_(bool, RigCatRestoreTIO, "RIGCATRESTORETIO",                               \
              "Restore original state of comm port when closing",                       \
              false)                                                                    \
        /* Hamlib parameters */                                                         \
        ELEM_(bool, HamlibRTSplus, "HAMLIBRTSPLUS",                                     \
              "RTS +12",                                                                \
              false)                                                                    \
        ELEM_(bool, HamlibDTRplus, "HAMLIBDTRPLUS",                                     \
              "DTR +12",                                                                \
              false)                                                                    \
        ELEM_(bool, HamlibRTSCTSflow, "HAMLIBRTSCTSFLOW",                               \
              "RTS/CTS flow control",                                                   \
              false)                                                                    \
        ELEM_(bool, HamlibXONXOFFflow, "HAMLIBXONXOFFFLOW",                             \
              "XON/XOFF flow control",                                                  \
              false)                                                                    \
        ELEM_(int, HamlibRetries, "HAMLIBRETRIES",                                      \
              "Number of times to resend command before giving up",                     \
              2)                                                                        \
        ELEM_(int, HamlibTimeout, "HAMLIBTIMEOUT",                                      \
              "Retry interval (milliseconds)",                                          \
              10)                                                                       \
        ELEM_(int, HamlibWait, "HAMLIBWAIT",                                            \
              "Wait interval before reading response (milliseconds)",                   \
              5)                                                                        \
        ELEM_(int, HamlibWriteDelay, "HAMLIBWRITEDELAY",                                \
              "Write delay (milliseconds)",                                             \
              0)                                                                        \
        ELEM_(int, HamlibSideband, "HAMLIBSIDEBAND",                                    \
              "Force the rig sideband (for the purpose of calculating frequencies).\n"  \
              "Values are as follows: 0: as reported by rig; 1: LSB; 2: USB.",          \
              0)   /* SIDEBAND_RIG */                                                   \
        /* Operator */                                                                  \
        ELEM_(std::string, myCall, "MYCALL",                                            \
              "Operator callsign",                                                      \
              "")                                                                       \
        ELEM_(std::string, myQth, "MYQTH",                                              \
              "Operator QTH",                                                           \
              "")                                                                       \
        ELEM_(std::string, myName, "MYNAME",                                            \
              "Operator name",                                                          \
              "")                                                                       \
        ELEM_(std::string, myLocator, "MYLOC",                                          \
              "Operator Maidenhead locator",                                            \
              "")                                                                       \
        ELEM_(std::string, myAntenna, "MYANTENNA",                                      \
              "Antenna description (keep short!)",                                      \
              "")                                                                       \
        /* Sound card */                                                                \
        ELEM_(int, btnAudioIOis, "AUDIOIO",                                             \
              "Audio subsystem.  Values are as follows:\n"                              \
              "  0: OSS; 1: PortAudio; 2: PulseAudio; 3: File I/O",                     \
              SND_IDX_NULL)                                                             \
        ELEM_(std::string, OSSdevice, "OSSDEVICE",                                      \
              "OSS device name",                                                        \
              "")                                                                       \
        ELEM_(std::string, PAdevice, "PADEVICE",                                        \
              "For compatibility with older versions",                                  \
              "")                                                                       \
        ELEM_(std::string, PortInDevice, "PORTINDEVICE",                                \
              "PortAudio input device name",                                            \
              "")                                                                       \
        ELEM_(int, PortInIndex, "PORTININDEX",                                          \
              "PortAudio input device index",                                           \
              -1)                                                                       \
        ELEM_(std::string, PortOutDevice, "PORTOUTDEVICE",                              \
              "PortAudio input device name",                                            \
              "")                                                                       \
        ELEM_(int, PortOutIndex, "PORTOUTINDEX",                                        \
              "PortAudio input device index",                                           \
              -1)                                                                       \
        ELEM_(int, PortFramesPerBuffer, "", "",  0)                                     \
        ELEM_(std::string, PulseServer, "PULSESERVER",                                  \
              "PulseAudio server string",                                               \
              "")                                                                       \
        ELEM_(int, in_channels, "INCHANNELS",                                           \
              "Number of audio input channels",                                         \
              1)                                                                        \
        ELEM_(bool, mono_audio, "MONOAUDIO",                                            \
              "Force use of mono audio output",                                         \
              false)                                                                    \
        ELEM_(bool, sig_on_right_channel, "SIGONRIGHTCHANNEL",                          \
              "Duplicate modem signal on left & right",                                 \
              false)                                                                    \
        ELEM_(bool, ReverseAudio, "REVERSEAUDIO",                                       \
              "Reverse left-right audio channels",                                      \
              false)                                                                    \
        ELEM_(int, sample_rate, "SAMPLERATE",                                           \
              "For compatibility with older versions",                                  \
              SAMPLE_RATE_UNSET)                                                        \
        ELEM_(int, in_sample_rate, "INSAMPLERATE",                                      \
              "Input sample rate",                                                      \
              SAMPLE_RATE_UNSET)                                                        \
        ELEM_(int, out_sample_rate, "OUTSAMPLERATE",                                    \
              "Output sample rate",                                                     \
              SAMPLE_RATE_UNSET)                                                        \
        ELEM_(int, sample_converter, "SAMPLECONVERTER",                                 \
              "Sample rate conversion type. Values are as follows:\n"                   \
              "  0: Best SINC; 1: Medium SINC; 2: Fastest SINC; 3: ZOH; 4: Linear.\n"   \
              "The default is 2.",                                                      \
              SRC_SINC_FASTEST)                                                         \
        ELEM_(int, RX_corr, "RXCORR",                                                   \
              "Input (RX) sample rate correction (PPM)",                                \
              0)                                                                        \
        ELEM_(int, TX_corr, "TXCORR",                                                   \
              "Output (TX) sample rate correction (PPM)",                               \
              0)                                                                        \
        ELEM_(int, TxOffset, "TXOFFSET",                                                \
              "Difference between RX and TX freq (rig offset)",                         \
              0)                                                                        \
        ELEM_(int, wavSampleRate, "WAV_SAMPLERATE",                                     \
              "Wave file record sample rate\n"                                          \
              "0 - 22050, 1 - 24000, 2 - 44100, 3 - 48000",                             \
              3)                                                                        \
        ELEM_(bool, loop_playback, "LOOPPLAYBACK",                                      \
              "true = continuous loop of sound file playback\n"                         \
              "false = single pass through playback file.",                             \
              false)                                                                    \
        ELEM_(int, PTT_on_delay, "PTTONDELAY",                                          \
              "Start of transmit delay before sending audio",                           \
              0)                                                                        \
        ELEM_(int, PTT_off_delay, "PTTOFFDELAY",                                        \
              "End of transmit delay before disabling PTT",                             \
              0)                                                                        \
        /* Contest controls and Logbook */                                              \
        ELEM_(std::string, logbookfilename, "LOGBOOKFILENAME",                          \
              "Logbook file name",                                                      \
              "")                                                                       \
        ELEM_(bool, fixed599, "FIXED599",                                               \
              "Force RST in/out to 599",                                                \
              false)                                                                    \
        ELEM_(bool, UseLeadingZeros, "USELEADINGZEROS",                                 \
              "Insert leading zeros into transmitted serial number (contest)",          \
              true)                                                                     \
        ELEM_(bool, cutnbrs, "CUTNBRS",                                                 \
              "Send CW cut numbers",                                                    \
              false)                                                                    \
        ELEM_(RGB, bwsrSliderColor, "BWSRSLIDERCOLOR",                                  \
              "Background color of signal browser detect level",                        \
              {185, 211, 238})                                                          \
        ELEM_(RGB, bwsrSldrSelColor,"BWSRSLDRSELCOLOR",                                 \
              "Button highlight color, signal browser detect level",                    \
              {54, 100, 139})                                                           \
        ELEM_(int, bwsrHiLight1, "BWSRHILIGHT1",                                        \
              "View Browser highlight color 1, default Dark Red",                       \
              FL_RED)                                                                   \
        ELEM_(int, bwsrHiLight2, "BWSRHILIGHT2",                                        \
              "View Browser highlight color 2, default Dark Green",                     \
              FL_GREEN)                                                                 \
        ELEM_(int, bwsrBackgnd1, "BWSRBACKGND1",                                        \
              "View Browser background odd lines",                                      \
              55)                                                                       \
        ELEM_(int, bwsrBackgnd2, "BWSRBACKGND2",                                        \
              "View Browser background odd lines",                                      \
              53)                                                                       \
        ELEM_(int, bwsrSelect, "BWSRSELECT",                                            \
              "View Browser line select color",                                         \
              FL_BLUE)                                                                  \
        ELEM_(RGB, dup_color, "dupcolor",                                               \
              "Callsign background color when duplicate detected",                      \
              {255, 110, 180})                                                          \
        ELEM_(bool, EnableDupCheck, "ENABLEDUPCHECK",                                   \
              "Check for duplicates (contest)",                                         \
              false)                                                                    \
        ELEM_(bool, dupmode, "DUPMODE",                                                 \
              "Predicate for ENABLEDUPCHECK (mode must match)",                         \
              true)                                                                     \
        ELEM_(bool, dupband, "DUPBAND",                                                 \
              "Predicate for ENABLEDUPCHECK (band must match)",                         \
              true)                                                                     \
        ELEM_(bool, dupstate, "DUPSTATE",                                               \
              "Predicate for ENABLEDUPCHECK (state must match)",                        \
              false)                                                                    \
        ELEM_(bool, dupxchg1, "DUPXCHG1",                                               \
              "Predicate for ENABLEDUPCHECK (exchange must match)",                     \
              false)                                                                    \
        ELEM_(bool, duptimespan, "DUPTIMESPAN",                                         \
              "Predicate for ENABLEDUPCHECK (QSO inside timespan)",                     \
              false)                                                                    \
        ELEM_(int, timespan, "TIMESPAN",                                                \
              "Time for DUPTIMESPAN (minutes)",                                         \
              120)                                                                      \
        ELEM_(int, ContestStart, "CONTESTSTART",                                        \
              "Contest starting number",                                                \
              0)                                                                        \
        ELEM_(int, ContestDigits, "CONTESTDIGITS",                                      \
              "Number of digits in serial number",                                      \
              4)                                                                        \
        ELEM_(std::string, nonwordchars, "NONWORDCHARS",                                \
              "Additional characters used to delimit WORDS",                            \
              "*,-.;")                                                                  \
        ELEM_(std::string, myXchg, "MYXCGH",                                            \
              "Free form exchange",                                                     \
              "")                                                                       \
        ELEM_(std::string, mytxpower, "TXPOWER",                                        \
              "TX power used for logbook entries",                                      \
              "")                                                                       \
        /* Macro controls */                                                            \
        ELEM_(bool, UseLastMacro, "USELASTMACRO",                                       \
              "Load last used macro file on startup",                                   \
              false)                                                                    \
        ELEM_(bool, DisplayMacroFilename, "DISPLAYMACROFILENAME",                       \
              "Display macro filename on startup",                                      \
              false)                                                                    \
        ELEM_(bool, SaveMacros, "SAVEMACROS",                                           \
              "Save current macros on exit",                                            \
              false)                                                                    \
        ELEM_(bool, macro_wheel, "MACROWHEEL",                                          \
              "Enable mouse wheel rotation to control visible macro set",               \
              false)                                                                    \
        ELEM_(bool, mbar1_pos, "MBAR1POS",                                              \
              "Principal macro bar position, true=above wf, false=below",               \
              true)                                                                     \
        ELEM_(int, mbar2_pos, "MBAR2POS",                                               \
              "Position second macro button above data stream panesl",                  \
              0)                                                                        \
        /* Mixer */                                                                     \
        ELEM_(std::string, MXdevice, "MXDEVICE",                                        \
              "Mixer device",                                                           \
              "")                                                                       \
        ELEM_(bool, MicIn, "MICIN",                                                     \
              "Control the microphone mixer channel",                                   \
              false)                                                                    \
        ELEM_(bool, LineIn, "LINEIN",                                                   \
              "Control the line-in mixer channel",                                      \
              true)                                                                     \
        ELEM_(bool, EnableMixer, "ENABLEMIXER",                                         \
              "Enable mixer controls",                                                  \
              false)                                                                    \
        ELEM_(double, PCMvolume, "PCMVOLUME",                                           \
              "PCM channel level",                                                      \
              0.8)                                                                      \
        ELEM_(double, txlevel, "TXATTEN",                                               \
              "TX attenuator (db) -30 .. 0",                                            \
              -3.0)                                                                     \
        ELEM_(bool, MuteInput, "MUTEINPUT",                                             \
              "This setting is currently unused",                                       \
              true)                                                                     \
        ELEM_(double, TxMonitorLevel, "TXMONITORLEVEL",                                 \
              "Level for monitored (on waterfall) transmit signal",                     \
              0.5)                                                                      \
        /* Waterfall palette */                                                         \
        ELEM_(std::string, PaletteName, "PALETTENAME",                                  \
              "Waterfall color palette file name",                                      \
              "default.pal")                                                            \
        ELEM_(RGB, cfgpal0, "PALETTE0",                                                 \
              "Custom palette 0",                                                       \
              { 0,0,0 })                                                                \
        ELEM_(RGB, cfgpal1, "PALETTE1",                                                 \
              "Custom palette 1",                                                       \
              { 0,0,136 })                                                              \
        ELEM_(RGB, cfgpal2, "PALETTE2",                                                 \
              "Custom palette 2",                                                       \
              { 0,19,198 })                                                             \
        ELEM_(RGB, cfgpal3, "PALETTE3",                                                 \
              "Custom palette 3",                                                       \
              { 0,32,239 })                                                             \
        ELEM_(RGB, cfgpal4, "PALETTE4",                                                 \
              "Custom palette 4",                                                       \
              { 172,167,105 })                                                          \
        ELEM_(RGB, cfgpal5, "PALETTE5",                                                 \
              "Custom palette 5",                                                       \
              { 194,198,49 })                                                           \
        ELEM_(RGB, cfgpal6, "PALETTE6",                                                 \
              "Custom palette 6",                                                       \
              { 225,228,107 })                                                          \
        ELEM_(RGB, cfgpal7, "PALETTE7",                                                 \
              "Custom palette 7",                                                       \
              { 255,255,0 })                                                            \
        ELEM_(RGB, cfgpal8, "PALETTE8",                                                 \
              "Custom palette 8",                                                       \
              { 251,51,0 })                                                             \
        /* Palettes for macro button groups */                                          \
        ELEM_(bool, useGroupColors, "USEGROUPCOLORS",                                   \
              "Use macro group colors",                                                 \
              true)                                                                     \
        ELEM_(RGB, btnGroup1, "FKEYGROUP1",                                             \
              "Macro group 1 color",                                                    \
              { 80, 144, 144 })                                                         \
        ELEM_(RGB, btnGroup2, "FKEYGROUP2",                                             \
              "Macro group 2 color",                                                    \
              { 144, 80, 80 })                                                          \
        ELEM_(RGB, btnGroup3, "FKEYGROUP3",                                             \
              "Macro group 3 color",                                                    \
              { 80, 80, 144 })                                                          \
        ELEM_(RGB, btnFkeyTextColor, "FKEYTEXTCOLOR",                                   \
              "Macro button foreground ",                                               \
              { 255, 255, 255 })                                                        \
        /* RX / TX / Waterfall text widgets */                                          \
        ELEM_(std::string, charset_name, "CHARSET_NAME",                                \
              "Default character set",                                                  \
              "UTF-8")                                                                  \
        ELEM_(std::string, RxFontName, "RXFONTNAME",                                    \
              "RX text font name",                                                      \
              "")                                                                       \
        ELEM_(bool, RxFontWarn, "RXFONTWARN",                                           \
              "Enable RX font warnings",                                                \
              true)                                                                     \
        ELEM_(Fl_Font, RxFontnbr, "RXFONTNBR",                                          \
              "RX text font index",                                                     \
              FL_COURIER)                                                               \
        ELEM_(int, RxFontsize, "RXFONTSIZE",                                            \
              "RX text font size",                                                      \
              16)                                                                       \
        ELEM_(Fl_Color, RxFontcolor, "RXFNTCOLOR",                                      \
              "RX text font color",                                                     \
              FL_BLACK)                                                                 \
        ELEM_(Fl_Color, RxTxSelectcolor, "RXTXSELCOLOR",                                \
              "RX/TX text select color",                                                \
              FL_MAGENTA)                                                               \
        ELEM_(std::string, TxFontName, "TXFONTNAME",                                    \
              "TX text font name",                                                      \
              "")                                                                       \
        ELEM_(bool, TxFontWarn, "TXFONTWARN",                                           \
              "Enable TX font warnings",                                                \
              true)                                                                     \
        ELEM_(Fl_Font, TxFontnbr, "TXFONTNBR",                                          \
              "TX text font index",                                                     \
              FL_COURIER)                                                               \
        ELEM_(int, TxFontsize, "TXFONTSIZE",                                            \
              "TX text font size",                                                      \
              16)                                                                       \
        ELEM_(Fl_Color, TxFontcolor, "TXFNTCOLOR",                                      \
              "TX text font color",                                                     \
              FL_BLACK)                                                                 \
        ELEM_(RGB, RxColor, "RXFONTCOLOR",                                              \
              "RX text font color (RGB)",                                               \
              { 255, 242, 190 })                                                        \
        ELEM_(RGB, TxColor, "TXFONTCOLOR",                                              \
              "TX text font color (RGB)",                                               \
              { 200, 235, 255 })                                                        \
        ELEM_(Fl_Color, XMITcolor, "XMITCOLOR",                                         \
              "Color for Transmit text style",                                          \
              FL_RED)                                                                   \
        ELEM_(Fl_Color, CTRLcolor, "CTRLCOLOR",                                         \
              "Color for Control text style",                                           \
              FL_DARK_GREEN)                                                            \
        ELEM_(Fl_Color, SKIPcolor, "SKIPCOLOR",                                         \
              "Color for Skipped text style",                                           \
              FL_BLUE)                                                                  \
        ELEM_(Fl_Color, ALTRcolor, "ALTRCOLOR",                                         \
              "Color for Alternate text style",                                         \
              FL_DARK_MAGENTA)                                                          \
        ELEM_(Fl_Color, LowSignal, "LOWSIGNAL",                                         \
              "Color for low signal level",                                             \
              FL_BLACK)                                                                 \
        ELEM_(Fl_Color, NormSignal, "NORMSIGNAL",                                       \
              "Color for normal signal level",                                          \
              FL_GREEN)                                                                 \
        ELEM_(Fl_Color, HighSignal, "HIGHSIGNAL",                                       \
              "Color for high signal level",                                            \
              FL_YELLOW)                                                                \
        ELEM_(Fl_Color, OverSignal, "OVERSIGNAL",                                       \
              "Color for over driven signal",                                           \
              FL_RED)                                                                   \
        ELEM_(std::string, WaterfallFontName, "WATERFALLFONTNAME",                      \
              "Waterfall font name",                                                    \
              "")                                                                       \
        ELEM_(Fl_Font, WaterfallFontnbr, "WATERFALLFONTNBR",                            \
              "Waterfall font number",                                                  \
              FL_COURIER)                                                               \
        ELEM_(int, WaterfallFontsize, "WATERFALLFONTSIZE",                              \
              "Waterfall font size",                                                    \
              12)                                                                       \
        ELEM_(Fl_Color, LOGGINGtextcolor, "LOGGINGTEXTCOLOR",                           \
              "Text color in logging controls",                                         \
              FL_BLACK)                                                                 \
        ELEM_(Fl_Color, LOGGINGcolor, "LOGGINGCOLOR",                                   \
              "Background color in logging controls",                                   \
              FL_BACKGROUND2_COLOR)                                                     \
        ELEM_(Fl_Font, LOGGINGtextfont, "LOGGINGTEXTFONT",                              \
              "Logging Controls font number",                                           \
              FL_HELVETICA)                                                             \
        ELEM_(int, LOGGINGtextsize, "LOGGINGTEXTSIZE",                                  \
              "Logging Controls font size",                                             \
              12)                                                                       \
        ELEM_(Fl_Color, LOGBOOKtextcolor, "LOGBOOKTEXTCOLOR",                           \
              "Text color in logbook dialog",                                           \
              FL_BLACK)                                                                 \
        ELEM_(Fl_Color, LOGBOOKcolor, "LOGBOOKCOLOR",                                   \
              "Background color in logbook dialog",                                     \
              FL_BACKGROUND2_COLOR)                                                     \
        ELEM_(Fl_Font, LOGBOOKtextfont, "LOGBOOKTEXTFONT",                              \
              "Logbook dialog controls font number",                                    \
              FL_HELVETICA)                                                             \
        ELEM_(int, LOGBOOKtextsize, "LOGBOOKTEXTSIZE",                                  \
              "Logbook dialog controls font size",                                      \
              12)                                                                       \
        ELEM_(std::string, FreqControlFontName, "FREQCONTROLFONTNAME",                  \
              "Frequency Control font name",                                            \
              "")                                                                       \
        ELEM_(Fl_Font, FreqControlFontnbr, "FREQCONTROLFONTNBR",                        \
              "Frequency Control font number",                                          \
              FL_COURIER)                                                               \
        ELEM_(std::string, ui_scheme, "UISCHEME",                                       \
              "FLTK UI scheme (none or base, gtk+, plastic)",                           \
              "gtk+")                                                                   \
        ELEM_(int, ui_language, "UILANGUAGE",                                           \
              "UI language",                                                            \
              0)                                                                        \
        ELEM_(bool, wf_audioscale, "WFAUDIOSCALE",                                      \
              "Always show audio frequencies on waterfall",                             \
              true)                                                                     \
        /* Freq Display colors */                                                       \
        ELEM_(RGB, FDbackground, "FDBACKGROUND",                                        \
              "Frequency display background color",                                     \
              { 0, 0, 0 })                                                              \
        ELEM_(RGB, FDforeground, "FDFOREGROUND",                                        \
              "Frequency display foreground color",                                     \
              { 0, 200, 0 })                                                            \
        /* Tab selection color */                                                       \
        ELEM_(Fl_Color, TabsColor, "TABSCOLOR",                                         \
              "UI tabs color",                                                          \
              FL_BACKGROUND2_COLOR)                                                     \
        /* Signal Viewer */                                                             \
        ELEM_(bool, VIEWERascend, "VIEWERASCEND",                                       \
              "Low frequency on bottom of viewer",                                      \
              true)                                                                     \
        ELEM_(bool, VIEWERmarquee, "VIEWERMARQUEE",                                     \
              "Signal Viewer text continuous scrolling",                                \
              true)                                                                     \
        ELEM_(bool, VIEWERsort, "VIEWERSORT",                                           \
              "Signal Viewer sort after channel changes- unused",                       \
              false)                                                                    \
        ELEM_(bool, VIEWERhistory, "VIEWERHISTORY",                                     \
              "Signal Viewer playback history on select",                               \
              false)                                                                    \
        ELEM_(bool, VIEWERfixed, "VIEWERfixed",                                         \
              "Signal Viewer data displayed on fixed 100 Hz intervals",                 \
              true)                                                                     \
        ELEM_(int, VIEWERlabeltype, "VIEWERSHOWFREQ",                                   \
              "Signal Viewer label type.  Values are as follows:\n"                     \
              "  0: None; 1: Audio freq; 2: Radio freq; 2: Channel #.",                 \
              VIEWER_LABEL_RF)                                                          \
        ELEM_(int, VIEWERchannels, "VIEWERCHANNELS",                                    \
              "Number of Signal Viewer Channels",                                       \
              30)                                                                       \
        ELEM_(int, VIEWERwidth, "VIEWERWIDTH",                                          \
              "Width of viewer (% of full panel width)",                                \
              25)                                                                       \
        ELEM_(int, VIEWERtimeout, "VIEWERTIMEOUT",                                      \
              "Signal Viewer inactivity timeout (to clear text)",                       \
              15)                                                                       \
        ELEM_(std::string, ViewerFontName, "VIEWERFONTNAME",                            \
              "Signal Viewer font name",                                                \
              "")                                                                       \
        ELEM_(Fl_Font, ViewerFontnbr, "VIEWERFONTNBR",                                  \
              "Signal Viewer font index",                                               \
              FL_COURIER)                                                               \
        ELEM_(int, ViewerFontsize, "VIEWERFONTSIZE",                                    \
              "Signal Viewer font size",                                                \
              FL_NORMAL_SIZE)                                                           \
                                                                                        \
        ELEM_(Fl_Color, Sql1Color, "SQL1COLOR",                                         \
              "UI SQL button select color 1",                                           \
              FL_YELLOW)                                                                \
        ELEM_(Fl_Color, Sql2Color, "SQL2COLOR",                                         \
              "UI SQL button select color 2",                                           \
              FL_GREEN)                                                                 \
        ELEM_(Fl_Color, AfcColor, "AFCCOLOR",                                           \
              "UI AFC button select color",                                             \
              FL_GREEN)                                                                 \
        ELEM_(Fl_Color, LkColor, "LKCOLOR",                                             \
              "UI Lk xmt frequ select color",                                           \
              FL_GREEN)                                                                 \
        ELEM_(Fl_Color, RevColor, "REVCOLOR",                                           \
              "UI Rev select color",                                                    \
              FL_GREEN)                                                                 \
        ELEM_(Fl_Color, XmtColor, "XMTCOLOR",                                           \
              "UI T/R select color",                                                    \
              FL_RED)                                                                   \
        ELEM_(Fl_Color, SpotColor, "SPOTCOLOR",                                         \
              "UI Spot select color",                                                   \
              FL_GREEN)                                                                 \
        ELEM_(Fl_Color, RxIDColor, "RXIDCOLOR",                                         \
              "UI RxID select color",                                                   \
              FL_GREEN)                                                                 \
        ELEM_(Fl_Color, TxIDColor, "TXIDCOLOR",                                         \
              "UI TxID select color",                                                   \
              FL_GREEN)                                                                 \
        ELEM_(Fl_Color, TuneColor, "TUNECOLOR",                                         \
              "UI Tune select color",                                                   \
              FL_RED)                                                                   \
                                                                                        \
        /* XMLRPC LOGBOOK server */                                                     \
        ELEM_(bool, xml_logbook, "XML_LOGBOOK",                                         \
              "Try to open remote xml logbook",                                         \
              false)                                                                    \
        ELEM_(std::string, xmllog_address, "XMLLOG_ADDRESS",                            \
              "Logbook server address",                                                 \
              "127.0.0.1")                                                              \
        ELEM_(std::string, xmllog_port, "XMLLOG_PORT",                                  \
              "Logbook server port",                                                    \
              "8421")                                                                   \
                                                                                        \
        ELEM_(bool, check_for_updates, "CHECK_FOR_UPDATES",                             \
              "Check for updates when starting program",                                \
              false)                                                                    \
        /* XML-RPC/ARQ servers */                                                       \
        ELEM_(std::string, xmlrpc_address, "", "",  "127.0.0.1")                        \
        ELEM_(std::string, xmlrpc_port, "", "",  "7362")                                \
        ELEM_(std::string, xmlrpc_allow, "", "",  "")                                   \
        ELEM_(std::string, xmlrpc_deny, "", "",  "")                                    \
        ELEM_(int, rx_msgid, "", "",  9876)                                             \
        ELEM_(int, tx_msgid, "", "",  6789)                                             \
        ELEM_(std::string, arq_address, "", "",  "127.0.0.1")                           \
        ELEM_(std::string, arq_port, "", "",  "7322")                                   \
        /* PSK reporter */                                                              \
        ELEM_(bool, usepskrep, "USEPSKREP",                                             \
              "(Set by fldigi)",                                                        \
              false)                                                                    \
        ELEM_(bool, pskrep_auto, "PSKREPAUTO",                                          \
              "Report callsigns spotted in received text",                              \
              false)                                                                    \
        ELEM_(bool, pskrep_log, "PSKREPLOG",                                            \
              "Report callsigns in logged QSOs",                                        \
              false)                                                                    \
        ELEM_(bool, pskrep_qrg, "PSKREPQRG",                                            \
              "Include rig frequency in reception report",                              \
              false)                                                                    \
        ELEM_(bool, report_when_visible, "REPORTWHENVISIBLE",                           \
              "Enable Reporter ONLY when a signal browser is visible",                  \
              false)                                                                    \
        ELEM_(std::string, pskrep_host, "PSKREPHOST",                                   \
              "Reception report server address",                                        \
              "report.pskreporter.info")                                                \
        ELEM_(std::string, pskrep_port, "PSKREPPORT",                                   \
              "Reception report server port",                                           \
              "4739")                                                                   \
       /* WEFAX configuration items */                                                  \
       ELEM_(double, wefax_slant, "WEFAXSLANT",                                         \
             "Slant correction for wefax Rx",                                           \
             0.0)                                                                       \
       ELEM_(std::string, wefax_save_dir, "WEFAXSAVEDIR",                               \
             "Target directory for storing automatically received images storage",      \
             "")                                                                        \
       ELEM_(std::string, wefax_load_dir, "WEFAXLOADDIR",                               \
             "Source directory for sending images",                                     \
             "")                                                                        \
       ELEM_(int, wefax_filter, "WEFAXFILTER",                                          \
             "Input filter for image reception",                                        \
             0)                                                                         \
       ELEM_(bool, WEFAX_EmbeddedGui, "WEFAXEMBEDDEDGUI",                               \
             "Embedded GUI",                                                            \
             true)                                                                      \
       ELEM_(bool, WEFAX_HideTx, "WEFAXHIDETX",                                         \
             "Hide transmission window",                                                \
             true)                                                                      \
       ELEM_(int, WEFAX_Shift, "WEFAXSHIFT",                                            \
             "Shift (Standard 800Hz)",                                                  \
             800 )                                                                      \
       ELEM_(int, WEFAX_MaxRows, "WEFAXMAXROWS",                                        \
             "Received fax maximum number of rows",                                     \
             2900 )                                                                     \
       ELEM_(int, WEFAX_NoiseMargin, "WEFAXNOISEMARGIN",                                \
             "Pixel margin for noise removal",                                          \
             1 )                                                                        \
       ELEM_(int, WEFAX_NoiseThreshold, "WEFAXNOISETHRESHOLD",                          \
             "Threshold level for noise detection and removal",                         \
             5 )                                                                        \
       ELEM_(int, WEFAX_SaveMonochrome, "WEFAXSAVEMONOCHROME",                          \
             "Save fax image as monochrome",                                            \
             true )                                                                     \
       ELEM_(bool, WEFAX_AdifLog, "WEFAXADIFLOG",                                       \
             "Logs wefax file names in Adif log file",                                  \
             false)                                                                     \
       /* NAVTEX configuration items */                                                 \
       ELEM_(bool, NVTX_AdifLog, "NAVTEXADIFLOG",                                       \
             "Logs Navtex messages in Adig log file",                                   \
             false)                                                                     \
       ELEM_(bool, NVTX_KmlLog, "NAVTEXKMLLOG",                                         \
             "Logs Navtex messages to KML document",                                    \
             false)                                                                     \
       ELEM_(int, NVTX_MinSizLoggedMsg, "NAVTEXMINSIZLOGGEDMSG",                        \
             "Minimum length of logged messages",                                       \
             0 )                                                                        \
        /* WX fetch from NOAA */                                                        \
        ELEM_(std::string, wx_eoh, "WX_EOH",                                            \
             "Text at end of METAR report header\n"                                     \
             "default = Connection: close",                                             \
             "Connection: close")                                                       \
        ELEM_(std::string, wx_sta, "WX_STA",                                            \
              "4 letter specifier for wx station",                                      \
              "KMDQ")                                                                   \
        ELEM_(bool, wx_condx, "WX_CONDX",                                               \
              "Weather conditions",                                                     \
              true)                                                                     \
        ELEM_(bool, wx_fahrenheit, "WX_FAHRENHEIT",                                     \
              "Report in Fahrenheit",                                                   \
              true)                                                                     \
        ELEM_(bool, wx_celsius, "WX_CELSIUS",                                           \
              "Report in Celsius",                                                      \
              true)                                                                     \
        ELEM_(bool, wx_mph, "WX_MPH",                                                   \
              "Report speed in miles per hour",                                         \
              true)                                                                     \
        ELEM_(bool, wx_kph, "WX_KPH",                                                   \
              "Report speed in kilometers per hour",                                    \
              true)                                                                     \
        ELEM_(bool, wx_inches, "WX_INCHES",                                             \
              "Report pressure in inches of mercury",                                   \
              true)                                                                     \
        ELEM_(bool, wx_mbars, "WX_MBARS",                                               \
              "Report pressure in millibars",                                           \
              true)                                                                     \
        ELEM_(bool, wx_full, "WX_FULL",                                                 \
              "Use complete METAR report",                                              \
              true)                                                                     \
        ELEM_(bool, wx_station_name, "WX_STATION_NAME",                                 \
              "Report station noun name",                                               \
              true)                                                                     \
	/* KML Keyhole Markup Language */                                               \
        ELEM_(bool, kml_purge_on_startup, "KML_PURGE_ON_STARTUP",                       \
              "Purge KML data at startup",                                              \
              false)                                                                    \
       ELEM_(std::string, kml_save_dir, "KML_SAVE_DIR",                                 \
             "Destination directory for generated KML documents",                       \
             "")                                                                        \
       ELEM_(std::string, kml_command, "KML_COMMAND",                                   \
             "Command executed when creating KML files",                                \
             "")                                                                        \
       ELEM_(int, kml_merge_distance, "KML_MERGE_DISTANCE",                             \
             "Minimum distance for splitting alias nodes",                              \
             10000)                                                                     \
       ELEM_(int, kml_retention_time, "KML_RETENTION_TIME",                             \
             "Number of hours for keeping data in each node",                           \
             0)                                                                         \
       ELEM_(int, kml_refresh_interval, "KML_REFRESH_INTERVAL",                         \
             "Refresh interval written in KML files (In seconds)",                      \
             120)                                                                       \
       ELEM_(int, kml_balloon_style, "KML_BALLOON_STYLE",                               \
             "KML balloons data displayed as text, HTML tables, HTML single matrix",    \
             2)                                                                         \
        ELEM_(std::string, auto_flrig_pathname, "AUTO_FLRIG_PATHNAME",                  \
              "Full pathname to the flrig executable",                                  \
              "")                                                                       \
        ELEM_(std::string, auto_flamp_pathname, "AUTO_FLAMP_PATHNAME",                  \
              "Full pathname to the flamp executable",                                  \
              "")                                                                       \
        ELEM_(std::string, auto_flnet_pathname, "AUTO_FLNET_PATHNAME",                  \
              "Full pathname to the flnet executable",                                  \
              "")                                                                       \
        ELEM_(std::string, auto_fllog_pathname, "AUTO_FLLOG_PATHNAME",                  \
              "Full pathname to the fllog executable",                                  \
              "")                                                                       \
        ELEM_(std::string, auto_prog1_pathname, "AUTO_PROG1_PATHNAME",                  \
              "Full pathname to the prog1 executable",                                  \
              "")                                                                       \
        ELEM_(std::string, auto_prog2_pathname, "AUTO_PROG2_PATHNAME",                  \
              "Full pathname to the prog2 executable",                                  \
              "")                                                                       \
        ELEM_(std::string, auto_prog3_pathname, "AUTO_PROG3_PATHNAME",                  \
              "Full pathname to the prog3 executable",                                  \
              "")                                                                       \
        ELEM_(bool, flrig_auto_enable, "FLRIG_AUTO_ENABLE",                             \
              "Enable on program start",                                                \
              false)                                                                    \
        ELEM_(bool, flnet_auto_enable, "FLNET_AUTO_ENABLE",                             \
              "Enable on program start",                                                \
              false)                                                                    \
        ELEM_(bool, fllog_auto_enable, "FLLOG_AUTO_ENABLE",                             \
              "Enable on program start",                                                \
              false)                                                                    \
        ELEM_(bool, flamp_auto_enable, "FLAMP_AUTO_ENABLE",                             \
              "Enable on program start",                                                \
              false)                                                                    \
        ELEM_(bool, prog1_auto_enable, "PROG1_AUTO_ENABLE",                             \
              "Enable on program start",                                                \
              false)                                                                    \
        ELEM_(bool, prog2_auto_enable, "PROG2_AUTO_ENABLE",                             \
              "Enable on program start",                                                \
              false)                                                                    \
        ELEM_(bool, prog3_auto_enable, "PROG3_AUTO_ENABLE",                             \
              "Enable on program start",                                                \
              false)                                                                    \


// declare the struct
#define ELEM_DECLARE_CONFIGURATION(type_, var_, tag_, ...) type_ var_;
#undef ELEM_
#define ELEM_ ELEM_DECLARE_CONFIGURATION
struct configuration
{
	CONFIG_LIST

	void writeDefaultsXML();
	void storeDefaults();
	bool readDefaultsXML();
	void loadDefaults();
	void saveDefaults();
	int  setDefaults();
	void resetDefaults(void);
	static void reset(void);

	void initInterface();
	void initMixerDevices();
	void testCommPorts();
	const char* strBaudRate();
	int  BaudRate(size_t);
	int  nBaudRate(const char *);
	void initFonts(void);
};

extern configuration progdefaults;

extern void mixerInputs();
extern void enableMixer(bool);
extern Fl_Font font_number(const char* name);

enum { SAMPLE_RATE_UNSET = -1, SAMPLE_RATE_AUTO, SAMPLE_RATE_NATIVE, SAMPLE_RATE_OTHER };

#endif
