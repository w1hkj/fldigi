# Copyright (C) 2007 Stelios Bounanos, M0GLD (m0gld AT enotty DOT net)
# Copyright (c) 2008 Dave Freese, W1HKJ (w1hkj AT w1hkj DOT com)

bin_PROGRAMS = fldigi


AM_CPPFLAGS = -I$(srcdir) -I$(srcdir)/include -I$(srcdir)/irrxml -I$(srcdir)/fileselector @BOOST_CPPFLAGS@

AM_CXXFLAGS = @PORTAUDIO_CFLAGS@ @FLTK_CFLAGS@ @SNDFILE_CFLAGS@ \
	@SAMPLERATE_CFLAGS@ @PULSEAUDIO_CFLAGS@ @HAMLIB_CFLAGS@ \
	@IMAGE_CFLAGS@ @XMLRPC_CFLAGS@ @MAC_UNIVERSAL_CFLAGS@ \
	-pipe -Wall -fexceptions @OPT_CFLAGS@ @DEBUG_CFLAGS@
AM_CFLAGS = $(AM_CXXFLAGS)

AM_LDFLAGS = @MAC_UNIVERSAL_LDFLAGS@

LDADD = @PORTAUDIO_LIBS@ @BOOST_LDFLAGS@ @FLTK_LIBS@ @SNDFILE_LIBS@ \
	@SAMPLERATE_LIBS@ @PULSEAUDIO_LIBS@ @HAMLIB_LIBS@ @IMAGE_LIBS@ @XMLRPC_LIBS@ @EXTRA_LIBS@


HAMLIB_SRC = include/hamlib.h rigcontrol/hamlib.cxx include/rigclass.h rigcontrol/rigclass.cxx
XMLRPC_SRC = include/xmlrpc.h misc/xmlrpc.cxx
WIN32_RES_SRC = fldigirc.rc

# We distribute these but do not always compile them
EXTRA_fldigi_SOURCES = $(HAMLIB_SRC) $(XMLRPC_SRC) $(WIN32_RES_SRC)

fldigi_SOURCES =

if ENABLE_DEBUG
  AM_CPPFLAGS += -UNDEBUG
  AM_LDFLAGS += @RDYNAMIC@
else
  AM_CPPFLAGS += -DNDEBUG
endif

if ENABLE_HAMLIB
  fldigi_SOURCES += $(HAMLIB_SRC)
endif

if ENABLE_XMLRPC
  fldigi_SOURCES += $(XMLRPC_SRC)
endif

FLDIGI_VERSION_MAJOR = @FLDIGI_VERSION_MAJOR@
FLDIGI_VERSION_MINOR = @FLDIGI_VERSION_MINOR@
FLDIGI_VERSION_PATCH = @FLDIGI_VERSION_PATCH@

.EXPORT_ALL_VARIABLES: versions.h appbundle hamlib-static

versions.h: $(srcdir)/include/versions.h.in
	sh $(srcdir)/../scripts/mkversions.sh $^ $@ || echo > $@

# Sources that are generated,
BUILT_SOURCES = versions.h
# not distributed,
nodist_fldigi_SOURCES = $(BUILT_SOURCES)
# and deleted by the clean targets
CLEANFILES = $(BUILT_SOURCES)

CLEAN_LOCAL_FILES =

if WIN32
if HAVE_WINDRES
.rc.o:
	$(WINDRES) -I$(srcdir)/include -I$(srcdir)/../data/win32 $< -O coff $@
fldigi_SOURCES += $(WIN32_RES_SRC)
endif
endif

install-data-local:
	if test -f $(srcdir)/../data/fldigi.xpm; then \
	    $(mkinstalldirs) $(DESTDIR)$(datadir)/pixmaps; \
	    $(INSTALL_DATA) $(srcdir)/../data/fldigi.xpm $(DESTDIR)/$(datadir)/pixmaps; \
	fi
	if test -f $(srcdir)/../data/fldigi.desktop; then \
	    $(mkinstalldirs) $(DESTDIR)$(datadir)/applications; \
	    $(INSTALL_DATA) $(srcdir)/../data/fldigi.desktop $(DESTDIR)/$(datadir)/applications; \
	fi

uninstall-local:
	rm -f $(DESTDIR)/$(datadir)/pixmaps/fldigi.xpm
	rm -f $(DESTDIR)/$(datadir)/applications/fldigi.desktop

if HAVE_FLUID
flgen: $(srcdir)/dialogs/confdialog.fl $(srcdir)/rigcontrol/rigdialog.fl
	(cd $(srcdir)/include; \
	$(FLUID) -c -o ../dialogs/confdialog.cxx -h confdialog.h ../dialogs/confdialog.fl; \
	$(FLUID) -c -o ../rigcontrol/rigdialog.cxx -h rigdialog.h ../rigcontrol/rigdialog.fl)
endif

if DARWIN
appbundle:
	sh $(srcdir)/../scripts/mkappbundle.sh "$(srcdir)/../data" . \
	$(PACKAGE_TARNAME)-$(PACKAGE_VERSION)-bundle-nolibs \
	$(PACKAGE_TARNAME)-$(PACKAGE_VERSION)-bundle
CLEAN_LOCAL_FILES += $(PACKAGE_TARNAME)-$(PACKAGE_VERSION)-bundle-nolibs \
	$(PACKAGE_TARNAME)-$(PACKAGE_VERSION)-bundle \
	$(PACKAGE_TARNAME)-$(PACKAGE_VERSION)*.dmg
endif

if ENABLE_HAMLIB
hamlib-static:
	sh $(srcdir)/../scripts/mkhamlibstatic.sh hls-tmp
CLEAN_LOCAL_FILES += hls-tmp
endif

clean-local:
	@rm -rf $(CLEAN_LOCAL_FILES)

TESTS = $(srcdir)/../scripts/tests/config-h.sh

# Sources that we build. It is OK to have headers here.
fldigi_SOURCES += \
	combo/combo.cxx \
	cw_rtty/cw.cxx \
	cw_rtty/morse.cxx \
	cw_rtty/rtty.cxx \
	dialogs/confdialog.cxx \
	dialogs/fl_digi.cxx \
	dialogs/font_browser.cxx \
	dialogs/Viewer.cxx \
	dialogs/htmlstrings.cxx \
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
	include/afcind.h \
	include/arq_io.h \
	include/Combo_Box.h \
	include/Combo_List.h \
	include/confdialog.h \
	include/Enumerations.h \
	include/FLdigiTypes.h \
	include/FTextView.h \
	include/fileselect.h \
	include/Fl_Text_Display_mod.H \
	include/Fl_Text_Editor_mod.H \
	include/FreqControl.h \
	include/analysis.h \
	include/ascii.h \
	include/colorbox.h \
	include/colorsfonts.h \
	include/combo.h \
	include/complex.h \
	include/configuration.h \
	include/cw.h \
	include/digiscope.h \
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
	include/fldigi-icon-48.xpm \
	include/flstring.h \
	include/font_browser.h \
	include/fontdef.h \
	include/globals.h \
	include/id.h \
	include/interleave.h \
	include/jalocha/pj_cmpx.h \
	include/jalocha/pj_fft.h \
	include/jalocha/pj_fht.h \
	include/jalocha/pj_fifo.h \
	include/jalocha/pj_gray.h \
	include/jalocha/pj_lowpass3.h \
	include/jalocha/pj_mfsk.h \
	include/jalocha/pj_struc.h \
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
	include/dsp.h \
	include/newinstall.h \
	include/olivia.h \
	include/picture.h \
	include/progress.h \
	include/psk.h \
	include/viewpsk.h \
	include/pskcoeff.h \
	include/pskvaricode.h \
	include/pskeval.h \
	include/ptt.h \
	include/qrunner.h \
	include/qrzcall.h \
	include/qrzlib.h \
	include/raster.h \
	include/rigCAT.h \
	include/rigMEM.h \
	include/rigdialog.h \
	include/rigio.h \
	include/rigsupport.h \
	include/rigxml.h \
	include/ringbuffer.h \
	include/rsid.h \
	include/rtty.h \
	include/serial.h \
	include/socket.h \
	include/sound.h \
	include/soundconf.h \
	include/stacktrace.h \
	include/status.h \
	include/testmodem.h \
	include/threads.h \
	include/throb.h \
	include/timeops.h \
	include/trx.h \
	include/util.h \
	include/versions.h.in \
	include/Viewer.h \
	include/viterbi.h \
	include/waterfall.h \
	include/wwv.h \
	include/xmlreader.h \
	irrxml/CXMLReaderImpl.h \
	irrxml/fast_atof.h \
	irrxml/heapsort.h \
	irrxml/irrArray.h \
	irrxml/irrString.h \
	irrxml/irrTypes.h \
	irrxml/irrXML.cpp \
	irrxml/irrXML.h \
	logger/logger.cxx \
	main.cxx \
	mfsk/interleave.cxx \
	mfsk/mfsk.cxx \
	mfsk/mfskvaricode.cxx \
	misc/ascii.cxx \
	misc/configuration.cxx \
	misc/colorsfonts.cxx \
	misc/flstring.c \
	misc/log.cxx \
	misc/macroedit.cxx \
	misc/macros.cxx \
	misc/misc.cxx \
	misc/newinstall.cxx \
	misc/arq_io.cxx \
	misc/qrzcall.cxx \
	misc/qrzlib.cxx \
	misc/socket.cxx \
	misc/stacktrace.cxx \
	misc/status.cxx \
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
	rigcontrol/rigdialog.cxx \
	rigcontrol/rigio.cxx \
	rigcontrol/rigsupport.cxx \
	rigcontrol/rigxml.cxx \
	rigcontrol/serial.cxx \
	rsid/rsid.cxx \
	soundcard/mixer.cxx \
	soundcard/sound.cxx \
	soundcard/soundconf.cxx \
	throb/throb.cxx \
	trx/modem.cxx \
	trx/trx.cxx \
	waterfall/colorbox.cxx \
	waterfall/digiscope.cxx \
	waterfall/raster.cxx \
	waterfall/waterfall.cxx \
	widgets/afcind.cxx \
	widgets/FTextView.cxx \
	widgets/Fl_Text_Display_mod.cxx \
	widgets/Fl_Text_Editor_mod.cxx \
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
	rigcontrol/linserial.cxx \
	rigcontrol/winserial.cxx \
	rsid/rsid_fft.cxx \
	rsid/rsid_fft.h \
	include/linserial.h \
	include/winserial.h

# Additional non-source files that we distribute
EXTRA_DIST = \
	$(srcdir)/../scripts/mkversions.sh \
	$(srcdir)/../scripts/mkappbundle.sh \
	$(srcdir)/../scripts/mkhamlibstatic.sh \
	$(srcdir)/../scripts/fldigi-shell \
	$(srcdir)/../scripts/tests/config-h.sh \
	$(srcdir)/../data/fldigi-psk.png \
	$(srcdir)/../data/fldigi-rtty.png \
	$(srcdir)/../data/fldigi.xpm \
	$(srcdir)/../data/fldigi.desktop \
	$(srcdir)/../data/mac/Info.plist.in \
	$(srcdir)/../data/mac/fldigi.icns \
	$(srcdir)/../data/win32/fldigi.ico \
	dialogs/confdialog.fl \
	rigcontrol/rigdialog.fl
