#ifndef _WAX_NODES_H
#define _WAX_NODES_H

#include "../util/gcbase.h"
#include "../util/strings.h"
#include "./tokens.h"

typedef struct _Node {
  Token* first_token;
  String* type;
} Node;
#define NODE_GC_FIELD_COUNT 2

typedef struct _ClassDefinition {
  Node node;
  Token* class_name;
  Dictionary* members;
  List* member_order;
  Token* base_class_token;
  struct _ClassDefinition* base_class_definition;
} ClassDefinition;
#define CLASS_DEFINITION_GC_FIELD_COUNT (NODE_GC_FIELD_COUNT + 5)
#define CLASS_DEFINITION_NAME "ClassDefinition"

ClassDefinition* new_class_definition(Token* first_token, Token* name_token) {
  ClassDefinition* cd = (ClassDefinition*) gc_create_struct(sizeof(ClassDefinition), CLASS_DEFINITION_NAME, CLASS_DEFINITION_GC_FIELD_COUNT);
  cd->node.first_token = first_token;
  cd->node.type = new_string(CLASS_DEFINITION_NAME);
  cd->class_name = name_token;
  cd->members = new_dictionary();
  cd->member_order = new_list();
  cd->base_class_token = NULL;
  return cd;
}

typedef struct _FunctionDefinition {
  Node node;
  Token* function_name;
  List* code;
  List* arg_tokens;
  List* arg_default_values;
} FunctionDefinition;
#define FUNCTION_DEFINITION_GC_FIELD_COUNT (NODE_GC_FIELD_COUNT + 4)
#define FUNCTION_DEFINITION_NAME "FunctionDefinition"

FunctionDefinition* new_function_definition(Token* first_token, Token* function_name) {
  FunctionDefinition* fd = (FunctionDefinition*) gc_create_struct(sizeof(FunctionDefinition), FUNCTION_DEFINITION_NAME, FUNCTION_DEFINITION_GC_FIELD_COUNT);
  fd->node.first_token = first_token;
  fd->node.type = new_string(FUNCTION_DEFINITION_NAME);
  fd->code = new_list();
  fd->arg_tokens = new_list();
  fd->arg_default_values = new_list();
  return fd;
}

typedef struct _ConstructorDefinition {
  Node node;
  List* code;
  List* arg_tokens;
  List* arg_default_values;
} ConstructorDefinition;
#define CONSTRUCTOR_DEFINITION_GC_FIELD_COUNT (NODE_GC_FIELD_COUNT + 3)
#define CONSTRUCTOR_DEFINITION_NAME "ConstructorDefinition"

typedef struct _FieldDefinition {
  Node node;
  Token* field_name;
  Node* default_value;
} FieldDefinition;
#define FIELD_DEFINITION_GC_FIELD_COUNT (NODE_GC_FIELD_COUNT + 2)
#define FIELD_DEFINITION_NAME "FieldDefinition"

typedef struct _Assignment {
  Node node;
  Node* target;
  Token* assignment_op;
  Node* value;
} Assignment;
#define NODE_ASSIGNMENT_GC_FIELD_COUNT (NODE_GC_FIELD_COUNT + 3)
#define NODE_ASSIGNMENT_NAME "Assignment"

Assignment* new_assignment(Node* target, Token* op, Node* value) {
  Assignment* asgn = (Assignment*) gc_create_struct(sizeof(Assignment), NODE_ASSIGNMENT_NAME, NODE_ASSIGNMENT_GC_FIELD_COUNT);
  asgn->node.first_token = target->first_token;
  asgn->node.type = new_string(NODE_ASSIGNMENT_NAME);
  asgn->target = target;
  asgn->assignment_op = op;
  asgn->value = value;
  return asgn;
}

typedef struct _IfStatement {
  Node node;
  Node* condition;
  List* true_code;
  List* false_code;
} IfStatement;
#define NODE_IF_STATEMENT_GC_FIELD_COUNT (NODE_GC_FIELD_COUNT + 3)
#define NODE_IF_STATEMENT_NAME "IfStatement"

IfStatement* new_if_statement(Token* if_token, Node* condition, List* true_code, List* false_code) {
  IfStatement* ifstat = (IfStatement*) gc_create_struct(sizeof(IfStatement), NODE_IF_STATEMENT_NAME, NODE_IF_STATEMENT_GC_FIELD_COUNT);
  ifstat->node.first_token = if_token;
  ifstat->node.type = new_string(NODE_IF_STATEMENT_NAME);
  ifstat->condition = condition;
  ifstat->true_code = true_code;
  ifstat->false_code = false_code;
  return ifstat;
}

typedef struct _ForLoop {
  Node node;
  List* inits;
  Node* condition;
  List* steps;
  List* code;
} ForLoop;
#define NODE_FOR_LOOP_GC_FIELD_COUNT (NODE_GC_FIELD_COUNT + 4)
#define NODE_FOR_LOOP_NAME "ForLoop"

ForLoop* new_for_loop(Token* for_token, List* inits, Node* condition, List* steps, List* code) {
  ForLoop* fl = (ForLoop*) gc_create_struct(sizeof(ForLoop), NODE_FOR_LOOP_NAME, NODE_FOR_LOOP_GC_FIELD_COUNT);
  fl->node.first_token = for_token;
  fl->node.type = new_string(NODE_FOR_LOOP_NAME);
  fl->inits = inits;
  fl->condition = condition;
  fl->steps = steps;
  fl->code = code;
  return fl;
}

typedef struct _ForEachLoop {
  Node node;
  Token* variable;
  Node* list_expr;
  List* code;
} ForEachLoop;
#define NODE_FOR_EACH_LOOP_GC_FIELD_COUNT (NODE_GC_FIELD_COUNT + 3)
#define NODE_FOR_EACH_LOOP_NAME "ForEachLoop"

ForEachLoop* new_for_each_loop(Token* for_token, Token* iterator_variable, Node* list_expr, List* code) {
  ForEachLoop* fl = (ForEachLoop*) gc_create_struct(sizeof(ForEachLoop), NODE_FOR_EACH_LOOP_NAME, NODE_FOR_EACH_LOOP_GC_FIELD_COUNT);
  fl->node.first_token = for_token;
  fl->node.type = new_string(NODE_FOR_EACH_LOOP_NAME);
  fl->variable = iterator_variable;
  fl->list_expr = list_expr;
  fl->code = code;
  return fl;
}

typedef struct _ExpressionAsExecutable {
  Node node;
  Node* expression;
} ExpressionAsExecutable;
#define NODE_EXPR_EXEC_GC_FIELD_COUNT (NODE_GC_FIELD_COUNT + 1)
#define NODE_EXPR_EXEC_NAME "ExpressionAsExecutable"

ExpressionAsExecutable* new_expression_as_executable(Node* expr) {
  ExpressionAsExecutable* ee = (ExpressionAsExecutable*) gc_create_struct(sizeof(ExpressionAsExecutable), NODE_EXPR_EXEC_NAME, NODE_EXPR_EXEC_GC_FIELD_COUNT);
  ee->node.first_token = expr->first_token;
  ee->node.type = new_string(NODE_EXPR_EXEC_NAME);
  ee->expression = expr;
  return ee;
}

typedef struct _BooleanConstant {
  Node node;
  int value;
} BooleanConstant;
#define NODE_BOOLEAN_CONSTANT_GC_FIELD_COUNT (NODE_GC_FIELD_COUNT + 0)
#define NODE_BOOLEAN_CONSTANT_NAME "BooleanConstant"

BooleanConstant* new_boolean_constant(Token* token, int value) {
  BooleanConstant* bo = (BooleanConstant*) gc_create_struct(sizeof(BooleanConstant), NODE_BOOLEAN_CONSTANT_NAME, NODE_BOOLEAN_CONSTANT_GC_FIELD_COUNT);
  bo->node.first_token = token;
  bo->node.type = new_string(NODE_BOOLEAN_CONSTANT_NAME);
  bo->value = value;
  return bo;
}

typedef struct _IntegerConstant {
  Node node;
  int value;
} IntegerConstant;
#define NODE_INTEGER_CONSTANT_GC_FIELD_COUNT (NODE_GC_FIELD_COUNT + 0)
#define NODE_INTEGER_CONSTANT_NAME "IntegerConstant"

IntegerConstant* new_integer_constant(Token* token, int value) {
  IntegerConstant* str = (IntegerConstant*) gc_create_struct(sizeof(IntegerConstant), NODE_INTEGER_CONSTANT_NAME, NODE_INTEGER_CONSTANT_GC_FIELD_COUNT);
  str->node.first_token = token;
  str->node.type = new_string(NODE_INTEGER_CONSTANT_NAME);
  str->value = value;
  return str;
}

typedef struct _StringConstant {
  Node node;
  String* value;
} StringConstant;
#define NODE_STRING_CONSTANT_GC_FIELD_COUNT (NODE_GC_FIELD_COUNT + 1)
#define NODE_STRING_CONSTANT_NAME "StringConstant"

StringConstant* new_string_constant(Token* token, String* value) {
  StringConstant* str = (StringConstant*) gc_create_struct(sizeof(StringConstant), NODE_STRING_CONSTANT_NAME, NODE_STRING_CONSTANT_GC_FIELD_COUNT);
  str->node.first_token = token;
  str->node.type = new_string(NODE_STRING_CONSTANT_NAME);
  str->value = value;
  return str;
}

typedef struct _Variable {
  Node node;
  String* name;
} Variable;
#define NODE_VARIABLE_GC_FIELD_COUNT (NODE_GC_FIELD_COUNT + 1)
#define NODE_VARIABLE_NAME "Variable"

Variable* new_variable(Token* token, String* name) {
  Variable* v = (Variable*) gc_create_struct(sizeof(Variable), NODE_VARIABLE_NAME, NODE_VARIABLE_GC_FIELD_COUNT);
  v->node.first_token = token;
  v->node.type = new_string(NODE_VARIABLE_NAME);
  v->name = name;
  return v;
}

typedef struct _InlineDictionary {
  Node node;
  List* keys;
  List* values;
} InlineDictionary;
#define NODE_INLINE_DICTIONARY_GC_FIELD_COUNT (NODE_GC_FIELD_COUNT + 2)
#define NODE_INLINE_DICTIONARY_NAME "InlineDictionary"

InlineDictionary* new_inline_dictionary(Token* first_token, List* keys, List* values) {
  InlineDictionary* d = (InlineDictionary*) gc_create_struct(sizeof(InlineDictionary), NODE_INLINE_DICTIONARY_NAME, NODE_INLINE_DICTIONARY_GC_FIELD_COUNT);
  d->node.first_token = first_token;
  d->node.type = new_string(NODE_INLINE_DICTIONARY_NAME);
  d->keys = keys;
  d->values = values;
  return d;
}

typedef struct _DotField {
  Node node;
  Node* root;
  Token* dot_token;
  Token* field_token;
} DotField;
#define NODE_DOT_FIELD_GC_FIELD_COUNT (NODE_GC_FIELD_COUNT + 3)
#define NODE_DOT_FIELD_NAME "DotToken"

DotField* new_dot_field(Node* root_expression, Token* dot_token, Token* field_token) {
  DotField* df = (DotField*) gc_create_struct(sizeof(DotField), NODE_DOT_FIELD_NAME, NODE_DOT_FIELD_GC_FIELD_COUNT);
  df->node.first_token = root_expression->first_token;
  df->node.type = new_string(NODE_DOT_FIELD_NAME);
  df->root = root_expression;
  df->dot_token = dot_token;
  df->field_token = field_token;
  return df;
}

typedef struct _BracketIndex {
  Node node;
  Node* root;
  Token* bracket_token;
  Node* index;
} BracketIndex;
#define NODE_BRACKET_INDEX_GC_FIELD_COUNT (NODE_GC_FIELD_COUNT + 3)
#define NODE_BRACKET_INDEX_FIELD_NAME "BracketIndex"

BracketIndex* new_bracket_index(Node* root_expression, Token* bracket_token, Node* index_expr) {
  BracketIndex* bi = (BracketIndex*) gc_create_struct(sizeof(BracketIndex), NODE_BRACKET_INDEX_FIELD_NAME, NODE_BRACKET_INDEX_GC_FIELD_COUNT);
  bi->node.first_token = root_expression->first_token;
  bi->node.type = new_string(NODE_BRACKET_INDEX_FIELD_NAME);
  bi->root = root_expression;
  bi->bracket_token = bracket_token;
  bi->index = index_expr;
  return bi;
}

typedef struct _OpChain {
  Node node;
  List* expressions;
  List* ops;
} OpChain;
#define NODE_OP_CHAIN_GC_FIELD_COUNT (NODE_GC_FIELD_COUNT + 2)
#define NODE_OP_CHAIN_NAME "OpChain"

OpChain* new_op_chain(List* expressions, List* ops) {
  OpChain* oc = (OpChain*) gc_create_struct(sizeof(OpChain), NODE_OP_CHAIN_NAME, NODE_OP_CHAIN_GC_FIELD_COUNT);
  oc->node.first_token = ((Node*) list_get(expressions, 0))->first_token;
  oc->node.type = new_string(NODE_OP_CHAIN_NAME);
  oc->expressions = list_clone(expressions);
  oc->ops = list_clone(ops);
  return oc;
}

typedef struct _Ternary {
  Node node;
  Node* condition;
  Token* question_mark;
  Node* true_expr;
  Node* false_expr;
} Ternary;
#define NODE_TERNARY_GC_FIELD_COUNT (NODE_GC_FIELD_COUNT + 4)
#define NODE_TERNARY_NAME "Ternary"

Ternary* new_ternary(Node* condition, Token* question_mark_token, Node* true_expr, Node* false_expr) {
  Ternary* ter = (Ternary*) gc_create_struct(sizeof(Ternary), NODE_TERNARY_NAME, NODE_TERNARY_GC_FIELD_COUNT);
  ter->node.first_token = condition->first_token;
  ter->node.type = new_string(NODE_TERNARY_NAME);
  ter->condition = condition;
  ter->question_mark = question_mark_token;
  ter->true_expr = true_expr;
  ter->false_expr = false_expr;
  return ter;
}

typedef struct _FunctionInvocation {
  Node node;
  Node* root;
  Token* open_paren;
  List* args;
} FunctionInvocation;
#define NODE_FUNCTION_INVOCATION_GC_FIELD_COUNT (NODE_GC_FIELD_COUNT + 3)
#define NODE_FUNCTION_INVOCATION_NAME "FunctionInvocation"

FunctionInvocation* new_function_invocation(Node* root_expression, Token* open_paren, List* args) {
  FunctionInvocation* fi = (FunctionInvocation*) gc_create_struct(sizeof(FunctionInvocation), NODE_FUNCTION_INVOCATION_NAME, NODE_FUNCTION_INVOCATION_GC_FIELD_COUNT);
  fi->node.first_token = root_expression->first_token;
  fi->node.type = new_string(NODE_FUNCTION_INVOCATION_NAME);
  fi->root = root_expression;
  fi->open_paren = open_paren;
  fi->args = args;
  return fi;
}


#endif
