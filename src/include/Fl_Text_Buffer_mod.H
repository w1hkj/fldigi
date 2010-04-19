#include <config.h>

#if FLDIGI_FLTK_API_MAJOR == 1 && FLDIGI_FLTK_API_MINOR < 3
#  include "Fl_Text_Buffer_mod_1_1.H"
#elif FLDIGI_FLTK_API_MAJOR == 1 && FLDIGI_FLTK_API_MINOR == 3
#  include "Fl_Text_Buffer_mod_1_3.H"
#endif
