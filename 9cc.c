#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
  ND_NUM = 256,
  TK_NUM = 256,
  TK_EOF,
};

typedef struct Node {
  int ty;
  struct Node *lhs;
  struct Node *rhs;
  int val;
} Node;

typedef struct {
  int ty;
  int val;
  char *input;
} Token;

int pos = 0;

Node *add();
Node *mul();
Node *term();

Token tokens[100];

void tokenize(char *p) {
  int i = 0;
  while (*p) {
    if (isspace(*p)) {
      p++;
      continue;
    }

    if (*p == '+' || *p =='-' || *p == '*' || *p == '/' || *p == '(' || *p == ')') {
      tokens[i].ty = *p;
      i++;
      p++;
      continue;
    }

    if (isdigit(*p)) {
      tokens[i].ty = TK_NUM;
      tokens[i].input = p;
      tokens[i].val = strtol(p, &p, 10);
      i++;
      continue;
    }

    fprintf(stderr, "Could not tokenize: %s\n", p);
    exit(1);
  }
  tokens[i].ty = TK_EOF;
  tokens[i].input = p;
}

Node *new_node(int ty, Node *lhs, Node *rhs) {
  Node *node = malloc(sizeof(Node));
  node->ty = ty;
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

Node *new_node_num(int val) {
  Node *node = malloc(sizeof(Node));
  node->ty = ND_NUM;
  node->val = val;
  return node;
}

int consume(int ty) {
  if (tokens[pos].ty != ty)
    return 0;
  pos++;
  return 1;
}

Node *term() {
  if (consume('(')) {
    Node *node = add();
    if (!consume(')'))
      error("tojiro: %s", tokens[pos].input);
    return node;
  }

  if (tokens[pos].ty == TK_NUM)
    return new_node_num(tokens[pos++].val);

  error("dame: %s", tokens[pos].input);
}

Node *mul() {
  Node *node = term();

  for (;;) {
    if (consume('*'))
      node = new_node('*', node, term());
    if (consume('/'))
      node = new_node('/', node, term());
    else
      return node;
  }
}

Node *add() {
  Node *node = mul();

  for (;;) {
    if (consume('+'))
      node = new_node('+', node, mul());
    if (consume('-'))
      node = new_node('-', node, mul());
    else
      return node;
  }
}

void gen(Node *node) {
  if (node->ty == ND_NUM) {
    printf("  push %d\n", node->val);
    return;
  }

  gen(node->lhs);
  gen(node->rhs);

  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch (node->ty) {
  case '+':
    printf("  add rax, rdi\n");
    break;
  case '-':
    printf("  sub rax, rdi\n");
    break;
  case '*':
    printf("  mul rdi\n");
    break;
  case '/':
    printf("  mov rdx, 0\n");
    printf("  div rax, rdi\n");
    break;
  }

  printf("  push rax\n");

}

void error(char* msg, char* input) {
/*  fprintf(stderr, "Unexpected token: %s\n", tokens[i].input);*/
  fprintf(stderr, msg, input);
  exit(1);
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "The number of arguments is incorrect.\n");
    return 1;
  }

  tokenize(argv[1]);
  Node *node = add();

  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");

  gen(node);

  printf("  pop rax\n")  ;
  printf("  ret\n")  ;
  return 0;
}
