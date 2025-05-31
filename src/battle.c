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
#include "battle_effects.h"
#include "hp_bar.h"

static bool botPotionUsed = false;     // Controla se o bot já usou poção na batalha
static bool botRandomItemUsed = false; // Controla se o bot já usou item aleatório na batalha


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
    InitBattleEffectsSystem();

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
    ClearAllBattleEffects();

    // Configurar os times
    battleSystem->playerTeam = playerTeam;
    battleSystem->opponentTeam = opponentTeam;

    // Resetar contadores e estado
    battleSystem->turn = 1;

    // INICIAR COM ANIMAÇÃO DE INTRODUÇÃO
    battleSystem->battleState = BATTLE_INTRO_ANIMATION;
    StartBattleIntroAnimation(); // Iniciar animação de pokébolas

    battleSystem->playerTurn = true;
    battleSystem->itemUsed = false;
    battleSystem->selectedAttack = 0;
    battleSystem->selectedAction = 0;
    botPotionUsed = false;
    botRandomItemUsed = false;

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

    // Mensagem inicial (será mostrada após a animação)
    strcpy(battleMessage, "Uma batalha selvagem começou!");

    printf("[BATTLE] Nova batalha iniciada com animação de introdução\n");
}

// Atualiza o estado da batalha
void updateBattle(void) {
    if (battleSystem == NULL) return;

    // Atualização global de timers
    float deltaTime = GetFrameTime();

    // Verificar se a batalha acabou (exceto durante a animação de introdução)
    if (battleSystem->battleState != BATTLE_INTRO_ANIMATION &&
        isBattleOver() && battleSystem->battleState != BATTLE_OVER &&
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

    UpdateBattleEffects();

    // Atualizar estado atual
    switch (battleSystem->battleState) {
        case BATTLE_INTRO_ANIMATION:
            // NOVO CASO: Atualizar animação de pokébolas
            UpdateBattleIntroAnimation();

            // Verificar se a animação terminou
            if (!IsBattleIntroActive()) {
                battleSystem->battleState = BATTLE_INTRO;
                stateTransitionDelay = 0.0f;
                printf("[BATTLE] Animação de introdução completa, mudando para BATTLE_INTRO\n");
            }

            // Permitir pular a animação
            if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                SkipBattleIntro();
                battleSystem->battleState = BATTLE_INTRO;
                stateTransitionDelay = 0.0f;
                printf("[BATTLE] Animação de introdução pulada pelo usuário\n");
            }
            break;

        case BATTLE_INTRO:
            // Lógica para introdução (após animação das pokébolas)
            stateTransitionDelay += deltaTime;
            if (stateTransitionDelay >= 1.0f) { // Reduzido para 1s pois já tivemos a animação
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

                    // Se o jogador precisa trocar de Pokémon (Pokémon desmaiado ou Cartão Vermelho)
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
                return;
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


        case BATTLE_PREPARING_ACTIONS:
            // Ordenar ações por velocidade
            determineAndExecuteTurnOrder();
            battleSystem->battleState = BATTLE_EXECUTING_ACTIONS;
            break;

        case BATTLE_EXECUTING_ACTIONS:

            stateTransitionDelay += deltaTime;
            if (stateTransitionDelay < 0.3f) {
                break; // Aguardar antes de executar
            }
            stateTransitionDelay = 0.0f;

            if (!isQueueEmpty(battleSystem->actionQueue)) {
                int action, parameter;
                PokeMonster* monster;

                dequeue(battleSystem->actionQueue, &action, &parameter, &monster);
                printf("[DEBUG] Dequeue executando ação: tipo=%d, param=%d\n", action, parameter);

                // Verificar se o monstro está desmaiado antes de executar a ação
                if (isMonsterFainted(monster)) {
                    printf("[DEBUG] Ignorando ação de monstro desmaiado\n");
                    battleSystem->battleState = BATTLE_MESSAGE_DISPLAY;
                    break;  // Sair do case sem executar a ação
                }

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
            stateTransitionDelay += deltaTime;
            if (stateTransitionDelay < 1.0f) {
                break;
            }
            // Processar fim de turno
            printf("[DEBUG TURN END] Processando fim de turno %d\n", battleSystem->turn);
            processTurnEnd();
            battleSystem->turn++;

            // Resetar para o novo turno
            clearQueue(battleSystem->actionQueue);
            actionQueueReady = false;

            // Verificar primeiro se algum Pokémon desmaiou
            if (isMonsterFainted(battleSystem->playerTeam->current)) {
                battleSystem->battleState = BATTLE_FORCED_SWITCH;
                battleSystem->playerTurn = true;
            } else {
                // Explicitamente definir que é a vez do jogador
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

    // Gerar descrição do ataque (apenas para ataques bem-sucedidos)
    char* description = generateAttackDescription(attacker, defender, attack);
    if (description) {
        strncpy(battleMessage, description, sizeof(battleMessage) - 1);
        free(description);
    } else {
        sprintf(battleMessage, "%s usou %s!", attacker->name, attack->name);
    }

    // Criar efeito visual do ataque
    Vector2 attackerPos, defenderPos;

    // Determinar posições baseadas em quem está atacando
    if (attacker == battleSystem->playerTeam->current) {
        // Jogador atacando
        attackerPos = (Vector2){GetScreenWidth() / 3, GetScreenHeight() / 1.8f - 20};
        defenderPos = (Vector2){GetScreenWidth() * 2 / 3, GetScreenHeight() / 2.6f - 20};
        CreateAttackEffect(attack->type, attackerPos, defenderPos, true);
    } else {
        // Oponente atacando
        attackerPos = (Vector2){GetScreenWidth() * 2 / 3, GetScreenHeight() / 2.6f - 20};
        defenderPos = (Vector2){GetScreenWidth() / 3, GetScreenHeight() / 1.8f - 20};
        CreateAttackEffect(attack->type, attackerPos, defenderPos, false);
    }

    // Calcular dano (se for um ataque de dano)
    if (attack->power > 0) {
    int damage = calculateDamage(attacker, defender, attack);

    // Verificar crítico (5% de chance)
    bool isCritical = (rand() % 100) < 5;
    if (isCritical) {
        damage = (int)(damage * 1.5f);
    }

    // Aplicar dano
    defender->hp -= damage;
    if (defender->hp < 0) {
        defender->hp = 0;
    }

    // ATIVAR SHAKE NO POKÉMON QUE RECEBEU DANO
    bool isPlayerTarget = (defender == battleSystem->playerTeam->current);

    // Intensidade do shake baseada no dano e crítico
    float shakeIntensity = 8.0f + (damage * 0.1f); // Base + proporcional ao dano
    if (isCritical) {
        shakeIntensity *= 1.5f; // Shake mais forte para críticos
    }

    // Duração baseada na intensidade
    float shakeDuration = 0.6f + (damage * 0.01f); // 0.6s base + proporcional
    if (isCritical) {
        shakeDuration += 0.3f; // Duração maior para críticos
    }

    // Ativar o shake no Pokémon correto
    TriggerPokemonShake(isPlayerTarget, shakeIntensity, shakeDuration);

    printf("[DAMAGE] %s recebeu %d de dano%s, shake: intensidade=%.1f, duração=%.1f\n",
           defender->name, damage, isCritical ? " (CRÍTICO)" : "",
           shakeIntensity, shakeDuration);

    // Criar efeito visual de dano
    Vector2 damagePos;

    if (isPlayerTarget) {
        damagePos = (Vector2){GetScreenWidth() / 3, GetScreenHeight() / 1.8f - 50};
    } else {
        damagePos = (Vector2){GetScreenWidth() * 2 / 3, GetScreenHeight() / 2.6f - 50};
    }

    CreateDamageEffect(damagePos, damage, isPlayerTarget, isCritical);

    // Adicionar informação de dano à mensagem
    char damageText[50];
    if (isCritical) {
        sprintf(damageText, " Acerto crítico! Causou %d de dano!", damage);
    } else {
        sprintf(damageText, " Causou %d de dano!", damage);
    }
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

            // Criar efeito visual de status
            Vector2 statusPos;
            if (defender == battleSystem->playerTeam->current) {
                statusPos = (Vector2){GetScreenWidth() / 3, GetScreenHeight() / 1.8f - 20};
            } else {
                statusPos = (Vector2){GetScreenWidth() * 2 / 3, GetScreenHeight() / 2.6f - 20};
            }
            CreateStatusEffect(statusPos, attack->statusEffect);

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
        // Criar efeito visual de desmaiado
        Vector2 faintPos;
        if (defender == battleSystem->playerTeam->current) {
            faintPos = (Vector2){GetScreenWidth() / 3, GetScreenHeight() / 1.8f - 20};
        } else {
            faintPos = (Vector2){GetScreenWidth() * 2 / 3, GetScreenHeight() / 2.6f - 20};
        }
        CreateFaintEffect(faintPos);

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
            // Limpar TODA a fila de ações para o turno atual
            // Isso garante que nenhum ataque aconteça após a troca
            clearQueue(battleSystem->actionQueue);

            // Configurar mensagem para exibição por mais tempo
            currentMessage.displayTime = 3.0f;
            currentMessage.elapsedTime = 0.0f;
            currentMessage.waitingForInput = true;
            currentMessage.autoAdvance = false;

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
    printf("[DEBUG] Ordenando ações com QuickSort Contagem atual na fila: %d\n",
           battleSystem->actionQueue->count);

    if (battleSystem == NULL || isQueueEmpty(battleSystem->actionQueue)) {
        return;
    }

    int actions[2] = {-1, -1};
    int parameters[2] = {-1, -1};
    PokeMonster* monsters[2] = {NULL, NULL};
    MonsterList* teams[2] = {NULL, NULL};
    bool isPlayerActions[2] = {false, false};
    int actionCount = 0;

    // Retirar todas as ações da fila
    while (!isQueueEmpty(battleSystem->actionQueue) && actionCount < 2) {
        dequeue(battleSystem->actionQueue, &actions[actionCount],
                &parameters[actionCount], &monsters[actionCount]);

        // Determinar qual time o monstro pertence
        if (monsters[actionCount] != NULL) {
            PokeMonster* current = battleSystem->playerTeam->first;
            while (current != NULL) {
                if (current == monsters[actionCount]) {
                    teams[actionCount] = battleSystem->playerTeam;
                    isPlayerActions[actionCount] = true;
                    break;
                }
                current = current->next;
            }

            if (teams[actionCount] == NULL) {
                current = battleSystem->opponentTeam->first;
                while (current != NULL) {
                    if (current == monsters[actionCount]) {
                        teams[actionCount] = battleSystem->opponentTeam;
                        isPlayerActions[actionCount] = false;
                        break;
                    }
                    current = current->next;
                }
            }
        }

        actionCount++;
    }

    printf("[DEBUG] Extraídas %d ações da fila\n", actionCount);

    // Caso especial: Ambos trocam Pokémon no mesmo turno
    if (actionCount == 2 && actions[0] == 1 && actions[1] == 1) {
        printf("[DEBUG] Caso especial: Ambos trocam Pokémon no mesmo turno\n");

        int playerIndex = isPlayerActions[0] ? 0 : 1;
        int botIndex = isPlayerActions[0] ? 1 : 0;

        if (monsters[playerIndex] != NULL && teams[playerIndex] != NULL) {
            executeMonsterSwitch(monsters[playerIndex], parameters[playerIndex]);
            char playerSwitchMessage[256];
            strncpy(playerSwitchMessage, battleMessage, sizeof(playerSwitchMessage) - 1);
            playerSwitchMessage[sizeof(playerSwitchMessage) - 1] = '\0';

            if (monsters[botIndex] != NULL && teams[botIndex] != NULL) {
                executeMonsterSwitch(monsters[botIndex], parameters[botIndex]);
                char combinedMessage[256];
                snprintf(combinedMessage, sizeof(combinedMessage), "%s\n%s",
                        playerSwitchMessage, battleMessage);
                strncpy(battleMessage, combinedMessage, sizeof(battleMessage) - 1);
                battleMessage[sizeof(battleMessage) - 1] = '\0';
            }
        }
        return;
    }

    // ===== QUICKSORT =====
    if (actionCount == 2) {
        // Primeiro aplicar regras de prioridade (Troca > Item > Ataque)
        int priority[2];
        for (int i = 0; i < 2; i++) {
            switch (actions[i]) {
                case 1: priority[i] = 1; break; // Troca - maior prioridade
                case 2: priority[i] = 2; break; // Item - média prioridade
                case 0:
                default: priority[i] = 3; break; // Ataque - menor prioridade
            }
        }

        // Se as prioridades são diferentes, ordenar por prioridade
        if (priority[0] > priority[1]) {
            // Swap - segunda ação tem maior prioridade
            printf("[DEBUG] Trocando ordem: ação 1 tem maior prioridade que ação 0\n");

            int tempAction = actions[0];
            int tempParam = parameters[0];
            PokeMonster* tempMonster = monsters[0];
            MonsterList* tempTeam = teams[0];
            bool tempIsPlayer = isPlayerActions[0];

            actions[0] = actions[1];
            parameters[0] = parameters[1];
            monsters[0] = monsters[1];
            teams[0] = teams[1];
            isPlayerActions[0] = isPlayerActions[1];

            actions[1] = tempAction;
            parameters[1] = tempParam;
            monsters[1] = tempMonster;
            teams[1] = tempTeam;
            isPlayerActions[1] = tempIsPlayer;
        }
        // Se as prioridades são iguais, usar QuickSort por velocidade
        else if (priority[0] == priority[1] && actions[0] == 0 && actions[1] == 0) {
            printf("[DEBUG] Mesma prioridade (ataques), usando QuickSort por velocidade\n");

            // Criar array de ponteiros para usar com o QuickSort existente
            PokeMonster* monsterArray[2] = {monsters[0], monsters[1]};

            printf("[DEBUG] ANTES do QuickSort:\n");
            printf("[DEBUG] Posição 0: %s (Velocidade: %d)\n",
                   monsterArray[0] ? monsterArray[0]->name : "NULL",
                   monsterArray[0] ? monsterArray[0]->speed : 0);
            printf("[DEBUG] Posição 1: %s (Velocidade: %d)\n",
                   monsterArray[1] ? monsterArray[1]->name : "NULL",
                   monsterArray[1] ? monsterArray[1]->speed : 0);

            // ===== USAR O QUICKSORT EXISTENTE DE MONSTERS.C =====
            quickSortMonstersBySpeed(monsterArray, 0, 1);

            printf("[DEBUG] DEPOIS do QuickSort:\n");
            printf("[DEBUG] Posição 0: %s (Velocidade: %d)\n",
                   monsterArray[0] ? monsterArray[0]->name : "NULL",
                   monsterArray[0] ? monsterArray[0]->speed : 0);
            printf("[DEBUG] Posição 1: %s (Velocidade: %d)\n",
                   monsterArray[1] ? monsterArray[1]->name : "NULL",
                   monsterArray[1] ? monsterArray[1]->speed : 0);

            // Se a ordem mudou, trocar as ações também
            if (monsterArray[0] != monsters[0]) {
                printf("[DEBUG] QuickSort alterou a ordem - aplicando mudanças\n");

                int tempAction = actions[0];
                int tempParam = parameters[0];
                MonsterList* tempTeam = teams[0];
                bool tempIsPlayer = isPlayerActions[0];

                actions[0] = actions[1];
                parameters[0] = parameters[1];
                monsters[0] = monsters[1];
                teams[0] = teams[1];
                isPlayerActions[0] = isPlayerActions[1];

                actions[1] = tempAction;
                parameters[1] = tempParam;
                monsters[1] = monsterArray[1]; // Usar do array ordenado
                teams[1] = tempTeam;
                isPlayerActions[1] = tempIsPlayer;
            } else {
                printf("[DEBUG] QuickSort manteve a ordem original\n");
            }
        }
    }

    // Recolocar na fila na ordem correta
    for (int i = 0; i < actionCount; i++) {
        if (monsters[i] != NULL) {
            enqueue(battleSystem->actionQueue, actions[i], parameters[i], monsters[i]);
            printf("[DEBUG] Recolocando ação %d na fila: tipo=%d, param=%d, monstro=%s\n",
                i, actions[i], parameters[i],
                monsters[i] ? monsters[i]->name : "NULL");
        }
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

                // Criar efeito visual de queimadura contínua
                Vector2 burnPos;
                if (monster == battleSystem->playerTeam->current) {
                    burnPos = (Vector2){GetScreenWidth() / 3, GetScreenHeight() / 1.8f - 20};
                } else {
                    burnPos = (Vector2){GetScreenWidth() * 2 / 3, GetScreenHeight() / 2.6f - 20};
                }
                CreateContinuousStatusEffect(burnPos, STATUS_BURNING, 2.0f);

                sprintf(battleMessage, "%s sofreu %d de dano por estar em chamas!",
                       monster->name, damage);
            }
            break;

        case STATUS_SLEEPING:
            printf("[DEBUG STATUS] %s está dormindo (não pode atacar)\n", monster->name);

            // Criar efeito visual de sono contínuo
            Vector2 sleepPos;
            if (monster == battleSystem->playerTeam->current) {
                sleepPos = (Vector2){GetScreenWidth() / 3, GetScreenHeight() / 1.8f - 20};
            } else {
                sleepPos = (Vector2){GetScreenWidth() * 2 / 3, GetScreenHeight() / 2.6f - 20};
            }
            CreateContinuousStatusEffect(sleepPos, STATUS_SLEEPING, 1.5f);
            break;

        case STATUS_PARALYZED:
            printf("[DEBUG STATUS] %s está paralisado\n", monster->name);

            // Criar efeito visual de paralisia contínuo
            Vector2 paralyzePos;
            if (monster == battleSystem->playerTeam->current) {
                paralyzePos = (Vector2){GetScreenWidth() / 3, GetScreenHeight() / 1.8f - 20};
            } else {
                paralyzePos = (Vector2){GetScreenWidth() * 2 / 3, GetScreenHeight() / 2.6f - 20};
            }
            CreateContinuousStatusEffect(paralyzePos, STATUS_PARALYZED, 1.0f);
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
 * Escolhe uma ação para o bot usando IA Gemini com fallback simples
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
                // Verificar se o bot já usou todos os itens disponíveis
                if ((battleSystem->itemType == ITEM_POTION && botPotionUsed) ||
                    (battleSystem->itemType == ITEM_RED_CARD && botRandomItemUsed) ||
                    (battleSystem->itemType == ITEM_COIN && botRandomItemUsed)) {

                    // Bot já usou todos os itens, escolher outra ação
                    int attackIndex = botChooseAttack(botMonster, playerMonster);
                    printf("[DEBUG BOT] Bot já usou todos os itens, vai atacar usando ataque %d\n", attackIndex);
                    enqueue(battleSystem->actionQueue, 0, attackIndex, botMonster);
                    break;
                }

                // Selecionar o item com base na situação
                ItemType itemType = battleSystem->itemType;

                // Determinar qual item usar com mais inteligência
                if (botMonster->hp < botMonster->maxHp * 0.4f && !botPotionUsed) {
                    // Se HP está baixo (< 40%), preferir Poção
                    itemType = ITEM_POTION;
                }
                else if (botMonster->hp > botMonster->maxHp * 0.7f && !botRandomItemUsed) {
                    // Se o HP está relativamente alto, considerar usar Cartão Vermelho
                    // O Cartão Vermelho é mais útil se o Pokémon do jogador é forte
                    if (battleSystem->itemType == ITEM_RED_CARD &&
                        (playerMonster->hp > playerMonster->maxHp * 0.5f)) {
                        itemType = ITEM_RED_CARD;
                    } else if (battleSystem->itemType == ITEM_COIN) {
                        itemType = ITEM_COIN;
                    }
                }
                // Se já usou esse tipo específico de item, tentar o outro
                if ((itemType == ITEM_POTION && botPotionUsed) ||
                    ((itemType == ITEM_RED_CARD || itemType == ITEM_COIN) && botRandomItemUsed)) {

                    // Tentar usar o outro tipo de item
                    if (botPotionUsed && !botRandomItemUsed) {
                        itemType = battleSystem->itemType; // Item randomico
                    } else if (!botPotionUsed && botRandomItemUsed) {
                        itemType = ITEM_POTION;
                    } else {
                        // Já usou os dois tipos, atacar
                        int attackIndex = botChooseAttack(botMonster, playerMonster);
                        printf("[DEBUG BOT] Bot já usou ambos os itens, vai atacar usando ataque %d\n", attackIndex);
                        enqueue(battleSystem->actionQueue, 0, attackIndex, botMonster);
                        break;
                    }
                }

                printf("[DEBUG BOT] Bot vai usar item %d\n", itemType);
                enqueue(battleSystem->actionQueue, 2, itemType, botMonster);
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
    baseDamage = (baseDamage / 50.0f) + 5;

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

            // Criar efeito visual de cura
            Vector2 healPos;
            if (target == battleSystem->playerTeam->current) {
                healPos = (Vector2){GetScreenWidth() / 3, GetScreenHeight() / 1.8f - 20};
            } else {
                healPos = (Vector2){GetScreenWidth() * 2 / 3, GetScreenHeight() / 2.6f - 20};
            }
            CreateHealEffect(healPos, hpToHeal);

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
    // Determinar o alvo do cartão vermelho
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
        sprintf(battleMessage, "Oponente usou Cartão Vermelho! Seu Pokémon foi forçado a sair!");
    }

    // Verificar se há outros monstros disponíveis para troca
    bool hasValidMonsters = false;
    PokeMonster* current = affectedTeam->first;
    int validMonsterIndex = -1;
    int index = 0;

    while (current != NULL) {
        if (!isMonsterFainted(current) && current != affectedMonster) {
            hasValidMonsters = true;
            if (validMonsterIndex == -1) {
                validMonsterIndex = index; // Guarda o índice do primeiro válido
            }
        }
        current = current->next;
        index++;
    }

    if (hasValidMonsters) {
        // Limpar TODA a fila de ações para o turno atual
        // Isso garante que nenhum ataque aconteça após a troca
        clearQueue(battleSystem->actionQueue);

        // Fazer a troca imediatamente
        PokeMonster* oldMonster = affectedTeam->current; // Guardar o Pokémon anterior para referência
        executeMonsterSwitch(affectedMonster, validMonsterIndex);

        // Atualizar mensagem informando a troca
        if (affectedTeam == battleSystem->playerTeam) {
            sprintf(battleMessage, "Cartão Vermelho usado! Seu %s foi substituído por %s!",
                   oldMonster->name, affectedTeam->current->name);

            // Como o jogador foi afetado, ele perde a vez de atacar
            battleSystem->playerTurn = false;
        } else {
            sprintf(battleMessage, "Cartão Vermelho usado! Oponente trocou para %s!",
                   affectedTeam->current->name);

            // Como o oponente foi afetado, ele perde a vez de atacar
            // (o jogador pode atacar normalmente)
            battleSystem->playerTurn = true;
        }

        // Marcar que a ação da fila está completa
        actionQueueReady = true;

        // Definir que o próximo estado deve ser exibir a mensagem da troca
        battleSystem->battleState = BATTLE_MESSAGE_DISPLAY;

        // Configurar a mensagem atual para exibição
        currentMessage.displayTime = 2.0f;
        currentMessage.elapsedTime = 0.0f;
        currentMessage.waitingForInput = true;
        currentMessage.autoAdvance = false;
        strcpy(currentMessage.message, battleMessage);
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

                // Criar efeito visual de cura massiva
                Vector2 healPos;
                if (target == battleSystem->playerTeam->current) {
                    healPos = (Vector2){GetScreenWidth() / 3, GetScreenHeight() / 1.8f - 20};
                } else {
                    healPos = (Vector2){GetScreenWidth() * 2 / 3, GetScreenHeight() / 2.6f - 20};
                }
                CreateHealEffect(healPos, target->maxHp);
                TriggerScreenFlash((Color){100, 255, 100, 150}, 1.0f, 0.5f);

                sprintf(battleMessage, "Moeda da Sorte: CARA! %s recuperou todo o HP!", target->name);
            } else {
                // HP = 0
                target->hp = 0;

                // Criar efeito visual de desmaiado
                Vector2 faintPos;
                if (target == battleSystem->playerTeam->current) {
                    faintPos = (Vector2){GetScreenWidth() / 3, GetScreenHeight() / 1.8f - 20};
                } else {
                    faintPos = (Vector2){GetScreenWidth() * 2 / 3, GetScreenHeight() / 2.6f - 20};
                }
                CreateFaintEffect(faintPos);

                sprintf(battleMessage, "Moeda da Sorte: COROA! %s desmaiou!", target->name);

                // Se o jogador desmaiou, forçar troca
                if (target == battleSystem->playerTeam->current) {
                    battleSystem->battleState = BATTLE_FORCED_SWITCH;
                    battleSystem->playerTurn = true;
                }
                // Se o oponente desmaiou, realizar troca automática
                else if (target == battleSystem->opponentTeam->current) {
                    // Verificar se há outros monstros disponíveis
                    PokeMonster* newMonster = NULL;
                    PokeMonster* current = battleSystem->opponentTeam->first;

                    // Encontrar o próximo monstro não desmaiado
                    while (current != NULL) {
                        if (!isMonsterFainted(current) && current != target) {
                            newMonster = current;
                            break;
                        }
                        current = current->next;
                    }

                    // Se encontrou um monstro para trocar
                    if (newMonster != NULL) {
                        // Trocar para o novo monstro
                        switchMonster(battleSystem->opponentTeam, newMonster);

                        // Adicionar informação sobre a troca à mensagem
                        char switchText[128];
                        sprintf(switchText, " Oponente enviou %s!", newMonster->name);
                        strncat(battleMessage, switchText, sizeof(battleMessage) - strlen(battleMessage) - 1);
                    }
                    // Se não houver mais monstros, a batalha terminará automaticamente na próxima verificação
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

    if (target == battleSystem->playerTeam->current) {
        printf("[useItem] Item usado pelo jogador\n");
        battleSystem->playerItemUsed = true;
    } else if (target == battleSystem->opponentTeam->current) {
        printf("[useItem] Item usado pelo oponente\n");
        battleSystem->botItemUsed = true;

        // Marcar o tipo específico de item como usado pelo bot
        if (itemType == ITEM_POTION) {
            botPotionUsed = true;
            printf("[useItem] Bot marcou poção como usada\n");
        } else if (itemType == ITEM_RED_CARD || itemType == ITEM_COIN) {
            botRandomItemUsed = true;
            printf("[useItem] Bot marcou item aleatório como usado\n");
        }
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
    printf("[BATTLE RESET] Iniciando reset completo da batalha...\n");

    if (battleSystem == NULL) {
        printf("[BATTLE RESET] Sistema de batalha não inicializado\n");
        return;
    }

    // Resetar estado da batalha
    battleSystem->turn = 0;
    battleSystem->battleState = BATTLE_IDLE;
    battleSystem->playerTurn = true;
    battleSystem->selectedAttack = 0;
    battleSystem->selectedAction = 0;
    battleSystem->itemUsed = false;
    battleSystem->playerItemUsed = false;
    battleSystem->botItemUsed = false;

    // Limpar estruturas de dados
    clearQueue(battleSystem->actionQueue);
    clearStack(battleSystem->effectStack);

    // Resetar flags globais
    actionQueueReady = false;
    stateTransitionDelay = 0.0f;

    // Limpar mensagens
    battleMessage[0] = '\0';
    memset(&currentMessage, 0, sizeof(currentMessage));
    memset(&currentAnimation, 0, sizeof(currentAnimation));

    // Limpar todos os efeitos visuais
    ClearAllBattleEffects();

    // Resetar sprites e renderizador
    resetBattleSprites();

    // Chamar limpeza do renderizador se a função existir
    extern void cleanupBattleRenderer(void);
    cleanupBattleRenderer();

    // Resetar barras de HP
    extern void ResetHPBarAnimations(void);
    ResetHPBarAnimations();

    printf("[BATTLE RESET] Reset completo finalizado\n");
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