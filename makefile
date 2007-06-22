# This is the default makefile used to produce a static lib executable in the
# same directory as the makefile.  This executable does not contain debug references
# The executable does not rely on libjpeg libpng or libzlib
# fltk should have been configured as:
# ./configure --enable-threads --enable-xft --enable-localjpeg --enable-localpng --enable-localzlib
#

CTARG = hamlib
ifneq (,$(findstring hamlib-debug, $(CFG)))
	override CTARG = hamlib-debug
endif
ifneq (,$(findstring hamlib-local, $(CFG)))
	override CTARG = hamlib-local
endif
ifneq (,$(findstring nhl, $(CFG)))
  	override CTARG = nhl
endif
ifneq (,$(findstring nhl-debug, $(CFG)))
	override CTARG = nhl-debug
endif
ifneq (,$(findstring nhl-local, $(CFG)))
	override CTARG = nhl-local
endif

#PROJECT = fldigi
CC = g++
#"/usr/bin/g++"

OBJ_DIR = ./Objects
OUTPUT_DIR = ./Install
TARGET = fldigi
LIB_DIRS = -L"/usr/local/lib" 
C_INCLUDE_DIRS = -I"src" -I"src/include"
CCFLAGS = `fltk-config --cxxflags` -Wno-deprecated -ffast-math -Wall -O2 -fno-rtti -fexceptions 

HAMLIBS = -lhamlib
IMGLIBS = -lfltk_jpeg -lfltk_png -lfltk_z
STATIC_LDFLAGS = -pipe `fltk-config --ldstaticflags --use-images`
DYN_LDFLAGS = -pipe `fltk-config --ldflags --use-images`


SRC_OBJS = \
  $(OBJ_DIR)/fft.o	\
  $(OBJ_DIR)/waterfall.o	\
  $(OBJ_DIR)/sound.o	\
  $(OBJ_DIR)/fl_digi.o	\
  $(OBJ_DIR)/threads.o	\
  $(OBJ_DIR)/viterbi.o	\
  $(OBJ_DIR)/trx.o	\
  $(OBJ_DIR)/psk.o	\
  $(OBJ_DIR)/pskvaricode.o	\
  $(OBJ_DIR)/pskcoeff.o	\
  $(OBJ_DIR)/mfsk.o	\
  $(OBJ_DIR)/interleave.o	\
  $(OBJ_DIR)/mfskvaricode.o	\
  $(OBJ_DIR)/ascii.o	\
  $(OBJ_DIR)/globals.o	\
  $(OBJ_DIR)/modem.o	\
  $(OBJ_DIR)/misc.o	\
  $(OBJ_DIR)/fftfilt.o	\
  $(OBJ_DIR)/Config.o	\
  $(OBJ_DIR)/configuration.o	\
  $(OBJ_DIR)/filters.o	\
  $(OBJ_DIR)/TextView.o	\
  $(OBJ_DIR)/font_browser.o	\
  $(OBJ_DIR)/macros.o	\
  $(OBJ_DIR)/macroedit.o \
  $(OBJ_DIR)/main.o	\
  $(OBJ_DIR)/ptt.o	\
  $(OBJ_DIR)/digiscope.o	\
  $(OBJ_DIR)/logger.o	\
  $(OBJ_DIR)/olivia.o	\
  $(OBJ_DIR)/dominoex.o	\
  $(OBJ_DIR)/dominovar.o	\
  $(OBJ_DIR)/wwv.o	\
  $(OBJ_DIR)/log.o	\
  $(OBJ_DIR)/pskmail.o	\
  $(OBJ_DIR)/cw.o	\
  $(OBJ_DIR)/rtty.o	\
  $(OBJ_DIR)/morse.o	\
  $(OBJ_DIR)/feld.o	\
  $(OBJ_DIR)/raster.o	\
  $(OBJ_DIR)/feldfonts.o	\
  $(OBJ_DIR)/analysis.o	\
  $(OBJ_DIR)/throb.o	\
  $(OBJ_DIR)/id.o	\
  $(OBJ_DIR)/picture.o	\
  $(OBJ_DIR)/samplerate.o	\
  $(OBJ_DIR)/src_linear.o	\
  $(OBJ_DIR)/src_sinc.o	\
  $(OBJ_DIR)/src_zoh.o	\
  $(OBJ_DIR)/combo.o	\
  $(OBJ_DIR)/File_Selector.o	\
  $(OBJ_DIR)/File_Selector2.o	\
  $(OBJ_DIR)/file_dir.o \
  $(OBJ_DIR)/status.o \
  $(OBJ_DIR)/qrzcall.o \
  $(OBJ_DIR)/qrzlib.o \
  $(OBJ_DIR)/FreqControl.o	\
  $(OBJ_DIR)/rigdialog.o	\
  $(OBJ_DIR)/rigsupport.o	\
  $(OBJ_DIR)/rigMEM.o	\
  $(OBJ_DIR)/rigio.o	\
  $(OBJ_DIR)/rigxml.o	\
  $(OBJ_DIR)/serial.o\
  $(OBJ_DIR)/newinstall.o \
  $(OBJ_DIR)/colorbox.o\
  $(OBJ_DIR)/mixer.o

HAMLIB_OBJS = \
  $(OBJ_DIR)/hamlib.o	\
  $(OBJ_DIR)/rigclass.o

define compile_source
@echo Compiling $<
@$(CC) $(C_INCLUDE_DIRS) $(CFLAGS) -c "$<" -o "$@"
endef

.PHONY: print_header directories

ifeq ($(CTARG),hamlib)
CFLAGS = $(CCFLAGS) -DPORTAUDIO
LDFLAGS = $(DYN_LDFLAGS) -lportaudiocpp
$(TARGET): print_header directories $(SRC_OBJS) $(HAMLIB_OBJS)
	$(CC) -s -o $(OUTPUT_DIR)/$(TARGET) $(SRC_OBJS) $(HAMLIB_OBJS) $(LDFLAGS) $(HAMLIBS)
endif

ifeq ($(CTARG),hamlib-local)
CFLAGS = $(CCFLAGS) -DPORTAUDIO
LDFLAGS = $(STATIC_LDFLAGS) -lportaudiocpp
$(TARGET): print_header directories $(SRC_OBJS) $(HAMLIB_OBJS)
	$(CC) -s -o $(OUTPUT_DIR)/$(TARGET) $(SRC_OBJS) $(HAMLIB_OBJS) $(LDFLAGS) $(HAMLIBS) $(IMGLIBS)
endif

ifeq ($(CTARG),hamlib-debug)
CFLAGS = $(CCFLAGS) -g -DPORTAUDIO
LDFLAGS = $(DYN_LDFLAGS) -lportaudiocpp
OBJS = $(SRC_OBJS) $(HAMLIB_OBJS)
$(TARGET): print_header directories $(SRC_OBJS) $(HAMLIB_OBJS)
	$(CC) -o $(OUTPUT_DIR)/$(TARGET) $(OBJS) $(LDFLAGS) $(HAMLIBS)
endif

ifeq ($(CTARG),nhl)
CFLAGS = $(CCFLAGS) -DNOHAMLIB -DPORTAUDIO
LDFLAGS = $(DYN_LDFLAGS) -lportaudiocpp
$(TARGET): print_header directories $(SRC_OBJS)
	$(CC) -s -o $(OUTPUT_DIR)/$(TARGET) $(SRC_OBJS) $(LDFLAGS)
endif

ifeq ($(CTARG),nhl-local)
CFLAGS = $(CCFLAGS) -DNOHAMLIB -DPORTAUDIO
LDFLAGS = $(STATIC_LDFLAGS) /usr/local/lib/libportaudiocpp.a /usr/local/lib/libportaudio.a
$(TARGET): print_header directories $(SRC_OBJS)
	$(CC) -s -o $(OUTPUT_DIR)/$(TARGET) $(SRC_OBJS) $(LDFLAGS) $(IMGLIBS) 
endif

ifeq ($(CTARG),nhl-debug)
CFLAGS = $(CCFLAGS) -DNOHAMLIB -DPORTAUDIO -g
LDFLAGS = $(DYN_LDFLAGS) -lportaudiocpp
$(TARGET): print_header directories $(SRC_OBJS)
	$(CC) -o $(OUTPUT_DIR)/$(TARGET) $(SRC_OBJS) $(LDFLAGS)
endif

clean:
	@echo Deleting intermediate files for fldigi
	-@rm -rf $(OBJ_DIR)
	-@rm -rf $(OUTPUT_DIR)/$(TARGET)
	-@rm -rf *~

print_header:
	@echo --- Building fldigi
	@echo ---    executable in directory $(OUTPUT_DIR)
	@echo ---    object files in $(OBJ_DIR)

directories:
	-@if [ ! -d $(OUTPUT_DIR) ]; then mkdir $(OUTPUT_DIR); fi
	-@if [ ! -d $(OBJ_DIR) ]; then mkdir $(OBJ_DIR); fi

$(OBJ_DIR)/fft.o: src/fft/fft.cxx
	$(compile_source)

$(OBJ_DIR)/waterfall.o: src/waterfall/waterfall.cxx
	$(compile_source)

$(OBJ_DIR)/sound.o: src/soundcard/sound.cxx
	$(compile_source)

$(OBJ_DIR)/fl_digi.o: src/dialogs/fl_digi.cxx
	$(compile_source)

$(OBJ_DIR)/threads.o: src/misc/threads.cxx
	$(compile_source)

$(OBJ_DIR)/viterbi.o: src/filters/viterbi.cxx
	$(compile_source)

$(OBJ_DIR)/trx.o: src/trx/trx.cxx	\
src/trx/tune.cxx
	$(compile_source)

$(OBJ_DIR)/psk.o: src/psk/psk.cxx
	$(compile_source)

$(OBJ_DIR)/pskvaricode.o: src/psk/pskvaricode.cxx
	$(compile_source)

$(OBJ_DIR)/pskcoeff.o: src/psk/pskcoeff.cxx
	$(compile_source)

$(OBJ_DIR)/mfsk.o: src/mfsk/mfsk.cxx
	$(compile_source)

$(OBJ_DIR)/interleave.o: src/mfsk/interleave.cxx
	$(compile_source)

$(OBJ_DIR)/mfskvaricode.o: src/mfsk/mfskvaricode.cxx
	$(compile_source)

$(OBJ_DIR)/ascii.o: src/misc/ascii.cxx
	$(compile_source)

$(OBJ_DIR)/globals.o: src/globals/globals.cxx
	$(compile_source)

$(OBJ_DIR)/modem.o: src/trx/modem.cxx
	$(compile_source)

$(OBJ_DIR)/misc.o: src/misc/misc.cxx
	$(compile_source)

$(OBJ_DIR)/fftfilt.o: src/filters/fftfilt.cxx
	$(compile_source)

$(OBJ_DIR)/Config.o: src/dialogs/Config.cxx
	$(compile_source)

$(OBJ_DIR)/configuration.o: src/misc/configuration.cxx
	$(compile_source)

$(OBJ_DIR)/filters.o: src/filters/filters.cxx
	$(compile_source)

$(OBJ_DIR)/TextView.o: src/dialogs/TextView.cxx
	$(compile_source)

$(OBJ_DIR)/font_browser.o: src/dialogs/font_browser.cxx
	$(compile_source)

$(OBJ_DIR)/macros.o: src/misc/macros.cxx
	$(compile_source)

$(OBJ_DIR)/main.o: src/main.cxx
	$(compile_source)

$(OBJ_DIR)/ptt.o: src/rigcontrol/ptt.cxx
	$(compile_source)

$(OBJ_DIR)/digiscope.o: src/waterfall/digiscope.cxx
	$(compile_source)

$(OBJ_DIR)/logger.o: src/logger/logger.cxx
	$(compile_source)

$(OBJ_DIR)/rigclass.o: src/rigcontrol/rigclass.cxx
	$(compile_source)

$(OBJ_DIR)/hamlib.o: src/rigcontrol/hamlib.cxx
	$(compile_source)

$(OBJ_DIR)/olivia.o: src/olivia/olivia.cxx
	$(compile_source)

$(OBJ_DIR)/dominoex.o: src/dominoex/dominoex.cxx
	$(compile_source)

$(OBJ_DIR)/dominovar.o: src/dominoex/dominovar.cxx
	$(compile_source)

$(OBJ_DIR)/wwv.o: src/wwv/wwv.cxx
	$(compile_source)

$(OBJ_DIR)/log.o: src/misc/log.cxx
	$(compile_source)

$(OBJ_DIR)/pskmail.o: src/misc/pskmail.cxx
	$(compile_source)

$(OBJ_DIR)/cw.o: src/cw_rtty/cw.cxx
	$(compile_source)

$(OBJ_DIR)/rtty.o: src/cw_rtty/rtty.cxx
	$(compile_source)

$(OBJ_DIR)/morse.o: src/cw_rtty/morse.cxx
	$(compile_source)

$(OBJ_DIR)/feld.o: src/feld/feld.cxx
	$(compile_source)

$(OBJ_DIR)/raster.o: src/waterfall/raster.cxx
	$(compile_source)

$(OBJ_DIR)/feldfonts.o: src/feld/feldfonts.cxx	\
src/feld/Feld7x7-14.cxx	\
src/feld/Feld7x7n-14.cxx	\
src/feld/FeldDx-14.cxx	\
src/feld/FeldFat-14.cxx	\
src/feld/FeldHell-12.cxx	\
src/feld/FeldLittle-12.cxx	\
src/feld/FeldLo8-14.cxx	\
src/feld/FeldLow-14.cxx	\
src/feld/FeldModern-14.cxx	\
src/feld/FeldModern8-14.cxx	\
src/feld/FeldNarr-14.cxx	\
src/feld/FeldReal-14.cxx	\
src/feld/FeldStyl-14.cxx	\
src/feld/FeldVert-14.cxx	\
src/feld/FeldWide-14.cxx
	$(compile_source)

$(OBJ_DIR)/analysis.o: src/wwv/analysis.cxx
	$(compile_source)

$(OBJ_DIR)/throb.o: src/throb/throb.cxx
	$(compile_source)

$(OBJ_DIR)/id.o: src/ider/id.cxx
	$(compile_source)

$(OBJ_DIR)/picture.o: src/widgets/picture.cxx
	$(compile_source)

$(OBJ_DIR)/samplerate.o: src/samplerate/samplerate.c	\
src/samplerate/srconfig.h	\
src/samplerate/samplerate.h	\
src/samplerate/common.h	\
src/samplerate/float_cast.h
	$(compile_source)

$(OBJ_DIR)/src_linear.o: src/samplerate/src_linear.c	\
src/samplerate/srconfig.h	\
src/samplerate/float_cast.h	\
src/samplerate/common.h	\
src/samplerate/samplerate.h
	$(compile_source)

$(OBJ_DIR)/src_sinc.o: src/samplerate/src_sinc.c	\
src/samplerate/srconfig.h	\
src/samplerate/float_cast.h	\
src/samplerate/common.h	\
src/samplerate/samplerate.h	\
src/samplerate/fastest_coeffs.h
	$(compile_source)

$(OBJ_DIR)/src_zoh.o: src/samplerate/src_zoh.c	\
src/samplerate/srconfig.h	\
src/samplerate/float_cast.h	\
src/samplerate/common.h	\
src/samplerate/samplerate.h
	$(compile_source)

$(OBJ_DIR)/combo.o: src/combo/combo.cxx
	$(compile_source)

$(OBJ_DIR)/File_Selector.o: src/fileselector/File_Selector.cxx
	$(compile_source)

$(OBJ_DIR)/File_Selector2.o: src/fileselector/File_Selector2.cxx
	$(compile_source)

$(OBJ_DIR)/file_dir.o: src/fileselector/file_dir.cxx
	$(compile_source)

$(OBJ_DIR)/status.o: src/misc/status.cxx
	$(compile_source)

$(OBJ_DIR)/qrzcall.o: src/misc/qrzcall.cxx
	$(compile_source)

$(OBJ_DIR)/qrzlib.o: src/misc/qrzlib.cxx
	$(compile_source)

$(OBJ_DIR)/FreqControl.o: src/rigcontrol/FreqControl.cxx
	$(compile_source)

$(OBJ_DIR)/rigdialog.o: src/rigcontrol/rigdialog.cxx
	$(compile_source)

$(OBJ_DIR)/rigsupport.o: src/rigcontrol/rigsupport.cxx
	$(compile_source)

$(OBJ_DIR)/rigMEM.o: src/rigcontrol/rigMEM.cxx
	$(compile_source)

$(OBJ_DIR)/rigio.o: src/rigcontrol/rigio.cxx
	$(compile_source)

$(OBJ_DIR)/rigxml.o: src/rigcontrol/rigxml.cxx
	$(compile_source)

$(OBJ_DIR)/serial.o: src/rigcontrol/serial.cxx
	$(compile_source)

$(OBJ_DIR)/newinstall.o: src/misc/newinstall.cxx
	$(compile_source)

$(OBJ_DIR)/colorbox.o: src/waterfall/colorbox.cxx
	$(compile_source)

$(OBJ_DIR)/mixer.o: src/soundcard/mixer.cxx
	$(compile_source)

$(OBJ_DIR)/macroedit.o: src/misc/macroedit.cxx
	$(compile_source)
