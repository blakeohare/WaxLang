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
  struct _GCValue* next;
  struct _GCValue* prev;
  const char* name;
  int id;
  int mark;
  int gc_field_count;
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
  item->name = NULL;
  item->save = 0;
  item->id = 0;

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

void* gc_create_struct(int size, const char* name, int field_count)
{
  static int obj_id = 1;

  void* item = gc_create_item(size, 'C');
  GCValue* gc_item = ((GCValue*) item) - 1;
  gc_item->gc_field_count = field_count;
  gc_item->name = name;
  gc_item->id = obj_id++;
  return item;
}

char gc_get_type(void* value)
{
  GCValue* gcvalue = (GCValue*)value;
  gcvalue -= 1;
  return (char) gcvalue->type;
}

int gc_is_type(void* value, char type)
{
  if (value == NULL) return 0;
  if (gc_get_type(value) == type) return 1;
  return 0;
}

int* _gc_get_current_pass_id()
{
  static int pass_id = 1;
  return &pass_id;
}


#endif
