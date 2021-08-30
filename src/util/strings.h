#ifndef _UTIL_STRINGS_H
#define _UTIL_STRINGS_H

#include <stdlib.h>

// Creates a wax string, which is a char* just like a regular string.
// However it allocates two integers before the returned pointer so that
// it can include a constant-time cache for the length and the hash.
char* wstring_from_cstring(const char* chars)
{
  int len = 0;
  while (chars[len] != '\0')
  {
    len++;
  }

  int* structlike_ptr = (int*) malloc(sizeof(char) * (len + 1 + 2 * sizeof(int)));
  structlike_ptr[0] = 0; // hash
  structlike_ptr[1] = len; // constant-access length
  char* str_ptr = (char*) (structlike_ptr + 2);
  for (int i = 0; i <= len; i++) {
    str_ptr[i] = chars[i];
  }
  return str_ptr;
}

void wstring_free(char* str)
{
  free(str - (sizeof(int) * 2));
}

int wstring_len(char* str)
{
  return ((int*)str)[-1];
}

int wstring_hash(char* str)
{
  int hash = ((int*)str)[-2];
  if (hash != 0) return hash;
  char c = str[0];
  int i = 0;
  hash = 5381;
  while (c != '\0')
  {
    hash = hash * 31 + c;
    c = str[++i];
  }
  if (hash == 0) hash = 1319;
  ((int*)str)[-2] = hash;
  return hash;
}

#endif
