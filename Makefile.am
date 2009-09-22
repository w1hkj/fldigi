ACLOCAL_AMFLAGS = -I m4 -I m4/intl
# AC_CONFIG_MACRO_DIR([m4])

SUBDIRS = po doc src

if HAVE_FLUID
flgen:
	@(cd src && $(MAKE) $(AM_MAKEFLAGS) $@)
endif

if DARWIN
appbundle:
	@(cd src && $(MAKE) $(AM_MAKEFLAGS) $@)
endif

if HAVE_NSIS
nsisinst:
	@(cd src && $(MAKE) $(AM_MAKEFLAGS) $@)
endif

if WANT_FLDIGI
if ENABLE_HAMLIB
hamlib-static:
	@(cd src && $(MAKE) $(AM_MAKEFLAGS) $@)
endif
endif

EXTRA_DIST = build-aux/config.rpath
