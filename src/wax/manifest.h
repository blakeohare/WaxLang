#ifndef _WAX_MANIFEST_H
#define _WAX_MANIFEST_H

#include "../util/fileio.h"
#include "../util/json.h"
#include "../util/dictionaries.h"
#include "../util/lists.h"
#include "../util/strings.h"

Dictionary* wax_manifest_make_error(String* msg)
{
  Dictionary* dict = new_dictionary();
  dictionary_set(dict, new_string("@error"), msg);
  return dict;
}

Dictionary* wax_manifest_load(char* path);
Dictionary* wax_manifest_load(char* path)
{
  String* str_inherit = new_string("inherit");
  String* str_error = new_string("@error");

  if (!file_exists(path))
    return wax_manifest_make_error(string_concat3("Could not find file: '", path, "'"));

  String* file_content = file_read_text(path);
  if (file_content == NULL)
    return wax_manifest_make_error(string_concat3("Error while reading file: '", path, "'"));
    
  JsonParseResult json_result = json_parse(file_content->cstring);
  if (json_result.error)
    return wax_manifest_make_error(string_concat4("JSON error while parsing '", path, "': ", json_get_error(json_result)->cstring));

  if (!is_dictionary(json_result.value))
    return wax_manifest_make_error(string_concat3("JSON error: '", path, "' does not contain an object as its root JSON value."));
  
  Dictionary* manifest = (Dictionary*) json_result.value;
  void* inherit_path = dictionary_get(manifest, str_inherit);
  if (inherit_path != NULL)
  {
    if (!is_string(inherit_path)) return wax_manifest_make_error(string_concat3("Inherit field in '", path, "' does not contain a string."));
    String* canonicalized_inherit_path = normalize_path(string_concat3(path, "/../", ((String*)inherit_path)->cstring)->cstring);
    printf("Loading the parent path of %s\n", canonicalized_inherit_path->cstring);
    Dictionary* parent_manifest = wax_manifest_load(canonicalized_inherit_path->cstring);

    if (dictionary_has_key(parent_manifest, str_error)) return parent_manifest;
    List* keys = dictionary_get_keys(manifest);
    String* str_module_targets = new_string("moduleTargets");
    for (int i = 0; i < keys->length; ++i)
    {
      String* key = list_get_string(keys, i);
      void* value = dictionary_get(manifest, key);
      void* parent_value = dictionary_get(parent_manifest, key);
      
      // for moduleTargets, append the manifest's value after the parent's value
      if (string_equals(key, str_module_targets) && is_list(value) && is_list(parent_value))
      {
        list_push_all(parent_value, value);
        value = parent_value;
      }

      dictionary_set(parent_manifest, key, value);
    }
    manifest = parent_manifest;
  }
  return manifest;
}

#endif
