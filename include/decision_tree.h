/**
 * PokeBattle - Árvore de Decisão para IA
 *
 * Este arquivo contém as declarações da estrutura de árvore de decisão
 * e funções para manipulação da estrutura.
 */

#ifndef DECISION_TREE_H
#define DECISION_TREE_H

#include "structures.h"


// Códigos de condição para nós da árvore
typedef enum {
    CONDITION_HP_PERCENT = 0,      // Compara HP atual/maxHP com um valor percentual
    CONDITION_SPEED_GREATER = 1,   // Verifica se velocidade do bot é maior que a do jogador
    CONDITION_TYPE_ADVANTAGE = 2,  // Verifica se o bot tem vantagem de tipo
    CONDITION_HAS_STATUS = 3,      // Verifica se o monstro tem algum status
    CONDITION_EFFECTIVE_MOVE = 4,  // Verifica se o bot tem um ataque super efetivo
    CONDITION_RANDOM = 5           // Condição aleatória para adicionar variedade
} DecisionCondition;

// Códigos de ação para folhas da árvore
typedef enum {
    ACTION_ATTACK = 0,    // Realizar um ataque (parameter = índice do ataque)
    ACTION_SWITCH = 1,    // Trocar de monstro (parameter = método de seleção)
    ACTION_ITEM = 2       // Usar um item (parameter = tipo do item)
} DecisionAction;

// Estrutura de um nó da árvore de decisão
typedef struct DecisionNode {
    DecisionCondition condition;    // Condição a avaliar
    int value;                      // Valor para comparação
    struct DecisionNode* yes;       // Nó se condição for verdadeira
    struct DecisionNode* no;        // Nó se condição for falsa
    DecisionAction action;          // Ação a executar (-1 se não for folha)
    int parameter;                  // Parâmetro para a ação
} DecisionNode;

// Resultado da decisão
typedef struct {
    DecisionAction action;
    int parameter;
} BotDecision;

// Protótipos das funções

// Cria um novo nó de decisão com a condição e valor especificados
DecisionNode* createDecisionNode(DecisionCondition condition, int value);

// Define uma ação de folha para um nó
void setDecisionLeafAction(DecisionNode* node, DecisionAction action, int parameter);

// Adiciona dois nós filhos a um nó pai
void addDecisionBranch(DecisionNode* parent, DecisionNode* yesNode, DecisionNode* noNode);

// Avalia uma condição específica
bool evaluateDecisionCondition(DecisionCondition condition, int value, PokeMonster* bot, PokeMonster* player);

// Percorre a árvore de decisão para determinar a ação do bot
BotDecision traverseDecisionTree(DecisionNode* root, PokeMonster* bot, PokeMonster* player);

// Cria uma árvore de decisão completa para o bot
DecisionNode* createBotDecisionTree(void);

// Libera a memória de toda a árvore de decisão
void freeDecisionTree(DecisionNode* root);

#endif // DECISION_TREE_H