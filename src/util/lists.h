#ifndef _UTIL_LISTS_H
#define _UTIL_LISTS_H

#include <stdlib.h>
#include <string.h>
#include "strings.h"
#include "gcbase.h"
#include "util.h"

typedef struct _List {
  int length;
  int capacity;
  void** items;
} List;

List* new_list()
{
  List* list = (List*) gc_create_item(sizeof(List), 'L');
  list->length = 0;
  list->capacity = 0;
  list->items = NULL;
  return list;
}

void list_add(List* list, void* value)
{
  if (list->length == list->capacity)
  {
    if (list->capacity == 0)
    {
      list->items = (void**) malloc_ptr_array(4);
      list->capacity = 4;
    }
    else
    {
      int new_capacity = list->capacity * 2;
      if (new_capacity < 10) new_capacity = 10;
      void** items = (void**) malloc_ptr_array(new_capacity);
      memcpy(items, list->items, list->length * sizeof(void*));
      free(list->items);
      list->items = items;
      list->capacity = new_capacity;
    }
  }

  list->items[list->length++] = value;
}

void* list_get(List* list, int index)
{
  return list->items[index];
}

String* list_get_string(List* list, int index)
{
  return (String*) list->items[index];
}

void* list_get_last(List* list)
{
  return list->items[list->length - 1];
}

String* list_get_last_string(List* list)
{
  return (String*) list->items[list->length - 1];
}

void* list_pop(List* list)
{
  return list->items[--list->length];
}

void list_push_all(List* list, List* items) 
{
  for (int i = 0; i < items->length; ++i)
  {
    list_add(list, items->items[i]);
  }
}

int is_list(void* obj)
{
  return gc_is_type(obj, 'L');
}

#endif
