/**
 * PokeBattle - Implementação das telas do jogo
 * 
 * Este arquivo contém as implementações das funções para as diferentes telas do jogo.
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include "raylib.h"
 #include "structures.h"
 #include "monsters.h"
 #include "battle.h"
 #include "ia_integration.h"
 #include "screens.h"
 #include <math.h>  // Para a função fmin
 #include "game_state.h"
 
 // Variáveis externas do main.c
 extern BattleSystem* battleSystem;
 extern GameState currentScreen;
 extern bool gameRunning;
 extern bool vsBot;
 extern bool playerTurn;
 
 // Recursos visuais
 static Font gameFont;
 static Texture2D backgroundTexture;
 static Texture2D menuBackground;
 static Texture2D battleBackground;
 static Texture2D monsterSelectBackground;
 static Texture2D typeIcons[TYPE_COUNT];
 static Sound selectSound;
 static Sound attackSound;
 static Sound hitSound;
 static Sound faintSound;
 static Music menuMusic;
 static Music battleMusic;
 
 // Variáveis para a seleção de monstros
 static MonsterList* playerTeam = NULL;
 static MonsterList* opponentTeam = NULL;
 static int selectedMonsterIndex = 0;
 static int teamSelectionCount = 0;
 static bool viewingStats = false;
 static PokeMonster* currentViewedMonster = NULL;
 
 // Variáveis de configuração
 static bool fullscreen = false;
 static float musicVolume = 0.7f;
 static float soundVolume = 0.8f;
 
 // Funções auxiliares de desenho
 static bool drawButton(Rectangle bounds, const char* text, Color color);
 static void drawMonsterCard(Rectangle bounds, PokeMonster* monster, bool selected);
 static void drawTypeIcon(Rectangle bounds, MonsterType type);
 static void drawHealthBar(Rectangle bounds, int currentHP, int maxHP);
 static void drawMonsterStats(Rectangle bounds, PokeMonster* monster);
 static void drawAttackList(Rectangle bounds, PokeMonster* monster, int selectedAttack);
 static void drawMessageBox(Rectangle bounds, const char* message);
 static void drawBattleHUD(void);
 static void drawConfirmationDialog(const char* message, const char* yesText, const char* noText);
 
 // Carrega texturas, fontes e recursos visuais
 void loadTextures(void) {
     // Carregar fonte
     gameFont = GetFontDefault(); // Em um jogo real, carregaria uma fonte personalizada
     
     // Carregar texturas
     backgroundTexture = LoadTexture("resources/background.png");
     menuBackground = LoadTexture("resources/menu_bg.png");
     battleBackground = LoadTexture("resources/battle_bg.png");
     monsterSelectBackground = LoadTexture("resources/select_bg.png");
     
     // Carregar ícones de tipo
     for (int i = 0; i < TYPE_COUNT; i++) {
         char filename[50];
         sprintf(filename, "resources/type_%s.png", getTypeName(i));
         typeIcons[i] = LoadTexture(filename);
     }
 }
 
 void unloadTextures(void) {
     // Descarregar fonte
     // UnloadFont(gameFont); // Se fosse uma fonte carregada
     
     // Descarregar texturas
     UnloadTexture(backgroundTexture);
     UnloadTexture(menuBackground);
     UnloadTexture(battleBackground);
     UnloadTexture(monsterSelectBackground);
     
     // Descarregar ícones de tipo
     for (int i = 0; i < TYPE_COUNT; i++) {
         UnloadTexture(typeIcons[i]);
     }
 }

 void drawBattleHUD(void) {
    // Implementação básica
    if (battleSystem == NULL) return;
    
    // Área para HUD
    Rectangle hudArea = { 0, GetScreenHeight() - 150, GetScreenWidth(), 150 };
    DrawRectangleRec(hudArea, RAYWHITE);
    DrawRectangleLines(hudArea.x, hudArea.y, hudArea.width, hudArea.height, BLACK);
    
    // Exibir informações básicas
    char turnText[32];
    sprintf(turnText, "Turno: %d", battleSystem->turn);
    DrawText(turnText, hudArea.x + 20, hudArea.y + 20, 20, BLACK);
}
 
 // Carrega sons e música
 void loadSounds(void) {
     // Inicializar áudio
     InitAudioDevice();
     
     // Carregar efeitos sonoros
     selectSound = LoadSound("resources/select.wav");
     attackSound = LoadSound("resources/attack.wav");
     hitSound = LoadSound("resources/hit.wav");
     faintSound = LoadSound("resources/faint.wav");
     
     // Carregar músicas
     menuMusic = LoadMusicStream("resources/menu_music.mp3");
     battleMusic = LoadMusicStream("resources/battle_music.mp3");
     
     // Configurar volume
     SetMusicVolume(menuMusic, musicVolume);
     SetMusicVolume(battleMusic, musicVolume);
     SetSoundVolume(selectSound, soundVolume);
     SetSoundVolume(attackSound, soundVolume);
     SetSoundVolume(hitSound, soundVolume);
     SetSoundVolume(faintSound, soundVolume);
     
     // Iniciar música do menu
     PlayMusicStream(menuMusic);
 }
 
 void unloadSounds(void) {
     // Parar músicas
     StopMusicStream(menuMusic);
     StopMusicStream(battleMusic);
     
     // Descarregar efeitos sonoros
     UnloadSound(selectSound);
     UnloadSound(attackSound);
     UnloadSound(hitSound);
     UnloadSound(faintSound);
     
     // Descarregar músicas
     UnloadMusicStream(menuMusic);
     UnloadMusicStream(battleMusic);
     
     // Encerrar áudio
     CloseAudioDevice();
 }
 
 // Menu principal
 void drawMainMenu(void) {
     // Atualizar música do menu
     UpdateMusicStream(menuMusic);
     
     // Desenhar fundo
     ClearBackground(RAYWHITE);
     DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), SKYBLUE);
     
     // Desenhar título
     const char* title = "PokeBattle";
     Vector2 titleSize = MeasureTextEx(gameFont, title, 60, 2);
     DrawTextEx(gameFont, title, (Vector2){ GetScreenWidth()/2 - titleSize.x/2, 50 }, 60, 2, DARKBLUE);
     
     // Desenhar botões do menu
     int buttonWidth = 200;
     int buttonHeight = 50;
     int buttonSpacing = 20;
     int startY = 200;
     
     if (drawButton((Rectangle){ GetScreenWidth()/2 - buttonWidth/2, startY, buttonWidth, buttonHeight }, "Jogar", BLUE)) {
         PlaySound(selectSound);
         currentScreen = OPPONENT_SELECTION;
     }
     
     startY += buttonHeight + buttonSpacing;
     if (drawButton((Rectangle){ GetScreenWidth()/2 - buttonWidth/2, startY, buttonWidth, buttonHeight }, "Configurações", GREEN)) {
         PlaySound(selectSound);
         currentScreen = SETTINGS;
     }
     
     startY += buttonHeight + buttonSpacing;
     if (drawButton((Rectangle){ GetScreenWidth()/2 - buttonWidth/2, startY, buttonWidth, buttonHeight }, "Tabela de Tipos", PURPLE)) {
         PlaySound(selectSound);
         currentScreen = TYPES_TABLE;
     }
     
     startY += buttonHeight + buttonSpacing;
     if (drawButton((Rectangle){ GetScreenWidth()/2 - buttonWidth/2, startY, buttonWidth, buttonHeight }, "Créditos", ORANGE)) {
         PlaySound(selectSound);
         currentScreen = CREDITS;
     }
     
     startY += buttonHeight + buttonSpacing;
     if (drawButton((Rectangle){ GetScreenWidth()/2 - buttonWidth/2, startY, buttonWidth, buttonHeight }, "Sair", RED)) {
         PlaySound(selectSound);
         currentScreen = EXIT;
     }
 }
 
 void updateMainMenu(void) {
     // Atualização da lógica do menu principal, se necessário
 }
 
 // Seleção de adversário
 void drawOpponentSelection(void) {
     // Desenhar fundo
     ClearBackground(DARKBLUE);
     
     // Desenhar título
     const char* title = "Selecione seu Adversário";
     Vector2 titleSize = MeasureTextEx(gameFont, title, 40, 2);
     DrawTextEx(gameFont, title, (Vector2){ GetScreenWidth()/2 - titleSize.x/2, 50 }, 40, 2, WHITE);
     
     // Desenhar botões de seleção
     int buttonWidth = 250;
     int buttonHeight = 80;
     int buttonSpacing = 40;
     int startY = GetScreenHeight()/2 - buttonHeight - buttonSpacing/2;
     
     if (drawButton((Rectangle){ GetScreenWidth()/2 - buttonWidth/2, startY, buttonWidth, buttonHeight }, "Computador (Bot)", DARKGREEN)) {
         PlaySound(selectSound);
         vsBot = true;
         currentScreen = MONSTER_SELECTION;
         teamSelectionCount = 0;
         
         // Liberar time anterior se existir
         if (playerTeam != NULL) {
             freeMonsterList(playerTeam);
         }
         
         // Criar novo time do jogador
         playerTeam = createMonsterList();
     }
     
     startY += buttonHeight + buttonSpacing;
     if (drawButton((Rectangle){ GetScreenWidth()/2 - buttonWidth/2, startY, buttonWidth, buttonHeight }, "Outro Jogador", MAROON)) {
         PlaySound(selectSound);
         vsBot = false;
         currentScreen = MONSTER_SELECTION;
         teamSelectionCount = 0;
         
         // Liberar times anteriores se existirem
         if (playerTeam != NULL) {
             freeMonsterList(playerTeam);
         }
         if (opponentTeam != NULL) {
             freeMonsterList(opponentTeam);
         }
         
         // Criar novos times
         playerTeam = createMonsterList();
         opponentTeam = createMonsterList();
     }
     
     // Botão de voltar
     if (drawButton((Rectangle){ 20, GetScreenHeight() - 70, 150, 50 }, "Voltar", GRAY)) {
         PlaySound(selectSound);
         currentScreen = MAIN_MENU;
     }
 }
 
 void updateOpponentSelection(void) {
     // Atualização da lógica de seleção de adversário, se necessário
 }
 
 // Seleção de monstros
 void drawMonsterSelection(void) {
     // Desenhar fundo
     ClearBackground(LIGHTGRAY);
     
     // Desenhar instruções
     const char* instruction;
     if (vsBot) {
         instruction = "Selecione 3 monstros para seu time";
     } else {
         if (teamSelectionCount == 0) {
             instruction = "Jogador 1: Selecione 3 monstros para seu time";
         } else {
             instruction = "Jogador 2: Selecione 3 monstros para seu time";
         }
     }
     
     Vector2 instrSize = MeasureTextEx(gameFont, instruction, 30, 2);
     DrawTextEx(gameFont, instruction, (Vector2){ GetScreenWidth()/2 - instrSize.x/2, 20 }, 30, 2, DARKGRAY);
     
     // Se estiver visualizando estatísticas de um monstro
     if (viewingStats && currentViewedMonster != NULL) {
         // Desenhar detalhes do monstro
         drawMonsterStats((Rectangle){ 50, 80, GetScreenWidth() - 100, GetScreenHeight() - 160 }, currentViewedMonster);
         
         // Botão de voltar
         if (drawButton((Rectangle){ GetScreenWidth()/2 - 75, GetScreenHeight() - 70, 150, 50 }, "Voltar", GRAY)) {
             PlaySound(selectSound);
             viewingStats = false;
         }
     } else {
         // Desenhar grade de monstros disponíveis
         int monsterCount = getMonsterCount();
         int columns = 3;
         int rows = (monsterCount + columns - 1) / columns;
         
         int cardWidth = 230;
         int cardHeight = 160;
         int spacingX = 20;
         int spacingY = 20;
         int startX = (GetScreenWidth() - (cardWidth * columns + spacingX * (columns - 1))) / 2;
         int startY = 80;
         
         for (int i = 0; i < monsterCount; i++) {
             int row = i / columns;
             int col = i % columns;
             
             Rectangle bounds = {
                 startX + col * (cardWidth + spacingX),
                 startY + row * (cardHeight + spacingY),
                 cardWidth,
                 cardHeight
             };
             
             PokeMonster* monster = getMonsterByIndex(i);
             if (monster != NULL) {
                 // Desenhar card do monstro
                 drawMonsterCard(bounds, monster, i == selectedMonsterIndex);
                 
                 // Botões de ação
                 Rectangle selectBounds = {
                     bounds.x + 10,
                     bounds.y + bounds.height - 60,
                     bounds.width / 2 - 15,
                     40
                 };
                 
                 Rectangle statsBounds = {
                     bounds.x + bounds.width / 2 + 5,
                     bounds.y + bounds.height - 60,
                     bounds.width / 2 - 15,
                     40
                 };
                 
                 // Verificar se o monstro já está no time
                 bool alreadySelected = false;
                 if (teamSelectionCount == 0) {
                     // Verificar no time do jogador 1
                     PokeMonster* current = playerTeam->first;
                     while (current != NULL) {
                         if (strcmp(current->name, monster->name) == 0) {
                             alreadySelected = true;
                             break;
                         }
                         current = current->next;
                     }
                 } else {
                     // Verificar no time do jogador 2
                     PokeMonster* current = opponentTeam->first;
                     while (current != NULL) {
                         if (strcmp(current->name, monster->name) == 0) {
                             alreadySelected = true;
                             break;
                         }
                         current = current->next;
                     }
                 }
                 
                 // Desenhar botão de seleção (desabilitado se já selecionado)
                 Color selectColor = alreadySelected ? GRAY : GREEN;
                 if (!alreadySelected && drawButton(selectBounds, "Selecionar", selectColor)) {
                     PlaySound(selectSound);
                     
                     // Adicionar monstro ao time apropriado
                     PokeMonster* newMonster = createMonsterCopy(monster);
                     
                     if (teamSelectionCount == 0) {
                         // Time do jogador 1
                         addMonster(playerTeam, newMonster);
                         
                         // Se já selecionou 3 monstros
                         if (playerTeam->count >= 3) {
                             if (vsBot) {
                                 // Gerar time do bot aleatoriamente
                                 opponentTeam = generateRandomTeam(3);
                                 
                                 // Iniciar batalha
                                 startNewBattle(playerTeam, opponentTeam);
                                 currentScreen = BATTLE_SCREEN;
                                 StopMusicStream(menuMusic);
                                 PlayMusicStream(battleMusic);
                             } else {
                                 // Passar para a seleção do jogador 2
                                 teamSelectionCount = 1;
                             }
                         }
                     } else {
                         // Time do jogador 2
                         addMonster(opponentTeam, newMonster);
                         
                         // Se já selecionou 3 monstros
                         if (opponentTeam->count >= 3) {
                             // Iniciar batalha
                             startNewBattle(playerTeam, opponentTeam);
                             currentScreen = BATTLE_SCREEN;
                             StopMusicStream(menuMusic);
                             PlayMusicStream(battleMusic);
                         }
                     }
                 }
                 
                 // Desenhar botão de estatísticas
                 if (drawButton(statsBounds, "Detalhes", BLUE)) {
                     PlaySound(selectSound);
                     viewingStats = true;
                     currentViewedMonster = monster;
                 }
             }
         }
         
         // Botão de voltar
         if (drawButton((Rectangle){ 20, GetScreenHeight() - 70, 150, 50 }, "Voltar", GRAY)) {
             PlaySound(selectSound);
             
             if (teamSelectionCount == 1 && !vsBot) {
                 // Voltar para a seleção do jogador 1
                 teamSelectionCount = 0;
                 
                 // Limpar o time do jogador 2
                 if (opponentTeam != NULL) {
                     freeMonsterList(opponentTeam);
                     opponentTeam = createMonsterList();
                 }
             } else {
                 // Voltar para a seleção de adversário
                 currentScreen = OPPONENT_SELECTION;
                 
                 // Limpar os times
                 if (playerTeam != NULL) {
                     freeMonsterList(playerTeam);
                     playerTeam = NULL;
                 }
                 
                 if (opponentTeam != NULL) {
                     freeMonsterList(opponentTeam);
                     opponentTeam = NULL;
                 }
             }
         }
     }
 }
 
 void updateMonsterSelection(void) {
     // Atualização da lógica de seleção de monstros, se necessário
 }
 
 // Tela de batalha
 void drawBattleScreen(void) {
     // Atualizar música da batalha
     UpdateMusicStream(battleMusic);
     
     // Desenhar fundo
     ClearBackground(DARKGREEN);
     
     // Desenhar área de batalha (simplificada)
     DrawRectangle(50, 50, GetScreenWidth() - 100, GetScreenHeight() - 220, DARKBROWN);
     DrawRectangle(55, 55, GetScreenWidth() - 110, GetScreenHeight() - 230, BROWN);
     
     // Desenhar monstros em batalha e suas informações
     if (battleSystem != NULL && 
         battleSystem->playerTeam != NULL && battleSystem->playerTeam->current != NULL &&
         battleSystem->opponentTeam != NULL && battleSystem->opponentTeam->current != NULL) {
         
         PokeMonster* playerMonster = battleSystem->playerTeam->current;
         PokeMonster* opponentMonster = battleSystem->opponentTeam->current;
         
         // Desenhar monstro do oponente (lado esquerdo, topo)
         Rectangle opponentRect = { 100, 80, 200, 200 };
         DrawRectangleRounded(opponentRect, 0.2f, 10, LIGHTGRAY);
         DrawText(opponentMonster->name, opponentRect.x + 10, opponentRect.y + 10, 20, BLACK);
         
         // Tipos do monstro do oponente
         DrawRectangleRec((Rectangle){ opponentRect.x + 10, opponentRect.y + 40, 60, 25 }, getTypeColor(opponentMonster->type1));
         DrawText(getTypeName(opponentMonster->type1), opponentRect.x + 15, opponentRect.y + 45, 15, WHITE);
         
         if (opponentMonster->type2 != TYPE_NONE) {
             DrawRectangleRec((Rectangle){ opponentRect.x + 80, opponentRect.y + 40, 60, 25 }, getTypeColor(opponentMonster->type2));
             DrawText(getTypeName(opponentMonster->type2), opponentRect.x + 85, opponentRect.y + 45, 15, WHITE);
         }
         
         // Barra de HP do oponente
         drawHealthBar((Rectangle){ opponentRect.x + 10, opponentRect.y + 70, 180, 20 }, 
                      opponentMonster->hp, opponentMonster->maxHp);
         
         // Desenhar monstro do jogador (lado direito, baixo)
         Rectangle playerRect = { GetScreenWidth() - 300, 230, 200, 200 };
         DrawRectangleRounded(playerRect, 0.2f, 10, LIGHTGRAY);
         DrawText(playerMonster->name, playerRect.x + 10, playerRect.y + 10, 20, BLACK);
         
         // Tipos do monstro do jogador
         DrawRectangleRec((Rectangle){ playerRect.x + 10, playerRect.y + 40, 60, 25 }, getTypeColor(playerMonster->type1));
         DrawText(getTypeName(playerMonster->type1), playerRect.x + 15, playerRect.y + 45, 15, WHITE);
         
         if (playerMonster->type2 != TYPE_NONE) {
             DrawRectangleRec((Rectangle){ playerRect.x + 80, playerRect.y + 40, 60, 25 }, getTypeColor(playerMonster->type2));
             DrawText(getTypeName(playerMonster->type2), playerRect.x + 85, playerRect.y + 45, 15, WHITE);
         }
         
         // Barra de HP do jogador
         drawHealthBar((Rectangle){ playerRect.x + 10, playerRect.y + 70, 180, 20 }, 
                      playerMonster->hp, playerMonster->maxHp);
         
         // Desenhar caixa de mensagem
         Rectangle messageBox = { 50, GetScreenHeight() - 160, GetScreenWidth() - 100, 60 };
         drawMessageBox(messageBox, getBattleDescription());
         
         // Desenhar interface de batalha baseada no estado atual
         switch (battleSystem->battleState) {
             case BATTLE_SELECT_ACTION:
                 // Mostrar opções de ação se for o turno do jogador
                 if (battleSystem->playerTurn) {
                     // Botões de ação
                     Rectangle actionArea = { 50, GetScreenHeight() - 90, GetScreenWidth() - 100, 80 };
                     DrawRectangleRounded(actionArea, 0.2f, 10, LIGHTGRAY);
                     
                     // Dividir em 4 botões (Lutar, Monstros, Mochila, Desistir)
                     int buttonWidth = (actionArea.width - 30) / 4;
                     
                     // Botão Lutar
                     if (drawButton((Rectangle){ actionArea.x + 10, actionArea.y + 10, buttonWidth, 60 }, "Lutar", RED)) {
                         PlaySound(selectSound);
                         battleSystem->battleState = BATTLE_SELECT_ATTACK;
                     }
                     
                     // Botão Monstros
                     if (drawButton((Rectangle){ actionArea.x + 20 + buttonWidth, actionArea.y + 10, buttonWidth, 60 }, "Monstros", GREEN)) {
                         PlaySound(selectSound);
                         battleSystem->battleState = BATTLE_SELECT_MONSTER;
                     }
                     
                     // Botão Mochila
                     Color itemColor = battleSystem->itemUsed ? GRAY : BLUE;
                     char itemName[32];
                     switch (battleSystem->itemType) {
                         case ITEM_POTION: strcpy(itemName, "Poção"); break;
                         case ITEM_RED_CARD: strcpy(itemName, "Cartão Vermelho"); break;
                         case ITEM_COIN: strcpy(itemName, "Moeda"); break;
                         default: strcpy(itemName, "Item"); break;
                     }
                     
                     if (!battleSystem->itemUsed && drawButton((Rectangle){ actionArea.x + 30 + buttonWidth * 2, actionArea.y + 10, buttonWidth, 60 }, itemName, itemColor)) {
                         PlaySound(selectSound);
                         battleSystem->battleState = BATTLE_ITEM_MENU;
                     } else if (battleSystem->itemUsed) {
                         // Item já usado, mostrar desabilitado
                         DrawRectangleRounded((Rectangle){ actionArea.x + 30 + buttonWidth * 2, actionArea.y + 10, buttonWidth, 60 }, 0.2f, 10, itemColor);
                         DrawText(itemName, actionArea.x + 40 + buttonWidth * 2, actionArea.y + 30, 20, WHITE);
                     }
                     
                     // Botão Desistir
                     if (drawButton((Rectangle){ actionArea.x + 40 + buttonWidth * 3, actionArea.y + 10, buttonWidth, 60 }, "Desistir", PURPLE)) {
                         PlaySound(selectSound);
                         battleSystem->battleState = BATTLE_CONFIRM_QUIT;
                     }
                 } else {
                     // Se for o turno do bot, mostrar mensagem de espera
                     Rectangle waitArea = { 50, GetScreenHeight() - 90, GetScreenWidth() - 100, 80 };
                     DrawRectangleRounded(waitArea, 0.2f, 10, LIGHTGRAY);
                     DrawText("Aguardando ação do oponente...", waitArea.x + 20, waitArea.y + 30, 24, DARKGRAY);
                     
                     // No jogo real, o bot toma sua decisão e executa a ação
                     updateBattle();
                 }
                 break;
                 
             case BATTLE_SELECT_ATTACK:
                 // Mostrar lista de ataques
                 if (battleSystem->playerTurn) {
                     Rectangle attackArea = { 50, GetScreenHeight() - 90, GetScreenWidth() - 100, 80 };
                     DrawRectangleRounded(attackArea, 0.2f, 10, LIGHTGRAY);
                     
                     // Desenhar ataques como botões
                     drawAttackList(attackArea, playerMonster, battleSystem->selectedAttack);
                     
                     // Botão de voltar
                     if (drawButton((Rectangle){ attackArea.x + attackArea.width - 110, attackArea.y + 10, 100, 60 }, "Voltar", GRAY)) {
                         PlaySound(selectSound);
                         battleSystem->battleState = BATTLE_SELECT_ACTION;
                     }
                 }
                 break;
                 
             case BATTLE_SELECT_MONSTER:
                 // Mostrar lista de monstros para troca
                 if (battleSystem->playerTurn) {
                     Rectangle monsterArea = { 50, GetScreenHeight() - 90, GetScreenWidth() - 100, 80 };
                     DrawRectangleRounded(monsterArea, 0.2f, 10, LIGHTGRAY);
                     
                     // Desenhar monstros disponíveis como botões
                     int count = 0;
                     PokeMonster* current = battleSystem->playerTeam->first;
                     
                     while (current != NULL) {
                         Rectangle monsterButton = {
                             monsterArea.x + 10 + count * (monsterArea.width - 120) / 3,
                             monsterArea.y + 10,
                             (monsterArea.width - 120) / 3 - 10,
                             60
                         };
                         
                         // Destacar o monstro atual
                         Color buttonColor = (current == battleSystem->playerTeam->current) ? DARKGRAY : BLUE;
                         
                         // Desabilitar se estiver desmaiado
                         if (isMonsterFainted(current)) {
                             buttonColor = GRAY;
                         }
                         
                         if (!isMonsterFainted(current) && current != battleSystem->playerTeam->current &&
                             drawButton(monsterButton, current->name, buttonColor)) {
                             PlaySound(selectSound);
                             
                             // Enfileirar ação de troca
                             enqueue(battleSystem->actionQueue, 1, count, battleSystem->playerTeam->current);
                             
                             // Passar para a execução de ações
                             battleSystem->battleState = BATTLE_EXECUTING_ACTIONS;
                             battleSystem->playerTurn = false;
                         } else if (current == battleSystem->playerTeam->current || isMonsterFainted(current)) {
                             // Mostrar desabilitado
                             DrawRectangleRounded(monsterButton, 0.2f, 10, buttonColor);
                             DrawText(current->name, monsterButton.x + 10, monsterButton.y + 20, 20, WHITE);
                         }
                         
                         count++;
                         current = current->next;
                     }
                     
                     // Botão de voltar
                     if (drawButton((Rectangle){ monsterArea.x + monsterArea.width - 110, monsterArea.y + 10, 100, 60 }, "Voltar", GRAY)) {
                         PlaySound(selectSound);
                         battleSystem->battleState = BATTLE_SELECT_ACTION;
                     }
                 }
                 break;
                 
             case BATTLE_ITEM_MENU:
                 // Confirmar uso do item
                 if (battleSystem->playerTurn) {
                     Rectangle itemArea = { 50, GetScreenHeight() - 90, GetScreenWidth() - 100, 80 };
                     DrawRectangleRounded(itemArea, 0.2f, 10, LIGHTGRAY);
                     
                     // Mostrar descrição do item
                     char itemDesc[128];
                     switch (battleSystem->itemType) {
                         case ITEM_POTION:
                             sprintf(itemDesc, "Poção: Restaura 20 HP do seu monstro.");
                             break;
                         case ITEM_RED_CARD:
                             sprintf(itemDesc, "Cartão Vermelho: Força o oponente a trocar de monstro.");
                             break;
                         case ITEM_COIN:
                             sprintf(itemDesc, "Moeda: 50%% de chance de curar todo HP, 50%% de chance de desmaiar.");
                             break;
                         default:
                             sprintf(itemDesc, "Item desconhecido.");
                             break;
                     }
                     
                     DrawText(itemDesc, itemArea.x + 20, itemArea.y + 15, 20, BLACK);
                     
                     // Botões de confirmar e cancelar
                     if (drawButton((Rectangle){ itemArea.x + 20, itemArea.y + 40, 150, 30 }, "Usar Item", GREEN)) {
                         PlaySound(selectSound);
                         
                         // Enfileirar ação de usar item
                         enqueue(battleSystem->actionQueue, 2, battleSystem->itemType, battleSystem->playerTeam->current);
                         
                         // Passar para a execução de ações
                         battleSystem->battleState = BATTLE_EXECUTING_ACTIONS;
                         battleSystem->playerTurn = false;
                     }
                     
                     if (drawButton((Rectangle){ itemArea.x + 180, itemArea.y + 40, 150, 30 }, "Cancelar", RED)) {
                         PlaySound(selectSound);
                         battleSystem->battleState = BATTLE_SELECT_ACTION;
                     }
                 }
                 break;
                 
             case BATTLE_CONFIRM_QUIT:
                 // Confirmação para desistir
                 drawConfirmationDialog("Tem certeza que deseja desistir da batalha?", "Sim", "Não");
                 break;
                 
             case BATTLE_OVER:
                 // Mostrar resultado da batalha
                 Rectangle resultArea = { 50, GetScreenHeight() - 90, GetScreenWidth() - 100, 80 };
                 DrawRectangleRounded(resultArea, 0.2f, 10, LIGHTGRAY);
                 
                 int winner = getBattleWinner();
                 if (winner == 1) {
                     DrawText("Você venceu a batalha!", resultArea.x + 20, resultArea.y + 20, 30, GREEN);
                 } else if (winner == 2) {
                     DrawText("Você perdeu a batalha!", resultArea.x + 20, resultArea.y + 20, 30, RED);
                 } else {
                     DrawText("A batalha terminou em empate!", resultArea.x + 20, resultArea.y + 20, 30, BLUE);
                 }
                 
                 // Botão para voltar ao menu
                 if (drawButton((Rectangle){ resultArea.x + resultArea.width - 180, resultArea.y + 30, 160, 40 }, "Menu Principal", BLUE)) {
                     PlaySound(selectSound);
                     StopMusicStream(battleMusic);
                     PlayMusicStream(menuMusic);
                     currentScreen = MAIN_MENU;
                     resetBattle();
                 }
                 break;
                 
             default:
                 break;
         }
     }
 }
 
 void updateBattleScreen(void) {
     // Atualização da lógica de batalha
     if (battleSystem != NULL) {
         processBattleInput();
     }
 }
 
 // Tabela de tipos
 void drawTypesTable(void) {
     // Desenhar fundo
     ClearBackground(RAYWHITE);
     
     // Desenhar título
     const char* title = "Tabela de Tipos";
     Vector2 titleSize = MeasureTextEx(gameFont, title, 40, 2);
     DrawTextEx(gameFont, title, (Vector2){ GetScreenWidth()/2 - titleSize.x/2, 20 }, 40, 2, BLACK);
     
     // Desenhar tabela de efetividade de tipos
     int cellSize = 50;
     int startX = (GetScreenWidth() - (cellSize * (TYPE_COUNT + 1))) / 2;
     int startY = 80;
     
     // Desenhar cabeçalhos de coluna (tipos atacantes)
     for (int i = 0; i < TYPE_COUNT; i++) {
         Rectangle cell = { startX + (i + 1) * cellSize, startY, cellSize, cellSize };
         DrawRectangleRec(cell, getTypeColor(i));
         DrawText(getTypeName(i), cell.x + 5, cell.y + 15, 10, WHITE);
     }
     
     // Desenhar cabeçalhos de linha (tipos defensores)
     for (int i = 0; i < TYPE_COUNT; i++) {
         Rectangle cell = { startX, startY + (i + 1) * cellSize, cellSize, cellSize };
         DrawRectangleRec(cell, getTypeColor(i));
         DrawText(getTypeName(i), cell.x + 5, cell.y + 15, 10, WHITE);
     }
     
     // Desenhar células de efetividade
     for (int i = 0; i < TYPE_COUNT; i++) {
         for (int j = 0; j < TYPE_COUNT; j++) {
             Rectangle cell = { 
                 startX + (j + 1) * cellSize, 
                 startY + (i + 1) * cellSize, 
                 cellSize, 
                 cellSize 
             };
             
             float effectiveness = typeEffectiveness[j][i];
             Color cellColor;
             
             if (effectiveness > 1.5f) {
                 cellColor = GREEN;
             } else if (effectiveness < 0.5f) {
                 cellColor = RED;
             } else if (effectiveness == 0.0f) {
                 cellColor = BLACK;
             } else {
                 cellColor = LIGHTGRAY;
             }
             
             DrawRectangleRec(cell, cellColor);
             
             char effText[10];
             if (effectiveness == 0.0f) {
                 strcpy(effText, "0");
             } else {
                 sprintf(effText, "%.1fx", effectiveness);
             }
             
             DrawText(effText, cell.x + cell.width/2 - MeasureText(effText, 20)/2, 
                     cell.y + cell.height/2 - 10, 20, WHITE);
         }
     }
     
     // Legenda
     DrawText("Efetividade:", startX, startY + (TYPE_COUNT + 1) * cellSize + 20, 20, BLACK);
     
     DrawRectangle(startX, startY + (TYPE_COUNT + 1) * cellSize + 50, 20, 20, GREEN);
     DrawText("Super efetivo (>1.5x)", startX + 30, startY + (TYPE_COUNT + 1) * cellSize + 50, 20, BLACK);
     
     DrawRectangle(startX, startY + (TYPE_COUNT + 1) * cellSize + 80, 20, 20, LIGHTGRAY);
     DrawText("Normal (0.5x - 1.5x)", startX + 30, startY + (TYPE_COUNT + 1) * cellSize + 80, 20, BLACK);
     
     DrawRectangle(startX, startY + (TYPE_COUNT + 1) * cellSize + 110, 20, 20, RED);
     DrawText("Pouco efetivo (<0.5x)", startX + 30, startY + (TYPE_COUNT + 1) * cellSize + 110, 20, BLACK);
     
     DrawRectangle(startX, startY + (TYPE_COUNT + 1) * cellSize + 140, 20, 20, BLACK);
     DrawText("Sem efeito (0x)", startX + 30, startY + (TYPE_COUNT + 1) * cellSize + 140, 20, BLACK);
     
     // Botão de voltar
     if (drawButton((Rectangle){ 20, GetScreenHeight() - 70, 150, 50 }, "Voltar", BLUE)) {
         PlaySound(selectSound);
         currentScreen = MAIN_MENU;
     }
 }
 
 void updateTypesTable(void) {
     // Atualização da lógica da tabela de tipos, se necessário
 }
 
 // Configurações
 void drawSettings(void) {
     // Desenhar fundo
     ClearBackground(DARKBLUE);
     
     // Desenhar título
     const char* title = "Configurações";
     Vector2 titleSize = MeasureTextEx(gameFont, title, 40, 2);
     DrawTextEx(gameFont, title, (Vector2){ GetScreenWidth()/2 - titleSize.x/2, 50 }, 40, 2, WHITE);
     
     // Área de configurações
     Rectangle settingsArea = { GetScreenWidth()/2 - 200, 120, 400, 300 };
     DrawRectangleRounded(settingsArea, 0.2f, 10, LIGHTGRAY);
     
     // Volume da música
     DrawText("Volume da Música", settingsArea.x + 20, settingsArea.y + 30, 20, BLACK);
     
     // Slider para volume da música
     Rectangle musicSlider = { settingsArea.x + 20, settingsArea.y + 60, 360, 30 };
     DrawRectangleRec(musicSlider, DARKGRAY);
     DrawRectangleRec((Rectangle){ musicSlider.x, musicSlider.y, musicSlider.width * musicVolume, musicSlider.height }, GREEN);
     
     // Verificar interação com o slider
     if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
         Vector2 mousePos = GetMousePosition();
         if (CheckCollisionPointRec(mousePos, musicSlider)) {
             musicVolume = (mousePos.x - musicSlider.x) / musicSlider.width;
             
             if (musicVolume < 0.0f) musicVolume = 0.0f;
             if (musicVolume > 1.0f) musicVolume = 1.0f;
             
             // Aplicar volume
             SetMusicVolume(menuMusic, musicVolume);
             SetMusicVolume(battleMusic, musicVolume);
         }
     }
     
     // Volume dos efeitos sonoros
     DrawText("Volume dos Efeitos", settingsArea.x + 20, settingsArea.y + 110, 20, BLACK);
     
     // Slider para volume dos efeitos
     Rectangle soundSlider = { settingsArea.x + 20, settingsArea.y + 140, 360, 30 };
     DrawRectangleRec(soundSlider, DARKGRAY);
     DrawRectangleRec((Rectangle){ soundSlider.x, soundSlider.y, soundSlider.width * soundVolume, soundSlider.height }, BLUE);
     
     // Verificar interação com o slider
     if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
         Vector2 mousePos = GetMousePosition();
         if (CheckCollisionPointRec(mousePos, soundSlider)) {
             soundVolume = (mousePos.x - soundSlider.x) / soundSlider.width;
             
             if (soundVolume < 0.0f) soundVolume = 0.0f;
             if (soundVolume > 1.0f) soundVolume = 1.0f;
             
             // Aplicar volume
             SetSoundVolume(selectSound, soundVolume);
             SetSoundVolume(attackSound, soundVolume);
             SetSoundVolume(hitSound, soundVolume);
             SetSoundVolume(faintSound, soundVolume);
         }
     }
     
     // Opção de tela cheia
     DrawText("Modo Tela Cheia", settingsArea.x + 20, settingsArea.y + 190, 20, BLACK);
     
     // Botão de alternar tela cheia
     Rectangle fullscreenButton = { settingsArea.x + 200, settingsArea.y + 190, 180, 30 };
     if (drawButton(fullscreenButton, fullscreen ? "Desativar" : "Ativar", fullscreen ? RED : GREEN)) {
         PlaySound(selectSound);
         fullscreen = !fullscreen;
         ToggleFullscreen();
     }
     
     // Botão de voltar
     if (drawButton((Rectangle){ settingsArea.x + 125, settingsArea.y + 240, 150, 40 }, "Salvar e Voltar", BLUE)) {
         PlaySound(selectSound);
         currentScreen = MAIN_MENU;
     }
 }
 
 void updateSettings(void) {
     // Atualização da lógica de configurações, se necessário
 }
 
 // Créditos
 void drawCredits(void) {
     // Desenhar fundo
     ClearBackground(BLACK);
     
     // Desenhar título
     const char* title = "Créditos";
     Vector2 titleSize = MeasureTextEx(gameFont, title, 40, 2);
     DrawTextEx(gameFont, title, (Vector2){ GetScreenWidth()/2 - titleSize.x/2, 50 }, 40, 2, WHITE);
     
     // Texto dos créditos
     const char* credits[] = {
         "PokeBattle",
         "Um jogo inspirado em Pokémon Stadium",
         "",
         "Desenvolvido por:",
         "Julia Torres, Maria Claudia, Matheus Martins, Vinicius Jose - Abril/2025",
         "",
         "Projeto para a disciplina de",
         "Algoritmos e Estruturas de Dados",
         "",
         "Agradecimentos:",
         "A todos os professores e colegas",
         "que tornaram este projeto possível.",
         "",
         "Recursos utilizados:",
         "Raylib - Biblioteca gráfica",
         "libcurl - Integração com API",
         "Gemini AI - API de IA para comportamento do bot"
     };
     
     int creditCount = sizeof(credits) / sizeof(credits[0]);
     int startY = 120;
     
     for (int i = 0; i < creditCount; i++) {
         int fontSize = (i == 0) ? 30 : 20;
         Color textColor = (i == 0 || i == 3 || i == 9 || i == 13) ? GOLD : WHITE;
         
         Vector2 textSize = MeasureTextEx(gameFont, credits[i], fontSize, 1);
         DrawTextEx(gameFont, credits[i], (Vector2){ GetScreenWidth()/2 - textSize.x/2, startY }, fontSize, 1, textColor);
         
         startY += fontSize + 10;
     }
     
     // Botão de voltar
     if (drawButton((Rectangle){ GetScreenWidth()/2 - 75, GetScreenHeight() - 70, 150, 50 }, "Voltar", BLUE)) {
         PlaySound(selectSound);
         currentScreen = MAIN_MENU;
     }
 }
 
 void updateCredits(void) {
     // Atualização da lógica dos créditos, se necessário
 }
 
 // Funções auxiliares de desenho
 
 // Desenha um botão e retorna true se foi clicado
 bool drawButton(Rectangle bounds, const char* text, Color color) {
     bool clicked = false;
     Vector2 mousePoint = GetMousePosition();
     
     // Verificar hover
     if (CheckCollisionPointRec(mousePoint, bounds)) {
         // Clarear a cor quando hover
         Color hoverColor = (Color){ 
             (unsigned char)fmin(255, color.r + 40),
             (unsigned char)fmin(255, color.g + 40),
             (unsigned char)fmin(255, color.b + 40),
             color.a
         };
         
         DrawRectangleRounded(bounds, 0.2f, 10, hoverColor);
         
         // Verificar clique
         if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
             clicked = true;
         }
     } else {
         DrawRectangleRounded(bounds, 0.2f, 10, color);
     }
     
     // Desenhar borda
     DrawRectangleRoundedLines(bounds, 0.2f, 10, 2, BLACK);
     
     // Desenhar texto
     Vector2 textSize = MeasureTextEx(gameFont, text, 20, 1);
     DrawTextEx(gameFont, text, (Vector2){ 
         bounds.x + bounds.width/2 - textSize.x/2,
         bounds.y + bounds.height/2 - textSize.y/2
     }, 20, 1, WHITE);
     
     return clicked;
 }
 
 // Desenha um card de monstro
 void drawMonsterCard(Rectangle bounds, PokeMonster* monster, bool selected) {
     // Desenhar fundo do card
     Color cardColor = selected ? SKYBLUE : LIGHTGRAY;
     DrawRectangleRounded(bounds, 0.2f, 10, cardColor);
     
     // Desenhar borda
     DrawRectangleRoundedLines(bounds, 0.2f, 10, selected ? 3 : 1, selected ? BLUE : BLACK);
     
     // Desenhar nome
     DrawText(monster->name, bounds.x + 10, bounds.y + 10, 20, BLACK);
     
     // Desenhar tipos
     Rectangle type1Rect = { bounds.x + 10, bounds.y + 40, 60, 25 };
     DrawRectangleRec(type1Rect, getTypeColor(monster->type1));
     DrawText(getTypeName(monster->type1), type1Rect.x + 5, type1Rect.y + 5, 15, WHITE);
     
     if (monster->type2 != TYPE_NONE) {
         Rectangle type2Rect = { bounds.x + 80, bounds.y + 40, 60, 25 };
         DrawRectangleRec(type2Rect, getTypeColor(monster->type2));
         DrawText(getTypeName(monster->type2), type2Rect.x + 5, type2Rect.y + 5, 15, WHITE);
     }
     
     // Desenhar estatísticas básicas
     char stats[100];
     sprintf(stats, "HP: %d  ATK: %d", monster->maxHp, monster->attack);
     DrawText(stats, bounds.x + 10, bounds.y + 70, 15, BLACK);
     
     sprintf(stats, "DEF: %d  SPD: %d", monster->defense, monster->speed);
     DrawText(stats, bounds.x + 10, bounds.y + 90, 15, BLACK);
 }
 
 // Desenha ícone de tipo
 void drawTypeIcon(Rectangle bounds, MonsterType type) {
     DrawRectangleRec(bounds, getTypeColor(type));
     DrawText(getTypeName(type), bounds.x + 5, bounds.y + 5, 15, WHITE);
 }
 
 // Desenha barra de HP
 void drawHealthBar(Rectangle bounds, int currentHP, int maxHP) {
     // Desenhar fundo da barra
     DrawRectangleRec(bounds, DARKGRAY);
     
     // Calcular preenchimento baseado no HP atual
     float fillRatio = (float)currentHP / maxHP;
     if (fillRatio < 0) fillRatio = 0;
     if (fillRatio > 1) fillRatio = 1;
     
     // Determinar cor baseada no HP
     Color fillColor;
     if (fillRatio > 0.5f) {
         fillColor = GREEN;
     } else if (fillRatio > 0.25f) {
         fillColor = YELLOW;
     } else {
         fillColor = RED;
     }
     
     // Desenhar preenchimento
     DrawRectangleRec((Rectangle){ 
         bounds.x, bounds.y, 
         bounds.width * fillRatio, bounds.height 
     }, fillColor);
     
     // Desenhar borda
     DrawRectangleLinesEx(bounds, 1, BLACK);
     
     // Desenhar texto de HP
     char hpText[32];
     sprintf(hpText, "%d / %d", currentHP, maxHP);
     
     int fontSize = 16;
     Vector2 textSize = MeasureTextEx(gameFont, hpText, fontSize, 1);
     
     DrawText(hpText, 
              bounds.x + bounds.width/2 - textSize.x/2, 
              bounds.y + bounds.height/2 - fontSize/2, 
              fontSize, WHITE);
 }
 
 // Desenha estatísticas detalhadas de um monstro
 void drawMonsterStats(Rectangle bounds, PokeMonster* monster) {
     // Desenhar fundo
     DrawRectangleRounded(bounds, 0.2f, 10, LIGHTGRAY);
     
     // Desenhar nome
     DrawText(monster->name, bounds.x + 20, bounds.y + 20, 30, BLACK);
     
     // Desenhar tipos
     Rectangle type1Rect = { bounds.x + 20, bounds.y + 60, 80, 30 };
     DrawRectangleRec(type1Rect, getTypeColor(monster->type1));
     DrawText(getTypeName(monster->type1), type1Rect.x + 10, type1Rect.y + 5, 20, WHITE);
     
     if (monster->type2 != TYPE_NONE) {
         Rectangle type2Rect = { bounds.x + 110, bounds.y + 60, 80, 30 };
         DrawRectangleRec(type2Rect, getTypeColor(monster->type2));
         DrawText(getTypeName(monster->type2), type2Rect.x + 10, type2Rect.y + 5, 20, WHITE);
     }
     
     // Desenhar estatísticas
     DrawText("Estatísticas:", bounds.x + 20, bounds.y + 100, 25, BLACK);
     
     char statText[50];
     sprintf(statText, "HP: %d", monster->maxHp);
     DrawText(statText, bounds.x + 30, bounds.y + 130, 20, BLACK);
     
     sprintf(statText, "Ataque: %d", monster->attack);
     DrawText(statText, bounds.x + 30, bounds.y + 160, 20, BLACK);
     
     sprintf(statText, "Defesa: %d", monster->defense);
     DrawText(statText, bounds.x + 30, bounds.y + 190, 20, BLACK);
     
     sprintf(statText, "Velocidade: %d", monster->speed);
     DrawText(statText, bounds.x + 30, bounds.y + 220, 20, BLACK);
     
     // Desenhar ataques
     DrawText("Ataques:", bounds.x + bounds.width/2, bounds.y + 100, 25, BLACK);
     
     for (int i = 0; i < 4; i++) {
         Rectangle attackRect = { 
             bounds.x + bounds.width/2, 
             bounds.y + 130 + i * 40, 
             bounds.width/2 - 30, 
             35 
         };
         
         DrawRectangleRec(attackRect, getTypeColor(monster->attacks[i].type));
         DrawText(monster->attacks[i].name, attackRect.x + 10, attackRect.y + 8, 20, WHITE);
         
         char attackInfo[50];
         if (monster->attacks[i].power > 0) {
             sprintf(attackInfo, "Poder: %d  Precisão: %d", 
                    monster->attacks[i].power, monster->attacks[i].accuracy);
         } else {
             sprintf(attackInfo, "Status  Precisão: %d", monster->attacks[i].accuracy);
         }
         
         DrawText(attackInfo, attackRect.x + 10, attackRect.y + 30, 15, BLACK);
     }
 }
 
 // Desenha lista de ataques
 void drawAttackList(Rectangle bounds, PokeMonster* monster, int selectedAttack) {
     // Dividir em 5 áreas: 4 ataques + botão voltar
     int attackWidth = (bounds.width - 130) / 4;
     
     for (int i = 0; i < 4; i++) {
         Rectangle attackBounds = {
             bounds.x + 10 + i * (attackWidth + 10),
             bounds.y + 10,
             attackWidth,
             60
         };
         
         // Verificar PP restante para determinar cor
         Color attackColor = getTypeColor(monster->attacks[i].type);
         
         // Escurecer se PP = 0
         if (monster->attacks[i].ppCurrent <= 0) {
             attackColor.r = attackColor.r / 2;
             attackColor.g = attackColor.g / 2;
             attackColor.b = attackColor.b / 2;
         }
         
         // Desenhar botão de ataque
         bool attackSelected = i == selectedAttack;
         
         if (monster->attacks[i].ppCurrent > 0 && drawButton(attackBounds, monster->attacks[i].name, attackColor)) {
             PlaySound(selectSound);
             battleSystem->selectedAttack = i;
             
             // Enfileirar ação de ataque
             enqueue(battleSystem->actionQueue, 0, i, battleSystem->playerTeam->current);
             
             // Passar para a execução de ações
             battleSystem->battleState = BATTLE_EXECUTING_ACTIONS;
             battleSystem->playerTurn = false;
         } else if (monster->attacks[i].ppCurrent <= 0) {
             // Desenhar ataque desabilitado
             DrawRectangleRounded(attackBounds, 0.2f, 10, attackColor);
             DrawRectangleRoundedLines(attackBounds, 0.2f, 10, 2, BLACK);
             
             Vector2 textSize = MeasureTextEx(gameFont, monster->attacks[i].name, 20, 1);
             DrawTextEx(gameFont, monster->attacks[i].name, (Vector2){ 
                 attackBounds.x + attackBounds.width/2 - textSize.x/2,
                 attackBounds.y + 10
             }, 20, 1, WHITE);
             
             // Mostrar PP
             char ppText[20];
             sprintf(ppText, "PP: %d/%d", monster->attacks[i].ppCurrent, monster->attacks[i].ppMax);
             DrawText(ppText, attackBounds.x + 10, attackBounds.y + 35, 15, WHITE);
         } else {
             // Mostrar informação de PP para ataques disponíveis
             char ppText[20];
             sprintf(ppText, "PP: %d/%d", monster->attacks[i].ppCurrent, monster->attacks[i].ppMax);
             DrawText(ppText, attackBounds.x + 10, attackBounds.y + 35, 15, WHITE);
         }
     }
 }
 
 // Desenha caixa de mensagem
 void drawMessageBox(Rectangle bounds, const char* message) {
     // Desenhar fundo
     DrawRectangleRounded(bounds, 0.2f, 10, WHITE);
     DrawRectangleRoundedLines(bounds, 0.2f, 10, 2, BLACK);
     
     // Desenhar texto
     DrawText(message, bounds.x + 15, bounds.y + bounds.height/2 - 10, 20, BLACK);
 }
 
 // Desenha diálogo de confirmação
 void drawConfirmationDialog(const char* message, const char* yesText, const char* noText) {
     // Desenhar fundo semi-transparente
     DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){ 0, 0, 0, 150 });
     
     // Desenhar caixa de diálogo
     Rectangle dialogBox = {
         GetScreenWidth()/2 - 200,
         GetScreenHeight()/2 - 100,
         400,
         200
     };
     
     DrawRectangleRounded(dialogBox, 0.2f, 10, WHITE);
     DrawRectangleRoundedLines(dialogBox, 0.2f, 10, 2, BLACK);
     
     // Desenhar mensagem
     Vector2 textSize = MeasureTextEx(gameFont, message, 20, 1);
     DrawTextEx(gameFont, message, (Vector2){ 
         dialogBox.x + dialogBox.width/2 - textSize.x/2,
         dialogBox.y + 40
     }, 20, 1, BLACK);
     
     // Desenhar botões
     Rectangle yesButton = {
         dialogBox.x + 50,
         dialogBox.y + 120,
         100,
         40
     };
     
     Rectangle noButton = {
         dialogBox.x + 250,
         dialogBox.y + 120,
         100,
         40
     };
     
     if (drawButton(yesButton, yesText, RED)) {
         PlaySound(selectSound);
         // Voltar ao menu principal
         StopMusicStream(battleMusic);
         PlayMusicStream(menuMusic);
         currentScreen = MAIN_MENU;
         resetBattle();
     }
     
     if (drawButton(noButton, noText, GREEN)) {
         PlaySound(selectSound);
         // Continuar a batalha
         battleSystem->battleState = BATTLE_SELECT_ACTION;
     }
 }