#include <stdio.h>
#include "util/strings.h"
#include "util/lists.h"
#include "util/dictionaries.h"
#include "util/valueutil.h"
#include "util/gc.h"
#include "wax/manifest.h"
#include "wax/compiler.h"

int main(int argc, char** argv) {
  argc = 2;
  char* arg[2];
  char* arg1 = "../samples/PrimeExample/manifest.python.json";
  argv = arg;
  argv[0] = arg1;
  argv[1] = arg1;
  argv = &arg;
  if (argc != 2) {
    printf("Usage: waxcli manifest-file.json\n");
    return 0;
  }

  ProjectManifest* manifest = wax_manifest_load(argv[1]);

  if (manifest->has_error) {
    printf("%s\n", manifest->error->cstring);
  } else {
    gc_save_item(manifest);

    List* modules = manifest->modules;
    for (int i = 0; i < modules->length; ++i) {
      ModuleMetadata* mm = (ModuleMetadata*) modules->items[i];
      if (mm->lang == LANG_WAX) {
        printf("Transpiling wax project '%s'...\n", mm->name->cstring);
        wax_compile(manifest, mm);
        printf("\n");
      } else {
        printf("TODO: wrap project %s from %s\n", mm->name->cstring, mm->src->cstring);
      }
    }

    gc_release_item(manifest);
  }

  gc_perform_pass();

  return 0;
}
