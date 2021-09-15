#ifndef _WAX_PARSER_H
#define _WAX_PARSER_H

#include <string.h>
#include "compilercontext.h"
#include "tokens.h"
#include "nodes.h"

int parse_top_level_entity(CompilerContext* ctx);
int parse_class(CompilerContext* ctx);

ConstructorDefinition* parse_constructor(CompilerContext* ctx);
FunctionDefinition* parse_function(CompilerContext* ctx);
FieldDefinition* parse_field(CompilerContext* ctx);
Node* parse_executable(CompilerContext* ctx, int allow_complex);
Node* parse_expression(CompilerContext* ctx);

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
    ClassDefinition* cd = parse_class(ctx);
    if (cd == NULL) return 0;
    list_add(ctx->class_definitions, cd);
    return 1;
  }

  if (string_equals_chars(next, "function")) {
    FunctionDefinition* fd = parse_function(ctx);
    if (fd == NULL) return 0;
    list_add(ctx->function_definitions, fd);
    return 1;
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

Node* parse_expression(CompilerContext* ctx) {
  parser_error_chars(ctx, tokens_peek_next(ctx), "NOT IMPLEMENTED: parse_expression");
  return NULL;
}

Node* parse_break(CompilerContext* ctx);
Node* parse_continue(CompilerContext* ctx);
Node* parse_do_while_loop(CompilerContext* ctx);
Node* parse_for_loop(CompilerContext* ctx);
Node* parse_if(CompilerContext* ctx);
Node* parse_return(CompilerContext* ctx);
Node* parse_switch(CompilerContext* ctx);
Node* parse_try(CompilerContext* ctx);
Node* parse_while_loop(CompilerContext* ctx);

Node* parse_executable(CompilerContext* ctx, int allow_complex) {
  String* next = tokens_peek_next_value_no_eof(ctx);
  if (next == NULL) return NULL;

  if (allow_complex) {
    const char* next_chars = next->cstring;
    switch (next_chars[0]) {
      case 'b':
        if (strcmp(next_chars, "break") == 0) return parse_break(ctx);
        break;
      case 'c':
        if (strcmp(next_chars, "continue") == 0) return parse_continue(ctx);
        break;
      case 'd':
        if (strcmp(next_chars, "do") == 0) return parse_do_while_loop(ctx);
        break;
      case 'f':
        if (strcmp(next_chars, "for") == 0) return parse_for_loop(ctx);
        break;
      case 'i':
        if (strcmp(next_chars, "if") == 0) return parse_if(ctx);
        break;
      case 'r':
        if (strcmp(next_chars, "return") == 0) return parse_return(ctx);
        break;
      case 's':
        if (strcmp(next_chars, "switch") == 0) return parse_switch(ctx);
        break;
      case 't':
        if (strcmp(next_chars, "try") == 0) return parse_try(ctx);
        break;
      case 'w':
        if (strcmp(next_chars, "while") == 0) return parse_while_loop(ctx);
        break;
    }
  }
  parser_error_chars(ctx, tokens_peek_next(ctx), "NOT IMPLEMENTED: parse_executable");
  return NULL;
}


Node* parse_break(CompilerContext* ctx) { parser_error_next_chars(ctx, "NOT IMPLEMENTED: parse_break"); return NULL; }
Node* parse_continue(CompilerContext* ctx) { parser_error_next_chars(ctx, "NOT IMPLEMENTED: parse_continue"); }
Node* parse_do_while_loop(CompilerContext* ctx) { parser_error_next_chars(ctx, "NOT IMPLEMENTED: parse_do_while_loop"); return NULL; }
Node* parse_for_loop(CompilerContext* ctx) { parser_error_next_chars(ctx, "NOT IMPLEMENTED: parse_for_loop"); return NULL; }
Node* parse_if(CompilerContext* ctx) { parser_error_next_chars(ctx, "NOT IMPLEMENTED: parse_if"); return NULL; }
Node* parse_return(CompilerContext* ctx) { parser_error_next_chars(ctx, "NOT IMPLEMENTED: parse_return"); return NULL; }
Node* parse_switch(CompilerContext* ctx) { parser_error_next_chars(ctx, "NOT IMPLEMENTED: parse_switch"); return NULL; }
Node* parse_try(CompilerContext* ctx) { parser_error_next_chars(ctx, "NOT IMPLEMENTED: parse_try"); return NULL; }
Node* parse_while_loop(CompilerContext* ctx) { parser_error_next_chars(ctx, "NOT IMPLEMENTED: parse_while_loop"); return NULL; }

int parse_arg_list(CompilerContext* ctx, List* arg_names_out, List* arg_default_values_out) {
  if (tokens_pop_expected(ctx, "(") == NULL) return 0;
  while (!tokens_pop_if_next(ctx, ")")) {
    if (arg_names_out->length > 0) {
      if (!tokens_pop_expected(ctx, ",")) return 0;
    }
    Token* arg_name = tokens_pop(ctx);
    if (!token_is_name(arg_name)) {
      return parser_error(ctx, arg_name, string_concat3("Expected an argument name but found '", arg_name->value->cstring, "' instead."));
    }
    list_add(arg_names_out, arg_name);
    if (tokens_pop_if_next(ctx, "=")) {
      Node* default_value = parse_expression(ctx);
      list_add(arg_default_values_out, default_value);
    } else {
      list_add(arg_default_values_out, NULL);
    }
  }
  return 1;
}

int parse_code_block(CompilerContext* ctx, List* code, int require_curly_brace) {
  if (require_curly_brace || tokens_is_next(ctx, "{")) {
    if (tokens_pop_expected(ctx, "{") == NULL) return 0;
    while (!tokens_pop_if_next(ctx, "}")) {
      Node* exec = parse_executable(ctx, 1);
      if (exec == NULL) return 0;
      list_add(code, exec);
    }
  } else {
    Node* exec = parse_executable(ctx, 1);
    if (exec == NULL) return 0;
    list_add(code, exec);
  }
  return 1;
}

FunctionDefinition* parse_function(CompilerContext* ctx) {
  Token* function_token = tokens_pop_expected(ctx, "function");
  if (function_token == NULL) return NULL;

  Token* function_name = tokens_pop(ctx);
  if (function_name == NULL) return NULL;
  if (!token_is_name(function_name)) {
    parser_error(ctx, function_name, string_concat3("Expected a function name but found '", function_name->value->cstring, "' instead."));
    return NULL;
  }
  
  FunctionDefinition* func_def = new_function_definition(function_token, function_name);

  if (!parse_arg_list(ctx, func_def->arg_tokens, func_def->arg_default_values)) return NULL;
  
  if (!parse_code_block(ctx, func_def->code, 1)) return NULL;

  return func_def;
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
