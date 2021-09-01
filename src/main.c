#include <stdio.h>
#include "util/strings.h"
#include "util/lists.h"
#include "util/dictionaries.h"
#include "util/valueutil.h"
#include "util/gc.h"
#include "wax/manifest.h"

int main(int argc, char** argv)
{
  if (argc != 2)
  {
    printf("Usage: waxcli manifest-file.json\n");
    return 0;
  }

  char* manifest_file_path = argv[1];
  Dictionary* manifest = wax_manifest_load(manifest_file_path);
  gc_run_with_single_saved_item(manifest);

  String* error_key = new_string("@error");
  if (dictionary_has_key(manifest, error_key))
  {
    printf("%s\n", ((String*)dictionary_get(manifest, error_key))->cstring);
    return 0;
  }

  printf("Found this manifest: %s\n", value_to_string(manifest)->cstring);

  gc_shutdown();

  return 0;
}
