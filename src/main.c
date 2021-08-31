#include <stdio.h>
#include "util/strings.h"
#include "util/lists.h"
#include "util/dictionaries.h"
#include "util/json.h"
#include "util/valueutil.h"
#include "util/fileio.h"

int main(int argc, char** argv)
{
  if (argc != 2)
  {
    printf("Usage: waxcli manifest-file.json\n");
    return 0;
  }

  char* manifest_file_path = argv[1];
  String* manifest_file_content = file_read_text(manifest_file_path);
  if (manifest_file_content == NULL)
  {
    printf("Manifest file does not exist: %s\n", manifest_file_path);
    return 0;
  }

  int error_code;
  int error_line;
  int error_col;
  
  void* manifest = json_parse(manifest_file_content->cstring, &error_code, &error_line, &error_col);
  if (error_code)
  {
    json_print_error(error_code, error_line, error_col);
    return 0;
  }
  printf("Found this manifest: %s\n", value_to_string(manifest)->cstring);
  
  return 0;
}
