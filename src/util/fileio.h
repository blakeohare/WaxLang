#ifndef _UTIL_FILEIO_H
#define _UTIL_FILEIO_H

#include <unistd.h>

#include "strings.h"
#include "lists.h"
#include "valueutil.h"

String* normalize_path(char* path)
{
  String* dot = new_string(".");
  String* dotdot = new_string("..");
  String* empty = new_string("");

  String* path_str = string_replace(path, "\\", "/");
  List* raw_path_parts = string_split(path_str->cstring, "/");
  List* path_parts = new_list();
  for (int i = 0; i < raw_path_parts->length; ++i)
  {
    String* part = list_get_string(raw_path_parts, i);
    if (string_equals(part, dot) || string_equals(part, empty))
    {
      // pass!
    }
    else if (string_equals(part, dotdot))
    {
      if (path_parts->length > 0 && !string_equals(list_get_last_string(path_parts), dotdot))
      {
        list_pop(path_parts);
      }
      else
      {
        list_add(path_parts, dotdot);
      }
    }
    else
    {
      list_add(path_parts, part);
    }
  }
  if (path_parts->length == 0) return dot;
  return list_join(path_parts, new_string("/"));
}

int file_exists(char* path)
{
  String* npath = normalize_path(path);
  if (access(npath->cstring, F_OK) == 0) return 1;
  return 0;
}

String* file_read_text(char* path)
{
  String* npath = normalize_path(path);
  FILE* file = fopen(npath->cstring, "r");
  if (!file) return NULL;
  int bytes_read = 0;
  char buffer[256];
  StringBuilder* sb = new_string_builder();
  while ((bytes_read = fread(buffer, 1, 255, file)) > 0)
  {
    buffer[bytes_read] = '\0';
    string_builder_append_chars(sb, buffer);

  }

  if (ferror(file))
  {
    string_builder_free(sb);
    fclose(file);
    return NULL;
  }

  fclose(file);
  return string_builder_to_string_and_free(sb);
}

#endif
