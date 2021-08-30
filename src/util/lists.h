#ifndef _UTIL_LISTS_H
#define _UTIL_LISTS_H

#include <stdlib.h>
#include <string.h>
#include "strings.h"
#include "gc.h"

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
    if (list->length == 0)
    {
      list->items = (void**) malloc(sizeof(void*) * 4);
      list->capacity = 4;
    }
    else
    {
      int new_capacity = list->capacity * 2;
      if (new_capacity < 10) new_capacity = 10;
      void** items = (void**) malloc(sizeof(void*) * new_capacity);
      memcpy(items, list->items, list->capacity * sizeof(void*));
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

#endif
