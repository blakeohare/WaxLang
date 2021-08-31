#ifndef _UTIL_UTIL_H
#define _UTIL_UTIL_H

#include <stdlib.h>

void** malloc_ptr_array(int length)
{
  void** arr = (void**) malloc(sizeof(void*) * length);
  for (int i = 0; i < length; ++i)
  {
    arr[i] = NULL;
  }
  return arr;
}

void* malloc_clean(int size)
{
  char* ptr = (char*) malloc(size);
  for (int i = 0; i < size; ++i)
  {
    ptr[i] = '\0';
  }
  return ptr;
}

int try_parse_int(const char* value, int* value_out)
{
  int output = 0;
  int start = 0;
  int sign = 1;
  if (value[0] == '-')
  {
    sign = -1;
    start = 1;
  }

  int i = start;
  for (; value[i] != '\0'; ++i)
  {
    char c = value[i];
    if (c < '0' || c > '9') return 0;
    output = output * 10 + (c - '0');
  }
  if (i == start) return 0;
  *value_out = output * sign;
  return 1;
}

int try_parse_float(const char* value, double* value_out)
{
  long int_portion = 0;
  long int_float_portion = 0;
  double divisor = 1.0;
  int start = 0;
  int sign = 1;
  if (value[0] == '-')
  {
    sign = -1;
    start = 1;
  }

  int in_int_portion = 1;
  int i = start;
  for (; value[i] != '\0'; ++i)
  {
    char c = value[i];
    if (c == '.')
    {
      if (!in_int_portion) return 0;
      in_int_portion = 0;
      divisor = 1.0;
    }
    else if (c >= '0' && c <= '9')
    {
      if (in_int_portion)
      {
        int_portion = int_portion * 10 + (c - '0');
      }
      else
      {
        int_float_portion = int_float_portion * 10 + (c - '0');
        divisor *= 10.0;
      }
    }
    else
    {
      return 0;
    }
  }
  if (i == start || (i == start + 1 && !in_int_portion)) return 0;
  double output = int_portion + (int_float_portion / divisor);
  *value_out = sign * output;
  return 1;
}

#endif
