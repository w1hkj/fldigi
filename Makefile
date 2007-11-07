# This is the default makefile used to produce a static executable in the
# Install directory directly under the makefile.  This executable does not 
# contain debug references.
# The executable does not rely on libjpeg libpng or libzlib
# fltk should have been configured as:
# ./configure --enable-threads --enable-xft --enable-localjpeg --enable-localpng --enable-localzlib
#
# Our default target is hamlib. Commands are echoed if V=1.


# This is the default shell for GNU make. We have tested with bash, zsh and dash.
SHELL = /bin/sh

# If we are compiling for IA-32/64, x86-64, SPARC32/64, SH, Alpha or S390
# we probably have TLS support.
USE_TLS ?= 1
ifeq ($(USE_TLS), 0)
    $(warning Compiling without TLS)
endif

# Do we compile with -g? This is not the same as CFG=foo-debug;
# debug targets will override flags and may compile different code.
DEBUG ?= 0
# Do we strip the binary at link time?
STRIP ?= 1

# the prefix for our install target
prefix ?= /usr/local

# pkg-config error template
pkg_config_error = $(1) could not find the location of $(2). Install the $(2) \
                   development headers and libraries, or set $(3) to the \
                   directory containing $(4) and rerun make. Alternatively, set \
                   the $(5)_CFLAGS and $(5)_LIBS variables and rerun make

# compiler and preprocessor options
CXX = g++

INCLUDE_DIRS = src src/include src/irrxml
CPPFLAGS = $(addprefix -I,$(INCLUDE_DIRS)) -DNDEBUG -DUSE_TLS=$(USE_TLS) -DPORTAUDIO

CXXFLAGS = -pipe -Wno-uninitialized -Wno-deprecated -O2 -ffast-math -fno-rtti \
           -fexceptions -finline-functions
ifeq ($(DEBUG), 1)
    CXXFLAGS += -g
endif

FLTK_CFLAGS ?= $(shell fltk-config --cxxflags 2>/dev/null)
PORTAUDIO_CFLAGS ?= $(shell pkg-config --silence-errors --cflags portaudiocpp)
SNDFILE_CFLAGS ?= $(shell pkg-config --silence-errors --cflags sndfile)
SAMPLERATE_CFLAGS ?= $(shell pkg-config --silence-errors --cflags samplerate)
HAMLIB_CFLAGS ?= $(shell pkg-config --silence-errors --cflags hamlib)
# we always need these
CXXFLAGS += $(FLTK_CFLAGS) $(PORTAUDIO_CFLAGS) $(SNDFILE_CFLAGS)


FLTK_LIBS = $(shell fltk-config --ldflags --use-images 2>/dev/null)
ifeq (,$(FLTK_LIBS))
    $(error $(call pkg_config_error,fltk-config,fltk,PATH,fltk-config,FLTK))
endif
FLTK_STATIC_LIBS = $(shell fltk-config --ldstaticflags --use-images)

PORTAUDIO_LIBS ?= $(shell pkg-config --silence-errors --libs portaudiocpp)
ifeq (,$(PORTAUDIO_LIBS))
    $(error $(call pkg_config_error,pkg-config,portaudio,PKG_CONFIG_PATH,portaudiocpp.pc,PORTAUDIO))
endif

SNDFILE_LIBS ?= $(shell pkg-config --silence-errors --libs sndfile)
ifeq (,$(SNDFILE_LIBS))
    $(error $(call pkg_config_error,pkg-config,sndfile,PKG_CONFIG_PATH,sndfile.pc,SNDFILE))
endif

SAMPLERATE_LIBS ?= $(shell pkg-config --silence-errors --libs samplerate)
HAMLIB_LIBS ?= $(shell pkg-config --silence-errors --libs hamlib)

# libraries and flags
DYN_LDFLAGS = $(FLTK_LIBS) $(PORTAUDIO_LIBS) $(SNDFILE_LIBS)

# pkg-config --static does not return anything useful for portaudio and
# sndfile. Therefore, assume that these libs have been configured with
# --enable-shared=no --enable-static=yes so that the usual -l switches
# will link statically. Further, assume that no other libraries are
# required at link time; this means that PA has been configured with:
#     --with-alsa=no --with-jack=no
# and sndfile with:
#     --disable-flac
# When linking PA statically we also need -lrt for clock_gettime().
STATIC_LDFLAGS = $(FLTK_STATIC_LIBS) $(PORTAUDIO_LIBS) $(SNDFILE_LIBS) -lrt


# our source files
SRC_DIR = src
SRC = \
	$(SRC_DIR)/combo/combo.cxx \
	$(SRC_DIR)/cw_rtty/cw.cxx \
	$(SRC_DIR)/cw_rtty/rtty.cxx \
	$(SRC_DIR)/cw_rtty/morse.cxx \
	$(SRC_DIR)/dialogs/Config.cxx \
	$(SRC_DIR)/dialogs/fl_digi.cxx \
	$(SRC_DIR)/dialogs/font_browser.cxx \
	$(SRC_DIR)/dominoex/dominoex.cxx \
	$(SRC_DIR)/dominoex/dominovar.cxx \
	$(SRC_DIR)/feld/feld.cxx \
	$(SRC_DIR)/feld/feldfonts.cxx \
	$(SRC_DIR)/fft/fft.cxx \
	$(SRC_DIR)/fileselector/File_Selector.cxx \
	$(SRC_DIR)/fileselector/File_Selector2.cxx \
	$(SRC_DIR)/fileselector/file_dir.cxx \
	$(SRC_DIR)/filters/fftfilt.cxx \
	$(SRC_DIR)/filters/filters.cxx \
	$(SRC_DIR)/filters/viterbi.cxx \
	$(SRC_DIR)/globals/globals.cxx \
	$(SRC_DIR)/irrxml/irrXML.cpp \
	$(SRC_DIR)/logger/logger.cxx \
	$(SRC_DIR)/main.cxx \
	$(SRC_DIR)/misc/ascii.cxx \
	$(SRC_DIR)/misc/configuration.cxx \
	$(SRC_DIR)/misc/log.cxx \
	$(SRC_DIR)/misc/macros.cxx \
	$(SRC_DIR)/misc/macroedit.cxx \
	$(SRC_DIR)/misc/misc.cxx \
	$(SRC_DIR)/misc/newinstall.cxx \
	$(SRC_DIR)/misc/pskmail.cxx \
	$(SRC_DIR)/misc/qrzcall.cxx \
	$(SRC_DIR)/misc/qrzlib.cxx \
	$(SRC_DIR)/misc/status.cxx \
	$(SRC_DIR)/misc/threads.cxx \
	$(SRC_DIR)/mfsk/mfsk.cxx \
	$(SRC_DIR)/mfsk/interleave.cxx \
	$(SRC_DIR)/mfsk/mfskvaricode.cxx \
	$(SRC_DIR)/olivia/olivia.cxx \
	$(SRC_DIR)/psk/psk.cxx \
	$(SRC_DIR)/psk/pskvaricode.cxx \
	$(SRC_DIR)/psk/pskcoeff.cxx \
	$(SRC_DIR)/rigcontrol/ptt.cxx \
	$(SRC_DIR)/rigcontrol/FreqControl.cxx \
	$(SRC_DIR)/rigcontrol/rigdialog.cxx \
	$(SRC_DIR)/rigcontrol/rigsupport.cxx \
	$(SRC_DIR)/rigcontrol/rigMEM.cxx \
	$(SRC_DIR)/rigcontrol/rigio.cxx \
	$(SRC_DIR)/rigcontrol/rigxml.cxx \
	$(SRC_DIR)/rigcontrol/serial.cxx \
	$(SRC_DIR)/soundcard/mixer.cxx \
	$(SRC_DIR)/soundcard/sound.cxx \
	$(SRC_DIR)/throb/throb.cxx \
	$(SRC_DIR)/trx/modem.cxx \
	$(SRC_DIR)/trx/trx.cxx \
	$(SRC_DIR)/waterfall/colorbox.cxx \
	$(SRC_DIR)/waterfall/raster.cxx \
	$(SRC_DIR)/waterfall/waterfall.cxx \
	$(SRC_DIR)/waterfall/digiscope.cxx \
	$(SRC_DIR)/widgets/picture.cxx \
	$(SRC_DIR)/widgets/TextView.cxx \
	$(SRC_DIR)/misc/flstring.c \
	$(SRC_DIR)/widgets/Fl_Text_Display_mod.cxx \
	$(SRC_DIR)/widgets/Fl_Text_Editor_mod.cxx \
	$(SRC_DIR)/widgets/FTextView.cxx \
	$(SRC_DIR)/wwv/analysis.cxx \
	$(SRC_DIR)/wwv/wwv.cxx \
	$(SRC_DIR)/qrunner/ringbuffer.c \
	$(SRC_DIR)/qrunner/qrunner.cxx \
	$(SRC_DIR)/misc/timeops.cxx

# We do not always compile these. CFG targets that link with hamlib
# will append HAMLIB_SRC to SRC.
HAMLIB_SRC = \
	$(SRC_DIR)/rigcontrol/hamlib.cxx \
	$(SRC_DIR)/rigcontrol/rigclass.cxx

# We compile these if there is no system-wide libsamplerate, or if
# CFG contains the string "w1hkj-static"
SAMPLERATE_SRC = \
	$(SRC_DIR)/samplerate/samplerate.c \
	$(SRC_DIR)/samplerate/src_linear.c \
	$(SRC_DIR)/samplerate/src_sinc.c \
	$(SRC_DIR)/samplerate/src_zoh.c

#################### begin cfg
CFG ?= hamlib

# CFG is *static*
ifneq (,$(findstring static,$(CFG)))
    LDFLAGS = $(STATIC_LDFLAGS)
else
    LDFLAGS = $(DYN_LDFLAGS)
endif

# CFG is *w1hkj-dist*
ifneq (,$(findstring w1hkj-dist,$(CFG)))
    LDFLAGS = $(FLTK_STATIC_LIBS) \
              /usr/local/lib/libportaudiocpp.a /usr/local/lib/libportaudio.a \
              /usr/local/lib/libsndfile.a -lfltk_jpeg -lfltk_png -lfltk_z
    SAMPLERATE_CFLAGS =
    SAMPLERATE_LIBS =
endif

# CFG is *hamlib*
ifneq (,$(findstring hamlib,$(CFG)))
    ifeq (,$(HAMLIB_LIBS))
            $(error $(call pkg_config_error,pkg-config,hamlib,PKG_CONFIG_PATH,hamlib.pc,HAMLIB))
    endif
    CXXFLAGS += $(HAMLIB_CFLAGS)
    LDFLAGS += $(HAMLIB_LIBS)
    SRC += $(HAMLIB_SRC)
else # nhl
    CPPFLAGS += -DNOHAMLIB
endif

# CFG is *debug*
ifneq (,$(findstring debug,$(CFG)))
    CPPFLAGS += -UNDEBUG
    CXXFLAGS += -O0 -ggdb3 -Wall
    SRC += $(SRC_DIR)/misc/stacktrace.cxx
    override STRIP = 0
endif

ifeq (,$(or $(SAMPLERATE_CFLAGS),$(SAMPLERATE_LIBS)))
    SRC += $(SAMPLERATE_SRC)
    CPPFLAGS += -Isrc/samplerate
else
    CXXFLAGS += $(SAMPLERATE_CFLAGS)
    LDFLAGS += $(SAMPLERATE_LIBS)
endif

ifeq ($(STRIP), 1)
    LDFLAGS += -s
endif
#################### end cfg


# binaries
DEP_DIR = Depends
OBJ_DIR = Objects
BIN_DIR = Install
BINARY  = $(BIN_DIR)/fldigi
VERSIONS = $(SRC_DIR)/include/versions.h


# our object files
OBJS = $(addprefix $(OBJ_DIR)/,$(addsuffix .o,$(basename $(notdir $(SRC)))))


# some "canned commands" variables
define preproc_cmd
    set -e; mkdir -p $(dir $@); \
    $(CXX) $(CPPFLAGS) -MM "$(subst $(DEP_DIR)/,$(SRC_DIR)/,$(subst .deps,,$@))" \
    -MT "$(addprefix $(OBJ_DIR)/,$(addsuffix .o,$(notdir $(basename $(basename $@)))))" \
    -MT "$@" -MF "$@" -MG
endef

define compile_cmd
    $(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
endef

define link_cmd
    $(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
endef

ifneq ($(V), 1) # be quiet
    define preproc_source
        @echo Generating $@
        @$(preproc_cmd) || { r=$$?; /bin/echo -e "Failed command was:\n$(preproc_cmd)" >&2; exit $$r; }
    endef

    define compile_source
        @echo Compiling $<
        @$(compile_cmd) || { r=$$?; /bin/echo -e "Failed command was:\n$(compile_cmd)" >&2; exit $$r; }
    endef

    define link_objects
        @echo Linking $@
        @$(link_cmd) || { r=$$?; /bin/echo -e "Failed command was:\n$(link_cmd)" >&2; exit $$r; }
    endef
else # be verbose
    define preproc_source
        $(preproc_cmd)
    endef

    define compile_source
        $(compile_cmd)
    endef

    define link_objects
        $(link_cmd)
    endef
endif # ($(V), 1)


# targets

.PHONY: all print_header directories clean
.EXPORT_ALL_VARIABLES: $(VERSIONS)

all: print_header directories $(VERSIONS) $(BINARY)

$(VERSIONS): $(VERSIONS).in
	@echo Generating $@
	@sh scripts/mkversions $< $@ || touch $@
versions.h: $(VERSIONS)

$(BINARY): $(OBJS)
	$(link_objects)

%.deps:
	$(preproc_source)

# We will generate the .deps in $(DEPS) below using the %.deps rule
# unless our target is ``clean'' or is included in the egrep args.
DEPS = $(subst src/,$(DEP_DIR)/,$(patsubst %,%.deps,$(SRC)))

#To ignore multiple targets use this instead:
#ifeq ($(shell echo $(MAKECMDGOALS) | egrep 'clean|anothertarget|onemore'),)
ifneq ($(MAKECMDGOALS),clean)
    include $(DEPS)
endif

# this target must appear after the deps have been included
%.o:
	$(compile_source)

print_header:
	@echo --- Building fldigi
	@echo ---    executable in directory $(BIN_DIR)
	@echo ---    object files in $(OBJ_DIR)

directories:
	@mkdir -p $(BIN_DIR) $(OBJ_DIR)

clean:
	@echo Deleting intermediate files for fldigi
	@rm -rf $(DEP_DIR) $(OBJ_DIR) $(BINARY) $(VERSIONS)

install: $(BINARY)
	@echo Installing $(BINARY) in $(prefix)/bin
	@install -m 755 $(BINARY) $(prefix)/bin

uninstall:
	@echo Deleting $(subst $(BIN_DIR),$(prefix)/bin,$(BINARY))
	@rm -f $(subst $(BIN_DIR),$(prefix)/bin,$(BINARY))
