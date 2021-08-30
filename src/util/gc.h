#ifndef _UTIL_GC_H
#define _UTIL_GC_H

#include <stdio.h>
#include <stdlib.h>

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
  char type;
} GCValue;

GCValue* _gc_get_allocations()
{
  static GCValue* alloc_head = NULL;
  if (alloc_head == NULL)
  {
    alloc_head = (GCValue*) malloc(sizeof(GCValue));
    alloc_head->mark = 0;
    alloc_head->type = '0';
    alloc_head->save = 1;
    alloc_head->gc_field_count = 0;
    alloc_head->next = alloc_head;
    alloc_head->prev = alloc_head;
  }
  return alloc_head;
}

void* gc_create_item(int size, char item_type)
{
  GCValue* item = (GCValue*) malloc(size + sizeof(GCValue));
  item->mark = 0;
  item->type = item_type;
  item->gc_field_count = 0; // if something needs to be stored here, the instance initializer will set it.

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
  return (void*)(item + 1);
}

char gc_get_type(void* value)
{
  GCValue* gcvalue = (GCValue*)value;
  gcvalue -= 1;
  return gcvalue->type;
}

#endif
