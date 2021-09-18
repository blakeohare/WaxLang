#ifndef _WAX_TOKENS_H
#define _WAX_TOKENS_H

#include "../util/strings.h"
#include "../util/lists.h"
#include "../util/valueutil.h"

enum TokenType {
  TOKEN_TYPE_PUNC,
  TOKEN_TYPE_STRING,
  TOKEN_TYPE_INTEGER,
  TOKEN_TYPE_FLOAT,
  TOKEN_TYPE_WORD,
  TOKEN_TYPE_KEYWORD
};

#define TOKEN_GC_FIELD_COUNT 2
#define TOKEN_NAME "Token"
typedef struct _Token {
  String* file;
  String* value;
  int line;
  int col;
  enum TokenType type;
} Token;

Token* new_token(String* file, String* value, int line, int col, enum TokenType type) {
  Token* token = (Token*) gc_create_struct(sizeof(Token), TOKEN_NAME, TOKEN_GC_FIELD_COUNT);
  token->file = file;
  token->value = value;
  token->line = line;
  token->col = col;
  token->type = type;
  return token;
}

int token_is_name(Token* token) {
  return token->type == TOKEN_TYPE_WORD;
}

#define TOKEN_STREAM_GC_FIELD_COUNT 4
#define TOKEN_STREAM_NAME "TokenStream"

typedef struct _TokenStream {
  List* tokens;
  String* filename;
  String* error;
  Token* error_token;
  int index;
  int length;
} TokenStream;

Dictionary* _tokenizer_create_string_set(const char* space_sep_values) {
  List* words = string_split(space_sep_values, " ");
  Dictionary* lookup = new_dictionary();
  for (int i = 0; i < words->length; ++i) {
    String* k = list_get_string(words, i);
    dictionary_set(lookup, k, wrap_bool(1));
  }
  return lookup;
}

TokenStream* tokenize(String* filename, String* content) {
  TokenStream* token_stream = (TokenStream*)gc_create_struct(sizeof(TokenStream), TOKEN_STREAM_NAME, TOKEN_STREAM_GC_FIELD_COUNT);
  token_stream->error = NULL;
  token_stream->filename = filename;
  token_stream->index = 0;
  token_stream->length = 0;
  token_stream->error_token = NULL;
  token_stream->tokens = new_list();

  content = string_concat(string_replace(content->cstring, "\r\n", "\n")->cstring, "\n");
  const char* chars = content->cstring;
  int len = content->length;
  int* lines = (int*) malloc(sizeof(int) * len);
  int* cols = (int*) malloc(sizeof(int) * len);
  int line = 1;
  int col = 1;
  char c;
  for (int i = 0; i < len; ++i) {
    lines[i] = line;
    cols[i] = col;
    c = chars[i];
    if (c == '\n') {
      line++;
      col = 1;
    } else {
      col++;
    }
  }

  Dictionary* keywords = _tokenizer_create_string_set("if else function for while do try catch except class constructor field return continue break switch case default");
  Dictionary* multichar_tokens = _tokenizer_create_string_set("++ -- << >> || && == != <= >= => += -= *= /= &= |= ^= ** ??");
  String* str_temp = NULL;

  char state = 'N'; // N - Normal, S - String, C - Comment, W - Word
  char token_type = ' '; // / or * for comments, ' or " for strings
  int token_start = 0;
  char c2;

  List* tokens = token_stream->tokens;
  char single_char_buf[2];
  char two_char_buf[3];
  single_char_buf[1] = '\0';
  two_char_buf[2] = '\0';

  for (int i = 0; i < len; ++i) {
    c = chars[i];
    c2 = chars[i + 1];
    switch (state) {
      case 'N':
        if (c == ' ' || c == '\n' || c == '\t') {
          // do nothing!
        } else if (c == '"' || c == '\'') {
          token_start = i;
          state = 'S';
          token_type = c;
        } else if (c == '/' && (c2 == '/' || c2 == '*')) {
          state = 'C';
          token_type = c2;
          i++;
        } else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_') {
          state = 'W';
          token_start = i;
          --i;
        } else if (c2 != '\0') {
          two_char_buf[0] = c;
          two_char_buf[1] = c2;
          str_temp = new_string(two_char_buf);
          if (dictionary_has_key(multichar_tokens, str_temp)) {
            list_add(tokens, new_token(filename, str_temp, lines[i], cols[i], TOKEN_TYPE_PUNC));
            i++;
          } else {
            single_char_buf[0] = c;
            list_add(tokens, new_token(filename, new_string(single_char_buf), lines[i], cols[i], TOKEN_TYPE_PUNC));
          }
        } else {
          single_char_buf[0] = c;
          list_add(tokens, new_token(filename, new_string(single_char_buf), lines[i], cols[i], TOKEN_TYPE_PUNC));
        }
        break;

      case 'S':
        if (c == '\\') {
          ++i; // worry about if the escape sequence is valid later
        } else if (c == token_type) {
          str_temp = new_string_from_range(chars, token_start, i + 1);
          list_add(tokens, new_token(filename, str_temp, lines[token_start], cols[token_start], TOKEN_TYPE_STRING));
          state = 'N';
        }
        break;

      case 'C':
        if (token_type == '/') {
          if (c == '\n') {
            state = 'N';
          }
        } else {
          if (c == '*' && c2 == '/') {
            state = 'N';
            i++;
          }
        }
        break;

      case 'W':
        if ((c < 'a' || c > 'z') &&
            (c < 'A' || c > 'Z') &&
            (c < '0' || c > '9') &&
            c != '_') {

          str_temp = new_string_from_range(chars, token_start, i);
          --i;
          state = 'N';
          enum TokenType tt = TOKEN_TYPE_WORD;
          if (dictionary_has_key(keywords, str_temp)) {
            tt = TOKEN_TYPE_KEYWORD;
          } else if (str_temp->cstring[0] >= '0' && str_temp->cstring[0] <= '9') {
            tt = TOKEN_TYPE_INTEGER;
          }
          list_add(tokens, new_token(filename, str_temp, lines[token_start], cols[token_start], tt));
        }
        break;
    }
  }

  if (state != 'N') {
    if (state == 'S') {
      token_stream->error = new_string("This code contains an unclosed string.");
    } else if (state == 'C') {
      token_stream->error = new_string("This code contains an unclosed comment.");
    }
    return token_stream;
  }

  token_stream->length = token_stream->tokens->length;
  return token_stream;
}

#endif
