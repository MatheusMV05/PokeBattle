/**
 * PokeBattle - Implementação do sistema de batalha
 *
 * Este arquivo contém as implementações das funções para o sistema de batalha.
 */
#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #define NOGDI
    #define NOUSER
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "raylib.h"
#include "battle.h"
#include "monsters.h"
#include "ia_integration.h"
#include "resources.h"
#include "decision_tree.h"
#include "globals.h"

// Variável global para armazenar a árvore de decisão
static DecisionNode* botDecisionTree = NULL;

// Inicializa o sistema de batalha
void initializeBattleSystem(void) {
    // Já inicializado
    if (battleSystem != NULL) {
        return;
    }

    // Alocar o sistema de batalha
    battleSystem = (BattleSystem*)malloc(sizeof(BattleSystem));
    if (battleSystem == NULL) {
        printf("Erro ao alocar sistema de batalha!\n");
        return;
    }

    // Inicializar componentes
    battleSystem->playerTeam = NULL;
    battleSystem->opponentTeam = NULL;
    battleSystem->actionQueue = createActionQueue(10);
    battleSystem->effectStack = createEffectStack(20);
    battleSystem->turn = 0;
    battleSystem->battleState = BATTLE_IDLE;
    battleSystem->playerTurn = true;
    battleSystem->selectedAttack = 0;
    battleSystem->selectedAction = 0;
    battleSystem->itemUsed = false;
    battleSystem->itemType = ITEM_POTION; // Padrão
    initBotDecisionTree();

    // Verificar se tudo foi alocado corretamente
    if (battleSystem->actionQueue == NULL || battleSystem->effectStack == NULL) {
        printf("Erro ao alocar componentes do sistema de batalha!\n");
        freeBattleSystem();
        return;
    }
}

// Inicia uma nova batalha
void startNewBattle(MonsterList* playerTeam, MonsterList* opponentTeam) {
    if (battleSystem == NULL || playerTeam == NULL || opponentTeam == NULL) {
        return;
    }

    // Configurar os times
    battleSystem->playerTeam = playerTeam;
    battleSystem->opponentTeam = opponentTeam;

    // Resetar contadores e estado
    battleSystem->turn = 1;
    battleSystem->battleState = BATTLE_INTRO;
    battleSystem->playerTurn = true;
    battleSystem->itemUsed = false;
    battleSystem->selectedAttack = 0;
    battleSystem->selectedAction = 0;

    // Escolher um item aleatório para a batalha
    battleSystem->itemType = rollRandomItem();

    // Limpar estruturas de dados
    clearQueue(battleSystem->actionQueue);
    clearStack(battleSystem->effectStack);
    actionQueueReady = false;

    // Resetar mensagens e animações
    memset(&currentMessage, 0, sizeof(currentMessage));
    memset(&currentAnimation, 0, sizeof(currentAnimation));
    stateTransitionDelay = 0.0f;

    // Mensagem inicial
    strcpy(battleMessage, "Uma batalha selvagem começou!");
}

// Atualiza o estado da batalha
void updateBattle(void) {
    if (battleSystem == NULL) return;

    // Atualizar timers globais
    float deltaTime = GetFrameTime();

    // Verificar se a batalha acabou
    if (isBattleOver() && battleSystem->battleState != BATTLE_OVER &&
        battleSystem->battleState != BATTLE_MESSAGE_DISPLAY) {
        battleSystem->battleState = BATTLE_OVER;

        // Definir mensagem de fim de batalha
        int winner = getBattleWinner();
        if (winner == 1) {
            strcpy(battleMessage, "Você venceu a batalha!");
        } else if (winner == 2) {
            strcpy(battleMessage, "Você perdeu a batalha!");
        } else {
            strcpy(battleMessage, "A batalha terminou em empate!");
        }
        return;
    }

    // Atualizar estado atual
    switch (battleSystem->battleState) {
        case BATTLE_INTRO:
            // Animação de entrada na batalha
            stateTransitionDelay += deltaTime;
            if (stateTransitionDelay >= 1.5f) {
                stateTransitionDelay = 0.0f;
                battleSystem->playerTurn = true;
                battleSystem->battleState = BATTLE_MESSAGE_DISPLAY;
                strcpy(battleMessage, "Uma batalha selvagem começou!");

                // Configurar a mensagem atual para exibição
                currentMessage.displayTime = 2.0f;
                currentMessage.elapsedTime = 0.0f;
                currentMessage.waitingForInput = true;
                currentMessage.autoAdvance = false;
                strcpy(currentMessage.message, battleMessage);
            }
            break;

        case BATTLE_MESSAGE_DISPLAY:
            // Atualizar mensagem atual
            currentMessage.elapsedTime += deltaTime;

            // Se está esperando input e já passou tempo suficiente
            if (currentMessage.waitingForInput &&
                currentMessage.elapsedTime >= currentMessage.displayTime) {

                // Verificar se houve clique ou tecla pressionada
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) ||
                    IsKeyPressed(KEY_SPACE) ||
                    IsKeyPressed(KEY_ENTER)) {

                    // CORREÇÃO: Garantir que vamos para o estado correto
                    // Se estivermos no início do turno, ir para seleção de ação
                    if (battleSystem->turn > 0 && isQueueEmpty(battleSystem->actionQueue)) {
                        battleSystem->battleState = BATTLE_SELECT_ACTION;
                        battleSystem->playerTurn = true; // Garantir que o jogador vai jogar
                    }
                    // Se ainda tiver ações na fila, continuar executando
                    else if (!isQueueEmpty(battleSystem->actionQueue)) {
                        battleSystem->battleState = BATTLE_EXECUTING_ACTIONS;
                    }
                    // Caso padrão
                    else {
                        battleSystem->battleState = BATTLE_SELECT_ACTION;
                    }
                }
            } else if (currentMessage.autoAdvance &&
                      currentMessage.elapsedTime >= currentMessage.displayTime) {
                // Avançar automaticamente
                if (!isQueueEmpty(battleSystem->actionQueue)) {
                    battleSystem->battleState = BATTLE_EXECUTING_ACTIONS;
                } else {
                    // CORREÇÃO: Garantir que vamos para seleção de ação
                    battleSystem->battleState = BATTLE_SELECT_ACTION;
                    // CORREÇÃO: Garantir que a vez é do jogador no início do turno
                    if (battleSystem->turn > 0) {
                        battleSystem->playerTurn = true;
                    }
                }
            }
            break;

        case BATTLE_SELECT_ACTION:
            // Aguardando seleção do jogador ou bot
            if (battleSystem->playerTurn) {
                // Jogador está selecionando - Interface cuida disso
                printf("[DEBUG] BATTLE_SELECT_ACTION: Vez do jogador\n");
            } else {
                printf("[DEBUG] BATTLE_SELECT_ACTION: Vez do bot\n");
                // Dar um pequeno delay antes do bot escolher
                stateTransitionDelay += deltaTime;
                if (stateTransitionDelay >= 0.5f) {
                    stateTransitionDelay = 0.0f;
                    printf("[DEBUG] Bot está escolhendo ação...\n");
                    botChooseAction();
                }
            }
            break;

        case BATTLE_PREPARING_ACTIONS:
            // Ordenar ações por velocidade
            determineAndExecuteTurnOrder();
            battleSystem->battleState = BATTLE_EXECUTING_ACTIONS;
            break;

        case BATTLE_EXECUTING_ACTIONS:
            // Executar próxima ação da fila
            if (!isQueueEmpty(battleSystem->actionQueue)) {
                int action, parameter;
                PokeMonster* monster;

                dequeue(battleSystem->actionQueue, &action, &parameter, &monster);
                printf("[DEBUG] Dequeue executando ação: tipo=%d, param=%d\n", action, parameter);

                switch (action) {
                    case 0: // Ataque
                    {
                        PokeMonster* target = (monster == battleSystem->playerTeam->current) ?
                                              battleSystem->opponentTeam->current :
                                              battleSystem->playerTeam->current;

                        executeAttack(monster, target, parameter);

                        // Mostrar mensagem do ataque
                        if (strlen(battleMessage) > 0) {
                            // Configurar a mensagem atual para exibição
                            currentMessage.displayTime = 2.0f;
                            currentMessage.elapsedTime = 0.0f;
                            currentMessage.waitingForInput = false;
                            currentMessage.autoAdvance = true;
                            strcpy(currentMessage.message, battleMessage);
                            battleSystem->battleState = BATTLE_MESSAGE_DISPLAY;
                        }
                    }
                    break;

                    case 1: // Troca
                        executeMonsterSwitch(monster, parameter);

                        // Mostrar mensagem de troca
                        if (strlen(battleMessage) > 0) {
                            // Configurar a mensagem atual para exibição
                            currentMessage.displayTime = 1.5f;
                            currentMessage.elapsedTime = 0.0f;
                            currentMessage.waitingForInput = false;
                            currentMessage.autoAdvance = true;
                            strcpy(currentMessage.message, battleMessage);
                            battleSystem->battleState = BATTLE_MESSAGE_DISPLAY;
                        }
                        break;

                    case 2: // Item
                        executeItemUse(monster, battleSystem->itemType);

                        // Mostrar mensagem de uso de item
                        if (strlen(battleMessage) > 0) {
                            // Configurar a mensagem atual para exibição
                            currentMessage.displayTime = 1.5f;
                            currentMessage.elapsedTime = 0.0f;
                            currentMessage.waitingForInput = false;
                            currentMessage.autoAdvance = true;
                            strcpy(currentMessage.message, battleMessage);
                            battleSystem->battleState = BATTLE_MESSAGE_DISPLAY;
                        }
                        break;
                }
            } else {
                // Todas ações executadas, fim de turno
                printf("[DEBUG] Fila vazia, indo para BATTLE_TURN_END\n");
                battleSystem->battleState = BATTLE_TURN_END;
            }
            break;

        case BATTLE_TURN_END:
            // Processar fim de turno
            printf("[DEBUG TURN END] Processando fim de turno %d\n", battleSystem->turn);
            processTurnEnd();
            battleSystem->turn++;

            // Resetar para o novo turno
            clearQueue(battleSystem->actionQueue);
            actionQueueReady = false;

            // CORREÇÃO: Explicitamente definir que é a vez do jogador
            battleSystem->playerTurn = true;

            // Mostrar mensagem de início de turno
            sprintf(battleMessage, "Turno %d - Escolha sua ação!", battleSystem->turn);
            currentMessage.displayTime = 1.0f;
            currentMessage.elapsedTime = 0.0f;
            currentMessage.waitingForInput = true;
            currentMessage.autoAdvance = false;
            strcpy(currentMessage.message, battleMessage);
            battleSystem->battleState = BATTLE_MESSAGE_DISPLAY;

            printf("[DEBUG TURN END] Iniciando turno %d, jogador: %s\n",
                  battleSystem->turn, battleSystem->playerTurn ? "SIM" : "NÃO");
            break;

        case BATTLE_FORCED_SWITCH:
            // Esperar jogador escolher novo monstro
            break;

        case BATTLE_OVER:
            // Batalha finalizada
            break;

        default:
            break;
    }
}

// Executa um ataque
void executeAttack(PokeMonster* attacker, PokeMonster* defender, int attackIndex) {
    if (attacker == NULL || defender == NULL || attackIndex < 0 || attackIndex >= 4) {
        return;
    }

    // Verificar se o monstro pode atacar
    if (attacker->statusCondition == STATUS_SLEEPING) {
        sprintf(battleMessage, "%s está dormindo e não pode atacar!", attacker->name);
        return;
    }

    if (attacker->statusCondition == STATUS_PARALYZED) {
        // 25% de chance de não conseguir atacar
        if (rand() % 100 < 25) {
            sprintf(battleMessage, "%s está paralisado e não conseguiu atacar!", attacker->name);
            return;
        }
    }

    // Verificar se o ataque tem PP
    Attack* attack = &attacker->attacks[attackIndex];
    if (attack->ppCurrent <= 0) {
        sprintf(battleMessage, "%s tentou usar %s, mas não tem mais PP!",
                attacker->name, attack->name);
        return;
    }

    // Consumir PP
    attack->ppCurrent--;

    // Verificar acerto (baseado na precisão)
    int hitRoll = rand() % 100;
    if (hitRoll >= attack->accuracy) {
        sprintf(battleMessage, "%s usou %s, mas errou!", attacker->name, attack->name);
        return;
    }

    // Gerar descrição do ataque com IA (apenas para ataques bem-sucedidos)
    char* description = generateAttackDescription(attacker, defender, attack);
    if (description) {
        strncpy(battleMessage, description, sizeof(battleMessage) - 1);
        free(description);
    } else {
        sprintf(battleMessage, "%s usou %s!", attacker->name, attack->name);
    }

    // Calcular dano (se for um ataque de dano)
    if (attack->power > 0) {
        int damage = calculateDamage(attacker, defender, attack);

        // Aplicar dano
        defender->hp -= damage;
        if (defender->hp < 0) {
            defender->hp = 0;
        }

        // Adicionar informação de dano à mensagem
        char damageText[50];
        sprintf(damageText, " Causou %d de dano!", damage);
        strncat(battleMessage, damageText, sizeof(battleMessage) - strlen(battleMessage) - 1);

        // Tocar som de ataque e hit
        PlaySound(attackSound);
        PlaySound(hitSound);
    }

    // Verificar e aplicar efeito de status (com chance)
    if (attack->statusEffect > 0 && attack->statusChance > 0) {
        int statusRoll = rand() % 100;
        if (statusRoll < attack->statusChance) {
            applyStatusEffect(defender, attack->statusEffect, attack->statusPower, 3);

            // Adicionar informação de status à mensagem
            char statusText[50];
            switch (attack->statusEffect) {
                case STATUS_ATK_DOWN: sprintf(statusText, " Reduziu o ataque!"); break;
                case STATUS_DEF_DOWN: sprintf(statusText, " Reduziu a defesa!"); break;
                case STATUS_SPD_DOWN: sprintf(statusText, " Reduziu a velocidade!"); break;
                case STATUS_PARALYZED: sprintf(statusText, " Causou paralisia!"); break;
                case STATUS_SLEEPING: sprintf(statusText, " Causou sono!"); break;
                case STATUS_BURNING: sprintf(statusText, " Causou queimadura!"); break;
                default: statusText[0] = '\0'; break;
            }

            strncat(battleMessage, statusText, sizeof(battleMessage) - strlen(battleMessage) - 1);
        }
    }

    // Verificar se o monstro desmaiou
    if (isMonsterFainted(defender)) {
        char faintedText[50];
        sprintf(faintedText, " %s desmaiou!", defender->name);
        strncat(battleMessage, faintedText, sizeof(battleMessage) - strlen(battleMessage) - 1);

        // Tocar som de faint
        PlaySound(faintSound);

        // Se o monstro do jogador desmaiou
        if (defender == battleSystem->playerTeam->current) {
            // Verificar se tem outro monstro disponível
            PokeMonster* current = battleSystem->playerTeam->first;
            bool hasAlivePokemon = false;
            while (current != NULL) {
                if (!isMonsterFainted(current)) {
                    hasAlivePokemon = true;
                    break;
                }
                current = current->next;
            }

            if (hasAlivePokemon) {
                // Forçar troca de monstro
                battleSystem->battleState = BATTLE_FORCED_SWITCH;
                battleSystem->playerTurn = true;
                sprintf(battleMessage, "%s desmaiou! Escolha outro monstro!", defender->name);

                // Limpar a fila de ações
                clearQueue(battleSystem->actionQueue);
            }
        }
        // Se o monstro do bot desmaiou
        else if (defender == battleSystem->opponentTeam->current) {
            // O bot troca automaticamente
            PokeMonster* newMonster = NULL;
            PokeMonster* current = battleSystem->opponentTeam->first;
            while (current != NULL) {
                if (!isMonsterFainted(current)) {
                    newMonster = current;
                    break;
                }
                current = current->next;
            }

            if (newMonster != NULL && newMonster != battleSystem->opponentTeam->current) {
                switchMonster(battleSystem->opponentTeam, newMonster);
                char switchText[64];
                sprintf(switchText, " Oponente enviou %s!", newMonster->name);
                strncat(battleMessage, switchText, sizeof(battleMessage) - strlen(battleMessage) - 1);
            }
        }
    }
}

// Executa a troca de monstro
void executeMonsterSwitch(PokeMonster* monster, int targetIndex) {
    MonsterList* team = NULL;
    bool isPlayer = false;

    // Determinar qual time
    if (monster == battleSystem->playerTeam->current) {
        team = battleSystem->playerTeam;
        isPlayer = true;
    } else {
        team = battleSystem->opponentTeam;
    }

    // Encontrar o monstro alvo
    PokeMonster* newMonster = NULL;
    PokeMonster* current = team->first;
    int count = 0;

    while (current != NULL && count < targetIndex) {
        current = current->next;
        count++;
    }

    if (current != NULL && !isMonsterFainted(current)) {
        newMonster = current;
        switchMonster(team, newMonster);

        // Mensagem de troca
        if (isPlayer) {
            sprintf(battleMessage, "Volte, %s! Vai, %s!",
                    monster->name, newMonster->name);
        } else {
            sprintf(battleMessage, "O oponente trocou para %s!", newMonster->name);
        }
    }
}

// Executa o uso de um item
void executeItemUse(PokeMonster* user, ItemType itemType) {
    useItem(itemType, user);
    // A mensagem já é definida na função useItem
    battleSystem->itemUsed = true;
}

// Define a ordem das ações com base na velocidade
void determineAndExecuteTurnOrder(void) {
    printf("[DEBUG] Ordenando ações. Contagem atual na fila: %d\n", battleSystem->actionQueue->count);

    if (battleSystem == NULL || isQueueEmpty(battleSystem->actionQueue)) {
        return;
    }

    int actions[2];
    int parameters[2];
    PokeMonster* monsters[2];
    int actionCount = 0;

    // Retirar todas as ações da fila
    while (!isQueueEmpty(battleSystem->actionQueue) && actionCount < 2) {
        dequeue(battleSystem->actionQueue, &actions[actionCount],
                &parameters[actionCount], &monsters[actionCount]);
        actionCount++;
    }

    if (actionCount == 2) {
        // Ordem de prioridade:
        // 1. Trocas (action = 1) - SEMPRE primeiro
        // 2. Itens (action = 2)
        // 3. Ataques (action = 0) ordenados por velocidade

        // Se o segundo é troca e o primeiro não é, inverter
        if (actions[1] == 1 && actions[0] != 1) {
            // Swap
            int tempAction = actions[0];
            int tempParam = parameters[0];
            PokeMonster* tempMonster = monsters[0];

            actions[0] = actions[1];
            parameters[0] = parameters[1];
            monsters[0] = monsters[1];

            actions[1] = tempAction;
            parameters[1] = tempParam;
            monsters[1] = tempMonster;
        }
        // Se ambos não são trocas, verificar itens
        else if (actions[0] != 1 && actions[1] != 1) {
            // Se o segundo é item e o primeiro é ataque, inverter
            if (actions[1] == 2 && actions[0] == 0) {
                // Swap
                int tempAction = actions[0];
                int tempParam = parameters[0];
                PokeMonster* tempMonster = monsters[0];

                actions[0] = actions[1];
                parameters[0] = parameters[1];
                monsters[0] = monsters[1];

                actions[1] = tempAction;
                parameters[1] = tempParam;
                monsters[1] = tempMonster;
            }
            // Se ambos são ataques, ordenar por velocidade
            else if (actions[0] == 0 && actions[1] == 0) {
                if (monsters[0]->speed < monsters[1]->speed) {
                    // Swap
                    int tempAction = actions[0];
                    int tempParam = parameters[0];
                    PokeMonster* tempMonster = monsters[0];

                    actions[0] = actions[1];
                    parameters[0] = parameters[1];
                    monsters[0] = monsters[1];

                    actions[1] = tempAction;
                    parameters[1] = tempParam;
                    monsters[1] = tempMonster;
                }
            }
        }
    }

    // Recolocar na fila na ordem correta
    for (int i = 0; i < actionCount; i++) {
        enqueue(battleSystem->actionQueue, actions[i], parameters[i], monsters[i]);
    }
}

// Processa o final do turno (efeitos de status, etc.)
void processTurnEnd(void) {
    if (battleSystem == NULL) {
        return;
    }

    printf("\n[DEBUG TURN] === PROCESSANDO FIM DE TURNO %d ===\n", battleSystem->turn);

    // Resetar flag de item usado para o próximo turno
    battleSystem->itemUsed = false;

    // Processar efeitos de status ativos para o jogador
    if (battleSystem->playerTeam && battleSystem->playerTeam->current) {
        printf("[DEBUG TURN] Processando status do jogador...\n");
        processStatusEffects(battleSystem->playerTeam->current);
    }

    // Processar efeitos de status ativos para o oponente
    if (battleSystem->opponentTeam && battleSystem->opponentTeam->current) {
        printf("[DEBUG TURN] Processando status do oponente...\n");
        processStatusEffects(battleSystem->opponentTeam->current);
    }

    printf("[DEBUG TURN] ==============================\n\n");
}

// Processa efeitos de status no final do turno
void processStatusEffects(PokeMonster* monster) {
    if (monster == NULL || monster->statusCondition == STATUS_NONE) {
        return;
    }

    printf("\n[DEBUG STATUS] === Processando Status ===\n");
    printf("[DEBUG STATUS] Monstro: %s\n", monster->name);
    printf("[DEBUG STATUS] Status atual: %d\n", monster->statusCondition);
    printf("[DEBUG STATUS] Turnos restantes ANTES: %d\n", monster->statusTurns);

    // Primeiro, aplicar efeitos do status atual
    switch (monster->statusCondition) {
        case STATUS_BURNING:
            {
                int damage = monster->maxHp / 8;
                if (damage < 1) damage = 1;
                monster->hp -= damage;

                if (monster->hp < 0) monster->hp = 0;

                sprintf(battleMessage, "%s sofreu %d de dano por estar em chamas!",
                       monster->name, damage);
            }
            break;

        case STATUS_SLEEPING:
            printf("[DEBUG STATUS] %s está dormindo (não pode atacar)\n", monster->name);
            break;

        case STATUS_PARALYZED:
            printf("[DEBUG STATUS] %s está paralisado\n", monster->name);
            break;
    }

    // Decrementar turnos restantes
    if (monster->statusTurns > 0) {
        monster->statusTurns--;
        printf("[DEBUG STATUS] Turnos restantes DEPOIS do decremento: %d\n", monster->statusTurns);

        // Se chegou a 0, remover o status
        if (monster->statusTurns == 0) {
            printf("[DEBUG STATUS] *** STATUS DEVE EXPIRAR AGORA ***\n");

            // Mensagem específica para cada status
            switch (monster->statusCondition) {
                case STATUS_SLEEPING:
                    sprintf(battleMessage, "%s acordou!", monster->name);
                    printf("[DEBUG STATUS] ACORDANDO %s\n", monster->name);
                    break;
                case STATUS_PARALYZED:
                    sprintf(battleMessage, "%s não está mais paralisado!", monster->name);
                    monster->speed *= 2;
                    break;
                case STATUS_BURNING:
                    sprintf(battleMessage, "%s não está mais em chamas!", monster->name);
                    break;
                default:
                    sprintf(battleMessage, "O status de %s acabou!", monster->name);
                    break;
            }

            // Remover o status
            monster->statusCondition = STATUS_NONE;
            printf("[DEBUG STATUS] Status removido! Novo status: %d\n", monster->statusCondition);
        }
    }
    printf("[DEBUG STATUS] ===========================\n\n");
}

// Aplica efeitos de status
void applyStatusEffect(PokeMonster* target, int statusEffect, int statusPower, int duration) {
    if (target == NULL || statusEffect <= 0) {
        return;
    }

    // Se duration não foi especificado, usar valores padrão
    if (duration <= 0) {
        switch (statusEffect) {
            case STATUS_SLEEPING:
                duration = 2 + rand() % 2; // 2-3 turnos
                break;
            case STATUS_BURNING:
                duration = 3 + rand() % 3; // 3-5 turnos
                break;
            case STATUS_PARALYZED:
                duration = 2 + rand() % 2; // 2-3 turnos
                break;
            default:
                duration = 3; // Padrão 3 turnos
                break;
        }
    }

    printf("\n[DEBUG APPLY] === APLICANDO STATUS ===\n");
    printf("[DEBUG APPLY] Alvo: %s\n", target->name);
    printf("[DEBUG APPLY] Status: %d\n", statusEffect);
    printf("[DEBUG APPLY] Duração: %d turnos\n", duration);

    // Status principais não se sobrepõem
    if (target->statusCondition > STATUS_SPD_DOWN &&
        statusEffect > STATUS_SPD_DOWN &&
        target->statusCondition != STATUS_NONE) {
        printf("[DEBUG APPLY] BLOQUEADO: %s já tem status %d\n",
               target->name, target->statusCondition);
        return;
    }

    // Aplicar o status
    target->statusCondition = statusEffect;
    target->statusTurns = duration;
    target->statusCounter = 0; // Resetar contador

    printf("[DEBUG APPLY] Status aplicado com sucesso!\n");
    printf("[DEBUG APPLY] =========================\n\n");

    // Aplicar o efeito imediato
    switch (statusEffect) {
        case STATUS_ATK_DOWN: // Reduzir ataque
            target->attack = (int)(target->attack * (100 - statusPower) / 100.0f);
            break;
        case STATUS_DEF_DOWN: // Reduzir defesa
            target->defense = (int)(target->defense * (100 - statusPower) / 100.0f);
            break;
        case STATUS_SPD_DOWN: // Reduzir velocidade
            target->speed = (int)(target->speed * (100 - statusPower) / 100.0f);
            break;
        case STATUS_PARALYZED: // Paralisia (reduz velocidade)
            target->speed = (int)(target->speed * 0.5f);
            break;
        case STATUS_SLEEPING: // Dormindo (nada extra a fazer)
        case STATUS_BURNING:  // Em chamas (dano no final do turno)
            break;
        default:
            break;
    }

    // Salvar na pilha para processamento futuro
    push(battleSystem->effectStack, statusEffect, duration, statusPower, target);
}

/**
 * Inicializa a árvore de decisão do bot
 */
void initBotDecisionTree(void) {
    if (botDecisionTree != NULL) {
        freeDecisionTree(botDecisionTree);
    }

    botDecisionTree = createBotDecisionTree();
}

/**
 * Libera o sistema de batalha
 */
void freeBattleSystem(void) {
    if (battleSystem == NULL) {
        return;
    }

    // Liberar filas e pilhas
    if (battleSystem->actionQueue) {
        freeActionQueue(battleSystem->actionQueue);
    }

    if (battleSystem->effectStack) {
        freeEffectStack(battleSystem->effectStack);
    }

    // Não liberar os times aqui, pois eles são gerenciados externamente

    // Liberar o próprio sistema
    free(battleSystem);
    battleSystem = NULL;
}

/**
 * Sorteia um tipo de item aleatório para a batalha
 */
ItemType rollRandomItem(void) {
    int roll = rand() % 3; // 0, 1 ou 2
    return (ItemType)roll;
}

/**
 * Verifica se a batalha acabou
 */
bool isBattleOver(void) {
    if (battleSystem == NULL ||
        battleSystem->playerTeam == NULL ||
        battleSystem->opponentTeam == NULL) {
        return true;
    }

    // Verificar se todos os monstros de um time estão incapacitados
    bool playerHasActiveMontser = false;
    bool opponentHasActiveMonster = false;

    // Verificar time do jogador
    PokeMonster* current = battleSystem->playerTeam->first;
    while (current != NULL) {
        if (!isMonsterFainted(current)) {
            playerHasActiveMontser = true;
            break;
        }
        current = current->next;
    }

    // Verificar time do oponente
    current = battleSystem->opponentTeam->first;
    while (current != NULL) {
        if (!isMonsterFainted(current)) {
            opponentHasActiveMonster = true;
            break;
        }
        current = current->next;
    }

    return (!playerHasActiveMontser || !opponentHasActiveMonster);
}

/**
 * Determina o vencedor da batalha
 */
int getBattleWinner(void) {
    if (battleSystem == NULL ||
        battleSystem->playerTeam == NULL ||
        battleSystem->opponentTeam == NULL) {
        return 0;
    }

    bool playerHasActiveMontser = false;
    bool opponentHasActiveMonster = false;

    // Verificar time do jogador
    PokeMonster* current = battleSystem->playerTeam->first;
    while (current != NULL) {
        if (!isMonsterFainted(current)) {
            playerHasActiveMontser = true;
            break;
        }
        current = current->next;
    }

    // Verificar time do oponente
    current = battleSystem->opponentTeam->first;
    while (current != NULL) {
        if (!isMonsterFainted(current)) {
            opponentHasActiveMonster = true;
            break;
        }
        current = current->next;
    }

    if (!playerHasActiveMontser && !opponentHasActiveMonster) {
        return 0; // Empate (raro)
    } else if (!playerHasActiveMontser) {
        return 2; // Oponente venceu
    } else if (!opponentHasActiveMonster) {
        return 1; // Jogador venceu
    }

    return 0; // Batalha ainda em andamento
}

/**
 * Escolhe uma ação para o bot
 */
void botChooseAction(void) {
    printf("[DEBUG BOT] Iniciando botChooseAction, playerTurn=%s\n", battleSystem->playerTurn ? "true" : "false");
    if (battleSystem == NULL ||
        battleSystem->opponentTeam == NULL || battleSystem->opponentTeam->current == NULL ||
        battleSystem->playerTeam == NULL || battleSystem->playerTeam->current == NULL) {
        printf("[DEBUG BOT] Erro: ponteiros inválidos\n");
        return;
    }

    PokeMonster* botMonster = battleSystem->opponentTeam->current;
    PokeMonster* playerMonster = battleSystem->playerTeam->current;

    printf("[DEBUG BOT] Bot escolhendo ação usando árvore de decisão...\n");

    // Simples por enquanto - sempre atacar
    int attackIndex = botChooseAttack(botMonster, playerMonster);
    enqueue(battleSystem->actionQueue, 0, attackIndex, botMonster);
    printf("[DEBUG BOT] Bot enfileirou ataque %d\n", attackIndex);

    // Verificar se ambos jogadores fizeram suas escolhas
    printf("[DEBUG BOT] Ações na fila: %d\n", battleSystem->actionQueue->count);
    if (battleSystem->actionQueue->count >= 2) {
        actionQueueReady = true;
        battleSystem->battleState = BATTLE_PREPARING_ACTIONS;
        printf("[DEBUG BOT] Ambos jogadores escolheram, indo para PREPARING_ACTIONS\n");
    }
}

/**
 * Escolhe um ataque para o bot
 */
int botChooseAttack(PokeMonster* botMonster, PokeMonster* playerMonster) {
    if (botMonster == NULL || playerMonster == NULL) {
        return 0;
    }

    // Por enquanto retorna apenas o primeiro ataque válido
    for (int i = 0; i < 4; i++) {
        if (botMonster->attacks[i].ppCurrent > 0) {
            return i;
        }
    }

    return 0; // Fallback: primeiro ataque
}

/**
 * Escolhe um monstro para o bot
 */
PokeMonster* botChooseMonster(MonsterList* botTeam, PokeMonster* playerMonster) {
    if (botTeam == NULL || playerMonster == NULL) {
        return NULL;
    }

    // Escolher o primeiro monstro que não esteja desmaiado
    PokeMonster* current = botTeam->first;
    while (current != NULL) {
        if (!isMonsterFainted(current) && current != botTeam->current) {
            return current;
        }
        current = current->next;
    }

    return botTeam->current; // Se não encontrar outro, retorna o atual
}

/**
 * Calcula o dano de um ataque
 */
int calculateDamage(PokeMonster* attacker, PokeMonster* defender, Attack* attack) {
    if (attacker == NULL || defender == NULL || attack == NULL || attack->power == 0) {
        return 0;
    }

    // Fórmula básica de dano (similar à dos jogos Pokémon):
    // Dano = (((2 * Nível / 5 + 2) * Poder * Ataque / Defesa) / 50 + 2) * Modificadores

    // Como não temos níveis, vamos simplificar:
    float baseDamage = (float)attack->power * attacker->attack / defender->defense;
    baseDamage = (baseDamage / 50.0f) + 2;

    // Modificador de tipo
    float typeModifier = calculateTypeEffectiveness(attack->type, defender->type1, defender->type2);

    // Bônus de Tipo Mesmo (Same Type Attack Bonus - STAB)
    float stabModifier = 1.0f;
    if (attack->type == attacker->type1 || attack->type == attacker->type2) {
        stabModifier = 1.5f;
    }

    // Variação aleatória (85-100%)
    float randomFactor = (float)(85 + rand() % 16) / 100.0f;

    // Cálculo final
    int damage = (int)(baseDamage * typeModifier * stabModifier * randomFactor);

    // Garantir dano mínimo de 1
    if (damage < 1) {
        damage = 1;
    }

    return damage;
}

/**
 * Verifica se um monstro está incapacitado
 */
bool isMonsterFainted(PokeMonster* monster) {
    return (monster == NULL || monster->hp <= 0);
}

/**
 * Troca o monstro atual por outro
 */
void switchMonster(MonsterList* team, PokeMonster* newMonster) {
    if (team == NULL || newMonster == NULL) {
        return;
    }

    team->current = newMonster;
}

/**
 * Usa um item
 */
void useItem(ItemType itemType, PokeMonster* target) {
    if (target == NULL) {
        return;
    }

    switch (itemType) {
        case ITEM_POTION:
            // Calcular quanto de HP pode ser curado
            int hpToHeal = 20; // Máximo que a poção pode curar
            int hpMissing = target->maxHp - target->hp; // Quanto de HP está faltando

            // Curar apenas o necessário
            if (hpMissing < hpToHeal) {
                hpToHeal = hpMissing;
            }

            // Aplicar a cura
            target->hp += hpToHeal;

            // Mensagem informando quanto foi curado
            if (hpToHeal > 0) {
                sprintf(battleMessage, "Poção usada! %s recuperou %d de HP!", target->name, hpToHeal);
            } else {
                sprintf(battleMessage, "Poção usada! %s já está com HP máximo!", target->name);
            }
            break;

        case ITEM_RED_CARD:
            // Força o oponente a trocar de monstro
            if (target == battleSystem->playerTeam->current) {
                // Forçar troca do oponente
                PokeMonster* newMonster = botChooseMonster(battleSystem->opponentTeam, target);
                if (newMonster && newMonster != battleSystem->opponentTeam->current) {
                    switchMonster(battleSystem->opponentTeam, newMonster);
                    sprintf(battleMessage, "Cartão Vermelho usado! Oponente trocou para %s!", newMonster->name);

                    // Limpar a fila de ações do bot para impedir ataques neste turno
                    clearQueue(battleSystem->actionQueue);
                } else {
                    sprintf(battleMessage, "Cartão Vermelho usado, mas falhou!");
                }
            } else {
                sprintf(battleMessage, "Cartão Vermelho usado, mas falhou!");
            }
            break;

        case ITEM_COIN:
            // 50% de chance de curar todo HP, 50% de chance de morrer
            if (rand() % 2 == 0) {
                // Cura total
                target->hp = target->maxHp;
                sprintf(battleMessage, "Moeda da Sorte: CARA! %s recuperou todo o HP!", target->name);
            } else {
                // HP = 0
                target->hp = 0;
                sprintf(battleMessage, "Moeda da Sorte: COROA! %s desmaiou!", target->name);
            }
            break;
    }
    battleSystem->itemUsed = true;
}

/**
 * Versão simplificada para IA quando não quiser usar Gemini
 */
int getAISuggestedActionSimple(PokeMonster* botMonster, PokeMonster* playerMonster) {
    // Decisão simples sem IA avançada
    if (botMonster == NULL || playerMonster == NULL) {
        return 0; // Atacar por padrão
    }

    // Se o monstro atual do bot está com pouca vida
    if (botMonster->hp < botMonster->maxHp * 0.25f) {
        // Tentar usar item (se disponível)
        if (!battleSystem->itemUsed && battleSystem->itemType == ITEM_POTION) {
            return 2;
        }

        // Trocar de monstro se possível
        PokeMonster* newMonster = botChooseMonster(battleSystem->opponentTeam, playerMonster);
        if (newMonster && newMonster != botMonster) {
            return 1;
        }
    }

    return 0; // Atacar por padrão
}

/**
 * Função para inicializar efeitos visuais da batalha
 */
void initBattleEffects(void) {
    // Inicialização básica para evitar erros
    printf("Inicializando efeitos de batalha...\n");
}

/**
 * Reinicia a batalha
 */
void resetBattle(void) {
    if (battleSystem == NULL) {
        return;
    }

    // Resetar estado
    battleSystem->turn = 0;
    battleSystem->battleState = BATTLE_IDLE;
    battleSystem->playerTurn = true;
    battleSystem->selectedAttack = 0;
    battleSystem->selectedAction = 0;
    battleSystem->itemUsed = false;

    // Limpar estruturas de dados
    clearQueue(battleSystem->actionQueue);
    clearStack(battleSystem->effectStack);

    // Limpar mensagem
    battleMessage[0] = '\0';
}