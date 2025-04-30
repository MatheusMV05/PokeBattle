/**
 * PokeBattle - Implementação do sistema de batalha
 * 
 * Este arquivo contém as implementações das funções para o sistema de batalha.
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <math.h>
 #include "raylib.h"
 #include "battle.h"
 #include "monsters.h"
 #include "ai_integration.h"
 
 // Sistema de batalha global
 static BattleSystem* battleSystem = NULL;
 
 // Mensagem de batalha atual
 static char battleMessage[256] = "";
 
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
     battleSystem->battleState = BATTLE_SELECT_ACTION;
     battleSystem->itemUsed = false;
     
     // Escolher um item aleatório para a batalha
     battleSystem->itemType = rollRandomItem();
     
     // Determinar quem joga primeiro
     determineTurnOrder();
     
     // Limpar estruturas de dados
     clearQueue(battleSystem->actionQueue);
     clearStack(battleSystem->effectStack);
     
     // Mensagem inicial
     sprintf(battleMessage, "Que a batalha comece! Escolha sua ação.");
 }
 
 // Atualiza o estado da batalha
 void updateBattle(void) {
     if (battleSystem == NULL) {
         return;
     }
     
     // Verificar se a batalha acabou
     if (isBattleOver()) {
         battleSystem->battleState = BATTLE_OVER;
         return;
     }
     
     // Processar estados da batalha
     switch (battleSystem->battleState) {
         case BATTLE_SELECT_ACTION:
             // Se for a vez do bot, ele escolhe a ação
             if (!battleSystem->playerTurn) {
                 botChooseAction();
             }
             break;
             
         case BATTLE_EXECUTING_ACTIONS:
             // Executar as ações na fila
             if (!isQueueEmpty(battleSystem->actionQueue)) {
                 int action, parameter;
                 PokeMonster* monster;
                 
                 dequeue(battleSystem->actionQueue, &action, &parameter, &monster);
                 
                 // Processar a ação
                 switch (action) {
                     case 0: // Ataque
                         if (monster == battleSystem->playerTeam->current) {
                             // Jogador atacando oponente
                             executeAttack(monster, battleSystem->opponentTeam->current, parameter);
                         } else {
                             // Oponente atacando jogador
                             executeAttack(monster, battleSystem->playerTeam->current, parameter);
                         }
                         break;
                         
                     case 1: // Troca de monstro
                         {
                             // Encontrar o monstro pelo índice (implementação simplificada)
                             PokeMonster* newMonster = NULL;
                             PokeMonster* current = NULL;
                             int count = 0;
                             
                             if (monster == battleSystem->playerTeam->current || 
                                 monster->prev == battleSystem->playerTeam->current || 
                                 monster->next == battleSystem->playerTeam->current) {
                                 // Troca do time do jogador
                                 current = battleSystem->playerTeam->first;
                                 while (current != NULL && count < parameter) {
                                     current = current->next;
                                     count++;
                                 }
                                 newMonster = current;
                                 
                                 if (newMonster != NULL && !isMonsterFainted(newMonster)) {
                                     switchMonster(battleSystem->playerTeam, newMonster);
                                     sprintf(battleMessage, "Você trocou para %s!", newMonster->name);
                                 }
                             } else {
                                 // Troca do time do oponente
                                 current = battleSystem->opponentTeam->first;
                                 while (current != NULL && count < parameter) {
                                     current = current->next;
                                     count++;
                                 }
                                 newMonster = current;
                                 
                                 if (newMonster != NULL && !isMonsterFainted(newMonster)) {
                                     switchMonster(battleSystem->opponentTeam, newMonster);
                                     sprintf(battleMessage, "Oponente trocou para %s!", newMonster->name);
                                 }
                             }
                         }
                         break;
                         
                     case 2: // Usar item
                         if (monster == battleSystem->playerTeam->current) {
                             // Jogador usando item
                             useItem(battleSystem->itemType, battleSystem->playerTeam->current);
                             battleSystem->itemUsed = true;
                         } else {
                             // Oponente usando item
                             useItem(battleSystem->itemType, battleSystem->opponentTeam->current);
                             battleSystem->itemUsed = true;
                         }
                         break;
                 }
                 
                 // Passar para o próximo estado
                 battleSystem->battleState = BATTLE_RESULT_MESSAGE;
             } else {
                 // Todas as ações foram executadas, processar fim do turno
                 processTurnEnd();
                 battleSystem->battleState = BATTLE_SELECT_ACTION;
                 battleSystem->turn++;
                 determineTurnOrder(); // Recalcular ordem de turno
             }
             break;
             
         case BATTLE_RESULT_MESSAGE:
             // Este estado apenas mostra a mensagem do resultado da ação
             // A transição acontece no processamento de input
             break;
             
         default:
             break;
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
             applyStatusEffect(defender, attack->statusEffect, attack->statusPower, 3); // 3 turnos de duração
             
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
     if (target == NULL || statusEffect <= 0 || battleSystem == NULL) {
         return;
     }
     
     // Salvar o efeito na pilha para poder desfazê-lo depois
     push(battleSystem->effectStack, statusEffect, duration, statusPower, target);
     
     // Aplicar o efeito
     switch (statusEffect) {
         case 1: // Reduzir ataque
             target->attack = (int)(target->attack * (100 - statusPower) / 100.0f);
             break;
         case 2: // Reduzir defesa
             target->defense = (int)(target->defense * (100 - statusPower) / 100.0f);
             break;
         case 3: // Reduzir velocidade
             target->speed = (int)(target->speed * (100 - statusPower) / 100.0f);
             break;
         case 4: // Paralisia (reduz velocidade e chance de não atacar)
             target->speed = (int)(target->speed * 0.5f);
             target->statusCondition = statusEffect;
             target->statusCounter = duration;
             break;
         case 5: // Sono (não pode atacar)
             target->statusCondition = statusEffect;
             target->statusCounter = duration;
             break;
         default:
             break;
     }
 }
 
 // Processa o final do turno (efeitos de status, etc.)
 void processTurnEnd(void) {
     if (battleSystem == NULL) {
         return;
     }
     
     // Processar efeitos de status ativos
     // Reduzir contadores de status e remover os que expiraram
     if (battleSystem->playerTeam && battleSystem->playerTeam->current) {
         PokeMonster* player = battleSystem->playerTeam->current;
         if (player->statusCounter > 0) {
             player->statusCounter--;
             if (player->statusCounter == 0) {
                 player->statusCondition = 0;
             }
         }
     }
     
     if (battleSystem->opponentTeam && battleSystem->opponentTeam->current) {
         PokeMonster* opponent = battleSystem->opponentTeam->current;
         if (opponent->statusCounter > 0) {
             opponent->statusCounter--;
             if (opponent->statusCounter == 0) {
                 opponent->statusCondition = 0;
             }
         }
     }
     
     // Processar a pilha de efeitos
     // Remover efeitos que expiraram e restaurar os stats
     if (!isStackEmpty(battleSystem->effectStack)) {
         EffectStack* stack = battleSystem->effectStack;
         
         for (int i = 0; i <= stack->top; i++) {
             stack->durations[i]--;
         }
         
         // Verificar de trás para frente e remover os expirados
         while (!isStackEmpty(stack) && stack->durations[stack->top] <= 0) {
             int type, duration, value;
             PokeMonster* target;
             
             pop(stack, &type, &duration, &value, &target);
             
             // Restaurar o stat modificado (simplificado)
             // Em um jogo real, seria necessário armazenar os valores originais
             // e restaurá-los corretamente
         }
     }
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
             // Restaura 20 de HP
             target->hp += 20;
             if (target->hp > target->maxHp) {
                 target->hp = target->maxHp;
             }
             sprintf(battleMessage, "Poção usada! %s recuperou 20 de HP!", target->name);
             break;
             
         case ITEM_RED_CARD:
             // Força o oponente a trocar de monstro
             if (target == battleSystem->playerTeam->current) {
                 // Forçar troca do oponente
                 PokeMonster* newMonster = botChooseMonster(battleSystem->opponentTeam, target);
                 if (newMonster && newMonster != battleSystem->opponentTeam->current) {
                     switchMonster(battleSystem->opponentTeam, newMonster);
                     sprintf(battleMessage, "Cartão Vermelho usado! Oponente trocou para %s!", newMonster->name);
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
 
 // Faz o bot escolher uma ação
 void botChooseAction(void) {
     if (battleSystem == NULL || 
         battleSystem->opponentTeam == NULL || battleSystem->opponentTeam->current == NULL ||
         battleSystem->playerTeam == NULL || battleSystem->playerTeam->current == NULL) {
         return;
     }
     
     PokeMonster* botMonster = battleSystem->opponentTeam->current;
     PokeMonster* playerMonster = battleSystem->playerTeam->current;
     
     // Usar a IA para escolher a ação
     int action = getAISuggestedAction(botMonster, playerMonster);
     
     // Validação e execução da ação
     switch (action) {
         case 0: // Atacar
             {
                 int attackIndex = botChooseAttack(botMonster, playerMonster);
                 
                 // Adicionar à fila de ações
                 enqueue(battleSystem->actionQueue, 0, attackIndex, botMonster);
                 
                 // Mudar estado para executar as ações
                 battleSystem->battleState = BATTLE_EXECUTING_ACTIONS;
             }
             break;
             
         case 1: // Trocar
             {
                 PokeMonster* newMonster = botChooseMonster(battleSystem->opponentTeam, playerMonster);
                 
                 if (newMonster && newMonster != botMonster) {
                     // Determinar o índice do monstro (simplificado)
                     int monsterIndex = 0;
                     PokeMonster* current = battleSystem->opponentTeam->first;
                     while (current != NULL && current != newMonster) {
                         monsterIndex++;
                         current = current->next;
                     }
                     
                     // Adicionar à fila de ações
                     enqueue(battleSystem->actionQueue, 1, monsterIndex, botMonster);
                     
                     // Mudar estado para executar as ações
                     battleSystem->battleState = BATTLE_EXECUTING_ACTIONS;
                 } else {
                     // Fallback para ataque se não puder trocar
                     int attackIndex = botChooseAttack(botMonster, playerMonster);
                     enqueue(battleSystem->actionQueue, 0, attackIndex, botMonster);
                     battleSystem->battleState = BATTLE_EXECUTING_ACTIONS;
                 }
             }
             break;
             
         case 2: // Usar item
             // O bot só pode usar o item se ainda não foi usado na batalha
             if (!battleSystem->itemUsed) {
                 enqueue(battleSystem->actionQueue, 2, 0, botMonster);
                 battleSystem->battleState = BATTLE_EXECUTING_ACTIONS;
             } else {
                 // Fallback para ataque
                 int attackIndex = botChooseAttack(botMonster, playerMonster);
                 enqueue(battleSystem->actionQueue, 0, attackIndex, botMonster);
                 battleSystem->battleState = BATTLE_EXECUTING_ACTIONS;
             }
             break;
             
         default:
             // Fallback para ataque
             int attackIndex = botChooseAttack(botMonster, playerMonster);
             enqueue(battleSystem->actionQueue, 0, attackIndex, botMonster);
             battleSystem->battleState = BATTLE_EXECUTING_ACTIONS;
             break;
     }
     
     // Passar o turno para o jogador
     battleSystem->playerTurn = true;
 }
 
 // Faz o bot escolher um ataque
 int botChooseAttack(PokeMonster* botMonster, PokeMonster* playerMonster) {
     if (botMonster == NULL || playerMonster == NULL) {
         return 0;
     }
     
     // Usar IA para escolher o ataque
     int attackIndex = getAISuggestedAttack(botMonster, playerMonster);
     
     // Validar o índice
     if (attackIndex < 0 || attackIndex > 3 || botMonster->attacks[attackIndex].ppCurrent <= 0) {
         // Fallback: escolher o primeiro ataque disponível
         for (int i = 0; i < 4; i++) {
             if (botMonster->attacks[i].ppCurrent > 0) {
                 return i;
             }
         }
         return 0; // Se nenhum ataque tiver PP, usar o primeiro mesmo
     }
     
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
     
     for (int i = 0; i < activeCount; i++) {
         PokeMonster* candidate = activeMonstros[i];
         
         // Verificar a efetividade do primeiro ataque contra o jogador
         float effectiveness = calculateTypeEffectiveness(
             candidate->attacks[0].type, 
             playerMonster->type1, 
             playerMonster->type2
         );
         
         if (effectiveness > bestEffectiveness) {
             bestEffectiveness = effectiveness;
             bestMonster = candidate;
         }
     }
     
     // Se não encontrar vantagem clara, escolher aleatoriamente
     if (bestMonster == NULL) {
         return activeMonstros[rand() % activeCount];
     }
     
     return bestMonster;
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