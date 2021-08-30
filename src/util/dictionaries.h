#ifndef _UTIL_DICTIONARIES_H
#define _UTIL_DICTIONARIES_H

#include <stdlib.h>
#include <string.h>
#include "strings.h"
#include "lists.h"
#include "gc.h"

typedef struct _DictEntry {
  String* key;
  int index;
  struct _DictEntry* next;
} DictEntry;

typedef struct _Dictionary {
  int bucket_length;
  int size;
  DictEntry** buckets;
  String** keys;
  void** values;
} Dictionary;

Dictionary* new_dictionary()
{
  Dictionary* dict = (Dictionary*) gc_create_item(sizeof(Dictionary), 'D');
  dict->bucket_length = 0;
  dict->size = 0;
  dict->buckets = NULL;
  dict->keys = NULL;
  dict->values = NULL;
  return dict;
}

void _dict_rehash(Dictionary* dict)
{
  int old_length = dict->bucket_length;
  int new_length = dict->bucket_length * 2;
  DictEntry* entries = NULL; // grab them all and put them in this linked list
  for (int i = 0; old_length; ++i)
  {
    DictEntry* walker = dict->buckets[i];
    while (walker != NULL)
    {
      DictEntry* next = walker->next;
      walker->next = entries;
      entries = walker;
      walker = next;
    }
  }

  free(dict->buckets);
  dict->buckets = (DictEntry**) malloc(sizeof(DictEntry*) * new_length);
  for (int i = 0; i < new_length; ++i)
  {
    dict->buckets[i] = NULL;
  }

  // key list goes up to bucket length + 1 when rehash is called
  for (int i = 0; i <= old_length; ++i)
  {
    String* key = dict->keys[i];
    void* value = dict->values[i];
    int bucket_index = key->hash % new_length;
    DictEntry* entry = entries;
    entries = entry->next;
    entry->next = dict->buckets[bucket_index];
    dict->buckets[bucket_index] = entry;
  }
}

int _dict_get_index(Dictionary* dict, String* key, int create_if_missing)
{
  if (dict->size == 0) 
  {
    if (create_if_missing)
    {
      dict->bucket_length = 8;
      dict->size = 1;
      dict->buckets = (DictEntry**) malloc(sizeof(DictEntry*) * dict->bucket_length);
      dict->keys = (String**) malloc(sizeof(String*) * (dict->bucket_length + 1));
      dict->values = (void**) malloc(sizeof(void*) * (dict->bucket_length + 1));
      for (int i = 0; i <= dict->bucket_length; ++i)
      {
        dict->keys[i] = NULL;
        dict->values[i] = NULL;
      }
      DictEntry* entry = (DictEntry*) malloc(sizeof(DictEntry));
      entry->key = key;
      entry->index = 0;
      entry->next = NULL;
      dict->buckets[key->hash % dict->bucket_length] = entry;
      dict->keys[0] = key;
      return 0;
    }
    else
    {
      return -1;
    }
  }
  else
  {
    int bucket_index = key->hash % dict->bucket_length;
    DictEntry* walker = dict->buckets[bucket_index];
    DictEntry* match = NULL;
    while (walker != NULL)
    {
      if (string_equals(walker->key, key))
      {
        match = walker;
      }
      else
      {
        walker = walker->next;
      }
    }
    if (match) return match->index;

    if (create_if_missing)
    {
      int index = dict->size;
      dict->keys[index] = key;
      dict->size++;
      DictEntry* entry = (DictEntry*) malloc(sizeof(DictEntry));
      entry->key = key;
      entry->index = index;
      entry->next = dict->buckets[bucket_index];
      dict->buckets[bucket_index] = entry;

      if (dict->size > dict->bucket_length)
      {
        _dict_rehash(dict);
      }
      return entry->index;
    }
    else
    {
      return -1;
    }
  }
}

// returns 1 if it's a collision/overwrite
int dictionary_set(Dictionary* dict, String* key, void* value)
{
  int size = dict->size;
  int index = _dict_get_index(dict, key, 1);
  dict->values[index] = value;
  return size == dict->size ? 1 : 0;
}

void* dictionary_get(Dictionary* dict, String* key)
{
  int index = _dict_get_index(dict, key, 0);
  if (index == -1) return NULL;
  return dict->values[index];
}

List* dictionary_get_values(Dictionary* dict)
{
  List* keys = new_list();
  for (int i = 0; i < dict->size; ++i)
  {
    list_add(keys, dict->keys[i]);
  }
  return keys;
}

#endif
