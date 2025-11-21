#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/dnf_converter.h"

void clear_buffer() {
  int c;
  while ((c = getchar()) != '\n' && c != EOF);
}

int main() {
  int choice;
  char buffer1[256];
  char buffer2[256];

  while (1) {
    printf("\n=== Mini-Projeto de Logica ===\n");
    printf(
        "Sintaxe: 1,2,3 (vars), n (not), a (and), v (or), x (xor), > "
        "(implies)\n");
    printf("Exemplo: (1 a 2) v n3\n\n");

    printf("1. Verificar Equivalencia entre duas sentencas\n");
    printf("2. Converter para FNC (CNF)\n");
    printf("3. Converter para FND (DNF)\n");
    printf("4. Verificar Satisfazibilidade (SAT)\n");
    printf("0. Sair\n");
    printf("Escolha: ");

    if (scanf("%d", &choice) != 1) break;
    clear_buffer();

    if (choice == 0) break;

    switch (choice) {
      case 1:
        printf("Digite a Sentenca 1: ");
        fgets(buffer1, 256, stdin);
        printf("Digite a Sentenca 2: ");
        fgets(buffer2, 256, stdin);
        // Remover newline
        buffer1[strcspn(buffer1, "\n")] = 0;
        buffer2[strcspn(buffer2, "\n")] = 0;

        if (AreEquivalent(buffer1, buffer2))
          printf("\n>> RESULTADO: Sao EQUIVALENTES.\n");
        else
          printf("\n>> RESULTADO: NAO sao equivalentes.\n");
        break;

      case 2:
        printf("Digite a sentenca: ");
        fgets(buffer1, 256, stdin);
        buffer1[strcspn(buffer1, "\n")] = 0;
        char* cnf = ConvertToCNF(buffer1);
        printf("\n>> FNC Gerada: %s\n", cnf);
        free(cnf);
        break;

      case 3:
        printf("Digite a sentenca: ");
        fgets(buffer1, 256, stdin);
        buffer1[strcspn(buffer1, "\n")] = 0;
        char* dnf = ConvertToDNF(buffer1);
        printf("\n>> FND Gerada: %s\n", dnf);
        free(dnf);
        break;

      case 4:
        // O requisito pede para usar a sentença gerada em (ii),
        // mas aqui permitimos entrada direta ou reuso
        printf("Digite a sentenca para checar SAT: ");
        fgets(buffer1, 256, stdin);
        buffer1[strcspn(buffer1, "\n")] = 0;

        if (IsSatisfiable(buffer1))
          printf("\n>> RESULTADO: A sentenca e SATISFATIVEL.\n");
        else
          printf(
              "\n>> RESULTADO: A sentenca e INSATISFATIVEL (Contradicao).\n");
        break;

      default:
        printf("Opcao invalida.\n");
    }
    printf("\nPressione ENTER para continuar...");
    getchar();
  }

  return 0;
}