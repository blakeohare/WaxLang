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

void _unknown_value_to_string(void* item, StringBuilder* sb)
{
  static char buffer[50];
  int buf_size = 0;
  switch (gc_get_type(item))
  {
    case 'N':
      string_builder_append_chars(sb, "null");
      break;
    case 'I': 
      {
        int value = ((Integer*)item)->value;
        if (value <= 0)
        {
          if (value == 0)
          {
            string_builder_append_char(sb, '0');
            return;
          }
          string_builder_append_char(sb, '-');
          value *= -1;
        }
        while (value > 0)
        {
          int digit = value % 10;
          value = value / 10;
          buffer[buf_size++] = '0' + digit;
        }
        while (buf_size --> 0)
        {
          string_builder_append_char(sb, buffer[buf_size]);
        }
      }
      break;

    case 'F':
      {
        // TODO: why are all the sprintf solutions causing corruption?
        double whole_value = ((Float*)item)->value;
        if (whole_value <= 0)
        {
          if (whole_value == 0.0)
          {
            string_builder_append_chars(sb, "0.0");
            return;
          }
          string_builder_append_char(sb, '-');
          whole_value *= -1;
        }

        whole_value += 0.0000000000001;

        int int_portion = (int)floor(whole_value);
        whole_value -= int_portion;
        while (int_portion > 0)
        {
          buffer[buf_size++] = (int_portion % 10) + '0';
          int_portion /= 10;
        }
        while (buf_size --> 0)
        {
          string_builder_append_char(sb, buffer[buf_size]);
        }
        string_builder_append_char(sb, '.');
        if (whole_value == 0)
        {
          string_builder_append_char(sb, '0');
          return;
        }
        for (int i = 0; i < 10; ++i)
        {
          if (whole_value == 0.0) break;
          whole_value *= 10;
          int digit = (int) floor(whole_value);
          buffer[buf_size++] = digit + '0';
          whole_value -= digit;
        }
        buffer[buf_size] = '\0';

        while (buf_size > 1 && buffer[buf_size - 1] == '0')
        {
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
        for (int i = 0; i < list->length; ++i)
        {
          if (i > 0) string_builder_append_chars(sb, ", ");
          _unknown_value_to_string(list->items[i], sb);
        }
        string_builder_append_chars(sb, "]");
      }
      break;
    case 'D':
      {
        Dictionary* dict = (Dictionary*) item;
        if (dict->size == 0)
        {
          string_builder_append_chars(sb, "{}");
        }
        else
        {
          string_builder_append_chars(sb, "{ ");
          for (int i = 0; i < dict->size; ++i)
          {
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

String* value_to_string(void* item)
{
  StringBuilder* sb = new_string_builder();
  _unknown_value_to_string(item, sb);
  String* str = string_builder_to_string(sb);
  string_builder_free(sb);
  return str;
}

#endif
