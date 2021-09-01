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

  ProjectManifest* manifest = wax_manifest_load(argv[1]);
  if (manifest->has_error)
  {
    printf("%s\n", manifest->error->cstring);
  }
  else
  {
    List* gc_save_collection = new_list();
    list_add(gc_save_collection, manifest);
    gc_run_with_single_saved_item(gc_save_collection);
  }
  
  gc_shutdown();

  return 0;
}
