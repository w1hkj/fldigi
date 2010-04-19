#include <config.h>

#if FLDIGI_FLTK_API_MAJOR == 1 && FLDIGI_FLTK_API_MINOR < 3
#  include "Fl_Text_Display_mod_1_1.cxx"
#elif FLDIGI_FLTK_API_MAJOR == 1 && FLDIGI_FLTK_API_MINOR == 3
#  include "Fl_Text_Display_mod_1_3.cxx"
#endif
