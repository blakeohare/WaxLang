#ifndef _WAX_COMPILER_H
#define _WAX_COMPILER_H

#include "../util/fileio.h"
#include "../util/lists.h"
#include "../util/dictionaries.h"
#include "../util/strings.h"
#include "manifest.h"
#include "tokens.h"
#include "compilercontext.h"
#include "parser.h"
#include "resolver.h"

Dictionary* wax_compiler_get_files(const char* path) {
  List* files = directory_gather_files_recursive(path);
  if (files == NULL) return NULL;
  
  String* dot_wax = new_string(".wax");
  Dictionary* src_files = new_dictionary();
  for (int j = 0; j < files->length; ++j) {
    String* file = list_get_string(files, j);
    if (string_ends_with(string_to_lower(file), dot_wax)) {
      String* full_path = normalize_path(string_concat3(path, "/", file->cstring)->cstring);
      dictionary_set(src_files, file, full_path);
    }
  }
  return src_files;
}

void wax_compile(ProjectManifest* manifest, ModuleMetadata* module) {
  Dictionary* src_files = wax_compiler_get_files(module->src->cstring);
  if (src_files == NULL || src_files->size == 0) {
    printf("Module is empty!\n");
    return;
  }

  gc_save_item(src_files);
  List* src_file_names = dictionary_get_keys(src_files);
  gc_save_item(src_file_names);

  CompilerContext* ctx = new_compiler_context();
  gc_save_item(ctx);

  for (int i = 0; i < src_file_names->length; ++i) {
    String* name = list_get_string(src_file_names, i);
    String* full_path = (String*) dictionary_get(src_files, name);
    String* content = file_read_text(full_path->cstring);
    ctx->tokens = tokenize(full_path, content);
    parse_first_pass(ctx);
    ctx->tokens = NULL;
    gc_perform_pass();
  }

  if (ctx->error_messages->length == 0) {
    wax_resolve_module(ctx);
  }

  List* errors = ctx->error_messages;
  List* error_tokens = ctx->error_tokens;
  if (errors->length > 0) {
    printf("The following errors were countered:\n");
    for (int i = 0; i < errors->length; ++i) {
      printf("  ");
      Token* token = (Token*) list_get(error_tokens, i);
      if (token != NULL) {
        printf("%s Line %d Col %d: ", token->file->cstring, token->line, token->col);
      }
      printf("%s\n", list_get_string(errors, i)->cstring);
    }
  } else {
    printf("Success!\n");
  }

  gc_release_item(src_files);
  gc_release_item(src_file_names);
  gc_perform_pass();
}
#endif
