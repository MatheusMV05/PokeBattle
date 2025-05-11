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
#import  "resources.h"

 char battleMessage[256] = "";
 BattleMessage currentMessage = {0};
 BattleAnimation currentAnimation = {0};
 bool actionQueueReady = false;
 float stateTransitionDelay = 0.0f;



 // Mensagem de batalha atual

void displayBattleMessage(const char* message, float duration, bool waitForInput, bool autoAdvance);
void startAttackAnimation(PokeMonster* attacker, PokeMonster* defender, int attackIndex);
void executeMonsterSwitch(PokeMonster* monster, int targetIndex);
void executeItemUse(PokeMonster* user, ItemType itemType);
void messageDisplayComplete(void);
void executeAttackWithEffects(PokeMonster* attacker, PokeMonster* defender, int attackIndex);

// Sistema de batalha global
BattleSystem* battleSystem = NULL;
 // No início de cada turno 
void startTurn(void) {
    // Limpar a fila de ações anterior
    clearQueue(battleSystem->actionQueue);
    
    // Resetar flags
    actionQueueReady = false;

    // O jogador sempre começa escolhendo sua ação
    battleSystem->playerTurn = true;
    battleSystem->battleState = BATTLE_SELECT_ACTION;
    
    sprintf(battleMessage, "Turno %d - Escolha sua ação!", battleSystem->turn);
}
 
 // Define um turno baseado na velocidade dos monstros
 void determineTurnOrder(void) {
    if (battleSystem == NULL || 
        battleSystem->playerTeam == NULL || battleSystem->playerTeam->current == NULL ||
        battleSystem->opponentTeam == NULL || battleSystem->opponentTeam->current == NULL) {
        return;
    }
    
    // Resetar a fila de ações
    clearQueue(battleSystem->actionQueue);
    
    // Obter os monstros ativos
    PokeMonster* playerMonster = battleSystem->playerTeam->current;
    PokeMonster* opponentMonster = battleSystem->opponentTeam->current;
    
    // Quem tem maior velocidade vai primeiro
    if (playerMonster->speed >= opponentMonster->speed) {
        battleSystem->playerTurn = true;
    } else {
        battleSystem->playerTurn = false;
    }
}

void checkBothPlayersReady(void) {
    // Se ambos escolheram (2 ações na fila), podemos processar
    if (battleSystem->actionQueue != NULL && battleSystem->actionQueue->count >= 2) {
        actionQueueReady = true;
        battleSystem->battleState = BATTLE_PREPARING_ACTIONS;
    }
}
 
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
     
     // Verificar se tudo foi alocado corretamente
     if (battleSystem->actionQueue == NULL || battleSystem->effectStack == NULL) {
         printf("Erro ao alocar componentes do sistema de batalha!\n");
         freeBattleSystem();
         return;
     }
 }

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

            printf("[DEBUG] Troca tem prioridade máxima - ordem ajustada\n");
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

                printf("[DEBUG] Item tem prioridade sobre ataque - ordem ajustada\n");
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

                    printf("[DEBUG] Ordenado por velocidade\n");
                }
            }
        }
    }

    // Recolocar na fila na ordem correta
    for (int i = 0; i < actionCount; i++) {
        enqueue(battleSystem->actionQueue, actions[i], parameters[i], monsters[i]);
    }
}
 
 // Libera o sistema de batalha
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
    battleSystem->itemUsed = false; // Garantir que está false no início
    battleSystem->selectedAttack = 0;
    battleSystem->selectedAction = 0;

    // Escolher um item aleatório para a batalha
    battleSystem->itemType = rollRandomItem();

    // ESCOLHER BACKGROUND ALEATÓRIO
    currentBattleBackground = rand() % BATTLE_BACKGROUNDS_COUNT;
    printf("Background selecionado: %d\n", currentBattleBackground);

    // Limpar estruturas de dados
    clearQueue(battleSystem->actionQueue);
    clearStack(battleSystem->effectStack);
    actionQueueReady = false;

    // Resetar mensagens e animações
    memset(&currentMessage, 0, sizeof(currentMessage));
    memset(&currentAnimation, 0, sizeof(currentAnimation));
    actionQueueReady = false;
    stateTransitionDelay = 0.0f;
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
                battleSystem->battleState = BATTLE_SELECT_ACTION;
                sprintf(battleMessage, "Uma batalha selvagem começou!");
            }
            break;

        case BATTLE_SELECT_ACTION:
            // Aguardando seleção do jogador ou bot
            if (battleSystem->playerTurn) {
                // Jogador está selecionando - Interface cuida disso
            } else {
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
            printf("[DEBUG] Preparando ações...\n");
            determineAndExecuteTurnOrder();
            battleSystem->battleState = BATTLE_EXECUTING_ACTIONS;
            break;

        case BATTLE_EXECUTING_ACTIONS:
            // Se estamos mostrando uma mensagem, esperar
            if (battleSystem->battleState == BATTLE_MESSAGE_DISPLAY) {
                break;
            }

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

                        // IMPORTANTE: Aguardar um frame antes de mostrar mensagem
                        if (strlen(battleMessage) > 0) {
                            displayBattleMessage(battleMessage, 1.5f, false, true);
                        } else {
                            // Se não houver mensagem, continuar
                            sprintf(battleMessage, "%s usou um ataque!", monster->name);
                            displayBattleMessage(battleMessage, 1.0f, false, true);
                        }
                    }
                        break;

                    case 1: // Troca
                        executeMonsterSwitch(monster, parameter);
                        break;

                    case 2: // Item
                        executeItemUse(monster, battleSystem->itemType);
                        break;
                }
            } else {
                // Todas ações executadas, fim de turno
                printf("[DEBUG] Fila vazia, indo para BATTLE_TURN_END\n");
                battleSystem->battleState = BATTLE_TURN_END;
            }
            break;

        case BATTLE_MESSAGE_DISPLAY:
            // Atualizar mensagem atual
            currentMessage.elapsedTime += deltaTime;

            if (currentMessage.autoAdvance &&
                currentMessage.elapsedTime >= currentMessage.displayTime) {
                // Avançar automaticamente
                printf("[DEBUG] Mensagem completada automaticamente\n");
                messageDisplayComplete();
            } else if (currentMessage.waitingForInput &&
                       currentMessage.elapsedTime > 0.5f &&
                       (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) ||
                        IsKeyPressed(KEY_SPACE) ||
                        IsKeyPressed(KEY_ENTER))) {
                // Avançar com input do jogador
                printf("[DEBUG] Mensagem completada por input\n");
                messageDisplayComplete();
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
            battleSystem->playerTurn = true;
            battleSystem->battleState = BATTLE_SELECT_ACTION;

            sprintf(battleMessage, "Turno %d - Escolha sua ação!", battleSystem->turn);
            printf("[DEBUG TURN END] Iniciando turno %d\n", battleSystem->turn);
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

// Nova função para quando a mensagem termina
void messageDisplayComplete(void) {
    printf("[DEBUG] messageDisplayComplete chamada\n");

    // Limpar a mensagem atual
    memset(&currentMessage, 0, sizeof(currentMessage));

    // Voltar para execução de ações ou próxima etapa
    if (!isQueueEmpty(battleSystem->actionQueue)) {
        printf("[DEBUG] Ainda há ações na fila, voltando para EXECUTING_ACTIONS\n");
        battleSystem->battleState = BATTLE_EXECUTING_ACTIONS;
    } else {
        printf("[DEBUG] Fila vazia, indo para BATTLE_TURN_END\n");
        battleSystem->battleState = BATTLE_TURN_END;
    }
}
 
 // Processa a entrada do jogador durante a batalha
 void processBattleInput(void) {
     if (battleSystem == NULL) {
         return;
     }
     
     // Processar entrada baseada no estado atual
     switch (battleSystem->battleState) {
         case BATTLE_SELECT_ACTION:
             // Jogador escolhe entre Lutar, Trocar, Item, Fugir
             if (battleSystem->playerTurn) {
                 // A UI chama as funções apropriadas baseadas nos botões clicados
             }
             break;
             
         case BATTLE_SELECT_ATTACK:
             // Jogador escolhe um dos 4 ataques
             if (battleSystem->playerTurn) {
                 // A UI chama as funções apropriadas baseadas nos botões clicados
             }
             break;
             
         case BATTLE_SELECT_MONSTER:
             // Jogador escolhe um monstro para trocar
             if (battleSystem->playerTurn) {
                 // A UI chama as funções apropriadas baseadas nos botões clicados
             }
             break;
             
         case BATTLE_ITEM_MENU:
             // Jogador escolhe usar o item ou não
             if (battleSystem->playerTurn) {
                 // A UI chama as funções apropriadas baseadas nos botões clicados
             }
             break;
             
         case BATTLE_RESULT_MESSAGE:
             // Aguardar confirmação do jogador para continuar
             if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                 // Se não houver mais ações na fila, voltar para seleção de ação
                 if (isQueueEmpty(battleSystem->actionQueue)) {
                     battleSystem->battleState = BATTLE_SELECT_ACTION;
                 } else {
                     battleSystem->battleState = BATTLE_EXECUTING_ACTIONS;
                 }
             }
             break;
             
         case BATTLE_CONFIRM_QUIT:
             // Confirmação para desistir
             // A UI chama as funções apropriadas baseadas nos botões clicados
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
        printf("[DEBUG] %s está dormindo e não pode atacar!\n", attacker->name);
        return; // IMPORTANTE: não mudar o estado aqui
    }

    if (attacker->statusCondition == STATUS_PARALYZED) {
        // 25% de chance de não conseguir atacar
        if (rand() % 100 < 25) {
            sprintf(battleMessage, "%s está paralisado e não conseguiu atacar!", attacker->name);
            printf("[DEBUG] %s está paralisado e não conseguiu atacar!\n", attacker->name);
            return; // IMPORTANTE: não mudar o estado aqui
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
    }
    
    // Verificar e aplicar efeito de status (com chance)
    if (attack->statusEffect > 0 && attack->statusChance > 0) {
        int statusRoll = rand() % 100;
        if (statusRoll < attack->statusChance) {
            applyStatusEffect(defender, attack->statusEffect, attack->statusPower, 3);
            
            // Adicionar informação de status à mensagem
            char statusText[50];
            switch (attack->statusEffect) {
                case 1: sprintf(statusText, " Reduziu o ataque!"); break;
                case 2: sprintf(statusText, " Reduziu a defesa!"); break;
                case 3: sprintf(statusText, " Reduziu a velocidade!"); break;
                case 4: sprintf(statusText, " Causou paralisia!"); break;
                case 5: sprintf(statusText, " Causou sono!"); break;
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
            
            // IMPORTANTE: Limpar a fila de ações para evitar processamento adicional
            clearQueue(battleSystem->actionQueue);
            
            // Interromper execução até a troca
            return;
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
            
            if (newMonster != NULL) {
                switchMonster(battleSystem->opponentTeam, newMonster);
                char switchText[64];
                sprintf(switchText, " Oponente enviou %s!", newMonster->name);
                strncat(battleMessage, switchText, sizeof(battleMessage) - strlen(battleMessage) - 1);
            }
        }
    }
}
 
 // Calcula o dano de um ataque
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

    printf("[DEBUG] Status %d aplicado a %s por %d turnos\n",
           statusEffect, target->name, duration);

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
 
 // Verifica se a batalha acabou
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
 
 // Determina o vencedor da batalha
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
 
 // Sorteia um tipo de item aleatório para a batalha
 ItemType rollRandomItem(void) {
     int roll = rand() % 3; // 0, 1 ou 2
     return (ItemType)roll;
 }
 
 // Usa um item
void useItem(ItemType itemType, PokeMonster* target) {
    if (target == NULL) {
        return;
    }
    
    switch (itemType) {
        case ITEM_POTION:
            // Calcular quanto de HP pode ser curado
            int hpToHeal = 20; // Máximo que a poção pode curar
            int hpMissing = target->maxHp - target->hp; // Quanto de HP está faltando

            // Curar apenas o necessário (o menor entre 20 e o HP faltante)
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
                    
                    // NOVA LÓGICA: Limpar a fila de ações do bot para impedir ataques neste turno
                    clearQueue(battleSystem->actionQueue);
                } else {
                    sprintf(battleMessage, "Cartão Vermelho usado, mas falhou!");
                }
            } else {
                // Este item não deveria ser usado pelo bot, mas por precaução
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
 
 // Verifica se um monstro está incapacitado
 bool isMonsterFainted(PokeMonster* monster) {
     return (monster == NULL || monster->hp <= 0);
 }
 
 // Troca o monstro atual por outro
 void switchMonster(MonsterList* team, PokeMonster* newMonster) {
     if (team == NULL || newMonster == NULL) {
         return;
     }
     
     team->current = newMonster;
 }
 
 
 // Retorna uma descrição do que aconteceu na batalha para exibição
 const char* getBattleDescription(void) {
     return battleMessage;
 }
 
 // Reinicia a batalha
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


 // Faz o bot escolher uma ação
void botChooseAction(void) {
    if (battleSystem == NULL ||
        battleSystem->opponentTeam == NULL || battleSystem->opponentTeam->current == NULL ||
        battleSystem->playerTeam == NULL || battleSystem->playerTeam->current == NULL) {
        printf("[DEBUG BOT] Erro: ponteiros inválidos\n");
        return;
        }

    PokeMonster* botMonster = battleSystem->opponentTeam->current;
    PokeMonster* playerMonster = battleSystem->playerTeam->current;

    printf("[DEBUG BOT] Bot escolhendo ação...\n");

    // Obter sugestão da IA
    int action = getAISuggestedAction(botMonster, playerMonster);
    printf("[DEBUG BOT] IA sugeriu ação: %d\n", action);

    // Sempre atacar se não puder fazer outra coisa
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

// Faz o bot escolher um ataque
int botChooseAttack(PokeMonster* botMonster, PokeMonster* playerMonster) {
    if (botMonster == NULL || playerMonster == NULL) {
        return 0;
    }
    
    printf("[Debug] Bot escolhendo ataque...\n");
    
    // Tentar usar a IA Gemini primeiro
    int attackIndex = getAISuggestedAttack(botMonster, playerMonster);
    
    // Verificar se o ataque é válido
    if (attackIndex < 0 || attackIndex > 3 || botMonster->attacks[attackIndex].ppCurrent <= 0) {
        printf("[Debug] Ataque escolhido é inválido, escolhendo fallback.\n");
        // Fallback: escolher o primeiro ataque disponível
        for (int i = 0; i < 4; i++) {
            if (botMonster->attacks[i].ppCurrent > 0) {
                printf("[Debug] Escolhendo ataque %d: %s\n", i, botMonster->attacks[i].name);
                return i;
            }
        }
        return 0;
    }
    
    printf("[Debug] Bot escolheu o ataque %d: %s\n", attackIndex, botMonster->attacks[attackIndex].name);
    return attackIndex;
}

// Faz o bot escolher um monstro para troca
PokeMonster* botChooseMonster(MonsterList* botTeam, PokeMonster* playerMonster) {
    if (botTeam == NULL || playerMonster == NULL) {
        return NULL;
    }
    
    // Se só houver um monstro ou todos estiverem desmaiados, não trocar
    if (botTeam->count <= 1) {
        return botTeam->current;
    }
    
    // Verificar quantos monstros ativos restam
    int activeCount = 0;
    PokeMonster* activeMonstros[3]; // Máximo 3 monstros
    
    PokeMonster* current = botTeam->first;
    while (current != NULL) {
        if (!isMonsterFainted(current) && current != botTeam->current) {
            activeMonstros[activeCount++] = current;
        }
        current = current->next;
    }
    
    // Se não houver monstros ativos restantes, manter o atual
    if (activeCount == 0) {
        return botTeam->current;
    }
    
    // Lógica para escolher o melhor monstro baseado em vantagem de tipo
    PokeMonster* bestMonster = NULL;
    float bestEffectiveness = 0.0f;
    int bestHP = 0;
    
    for (int i = 0; i < activeCount; i++) {
        PokeMonster* candidate = activeMonstros[i];
        
        // Verificar a efetividade do primeiro ataque contra o jogador
        float effectiveness = calculateTypeEffectiveness(
            candidate->attacks[0].type, 
            playerMonster->type1, 
            playerMonster->type2
        );
        
        // Considerar também a "saúde" do monstro
        float healthFactor = (float)candidate->hp / candidate->maxHp;
        
        // Score combinado
        float totalScore = effectiveness + healthFactor;
        
        if (totalScore > bestEffectiveness || 
            (fabsf(totalScore - bestEffectiveness) < 0.01f && candidate->hp > bestHP)) {
            bestEffectiveness = totalScore;
            bestMonster = candidate;
            bestHP = candidate->hp;
        }
    }
    
    // Se não encontrar vantagem clara, escolher o mais saudável
    if (bestMonster == NULL || bestEffectiveness <= 1.0f) {
        bestMonster = NULL;
        int bestHPRemaining = 0;
        
        for (int i = 0; i < activeCount; i++) {
            if (activeMonstros[i]->hp > bestHPRemaining) {
                bestHPRemaining = activeMonstros[i]->hp;
                bestMonster = activeMonstros[i];
            }
        }
    }
    
    return bestMonster;
}

// Implementação básica para IA quando não quiser usar Gemini
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
    
    // Verificar efetividade dos ataques
    bool hasEffectiveAttack = false;
    for (int i = 0; i < 4; i++) {
        if (botMonster->attacks[i].ppCurrent > 0) {
            float effectiveness = calculateTypeEffectiveness(
                botMonster->attacks[i].type,
                playerMonster->type1,
                playerMonster->type2
            );
            
            if (effectiveness > 1.5f) {
                hasEffectiveAttack = true;
                break;
            }
        }
    }
    
    // Se não tem ataque super efetivo, considerar trocar
    if (!hasEffectiveAttack) {
        // 30% de chance de trocar
        if (rand() % 100 < 30) {
            return 1; // Trocar
        }
    }
    
    return 0; // Atacar por padrão
}

// Verificar se o monstro pode atacar baseado no status
bool canAttack(PokeMonster* monster) {
    if (monster == NULL || isMonsterFainted(monster)) {
        return false;
    }
    
    // Verificar status que impedem ataque
    if (monster->statusCondition == STATUS_SLEEPING) {
        printf("[DEBUG] %s está dormindo e não pode atacar!\n", monster->name);
        sprintf(battleMessage, "%s está dormindo!", monster->name);
        return false;
    }

    if (monster->statusCondition == STATUS_PARALYZED) {
        // Paralisado tem 25% de chance de não conseguir atacar
        if (rand() % 100 < 25) {
            printf("[DEBUG] %s está paralisado e não conseguiu se mover!\n", monster->name);
            sprintf(battleMessage, "%s está paralisado e não conseguiu se mover!", monster->name);
            return false;
        }
    }
    
    return true;
}

// Processar efeitos de status no final do turno
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

// Função auxiliar para exibir mensagens de status
void displayStatusMessage(const char* message) {
    // Copiar a mensagem para a variável global de mensagem da batalha
    strncpy(battleMessage, message, sizeof(battleMessage) - 1);
    
    // Garantir que a string termina com nulo
    battleMessage[sizeof(battleMessage) - 1] = '\0';
}

// Nova função para exibir mensagens suavemente
void displayBattleMessage(const char* message, float duration, bool waitForInput, bool autoAdvance) {
    if (message == NULL || strlen(message) == 0) {
        printf("[DEBUG] Tentativa de mostrar mensagem vazia\n");
        return;
    }

    printf("[DEBUG] Exibindo mensagem: '%s' (duração: %.1f)\n", message, duration);

    strncpy(currentMessage.message, message, sizeof(currentMessage.message) - 1);
    currentMessage.message[sizeof(currentMessage.message) - 1] = '\0';
    currentMessage.displayTime = duration;
    currentMessage.elapsedTime = 0.0f;
    currentMessage.waitingForInput = waitForInput;
    currentMessage.autoAdvance = autoAdvance;
    battleSystem->battleState = BATTLE_MESSAGE_DISPLAY;
}

// Nova função para iniciar animação de ataque
void startAttackAnimation(PokeMonster* attacker, PokeMonster* defender, int attackIndex) {
    currentAnimation.isAnimating = true;
    currentAnimation.animationTime = 1.0f; // 1 segundo de animação
    currentAnimation.elapsedTime = 0.0f;
    currentAnimation.animationType = attackIndex;
    currentAnimation.source = attacker;
    currentAnimation.target = defender;
    battleSystem->battleState = BATTLE_ATTACK_ANIMATION;
}
