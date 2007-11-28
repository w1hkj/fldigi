# Copyright (C) 2007 Stelios Bounanos, M0GLD (m0gld AT enotty DOT net)

bin_PROGRAMS = fldigi


AM_CPPFLAGS = -I$(srcdir) -I$(srcdir)/include -I$(srcdir)/irrxml @BOOST_CPPFLAGS@

AM_CXXFLAGS = @FLTK_CFLAGS@ @PORTAUDIOCPP_CFLAGS@ @SNDFILE_CFLAGS@ \
	@SAMPLERATE_CFLAGS@ @HAMLIB_CFLAGS@ \
	-pipe -Wall -O2 -ffast-math -fno-rtti -fexceptions -finline-functions

LDADD = @BOOST_LDFLAGS@ @FLTK_LIBS@ @PORTAUDIOCPP_LIBS@ @SNDFILE_LIBS@ \
	@SAMPLERATE_LIBS@ @HAMLIB_LIBS@ @RTLIB@


SAMPLERATE_SRC = samplerate/common.h samplerate/fastest_coeffs.h \
	samplerate/float_cast.h samplerate/samplerate.c samplerate/samplerate.h \
	samplerate/src_linear.c samplerate/src_sinc.c samplerate/src_zoh.c \
	samplerate/srconfig.h
HAMLIB_SRC = include/hamlib.h rigcontrol/hamlib.cxx include/rigclass.h rigcontrol/rigclass.cxx
TRACE_SRC = misc/stacktrace.cxx

# We distribute these but do not always compile them
EXTRA_fldigi_SOURCES = $(SAMPLERATE_SRC) $(HAMLIB_SRC) $(TRACE_SRC)

fldigi_SOURCES =

if ENABLE_DEBUG
  AM_CPPFLAGS += -UNDEBUG
  fldigi_SOURCES += $(TRACE_SRC)
else
  AM_CPPFLAGS += -DNDEBUG
endif

if NO_SAMPLERATE
  AM_CPPFLAGS += -I$(srcdir)/samplerate
  fldigi_SOURCES += $(SAMPLERATE_SRC)
endif

if ENABLE_HAMLIB
  fldigi_SOURCES += $(HAMLIB_SRC)
endif


.EXPORT_ALL_VARIABLES: versions.h
versions.h: $(srcdir)/include/versions.h.in
	sh $(srcdir)/../scripts/mkversions.sh $^ $@ || echo > $@

# Sources that are generated,
BUILT_SOURCES = versions.h
# not distributed,
nodist_fldigi_SOURCES = $(BUILT_SOURCES)
# and deleted by the clean targets
CLEANFILES = $(BUILT_SOURCES)


install-data-local:
	if test -f $(srcdir)/../icons/fldigi.xpm; then \
	    $(mkinstalldirs) $(DESTDIR)$(datadir)/pixmaps; \
	    $(INSTALL_DATA) $(srcdir)/../icons/fldigi.xpm $(DESTDIR)/$(datadir)/pixmaps; \
	fi

uninstall-local:
	rm -f $(DESTDIR)/$(datadir)/pixmaps/fldigi.xpm

# Sources that we build. It is OK to have headers here.
fldigi_SOURCES += \
	combo/combo.cxx \
	cw_rtty/cw.cxx \
	cw_rtty/morse.cxx \
	cw_rtty/rtty.cxx \
	dialogs/Config.cxx \
	dialogs/fl_digi.cxx \
	dialogs/font_browser.cxx \
	dominoex/dominoex.cxx \
	dominoex/dominovar.cxx \
	feld/feld.cxx \
	feld/feldfonts.cxx \
	fft/fft.cxx \
	fileselector/File_Selector.cxx \
	fileselector/File_Selector2.cxx \
	fileselector/file_dir.cxx \
	filters/fftfilt.cxx \
	filters/filters.cxx \
	filters/viterbi.cxx \
	globals/globals.cxx \
	include/Combo_Box.h \
	include/Combo_List.h \
	include/Config.h \
	include/Enumerations.h \
	include/FLdigiTypes.h \
	include/FTextView.h \
	include/FileString.h \
	include/File_Selector.h \
	include/Fl_Text_Display_mod.H \
	include/Fl_Text_Editor_mod.H \
	include/FreqControl.h \
	include/TextView.h \
	include/analysis.h \
	include/ascii.h \
	include/colorbox.h \
	include/combo.h \
	include/complex.h \
	include/configuration.h \
	include/cw.h \
	include/digiscope.h \
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
	include/newinstall.h \
	include/olivia.h \
	include/picture.h \
	include/psk.h \
	include/pskcoeff.h \
	include/pskvaricode.h \
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
	include/rtty.h \
	include/serial.h \
	include/sound.h \
	include/status.h \
	include/testmodem.h \
	include/threads.h \
	include/throb.h \
	include/timeops.h \
	include/trx.h \
	include/versions.h.in \
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
	misc/flstring.c \
	misc/log.cxx \
	misc/macroedit.cxx \
	misc/macros.cxx \
	misc/misc.cxx \
	misc/newinstall.cxx \
	misc/pskmail.cxx \
	misc/qrzcall.cxx \
	misc/qrzlib.cxx \
	misc/status.cxx \
	misc/threads.cxx \
	misc/timeops.cxx \
	olivia/olivia.cxx \
	psk/psk.cxx \
	psk/pskcoeff.cxx \
	psk/pskvaricode.cxx \
	qrunner/fqueue.h \
	qrunner/qrunner.cxx \
	qrunner/ringbuffer.c \
	qrunner/ringbuffer.h \
	rigcontrol/FreqControl.cxx \
	rigcontrol/ptt.cxx \
	rigcontrol/rigMEM.cxx \
	rigcontrol/rigdialog.cxx \
	rigcontrol/rigdialog.h \
	rigcontrol/rigio.cxx \
	rigcontrol/rigsupport.cxx \
	rigcontrol/rigxml.cxx \
	rigcontrol/serial.cxx \
	soundcard/mixer.cxx \
	soundcard/sound.cxx \
	throb/throb.cxx \
	trx/modem.cxx \
	trx/trx.cxx \
	waterfall/colorbox.cxx \
	waterfall/digiscope.cxx \
	waterfall/raster.cxx \
	waterfall/waterfall.cxx \
	widgets/FTextView.cxx \
	widgets/Fl_Text_Display_mod.cxx \
	widgets/Fl_Text_Editor_mod.cxx \
	widgets/TextView.cxx \
	widgets/picture.cxx \
	wwv/analysis.cxx \
	wwv/wwv.cxx

# Sources that are part of the distribution but are not compiled
EXTRA_fldigi_SOURCES += \
	blank/blank.cxx \
	blank/blank.h \
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
	trx/tune.cxx

# Additional non-source files that we distribute
EXTRA_DIST = \
	$(srcdir)/../scripts/mkversions.sh \
	$(srcdir)/../icons/fldigi-psk.png \
	$(srcdir)/../icons/fldigi-rtty.png \
	$(srcdir)/../icons/fldigi.xpm \
	fileselector/allfiles.xbm \
	fileselector/d1.xbm \
	fileselector/d1_mask.xbm \
	fileselector/ew.xbm \
	fileselector/ew_mask.xbm \
	fileselector/new.xbm \
	fileselector/ns.xbm \
	fileselector/ns_mask.xbm \
	fileselector/up.xbm \
	dialogs/Config.fl \
	rigcontrol/rigdialog.fl
