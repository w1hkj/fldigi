## Copyright (C) 2007-2009 Stelios Bounanos, M0GLD (m0gld AT enotty DOT net)
# Copyright (c) 2008 Dave Freese, W1HKJ (w1hkj AT w1hkj DOT com)
# Copyright (c) 2012 Remi Chateauneu, F4ECW (remi dot chateauneu AT gmail DOT com)
# License: GPLv3+: GNU GPL version 3 or later.

bin_PROGRAMS =
SUBDIRS =
if WANT_FLDIGI
    bin_PROGRAMS += fldigi
endif
if WANT_FLARQ
    bin_PROGRAMS += flarq
endif

# The BUILD_* variables are defined in build.m4
fldigi_CPPFLAGS = -DBUILD_FLDIGI -DLOCALEDIR=\"$(localedir)\" @FLDIGI_BUILD_CPPFLAGS@ -DPKGDATADIR=\"$(pkgdatadir)\"
fldigi_CXXFLAGS = @FLDIGI_BUILD_CXXFLAGS@
fldigi_CFLAGS = @FLDIGI_BUILD_CFLAGS@
fldigi_LDFLAGS = @FLDIGI_BUILD_LDFLAGS@
fldigi_LDADD = @FLDIGI_BUILD_LDADD@

flarq_CPPFLAGS = -DBUILD_FLARQ -DLOCALEDIR=\"$(localedir)\" @FLARQ_BUILD_CPPFLAGS@
flarq_CXXFLAGS = @FLARQ_BUILD_CXXFLAGS@
flarq_CFLAGS = $(fldigi_CFLAGS)
flarq_LDFLAGS = @FLARQ_BUILD_LDFLAGS@
flarq_LDADD = @FLARQ_BUILD_LDADD@

if WIN32
fldigi_CPPFLAGS += -fno-var-tracking
fldigi_CXXFLAGS += -fno-var-tracking
endif

HAMLIB_SRC = \
include/hamlib.h \
rigcontrol/hamlib.cxx \
include/rigclass.h \
rigcontrol/rigclass.cxx

XMLRPC_SRC = \
include/xmlrpc.h \
network/xmlrpc.cxx

FLDIGI_WIN32_RES_SRC = fldigirc.rc

FLARQ_WIN32_RES_SRC = flarq-src/flarqrc.rc

COMMON_WIN32_RES_SRC = common.rc

BENCHMARK_SRC = include/benchmark.h misc/benchmark.cxx

REGEX_SRC = compat/regex.h compat/regex.c

STACK_SRC = include/stack.h misc/stack.cxx

MINGW32_SRC = include/compat.h compat/getsysinfo.c compat/mingw.c compat/mingw.h

NLS_SRC = misc/nls.cxx include/nls.h

# We distribute these but do not always compile them
# Build the xmlrpcpp source if libflxmlrpc is not found
XMLRPCPP_SRC = \
	xmlrpcpp/XmlRpcBase64.h \
	xmlrpcpp/XmlRpcClient.cpp \
	xmlrpcpp/XmlRpcClient.h \
	xmlrpcpp/XmlRpcDispatch.cpp \
	xmlrpcpp/XmlRpcDispatch.h \
	xmlrpcpp/XmlRpcException.h \
	xmlrpcpp/XmlRpc.h \
	xmlrpcpp/XmlRpcMutex.cpp \
	xmlrpcpp/XmlRpcMutex.h \
	xmlrpcpp/XmlRpcServerConnection.cpp \
	xmlrpcpp/XmlRpcServerConnection.h \
	xmlrpcpp/XmlRpcServer.cpp \
	xmlrpcpp/XmlRpcServer.h \
	xmlrpcpp/XmlRpcServerMethod.cpp \
	xmlrpcpp/XmlRpcServerMethod.h \
	xmlrpcpp/XmlRpcSocket.cpp \
	xmlrpcpp/XmlRpcSocket.h \
	xmlrpcpp/XmlRpcSource.cpp \
	xmlrpcpp/XmlRpcSource.h \
	xmlrpcpp/XmlRpcUtil.cpp \
	xmlrpcpp/XmlRpcUtil.h \
	xmlrpcpp/XmlRpcValue.cpp \
	xmlrpcpp/XmlRpcValue.h

# Build the libmbedtls source if libmbedtls is not found
LIBMBEDTLS_SRC = \
	mbedtls/aes.c \
	mbedtls/aesni.c \
	mbedtls/arc4.c \
	mbedtls/aria.c \
	mbedtls/asn1parse.c \
	mbedtls/asn1write.c \
	mbedtls/base64.c \
	mbedtls/bignum.c \
	mbedtls/blowfish.c \
	mbedtls/camellia.c \
	mbedtls/ccm.c \
	mbedtls/certs.c \
	mbedtls/chacha20.c \
	mbedtls/chachapoly.c \
	mbedtls/cipher.c \
	mbedtls/cipher_wrap.c \
	mbedtls/cmac.c \
	mbedtls/ctr_drbg.c \
	mbedtls/debug.c \
	mbedtls/des.c \
	mbedtls/dhm.c \
	mbedtls/ecdh.c \
	mbedtls/ecdsa.c \
	mbedtls/ecjpake.c \
	mbedtls/ecp.c \
	mbedtls/ecp_curves.c \
	mbedtls/entropy.c \
	mbedtls/entropy_poll.c \
	mbedtls/error.c \
	mbedtls/gcm.c \
	mbedtls/havege.c \
	mbedtls/hkdf.c \
	mbedtls/hmac_drbg.c \
	mbedtls/md2.c \
	mbedtls/md4.c \
	mbedtls/md5.c \
	mbedtls/md.c \
	mbedtls/md_wrap.c \
	mbedtls/memory_buffer_alloc.c \
	mbedtls/net_sockets.c \
	mbedtls/nist_kw.c \
	mbedtls/oid.c \
	mbedtls/padlock.c \
	mbedtls/pem.c \
	mbedtls/pk.c \
	mbedtls/pkcs11.c \
	mbedtls/pkcs12.c \
	mbedtls/pkcs5.c \
	mbedtls/pkparse.c \
	mbedtls/pk_wrap.c \
	mbedtls/pkwrite.c \
	mbedtls/platform.c \
	mbedtls/platform_util.c \
	mbedtls/poly1305.c \
	mbedtls/ripemd160.c \
	mbedtls/rsa.c \
	mbedtls/rsa_internal.c \
	mbedtls/sha1.c \
	mbedtls/sha256.c \
	mbedtls/sha512.c \
	mbedtls/ssl_cache.c \
	mbedtls/ssl_ciphersuites.c \
	mbedtls/ssl_cli.c \
	mbedtls/ssl_cookie.c \
	mbedtls/ssl_srv.c \
	mbedtls/ssl_ticket.c \
	mbedtls/ssl_tls.c \
	mbedtls/threading.c \
	mbedtls/timing.c \
	mbedtls/version.c \
	mbedtls/version_features.c \
	mbedtls/x509.c \
	mbedtls/x509_create.c \
	mbedtls/x509_crl.c \
	mbedtls/x509_crt.c \
	mbedtls/x509_csr.c \
	mbedtls/x509write_crt.c \
	mbedtls/x509write_csr.c \
	mbedtls/xtea.c \
	include/mbedtls/aes.h \
	include/mbedtls/aesni.h \
	include/mbedtls/arc4.h \
	include/mbedtls/aria.h \
	include/mbedtls/asn1.h \
	include/mbedtls/asn1write.h \
	include/mbedtls/base64.h \
	include/mbedtls/bignum.h \
	include/mbedtls/blowfish.h \
	include/mbedtls/bn_mul.h \
	include/mbedtls/camellia.h \
	include/mbedtls/ccm.h \
	include/mbedtls/certs.h \
	include/mbedtls/chacha20.h \
	include/mbedtls/chachapoly.h \
	include/mbedtls/check_config.h \
	include/mbedtls/cipher.h \
	include/mbedtls/cipher_internal.h \
	include/mbedtls/cmac.h \
	include/mbedtls/compat-1.3.h \
	include/mbedtls/config.h \
	include/mbedtls/ctr_drbg.h \
	include/mbedtls/debug.h \
	include/mbedtls/des.h \
	include/mbedtls/dhm.h \
	include/mbedtls/ecdh.h \
	include/mbedtls/ecdsa.h \
	include/mbedtls/ecjpake.h \
	include/mbedtls/ecp.h \
	include/mbedtls/ecp_internal.h \
	include/mbedtls/entropy.h \
	include/mbedtls/entropy_poll.h \
	include/mbedtls/error.h \
	include/mbedtls/gcm.h \
	include/mbedtls/havege.h \
	include/mbedtls/hkdf.h \
	include/mbedtls/hmac_drbg.h \
	include/mbedtls/md2.h \
	include/mbedtls/md4.h \
	include/mbedtls/md5.h \
	include/mbedtls/md.h \
	include/mbedtls/md_internal.h \
	include/mbedtls/memory_buffer_alloc.h \
	include/mbedtls/net.h \
	include/mbedtls/net_sockets.h \
	include/mbedtls/nist_kw.h \
	include/mbedtls/oid.h \
	include/mbedtls/padlock.h \
	include/mbedtls/pem.h \
	include/mbedtls/pkcs11.h \
	include/mbedtls/pkcs12.h \
	include/mbedtls/pkcs5.h \
	include/mbedtls/pk.h \
	include/mbedtls/pk_internal.h \
	include/mbedtls/platform.h \
	include/mbedtls/platform_time.h \
	include/mbedtls/platform_util.h \
	include/mbedtls/poly1305.h \
	include/mbedtls/ripemd160.h \
	include/mbedtls/rsa.h \
	include/mbedtls/rsa_internal.h \
	include/mbedtls/sha1.h \
	include/mbedtls/sha256.h \
	include/mbedtls/sha512.h \
	include/mbedtls/ssl_cache.h \
	include/mbedtls/ssl_ciphersuites.h \
	include/mbedtls/ssl_cookie.h \
	include/mbedtls/ssl.h \
	include/mbedtls/ssl_internal.h \
	include/mbedtls/ssl_ticket.h \
	include/mbedtls/threading.h \
	include/mbedtls/timing.h \
	include/mbedtls/version.h \
	include/mbedtls/x509_crl.h \
	include/mbedtls/x509_crt.h \
	include/mbedtls/x509_csr.h \
	include/mbedtls/x509.h \
	include/mbedtls/xtea.h


EXTRA_fldigi_SOURCES = \
	$(HAMLIB_SRC) $(XMLRPC_SRC) $(FLDIGI_WIN32_RES_SRC) \
	$(COMMON_WIN32_RES_SRC) $(BENCHMARK_SRC) $(REGEX_SRC) $(STACK_SRC) \
	$(MINGW32_SRC) $(NLS_SRC) $(XMLRPCPP_SRC) $(LIBMBEDTLS_SRC)

EXTRA_flarq_SOURCES = $(FLARQ_WIN32_RES_SRC) $(COMMON_WIN32_RES_SRC) $(XMLRPCPP_SRC)

fldigi_SOURCES =
flarq_SOURCES =

fldigi_SOURCES += $(XMLRPC_SRC)

if !ENABLE_FLXMLRPC
fldigi_SOURCES += $(XMLRPCPP_SRC)
flarq_SOURCES  += $(XMLRPCPP_SRC)
else
fldigi_CPPFLAGS += @FLXMLRPC_CFLAGS@
fldigi_CXXFLAGS += @FLXMLRPC_CFLAGS@
fldigi_CFLAGS   += @FLXMLRPC_CFLAGS@
fldigi_LDFLAGS  += @FLXMLRPC_LIBS@
flarq_CPPFLAGS += @FLXMLRPC_CFLAGS@
flarq_CXXFLAGS += @FLXMLRPC_CFLAGS@
flarq_CFLAGS   += @FLXMLRPC_CFLAGS@
flarq_LDFLAGS  += @FLXMLRPC_LIBS@
endif

if !ENABLE_LIBMBEDTLS
fldigi_SOURCES += $(LIBMBEDTLS_SRC)
else
fldigi_CPPFLAGS += @LIBMBEDTLS_CFLAGS@
fldigi_CXXFLAGS += @LIBMBEDTLS_CFLAGS@
fldigi_CFLAGS   += @LIBMBEDTLS_CFLAGS@
fldigi_LDFLAGS  += @LIBMBEDTLS_LIBS@
endif

if ENABLE_HAMLIB
  fldigi_SOURCES += $(HAMLIB_SRC)
endif
if NEED_HAMLIB_LOCATOR
  fldigi_SOURCES += $(LOCATOR_SRC)
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
	if test -f $(srcdir)/../scripts/ftp_kml_files.sh; then \
	    $(mkinstalldirs) $(DESTDIR)/$(bindir); \
        $(INSTALL_SCRIPT) $(srcdir)/../scripts/ftp_kml_files.sh $(DESTDIR)/$(bindir); \
	fi
endif

# TODO: xpm files should probably go to $(datadir)/pixmaps/fldigi instead of $(datadir)/pixmaps

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
	if test -f $(srcdir)/../data/NAVTEX_Stations.csv; then \
	    $(mkinstalldirs) $(DESTDIR)/$(datadir)/fldigi; \
	    $(INSTALL_DATA) $(srcdir)/../data/NAVTEX_Stations.csv $(DESTDIR)/$(pkgdatadir); \
	fi
	if test -f $(srcdir)/../data/nsd_bbsss.txt; then \
	    $(mkinstalldirs) $(DESTDIR)/$(datadir)/fldigi; \
	    $(INSTALL_DATA) $(srcdir)/../data/nsd_bbsss.txt $(DESTDIR)/$(pkgdatadir); \
	fi
	if test -f $(srcdir)/../data/station_table.txt; then \
	    $(mkinstalldirs) $(DESTDIR)/$(datadir)/fldigi; \
	    $(INSTALL_DATA) $(srcdir)/../data/station_table.txt $(DESTDIR)/$(pkgdatadir); \
	fi
	if test -f $(srcdir)/../data/ToR-Stats-SHIP.csv; then \
	    $(mkinstalldirs) $(DESTDIR)/$(datadir)/fldigi; \
	    $(INSTALL_DATA) $(srcdir)/../data/ToR-Stats-SHIP.csv $(DESTDIR)/$(pkgdatadir); \
	fi
	if test -f $(srcdir)/../data/wmo_list.txt; then \
	    $(mkinstalldirs) $(DESTDIR)/$(datadir)/fldigi; \
	    $(INSTALL_DATA) $(srcdir)/../data/wmo_list.txt $(DESTDIR)/$(pkgdatadir); \
	fi
	if test -f $(srcdir)/../kml/styles.kml; then \
	    $(mkinstalldirs) $(DESTDIR)/$(pkgdatadir)/kml; \
	    $(INSTALL_DATA) $(srcdir)/../kml/styles.kml $(DESTDIR)/$(pkgdatadir)/kml; \
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
	rm -f $(DESTDIR)/$(datadir)/pixmaps/fldigi.xpm
	rm -f $(DESTDIR)/$(datadir)/applications/fldigi.desktop
	rm -fr $(DESTDIR)/$(datadir)/fldigi
endif
if WANT_FLARQ
	rm -f $(DESTDIR)/$(datadir)/pixmaps/flarq.xpm
	rm -f $(DESTDIR)/$(datadir)/applications/flarq.desktop
endif

FLDIGI_FL_SRC = \
		debug/debug_dialog.fl \
		dialogs/confdialog.fl \
		dialogs/notifydialog.fl \
		dialogs/record_browse.fl \
		dialogs/rxmon.fl \
		dialogs/testsigs.fl \
		logbook/lgbook.fl \
		logbook/fd_view.fl \
		dxcluster/dx_dialog.fl
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

if USE_NLS
    CLEAN_LOCAL += share
endif

if WANT_FLDIGI
if ENABLE_HAMLIB
hamlib-static: $(fldigi_OBJECTS)
	$(call silent,HLS   ,fldigi$(EXEEXT))sh $(srcdir)/../scripts/mkhamlibstatic.sh fldigi
endif
endif

tmp_srcdir_var=$(srcdir)
TESTS = $(tmp_srcdir_var)/../scripts/tests/config-h.sh $(tmp_srcdir_var)/../scripts/tests/cr.sh

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
	cmedia/cmedia.cxx \
	cmedia/hid.cxx \
	combo/combo.cxx \
	config_script/create_default_script.cxx \
	config_script/run_script.h \
	config_script/run_scripts.cxx \
	config_script/script_parsing.cxx \
	config_script/script_parsing.h \
	contestia/contestia.cxx \
	cw_rtty/cw.cxx \
	cw_rtty/morse.cxx \
	cw_rtty/nanoIO.cxx \
	cw_rtty/ICOMkeying.cxx \
	cw_rtty/KYkeying.cxx  \
	cw_rtty/YAESUkeying.cxx \
	cw_rtty/Nav.cxx \
	cw_rtty/rtty.cxx \
	cw_rtty/view_cw.cxx \
	cw_rtty/view_rtty.cxx \
	cw_rtty/winkeyer.cxx \
	debug/debug.cxx \
	debug/debug_dialog.cxx \
	dialogs/confdialog.cxx \
	dialogs/fl_digi.cxx \
	dialogs/font_browser.cxx \
	dialogs/htmlstrings.cxx \
	dialogs/notifydialog.cxx \
	dialogs/record_browse.cxx \
	dialogs/rxmon.cxx \
	dialogs/testsigs.cxx \
	dialogs/tod_clock.cxx \
	dialogs/Viewer.cxx \
	dominoex/dominoex.cxx \
	dominoex/dominovar.cxx \
	dtmf/dtmf.cxx \
	dxcluster/dxcluster.cxx \
	dxcluster/dx_dialog.cxx \
	feld/feld.cxx \
	feld/feldfonts.cxx \
	fft-monitor/fft-monitor.cxx \
	fft-monitor/spectrum.cxx \
	fft-monitor/spectrum_viewer.cxx \
	fileselector/fileselect.cxx \
	filters/fftfilt.cxx \
	filters/filters.cxx \
	filters/viterbi.cxx \
	fmt/fmt.cxx \
	fmt/fmt_dialog.cxx \
	fsq/fsq.cxx \
	fsq/fsq_monitor.cxx \
	globals/globals.cxx \
	ifkp/ifkp.cxx \
	ifkp/tux.cxx \
	irrxml/CXMLReaderImpl.h \
	irrxml/fast_atof.h \
	irrxml/heapsort.h \
	irrxml/irrArray.h \
	irrxml/irrString.h \
	irrxml/irrTypes.h \
	irrxml/irrXML.cpp \
	irrxml/irrXML.h \
	kml/kmlserver.cxx \
	libtiniconv/tiniconv.c \
	libtiniconv/tiniconv_desc.c \
	logbook/adif_io.cxx \
	logbook/calendar.cxx \
	logbook/contest.cxx \
	logbook/counties.cxx \
	logbook/county_lists.cxx \
	logbook/cty-dat.cxx \
	logbook/date.cxx \
	logbook/fd_logger.cxx \
	logbook/fd_view.cxx \
	logbook/lgbook.cxx \
	logbook/logbook.cxx \
	logbook/logsupport.cxx \
	logbook/lookupcall.cxx \
	logbook/maclogger.cxx \
	logbook/n3fjp_logger.cxx \
	logbook/qrzlib.cxx \
	logbook/qso_db.cxx \
	logbook/table.cxx \
	logbook/textio.cxx \
	logbook/xmlrpc_log.cxx \
	logger/logger.cxx \
	logger/rx_extract.cxx \
	logger/speak.cxx \
	main.cxx \
	mfsk/interleave.cxx \
	mfsk/mfsk.cxx \
	mfsk/mfskvaricode.cxx \
	misc/arq_io.cxx \
	misc/ascii.cxx \
	misc/ax25_decode.cxx \
	misc/charsetdistiller.cxx \
	misc/charsetlist.cxx \
	misc/configuration.cxx \
	misc/coordinate.cxx \
	misc/dxcc.cxx \
	misc/estrings.cxx \
	misc/icons.cxx \
	misc/kiss_io.cxx \
	misc/locator.cxx \
	misc/log.cxx \
	misc/macroedit.cxx \
	misc/macros.cxx \
	misc/misc.cxx \
	misc/newinstall.cxx \
	misc/outputencoder.cxx \
	misc/pixmaps.cxx \
	misc/pixmaps_tango.cxx \
	misc/record_loader.cxx \
	misc/re.cxx \
	misc/stacktrace.cxx \
	misc/status.cxx \
	misc/strutil.cxx \
	misc/threads.cxx \
	misc/timeops.cxx \
	misc/utf8file_io.cxx \
	misc/util.cxx \
	mt63/dsp.cxx \
	mt63/mt63base.cxx \
	mt63/mt63.cxx \
	navtex/navtex.cxx \
	network/ca_cert.cxx \
	network/metar.cxx \
	network/network.cxx \
	network/socket.cxx \
	network/weather.cxx \
	olivia/olivia.cxx \
	psk/pskcoeff.cxx \
	psk/psk.cxx \
	psk/pskeval.cxx \
	psk/pskvaricode.cxx \
	psk/viewpsk.cxx \
	psm/psm.cxx \
	psm/psm.h \
	qrunner/fqueue.h \
	qrunner/qrunner.cxx \
	rigcontrol/FreqControl.cxx \
	rigcontrol/norig.cxx \
	rigcontrol/ptt.cxx \
	rigcontrol/rigio.cxx \
	rigcontrol/rigsupport.cxx \
	rigcontrol/rigxml.cxx \
	rigcontrol/serial.cxx \
	rigcontrol/xmlrpc_rig.cxx \
	rsid/rsid.cxx \
	soundcard/audio_alert.cxx \
	soundcard/bark.cxx \
	soundcard/checkout.cxx \
	soundcard/diesel.cxx \
	soundcard/dinnerbell.cxx \
	soundcard/doesnot.cxx \
	soundcard/play.pa.cxx \
	soundcard/soundconf.cxx \
	soundcard/sound.cxx \
	soundcard/steam_train.cxx \
	soundcard/ttybell.cxx \
	spot/notify.cxx \
	spot/pskrep.cxx \
	spot/spot.cxx \
	ssb/ssb.cxx \
	synop-src/synop.cxx \
	thor/thor.cxx \
	thor/thorvaricode.cxx \
	throb/throb.cxx \
	trx/modem.cxx \
	trx/nullmodem.cxx \
	trx/test_signal.cxx \
	trx/trx.cxx \
	waterfall/colorbox.cxx \
	waterfall/digiscope.cxx \
	waterfall/raster.cxx \
	waterfall/waterfall.cxx \
	wefax/wefax.cxx \
	wefax/wefax_map.cxx \
	wefax/wefax-pic.cxx \
	widgets/flinput2.cxx \
	widgets/flmisc.cxx \
	widgets/flslider2.cxx \
	widgets/Fl_Text_Buffer_mod.cxx \
	widgets/Fl_Text_Display_mod.cxx \
	widgets/Fl_Text_Editor_mod.cxx \
	widgets/FTextRXTX.cxx \
	widgets/FTextView.cxx \
	widgets/Panel.cxx \
	widgets/picture.cxx \
	widgets/plot_xy.cxx \
	widgets/progress.cxx \
	widgets/psk_browser.cxx \
	widgets/pwrmeter.cxx \
	widgets/smeter.cxx \
	widgets/vumeter.cxx \
	wwv/analysis.cxx \
	wwv/wwv.cxx

#	packet/pkt.cxx
#	include/pkt.h

# Sources that are part of the distribution but are not compiled directly
EXTRA_fldigi_SOURCES += \
	blank/blank.cxx \
	blank/blank.h \
	cmedia/hid_lin.cxx \
	cmedia/hid_mac.cxx \
	cmedia/hid_win.cxx \
	dialogs/fl_digi_main.cxx \
	dxcluster/arc-help.cxx \
	dxcluster/CCC_Commands.cxx \
	dxcluster/DXClusterServers.cxx \
	dxcluster/DXSpiderCommandReference.cxx \
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
	fsq/fsq_varicode.cxx \
	include/crc8.h \
	include/Fl_Text_Buffer_mod.H \
	include/Fl_Text_Display_mod.H \
	include/Fl_Text_Editor_mod.H \
	fsq/fsq-pic.cxx \
	fsq/bitmaps.cxx \
	ifkp/ifkp_bitmaps.cxx \
	ifkp/ifkp_varicode.cxx \
	ifkp/ifkp-pic.cxx \
	mfsk/mfsk-pic.cxx \
	mt63/alias_1k.dat \
	mt63/alias_2k.dat \
	mt63/morse.dat \
	mt63/symbol.dat \
	mt63/alias_k5.dat \
	mt63/mt63intl.dat \
	rsid/rsid_defs.cxx \
	thor/thor-pic.cxx \
	trx/tune.cxx \
	dialogs/guide.cxx \
	include/adif_def.h \
	include/adif_io.h \
	include/analysis.h \
	include/arq_io.h \
	include/ascii.h \
	include/audio_alert.h \
	include/ax25_decode.h \
	include/calendar.h \
	include/charsetdistiller.h \
	include/charsetlist.h \
	include/cmedia.h \
	include/colorbox.h \
	include/combo.h \
	include/complex.h \
	include/confdialog.h \
	include/configuration.h \
	include/contest.h \
	include/contestia.h \
	include/coordinate.h \
	include/counties.h \
	include/cw.h \
	include/data_io.h \
	include/date.h \
	include/debug.h \
	include/debug_dialog.h \
	include/digiscope.h \
	include/dominoex.h \
	include/dominovar.h \
	include/dr_mp3.h \
	include/dsp.h \
	include/dtmf.h \
	include/dxcc.h \
	include/dx_cluster.h \
	include/dx_dialog.h \
	include/estrings.h \
	include/fd_logger.h \
	include/fd_view.h \
	include/feld.h \
	include/fftfilt.h \
	include/fft-monitor.h \
	include/field_def.h \
	include/fileselect.h \
	include/filters.h \
	include/fldigi-config.h \
	include/fl_digi.h \
	include/flinput2.h \
	include/flmisc.h \
	include/flslider2.h \
	include/fmt.h \
	include/fmt_dialog.h \
	include/font_browser.h \
	include/fontdef.h \
	include/FreqControl.h \
	include/fsq.h \
	include/FTextRXTX.h \
	include/FTextView.h \
	include/gettext.h \
	include/gfft.h \
	include/globals.h \
	include/hidapi.h \
	include/hid_lin.h \
	include/hid_mac.h \
	include/hid_win.h \
	include/htmlstrings.h \
	include/https.h \
	include/icons.h \
	include/ifkp.h \
	include/interleave.h \
	include/jalocha/pj_cmpx.h \
	include/jalocha/pj_fft.h \
	include/jalocha/pj_fht.h \
	include/jalocha/pj_fifo.h \
	include/jalocha/pj_gray.h \
	include/jalocha/pj_lowpass3.h \
	include/jalocha/pj_mfsk.h \
	include/jalocha/pj_struc.h \
	include/kiss_io.h \
	include/kmlserver.h \
	include/ICOMkeying.h \
	include/KYkeying.h \
	include/YAESUkeying.h \
	include/lgbook.h \
	include/locator.h \
	include/logbook.h \
	include/logger.h \
	include/log.h \
	include/logsupport.h \
	include/lookupcall.h \
	include/maclogger.h \
	include/macroedit.h \
	include/macros.h \
	include/main.h \
	include/mbuffer.h \
	include/metar.h \
	include/mfsk.h \
	include/mfskvaricode.h \
	include/misc.h \
	include/modem.h \
	include/morse.h \
	include/mt63base.h \
	include/mt63.h \
	include/n3fjp_logger.h \
	include/nanoIO.h \
	include/Nav.h \
	include/navtex.h \
	include/navtex.h \
	include/nco.h \
	include/network.h \
	include/newinstall.h \
	include/notifydialog.h \
	include/notify.h \
	include/nullmodem.h \
	include/olivia.h \
	include/outputencoder.h \
	include/Panel.h \
	include/picture.h \
	include/plot_xy.h \
	include/pixmaps.h \
	include/pkg.h \
	include/play.pa.h \
	include/progress.h \
	include/psk_browser.h \
	include/pskcoeff.h \
	include/pskeval.h \
	include/psk.h \
	include/pskrep.h \
	include/pskvaricode.h \
	include/ptt.h \
	include/pwrmeter.h \
	include/qrunner.h \
	include/qrzlib.h \
	include/qso_db.h \
	include/raster.h \
	include/record_browse.h \
	include/record_loader_gui.h \
	include/record_loader.h \
	include/re.h \
	include/rigCAT.h \
	include/rigio.h \
	include/rigsupport.h \
	include/rigxml.h \
	include/ringbuffer.h \
	include/rsid.h \
	include/rtty.h \
	include/rx_extract.h \
	include/rxmon.h \
	include/serial.h \
	include/smeter.h \
	include/vumeter.h \
	include/socket.h \
	include/soundconf.h \
	include/sound.h \
	include/speak.h \
	include/spectrum.h \
	include/spectrum_viewer.h \
	include/spot.h \
	include/ssb.h \
	include/stacktrace.h \
	include/status.h \
	include/status_box.h \
	include/strutil.h \
	include/synop.h \
	include/tabdefs.h \
	include/table.h \
	include/testmodem.h \
	include/test_signal.h \
	include/testsigs.h \
	include/textio.h \
	include/thor.h \
	include/thorvaricode.h \
	include/threads.h \
	include/throb.h \
	include/timeops.h \
	include/trx.h \
	include/utf8file_io.h \
	include/util.h \
	include/view_cw.h \
	include/Viewer.h \
	include/viewpsk.h \
	include/view_rtty.h \
	include/viterbi.h \
	include/waterfall.h \
	include/weather.h \
	include/wefax.h \
	include/wefax_map.h \
	include/wefax-pic.h \
	include/winkeyer.h \
	include/wwv.h \
	include/xmlreader.h \
	libtiniconv/tiniconv.h \
	libtiniconv/tiniconv_int.h \
	libtiniconv/encdec/ascii.h \
	libtiniconv/encdec/big5.h \
	libtiniconv/encdec/ces_big5.h \
	libtiniconv/encdec/ces_gbk.h \
	libtiniconv/encdec/cp1250.h \
	libtiniconv/encdec/cp1251.h \
	libtiniconv/encdec/cp1252.h \
	libtiniconv/encdec/cp1253.h \
	libtiniconv/encdec/cp1254.h \
	libtiniconv/encdec/cp1255.h \
	libtiniconv/encdec/cp1256.h \
	libtiniconv/encdec/cp1257.h \
	libtiniconv/encdec/cp1258.h \
	libtiniconv/encdec/cp866.h \
	libtiniconv/encdec/cp936ext.h \
	libtiniconv/encdec/cp936.h \
	libtiniconv/encdec/euc_cn.h \
	libtiniconv/encdec/flushwc.h \
	libtiniconv/encdec/gb2312.h \
	libtiniconv/encdec/gbkext1.h \
	libtiniconv/encdec/gbkext2.h \
	libtiniconv/encdec/gbkext_inv.h \
	libtiniconv/encdec/gbk.h \
	libtiniconv/encdec/iso2022_jp.h \
	libtiniconv/encdec/iso8859_10.h \
	libtiniconv/encdec/iso8859_11.h \
	libtiniconv/encdec/iso8859_13.h \
	libtiniconv/encdec/iso8859_14.h \
	libtiniconv/encdec/iso8859_15.h \
	libtiniconv/encdec/iso8859_16.h \
	libtiniconv/encdec/iso8859_1.h \
	libtiniconv/encdec/iso8859_2.h \
	libtiniconv/encdec/iso8859_3.h \
	libtiniconv/encdec/iso8859_4.h \
	libtiniconv/encdec/iso8859_5.h \
	libtiniconv/encdec/iso8859_6.h \
	libtiniconv/encdec/iso8859_7.h \
	libtiniconv/encdec/iso8859_8.h \
	libtiniconv/encdec/iso8859_9.h \
	libtiniconv/encdec/jisx0201.h \
	libtiniconv/encdec/jisx0208.h \
	libtiniconv/encdec/koi8_r.h \
	libtiniconv/encdec/koi8_ru.h \
	libtiniconv/encdec/koi8_u.h \
	libtiniconv/encdec/mac_cyrillic.h \
	libtiniconv/encdec/ucs2.h \
	libtiniconv/encdec/utf7.h \
	libtiniconv/encdec/utf8.h \
	libtiniconv/encdec/vietcomb.h

flarq_SOURCES += \
	dialogs/font_browser.cxx \
	flarq-src/arq.cxx \
	flarq-src/arqdialogs.cxx \
	flarq-src/arqhelp.cxx \
	flarq-src/b64.cxx \
	flarq-src/flarq.cxx \
	flarq-src/flarqenv.cxx \
	flarq-src/xml_server.cxx \
	flarq-src/include/arq.h \
	flarq-src/include/arqdialogs.h \
	flarq-src/include/b64.h \
	flarq-src/include/flarq.h \
	flarq-src/include/flarqenv.h \
	flarq-src/include/xml_server.h \
	include/F_Edit.h \
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
	fileselector/fileselect.cxx \
	logbook/table.cxx \
	misc/ascii.cxx \
	misc/stacktrace.cxx \
	misc/threads.cxx \
	misc/timeops.cxx \
	network/socket.cxx \
	widgets/flinput2.cxx \
	widgets/flmisc.cxx \
	widgets/flslider2.cxx \
	misc/icons.cxx \
	misc/pixmaps.cxx \
	misc/pixmaps_tango.cxx \
	misc/re.cxx \
	misc/util.cxx \
	widgets/Fl_Text_Buffer_mod.cxx \
	widgets/Fl_Text_Display_mod.cxx \
	widgets/Fl_Text_Editor_mod.cxx \
	widgets/FTextView.cxx \
	widgets/F_Edit.cxx

# Additional files that we distribute
EXTRA_DIST = \
    $(srcdir)/../build-aux/config.rpath \
	$(srcdir)/../scripts/mkappbundle.sh \
	$(srcdir)/../scripts/mkhamlibstatic.sh \
	$(srcdir)/../scripts/mknsisinst.sh \
	$(srcdir)/../scripts/buildmxe.sh \
	$(srcdir)/../scripts/builddist.sh \
	$(srcdir)/../scripts/tests/cr.sh \
	$(srcdir)/../scripts/tests/config-h.sh \
	$(srcdir)/../data/fldigi-psk.png \
	$(srcdir)/../data/fldigi-rtty.png \
	$(srcdir)/../data/fldigi.xpm \
	$(srcdir)/../data/fldigi.desktop \
	$(srcdir)/../data/NAVTEX_Stations.csv \
	$(srcdir)/../data/nsd_bbsss.txt \
	$(srcdir)/../data/station_table.txt \
	$(srcdir)/../data/ToR-Stats-SHIP.csv \
	$(srcdir)/../data/mac/Info.plist.in \
	$(srcdir)/../data/mac/fldigi.icns \
	$(srcdir)/../data/win32/fldigi.ico \
	$(srcdir)/../data/win32/fldigi.nsi \
	$(srcdir)/../data/flarq.desktop \
	$(srcdir)/../data/flarq.xpm \
	$(srcdir)/../data/mac/flarq.icns \
	$(srcdir)/../data/win32/flarq.ico \
	$(srcdir)/mbedtls/libmbedtls.pc \
	$(srcdir)/mbedtls/LICENSE \
	$(srcdir)/mbedtls/gpl-2.0.txt \
	$(srcdir)/synop-src/synop_tool.cxx \
	$(srcdir)/synop-src/README \
	$(srcdir)/synop-src/Makefile \
	$(FLDIGI_FL_SRC) \
	$(FLARQ_FL_SRC)
