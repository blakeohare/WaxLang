#ifndef _UTIL_GCBASE_H
#define _UTIL_GCBASE_H

#include <stdio.h>
#include <stdlib.h>

#include "util.h"

/*
  Types:
    S - string
    N - null
    I - int
    F - float (double)
    L - list
    D - dictionary
    C - instance of a struct (complex)
*/

typedef struct _GCValue {
  int mark;
  int gc_field_count;
  struct _GCValue* next;
  struct _GCValue* prev;
  int save;
  int type;
} GCValue;

GCValue* _gc_get_allocations()
{
  static GCValue* alloc_head = NULL;
  if (alloc_head == NULL)
  {
    alloc_head = (GCValue*) malloc_clean(sizeof(GCValue));
    alloc_head->mark = 0;
    alloc_head->gc_field_count = 0;
    alloc_head->type = 0;
    alloc_head->save = 1;
    alloc_head->next = alloc_head;
    alloc_head->prev = alloc_head;
  }
  return alloc_head;
}

void* gc_create_item(int size, char item_type)
{
  GCValue* item = (GCValue*) malloc_clean(size + sizeof(GCValue));
  GCValue* payload = item + 1;
  item->mark = 0;
  item->type = item_type;
  item->gc_field_count = 0; // if something needs to be stored here, the instance initializer will set it.
  item->save = 0;

  GCValue* head = _gc_get_allocations();
  if (head->next == head)
  {
    head->next = item;
    head->prev = item;
    item->next = head;
    item->prev = head;
  }
  else
  {
    GCValue* next = head->next;
    item->next = next;
    next->prev = item;
    head->next = item;
    item->prev = head;
  }
  return (void*)payload;
}

char gc_get_type(void* value)
{
  GCValue* gcvalue = (GCValue*)value;
  gcvalue -= 1;
  return (char) gcvalue->type;
}

int* _gc_get_current_pass_id()
{
  static int pass_id = 1;
  return &pass_id;
}


#endif
