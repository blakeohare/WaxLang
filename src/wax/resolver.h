#ifndef _WAX_RESOLVER_H
#define _WAX_RESOLVER_H

#include <stdio.h>
#include "../util/dictionaries.h"
#include "../util/lists.h"
#include "compilercontext.h"
#include "nodes.h"

typedef struct _ResolverContext {
  Dictionary* classes_by_name;
  Dictionary* functions_by_name;
  CompilerContext* ctx;
} ResolverContext;

int wax_resolve_class(ResolverContext* rctx, ClassDefinition* cd);
int wax_resolve_function(ResolverContext* rctx, FunctionDefinition* cd);
int wax_resolve_executable(ResolverContext* rctx, Node* line, List* code_out);
Node* wax_resolve_expression(ResolverContext* rctx, Node* expression);

void wax_resolve_module(CompilerContext* ctx) {

  ResolverContext rctx;
  rctx.ctx = ctx;
  rctx.classes_by_name = new_dictionary();
  rctx.functions_by_name = new_dictionary();

  // Create lookups for classes and functions
  for (int i = 0; i < ctx->class_definitions->length; ++i) {
    ClassDefinition* class_def = (ClassDefinition*) list_get(ctx->class_definitions, i);
    String* name = class_def->class_name->value;
    if (dictionary_has_key(rctx.classes_by_name, name)) {
      parser_error(ctx, class_def->class_name, string_concat3("There are multiple classes named '", name->cstring, "'."));
    }
    dictionary_set(rctx.classes_by_name, name, class_def);
  }
  for (int i = 0; i < ctx->class_definitions->length; ++i) {
    FunctionDefinition* func_def = (FunctionDefinition*) list_get(ctx->function_definitions, i);
    String* name = func_def->function_name->value;
    if (dictionary_has_key(rctx.functions_by_name, name)) {
      parser_error(ctx, func_def->function_name, string_concat3("There are multiple functions named '", name->cstring, "'."));
    }
    dictionary_set(rctx.functions_by_name, name, func_def);
  }

  // Resolve base class references
  for (int i = 0; i < ctx->class_definitions->length; ++i) {
    ClassDefinition* class_def = (ClassDefinition*) list_get(ctx->class_definitions, i);
    if (class_def->base_class_token != NULL) {
      String* name = class_def->base_class_token->value;
      ClassDefinition* base_class = dictionary_get(rctx.classes_by_name, name);
      if (base_class == NULL) {
        parser_error(ctx, class_def->base_class_token, string_concat3("There is no class named '", name->cstring, "'."));
      }
    }
  }

  // If the metadata names are fundamentally wrong, then don't move on.
  if (ctx->has_error) return;

  for (int i = 0; i < ctx->class_definitions->length; ++i) {
    wax_resolve_class(&rctx, (ClassDefinition*) list_get(ctx->class_definitions, i));
    if (ctx->has_error) return;
  }

  for (int i = 0; i < ctx->function_definitions->length; ++i) {
    wax_resolve_function(&rctx, (FunctionDefinition*) list_get(ctx->function_definitions, i));
    if (ctx->has_error) return;
  }
}

int wax_resolve_class(ResolverContext* rctx, ClassDefinition* cd) {
  parser_error_chars(rctx->ctx, cd->node.first_token, "TODO: class resolver");
  return 0;
}

List* wax_resolve_code_block(ResolverContext* rctx, List* code, int* ok);

int wax_resolve_function(ResolverContext* rctx, FunctionDefinition* func_def) {
  // resolve argument names and default values
  int arg_count = func_def->arg_tokens->length;
  Dictionary* arg_lookup = new_dictionary(); // name collision check
  for (int i = 0; i < arg_count; ++i) {
    Token* name_token = (Token*) list_get(func_def->arg_tokens, i);
    String* name = name_token->value;
    Node* default_value = (Node*) list_get(func_def->arg_default_values, i);
    if (dictionary_has_key(arg_lookup, name)) {
      parser_error(rctx->ctx, name_token, string_concat3("There are multiple arguments for this function named '", name->cstring, "'."));
    } else if (dictionary_has_key(rctx->classes_by_name, name)) {
      parser_error(rctx->ctx, name_token, string_concat3("The argument name '", name->cstring, "' collides with a class definition name."));
    } else if (dictionary_has_key(rctx->functions_by_name, name)) {
      parser_error(rctx->ctx, name_token, string_concat3("The argument name '", name->cstring, "' collides with a function definition name."));
    }

    if (default_value != NULL) {
      list_set(func_def->arg_default_values, i, wax_resolve_expression(rctx, default_value));
    }
  }

  int ok;
  func_def->code = wax_resolve_code_block(rctx, func_def->code, &ok);
  return ok;
}

List* wax_resolve_code_block(ResolverContext* rctx, List* code, int* ok) {
  *ok = 1;
  List* new_code = new_list();
  for (int i = 0; i < code->length; ++i) {
    Node* line = (Node*) list_get(code, i);
    if (wax_resolve_executable(rctx, line, new_code) == 0) {
      *ok = 0;
    }
  }
  return new_code;
}

int wax_resolver_is_assignable(Node* expr) {
  static String* expr_dot_field = NULL;
  static String* expr_variable = NULL;
  static String* expr_bracket_index = NULL;
  if (expr_dot_field == NULL) {
    expr_dot_field = new_common_string(NODE_DOT_FIELD_NAME);
    expr_variable = new_common_string(NODE_VARIABLE_NAME);
    expr_bracket_index = new_common_string(NODE_BRACKET_INDEX_NAME);
  }
  if (string_equals(expr->type, expr_dot_field)) return 1;
  if (string_equals(expr->type, expr_variable)) return 1;
  if (string_equals(expr->type, expr_bracket_index)) return 1;
  return 0;
}

int wax_resolve_executable(ResolverContext* rctx, Node* line, List* code_out) {
  static String* str_assignment = NULL;
  static String* str_exec_expr = NULL;
  static String* str_foreach_loop = NULL;
  static String* str_if = NULL;
  if (str_if == NULL) {
    str_assignment = new_common_string(NODE_ASSIGNMENT_NAME);
    str_exec_expr = new_common_string(NODE_EXPR_EXEC_NAME);
    str_foreach_loop = new_common_string(NODE_FOR_EACH_LOOP_NAME);
    str_if = new_common_string(NODE_IF_STATEMENT_NAME);
  }
  String* type = line->type;
  Node* expr = NULL;
  int keep = 0;
  int found = 0;
  switch (type->cstring[0]) {
    case 'A':
      // Assignment
      if (string_equals(type, str_assignment)) {
        found = 1;
        Assignment* asgn = (Assignment*) line;
        asgn->target = wax_resolve_expression(rctx, asgn->target);
        if (asgn->target == NULL) return NULL;

        asgn->value = wax_resolve_expression(rctx, asgn->value);
        if (asgn->value == NULL) return NULL;

        if (!wax_resolver_is_assignable(asgn->target)) {
          parser_error_chars(rctx->ctx, asgn->target->first_token, "Cannot assign to this type of expression.");
          return NULL;
        }
        keep = 1;
      }
      break;
    case 'E':
      if (string_equals(type, str_exec_expr)) {
        // Expression as Executable
        found = 1;
        ExpressionAsExecutable* ee = (ExpressionAsExecutable*) line;
        ee->expression = wax_resolve_expression(rctx, ee->expression);
        if (ee->expression == NULL) return 0;
        keep = 1;
      }
      break;
    case 'F':
      if (string_equals(type, str_foreach_loop)) {
        // Foreach Loop
        found = 1;
        ForEachLoop* fel = (ForEachLoop*) line;
        fel->list_expr = wax_resolve_expression(rctx, fel->list_expr);
        if (fel->list_expr == NULL) return 0;
        int ok;
        fel->code = wax_resolve_code_block(rctx, fel->code, &ok);
        if (!ok) return 0;
        keep = 1;
      }
      break;
    case 'I':
      if (string_equals(type, str_if)) {
        // If Statement
        found = 1;
        IfStatement* _if = (IfStatement*) line;
        _if->condition = wax_resolve_expression(rctx, _if->condition);
        if (_if->condition == NULL) return 0;
        int ok;
        _if->true_code = wax_resolve_code_block(rctx, _if->true_code, &ok);
        if (!ok) return 0;
        _if->false_code = wax_resolve_code_block(rctx, _if->false_code, &ok);
        if (!ok) return 0;
        keep = 1;
        break;
      }
      break;
  }

  if (keep) list_add(code_out, line);

  if (!found) {
    parser_error(rctx->ctx, line->first_token, string_concat("No executable resolver for: ", type->cstring));
    return 0;
  }

  return 1;
}

Node* wax_resolve_bracket_index(ResolverContext* rctx, BracketIndex* bi);
Node* wax_resolve_dot_token(ResolverContext* rctx, DotField* df);
Node* wax_resolve_function_invocation(ResolverContext* rctx, FunctionInvocation* fi);
Node* wax_resolve_inline_dictionary(ResolverContext* rctx, InlineDictionary* dict);
Node* wax_resolve_op_chain(ResolverContext* rctx, OpChain* oc);
Node* wax_resolve_ternary(ResolverContext* rctx, Ternary* ter);

Node* wax_resolve_expression(ResolverContext* rctx, Node* expression) {
  static String* str_boolean_constant = NULL;
  static String* str_bracket_index = NULL;
  static String* str_dot_field = NULL;
  static String* str_inline_dictionary = NULL;
  static String* str_integer_constant = NULL;
  static String* str_function_invocation = NULL;
  static String* str_null_constant = NULL;
  static String* str_op_chain = NULL;
  static String* str_string_constant = NULL;
  static String* str_ternary = NULL;
  static String* str_variable = NULL;
  if (str_boolean_constant == NULL) {
    str_boolean_constant = new_common_string(NODE_BOOLEAN_CONSTANT_NAME);
    str_bracket_index = new_common_string(NODE_BRACKET_INDEX_NAME);
    str_dot_field = new_common_string(NODE_DOT_FIELD_NAME);
    str_inline_dictionary = new_common_string(NODE_INLINE_DICTIONARY_NAME);
    str_integer_constant = new_common_string(NODE_INTEGER_CONSTANT_NAME);
    str_function_invocation = new_common_string(NODE_FUNCTION_INVOCATION_NAME);
    str_null_constant = new_common_string(NODE_NULL_CONSTANT_NAME);
    str_op_chain = new_common_string(NODE_OP_CHAIN_NAME);
    str_string_constant = new_common_string(NODE_STRING_CONSTANT_NAME);
    str_ternary = new_common_string(NODE_TERNARY_NAME);
    str_variable = new_common_string(NODE_VARIABLE_NAME);
  }
  String* type = expression->type;
  switch (type->cstring[0]) {
    case 'B':
      if (string_equals(type, str_boolean_constant)) return expression; // nothing to resolve
      if (string_equals(type, str_bracket_index)) return wax_resolve_bracket_index(rctx, (BracketIndex*) expression);
      break;
    case 'D':
      if (string_equals(type, str_dot_field)) return wax_resolve_dot_token(rctx, (DotField*) expression);
      break;
    case 'F':
      if (string_equals(type, str_function_invocation)) return wax_resolve_function_invocation(rctx, (FunctionInvocation*) expression);
      break;
    case 'I':
      if (string_equals(type, str_integer_constant)) return expression; // nothing to resolve
      if (string_equals(type, str_inline_dictionary)) return wax_resolve_inline_dictionary(rctx, (InlineDictionary*) expression); // nothing to resolve
      break;
    case 'N':
      if (string_equals(type, str_null_constant)) return expression; // nothing to resolve
      break;
    case 'O':
      if (string_equals(type, str_op_chain)) return wax_resolve_op_chain(rctx, (OpChain*) expression);
      break;
    case 'S':
      if (string_equals(type, str_string_constant)) return expression; // nothing to resolve
      break;
    case 'T':
      if (string_equals(type, str_ternary)) return wax_resolve_ternary(rctx, (Ternary*) expression);
      break;
    case 'V':
      if (string_equals(type, str_variable)) return expression; // nothing to resolve...yet
      break;
  }
  parser_error(rctx->ctx, expression->first_token, string_concat("No expression resolver for: ", type->cstring));
  return NULL;
}

Node* wax_resolve_bracket_index(ResolverContext* rctx, BracketIndex* bi) {
  if ((bi->root = wax_resolve_expression(rctx, bi->root)) == NULL) return NULL;
  if ((bi->index = wax_resolve_expression(rctx, bi->index)) == NULL) return NULL;
  return (Node*) bi;
}

Node* wax_resolve_dot_token(ResolverContext* rctx, DotField* df) {
  if ((df->root = wax_resolve_expression(rctx, df->root)) == NULL) return NULL;
  return (Node*) df;
}

Node* wax_resolve_function_invocation(ResolverContext* rctx, FunctionInvocation* fi) {
  if ((fi->root = wax_resolve_expression(rctx, fi->root)) == NULL) return NULL;
  for (int i = 0; i < fi->args->length; ++i) {
    Node* arg = list_get(fi->args, i);
    if ((arg = wax_resolve_expression(rctx, arg)) == NULL) return NULL;
    list_set(fi->args, i, arg);
  }
  return (Node*) fi;
}

Node* wax_resolve_inline_dictionary(ResolverContext* rctx, InlineDictionary* dict) {
  static String* str_string_constant = NULL;
  if (str_string_constant == NULL) {
    str_string_constant = new_common_string(NODE_STRING_CONSTANT_NAME);
  }
  int kcount = dict->keys->length;
  Dictionary* collisions = new_dictionary();
  for (int i = 0; i < kcount; ++i) {
    Node* key = list_get(dict->keys, i);
    key = wax_resolve_expression(rctx, key);
    if (key == NULL) return NULL;
    if (!string_equals(key->type, str_string_constant)) {
      parser_error_chars(rctx->ctx, key->first_token, "Only a string can be used as the key of a dictionary.");
      return NULL;
    }
    list_set(dict->keys, i, key);

    if (dictionary_has_key(collisions, key)) {
      parser_error_chars(rctx->ctx, key->first_token, "This dictionary contains a key collisions here.");
      return NULL;
    }
    dictionary_set(collisions, key, key);

    Node* value = list_get(dict->values, i);
    value = wax_resolve_expression(rctx, value);
    if (value == NULL) return NULL;
    list_set(dict->values, i, value);
  }
  return (Node*) dict;
}

Node* wax_resolve_op_chain(ResolverContext* rctx, OpChain* oc) {
  for (int i = 0; i < oc->expressions->length; ++i) {
    Node* ex = (Node*) list_get(oc->expressions, i);
    ex = wax_resolve_expression(rctx, ex);
    if (ex == NULL) return NULL;
    list_set(oc->expressions, i, ex);
  }
  return (Node*) oc;
}

Node* wax_resolve_ternary(ResolverContext* rctx, Ternary* ter) {
  ter->condition = wax_resolve_expression(rctx, ter->condition);
  if (ter->condition == NULL) return NULL;
  ter->true_expr = wax_resolve_expression(rctx, ter->true_expr);
  if (ter->true_expr == NULL) return NULL;
  ter->false_expr = wax_resolve_expression(rctx, ter->false_expr);
  if (ter->false_expr == NULL) return NULL;
  return (Node*) ter;
}

#endif
