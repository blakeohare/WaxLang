#ifndef _UTIL_FILEIO_H
#define _UTIL_FILEIO_H

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#include "strings.h"
#include "lists.h"
#include "valueutil.h"

String* _fileio_to_system_path(const char* path)
{
  // TODO: for windows, this will have to do replacements.
  return new_string(path);
}

String* normalize_path(const char* path)
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

int file_exists(const char* path)
{
  String* npath = normalize_path(path);
  if (access(_fileio_to_system_path(npath->cstring)->cstring, F_OK) == 0) return 1;
  return 0;
}

int is_directory(const char* path)
{
  struct stat statbuf;
  if (stat(_fileio_to_system_path(path)->cstring, &statbuf) != 0) return 0;
  return S_ISDIR(statbuf.st_mode);
}

String* file_read_text(const char* path)
{
  String* npath = normalize_path(path);
  FILE* file = fopen(_fileio_to_system_path(npath->cstring)->cstring, "r");
  if (!file) return NULL;
  int bytes_read = 0;
  char buffer[1025];
  StringBuilder* sb = new_string_builder();
  while ((bytes_read = fread(buffer, 1, 1024, file)) > 0)
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

List* directory_list(const char* path)
{
  List* output = new_list();
  DIR* dir;
  struct dirent* entry;
  dir = opendir(_fileio_to_system_path(path)->cstring);
  if (dir)
  {
    while ((entry = readdir(dir)) != NULL)
    {
      String* name = new_string(entry->d_name);
      if (string_equals_chars(name, ".") || string_equals_chars(name, "..")) continue;
      list_add(output, name);
    }
    closedir(dir);
  }
  return output;
}

void _directory_gather_files_recursive(List* output, const char* current_path, const char* prefix)
{
  List* files = directory_list(current_path);
  for (int i = 0; i < files->length; ++i)
  {
    String* filename = list_get_string(files, i);
    String* full_path = normalize_path(string_concat3(current_path, "/", filename->cstring)->cstring);
    String* relative_path = prefix == NULL ? filename : string_concat3(prefix, "/", filename->cstring);
    if (is_directory(full_path->cstring))
    {
      _directory_gather_files_recursive(output, full_path->cstring, relative_path->cstring);
    }
    else
    {
      list_add(output, relative_path);
    }
  }
}

List* directory_gather_files_recursive(const char* path)
{
  List* output = new_list();
  if (!is_directory(path)) return NULL;
  _directory_gather_files_recursive(output, path, NULL);
  return output;
}

#endif
