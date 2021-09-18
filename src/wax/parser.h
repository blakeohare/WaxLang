#ifndef _WAX_PARSER_H
#define _WAX_PARSER_H

#include <string.h>
#include "../util/gc.h"
#include "../util/strings.h"
#include "../util/lists.h"
#include "../util/dictionaries.h"
#include "../util/primitives.h"
#include "compilercontext.h"
#include "tokens.h"
#include "nodes.h"

int parse_top_level_entity(CompilerContext* ctx);
int parse_class(CompilerContext* ctx);

ConstructorDefinition* parse_constructor(CompilerContext* ctx);
FunctionDefinition* parse_function(CompilerContext* ctx);
FieldDefinition* parse_field(CompilerContext* ctx);
Node* parse_executable(CompilerContext* ctx, int allow_complex, int with_semicolon);
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
    return parse_class(ctx);
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

Node* parse_break(CompilerContext* ctx);
Node* parse_continue(CompilerContext* ctx);
Node* parse_do_while_loop(CompilerContext* ctx);
Node* parse_for_loop(CompilerContext* ctx);
Node* parse_if(CompilerContext* ctx);
Node* parse_return(CompilerContext* ctx);
Node* parse_switch(CompilerContext* ctx);
Node* parse_try(CompilerContext* ctx);
Node* parse_while_loop(CompilerContext* ctx);

Node* parse_executable(CompilerContext* ctx, int allow_complex, int with_semicolon) {
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

  Node* expr = parse_expression(ctx);
  if (expr == NULL) return NULL;

  static Dictionary* assign_ops = NULL;
  if (assign_ops == NULL) {
    assign_ops = new_dictionary();
    gc_save_item(assign_ops);
    List* ops = string_split("= += -= *= /= &= |= ^= **= >>= <<=", " ");
    for (int i = 0; i < ops->length; ++i) {
      dictionary_set(assign_ops, list_get_string(ops, i), wrap_bool(1));
    }
  }

  Node * ex = NULL;
  next = tokens_peek_next_value_no_eof(ctx);
  if (next == NULL) return NULL;
  if (dictionary_has_key(assign_ops, next)) {
    Token* assign_token = tokens_pop(ctx);
    Node* assigned_expr = parse_expression(ctx);
    if (assigned_expr == NULL) return NULL;
    ex = (Node*) new_assignment(expr, assign_token, assigned_expr);
  } else {
    ex = (Node*) new_expression_as_executable(expr);
  }

  if (with_semicolon) {
    if (tokens_pop_expected(ctx, ";") == NULL) return NULL;
  }

  return ex;
}

Node* parse_if(CompilerContext* ctx) {
  Token* if_token = tokens_pop_expected(ctx, "if");
  if (if_token == NULL) return NULL;
  if (tokens_pop_expected(ctx, "(") == NULL) return NULL;
  Node* condition = parse_expression(ctx);
  if (condition == NULL) return NULL;
  if (tokens_pop_expected(ctx, ")") == NULL) return NULL;

  List* true_code = new_list();
  if (!parse_code_block(ctx, true_code, 0)) return NULL;
  List* false_code = new_list();
  if (tokens_pop_if_next(ctx, "else")) {
    if (!parse_code_block(ctx, false_code, 0)) return NULL;
  }
  return new_if_statement(if_token, condition, true_code, false_code);
}

Node* parse_break(CompilerContext* ctx) { parser_error_next_chars(ctx, "NOT IMPLEMENTED: parse_break"); return NULL; }
Node* parse_continue(CompilerContext* ctx) { parser_error_next_chars(ctx, "NOT IMPLEMENTED: parse_continue"); return NULL; }
Node* parse_do_while_loop(CompilerContext* ctx) { parser_error_next_chars(ctx, "NOT IMPLEMENTED: parse_do_while_loop"); return NULL; }
Node* parse_for_loop(CompilerContext* ctx) { parser_error_next_chars(ctx, "NOT IMPLEMENTED: parse_for_loop"); return NULL; }
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
      Node* exec = parse_executable(ctx, 1, 1);
      if (exec == NULL) return 0;
      list_add(code, exec);
    }
  } else {
    Node* exec = parse_executable(ctx, 1, 1);
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

Node* parse_expr_ternary(CompilerContext* ctx);
Node* parse_expr_boolean_combination(CompilerContext* ctx);
Node* parse_expr_bitwise(CompilerContext* ctx);
Node* parse_expr_equality(CompilerContext* ctx);
Node* parse_expr_inequality(CompilerContext* ctx);
Node* parse_expr_bitshift(CompilerContext* ctx);
Node* parse_expr_addition(CompilerContext* ctx);
Node* parse_expr_multiplication(CompilerContext* ctx);
Node* parse_expr_postprefix(CompilerContext* ctx);
Node* parse_expr_entity_with_suffix(CompilerContext* ctx);
Node* parse_expr_entity(CompilerContext* ctx);

Node* parse_expression(CompilerContext* ctx) {
  return parse_expr_ternary(ctx);
}

Node* parse_expr_ternary(CompilerContext* ctx) {
  Node* expr = parse_expr_boolean_combination(ctx);
  if (expr == NULL) return NULL;
  if (tokens_is_next(ctx, "?")) {
    parser_error_chars(ctx, tokens_peek_next(ctx), "NOT IMPLEMENTED! ternary");
    return NULL;
  }
  return expr;
}

Node* parse_expr_boolean_combination(CompilerContext* ctx) {
  static String* op_and = NULL;
  static String* op_or = NULL;
  if (op_and == NULL) {
    op_and = new_common_string("&&");
    op_or = new_common_string("||");
  }
  
  Node* expr = parse_expr_bitwise(ctx);
  if (expr == NULL) return NULL;
  if (tokens_is_next_str(ctx, op_and) || tokens_is_next_str(ctx, op_or)) {
    parser_error_chars(ctx, tokens_peek_next(ctx), "NOT IMPLEMENTED: && and ||");
    return NULL;
  }
  return expr;
}

Node* parse_expr_bitwise(CompilerContext* ctx) {
  static String* op_and = NULL;
  static String* op_or = NULL;
  static String* op_xor = NULL;
  if (op_and == NULL) {
    op_and = new_common_string("&");
    op_or = new_common_string("|");
    op_xor = new_common_string("^");
  }
  Node* expr = parse_expr_equality(ctx);
  if (expr == NULL) return NULL;
  if (tokens_is_next_str(ctx, op_and) || tokens_is_next_str(ctx, op_or) || tokens_is_next_str(ctx, op_xor)) {
    parser_error_chars(ctx, tokens_peek_next(ctx), "NOT IMPLEMENTED: & and | and ^");
    return NULL;
  }
  return expr;
}

Node* parse_expr_equality(CompilerContext* ctx) {
  static String* op_equals = NULL;
  static String* op_neq = NULL;
  if (op_equals == NULL) {
    op_equals = new_common_string("==");
    op_neq = new_common_string("!=");
  }
  Node* expr = parse_expr_inequality(ctx);
  if (expr == NULL) return NULL;
  if (tokens_is_next_str(ctx, op_equals) || tokens_is_next_str(ctx, op_neq)) {
    parser_error_chars(ctx, tokens_peek_next(ctx), "NOT IMPLEMENTED: == and !=");
    return NULL;
  }
  return expr;
}

Node* parse_expr_inequality(CompilerContext* ctx) {
  static String* op_lt = NULL;
  static String* op_gt = NULL;
  static String* op_lteq = NULL;
  static String* op_gteq = NULL;
  if (op_lt == NULL) {
    op_lt = new_common_string("<");
    op_gt = new_common_string(">");
    op_lteq = new_common_string("<=");
    op_gteq = new_common_string(">=");
  }
  Node* expr = parse_expr_bitshift(ctx);
  if (expr == NULL) return NULL;
  if (tokens_is_next_str(ctx, op_lt) ||
      tokens_is_next_str(ctx, op_gt) ||
      tokens_is_next_str(ctx, op_lteq) ||
      tokens_is_next_str(ctx, op_gteq)) {
    parser_error_chars(ctx, tokens_peek_next(ctx), "NOT IMPLEMENTED: < > <= and >=");
    return NULL;
  }
  return expr;
}

Node* parse_expr_bitshift(CompilerContext* ctx) {
  static String* op_left = NULL;
  static String* op_right = NULL;
  if (op_left == NULL) {
    op_left = new_common_string("<<");
    op_right = new_common_string(">>");
  }
  Node* expr = parse_expr_addition(ctx);
  if (expr == NULL) return NULL;
  if (tokens_is_next_str(ctx, op_left) || tokens_is_next_str(ctx, op_right)) {
    parser_error_chars(ctx, tokens_peek_next(ctx), "NOT IMPLEMENTED: << and >>");
    return NULL;
  }
  return expr;
}

Node* parse_expr_addition(CompilerContext* ctx) {
  static String* op_add = NULL;
  static String* op_sub = NULL;
  if (op_add == NULL) {
    op_add = new_common_string("+");
    op_sub = new_common_string("-");
  }
  Node* expr = parse_expr_multiplication(ctx);
  if (expr == NULL) return NULL;
  if (tokens_is_next_str(ctx, op_add) || tokens_is_next_str(ctx, op_sub)) {
    parser_error_chars(ctx, tokens_peek_next(ctx), "NOT IMPLEMENTED: + and -");
    return NULL;
  }
  return expr;
}

Node* parse_expr_multiplication(CompilerContext* ctx) {
  static String* op_mul = NULL;
  static String* op_div = NULL;
  static String* op_mod = NULL;
  if (op_mul == NULL) {
    op_mul = new_common_string("*");
    op_div = new_common_string("/");
    op_mod = new_common_string("%");
  }
  Node* expr = parse_expr_postprefix(ctx);
  if (expr == NULL) return NULL;
  if (tokens_is_next_str(ctx, op_mul) || 
      tokens_is_next_str(ctx, op_div) || 
      tokens_is_next_str(ctx, op_mod)) {
    parser_error_chars(ctx, tokens_peek_next(ctx), "NOT IMPLEMENTED: * / and %");
    return NULL;
  }
  return expr;
}

Node* parse_expr_postprefix(CompilerContext* ctx) {
  static String* op_pp = NULL;
  static String* op_mm = NULL;
  static String* op_neg = NULL;
  static String* op_not = NULL;
  if (op_pp == NULL) {
    op_pp = new_common_string("++");
    op_mm = new_common_string("--");
    op_neg = new_common_string("-");
    op_not = new_common_string("!");
  }

  if (tokens_is_next_str(ctx, op_pp) || tokens_is_next_str(ctx, op_mm)) {
    parser_error_chars(ctx, tokens_peek_next(ctx), "NOT IMPLEMENTED: ++ and -- prefix");
    return NULL;
  }

  if (tokens_is_next_str(ctx, op_neg)) {
    parser_error_chars(ctx, tokens_peek_next(ctx), "NOT IMPLEMENTED: negative sign");
    return NULL;
  }

  if (tokens_is_next_str(ctx, op_not)) {
    parser_error_chars(ctx, tokens_peek_next(ctx), "NOT IMPLEMENTED: boolean not");
    return NULL;
  }

  Node* expr = parse_expr_entity_with_suffix(ctx);
  if (expr == NULL) return NULL;
  while (tokens_is_next_str(ctx, op_pp) || tokens_is_next_str(ctx, op_mm)) {
    parser_error_chars(ctx, tokens_peek_next(ctx), "NOT IMPLEMENTED: ++ and -- suffix");
    return NULL;
  }

  return expr;
}

Node* parse_expr_entity_with_suffix(CompilerContext* ctx) {
  Node* expr = parse_expr_entity(ctx);
  if (expr == NULL) return NULL;
  int keep_looking = 1;
  String* next = tokens_peek_next_value(ctx);
  while (next != NULL && next->length == 1) {
    switch (next->cstring[0]) {
      case '.':
        {
          Token* dot_token = tokens_pop(ctx);
          Token* field_token = tokens_pop(ctx);
          if (field_token == NULL) return NULL;
          if (!token_is_name(field_token)) {
            parser_error(ctx, field_token, string_concat3("Expected a field name but found '", field_token->value->cstring, "'."));
            return NULL;
          }
          expr = (Node*) new_dot_field(expr, dot_token, field_token);
        }
        break;
      case '[':
        parser_error_chars(ctx, tokens_peek_next(ctx), "NOT IMPLEMENTED: bracket index");
        return NULL;
      case '(':
        {
          Token* open_paren = tokens_pop(ctx);
          List* args = new_list();
          while (!tokens_pop_if_next(ctx, ")")) {
            if (args->length > 0) {
              if (tokens_pop_expected(ctx, ",") == NULL) return NULL;
            }
            Node* arg = parse_expression(ctx);
            if (arg == NULL) return NULL;
            list_add(args, arg);
          }
          expr = (Node*) new_function_invocation(expr, open_paren,  args);
        }
        break;
      default:
        return expr;
    }
    next = tokens_peek_next_value(ctx);
  }
  return expr;
}

String* parse_string_value(CompilerContext* ctx, Token* throw_token, String* string_token_value);
int parse_integer_value(CompilerContext* ctx, Token* throw_token, String* raw_value, int* value_out);

Node* parse_expr_entity(CompilerContext* ctx) {

  static String* str_true = NULL;
  static String* str_false = NULL;
  static String* str_null = NULL;
  if (str_true == NULL) {
    str_true = new_common_string("true");
    str_false = new_common_string("false");
    str_null = new_common_string("null");
  }

  if (tokens_is_next(ctx, "(")) {
    tokens_pop(ctx);
    Node* expr = parse_expression(ctx);
    if (tokens_pop_expected(ctx, ")") == NULL) return NULL;
  }

  if (!tokens_ensure_not_eof(ctx)) return NULL;
  Token* next_token = tokens_peek_next_no_eof(ctx);
  String* next = next_token->value;

  switch (next->cstring[0]) {
    case 't': 
      if (string_equals(next, str_true)) {
        parser_error_chars(ctx, tokens_peek_next(ctx), "TODO: true bool constant");
      }
      break;
    case 'f': 
      if (string_equals(next, str_false)) {
        parser_error_chars(ctx, tokens_peek_next(ctx), "TODO: false bool constant");
      }
      break;
    case 'n': 
      if (string_equals(next, str_null)) {
        parser_error_chars(ctx, tokens_peek_next(ctx), "TODO: null constant");
      }
      break;
    case '[':
      parser_error_chars(ctx, next_token, "TODO: inline list definitions");
      return NULL;
    case '{': {
        Token* open_curly_brace = tokens_pop(ctx);
        List* keys = new_list();
        List* values = new_list();
        int allow_next = 1;
        while (!tokens_pop_if_next(ctx, "}")) {
          if (!allow_next) {
            tokens_pop_expected(ctx, "}"); // pushes error
            return NULL;
          }
          if (!tokens_ensure_not_eof(ctx)) return NULL;
          Node* key = parse_expr_entity(ctx);
          if (!string_equals_chars(key->type, NODE_STRING_CONSTANT_NAME)) {
            parser_error_chars(ctx, next_token, "Only a string can be the key of a dictionary definition.");
            return NULL;
          }
          list_add(keys, key);
          if (tokens_pop_expected(ctx, ":") == NULL) return NULL;
          Node* value = parse_expression(ctx);
          if (value == NULL) return NULL;
          list_add(values, value);

          allow_next = tokens_pop_if_next(ctx, ",");
        }
        return new_inline_dictionary(open_curly_brace, keys, values);
      }
  }

  if (next_token->type == TOKEN_TYPE_FLOAT) {
    parser_error_chars(ctx, next_token, "TODO: floats");
    return NULL;
  }

  if (next_token->type == TOKEN_TYPE_INTEGER) {
    tokens_pop(ctx);
    int value;
    if (!parse_integer_value(ctx, next_token, next_token->value, &value)) return NULL;
    return new_integer_constant(next_token, value);
  }

  if (next_token->type == TOKEN_TYPE_STRING) {
    tokens_pop(ctx);
    String* str_value = parse_string_value(ctx, next_token, next);
    return new_string_constant(next_token, str_value);
  }

  if (next_token->type == TOKEN_TYPE_WORD) {
    tokens_pop(ctx);
    return (Node*) new_variable(next_token, next);
  }

  parser_error(ctx, next_token, string_concat3("Unexpected token :'", next->cstring, "'."));
  return NULL;
}

int parse_integer_value(CompilerContext* ctx, Token* throw_token, String* raw_value, int* value_out) {
  int output = 0;
  int sign = 1;
  char* str = raw_value->cstring;
  int start = 0;
  int len = raw_value->length;
  if (str[0] == '-') {
    sign = -1;
    start = 1;
  }
  int is_hex = 0;
  for (int i = start; i < len; i++) {
    char c = str[i];
    if ((c == 'x' || c == 'X') && i - start == 1 && output == 0) {
      is_hex = 1;
    } else {
      int digit_value = -1;
      if (c >= '0' && c <= '9') {
        digit_value = c - '0';
      } else if (c >= 'A' && c <= 'F') {
        digit_value = c - 'A' + 10;
      } else if (c >= 'a' && c <= 'f') {
        digit_value = c - 'a' + 10;
      } else {
        parser_error(ctx, throw_token, string_concat3("Invalid integer constant: '", raw_value->cstring, "'."));
        return NULL;
      }

      if (digit_value >= 10 && !is_hex) {
        parser_error(ctx, throw_token, "Invalid integer constant. If this is a hexadecimal number, it must start with a '0x' prefix");
        return NULL;
      }
      output = output * (is_hex ? 16 : 10) + digit_value;
    }
  }
  *value_out = output * sign;
}

String* parse_string_value(CompilerContext* ctx, Token* throw_token, String* string_token_value) {
  StringBuilder* sb = new_string_builder();
  const char* str = string_token_value->cstring;
  int len = string_token_value->length - 1;
  for (int i = 1; i < len; ++i) {
    char c = str[i];
    if (c == '\\') {
      if (i + 1 < len) {
        parser_error_chars(ctx, throw_token, "String ends with an unescaped backslash");
        return NULL;
      }
      c = str[++i];
      switch (c) {
        case 'n': c = '\n'; break;
        case 'r': c = '\r'; break;
        case 't': c = '\t'; break;
        case '"': c = '"'; break;
        case '\'': c = '\''; break;
        case '0': c = '\0'; break;
        case '\\': c = '\\'; break;
        default:
          {
            const char c2[2] = { c, '\0' };
            parser_error(ctx, throw_token, string_concat("Invalid string escape sequence: \\", c2));
          }
          return NULL;
      }
    }
    string_builder_append_char(sb, c);
  }
  return string_builder_to_string_and_free(sb);
}

#endif
