## Copyright (C) 2007-2009 Stelios Bounanos, M0GLD (m0gld AT enotty DOT net)
# Copyright (c) 2008 Dave Freese, W1HKJ (w1hkj AT w1hkj DOT com)
# License: GPLv3+: GNU GPL version 3 or later.

bin_PROGRAMS =
if WANT_FLDIGI
    bin_PROGRAMS += fldigi
endif
if WANT_FLARQ
    bin_PROGRAMS += flarq
endif

# The BUILD_* variables are defined in build.m4
fldigi_CPPFLAGS = -DBUILD_FLDIGI -DLOCALEDIR=\"$(localedir)\" @FLDIGI_BUILD_CPPFLAGS@
fldigi_CXXFLAGS = @FLDIGI_BUILD_CXXFLAGS@
fldigi_CFLAGS = $(fldigi_CXXFLAGS)
fldigi_LDFLAGS = @FLDIGI_BUILD_LDFLAGS@
fldigi_LDADD = @FLDIGI_BUILD_LDADD@

flarq_CPPFLAGS = -DBUILD_FLARQ -DLOCALEDIR=\"$(localedir)\" @FLARQ_BUILD_CPPFLAGS@
flarq_CXXFLAGS = @FLARQ_BUILD_CXXFLAGS@
flarq_CFLAGS = $(flarq_CXXFLAGS)
flarq_LDFLAGS = @FLARQ_BUILD_LDFLAGS@
flarq_LDADD = @FLARQ_BUILD_LDADD@


HAMLIB_SRC = include/hamlib.h rigcontrol/hamlib.cxx include/rigclass.h rigcontrol/rigclass.cxx
XMLRPC_SRC = include/xmlrpc.h misc/xmlrpc.cxx
FLDIGI_WIN32_RES_SRC = fldigirc.rc
FLARQ_WIN32_RES_SRC = flarq-src/flarqrc.rc
COMMON_WIN32_RES_SRC = common.rc
LOCATOR_SRC = misc/locator.c
BENCHMARK_SRC = include/benchmark.h misc/benchmark.cxx
REGEX_SRC = compat/regex.h compat/regex.c
STACK_SRC = include/stack.h misc/stack.cxx
MINGW32_SRC = include/compat.h compat/getsysinfo.c compat/mingw.c compat/mingw.h
NLS_SRC = misc/nls.cxx

# We distribute these but do not always compile them
EXTRA_fldigi_SOURCES = $(HAMLIB_SRC) $(XMLRPC_SRC) $(FLDIGI_WIN32_RES_SRC) $(COMMON_WIN32_RES_SRC) \
	$(LOCATOR_SRC) $(BENCHMARK_SRC) $(REGEX_SRC) $(STACK_SRC) $(MINGW32_SRC) $(NLS_SRC)
EXTRA_flarq_SOURCES = $(FLARQ_WIN32_RES_SRC) $(COMMON_WIN32_RES_SRC)

fldigi_SOURCES =
flarq_SOURCES =

if ENABLE_HAMLIB
  fldigi_SOURCES += $(HAMLIB_SRC)
endif
if NEED_HAMLIB_LOCATOR
  fldigi_SOURCES += $(LOCATOR_SRC)
endif

if ENABLE_XMLRPC
  fldigi_SOURCES += $(XMLRPC_SRC)
endif

if ENABLE_BENCHMARK
  fldigi_SOURCES += $(BENCHMARK_SRC)
endif

if COMPAT_REGEX
  fldigi_SOURCES += $(REGEX_SRC)
  flarq_SOURCES += $(REGEX_SRC)
endif

if COMPAT_STACK
  fldigi_SOURCES += $(STACK_SRC)
  flarq_SOURCES += $(STACK_SRC)
endif

if MINGW32
  fldigi_SOURCES += $(MINGW32_SRC)
  flarq_SOURCES += $(MINGW32_SRC)
endif

if USE_NLS
  fldigi_SOURCES += $(NLS_SRC)
  flarq_SOURCES += $(NLS_SRC)
endif

########################################################################


FLDIGI_VERSION_MAJOR = @FLDIGI_VERSION_MAJOR@
FLDIGI_VERSION_MINOR = @FLDIGI_VERSION_MINOR@
FLDIGI_VERSION_PATCH = @FLDIGI_VERSION_PATCH@
FLDIGI_VERSION = @FLDIGI_VERSION@
FLARQ_VERSION_MAJOR = @FLARQ_VERSION_MAJOR@
FLARQ_VERSION_MINOR = @FLARQ_VERSION_MINOR@
FLARQ_VERSION_PATCH = @FLARQ_VERSION_PATCH@
FLARQ_VERSION = @FLARQ_VERSION@

# Define the custom silent rule function
@SILENT_CMDS@

.EXPORT_ALL_VARIABLES: appbundle nsisinst hamlib-static

# Sources that are generated,
BUILT_SOURCES =
# not distributed,
nodist_fldigi_SOURCES = $(BUILT_SOURCES)
# and deleted by the clean targets
CLEANFILES = $(BUILT_SOURCES)
CLEAN_LOCAL =

if WIN32
if HAVE_WINDRES
.rc.o:
	$(call silent,WRES  ,$@)$(WINDRES) -DRC_BUILD_TIME=\\\"$(shell date +%s)\\\" -I$(srcdir) -I$(srcdir)/include -I$(srcdir)/../data/win32 $< -O coff $@
fldigi_SOURCES += $(FLDIGI_WIN32_RES_SRC)
flarq_SOURCES += $(FLARQ_WIN32_RES_SRC)
endif
endif

install-exec-local:
if WANT_FLDIGI
if ENABLE_XMLRPC
	if test -f $(srcdir)/../scripts/fldigi-shell; then \
	    $(mkinstalldirs) $(DESTDIR)/$(bindir); \
            $(INSTALL_SCRIPT) $(srcdir)/../scripts/fldigi-shell $(DESTDIR)/$(bindir); \
        fi
endif
endif

install-data-local:
if WANT_FLDIGI
	if test -f $(srcdir)/../data/fldigi.xpm; then \
	    $(mkinstalldirs) $(DESTDIR)/$(datadir)/pixmaps; \
	    $(INSTALL_DATA) $(srcdir)/../data/fldigi.xpm $(DESTDIR)/$(datadir)/pixmaps; \
	fi
	if test -f $(srcdir)/../data/fldigi.desktop; then \
	    $(mkinstalldirs) $(DESTDIR)/$(datadir)/applications; \
	    $(INSTALL_DATA) $(srcdir)/../data/fldigi.desktop $(DESTDIR)/$(datadir)/applications; \
	fi
endif
if WANT_FLARQ
	if test -f $(srcdir)/../data/flarq.xpm; then \
	    $(mkinstalldirs) $(DESTDIR)/$(datadir)/pixmaps; \
	    $(INSTALL_DATA) $(srcdir)/../data/flarq.xpm $(DESTDIR)/$(datadir)/pixmaps; \
	fi
	if test -f $(srcdir)/../data/flarq.desktop; then \
	    $(mkinstalldirs) $(DESTDIR)/$(datadir)/applications; \
	    $(INSTALL_DATA) $(srcdir)/../data/flarq.desktop $(DESTDIR)/$(datadir)/applications; \
	fi
endif

uninstall-local:
if WANT_FLDIGI
	rm -f $(DESTDIR)$(bindir)/fldigi-shell
	rm -f $(DESTDIR)/$(datadir)/pixmaps/fldigi.xpm
	rm -f $(DESTDIR)/$(datadir)/applications/fldigi.desktop
endif
if WANT_FLARQ
	rm -f $(DESTDIR)/$(datadir)/pixmaps/flarq.xpm
	rm -f $(DESTDIR)/$(datadir)/applications/flarq.desktop
endif

FLDIGI_FL_SRC = dialogs/confdialog.fl dialogs/colorsfonts.fl \
		logbook/lgbook.fl dialogs/notifydialog.fl
FLARQ_FL_SRC = flarq-src/arqdialogs.fl

if HAVE_FLUID
flgen: $(FLDIGI_FL_SRC) $(FLARQ_FL_SRC)
if WANT_FLDIGI
	$(call silent,FLUID ,$(FLDIGI_FL_SRC))(cd $(srcdir)/include; \
	for f in $(FLDIGI_FL_SRC); do \
	    c=$${f%.fl}.cxx; h=$${f%.fl}.h; h=$${h##*/}; \
	    $(FLUID) -c -o ../$$c -h $$h ../$$f; \
	done)
endif
if WANT_FLARQ
	$(call silent,FLUID ,$(FLARQ_FL_SRC))(cd $(srcdir)/flarq-src/include; \
	for f in $(FLARQ_FL_SRC); do \
	    c=$${f%.fl}.cxx; h=$${f%.fl}.h; h=$${h##*/}; \
	    $(FLUID) -c -o ../../$$c -h $$h ../../$$f; \
	done)
endif
endif


if WANT_FLDIGI
    WANT_FLDIGI = yes
    INSTALLER_FILE = $(PACKAGE_TARNAME)-$(PACKAGE_VERSION)_setup.exe
    APPBUNDLE=$(PACKAGE_TARNAME)-$(PACKAGE_VERSION)
    APPBUNDLE_NOLIBS=$(APPBUNDLE)-nolibs
endif
if WANT_FLARQ
    WANT_FLARQ = yes
if !WANT_FLDIGI
        INSTALLER_FILE = flarq-$(FLARQ_VERSION)_setup.exe
        APPBUNDLE=flarq-$(FLARQ_VERSION)
        APPBUNDLE_NOLIBS=$(APPBUNDLE)-nolibs
endif
endif

if DARWIN
appbundle: $(bin_PROGRAMS)
	$(call silent,APPB  ,$(APPBUNDLE_NOLIBS) $(APPBUNDLE))sh $(srcdir)/../scripts/mkappbundle.sh "$(srcdir)/../data" .
    CLEAN_LOCAL += $(APPBUNDLE_NOLIBS) $(APPBUNDLE) $(APPBUNDLE)*.dmg
endif

if HAVE_NSIS
nsisinst: $(bin_PROGRAMS)
	$(call silent,NSIS  ,$(INSTALLER_FILE))sh $(srcdir)/../scripts/mknsisinst.sh "$(srcdir)/../data" .
    CLEANFILES += $(INSTALLER_FILE)
endif

if WANT_FLDIGI
if ENABLE_HAMLIB
hamlib-static: $(fldigi_OBJECTS)
	$(call silent,HLS   ,fldigi$(EXEEXT))sh $(srcdir)/../scripts/mkhamlibstatic.sh fldigi
endif
endif

TESTS = $(srcdir)/../scripts/tests/config-h.sh $(srcdir)/../scripts/tests/cr.sh

if HAVE_ASCIIDOC
$(builddir)/../doc/guide.html: $(builddir)/../doc/guide.txt
	@$(MAKE) -C $(builddir)/../doc $(AM_MAKEFLAGS) guide.html

$(builddir)/dialogs/guide.cxx: $(builddir)/../doc/guide.html
	@mkdir -p $(builddir)/dialogs
	$(call silent,GUIDE ,$@)sed 's/\\/\\\\/g; s/"/\\"/g; s/$$/\\n\\/g; 1 s/.*/const char* szBeginner = "&/; $$ s/.*/&n";/' $< > $@
dialogs/htmlstrings.cxx: $(builddir)/dialogs/guide.cxx

distclean-local:
	@if test "$(builddir)" != "$(srcdir)"; then \
	    rm -f $(builddir)/dialogs/guide.cxx; \
        fi
endif

clean-local:
	-rm -rf $(CLEAN_LOCAL)

# Sources that we build. It is OK to have headers here.
fldigi_SOURCES += \
	combo/combo.cxx \
	cw_rtty/cw.cxx \
	cw_rtty/morse.cxx \
	cw_rtty/rtty.cxx \
	contestia/contestia.cxx \
	dialogs/colorsfonts.cxx \
	dialogs/confdialog.cxx \
	dialogs/fl_digi.cxx \
	dialogs/font_browser.cxx \
	dialogs/Viewer.cxx \
	dialogs/htmlstrings.cxx \
	dialogs/notifydialog.cxx \
	thor/thor.cxx \
	thor/thorvaricode.cxx \
	dominoex/dominoex.cxx \
	dominoex/dominovar.cxx \
	feld/feld.cxx \
	feld/feldfonts.cxx \
	fft/fft.cxx \
	fileselector/FL/Fl_Native_File_Chooser.H \
	fileselector/Fl_Native_File_Chooser.cxx \
	fileselector/fileselect.cxx \
	filters/fftfilt.cxx \
	filters/filters.cxx \
	filters/viterbi.cxx \
	globals/globals.cxx \
	include/htmlstrings.h \
	include/arq_io.h \
	include/confdialog.h \
	include/FTextView.h \
	include/FTextRXTX.h \
	include/fileselect.h \
	include/Fl_Text_Display_mod.H \
	include/Fl_Text_Editor_mod.H \
	include/Fl_Tile_Check.h \
	include/FreqControl.h \
	include/analysis.h \
	include/ascii.h \
	include/colorbox.h \
	include/colorsfonts.h \
	include/combo.h \
	include/complex.h \
	include/configuration.h \
	include/contestia.h \
	include/cw.h \
	include/debug.h \
	include/digiscope.h \
	include/dxcc.h \
	include/thor.h \
	include/thorvaricode.h \
	include/dominoex.h \
	include/dominovar.h \
	include/feld.h \
	include/fft.h \
	include/fftfilt.h \
	include/filters.h \
	include/fl_digi.h \
	include/fl_lock.h \
	include/fldigi-config.h \
	include/flinput2.h \
	include/flmisc.h \
	include/flslider2.h \
	include/flstring.h \
	include/font_browser.h \
	include/fontdef.h \
	include/gettext.h \
	include/globals.h \
	include/icons.h \
	include/interleave.h \
	include/jalocha/pj_cmpx.h \
	include/jalocha/pj_fft.h \
	include/jalocha/pj_fht.h \
	include/jalocha/pj_fifo.h \
	include/jalocha/pj_gray.h \
	include/jalocha/pj_lowpass3.h \
	include/jalocha/pj_mfsk.h \
	include/jalocha/pj_struc.h \
	include/locator.h \
	include/log.h \
	include/logger.h \
	include/macroedit.h \
	include/macros.h \
	include/main.h \
	include/mbuffer.h \
	include/mfsk.h \
	include/mfskvaricode.h \
	include/misc.h \
	include/mixer.h \
	include/modem.h \
	include/morse.h \
	include/mt63base.h \
	include/mt63.h \
	include/network.h \
	include/dsp.h \
	include/newinstall.h \
	include/notify.h \
	include/notifydialog.h \
	include/olivia.h \
	include/pkg.h \
	include/picture.h \
	include/progress.h \
	include/psk.h \
	include/viewpsk.h \
	include/pskcoeff.h \
	include/pskvaricode.h \
	include/pskeval.h \
	include/ptt.h \
	include/pixmaps.h \
	include/pskrep.h \
	include/qrunner.h \
	include/lookupcall.h \
	include/qrzlib.h \
	include/raster.h \
	include/re.h \
	include/rigCAT.h \
	include/rigMEM.h \
	include/rigio.h \
	include/rigsupport.h \
	include/rigxml.h \
	include/ringbuffer.h \
	include/rsid.h \
	include/rtty.h \
	include/rx_extract.h \
	include/speak.h \
	include/serial.h \
	include/socket.h \
	include/sound.h \
	include/soundconf.h \
	include/spot.h \
	include/ssb.h \
	include/stacktrace.h \
	include/status.h \
	include/strutil.h \
	include/testmodem.h \
	include/threads.h \
	include/throb.h \
	include/timeops.h \
	include/trx.h \
	include/util.h \
	include/Viewer.h \
	include/viterbi.h \
	include/waterfall.h \
	include/wwv.h \
	include/xmlreader.h \
	include/adif_def.h \
	include/adif_io.h \
	include/calendar.h \
	include/date.h \
	include/field_def.h \
	include/logbook.h \
	include/lgbook.h \
	include/logsupport.h \
	include/qso_db.h \
	include/table.h \
	include/textio.h \
	irrxml/CXMLReaderImpl.h \
	irrxml/fast_atof.h \
	irrxml/heapsort.h \
	irrxml/irrArray.h \
	irrxml/irrString.h \
	irrxml/irrTypes.h \
	irrxml/irrXML.cpp \
	irrxml/irrXML.h \
	logbook/adif_io.cxx \
	logbook/calendar.cxx \
	logbook/date.cxx \
	logbook/lgbook.cxx \
	logbook/logbook.cxx \
	logbook/logsupport.cxx \
	logbook/qso_db.cxx \
	logbook/table.cxx \
	logbook/textio.cxx \
	logger/logger.cxx \
	logger/rx_extract.cxx \
	logger/speak.cxx \
	main.cxx \
	mfsk/interleave.cxx \
	mfsk/mfsk.cxx \
	mfsk/mfskvaricode.cxx \
	misc/ascii.cxx \
	misc/configuration.cxx \
	misc/debug.cxx \
	misc/dxcc.cxx \
	misc/flstring.c \
	misc/icons.cxx \
	misc/log.cxx \
	misc/macroedit.cxx \
	misc/macros.cxx \
	misc/misc.cxx \
	misc/network.cxx \
	misc/newinstall.cxx \
	misc/arq_io.cxx \
	misc/lookupcall.cxx \
	misc/pixmaps.cxx \
	misc/qrzlib.cxx \
	misc/re.cxx \
	misc/socket.cxx \
	misc/stacktrace.cxx \
	misc/status.cxx \
	misc/strutil.cxx \
	misc/threads.cxx \
	misc/timeops.cxx \
	misc/util.cxx \
	mt63/dsp.cxx \
	mt63/mt63.cxx \
	mt63/mt63base.cxx \
	olivia/olivia.cxx \
	psk/psk.cxx \
	psk/pskcoeff.cxx \
	psk/pskvaricode.cxx \
	psk/viewpsk.cxx \
	psk/pskeval.cxx \
	qrunner/fqueue.h \
	qrunner/qrunner.cxx \
	rigcontrol/FreqControl.cxx \
	rigcontrol/ptt.cxx \
	rigcontrol/rigMEM.cxx \
	rigcontrol/rigio.cxx \
	rigcontrol/rigsupport.cxx \
	rigcontrol/rigxml.cxx \
	rigcontrol/serial.cxx \
	rsid/rsid.cxx \
	soundcard/mixer.cxx \
	soundcard/sound.cxx \
	soundcard/soundconf.cxx \
	spot/notify.cxx \
	spot/pskrep.cxx \
	spot/spot.cxx \
	ssb/ssb.cxx \
	throb/throb.cxx \
	trx/modem.cxx \
	trx/trx.cxx \
	waterfall/colorbox.cxx \
	waterfall/digiscope.cxx \
	waterfall/raster.cxx \
	waterfall/waterfall.cxx \
	widgets/FTextView.cxx \
	widgets/FTextRXTX.cxx \
	widgets/Fl_Text_Display_mod.cxx \
	widgets/Fl_Text_Editor_mod.cxx \
	widgets/Fl_Tile_Check.cxx \
	widgets/flinput2.cxx \
	widgets/flmisc.cxx \
	widgets/flslider2.cxx \
	widgets/picture.cxx \
	widgets/progress.cxx \
	wwv/analysis.cxx \
	wwv/wwv.cxx

# Sources that are part of the distribution but are not compiled directly
EXTRA_fldigi_SOURCES += \
	blank/blank.cxx \
	blank/blank.h \
	fileselector/FL/Fl_Native_File_Chooser_FLTK.H \
	fileselector/FL/Fl_Native_File_Chooser_MAC.H \
	fileselector/FL/Fl_Native_File_Chooser_WIN32.H \
	fileselector/Fl_Native_File_Chooser_FLTK.cxx \
	fileselector/Fl_Native_File_Chooser_MAC.cxx \
	fileselector/Fl_Native_File_Chooser_WIN32.cxx \
	fileselector/flnfc_common.cxx \
	feld/Feld7x7-14.cxx \
	feld/Feld7x7n-14.cxx \
	feld/FeldDx-14.cxx \
	feld/FeldFat-14.cxx \
	feld/FeldHell-12.cxx \
	feld/FeldLittle-12.cxx \
	feld/FeldLo8-14.cxx \
	feld/FeldLow-14.cxx \
	feld/FeldModern-14.cxx \
	feld/FeldModern8-14.cxx \
	feld/FeldNarr-14.cxx \
	feld/FeldReal-14.cxx \
	feld/FeldStyl-14.cxx \
	feld/FeldVert-14.cxx \
	feld/FeldWide-14.cxx \
	mfsk/mfsk-pic.cxx \
	mt63/alias_1k.dat \
	mt63/alias_2k.dat \
	mt63/morse.dat \
	mt63/symbol.dat \
	mt63/alias_k5.dat \
	mt63/mt63intl.dat \
	trx/tune.cxx \
	dialogs/guide.cxx

flarq_SOURCES += \
	dialogs/font_browser.cxx \
	flarq-src/arq.cxx \
	flarq-src/arqdialogs.cxx \
	flarq-src/arqhelp.cxx \
	flarq-src/b64.cxx \
	flarq-src/flarq.cxx \
	flarq-src/flarqenv.cxx \
	flarq-src/include/arq.h \
	flarq-src/include/arqdialogs.h \
	flarq-src/include/b64.h \
	flarq-src/include/flarq.h \
	flarq-src/include/flarqenv.h \
	include/Fl_Text_Display_mod.H \
	include/Fl_Text_Editor_mod.H \
	include/FTextView.h \
	include/debug.h \
	include/fileselect.h \
	include/flinput2.h \
	include/flmisc.h \
	include/flslider2.h \
	include/font_browser.h \
	include/icons.h \
	include/pixmaps.h \
	include/re.h \
	include/socket.h \
	include/stacktrace.h \
	include/threads.h \
	include/table.h \
	include/util.h \
	combo/combo.cxx \
	fileselector/FL/Fl_Native_File_Chooser.H \
	fileselector/Fl_Native_File_Chooser.cxx \
	fileselector/fileselect.cxx \
	logbook/table.cxx \
	misc/ascii.cxx \
	misc/debug.cxx \
	misc/stacktrace.cxx \
	misc/threads.cxx \
	misc/timeops.cxx \
	widgets/flinput2.cxx \
	widgets/flmisc.cxx \
	widgets/flslider2.cxx \
	misc/icons.cxx \
	misc/pixmaps.cxx \
	misc/re.cxx \
	misc/socket.cxx \
	misc/util.cxx \
	widgets/Fl_Text_Display_mod.cxx \
	widgets/Fl_Text_Editor_mod.cxx \
	widgets/FTextView.cxx

# Additional non-source files that we distribute
EXTRA_DIST = \
	$(srcdir)/../scripts/mkappbundle.sh \
	$(srcdir)/../scripts/mkhamlibstatic.sh \
	$(srcdir)/../scripts/fldigi-shell \
	$(srcdir)/../scripts/tests/cr.sh \
	$(srcdir)/../scripts/tests/config-h.sh \
	$(srcdir)/../data/fldigi-psk.png \
	$(srcdir)/../data/fldigi-rtty.png \
	$(srcdir)/../data/fldigi.xpm \
	$(srcdir)/../data/fldigi.desktop \
	$(srcdir)/../data/mac/Info.plist.in \
	$(srcdir)/../data/mac/fldigi.icns \
	$(srcdir)/../data/win32/fldigi.ico \
	$(srcdir)/../data/flarq.desktop \
	$(srcdir)/../data/flarq.xpm \
	$(srcdir)/../data/mac/flarq.icns \
	$(srcdir)/../data/win32/flarq.ico \
	$(FLDIGI_FL_SRC) \
	$(FLARQ_FL_SRC)
