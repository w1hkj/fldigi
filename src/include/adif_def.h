#ifndef ADIF_DEF
#define ADIF_DEF
#include "field_def.h"

struct FIELD {
  char *name;
  int  size;
};

extern FIELD fields[];

#endif
