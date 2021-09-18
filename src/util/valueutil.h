#ifndef _UTIL_VALUEUTIL_H
#define _UTIL_VALUEUTIL_H

#include <stdlib.h>
#include <stdio.h>
#include "strings.h"
#include "lists.h"
#include "dictionaries.h"
#include "primitives.h"
#include <math.h>

void _unknown_value_to_string(void* item, StringBuilder* sb);

void _unknown_value_to_string(void* item, StringBuilder* sb) {
  if (item == NULL) {
    string_builder_append_chars(sb, "NULL");
    return;
  }
  static char buffer[50];
  int buf_size = 0;
  switch (gc_get_type(item)) {
    case 'N':
      string_builder_append_chars(sb, "<null>");
      break;
    case 'I':
      {
        int value = ((Integer*)item)->value;
        if (value <= 0) {
          if (value == 0) {
            string_builder_append_char(sb, '0');
            return;
          }
          string_builder_append_char(sb, '-');
          value *= -1;
        }
        while (value > 0) {
          int digit = value % 10;
          value = value / 10;
          buffer[buf_size++] = '0' + digit;
        }
        while (buf_size --> 0) {
          string_builder_append_char(sb, buffer[buf_size]);
        }
      }
      break;

    case 'F':
      {
        // TODO: why are all the sprintf solutions causing corruption?
        double whole_value = ((Float*)item)->value;
        if (whole_value <= 0) {
          if (whole_value == 0.0) {
            string_builder_append_chars(sb, "0.0");
            return;
          }
          string_builder_append_char(sb, '-');
          whole_value *= -1;
        }

        whole_value += 0.0000000000001;

        int int_portion = (int)floor(whole_value);
        whole_value -= int_portion;
        while (int_portion > 0) {
          buffer[buf_size++] = (int_portion % 10) + '0';
          int_portion /= 10;
        }
        while (buf_size --> 0) {
          string_builder_append_char(sb, buffer[buf_size]);
        }
        string_builder_append_char(sb, '.');
        if (whole_value == 0) {
          string_builder_append_char(sb, '0');
          return;
        }
        for (int i = 0; i < 10; ++i) {
          if (whole_value == 0.0) break;
          whole_value *= 10;
          int digit = (int) floor(whole_value);
          buffer[buf_size++] = digit + '0';
          whole_value -= digit;
        }
        buffer[buf_size] = '\0';

        while (buf_size > 1 && buffer[buf_size - 1] == '0') {
          buffer[--buf_size] = '\0';
        }
        string_builder_append_chars(sb, buffer);
      }
      break;

    case 'B':
      string_builder_append_chars(sb, ((Boolean*)item)->value ? "true" : "false");
      break;
    case 'S':
      string_builder_append_chars(sb, ((String*)item)->cstring);
      break;
    case 'L':
      {
        string_builder_append_chars(sb, "[");
        List* list = (List*) item;
        for (int i = 0; i < list->length; ++i) {
          if (i > 0) string_builder_append_chars(sb, ", ");
          _unknown_value_to_string(list->items[i], sb);
        }
        string_builder_append_chars(sb, "]");
      }
      break;
    case 'D':
      {
        Dictionary* dict = (Dictionary*) item;
        if (dict->size == 0) {
          string_builder_append_chars(sb, "{}");
        } else {
          string_builder_append_chars(sb, "{ ");
          for (int i = 0; i < dict->size; ++i) {
            String* key = dict->keys[i];
            void* value = dict->values[i];
            if (i > 0) string_builder_append_chars(sb, ", ");
            _unknown_value_to_string(key, sb);
            string_builder_append_chars(sb, ": ");
            _unknown_value_to_string(value, sb);
          }
          string_builder_append_chars(sb, " }");
        }
      }
      break;
    case 'C':
      {
        string_builder_append_chars(sb, "<TODO:Instances>");
      }
      break;
    default:
      string_builder_append_chars(sb, "UNKNONWN_TYPE:");
      string_builder_append_char(sb, gc_get_type(item));
      break;
  }
}

String* value_to_string(void* item) {
  StringBuilder* sb = new_string_builder();
  _unknown_value_to_string(item, sb);
  String* str = string_builder_to_string(sb);
  string_builder_free(sb);
  return str;
}

List* string_split(const char* value, const char* sep) {
  List* output = new_list();
  if (sep[0] == '\0') {
    char buf[2];
    buf[1] = '\0';
    for (int i = 0; value[i] != '\0'; ++i) {
      buf[0] = value[i];
      list_add(output, new_string(buf));
    }
    return output;
  }
  char first_char = sep[0];
  int sep_len = strlen(sep);
  char c;
  StringBuilder* current_item = new_string_builder();
  int is_match = 0;
  for (int i = 0; value[i] != '\0'; ++i) {
    is_match = 0;
    c = value[i];
    if (c == first_char) {
      is_match = 1;
      for (int j = 1; j < sep_len; ++j) {
        if (value[i + j] != sep[j]) {
          is_match = 0;
          break;
        }
      }
    }

    if (is_match) {
      list_add(output, string_builder_to_string(current_item));
      current_item->length = 0;
      i += sep_len - 1;
    } else {
      string_builder_append_char(current_item, c);
    }
  }
  list_add(output, string_builder_to_string_and_free(current_item));
  return output;
}

String* list_join(List* list, String* sep) {
  if (list->length == 0) return new_string("");
  if (list->length == 1 && gc_get_type(list->items[0]) == 'S') return (String*) list->items[0];
  StringBuilder* sb = new_string_builder();
  int sep_non_empty = sep->length > 0;
  for (int i = 0; i < list->length; ++i) {
    if (i > 0 && sep_non_empty) string_builder_append_chars(sb, sep->cstring);
    void* item = list->items[i];
    _unknown_value_to_string(item, sb);
  }
  return string_builder_to_string_and_free(sb);
}

#endif
