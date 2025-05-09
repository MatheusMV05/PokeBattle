#include "selection_renderer.h"
#include "hud_elements.h"
#include "resources.h"
#include "../game_state.h"
#include "../battle.h"
#include "../monsters.h"
#include <stdio.h> 
#include <string.h>

// Variáveis estáticas para seleção
static MonsterList* playerTeam = NULL;
static MonsterList* opponentTeam = NULL;
static int selectedMonsterIndex = 0;
static int teamSelectionCount = 0;
static bool viewingStats = false;
static PokeMonster* currentViewedMonster = NULL;

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
                   printf("Criado novo monstro: %s\n", newMonster->name); // Debug
                   
                   if (teamSelectionCount == 0) {
                       // Time do jogador 1
                       addMonster(playerTeam, newMonster);
                       printf("Monstros do jogador após adição: %d\n", playerTeam->count); // Debug
                       
                       // Se já selecionou 3 monstros
                       if (playerTeam->count >= 3) {
                           printf("3 monstros selecionados para o jogador!\n"); // Debug
                           
                           if (vsBot) {
                               // Gerar time do bot aleatoriamente
                               opponentTeam = generateRandomTeam(3);
                               printf("Time do bot gerado: %d monstros\n", 
                                       opponentTeam ? opponentTeam->count : 0); // Debug
                               
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