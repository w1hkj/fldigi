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


# argument handling

CTARG = hamlib
ifneq (,$(findstring hamlib-debug, $(CFG)))
	override CTARG = hamlib-debug
endif
ifneq (,$(findstring hamlib-static, $(CFG)))
	override CTARG = hamlib-static
endif
ifneq (,$(findstring nhl, $(CFG)))
	override CTARG = nhl
endif
ifneq (,$(findstring nhl-debug, $(CFG)))
	override CTARG = nhl-debug
endif
ifneq (,$(findstring nhl-static, $(CFG)))
	override CTARG = nhl-static
endif
ifneq (,$(findstring emcomm, $(CFG)))
    override CTARG = emcomm
endif
ifneq (,$(findstring nhl-emcomm, $(CFG)))
    override CTARG = nhl-emcomm
endif


# compiler and preprocessor options
CXX = g++

INCLUDE_DIRS = src src/include src/irrxml
CPPFLAGS = $(addprefix -I,$(INCLUDE_DIRS)) -DNDEBUG -DUSE_TLS=$(USE_TLS)

#CXXFLAGS = -pipe $(shell fltk-config --cxxflags) -Wall -Wno-deprecated -O2 -ffast-math -fno-rtti -fexceptions
CXXFLAGS = -pipe $(shell fltk-config --cxxflags) -Wno-uninitialized -Wno-deprecated \
           -O2 -ffast-math -fno-rtti -fexceptions -finline-functions
ifeq ($(DEBUG), 1)
    CXXFLAGS += -g
endif

# libraries and flags
HAMLIBS = -lhamlib
IMGLIBS = -lfltk_jpeg -lfltk_png -lfltk_z
#LIB_DIRS = -L/usr/local/lib

DYN_LDFLAGS = $(shell fltk-config --ldflags --use-images)
STATIC_LDFLAGS = $(shell fltk-config --ldstaticflags --use-images)

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
	$(SRC_DIR)/ider/id.cxx \
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
	$(SRC_DIR)/samplerate/samplerate.c \
	$(SRC_DIR)/samplerate/src_linear.c \
	$(SRC_DIR)/samplerate/src_sinc.c \
	$(SRC_DIR)/samplerate/src_zoh.c \
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

# binaries
DEP_DIR = Depends
OBJ_DIR = Objects
BIN_DIR = Install
BINARY  = $(BIN_DIR)/fldigi

#################### begin cfg
ifeq ($(CTARG),hamlib)
    CPPFLAGS += -DPORTAUDIO
    LDFLAGS = $(DYN_LDFLAGS) -lportaudiocpp -lportaudio -lsndfile $(HAMLIBS)
    SRC += $(HAMLIB_SRC)
endif

ifeq ($(CTARG),hamlib-static)
    CPPFLAGS += -DPORTAUDIO
    LDFLAGS = $(STATIC_LDFLAGS) /usr/local/lib/libportaudiocpp.a \
              /usr/local/lib/libportaudio.a /usr/local/lib/libsndfile.a \
              $(HAMLIBS) $(IMGLIBS)
    SRC += $(HAMLIB_SRC)
endif

ifeq ($(CTARG),emcomm)
    CPPFLAGS += -DPORTAUDIO -DEMCOMM
    LDFLAGS = $(STATIC_LDFLAGS) /usr/local/lib/libportaudiocpp.a \
              /usr/local/lib/libportaudio.a /usr/local/lib/libsndfile.a \
              $(HAMLIBS) $(IMGLIBS)
    SRC += $(HAMLIB_SRC)
endif

ifeq ($(CTARG),hamlib-debug)
    CPPFLAGS += -DPORTAUDIO -UNDEBUG
    CXXFLAGS += -O0 -ggdb3 -Wall
    LDFLAGS = $(DYN_LDFLAGS) -lportaudiocpp -lportaudio -lsndfile $(HAMLIBS)
    SRC += $(HAMLIB_SRC)
    SRC += $(SRC_DIR)/misc/stacktrace.cxx
    override STRIP = 0
endif

ifeq ($(CTARG),nhl)
    CPPFLAGS += -DNOHAMLIB -DPORTAUDIO
    LDFLAGS = $(DYN_LDFLAGS) -lportaudiocpp -lportaudio -lsndfile
endif

ifeq ($(CTARG),nhl-static)
    CPPFLAGS += -DNOHAMLIB -DPORTAUDIO
    LDFLAGS = $(STATIC_LDFLAGS) /usr/local/lib/libportaudiocpp.a \
              /usr/local/lib/libportaudio.a /usr/local/lib/libsndfile.a \
              $(IMGLIBS)
endif

ifeq ($(CTARG),nhl-debug)
    CPPFLAGS += -DNOHAMLIB -DPORTAUDIO -UNDEBUG
    CXXFLAGS += -O0 -ggdb3 -Wall
    LDFLAGS = $(DYN_LDFLAGS) -lportaudiocpp -lportaudio -lsndfile
    SRC += $(SRC_DIR)/misc/stacktrace.cxx
    override STRIP = 0
endif

ifeq ($(CTARG),nhl-emcomm)
    CPPFLAGS += -DNOHAMLIB -DPORTAUDIO -DEMCOMM
    LDFLAGS = $(STATIC_LDFLAGS) /usr/local/lib/libportaudiocpp.a \
              /usr/local/lib/libportaudio.a /usr/local/lib/libsndfile.a \
              $(IMGLIBS)
endif
#################### end cfg

ifeq ($(STRIP), 1)
    LDFLAGS += -s
endif

# our object files
OBJS = $(addprefix $(OBJ_DIR)/,$(addsuffix .o,$(basename $(notdir $(SRC)))))


# some "canned commands" variables
define preproc_cmd
    set -e; mkdir -p $(dir $@); \
    $(CXX) $(CPPFLAGS) -MM "$(subst $(DEP_DIR)/,$(SRC_DIR)/,$(subst .deps,,$@))" \
    -MT "$(addprefix $(OBJ_DIR)/,$(addsuffix .o,$(notdir $(basename $(basename $@)))))" \
    -MT "$@" -MF "$@"
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

all: print_header directories $(BINARY)

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
	@rm -rf $(DEP_DIR) $(OBJ_DIR) $(BINARY)
