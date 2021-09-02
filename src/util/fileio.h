#ifndef _UTIL_FILEIO_H
#define _UTIL_FILEIO_H

#include <sys/types.h>
#include <sys/stat.h>

#ifdef WINDOWS
#include <strsafe.h>
#include <tchar.h>
#include <windows.h>
#include <fileapi.h>
#else
#include <unistd.h>
#include <dirent.h>
#endif

#include "strings.h"
#include "lists.h"
#include "valueutil.h"

String* _fileio_to_system_path(const char* path)
{
#ifdef WINDOWS
  List* parts = string_split(path, "/");
  return list_join(parts, new_string("\\"));
#else
  return new_string(path);
#endif
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
  char* npath = _fileio_to_system_path(normalize_path(path)->cstring)->cstring;
#ifdef WINDOWS
  struct stat buffer;
  if (stat(npath, &buffer) == 0) return 1;
#else
  if (access(npath, F_OK) == 0) return 1;
#endif
  return 0;
}

#ifdef WINDOWS
TCHAR* _fileio_make_tchar_buffer(const char* path, const char* suffix)
{
    // HACK: not sure why the proper way isn't working
    char* npath = _fileio_to_system_path(normalize_path(path)->cstring)->cstring;
    int path_len = strlen(npath);
    int suffix_len = suffix == NULL ? 0 : strlen(suffix);
    TCHAR* buffer = (TCHAR*)malloc_clean(sizeof(TCHAR) * (MAX_PATH + 1));
    char* copy_this = npath;
    int j = 0;
    for (int i = 0; i < path_len + suffix_len; ++i) {
        if (i == path_len) {
            copy_this = suffix;
            j = 0;
        }
        buffer[i] = copy_this[j++];
    }
    return buffer;
}
#endif

int is_directory(const char* path)
{
#ifdef WINDOWS
  TCHAR* szPath = _fileio_make_tchar_buffer(path, NULL);
  if (szPath == NULL) return 0;
  DWORD dwAttrib = GetFileAttributes(szPath);
  free(szPath);
  if (dwAttrib == INVALID_FILE_ATTRIBUTES) return 0;
  int is_dir = (dwAttrib & FILE_ATTRIBUTE_DIRECTORY) != 0;
  return is_dir;
#else
  struct stat statbuf;
  if (stat(_fileio_to_system_path(path)->cstring, &statbuf) != 0) return 0;
  return S_ISDIR(statbuf.st_mode);
#endif
}

String* file_read_text(const char* path)
{
  char* npath = _fileio_to_system_path(normalize_path(path)->cstring)->cstring;
#ifdef WINDOWS
  FILE* file;
  if (fopen_s(&file, npath, "r") != 0) return NULL;
#else
  FILE* file = fopen(npath, "r");
#endif
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
#ifdef WINDOWS
  TCHAR* szDir = _fileio_make_tchar_buffer(path, "\\*");
  if (szDir == NULL) return NULL;
  WIN32_FIND_DATA ffd;
  HANDLE hFind = INVALID_HANDLE_VALUE;
  hFind = FindFirstFile(szDir, &ffd);
  if (hFind != INVALID_HANDLE_VALUE) {
    do {
      // TODO: There's some data loss here for wide characters.
      StringBuilder* sb = new_string_builder();
      for (int i = 0; ffd.cFileName[i] != 0; i ++) {
          char c = ffd.cFileName[i];
          string_builder_append_char(sb, c);
      }
      String* filename = string_builder_to_string_and_free(sb);
      if (!string_equals_chars(filename, ".") && !string_equals_chars(filename, "..")) {
        list_add(output, filename);
      }
    }
    while (FindNextFile(hFind, &ffd) != 0);
  }
  FindClose(hFind);
  free(szDir);
#else
  DIR* dir;
  struct dirent* entry;
  dir = opendir(npath);
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
#endif
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
