#include "../include/dnf_converter.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ExprNode* CreateVarNode(int var) {
  ExprNode* node = (ExprNode*)malloc(sizeof(ExprNode));
  if (!node) return NULL;

  node->type_ = NODE_VAR;
  node->variable_ = var;
  node->left_ = NULL;
  node->right_ = NULL;
  return node;
}

ExprNode* CreateNotNode(ExprNode* child) {
  if (!child) return NULL;

  ExprNode* node = (ExprNode*)malloc(sizeof(ExprNode));
  if (!node) {
    FreeExprTree(child);
    return NULL;
  }

  node->type_ = NODE_NOT;
  node->variable_ = 0;
  node->left_ = child;
  node->right_ = NULL;
  return node;
}

ExprNode* CreateBinaryNode(NodeType type, ExprNode* left, ExprNode* right) {
  if (!left || !right) {
    FreeExprTree(left);
    FreeExprTree(right);
    return NULL;
  }

  ExprNode* node = (ExprNode*)malloc(sizeof(ExprNode));
  if (!node) {
    FreeExprTree(left);
    FreeExprTree(right);
    return NULL;
  }

  node->type_ = type;
  node->variable_ = 0;
  node->left_ = left;
  node->right_ = right;
  return node;
}

void FreeExprTree(ExprNode* node) {
  if (!node) return;

  FreeExprTree(node->left_);
  FreeExprTree(node->right_);
  free(node);
}

ExprNode* ParsePrimary(const char* input, int* pos) {
  while (input[*pos] == ' ') (*pos)++;

  if (input[*pos] == 'n') {
    (*pos)++;
    ExprNode* child = ParsePrimary(input, pos);
    return CreateNotNode(child);
  }

  if (input[*pos] == '(') {
    (*pos)++;
    ExprNode* expr = ParseExpression(input, pos);
    while (input[*pos] == ' ') (*pos)++;
    if (input[*pos] == ')') (*pos)++;
    return expr;
  }

  if (input[*pos] >= '1' && input[*pos] <= '3') {
    int var = input[*pos] - '0';
    (*pos)++;
    return CreateVarNode(var);
  }

  return NULL;
}

ExprNode* ParseExpression(const char* input, int* pos) {
  ExprNode* left = ParsePrimary(input, pos);
  if (!left) return NULL;

  while (input[*pos] == ' ') (*pos)++;

  while (input[*pos] == 'a' || input[*pos] == 'x') {
    char op = input[*pos];
    (*pos)++;

    ExprNode* right = ParsePrimary(input, pos);
    if (!right) {
      FreeExprTree(left);
      return NULL;
    }

    NodeType type = (op == 'a') ? NODE_AND : NODE_XOR;
    left = CreateBinaryNode(type, left, right);

    while (input[*pos] == ' ') (*pos)++;
  }

  return left;
}

ExprNode* CloneTree(ExprNode* node) {
  if (!node) return NULL;

  ExprNode* clone = (ExprNode*)malloc(sizeof(ExprNode));
  if (!clone) return NULL;

  clone->type_ = node->type_;
  clone->variable_ = node->variable_;
  clone->left_ = CloneTree(node->left_);
  clone->right_ = CloneTree(node->right_);

  return clone;
}

ExprNode* EliminateXOR(ExprNode* node) {
  if (!node) return NULL;

  if (node->type_ == NODE_VAR) {
    return CloneTree(node);
  }

  if (node->type_ == NODE_NOT) {
    ExprNode* child = EliminateXOR(node->left_);
    return CreateNotNode(child);
  }

  ExprNode* left = EliminateXOR(node->left_);
  ExprNode* right = EliminateXOR(node->right_);

  if (node->type_ == NODE_AND) {
    return CreateBinaryNode(NODE_AND, left, right);
  }

  // XOR: A x B = (A a nB) x (nA a B)
  ExprNode* not_left = CreateNotNode(CloneTree(left));
  ExprNode* not_right = CreateNotNode(CloneTree(right));

  ExprNode* term1 = CreateBinaryNode(NODE_AND, CloneTree(left), not_right);
  ExprNode* term2 = CreateBinaryNode(NODE_AND, not_left, CloneTree(right));

  FreeExprTree(left);
  FreeExprTree(right);

  return CreateBinaryNode(NODE_XOR, term1, term2);
}

ExprNode* ConvertToNNF(ExprNode* node, bool negate) {
  if (!node) return NULL;

  if (node->type_ == NODE_VAR) {
    if (negate) {
      return CreateNotNode(CloneTree(node));
    }
    return CloneTree(node);
  }

  if (node->type_ == NODE_NOT) {
    return ConvertToNNF(node->left_, !negate);
  }

  if (node->type_ == NODE_AND) {
    if (negate) {
      // De Morgan: n(A a B) = nA x nB
      ExprNode* left = ConvertToNNF(node->left_, true);
      ExprNode* right = ConvertToNNF(node->right_, true);
      return CreateBinaryNode(NODE_XOR, left, right);
    } else {
      ExprNode* left = ConvertToNNF(node->left_, false);
      ExprNode* right = ConvertToNNF(node->right_, false);
      return CreateBinaryNode(NODE_AND, left, right);
    }
  }

  if (node->type_ == NODE_XOR) {
    if (negate) {
      // De Morgan: n(A x B) = nA a nB
      ExprNode* left = ConvertToNNF(node->left_, true);
      ExprNode* right = ConvertToNNF(node->right_, true);
      return CreateBinaryNode(NODE_AND, left, right);
    } else {
      ExprNode* left = ConvertToNNF(node->left_, false);
      ExprNode* right = ConvertToNNF(node->right_, false);
      return CreateBinaryNode(NODE_XOR, left, right);
    }
  }

  return NULL;
}

ExprNode* DistributeAndOverOr(ExprNode* node) {
  if (!node) return NULL;

  if (node->type_ == NODE_VAR || node->type_ == NODE_NOT) {
    return CloneTree(node);
  }

  if (node->type_ == NODE_XOR) {
    ExprNode* left = DistributeAndOverOr(node->left_);
    ExprNode* right = DistributeAndOverOr(node->right_);
    return CreateBinaryNode(NODE_XOR, left, right);
  }

  // NODE_AND
  ExprNode* left = DistributeAndOverOr(node->left_);
  ExprNode* right = DistributeAndOverOr(node->right_);

  // Se um dos lados é XOR, distribuir: A a (B x C) = (A a B) x (A a C)
  if (left->type_ == NODE_XOR) {
    ExprNode* t1 =
        CreateBinaryNode(NODE_AND, CloneTree(left->left_), CloneTree(right));
    ExprNode* t2 =
        CreateBinaryNode(NODE_AND, CloneTree(left->right_), CloneTree(right));

    t1 = DistributeAndOverOr(t1);
    t2 = DistributeAndOverOr(t2);

    FreeExprTree(left);
    FreeExprTree(right);

    return CreateBinaryNode(NODE_XOR, t1, t2);
  }

  if (right->type_ == NODE_XOR) {
    ExprNode* t1 =
        CreateBinaryNode(NODE_AND, CloneTree(left), CloneTree(right->left_));
    ExprNode* t2 =
        CreateBinaryNode(NODE_AND, CloneTree(left), CloneTree(right->right_));

    t1 = DistributeAndOverOr(t1);
    t2 = DistributeAndOverOr(t2);

    FreeExprTree(left);
    FreeExprTree(right);

    return CreateBinaryNode(NODE_XOR, t1, t2);
  }

  return CreateBinaryNode(NODE_AND, left, right);
}

ExprNode* ConvertNNFToDNF(ExprNode* node) {
  if (!node) return NULL;

  return DistributeAndOverOr(node);
}

int TreeToString(ExprNode* node, char* buffer, int size) {
  if (!node || size <= 0) return 0;

  int written = 0;

  if (node->type_ == NODE_VAR) {
    written = snprintf(buffer, size, "%d", node->variable_);
    return written;
  }

  if (node->type_ == NODE_NOT) {
    written = snprintf(buffer, size, "n");
    if (written < size) {
      written += TreeToString(node->left_, buffer + written, size - written);
    }
    return written;
  }

  char op = (node->type_ == NODE_AND) ? 'a' : 'x';

  written = snprintf(buffer, size, "(");
  if (written < size) {
    written += TreeToString(node->left_, buffer + written, size - written);
  }
  if (written < size) {
    written += snprintf(buffer + written, size - written, "%c", op);
  }
  if (written < size) {
    written += TreeToString(node->right_, buffer + written, size - written);
  }
  if (written < size) {
    written += snprintf(buffer + written, size - written, ")");
  }

  return written;
}

char* ConvertToDNF(const char* input) {
  if (!input) return NULL;

  int pos = 0;
  ExprNode* tree = ParseExpression(input, &pos);
  if (!tree) return NULL;

  // Passo 1: Eliminar XOR
  ExprNode* no_xor = EliminateXOR(tree);
  FreeExprTree(tree);

  // Passo 2: Converter para NNF (Negation Normal Form)
  ExprNode* nnf = ConvertToNNF(no_xor, false);
  FreeExprTree(no_xor);

  // Passo 3: Converter NNF para DNF
  ExprNode* dnf = ConvertNNFToDNF(nnf);
  FreeExprTree(nnf);

  // Passo 4: Converter árvore para string
  char* result = (char*)malloc(1024);
  if (!result) {
    FreeExprTree(dnf);
    return NULL;
  }

  TreeToString(dnf, result, 1024);
  FreeExprTree(dnf);

  return result;
}
