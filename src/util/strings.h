#ifndef _UTIL_STRINGS_H
#define _UTIL_STRINGS_H

#include <stdlib.h>
#include <string.h>
#include "gc.h"

typedef struct _String {
  int length;
  int hash;
  char* cstring;
} String;

typedef struct _StringBuilder {
  int length;
  int capacity;
  char* chars;
} StringBuilder;

String* new_string(const char* value)
{
  int len = 0;
  int hash = 0;
  char c;
  while ((c = value[len]) != '\0')
  {
    hash = hash * 31 + c;
    len++;
  }
  if (hash == 0) hash = 1319;

  char* cstring = (char*) malloc(sizeof(char) * (len + 1));
  memcpy(cstring, value, len + 1);
  String* str = (String*) gc_create_item(sizeof(String), 'S');
  str->length = len;
  str->hash = hash;
  str->cstring = cstring;
  return str;
}

StringBuilder* new_string_builder()
{
  StringBuilder* sb = (StringBuilder*) malloc(sizeof(StringBuilder));
  sb->length = 0;
  sb->capacity = 10;
  sb->chars = (char*) malloc(sizeof(char) * sb->capacity);
  return sb;
}

void _string_builder_ensure_capacity(StringBuilder* sb, int additional_chars)
{
  int required_capacity = sb->length + additional_chars;
  int new_capacity = sb->capacity;
  while (required_capacity < new_capacity)
  {
    new_capacity *= 2;
  }
  if (new_capacity > sb->capacity)
  {
    char* new_chars = (char*) malloc(sizeof(char) * new_capacity);
    memcpy(new_chars, sb->chars, sb->length);
    sb->capacity = new_capacity;
    free(sb->chars);
    sb->chars = new_chars;
  }
}

void string_builder_append_char(StringBuilder* sb, char c)
{
  if (sb->length == sb->capacity) _string_builder_ensure_capacity(sb, 1);
  sb->chars[sb->length++] = c;
}

void string_builder_append_chars(StringBuilder* sb, const char* str)
{
  while (str[0] != '\0')
  {
    if (sb->length == sb->capacity) _string_builder_ensure_capacity(sb, 1);
    sb->chars[sb->length++] = str[0];
    str += 1;
  }
}

String* string_builder_to_string(StringBuilder* sb)
{
  string_builder_append_char(sb, '\0');
  String* str = new_string(sb->chars);
  return str;
}

void string_builder_free(StringBuilder* sb)
{
  free(sb->chars);
  free(sb);
}

int string_equals(String* a, String* b)
{
  if (a->hash != b->hash) return 0;
  if (a->length != b->length) return 0;
  if (a == b) return 1;
  if (a->length == 0) return 1;

  int last = a->length - 1;
  char* achars = a->cstring;
  char* bchars = b->cstring;
  if (achars[last] != bchars[last]) return 0;
  for (int i = 0; i < last; ++i)
  {
    if (achars[i] != bchars[i]) return 0;
  }
  return 1;
}

String* string_replace(const char* value, const char* old_value, const char* new_value)
{
  StringBuilder* output = new_string_builder();
  
  int old_len = strlen(old_value);
  int new_len = strlen(new_value);
  if (old_len == 0) return new_string(value);
  if (value[0] == '\0') return new_string("");
  char first_char = old_value[0];
  int i = 0;
  do
  {
    if (value[i] == first_char)
    {
      int match = 1;
      for (int j = 1; j < old_len; ++j) // don't have to worry about the pointer going off the end since there are \0's for both.
      {
        if (value[i + j] != old_value[j]) {
          match = 0;
          break;
        }
      }
      if (match)
      {
        string_builder_append_chars(output, new_value);
        i += new_len;
      }
      else
      {
        string_builder_append_char(output, first_char);
      }
    }
    else
    {
      string_builder_append_char(output, value[i]);
    }
    ++i;
  } while (value[i] != '\0');
  
  String* str = string_builder_to_string(output);
  string_builder_free(output);
  return str;
}

#endif
