#ifndef _RESOURCES_H
#define _RESOURCES_H

#include "../util/strings.h"
#include "../util/dictionaries.h"

// This file is generated

void _resources_populate_dict(Dictionary* dict) {
    StringBuilder* sb = NULL;
    String* name = NULL;
    // GEN_BEGIN
    name = new_string("test_file.txt");
    sb = new_string_builder();
    string_builder_append_chars(sb, "This is a test.\n1 2 3\n");
    dictionary_set(dict, name, string_builder_to_string_and_free(sb));
    // GEN_END
}

String* get_resource(String* name) {
    static Dictionary* lookup = NULL;
    if (lookup == NULL) {
        lookup = new_dictionary();
        _resources_populate_dict(lookup);
    }
    return (String*) dictionary_get(lookup, name);
}
#endif
