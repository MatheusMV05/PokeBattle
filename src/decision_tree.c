/**
 * PokeBattle - Implementação da Árvore de Decisão para IA
 *
 * Este arquivo contém as implementações das funções para manipular
 * a estrutura de árvore de decisão usada pelo bot.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "decision_tree.h"
#include "monsters.h"
#include "battle.h"

// Cria um novo nó de decisão
DecisionNode* createDecisionNode(DecisionCondition condition, int value) {
    DecisionNode* node = (DecisionNode*)malloc(sizeof(DecisionNode));
    if (node == NULL) {
        printf("Erro: Falha ao alocar memória para nó de decisão!\n");
        return NULL;
    }

    node->condition = condition;
    node->value = value;
    node->yes = NULL;
    node->no = NULL;
    node->action = -1;  // -1 indica que não é um nó folha
    node->parameter = 0;

    return node;
}

// Define uma ação para um nó folha
void setDecisionLeafAction(DecisionNode* node, DecisionAction action, int parameter) {
    if (node == NULL) return;

    // Garantir que é um nó folha removendo quaisquer filhos
    if (node->yes != NULL) {
        freeDecisionTree(node->yes);
        node->yes = NULL;
    }

    if (node->no != NULL) {
        freeDecisionTree(node->no);
        node->no = NULL;
    }

    node->action = action;
    node->parameter = parameter;
}

// Adiciona dois nós filhos a um nó pai
void addDecisionBranch(DecisionNode* parent, DecisionNode* yesNode, DecisionNode* noNode) {
    if (parent == NULL) return;

    // Liberar qualquer filho existente primeiro
    if (parent->yes != NULL) {
        freeDecisionTree(parent->yes);
    }

    if (parent->no != NULL) {
        freeDecisionTree(parent->no);
    }

    parent->yes = yesNode;
    parent->no = noNode;

    // Garantir que não é um nó folha
    parent->action = -1;
}

// Avalia uma condição específica
bool evaluateDecisionCondition(DecisionCondition condition, int value, PokeMonster* bot, PokeMonster* player) {
    if (bot == NULL || player == NULL) return false;

    switch (condition) {
        case CONDITION_HP_PERCENT: {
            // Verifica se o HP do bot está abaixo de um percentual
            float hpPercent = (float)bot->hp / bot->maxHp * 100.0f;
            return hpPercent < value;
        }

        case CONDITION_SPEED_GREATER:
            // Verifica se o bot é mais rápido que o jogador
            return bot->speed > player->speed;

        case CONDITION_TYPE_ADVANTAGE: {
            // Verifica se o bot tem vantagem de tipo sobre o jogador
            float effectiveness = calculateTypeEffectiveness(
                bot->type1, player->type1, player->type2);
            return effectiveness > 1.0f;
        }

        case CONDITION_HAS_STATUS:
            // value = 0: verifica status do bot
            // value = 1: verifica status do jogador
            if (value == 0) {
                return bot->statusCondition != STATUS_NONE;
            } else {
                return player->statusCondition != STATUS_NONE;
            }

        case CONDITION_EFFECTIVE_MOVE: {
            // Verifica se o bot tem pelo menos um ataque super efetivo
            for (int i = 0; i < 4; i++) {
                if (bot->attacks[i].ppCurrent <= 0) continue;

                float effectiveness = calculateTypeEffectiveness(
                    bot->attacks[i].type, player->type1, player->type2);

                if (effectiveness >= 2.0f) {
                    return true;
                }
            }
            return false;
        }

        case CONDITION_RANDOM:
            // Condição aleatória com probabilidade value%
            return (rand() % 100) < value;

        default:
            return false;
    }
}

// Percorre a árvore de decisão para determinar a ação do bot
BotDecision traverseDecisionTree(DecisionNode* root, PokeMonster* bot, PokeMonster* player) {
    BotDecision decision = {ACTION_ATTACK, 0}; // Decisão padrão

    if (root == NULL || bot == NULL || player == NULL) {
        return decision;
    }

    // Se o nó é uma folha, retorna sua ação
    if (root->action != -1) {
        decision.action = root->action;
        decision.parameter = root->parameter;
        return decision;
    }

    // Avaliar a condição e seguir o ramo apropriado
    bool result = evaluateDecisionCondition(root->condition, root->value, bot, player);

    if (result) {
        // Seguir o ramo 'sim'
        return traverseDecisionTree(root->yes, bot, player);
    } else {
        // Seguir o ramo 'não'
        return traverseDecisionTree(root->no, bot, player);
    }
}

// Cria uma árvore de decisão completa para o bot
DecisionNode* createBotDecisionTree(void) {
    // Criar o nó raiz - Verificar HP baixo
    DecisionNode* root = createDecisionNode(CONDITION_HP_PERCENT, 25);

    // Nó para HP baixo - verificar se tem item de cura
    DecisionNode* lowHpNode = createDecisionNode(CONDITION_RANDOM, 70);

    // Nó para HP ok - verificar vantagem de tipo
    DecisionNode* typeAdvantageNode = createDecisionNode(CONDITION_TYPE_ADVANTAGE, 0);

    // Nós para ação de cura
    DecisionNode* healNode = createDecisionNode(CONDITION_RANDOM, 0);
    setDecisionLeafAction(healNode, ACTION_ITEM, ITEM_POTION);

    // Nós para ação de troca quando HP está baixo
    DecisionNode* switchLowHpNode = createDecisionNode(CONDITION_RANDOM, 0);
    setDecisionLeafAction(switchLowHpNode, ACTION_SWITCH, 0);

    // Adicionar ramos para nó de HP baixo
    addDecisionBranch(lowHpNode, healNode, switchLowHpNode);

    // Nós para vantagem de tipo
    DecisionNode* hasAdvantageNode = createDecisionNode(CONDITION_EFFECTIVE_MOVE, 0);
    DecisionNode* noAdvantageNode = createDecisionNode(CONDITION_SPEED_GREATER, 0);

    // Nós para ataque com vantagem
    DecisionNode* attackAdvantageNode = createDecisionNode(CONDITION_RANDOM, 0);
    setDecisionLeafAction(attackAdvantageNode, ACTION_ATTACK, 0); // 0 = melhor ataque

    // Nós quando o bot é mais rápido
    DecisionNode* fasterNode = createDecisionNode(CONDITION_RANDOM, 20);
    DecisionNode* slowerNode = createDecisionNode(CONDITION_RANDOM, 50);

    // Nós folha para ações finais
    DecisionNode* attackNode = createDecisionNode(CONDITION_RANDOM, 0);
    setDecisionLeafAction(attackNode, ACTION_ATTACK, 0);

    DecisionNode* statusMoveNode = createDecisionNode(CONDITION_RANDOM, 0);
    setDecisionLeafAction(statusMoveNode, ACTION_ATTACK, 1); // 1 = ataque de status

    DecisionNode* switchNode = createDecisionNode(CONDITION_RANDOM, 0);
    setDecisionLeafAction(switchNode, ACTION_SWITCH, 0);

    // Adicionar branches para o nó de vantagem de ataque
    addDecisionBranch(hasAdvantageNode, attackAdvantageNode, attackNode);

    // Adicionar branches para os nós de velocidade
    addDecisionBranch(fasterNode, attackNode, statusMoveNode);
    addDecisionBranch(slowerNode, switchNode, attackNode);

    // Adicionar branches para o nó de vantagem de tipo
    addDecisionBranch(typeAdvantageNode, hasAdvantageNode, noAdvantageNode);

    // Adicionar branches para o nó de velocidade em caso de não ter vantagem
    addDecisionBranch(noAdvantageNode, fasterNode, slowerNode);

    // Finalizar a árvore conectando tudo à raiz
    addDecisionBranch(root, lowHpNode, typeAdvantageNode);

    return root;
}

// Libera a memória de toda a árvore de decisão
void freeDecisionTree(DecisionNode* root) {
    if (root == NULL) return;

    // Liberar recursivamente os filhos
    if (root->yes != NULL) {
        freeDecisionTree(root->yes);
    }

    if (root->no != NULL) {
        freeDecisionTree(root->no);
    }

    // Liberar o nó atual
    free(root);
}