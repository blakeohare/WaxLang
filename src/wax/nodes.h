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

typedef struct _OpChain {
  Node node;
  List* expressions;
  List* ops;
} OpChain;
#define NODE_OP_CHAIN_DEFINITION_GC_FIELD_COUNT (NODE_GC_FIELD_COUNT + 2)
#define NODE_OP_CHAIN_DEFINITION_NAME "OpChain"

OpChain* new_op_chain(List* expressions, List* ops) {
  OpChain* oc = (OpChain*) gc_create_struct(sizeof(OpChain), NODE_OP_CHAIN_DEFINITION_NAME, NODE_OP_CHAIN_DEFINITION_GC_FIELD_COUNT);
  oc->node.first_token = ((Node*) list_get(expressions, 0))->first_token;
  oc->node.type = new_string(NODE_OP_CHAIN_DEFINITION_NAME);
  oc->expressions = list_clone(expressions);
  oc->ops = list_clone(ops);
  return oc;
}

#endif
