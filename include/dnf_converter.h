#ifndef DNF_CONVERTER_H
#define DNF_CONVERTER_H

#include <stdbool.h>

/**
 * @brief Tipo de nó na árvore de expressão
 */
typedef enum {
  NODE_VAR,  // Variável (1, 2, 3)
  NODE_NOT,  // Negação
  NODE_AND,  // Conjunção
  NODE_XOR   // Ou exclusivo
} NodeType;

/**
 * @brief Estrutura representando um nó na árvore de expressão lógica
 */
typedef struct ExprNode {
  NodeType type_;
  int variable_;  // Usado quando type_ == NODE_VAR
  struct ExprNode* left_;
  struct ExprNode* right_;
} ExprNode;

/**
 * @brief Converte uma sentença da lógica proposicional para Forma Normal
 * Disjuntiva
 * @param input Sentença de entrada como string (formato: 1,2,3 para variáveis,
 * n=NOT, a=AND, x=XOR)
 * @return String contendo a sentença equivalente em FND, ou NULL em caso de
 * erro
 */
char* ConvertToDNF(const char* input);

/**
 * @brief Parseia uma sentença lógica e constrói uma árvore de expressão
 * @param input String contendo a sentença lógica
 * @param pos Ponteiro para a posição atual na string (modificado durante o
 * parsing)
 * @return Ponteiro para o nó raiz da árvore de expressão
 */
ExprNode* ParseExpression(const char* input, int* pos);

/**
 * @brief Libera a memória alocada para uma árvore de expressão
 * @param node Ponteiro para o nó raiz da árvore a ser liberada
 * @return Nenhum valor de retorno (void)
 */
void FreeExprTree(ExprNode* node);

/**
 * @brief Elimina operadores XOR da expressão convertendo-os para AND e NOT
 * @param node Nó raiz da árvore de expressão
 * @return Nova árvore de expressão sem operadores XOR
 */
ExprNode* EliminateXOR(ExprNode* node);

/**
 * @brief Converte a expressão para Forma Normal de Negação (move negações para
 * as folhas)
 * @param node Nó raiz da árvore de expressão
 * @param negate Flag indicando se deve negar o nó atual
 * @return Nova árvore em forma normal de negação
 */
ExprNode* ConvertToNNF(ExprNode* node, bool negate);

/**
 * @brief Converte expressão em NNF para Forma Normal Disjuntiva
 * @param node Nó raiz da árvore em NNF
 * @return Nova árvore em forma normal disjuntiva
 */
ExprNode* ConvertNNFToDNF(ExprNode* node);

/**
 * @brief Cria uma cópia profunda de uma árvore de expressão
 * @param node Nó raiz da árvore a ser copiada
 * @return Ponteiro para a nova árvore copiada
 */
ExprNode* CloneTree(ExprNode* node);

/**
 * @brief Distribui AND sobre OR para formar DNF (A a (B x C)) -> ((A a B) x (A
 * a C))
 * @param node Nó raiz da árvore de expressão
 * @return Nova árvore com AND distribuído sobre OR
 */
ExprNode* DistributeAndOverOr(ExprNode* node);

/**
 * @brief Converte uma árvore de expressão para string
 * @param node Nó raiz da árvore de expressão
 * @param buffer Buffer onde a string será armazenada
 * @param size Tamanho do buffer
 * @return Número de caracteres escritos
 */
int TreeToString(ExprNode* node, char* buffer, int size);

/**
 * @brief Cria um novo nó de variável
 * @param var Número da variável (1, 2, ou 3)
 * @return Ponteiro para o novo nó criado
 */
ExprNode* CreateVarNode(int var);

/**
 * @brief Cria um novo nó de operador unário (NOT)
 * @param child Nó filho
 * @return Ponteiro para o novo nó criado
 */
ExprNode* CreateNotNode(ExprNode* child);

/**
 * @brief Cria um novo nó de operador binário (AND ou XOR)
 * @param type Tipo do operador (NODE_AND ou NODE_XOR)
 * @param left Nó filho esquerdo
 * @param right Nó filho direito
 * @return Ponteiro para o novo nó criado
 */
ExprNode* CreateBinaryNode(NodeType type, ExprNode* left, ExprNode* right);

#endif  // DNF_CONVERTER_H
