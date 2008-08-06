ACLOCAL_AMFLAGS = -I m4
# AC_CONFIG_MACRO_DIR([m4])

SUBDIRS = src

flgen:
	(cd src && $(MAKE) $(AM_MAKEFLAGS) $@)

if DARWIN
appbundle:
	(cd src && $(MAKE) $(AM_MAKEFLAGS) $@)
endif
