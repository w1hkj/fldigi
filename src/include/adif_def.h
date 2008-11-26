#ifndef ADIF_DEF
#define ADIF_DEF
#include "field_def.h"

struct FIELD {
  int  type;
  const char *name;
  int  size;
};

extern FIELD fields[];

#endif
