#ifndef _WAX_MANIFEST_H
#define _WAX_MANIFEST_H

#include "../util/fileio.h"
#include "../util/json.h"
#include "../util/dictionaries.h"
#include "../util/lists.h"
#include "../util/strings.h"
#include "../util/gcbase.h"
#include "../util/gc.h"

enum Lang { LANG_UNKNOWN, LANG_WAX, LANG_PYTHON, LANG_C, LANG_JAVASCRIPT, LANG_PHP };
enum ModuleAction { ACTION_UNKNOWN, ACTION_BUNDLE, ACTION_EXTENSION, ACTION_SERVICE };

#define MODULE_METADATA_GC_FIELDS 2
#define MODULE_METADATA_NAME "ModuleMetadata"
typedef struct _ModuleMetadata {
  String* name;
  String* src;
  enum ModuleAction action;
  enum Lang lang;
  int is_main;
} ModuleMetadata;

#define PROJECT_MANIFEST_GC_FIELDS 4
#define PROJECT_MANIFEST_NAME "ProjectManifest"
typedef struct _ProjectManifest {
  List* modules;
  String* output_path;
  String* output_type;
  String* error;
  int has_error;
} ProjectManifest;

ProjectManifest* new_project_manifest(String* output_path, String* output_type) {
  ProjectManifest* pm = (ProjectManifest*) gc_create_struct(sizeof(ProjectManifest), PROJECT_MANIFEST_NAME, PROJECT_MANIFEST_GC_FIELDS);
  pm->output_path = output_path;
  pm->output_type = output_type;
  pm->modules = new_list();
  pm->error = NULL;
  pm->has_error = 0;
  return pm;
}

ModuleMetadata* new_module_metadata(String* name, String* src, String* action, String* lang) {
  ModuleMetadata* mm = (ModuleMetadata*) gc_create_struct(sizeof(ModuleMetadata), MODULE_METADATA_NAME, MODULE_METADATA_GC_FIELDS);
  mm->name = name;
  mm->src = src;

  if (string_equals_chars(action, "bundle")) mm->action = ACTION_BUNDLE; 
  else if (string_equals_chars(action, "extension")) mm->action = ACTION_EXTENSION;
  else if (string_equals_chars(action, "service")) mm->action = ACTION_SERVICE;
  else mm->action = ACTION_UNKNOWN;

  if (string_equals_chars(lang, "wax")) mm->lang = LANG_WAX;
  else if (string_equals_chars(lang, "python")) mm->lang = LANG_PYTHON;
  else if (string_equals_chars(lang, "c")) mm->lang = LANG_C;
  else if (string_equals_chars(lang, "javascript")) mm->lang = LANG_JAVASCRIPT;
  else if (string_equals_chars(lang, "php")) mm->lang = LANG_PHP;
  else mm->lang = LANG_UNKNOWN;

  mm->is_main = 0;

  return mm;
}

Dictionary* wax_manifest_make_error_dict(String* msg) {
  Dictionary* dict = new_dictionary();
  dictionary_set(dict, new_string("@error"), msg);
  return dict;
}

ProjectManifest* wax_manifest_make_error(String* error) {
  ProjectManifest* pm = new_project_manifest(new_string(""), new_string(""));
  pm->has_error = 1;
  pm->error = error;
  return pm;
}

Dictionary* wax_manifest_load_impl(const char* path);
Dictionary* wax_manifest_load_impl(const char* path) {
  String* str_inherit = new_string("inherit");
  String* str_error = new_string("@error");
  String* str_module_targets = new_string("moduleTargets");
  String* str_src = new_string("src");

  if (!file_exists(path))
    return wax_manifest_make_error_dict(string_concat3("Could not find file: '", path, "'"));

  String* file_content = file_read_text(path);
  if (file_content == NULL)
    return wax_manifest_make_error_dict(string_concat3("Error while reading file: '", path, "'"));
    
  JsonParseResult json_result = json_parse(file_content->cstring);
  if (json_result.error)
    return wax_manifest_make_error_dict(string_concat4("JSON error while parsing '", path, "': ", json_get_error(json_result)->cstring));

  if (!is_dictionary(json_result.value))
    return wax_manifest_make_error_dict(string_concat3("JSON error: '", path, "' does not contain an object as its root JSON value."));

  Dictionary* manifest = (Dictionary*) json_result.value;

  // All the src paths must be adjusted relative to the current working directory
  if (is_list(dictionary_get(manifest, str_module_targets))) {
    List* module_targets = (List*) dictionary_get(manifest, str_module_targets);
    for (int i = 0; i < module_targets->length; ++i) {
      if (is_dictionary(list_get(module_targets, i))) {
        Dictionary* module_target = (Dictionary*) list_get(module_targets, i);
        if (is_string(dictionary_get(module_target, str_src))) {
          String* src_path_raw = (String*) dictionary_get(module_target, str_src);
          String* src_path_adjusted = normalize_path(string_concat3(path, "/../", src_path_raw->cstring)->cstring);
          dictionary_set(module_target, str_src, src_path_adjusted);
        }
      }
    }
  }

  void* inherit_path = dictionary_get(manifest, str_inherit);
  if (inherit_path != NULL) {
    if (!is_string(inherit_path)) return wax_manifest_make_error_dict(string_concat3("Inherit field in '", path, "' does not contain a string."));
    String* canonicalized_inherit_path = normalize_path(string_concat3(path, "/../", ((String*)inherit_path)->cstring)->cstring);
    
    Dictionary* parent_manifest = wax_manifest_load_impl(canonicalized_inherit_path->cstring);

    if (dictionary_has_key(parent_manifest, str_error)) return parent_manifest;
    List* keys = dictionary_get_keys(manifest);
    for (int i = 0; i < keys->length; ++i) {
      String* key = list_get_string(keys, i);
      void* value = dictionary_get(manifest, key);
      void* parent_value = dictionary_get(parent_manifest, key);
      
      // for moduleTargets, append the manifest's value after the parent's value
      if (string_equals(key, str_module_targets) && is_list(value) && is_list(parent_value)) {
        list_push_all(parent_value, value);
        value = parent_value;
      }

      dictionary_set(parent_manifest, key, value);
    }
    manifest = parent_manifest;
  }
  return manifest;
}

ProjectManifest* _wax_manifest_load_verifier(const char* path) {
  Dictionary* manifest = wax_manifest_load_impl(path);
  String* str_error = new_string("@error");
  if (dictionary_has_key(manifest, str_error)) return wax_manifest_make_error((String*) dictionary_get(manifest, str_error));

  String* str_main_module = new_string("mainModule");
  String* str_module_targets = new_string("moduleTargets");
  String* str_name = new_string("name");
  String* str_src = new_string("src");
  String* str_lang = new_string("lang");
  String* str_action = new_string("action");
  String* str_output = new_string("output");
  String* str_output_type = new_string("outputType");

  String* req_fields[4] = {
    str_main_module,
    str_module_targets,
    str_output,
    str_output_type,
  };
  for (int i = 0; i < 4; ++i) {
    if (!dictionary_has_key(manifest, req_fields[i])) {
      return wax_manifest_make_error(string_concat3("Manifest is missing a '", req_fields[i]->cstring, "' field."));
    }
  }

  String* req_str_fields[3] = {
    str_main_module,
    str_output,
    str_output_type,
  };
  for (int i = 0; i < 3; ++i) {
    void* raw_value = dictionary_get(manifest, req_str_fields[i]);
    if (!is_string(raw_value)) {
      return wax_manifest_make_error(string_concat3("The manifest field '", req_str_fields[i]->cstring, "'"));
    }
    req_str_fields[i] = (String*) raw_value;
  }

  ProjectManifest* output = new_project_manifest(
    (String*) dictionary_get(manifest, str_output),
    (String*) dictionary_get(manifest, str_output_type));
  String* main_module_name = (String*) dictionary_get(manifest, str_main_module);

  void* raw_module_targets = dictionary_get(manifest, str_module_targets);
  if (!is_list(raw_module_targets)) return wax_manifest_make_error(new_string("Manifest moduleTargets field must be a list."));
  List* module_targets = (List*) raw_module_targets;
  if (module_targets->length == 0) return wax_manifest_make_error(new_string("Manifest moduleTargets list was empty."));
  String* req_str_mod_target_fields[4] = {
    str_name,
    str_src,
    str_lang,
    str_action,
  };

  for (int i = 0; i < module_targets->length; ++i) {
    void* raw_module_target = list_get(module_targets, i);
    if (!is_dictionary(raw_module_target)) return wax_manifest_make_error(new_string("Manifest moduleTargets contains an invalid element."));
    Dictionary* module_target = (Dictionary*) raw_module_target;
    for (int j = 0; j < 4; ++j) {
      String* key = req_str_mod_target_fields[j];
      void* value = dictionary_get(raw_module_target, key);
      if (!is_string(value)) return wax_manifest_make_error(string_concat3("Manifest moduleTargets contains an invalid value for a '", key->cstring, "' field."));
    }
    ModuleMetadata* mm = new_module_metadata(
      dictionary_get(module_target, str_name),
      dictionary_get(module_target, str_src),
      dictionary_get(module_target, str_action),
      dictionary_get(module_target, str_lang));
    if (string_equals(main_module_name, mm->name)) {
      mm->is_main = 1;
    }
    list_add(output->modules, mm);
  }

  return output;
}

ProjectManifest* wax_manifest_load(const char* path) {
  ProjectManifest* manifest = _wax_manifest_load_verifier(path);

  gc_save_item(manifest);
  gc_perform_pass();
  gc_release_item(manifest);

  return manifest;
}
#endif
