ACLOCAL_AMFLAGS = -I m4
# AC_CONFIG_MACRO_DIR([m4])

SUBDIRS = src

if HAVE_FLUID
flgen:
	(cd src && $(MAKE) $(AM_MAKEFLAGS) $@)
endif

if DARWIN
appbundle:
	(cd src && $(MAKE) $(AM_MAKEFLAGS) $@)
endif

if ENABLE_HAMLIB
hamlib-static:
	(cd src && $(MAKE) $(AM_MAKEFLAGS) $@)
endif
