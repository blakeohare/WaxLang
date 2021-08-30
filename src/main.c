#include <stdio.h>
#include "util/strings.h"
#include "util/lists.h"
#include "util/dictionaries.h"
#include "util/json.h"
#include "util/valueutil.h"

int main(int argc, char** argv) {
  String* value = new_string("Hello, World!");
  printf("%s\n", value->cstring);

  List* list = new_list();
  for (int i = 0; i < argc; ++i)
  {
    list_add(list, new_string(argv[i]));
  }

  printf("Args length is %d\n", list->length);
  for (int i = 0; i < list->length; ++i)
  {
    String* str = (String*) list_get(list, i);
    printf("Arg #%d is '%s'\n", (i + 1), str->cstring);
  }

  int error_code;
  int error_line;
  int error_col;
  char* test = "{\"a\": 42, \"b\": [1, 2, true], \"c\":\n false, \"d\": { \"nested?\": null, \"yes\": \"nifty!\"   ,\n\"pi\":3.1415926535897 }\n }";
  void* output = json_parse(test, &error_code, &error_line, &error_col);
  if (error_code) json_print_error(error_code, error_line, error_col);
  else printf("%s\n", value_to_string(output)->cstring);
}
