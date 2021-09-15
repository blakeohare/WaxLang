#ifndef _WAX_PARSER_H
#define _WAX_PARSER_H

#include "compilercontext.h"
#include "tokens.h"
#include "nodes.h"

int parse_top_level_entity(CompilerContext* ctx);
int parse_class(CompilerContext* ctx);

ConstructorDefinition* parse_constructor(CompilerContext* ctx);
FunctionDefinition* parse_function(CompilerContext* ctx);
FieldDefinition* parse_field(CompilerContext* ctx);

void parse_first_pass(CompilerContext* ctx) {
  ctx->has_error = 0; // keep parsing these fresh tokens even if there's a syntax error in previous files
  while (tokens_has_more(ctx)) {
    if (!parse_top_level_entity(ctx)) {
      return;
    }
  }
}

int parse_top_level_entity(CompilerContext* ctx) {
  String* next = tokens_peek_next_value(ctx);
  if (string_equals_chars(next, "class")) {
    return parse_class(ctx);
  }
  if (string_equals_chars(next, "function")) {
    FunctionDefinition* fd = parse_function(ctx);
    list_add(ctx->function_definitions, fd);
  }

  tokens_pop_expected(ctx, "function"); // throws
  return 0;
}

int parse_class(CompilerContext* ctx) {
  Token* first_token = tokens_pop_expected(ctx, "class");
  if (first_token == NULL) return 0;

  Token* name_token = tokens_pop(ctx);
  if (name_token == NULL) return 0;

  if (!token_is_name(name_token)) {
    return parser_error(ctx, name_token, string_concat3(
      "Expected a class name but found '", name_token->value->cstring, "' instead."));
  }

  ClassDefinition* class_def = new_class_definition(first_token, name_token);

  if (tokens_pop_expected(ctx, "{") == NULL) return 0;

  String* str_function = new_string("function");
  String* str_constructor = new_string("constructor");
  String* str_field = new_string("field");

  while (!tokens_pop_if_next(ctx, "}")) {
    String* next = tokens_peek_next_value_no_eof(ctx);
    Node* member = NULL;
    String* member_name = NULL;
    if (next == NULL) {
      return 0;
    }
    
    Token* next_token = tokens_peek_next(ctx);
    if (string_equals(next, str_function)) {
      FunctionDefinition* fd = parse_function(ctx);
      if (fd != NULL) member_name = fd->function_name->value;
      member = (Node*) fd;
    } else if (string_equals(next, str_field)) {
      FieldDefinition* fd = parse_field(ctx);
      if (fd != NULL) member_name = fd->field_name->value;
      member = (Node*) fd;
    } else if (string_equals(next, str_constructor)) {
      ConstructorDefinition* ctor = parse_constructor(ctx);
      if (ctor != NULL) member_name = new_string("[ctor]");
      member = (Node*) ctor;
    } else {
      return parser_error(ctx, tokens_peek_next(ctx), new_string("Unexpected token in class"));
    }

    if (member == NULL) {
      return parser_error_chars(ctx, next_token, "Unexpected token inside class. Only function, constructor, and field definitions can exist in a class.");
    }

    if (dictionary_has_key(class_def->members, member_name)) {
      return parser_error(ctx, next_token, string_concat3("This class already has a member named '", member_name->cstring, "'."));
    }

    dictionary_set(class_def->members, member_name, member);
    list_add(class_def->member_order, member_name);
  }

  list_add(ctx->class_definitions, class_def);
  return 1;
}

FunctionDefinition* parse_function(CompilerContext* ctx) {
  parser_error_chars(ctx, tokens_peek_next(ctx), "parse_function NOT IMPLEMENTED");
  return NULL;
}

FieldDefinition* parse_field(CompilerContext* ctx) {
  parser_error_chars(ctx, tokens_peek_next(ctx), "parse_field NOT IMPLEMENTED");
  return NULL;
}

ConstructorDefinition* parse_constructor(CompilerContext* ctx) {
  parser_error_chars(ctx, tokens_peek_next(ctx), "parse_constructor NOT IMPLEMENTED");
  return NULL;
}

#endif
