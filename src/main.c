#include <stdio.h>
#include "util/strings.h"

int main(int argc, char** argv) {
  char* value = wstring_from_cstring("Hello, World!");
  printf("%s\n", value);
  wstring_free(value);
}
