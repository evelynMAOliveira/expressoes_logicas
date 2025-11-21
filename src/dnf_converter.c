#include "../include/dnf_converter.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- Construtores Básicos ---

ExprNode* CreateNode(NodeType type, ExprNode* left, ExprNode* right) {
  ExprNode* node = (ExprNode*)malloc(sizeof(ExprNode));
  node->type_ = type;
  node->variable_ = 0;
  node->left_ = left;
  node->right_ = right;
  return node;
}

ExprNode* CreateVar(int var) {
  ExprNode* node = (ExprNode*)malloc(sizeof(ExprNode));
  node->type_ = NODE_VAR;
  node->variable_ = var;
  node->left_ = NULL;
  node->right_ = NULL;
  return node;
}

void FreeExprTree(ExprNode* node) {
  if (!node) return;
  FreeExprTree(node->left_);
  FreeExprTree(node->right_);
  free(node);
}

ExprNode* CloneTree(ExprNode* node) {
  if (!node) return NULL;
  ExprNode* new_node = (ExprNode*)malloc(sizeof(ExprNode));
  *new_node = *node;  // Cópia rasa
  new_node->left_ = CloneTree(node->left_);
  new_node->right_ = CloneTree(node->right_);
  return new_node;
}

// --- Parser Atualizado (Suporta v, >, x, a, n) ---
// Precedência básica: ( ) > n > a > v > > x

ExprNode* ParsePrimary(const char* input, int* pos);

ExprNode* ParseExpression(const char* input, int* pos) {
  ExprNode* left = ParsePrimary(input, pos);
  while (input[*pos] == ' ') (*pos)++;

  // Simples parser recursivo (pode ser melhorado para precedência correta)
  // Aqui tratamos operadores binários genericamente
  while (strchr("avx>", input[*pos]) && input[*pos] != '\0') {
    char op = input[*pos];
    (*pos)++;
    ExprNode* right = ParsePrimary(input, pos);

    NodeType type;
    if (op == 'a')
      type = NODE_AND;
    else if (op == 'v')
      type = NODE_OR;
    else if (op == 'x')
      type = NODE_XOR;
    else if (op == '>')
      type = NODE_IMPLIES;

    left = CreateNode(type, left, right);
    while (input[*pos] == ' ') (*pos)++;
  }
  return left;
}

ExprNode* ParsePrimary(const char* input, int* pos) {
  while (input[*pos] == ' ') (*pos)++;

  if (input[*pos] == 'n') {  // NOT
    (*pos)++;
    return CreateNode(NODE_NOT, ParsePrimary(input, pos), NULL);
  }
  if (input[*pos] == '(') {
    (*pos)++;
    ExprNode* node = ParseExpression(input, pos);
    while (input[*pos] != ')' && input[*pos] != '\0') (*pos)++;
    if (input[*pos] == ')') (*pos)++;
    return node;
  }
  if (input[*pos] >= '0' && input[*pos] <= '9') {
    int var = 0;
    while (input[*pos] >= '0' && input[*pos] <= '9') {
      var = var * 10 + (input[*pos] - '0');
      (*pos)++;
    }
    return CreateVar(var);
  }
  return NULL;
}

// --- Lógica de Simplificação e Conversão ---

// Remove XOR e IMPLIES transformando em AND/OR/NOT básico
ExprNode* NormalizeOperators(ExprNode* node) {
  if (!node) return NULL;

  if (node->type_ == NODE_VAR) return CloneTree(node);

  if (node->type_ == NODE_NOT) {
    return CreateNode(NODE_NOT, NormalizeOperators(node->left_), NULL);
  }

  ExprNode* l = NormalizeOperators(node->left_);
  ExprNode* r = NormalizeOperators(node->right_);

  if (node->type_ == NODE_IMPLIES) {
    // A > B  ===  nA v B
    return CreateNode(NODE_OR, CreateNode(NODE_NOT, l, NULL), r);
  }

  if (node->type_ == NODE_XOR) {
    // A x B === (A a nB) v (nA a B) -- usando OR para DNF
    ExprNode* not_l = CreateNode(NODE_NOT, CloneTree(l), NULL);
    ExprNode* not_r = CreateNode(NODE_NOT, CloneTree(r), NULL);
    ExprNode* term1 = CreateNode(NODE_AND, l, not_r);
    ExprNode* term2 = CreateNode(NODE_AND, not_l, r);
    return CreateNode(NODE_OR, term1, term2);
  }

  return CreateNode(node->type_, l, r);
}

// Move Negações para as folhas (NNF)
ExprNode* PushNegations(ExprNode* node) {
  if (!node) return NULL;

  if (node->type_ == NODE_NOT) {
    ExprNode* child = node->left_;
    if (child->type_ == NODE_NOT) {  // n(n(A)) -> A
      ExprNode* res = PushNegations(child->left_);
      return res;
    }
    if (child->type_ == NODE_AND) {  // n(A a B) -> nA v nB
      ExprNode* nA = CreateNode(NODE_NOT, child->left_, NULL);
      ExprNode* nB = CreateNode(NODE_NOT, child->right_, NULL);
      ExprNode* res = CreateNode(NODE_OR, nA, nB);
      ExprNode* final = PushNegations(res);
      FreeExprTree(
          res);  // Limpar intermediário se necessário, mas aqui simplificamos
      return final;
    }
    if (child->type_ == NODE_OR) {  // n(A v B) -> nA a nB
      ExprNode* nA = CreateNode(NODE_NOT, child->left_, NULL);
      ExprNode* nB = CreateNode(NODE_NOT, child->right_, NULL);
      ExprNode* res = CreateNode(NODE_AND, nA, nB);
      ExprNode* final = PushNegations(res);
      return final;
    }
    // Se for Var, mantém nA
    if (child->type_ == NODE_VAR) {
      return CreateNode(NODE_NOT, CloneTree(child), NULL);
    }
  }

  if (node->type_ == NODE_VAR) return CloneTree(node);

  return CreateNode(node->type_, PushNegations(node->left_),
                    PushNegations(node->right_));
}

// Distributiva para DNF: AND sobre OR -> A a (B v C) = (A a B) v (A a C)
ExprNode* DistributeDNF(ExprNode* node) {
  if (!node) return NULL;
  if (node->type_ == NODE_VAR || node->type_ == NODE_NOT)
    return CloneTree(node);

  ExprNode* l = DistributeDNF(node->left_);
  ExprNode* r = DistributeDNF(node->right_);

  if (node->type_ == NODE_AND) {
    if (l->type_ == NODE_OR) {
      // (P v Q) a R -> (P a R) v (Q a R)
      ExprNode* P = l->left_;
      ExprNode* Q = l->right_;
      ExprNode* R = r;
      ExprNode* t1 = CreateNode(NODE_AND, CloneTree(P), CloneTree(R));
      ExprNode* t2 = CreateNode(NODE_AND, CloneTree(Q), CloneTree(R));
      FreeExprTree(l);
      FreeExprTree(r);
      return DistributeDNF(CreateNode(NODE_OR, t1, t2));
    }
    if (r->type_ == NODE_OR) {
      // L a (P v Q) -> (L a P) v (L a Q)
      ExprNode* L = l;
      ExprNode* P = r->left_;
      ExprNode* Q = r->right_;
      ExprNode* t1 = CreateNode(NODE_AND, CloneTree(L), CloneTree(P));
      ExprNode* t2 = CreateNode(NODE_AND, CloneTree(L), CloneTree(Q));
      FreeExprTree(l);
      FreeExprTree(r);
      return DistributeDNF(CreateNode(NODE_OR, t1, t2));
    }
  }
  return CreateNode(node->type_, l, r);
}

// Distributiva para CNF: OR sobre AND -> A v (B a C) = (A v B) a (A v C)
ExprNode* DistributeCNF(ExprNode* node) {
  if (!node) return NULL;
  if (node->type_ == NODE_VAR || node->type_ == NODE_NOT)
    return CloneTree(node);

  ExprNode* l = DistributeCNF(node->left_);
  ExprNode* r = DistributeCNF(node->right_);

  if (node->type_ == NODE_OR) {
    if (l->type_ == NODE_AND) {
      ExprNode* P = l->left_;
      ExprNode* Q = l->right_;
      ExprNode* R = r;
      ExprNode* t1 = CreateNode(NODE_OR, CloneTree(P), CloneTree(R));
      ExprNode* t2 = CreateNode(NODE_OR, CloneTree(Q), CloneTree(R));
      FreeExprTree(l);
      FreeExprTree(r);
      return DistributeCNF(CreateNode(NODE_AND, t1, t2));
    }
    if (r->type_ == NODE_AND) {
      ExprNode* L = l;
      ExprNode* P = r->left_;
      ExprNode* Q = r->right_;
      ExprNode* t1 = CreateNode(NODE_OR, CloneTree(L), CloneTree(P));
      ExprNode* t2 = CreateNode(NODE_OR, CloneTree(L), CloneTree(Q));
      FreeExprTree(l);
      FreeExprTree(r);
      return DistributeCNF(CreateNode(NODE_AND, t1, t2));
    }
  }
  return CreateNode(node->type_, l, r);
}

// --- Funções de Avaliação (Tabela Verdade) ---

bool EvaluateTree(ExprNode* node, int vars_mask) {
  if (node->type_ == NODE_VAR) {
    return (vars_mask >> (node->variable_ - 1)) & 1;
  }
  if (node->type_ == NODE_NOT) return !EvaluateTree(node->left_, vars_mask);

  bool l = EvaluateTree(node->left_, vars_mask);
  bool r = EvaluateTree(node->right_, vars_mask);

  if (node->type_ == NODE_AND) return l && r;
  if (node->type_ == NODE_OR) return l || r;
  if (node->type_ == NODE_XOR) return l != r;
  if (node->type_ == NODE_IMPLIES) return !l || r;
  return false;
}

// --- Implementação das Funções Públicas ---

char* ConvertToDNF(const char* input) {
  int pos = 0;
  ExprNode* root = ParseExpression(input, &pos);
  if (!root) return NULL;

  root = NormalizeOperators(root);
  root = PushNegations(root);
  root = DistributeDNF(root);

  char* buffer = (char*)malloc(4096);  // Buffer grande
  TreeToString(root, buffer, 4096);
  FreeExprTree(root);
  return buffer;
}

char* ConvertToCNF(const char* input) {
  int pos = 0;
  ExprNode* root = ParseExpression(input, &pos);
  if (!root) return NULL;

  root = NormalizeOperators(root);
  root = PushNegations(root);
  root = DistributeCNF(root);  // Diferença chave aqui

  char* buffer = (char*)malloc(4096);
  TreeToString(root, buffer, 4096);
  FreeExprTree(root);
  return buffer;
}

bool AreEquivalent(const char* input1, const char* input2) {
  int pos1 = 0, pos2 = 0;
  ExprNode* t1 = ParseExpression(input1, &pos1);
  ExprNode* t2 = ParseExpression(input2, &pos2);

  // Verificar todas as combinações de 3 variáveis (000 a 111)
  // O projeto assume variáveis 1, 2, 3? Vamos testar até 5 para segurança
  for (int i = 0; i < 32; i++) {
    if (EvaluateTree(t1, i) != EvaluateTree(t2, i)) {
      FreeExprTree(t1);
      FreeExprTree(t2);
      return false;
    }
  }
  FreeExprTree(t1);
  FreeExprTree(t2);
  return true;
}

bool IsSatisfiable(const char* input) {
  int pos = 0;
  ExprNode* t = ParseExpression(input, &pos);

  for (int i = 0; i < 32; i++) {
    if (EvaluateTree(t, i) == true) {
      FreeExprTree(t);
      return true;
    }
  }
  FreeExprTree(t);
  return false;
}

int TreeToString(ExprNode* node, char* buffer, int size) {
  if (!node) return 0;

  if (node->type_ == NODE_VAR) {
    return sprintf(buffer, "%d", node->variable_);
  }
  if (node->type_ == NODE_NOT) {
    return sprintf(buffer, "n%d", node->left_->variable_);  // Simplificado
  }

  char op = '?';
  switch (node->type_) {
    case NODE_AND:
      op = 'a';
      break;
    case NODE_OR:
      op = 'v';
      break;
    case NODE_XOR:
      op = 'x';
      break;
    case NODE_IMPLIES:
      op = '>';
      break;
    default:
      break;
  }

  int w = sprintf(buffer, "(");
  w += TreeToString(node->left_, buffer + w, size - w);
  w += sprintf(buffer + w, " %c ", op);
  w += TreeToString(node->right_, buffer + w, size - w);
  w += sprintf(buffer + w, ")");
  return w;
}