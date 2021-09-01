#ifndef _UTIL_JSON_H
#define _UTIL_JSON_H

#include <stdio.h>
#include <stdlib.h>
#include "primitives.h"
#include "dictionaries.h"
#include "strings.h"
#include "util.h"
#include "valueutil.h"

#define JSON_OK 0
#define JSON_ERROR_BAD_SYNTAX 1
#define JSON_ERROR_EOF 2
#define JSON_ERROR_OBJECT_KEY_COLLISION 3
#define JSON_ERROR_INVALID_STRING_ESCAPE_SEQUENCE 4

typedef struct _JsonParseContext {
  char* str;
  int index;
  int length;
  int line;
  int col;
  int error_code;
  int error_col;
  int error_line;
} JsonParserContext;

typedef struct _JsonParseResult {
  void* value;
  int error;
  int line;
  int col;
} JsonParseResult;

void* json_parse_thing(JsonParserContext* ctx);

int json_parse_is_next(JsonParserContext* ctx, const char* value)
{
  char* current = ctx->str + ctx->index;
  int i = 0;
  while (1)
  {
    if (value[i] == '\0') return 1;
    if (current[i] != value[i]) return 0;
    ++i;
  }
}

char json_parse_peek(JsonParserContext* ctx)
{
  return ctx->str[ctx->index];
}

char json_parse_pop(JsonParserContext* ctx)
{
  char c = ctx->str[ctx->index++];
  if (c == '\n')
  {
    ctx->line++;
    ctx->col = 1;
  }
  else
  {
    ctx->col++;
  }
  return c;
}

void json_parse_skip_whitespace(JsonParserContext* ctx)
{
  char c = ctx->str[ctx->index];
  while (c == ' ' || c == '\t' || c == '\n')
  {
    json_parse_pop(ctx);
    c = ctx->str[ctx->index];
  }
}

int json_parse_is_eof(JsonParserContext* ctx)
{
  return ctx->index == ctx->length;
}

void* json_throw_error(JsonParserContext* ctx, int error_code)
{
  ctx->error_code = error_code;
  ctx->error_col = ctx->col;
  ctx->error_line = ctx->line;
  return NULL;
}

int json_parse_pop_if_next(JsonParserContext* ctx, const char* value)
{
  if (json_parse_is_next(ctx, value))
  {
    int len = strlen(value);
    for (int i = 0; i < len; ++i) json_parse_pop(ctx);
    return 1;
  }
  return 0;
}

void* json_parse_null(JsonParserContext* ctx)
{
  if (json_parse_pop_if_next(ctx, "null")) return get_null();
  return json_throw_error(ctx, JSON_ERROR_BAD_SYNTAX);
}

void* json_parse_boolean(JsonParserContext* ctx)
{
  if (json_parse_pop_if_next(ctx, "true")) return wrap_bool(1);
  if (json_parse_pop_if_next(ctx, "false")) return wrap_bool(0);
  return json_throw_error(ctx, JSON_ERROR_BAD_SYNTAX);
}

void* json_parse_string(JsonParserContext* ctx)
{
  StringBuilder* sb = new_string_builder();
  json_parse_skip_whitespace(ctx);
  if (!json_parse_pop_if_next(ctx, "\""))
  {
    return json_throw_error(ctx, JSON_ERROR_BAD_SYNTAX);
  }
  while (!json_parse_is_eof(ctx))
  {
    char c = json_parse_pop(ctx);
    if (c == '"')
    {
      break;
    }
    if (c == '\\')
    {
      c = json_parse_pop(ctx);
      switch (c)
      {
        case 'n': string_builder_append_char(sb, '\n'); break;
        case 'r': string_builder_append_char(sb, '\r'); break;
        case '"': string_builder_append_char(sb, '"'); break;
        case '\'': string_builder_append_char(sb, '\''); break;
        case '\\': string_builder_append_char(sb, '\\'); break;
        case 't': string_builder_append_char(sb, '\t'); break;
        default: return json_throw_error(ctx, JSON_ERROR_INVALID_STRING_ESCAPE_SEQUENCE);
      }
    }
    else
    {
      string_builder_append_char(sb, c);
    }
  }

  if (json_parse_is_eof(ctx))
  {
    printf("EOF while parsing string!\n");
    return json_throw_error(ctx, JSON_ERROR_EOF);
  }

  String* output = string_builder_to_string(sb);
  string_builder_free(sb);
  return output;
}

void* json_parse_list(JsonParserContext* ctx)
{
  json_parse_skip_whitespace(ctx);
  if (!json_parse_pop_if_next(ctx, "[")) return json_throw_error(ctx, JSON_ERROR_BAD_SYNTAX);
  json_parse_skip_whitespace(ctx);
  List* output = new_list();
  while (!json_parse_pop_if_next(ctx, "]"))
  {
    if (output->length > 0)
    {
      if (!json_parse_pop_if_next(ctx, ",")) return json_throw_error(ctx, JSON_ERROR_BAD_SYNTAX);
      json_parse_skip_whitespace(ctx);
    }

    void* item = json_parse_thing(ctx);
    if (item == NULL) return NULL;
    list_add(output, item);
    json_parse_skip_whitespace(ctx);
  }

  json_parse_skip_whitespace(ctx);
  return output;
}

void* json_parse_object(JsonParserContext* ctx)
{
  json_parse_skip_whitespace(ctx);
  if (!json_parse_pop_if_next(ctx, "{")) return json_throw_error(ctx, JSON_ERROR_BAD_SYNTAX);
  json_parse_skip_whitespace(ctx);
  Dictionary* output = new_dictionary();
  while (!json_parse_pop_if_next(ctx, "}"))
  {
    if (output->size > 0)
    {
      if (!json_parse_pop_if_next(ctx, ",")) return json_throw_error(ctx, JSON_ERROR_BAD_SYNTAX);
      json_parse_skip_whitespace(ctx);
    }
    String* key = (String*) json_parse_string(ctx);
    if (key == NULL) return NULL;
    json_parse_skip_whitespace(ctx);
    if (!json_parse_pop_if_next(ctx, ":")) return json_throw_error(ctx, JSON_ERROR_BAD_SYNTAX);
    json_parse_skip_whitespace(ctx);
    void* item = json_parse_thing(ctx);
    json_parse_skip_whitespace(ctx);
    int is_collision = dictionary_set(output, key, item);
    if (is_collision) return json_throw_error(ctx, JSON_ERROR_OBJECT_KEY_COLLISION);
  }
  return output;
}

void* json_parse_number(JsonParserContext* ctx)
{
  StringBuilder* sb = new_string_builder();
  int decimal_found = 0;
  int sign = json_parse_pop_if_next(ctx, "-") ? -1 : 1;

  while (!json_parse_is_eof(ctx))
  {
    char c = json_parse_peek(ctx);

    if (c == '.')
    {
      if (decimal_found)
      {
        string_builder_free(sb); 
        return json_throw_error(ctx, JSON_ERROR_BAD_SYNTAX);
      }
      decimal_found = 1;
      string_builder_append_char(sb, '.');
      json_parse_pop(ctx);
    }
    else if (c >= '0' && c <= '9')
    {
      string_builder_append_char(sb, c);
      json_parse_pop(ctx);
    }
    else
    {
      break;
    }
  }

  if (sb->length == 1)
  {
    char c = sb->chars[0];
    string_builder_free(sb);
    if (decimal_found) return json_throw_error(ctx, JSON_ERROR_BAD_SYNTAX);
    if (sign == -1) return json_throw_error(ctx, JSON_ERROR_BAD_SYNTAX);
    return wrap_int(sign * (c - '0'));
  }

  String* str = string_builder_to_string(sb);
  free(sb);
  if (decimal_found)
  {
    double value;
    if (!try_parse_float(str->cstring, &value)) return json_throw_error(ctx, JSON_ERROR_BAD_SYNTAX);
    return wrap_float(sign * value);
  }
  else
  {
    int value;
    if (!try_parse_int(str->cstring, &value)) return json_throw_error(ctx, JSON_ERROR_BAD_SYNTAX);
    return wrap_int(sign * value);
  }

}

void* json_parse_thing(JsonParserContext* ctx)
{
  json_parse_skip_whitespace(ctx);
  char next = ctx->str[ctx->index];
  switch (next)
  {
    case '\0':
      return json_throw_error(ctx, JSON_ERROR_EOF);
    case '{': return json_parse_object(ctx);
    case '[': return json_parse_list(ctx);
    case '"': return json_parse_string(ctx);
    default:
      if ((next >= '0' && next <= '9') || next == '.') return json_parse_number(ctx);
      if (next == 't' || next == 'f') return json_parse_boolean(ctx);
      return json_parse_null(ctx);
  }
}

String* json_get_error(JsonParseResult result)
{
  int line = result.line;
  int col = result.col;
  String* line_str = value_to_string(wrap_int(line));
  String* col_str = value_to_string(wrap_int(col));
  String* msg = NULL;
  switch (result.error)
  {
    case JSON_OK: return NULL;
    case JSON_ERROR_BAD_SYNTAX: msg = new_string("Bad syntax"); break;
    case JSON_ERROR_EOF: msg = new_string("Unexpected EOF"); break;
    case JSON_ERROR_OBJECT_KEY_COLLISION: msg = new_string("Object key collision"); break;
    case JSON_ERROR_INVALID_STRING_ESCAPE_SEQUENCE: msg = new_string("Invalid string escape sequence"); break;
    default: msg = new_string("Unknown JSON error"); break;
  }
  return string_concat6(msg->cstring, " on line ", line_str->cstring, ", col ", col_str->cstring, ".");
}

void json_print_error(JsonParseResult result)
{
  String* output = json_get_error(result);
  if (output != NULL)
  {
    printf("JSON Error: %s\n", output->cstring);
  }
}

JsonParseResult json_parse(char* data)
{
  JsonParseResult result;
  
  String* str = string_replace(data, "\r\n", "\n");
  str = string_replace(str->cstring, "\r", "\n");

  JsonParserContext ctx;
  ctx.str = str->cstring;
  ctx.index = 0;
  ctx.length = str->length;
  ctx.error_code = JSON_OK;
  ctx.line = 1;
  ctx.col = 1;

  void* output = json_parse_thing(&ctx);
  if (ctx.error_code)
  {
    result.error = ctx.error_code;
    result.line = ctx.error_line;
    result.col = ctx.error_col;
    result.value = NULL;
  }
  else
  {
    result.error = 0;
    result.line = 0;
    result.col = 0;
    result.value = output;
  }
  return result;
}

#endif
