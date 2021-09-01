#ifndef _UTIL_DICTIONARIES_H
#define _UTIL_DICTIONARIES_H

#include <stdlib.h>
#include <string.h>
#include "gcbase.h"
#include "lists.h"
#include "strings.h"
#include "util.h"

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
  dict->buckets = (DictEntry**) malloc_ptr_array(new_length);

  // key list goes up to bucket length + 1 when rehash is called
  for (int i = 0; i < dict->size; ++i)
  {
    String* key = dict->keys[i];
    void* value = dict->values[i];
    int bucket_index = key->hash & (new_length - 1);
    DictEntry* entry = entries;
    entries = entry->next;
    entry->next = dict->buckets[bucket_index];
    dict->buckets[bucket_index] = entry;
  }
}

int _dict_get_index(Dictionary* dict, String* key, int create_if_missing)
{
  if (dict->buckets == NULL) 
  {
    if (!create_if_missing) return -1;
    
    dict->bucket_length = 8;
    dict->size = 0;
    dict->buckets = (DictEntry**) malloc_ptr_array(dict->bucket_length);
    dict->keys = (String**) malloc_ptr_array(dict->bucket_length + 1);
    dict->values = (void**) malloc_ptr_array(dict->bucket_length + 1);
  }

  int bucket_index = key->hash & (dict->bucket_length - 1);
  for (DictEntry* walker = dict->buckets[bucket_index]; walker != NULL; walker = walker->next)
  {
    if (string_equals(walker->key, key)) return walker->index;
  }
  
  if (create_if_missing)
  {
    int index = dict->size;
    dict->keys[index] = key;
    dict->size++;
    DictEntry* entry = (DictEntry*) malloc_clean(sizeof(DictEntry));
    entry->key = key;
    entry->index = index;
    entry->next = dict->buckets[bucket_index];
    dict->buckets[bucket_index] = entry;

    if (dict->size > dict->bucket_length) _dict_rehash(dict);
    
    return index;
  }
  return -1;
}

// returns 1 if it's a collision/overwrite
int dictionary_set(Dictionary* dict, String* key, void* value)
{
  int original_size = dict->size;
  int index = _dict_get_index(dict, key, 1);
  dict->values[index] = value;
  if (dict->size > original_size)
  {
    // only overwrite the key if it was added so that the 
    // actual string instance is the same in the DictEntry 
    // as in the Key list so that garbage collection has
    // fewer collections to step through.
    dict->keys[index] = key; 
    return 0;
  }
  return 1;
}

void* dictionary_get(Dictionary* dict, String* key)
{
  int index = _dict_get_index(dict, key, 0);
  if (index == -1) return NULL;
  return dict->values[index];
}

List* dictionary_get_keys(Dictionary* dict)
{
  List* keys = new_list();
  for (int i = 0; i < dict->size; ++i)
  {
    list_add(keys, dict->keys[i]);
  }
  return keys;
}

int dictionary_has_key(Dictionary* dict, String* key)
{
  int index = _dict_get_index(dict, key, 0);
  return index == -1 ? 0 : 1;
}

int is_dictionary(void* obj)
{
  return gc_is_type(obj, 'D');
}

#endif
