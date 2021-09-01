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
    gc_save_item(manifest);
    
    List* modules = manifest->modules;
    printf("Found %d modules\n", modules->length);
    for (int i = 0; i < modules->length; ++i)
    {
      ModuleMetadata* mm = (ModuleMetadata*) modules->items[i];
      printf("TODO: compile %s from %s\n", mm->name->cstring, mm->src->cstring);
    }

    gc_release_item(manifest);
  }
  
  gc_perform_pass();

  return 0;
}
