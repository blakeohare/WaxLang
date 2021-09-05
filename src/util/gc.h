#ifndef _UTIL_GC_H
#define _UTIL_GC_H

#include "gcbase.h"
#include "dictionaries.h"
#include "lists.h"
#include "strings.h"

typedef struct _GCQueue {
  GCValue* value;
  struct _GCQueue* next;
} GCQueue;

void gc_tag_item(void* ptr) {
  int* pass_id_ptr = _gc_get_current_pass_id();
  GCValue* value = ((GCValue*)ptr) - 1;
  value->mark = *pass_id_ptr;
}

void gc_init_pass() {
  GCValue* head = _gc_get_allocations();
  int* pass_id_ptr = _gc_get_current_pass_id();
  int pass_id = *pass_id_ptr + 1;
  if (pass_id > 2100000000) {
    GCValue* walker = head->next;
    while (walker != head) {
      walker->mark = 1;
      walker = walker->next;
    }
    pass_id = 2;
    *pass_id_ptr = pass_id;
  }
  *pass_id_ptr = pass_id;
  gc_tag_item((void*)(head + 1));
}

void _gc_add_to_queue(GCQueue** discard_queue, GCQueue** queue, GCValue* item) {
  switch (item->type) {
    case 'L':
    case 'D':
    case 'C':
      break;
    default: return; // no nested values to check.
  }
  if (*discard_queue == NULL) {
    *discard_queue = (GCQueue*) malloc(sizeof(GCQueue));
    (*discard_queue)->next = NULL;
  }

  GCQueue* q = *discard_queue;
  *discard_queue = q->next;
  q->next = *queue;
  q->value = item;
  *queue = q;
}

void gc_run() {
  int* pass_id_ptr = _gc_get_current_pass_id();
  int pass_id = *pass_id_ptr;
  GCQueue* queue = NULL;
  GCValue* head = _gc_get_allocations();
  GCValue* walker = head;

  do {
    int tagged = 0;
    if (walker->mark == pass_id) tagged = 1;
    else if (walker->save > 0) {
      int type = walker->type;
      if (type == 'L' || type == 'D' || type == 'C') tagged = 1;
    }
    if (tagged) {
      GCQueue* entry = (GCQueue*) malloc(sizeof(GCQueue));
      entry->value = walker;
      entry->next = queue;
      queue = entry;
    }
    walker = walker->next;
  } while (walker->next != head);

  GCQueue* discard_queue = NULL;
  while (queue != NULL) {
    GCQueue* t = queue;
    queue = t->next;
    GCValue* current = t->value;
    t->next = discard_queue;
    discard_queue = t;
    switch (current->type) {
      case 'C':
        {
          void** value = (void**) (current + 1);
          for (int i = 0; i < current->gc_field_count; ++i) {
            void* item = value[i];
            if (item != NULL) {
              GCValue* gcitem = ((GCValue*)item) - 1;
              if (gcitem->mark != pass_id) {
                gcitem->mark = pass_id;
                _gc_add_to_queue(&discard_queue, &queue, gcitem);
              }
            }
          }
        }
        break;
      case 'L':
        {
          List* list = (List*) (current + 1);
          for (int i = 0; i < list->length; ++i) {
            void* item = list->items[i];
            if (item != NULL) {
              GCValue* gcitem = ((GCValue*)item) - 1;
              if (gcitem->mark != pass_id) {
                gcitem->mark = pass_id;
                _gc_add_to_queue(&discard_queue, &queue, gcitem);
              }
            }
          }
        }
        break;
      case 'D':
        {
          Dictionary* dict = (Dictionary*) (current + 1);
          String** keys = dict->keys;
          void** values = dict->values;
          // Note that the actual string instance in the bucket is the same as the one in the
          // keys list, even if it is overwritten.
          for (int i = 0; i < dict->size; ++i) {
            GCValue* gckey = ((GCValue*)keys[i]) - 1;
            gckey->mark = pass_id;

            if (values[i] != NULL) {
              GCValue* gcvalue = ((GCValue*)values[i]) - 1;
              if (gcvalue->mark != pass_id) {
                gcvalue->mark = pass_id;
                _gc_add_to_queue(&discard_queue, &queue, gcvalue);
              }
            }
          }
        }
        break;
      default:
        // ignore! no recursive data
        break;
    }
  }

  walker = head->next;
  while (walker != head) {
    if (walker->mark != pass_id && walker->save == 0) {
      GCValue* prev = walker->prev;
      GCValue* next = walker->next;
      GCValue* remove_me = walker;
      walker = prev;
      next->prev = prev;
      prev->next = next;
      switch (remove_me->type) {
        case 'I':
        case 'F':
          free(remove_me);
          break;
        case 'S':
          {
            String* str = (String*) (remove_me + 1);
            free(str->cstring);
            free(remove_me);
          }
          break;
        case 'L':
          {
            List* list = (List*) (remove_me + 1);
            free(list->items);
            free(remove_me);
          }
          break;
        case 'D':
          {
            Dictionary* dict = (Dictionary*) (remove_me + 1);
            free(dict->keys);
            free(dict->values);
            for (int i = 0; i < dict->bucket_length; ++i) {
              DictEntry* bucket = dict->buckets[i];
              while (bucket != NULL) {
                DictEntry* next = bucket->next;
                free(bucket);
                bucket = next;
              }
            }
            free(dict->buckets);
            free(remove_me);
          }
          break;
        case 'C':
          {
            // TODO: callbacks for complex types
            void** value = (void**) (remove_me + 1);
            free(remove_me);
          }
          break;
      }
    }
    walker = walker->next;
  }

  while (discard_queue != NULL) {
    GCQueue* next = discard_queue->next;
    free(discard_queue);
    discard_queue = next;
  }
}

void gc_save_item(void* item) {
  GCValue* gc_item = ((GCValue*) item) - 1;
  gc_item->save++;
}

void gc_release_item(void* item) {
  GCValue* gc_item = ((GCValue*) item) - 1;
  gc_item->save--;
}

void gc_run_with_single_saved_item(void* item) {
  gc_init_pass();
  gc_tag_item(item);
  gc_run();
}

void gc_perform_pass() {
  gc_init_pass();
  gc_run();
}

#endif
