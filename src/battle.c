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
#include "raylib.h"
#include "battle.h"
#include "monsters.h"
#include "ia_integration.h"
#include "resources.h"
#include "globals.h"
#include "battle_renderer.h"
#include "hp_bar.h"



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
    battleSystem->playerItemUsed = false;
    battleSystem->botItemUsed = false;
    battleSystem->itemType = ITEM_POTION; // Padrão

    // Inicializar sistema de barras de HP
    InitHPBarSystem();

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

    // Resetar as animações de barras de HP
    ResetHPBarAnimations();
    resetBattleSprites();

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

    // Atualização global de timers
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
            // Lógica para introdução
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

                    // CORREÇÃO: Se o jogador precisa trocar de Pokémon (Pokémon desmaiado ou Cartão Vermelho)
                    if (isMonsterFainted(battleSystem->playerTeam->current)) {
                        battleSystem->battleState = BATTLE_FORCED_SWITCH;
                        battleSystem->playerTurn = true;
                    }
                    // Se estivermos no início do turno, ir para seleção de ação
                    else if (battleSystem->turn > 0 && isQueueEmpty(battleSystem->actionQueue)) {
                        battleSystem->battleState = BATTLE_SELECT_ACTION;
                        battleSystem->playerTurn = true; // Garantir que o jogador vai jogar
                    }
                    // Se ainda tiver ações na fila, continuar executando
                    else if (!isQueueEmpty(battleSystem->actionQueue)) {
                        battleSystem->battleState = BATTLE_EXECUTING_ACTIONS;
                    }
                    // Caso padrão
                    else {
                        battleSystem->battleState = BATTLE_TURN_END;
                    }
                }
            } else if (currentMessage.autoAdvance &&
                      currentMessage.elapsedTime >= currentMessage.displayTime) {
                // Avançar automaticamente
                // Verificar se o Pokémon do jogador desmaiou
                if (isMonsterFainted(battleSystem->playerTeam->current)) {
                    battleSystem->battleState = BATTLE_FORCED_SWITCH;
                    battleSystem->playerTurn = true;
                }
                else if (!isQueueEmpty(battleSystem->actionQueue)) {
                    battleSystem->battleState = BATTLE_EXECUTING_ACTIONS;
                } else {
                    if (isQueueEmpty(battleSystem->actionQueue) && actionQueueReady) {
                        battleSystem->battleState = BATTLE_TURN_END;
                    } else {
                        battleSystem->battleState = BATTLE_SELECT_ACTION;
                    }
                }
            }
            break;

        case BATTLE_SELECT_ACTION:
            // Verificar primeiro se o Pokémon do jogador desmaiou
            if (isMonsterFainted(battleSystem->playerTeam->current)) {
                battleSystem->battleState = BATTLE_FORCED_SWITCH;
                battleSystem->playerTurn = true;
                return; // Sair imediatamente
            }

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

        case BATTLE_FORCED_SWITCH:
            // Nada a fazer aqui, a interface e os inputs cuidam disso
            // Apenas verificar se o jogador ainda tem algum Pokémon válido
            if (!hasActiveMonstersLeft(battleSystem->playerTeam)) {
                // Jogador não tem mais Pokémon, acabar a batalha
                battleSystem->battleState = BATTLE_OVER;
                strcpy(battleMessage, "Você não tem mais Pokémon! Você perdeu!");

                currentMessage.displayTime = 3.0f;
                currentMessage.elapsedTime = 0.0f;
                currentMessage.waitingForInput = true;
                currentMessage.autoAdvance = false;
                strcpy(currentMessage.message, battleMessage);

                battleSystem->battleState = BATTLE_MESSAGE_DISPLAY;
            }
            break;

        // Outros estados de batalha permanecem os mesmos...
        case BATTLE_PREPARING_ACTIONS:
            // Ordenar ações por velocidade
            determineAndExecuteTurnOrder();
            battleSystem->battleState = BATTLE_EXECUTING_ACTIONS;
            break;

        case BATTLE_EXECUTING_ACTIONS:
    // Resto da implementação existente...
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
                // Verificar se a troca já foi executada pela determineAndExecuteTurnOrder
                // para o caso de ambos trocarem no mesmo turno
                if (strlen(battleMessage) > 0 &&
                    (strstr(battleMessage, "Volte") != NULL ||
                     strstr(battleMessage, "trocou") != NULL)) {
                    // A troca já foi executada, apenas exibir a mensagem
                    currentMessage.displayTime = 1.5f;
                    currentMessage.elapsedTime = 0.0f;
                    currentMessage.waitingForInput = false;
                    currentMessage.autoAdvance = true;
                    strcpy(currentMessage.message, battleMessage);
                    battleSystem->battleState = BATTLE_MESSAGE_DISPLAY;
                } else {
                    // Executar a troca normalmente
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
                }
                break;

            case 2: // Item
                executeItemUse(monster, parameter);

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

            // CORREÇÃO: Verificar primeiro se algum Pokémon desmaiou
            if (isMonsterFainted(battleSystem->playerTeam->current)) {
                battleSystem->battleState = BATTLE_FORCED_SWITCH;
                battleSystem->playerTurn = true;
            } else {
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
            }

            printf("[DEBUG TURN END] Iniciando turno %d, jogador: %s\n",
                  battleSystem->turn, battleSystem->playerTurn ? "SIM" : "NÃO");
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
            // Forçar troca imediatamente
            battleSystem->battleState = BATTLE_FORCED_SWITCH;
            battleSystem->playerTurn = true; // Dar controle ao jogador

            // Limpar ações pendentes
            clearQueue(battleSystem->actionQueue);
            actionQueueReady = false;

            // Atualizar mensagem
            strcpy(battleMessage, "Seu Pokémon desmaiou! Escolha outro!");
            currentMessage.displayTime = 3.0f;
            currentMessage.waitingForInput = false;
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

/**
 * Executa a troca de monstro
 */
void executeMonsterSwitch(PokeMonster* monster, int targetIndex) {
    MonsterList* team = NULL;
    bool isPlayer = false;

    // Determinar qual time
    if (battleSystem->playerTeam && monster == battleSystem->playerTeam->current) {
        team = battleSystem->playerTeam;
        isPlayer = true;
    } else if (battleSystem->opponentTeam && monster == battleSystem->opponentTeam->current) {
        team = battleSystem->opponentTeam;
    } else {
        // Aqui ocorre o problema - o monster não corresponde a nenhum dos atuais
        // Vamos tentar deduzir o time de outra forma
        if (monster) {
            printf("DEBUG: Monster %s não é o atual de nenhum time, detectando time...\n", monster->name);

            // Verificar se monster pertence ao time do jogador
            PokeMonster* current = battleSystem->playerTeam->first;
            while (current) {
                if (current == monster) {
                    team = battleSystem->playerTeam;
                    isPlayer = true;
                    printf("DEBUG: Monster %s pertence ao time do jogador\n", monster->name);
                    break;
                }
                current = current->next;
            }

            // Se não encontrou no time do jogador, verificar no time do oponente
            if (team == NULL) {
                current = battleSystem->opponentTeam->first;
                while (current) {
                    if (current == monster) {
                        team = battleSystem->opponentTeam;
                        printf("DEBUG: Monster %s pertence ao time do oponente\n", monster->name);
                        break;
                    }
                    current = current->next;
                }
            }
        }
    }

    if (team == NULL) {
        printf("ERRO: Não foi possível determinar o time em executeMonsterSwitch\n");
        return;
    }

    // IMPORTANTE: Armazenar explicitamente o nome do monstro que está saindo da batalha
    char currentMonsterName[32] = {0};
    if (team->current) {
        strcpy(currentMonsterName, team->current->name);
    } else {
        strcpy(currentMonsterName, "???");
    }

    printf("DEBUG: Trocando %s (time %s) para monstro no índice %d\n",
           currentMonsterName, isPlayer ? "jogador" : "oponente", targetIndex);

    // Encontrar o monstro alvo pelo índice
    PokeMonster* newMonster = NULL;
    PokeMonster* current = team->first;
    int count = 0;

    while (current != NULL) {
        if (count == targetIndex) {
            newMonster = current;
            break;
        }
        current = current->next;
        count++;
    }

    // Verificar se encontrou o monstro e se ele não está desmaiado
    if (newMonster != NULL && !isMonsterFainted(newMonster)) {
        // Trocar para o novo monstro
        switchMonster(team, newMonster);

        // Formatar mensagem de troca - usando o nome armazenado
        if (isPlayer) {
            sprintf(battleMessage, "Vai, %s!",newMonster->name);
        } else {
            sprintf(battleMessage, "O oponente trocou para %s!", newMonster->name);
        }

        printf("DEBUG: Troca realizada de %s para %s (isPlayer=%d)\n",
               currentMonsterName, newMonster->name, isPlayer);
    } else {
        // Informar erro ao trocar
        if (newMonster == NULL) {
            printf("ERRO: Não foi possível encontrar o monstro com índice %d\n", targetIndex);
            sprintf(battleMessage, "Não foi possível realizar a troca!");
        } else if (isMonsterFainted(newMonster)) {
            printf("ERRO: Tentativa de trocar para monstro desmaiado: %s\n", newMonster->name);
            sprintf(battleMessage, "%s está desmaiado e não pode entrar em batalha!", newMonster->name);
        }
    }
}



/**
 * Executa o uso de um item
 */
void executeItemUse(PokeMonster* user, ItemType itemType) {
    // Usar o item no usuário apropriado
    useItem(itemType, user);

    // A mensagem já é definida na função useItem

    // Se o item foi usado pelo bot/oponente e foi um Cartão Vermelho,
    // ele afetará o jogador, não o bot
    if (user == battleSystem->opponentTeam->current && itemType == ITEM_RED_CARD) {
        // A lógica específica já está em useItem
    }
}

// Define a ordem das ações com base na velocidade
void determineAndExecuteTurnOrder(void) {
    printf("[DEBUG] Ordenando ações. Contagem atual na fila: %d\n", battleSystem->actionQueue->count);

    if (battleSystem == NULL || isQueueEmpty(battleSystem->actionQueue)) {
        return;
    }

    int actions[2] = {-1, -1};        // Tipo da ação: 0=ataque, 1=troca, 2=item
    int parameters[2] = {-1, -1};     // Parâmetro da ação (índice do ataque, do monstro, ou do item)
    PokeMonster* monsters[2] = {NULL, NULL};  // Quem está executando a ação
    MonsterList* teams[2] = {NULL, NULL};     // Time do executor
    int actionCount = 0;

    // Retirar todas as ações da fila
    while (!isQueueEmpty(battleSystem->actionQueue) && actionCount < 2) {
        dequeue(battleSystem->actionQueue, &actions[actionCount],
                &parameters[actionCount], &monsters[actionCount]);

        // Determinar qual time o monstro pertence
        if (monsters[actionCount] != NULL) {
            if (battleSystem->playerTeam &&
                monsters[actionCount] == battleSystem->playerTeam->current) {
                teams[actionCount] = battleSystem->playerTeam;
            } else if (battleSystem->opponentTeam &&
                      monsters[actionCount] == battleSystem->opponentTeam->current) {
                teams[actionCount] = battleSystem->opponentTeam;
            }
        }

        actionCount++;
    }

    printf("[DEBUG] Extraídas %d ações da fila\n", actionCount);

    // Caso especial: Ambos trocam Pokémon no mesmo turno
    if (actionCount == 2 && actions[0] == 1 && actions[1] == 1) {
        printf("[DEBUG] Caso especial: Ambos trocam Pokémon no mesmo turno\n");

        // Guardar os monstros atuais para referência na mensagem
        PokeMonster* player1Current = teams[0]->current;
        PokeMonster* player2Current = teams[1]->current;

        // Executar ambas as trocas
        executeMonsterSwitch(monsters[0], parameters[0]);

        // Salvar a mensagem da primeira troca
        char firstSwitchMessage[256];
        strcpy(firstSwitchMessage, battleMessage);

        // Executar a segunda troca
        executeMonsterSwitch(monsters[1], parameters[1]);

        // Combinar as mensagens
        char combinedMessage[256];
        sprintf(combinedMessage, "%s\n%s", firstSwitchMessage, battleMessage);
        strcpy(battleMessage, combinedMessage);

        return;  // Ambas as trocas foram executadas, sair da função
    }

    // Caso normal: Ordem de prioridade padrão
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
            MonsterList* tempTeam = teams[0];

            actions[0] = actions[1];
            parameters[0] = parameters[1];
            monsters[0] = monsters[1];
            teams[0] = teams[1];

            actions[1] = tempAction;
            parameters[1] = tempParam;
            monsters[1] = tempMonster;
            teams[1] = tempTeam;

            printf("[DEBUG] Invertendo ordem: troca vai primeiro\n");
        }
        // Se ambos não são trocas, verificar itens
        else if (actions[0] != 1 && actions[1] != 1) {
            // Se o segundo é item e o primeiro é ataque, inverter
            if (actions[1] == 2 && actions[0] == 0) {
                // Swap
                int tempAction = actions[0];
                int tempParam = parameters[0];
                PokeMonster* tempMonster = monsters[0];
                MonsterList* tempTeam = teams[0];

                actions[0] = actions[1];
                parameters[0] = parameters[1];
                monsters[0] = monsters[1];
                teams[0] = teams[1];

                actions[1] = tempAction;
                parameters[1] = tempParam;
                monsters[1] = tempMonster;
                teams[1] = tempTeam;

                printf("[DEBUG] Invertendo ordem: item vai primeiro\n");
            }
            // Se ambos são ataques, ordenar por velocidade
            else if (actions[0] == 0 && actions[1] == 0) {
                if (monsters[0]->speed < monsters[1]->speed) {
                    // Swap
                    int tempAction = actions[0];
                    int tempParam = parameters[0];
                    PokeMonster* tempMonster = monsters[0];
                    MonsterList* tempTeam = teams[0];

                    actions[0] = actions[1];
                    parameters[0] = parameters[1];
                    monsters[0] = monsters[1];
                    teams[0] = teams[1];

                    actions[1] = tempAction;
                    parameters[1] = tempParam;
                    monsters[1] = tempMonster;
                    teams[1] = tempTeam;

                    printf("[DEBUG] Invertendo ordem: monstro mais rápido ataca primeiro\n");
                }
            }
        }
    }

    // Recolocar na fila na ordem correta
    for (int i = 0; i < actionCount; i++) {
        enqueue(battleSystem->actionQueue, actions[i], parameters[i], monsters[i]);
        printf("[DEBUG] Recolocando ação %d na fila: tipo=%d, param=%d\n",
               i, actions[i], parameters[i]);
    }
}

// Processa o final do turno (efeitos de status, etc.)
void processTurnEnd(void) {
    if (battleSystem == NULL) {
        return;
    }

    printf("\n[DEBUG TURN] === PROCESSANDO FIM DE TURNO %d ===\n", battleSystem->turn);

    // Resetar flag de item usado para o próximo turno
    battleSystem->playerItemUsed = false;
    battleSystem->botItemUsed = false;

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
// Escolhe uma ação para o bot usando IA Gemini com fallback simples
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

    // Decidir qual ação tomar usando a IA ou fallback
    int action = getAISuggestedAction(botMonster, playerMonster);

    // Com base na ação escolhida pela IA
    switch (action) {
        case 0: // Atacar
        {
            int attackIndex = getAISuggestedAttack(botMonster, playerMonster);
            printf("[DEBUG BOT] Bot vai atacar usando ataque %d\n", attackIndex);
            enqueue(battleSystem->actionQueue, 0, attackIndex, botMonster);
        }
        break;

        case 1: // Trocar
        {
            int monsterIndex = getAISuggestedMonster(battleSystem->opponentTeam, playerMonster);
            printf("[DEBUG BOT] Bot vai trocar para monstro %d\n", monsterIndex);

            // IMPORTANTE: Usar o monstro atual como parâmetro
            PokeMonster* currentMonster = battleSystem->opponentTeam->current;
            enqueue(battleSystem->actionQueue, 1, monsterIndex, currentMonster);

            // Opcional: Atualizar o monstro atual para atualizar a UI
            // Este código assumindo que getAISuggestedMonster retorna o índice do monstro
            PokeMonster* newMonster = NULL;
            PokeMonster* temp = battleSystem->opponentTeam->first;
            int count = 0;

            while (temp != NULL && count < monsterIndex) {
                temp = temp->next;
                count++;
            }

            if (temp != NULL && !isMonsterFainted(temp)) {
                switchMonster(battleSystem->opponentTeam, temp);
            }
        }
            break;

        case 2: // Usar item
        {
            // Selecionar o item com base na situação
            ItemType itemType = battleSystem->itemType;

            // Alterar a lógica para determinar qual item usar com mais inteligência
            if (botMonster->hp < botMonster->maxHp * 0.4f) {
                // Se HP está baixo (< 40%), preferir Poção
                itemType = ITEM_POTION;
            }
            else if (botMonster->hp > botMonster->maxHp * 0.7f) {
                // Se o HP está relativamente alto, considerar usar Cartão Vermelho
                // O Cartão Vermelho é mais útil se o Pokémon do jogador é forte
                if (battleSystem->itemType == ITEM_RED_CARD &&
                    (playerMonster->hp > playerMonster->maxHp * 0.5f)) {
                    itemType = ITEM_RED_CARD;
                }
            }
            // Caso contrário, usar o item padrão definido para a batalha

            printf("[DEBUG BOT] Bot vai usar item %d\n", itemType);
            enqueue(battleSystem->actionQueue, 2, itemType, botMonster);

            // Importante: NÃO marcar o item como usado aqui.
            // Isso será feito quando a ação for realmente executada.
        }
        break;

        default: // Fallback - sempre atacar
        {
            int attackIndex = botChooseAttack(botMonster, playerMonster);
            printf("[DEBUG BOT] Bot vai atacar (fallback) usando ataque %d\n", attackIndex);
            enqueue(battleSystem->actionQueue, 0, attackIndex, botMonster);
        }
        break;
    }

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
/**
 * Escolhe um ataque para o bot (usado pelo sistema de fallback)
 */
int botChooseAttack(PokeMonster* botMonster, PokeMonster* playerMonster) {
    if (botMonster == NULL || playerMonster == NULL) {
        return 0; // Primeiro ataque como padrão
    }

    // Se temos um ataque previamente determinado pelo getAISuggestedActionSimple
    if (botMonster->statusCounter >= 0 && botMonster->statusCounter < 4 &&
        botMonster->attacks[botMonster->statusCounter].ppCurrent > 0) {
        return botMonster->statusCounter;
        }

    // Fallback: encontrar o primeiro ataque com PP
    for (int i = 0; i < 4; i++) {
        if (botMonster->attacks[i].ppCurrent > 0) {
            return i;
        }
    }

    return 0; // Primeiro ataque como última opção
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
        printf("ERRO: Tentativa de usar item com target NULL\n");
        return;
    }

    printf("[useItem] Usando item %d no monstro %s\n", itemType, target->name);

    switch (itemType) {
        case ITEM_POTION:
            // Calcular quanto de HP pode ser curado
        {
            int hpToHeal = 20; // Máximo que a poção pode curar
            int hpMissing = target->maxHp - target->hp; // Quanto de HP está faltando

            // Curar apenas o necessário
            if (hpMissing < hpToHeal) {
                hpToHeal = hpMissing;
            }

            // Aplicar a cura
            target->hp += hpToHeal;

            // Garantir que o HP não ultrapasse o máximo
            if (target->hp > target->maxHp) {
                target->hp = target->maxHp;
            }

            // Mensagem informando quanto foi curado
            if (hpToHeal > 0) {
                sprintf(battleMessage, "Poção usada! %s recuperou %d de HP!", target->name, hpToHeal);
            } else {
                sprintf(battleMessage, "Poção usada! %s já está com HP máximo!", target->name);
            }

            // Debug para verificar o HP após usar a poção
            printf("[useItem] Após usar poção: %s HP = %d/%d\n", target->name, target->hp, target->maxHp);
        }
            break;

        case ITEM_RED_CARD:
            {
                // Importante: determinar quem usou o item e a quem afeta
                // Se o item é usado pelo jogador, afeta o pokemon do oponente (bot)
                // Se o item é usado pelo oponente (bot), afeta o pokemon do jogador

                PokeMonster* affectedMonster = NULL;
                MonsterList* affectedTeam = NULL;

                // Verificar quem usou o item e quem deve ser afetado
                if (target == battleSystem->playerTeam->current) {
                    // Jogador usou o item, afeta o oponente
                    affectedMonster = battleSystem->opponentTeam->current;
                    affectedTeam = battleSystem->opponentTeam;
                    sprintf(battleMessage, "Cartão Vermelho usado! Forçando o oponente a trocar!");
                } else {
                    // Oponente usou o item, afeta o jogador
                    affectedMonster = battleSystem->playerTeam->current;
                    affectedTeam = battleSystem->playerTeam;
                    sprintf(battleMessage, "Oponente usou Cartão Vermelho! Você precisa trocar de Pokémon!");
                }

                // Verificar se há outros monstros disponíveis para troca
                bool hasValidMonsters = false;
                PokeMonster* current = affectedTeam->first;

                while (current != NULL) {
                    if (!isMonsterFainted(current) && current != affectedMonster) {
                        hasValidMonsters = true;
                        break;
                    }
                    current = current->next;
                }

                if (hasValidMonsters) {
                    // Se o jogador foi afetado, forçar tela de seleção
                    if (affectedTeam == battleSystem->playerTeam) {
                        battleSystem->battleState = BATTLE_FORCED_SWITCH;
                        battleSystem->playerTurn = true;

                        // Limpar ações pendentes para dar prioridade à troca
                        clearQueue(battleSystem->actionQueue);

                        // Atualizar mensagem para ser mais clara
                        strcpy(battleMessage, "Oponente usou Cartão Vermelho! Escolha outro Pokémon!");
                    } else {
                        // Bot é afetado, escolher automaticamente outro monstro
                        PokeMonster* newMonster = NULL;

                        // Escolher o primeiro monstro não desmaiado que não seja o atual
                        current = affectedTeam->first;
                        while (current != NULL) {
                            if (!isMonsterFainted(current) && current != affectedMonster) {
                                newMonster = current;
                                break;
                            }
                            current = current->next;
                        }

                        if (newMonster) {
                            switchMonster(affectedTeam, newMonster);
                            sprintf(battleMessage, "Cartão Vermelho usado! Oponente trocou para %s!", newMonster->name);
                        }
                    }
                } else {
                    // Não há outros monstros para trocar
                    sprintf(battleMessage, "Cartão Vermelho usado, mas não há outros Pokémon disponíveis!");
                }
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

                // Se o jogador desmaiou, forçar troca
                if (target == battleSystem->playerTeam->current) {
                    battleSystem->battleState = BATTLE_FORCED_SWITCH;
                    battleSystem->playerTurn = true;
                }
            }
            break;

        default:
            sprintf(battleMessage, "Item desconhecido usado!");
            break;
    }

    // Marcar o item como usado pelo usuário correto
    if (target == battleSystem->playerTeam->current) {
        printf("[useItem] Item usado pelo jogador\n");
        battleSystem->playerItemUsed = true;
    } else if (target == battleSystem->opponentTeam->current) {
        printf("[useItem] Item usado pelo oponente\n");
        battleSystem->botItemUsed = true;
    }
}

/**
 * Sistema de fallback simples para quando a IA não estiver disponível
 * Faz decisões básicas baseadas no estado do jogo
 */
int getAISuggestedActionSimple(PokeMonster* botMonster, PokeMonster* playerMonster) {
    if (botMonster == NULL || playerMonster == NULL) {
        return 0; // Atacar por padrão
    }

    // Calcular porcentagem de HP
    float botHpPercent = (float)botMonster->hp / botMonster->maxHp * 100.0f;
    float playerHpPercent = (float)playerMonster->hp / playerMonster->maxHp * 100.0f;

    // 1. Verificar se há necessidade de curar (HP baixo)
    if (botHpPercent < 25.0f) {
        // Se tiver item de cura e ele não foi usado ainda neste turno
        if (!battleSystem->itemUsed && battleSystem->itemType == ITEM_POTION) {
            return 2; // Usar item
        }

        // Se HP muito baixo e não puder usar item, considerar trocar
        if (botHpPercent < 15.0f) {
            // Verificar se há algum monstro saudável para trocar
            PokeMonster* current = battleSystem->opponentTeam->first;
            while (current != NULL) {
                if (current != botMonster && !isMonsterFainted(current) &&
                    (float)current->hp / current->maxHp > 0.5f) {
                    return 1; // Trocar
                }
                current = current->next;
            }
        }
    }

    // 2. Verificar se tem vantagem de tipo - atacar se tiver
    float bestEffectiveness = 0.0f;
    int bestAttackIndex = -1;

    for (int i = 0; i < 4; i++) {
        if (botMonster->attacks[i].ppCurrent <= 0) continue;

        float effectiveness = calculateTypeEffectiveness(
            botMonster->attacks[i].type,
            playerMonster->type1,
            playerMonster->type2
        );

        if (effectiveness > bestEffectiveness) {
            bestEffectiveness = effectiveness;
            bestAttackIndex = i;
        }
    }

    // Se encontrar um ataque super efetivo (>=2x), usar
    if (bestEffectiveness >= 2.0f && bestAttackIndex >= 0) {
        // Armazenar o melhor ataque para a próxima função
        botMonster->statusCounter = bestAttackIndex; // Usar statusCounter para armazenar temporariamente
        return 0; // Atacar
    }

    // 3. Se o oponente estiver com pouca vida, tentar finalizar
    if (playerHpPercent < 25.0f) {
        // Encontrar o ataque com maior poder
        int highestPower = 0;
        int powerfulAttackIndex = 0;

        for (int i = 0; i < 4; i++) {
            if (botMonster->attacks[i].ppCurrent <= 0) continue;

            if (botMonster->attacks[i].power > highestPower) {
                highestPower = botMonster->attacks[i].power;
                powerfulAttackIndex = i;
            }
        }

        // Armazenar o ataque mais poderoso
        botMonster->statusCounter = powerfulAttackIndex;
        return 0; // Atacar
    }

    // 4. Considerar aplicar efeitos de status
    // Se o inimigo estiver saudável, tentar causar status negativo
    if (playerHpPercent > 50.0f && playerMonster->statusCondition == STATUS_NONE) {
        for (int i = 0; i < 4; i++) {
            if (botMonster->attacks[i].ppCurrent <= 0) continue;

            // Verificar se o ataque causa algum efeito de status
            if (botMonster->attacks[i].statusEffect > 0 &&
                botMonster->attacks[i].statusChance > 30) {
                botMonster->statusCounter = i;
                return 0; // Atacar com movimento de status
            }
        }
    }

    // 5. Opções aleatórias com pesos para adicionar variedade (20% de chance)
    if (rand() % 100 < 20) {
        int action = rand() % 10;

        if (action < 7) { // 70% chance de atacar aleatoriamente
            botMonster->statusCounter = rand() % 4;
            // Verificar se o ataque tem PP
            if (botMonster->attacks[botMonster->statusCounter].ppCurrent <= 0) {
                // Encontrar primeiro ataque com PP
                for (int i = 0; i < 4; i++) {
                    if (botMonster->attacks[i].ppCurrent > 0) {
                        botMonster->statusCounter = i;
                        break;
                    }
                }
            }
            return 0; // Atacar
        }
        else if (action < 9 && !battleSystem->itemUsed) { // 20% chance de usar item
            return 2; // Usar item
        }
        else { // 10% chance de trocar
            return 1; // Trocar
        }
    }

    // 6. Padrão: usar o melhor ataque disponível conforme determinado anteriormente
    if (bestAttackIndex >= 0) {
        botMonster->statusCounter = bestAttackIndex;
    } else {
        // Simplesmente usar o primeiro ataque com PP
        for (int i = 0; i < 4; i++) {
            if (botMonster->attacks[i].ppCurrent > 0) {
                botMonster->statusCounter = i;
                break;
            }
        }
    }

    return 0; // Atacar como ação padrão
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

bool hasActiveMonstersLeft(MonsterList* team) {
    if (team == NULL) {
        return false;
    }

    PokeMonster* current = team->first;
    while (current != NULL) {
        if (!isMonsterFainted(current)) {
            return true;
        }
        current = current->next;
    }

    return false;
}