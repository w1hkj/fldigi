#ifndef ADIF_DEF
#define ADIF_DEF
#include "field_def.h"

#include <FL/Fl_Check_Button.H>

struct FIELD {
  int  type;
  const char *name;
  int  size;
  Fl_Check_Button **btn;
};

extern FIELD fields[];

#endif
