#ifndef _WAX_COMPILERCONTEXT_H
#define _WAX_COMPILERCONTEXT_H

#include <string.h>
#include "../util/gcbase.h"
#include "../util/lists.h"
#include "tokens.h"

typedef struct _CompilerContext {
  List* error_messages;
  List* error_tokens;
  TokenStream* tokens;
  List* class_definitions;
  List* function_definitions;
  int has_error;
} CompilerContext;

#define COMPILER_CONTEXT_GC_FIELD_COUNT 5
#define COMPILER_CONTEXT_NAME "CompilerContext"

CompilerContext* new_compiler_context() {
  CompilerContext* ctx = (CompilerContext*)gc_create_struct(sizeof(CompilerContext), COMPILER_CONTEXT_NAME, COMPILER_CONTEXT_GC_FIELD_COUNT);
  ctx->has_error = 0;
  ctx->error_messages = new_list();
  ctx->error_tokens = new_list();
  ctx->tokens = NULL;
  ctx->class_definitions = new_list();
  ctx->function_definitions = new_list();
  return ctx;
}

int parser_error(CompilerContext* ctx, Token* token, String* msg) {
  list_add(ctx->error_messages, msg);
  list_add(ctx->error_tokens, token);
  ctx->has_error = 1;
  return 0;
}

int parser_error_chars(CompilerContext* ctx, Token* token, const char* msg) {
  return parser_error(ctx, token, new_string(msg));
}

int tokens_has_more(CompilerContext* ctx) {
  if (ctx->tokens->index < ctx->tokens->length) return 1;
  return 0;
}

Token* tokens_peek_next(CompilerContext* ctx) {
  if (ctx->tokens->index < ctx->tokens->length) {
    return (Token*) list_get(ctx->tokens->tokens, ctx->tokens->index);
  }
  return NULL;
}

int parser_error_next_chars(CompilerContext* ctx, const char* msg) {
  return parser_error_chars(ctx, tokens_has_more(ctx) ? tokens_peek_next(ctx) : NULL, msg);
}

String* tokens_peek_next_value(CompilerContext* ctx) {
  Token* t = tokens_peek_next(ctx);
  if (t == NULL) return NULL;
  return t->value;
}

Token* tokens_pop(CompilerContext* ctx) {
  if (tokens_has_more(ctx)) {
    return (Token*) list_get(ctx->tokens->tokens, ctx->tokens->index++);
  } else {
    parser_error_chars(ctx, NULL, "Unexpected EOF");
    return NULL;
  }
}

Token* tokens_pop_expected(CompilerContext* ctx, const char* value) {
  Token* token = tokens_pop(ctx);
  if (token == NULL) {
    list_pop(ctx->error_messages);
    list_pop(ctx->error_tokens);
    parser_error(ctx, NULL, string_concat3("Expected '", value, "' but found End of File instead"));
    return NULL;
  }
  if (string_equals_chars(token->value, value)) {
    return token;
  }
  parser_error(ctx, token, string_concat5("Expected '", value, "' but found '", token->value->cstring, "' instead."));
  return NULL;
}

int tokens_ensure_not_eof(CompilerContext* ctx) {
  if (tokens_has_more(ctx)) return 1;
  tokens_pop(ctx); // adds an error
  return 0;
}

String* tokens_peek_next_value_no_eof(CompilerContext* ctx) {
  String* next = tokens_peek_next_value(ctx);
  if (next == NULL) tokens_ensure_not_eof(ctx);
  return next;
}

int tokens_pop_if_next(CompilerContext* ctx, const char* token) {
  String* next = tokens_peek_next_value(ctx);
  if (next != NULL && string_equals_chars(next, token)) {
    ctx->tokens->index++;
    return 1;
  }
  return 0;
}

int tokens_is_next(CompilerContext* ctx, const char* token) {
  String* next = tokens_peek_next_value(ctx);
  if (next == NULL) return 0;
  return strcmp(token, next->cstring) == 0 ? 1 : 0;
}

#endif
