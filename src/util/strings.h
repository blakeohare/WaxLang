#ifndef _UTIL_STRINGS_H
#define _UTIL_STRINGS_H

#include <stdlib.h>
#include <string.h>
#include "gcbase.h"
#include "util.h"

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
  static String** SINGLE_CHARS = NULL;
  if (SINGLE_CHARS == NULL)
  {
    SINGLE_CHARS = (String**) malloc_clean(sizeof(String*) * 128);
    for (int i = 0; i < 128; ++i)
    {
      String* s = (String*) gc_create_item(sizeof(String), 'S');
      s->length = i == 0 ? 0 : 1;
      s->hash = value[0];
      s->cstring = (char*) malloc_clean(sizeof(char) * 2);
      s->cstring[0] = (char) i;
      s->cstring[1] = '\0';
      GCValue* gc_str = ((GCValue*)s) - 1;
      gc_str->save = 1;
      SINGLE_CHARS[i] = s;
    }
  }
  int len = 0;
  int hash = 0;
  char c;
  while ((c = value[len]) != '\0')
  {
    hash = hash * 31 + c;
    len++;
  }
  if (hash == 0) hash = 1319;

  if (len <= 1 && value[0] >= 0) {
    return SINGLE_CHARS[(int) value[0]];
  }

  char* cstring = (char*) malloc_clean(sizeof(char) * (len + 1));
  memcpy(cstring, value, len);
  cstring[len] = '\0';
  String* str = (String*) gc_create_item(sizeof(String), 'S');
  str->length = len;
  str->hash = hash;
  str->cstring = cstring;
  return str;
}

String* new_string_from_range(const char* chars, int start, int end) {
  if (start == end) return new_string("");
  int len = end - start;
  char* buf = (char*) malloc(sizeof(char) * (len + 1));
  for (int i = 0; i < len; ++i) {
    buf[i] = chars[i + start];
  }
  buf[len] = '\0';
  String* str = new_string(buf);
  free(buf);
  return str;
}

StringBuilder* new_string_builder()
{
  StringBuilder* sb = (StringBuilder*) malloc_clean(sizeof(StringBuilder));
  sb->length = 0;
  sb->capacity = 20;
  sb->chars = (char*) malloc_clean(sizeof(char) * sb->capacity);
  return sb;
}

void _string_builder_ensure_capacity(StringBuilder* sb, int additional_chars)
{
  int required_capacity = sb->length + additional_chars;
  int new_capacity = sb->capacity;
  while (required_capacity > new_capacity)
  {
    new_capacity *= 2;
  }
  if (new_capacity > sb->capacity)
  {
    char* new_chars = (char*) malloc_clean(sizeof(char) * new_capacity);
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

String* string_builder_to_string_and_free(StringBuilder* sb)
{
  String* output = string_builder_to_string(sb);
  string_builder_free(sb);
  return output;
}

String* string_concat(const char* a, const char* b)
{
  StringBuilder* sb = new_string_builder();
  string_builder_append_chars(sb, a);
  string_builder_append_chars(sb, b);
  return string_builder_to_string_and_free(sb);
}

String* string_concat3(const char* a, const char* b, const char* c)
{
  StringBuilder* sb = new_string_builder();
  string_builder_append_chars(sb, a);
  string_builder_append_chars(sb, b);
  string_builder_append_chars(sb, c);
  return string_builder_to_string_and_free(sb);
}

String* string_concat6(const char* a, const char* b, const char* c, const char* d, const char* e, const char* f)
{
  StringBuilder* sb = new_string_builder();
  string_builder_append_chars(sb, a);
  string_builder_append_chars(sb, b);
  string_builder_append_chars(sb, c);
  string_builder_append_chars(sb, d);
  string_builder_append_chars(sb, e);
  string_builder_append_chars(sb, f);
  return string_builder_to_string_and_free(sb);
}

String* string_concat5(const char* a, const char* b, const char* c, const char* d, const char* e)
{
  return string_concat6(a, b, c, d, e, "");
}

String* string_concat4(const char* a, const char* b, const char* c, const char* d)
{
  return string_concat6(a, b, c, d, "", "");
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
  while (value[i] != '\0')
  {
    if (value[i] == first_char)
    {
      int match = 1;
      for (int j = 1; j < old_len; ++j) // don't have to worry about the pointer going off the end since there are \0's for both.
      {
        if (value[i + j] != old_value[j])
        {
          match = 0;
          break;
        }
      }
      if (match)
      {
        string_builder_append_chars(output, new_value);
        i += old_len;
      }
      else
      {
        string_builder_append_char(output, first_char);
        i++;
      }
    }
    else
    {
      string_builder_append_char(output, value[i]);
      i++;
    }
  }
  
  String* str = string_builder_to_string(output);
  string_builder_free(output);
  return str;
}

int string_equals_chars(String* str, const char* chars)
{
  return string_equals(str, new_string(chars));
}

int is_string(void* obj)
{
  return gc_is_type(obj, 'S');
}

String* string_to_lower(String* str)
{
  int len = str->length;
  char* sb = (char*) malloc(sizeof(char) * (len + 1));
  const char* str_chars = str->cstring;
  char c;
  for (int i = 0; i < len; ++i)
  {
    c = str_chars[i];
    if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
    sb[i] = c;
  }
  sb[len] = '\0';
  String* output = new_string(sb);
  free(sb);
  return output;
}

int string_ends_with(String* str, String* chars)
{
  if (chars->length >= str->length) return string_equals(str, chars);
  if (chars->length == 0) return 1;
  int a_last = str->length - 1;
  int b_last = chars->length - 1;
  const char* a = str->cstring;
  const char* b = chars->cstring;
  if (a[a_last] != b[b_last]) return 0;
  int a_index = str->length - chars->length;
  int b_index = 0;
  while (b_index < b_last)
  {
    if (a[a_index++] != b[b_index++]) return 0;
  }
  return 1;
}

#endif
