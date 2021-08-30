#ifndef _UTIL_PRIMITIVES_H
#define _UTIL_PRIMITIVES_H

#include <stdlib.h>
#include "gc.h"

typedef struct _Integer {
  int value;
} Integer;

typedef struct _Float {
  double value;
} Float;

typedef struct _Boolean {
  int value;
} Boolean;

Integer* wrap_int(int value)
{
  static int initialized = 0;
  static Integer** ints = NULL;
  if (!initialized)
  {
    ints = (Integer**) malloc(sizeof(Integer*) * 2048);
    initialized = 1;
    for (int i = 0; i < 2048; ++i)
    {
      Integer* n = (Integer*) gc_create_item(sizeof(Integer), 'I');
      n->value = i - 1024;
      GCValue* gcn = ((GCValue*)n) - 1;
      gcn->save = 1;
      ints[i] = n;
    }
  }

  if (value < 1024 && value >= -1024) return ints[value + 1024];

  Integer* i = (Integer*) gc_create_item(sizeof(Integer), 'I');
  i->value = value;
  return i;
}

Float* wrap_float(double value)
{
  static Float* ZERO = NULL;
  static Float* ONE = NULL;
  if (ZERO == NULL)
  {
    for (int i = 0; i < 2; ++i)
    {
      Float* f = (Float*) gc_create_item(sizeof(Float), 'F');
      f->value = i + 0.0;
      GCValue* gcf = ((GCValue*)f) - 1;
      gcf->save = 1;
    }
  }
  if (value <= 1.0)
  {
    if (value == 0) return ZERO;
    if (value == 1) return ONE;
  }
  Float* f = (Float*) gc_create_item(sizeof(Float), 'F');
  f->value = value;
  return f;
}

Boolean* wrap_bool(int value)
{
  static Boolean* TRUE = NULL;
  static Boolean* FALSE = NULL;
  if (TRUE == NULL)
  {
    for (int i = 0; i < 2; ++i)
    {
      Boolean* b = (Boolean*) gc_create_item(sizeof(Boolean), 'B');
      GCValue* gcb = ((GCValue*)b) - 1;
      b->value = i == 1;
      gcb->save = 1;
      if (i == 0) FALSE = b;
      else TRUE = b;
    }
  }
  return value ? TRUE : FALSE;
}

void* get_null()
{
  static void* NULL_VALUE = NULL;
  if (NULL_VALUE == NULL)
  {
    NULL_VALUE = gc_create_item(0, 'N');
  }
  return NULL_VALUE;
}

int is_null(void* value)
{
  return (((GCValue*)value) - 1)->type == 'N';
}

#endif
