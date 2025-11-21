#ifndef DNF_CONVERTER_H
#define DNF_CONVERTER_H

#include <stdbool.h>

typedef enum {
  NODE_VAR,     // Variável (1, 2, 3)
  NODE_NOT,     // Negação (n)
  NODE_AND,     // Conjunção (a)
  NODE_OR,      // Disjunção (v)
  NODE_XOR,     // Ou exclusivo (x)
  NODE_IMPLIES  // Implicação (>)
} NodeType;

typedef struct ExprNode {
  NodeType type_;
  int variable_;
  struct ExprNode* left_;
  struct ExprNode* right_;
} ExprNode;

// --- Funções Principais do Projeto ---

// (i) Verifica se duas expressões são equivalentes
bool AreEquivalent(const char* input1, const char* input2);

// (ii) Converte para Forma Normal Conjuntiva (FNC/CNF)
char* ConvertToCNF(const char* input);

// (iii) Converte para Forma Normal Disjuntiva (FND/DNF)
char* ConvertToDNF(const char* input);

// (iv) Verifica se é Satisfatível (SAT)
bool IsSatisfiable(const char* input);

// --- Funções Auxiliares de Manipulação ---
ExprNode* ParseExpression(const char* input, int* pos);
void FreeExprTree(ExprNode* node);
int TreeToString(ExprNode* node, char* buffer, int size);
ExprNode* CloneTree(ExprNode* node);

#endif  // DNF_CONVERTER_H