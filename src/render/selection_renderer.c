#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#endif

#include "selection_renderer.h"
#include "hud_elements.h"
#include "resources.h"
#include "../game_state.h"
#include "../battle.h"
#include "../monsters.h"
#include "../scaling.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "globals.h"
#include "gui.h"

static int teamSelectionCount = 0;
static bool viewingStats = false;
static PokeMonster* currentViewedMonster = NULL;

// Variáveis para controle de rolagem
static int scrollOffset = 0;
static bool draggingScrollbar = false;
static float dragStartY = 0;
static float initialScrollOffset = 0;

// Seleção de adversário
void drawOpponentSelection(void) {
    // Fundo padronizado
    ClearBackground((Color){240, 248, 255, 255});

    // Título
    const char* title = "Selecione o Modo de Jogo";
    float fontSize = ScaleFontSize(30);
    Vector2 titleSize = MeasureTextEx(gameFont, title, fontSize, 2);

    // Dimensões
    float buttonWidth = 300 * GetScaleX();
    float buttonHeight = 60 * GetScaleY();
    float spacing = 30 * GetScaleY();
    float titleSpacing = 40 * GetScaleY();

    float totalHeight = titleSize.y + titleSpacing + buttonHeight * 2 + spacing;
    float startY = (GetScreenHeight() - totalHeight) / 2;
    float centerX = GetScreenWidth() / 2 - buttonWidth / 2;

    // Título centralizado
    Vector2 titlePos = { GetScreenWidth() / 2 - titleSize.x / 2, startY };
    DrawTextEx(gameFont, title, titlePos, fontSize, 2, DARKGRAY);

    // Botões
    Rectangle freeBattleBtn = { centerX, startY + titleSize.y + titleSpacing, buttonWidth, buttonHeight };
    Rectangle careerBtn = { centerX, freeBattleBtn.y + buttonHeight + spacing, buttonWidth, buttonHeight };

    Vector2 mouse = GetMousePosition();
    bool hoveringFree = CheckCollisionPointRec(mouse, freeBattleBtn);
    bool hoveringCareer = CheckCollisionPointRec(mouse, careerBtn);

    // Botão "Batalha Livre"
    if (GuiPokemonButton(freeBattleBtn, "BATALHA LIVRE", true)) {
        PlaySound(selectSound);
        vsBot = true;
        currentScreen = MONSTER_SELECTION;
        teamSelectionCount = 0;
        scrollOffset = 0;

        if (playerTeam != NULL) freeMonsterList(playerTeam);
        playerTeam = createMonsterList();
    }

    // Botão "Modo Carreira"
    if (GuiPokemonButton(careerBtn, "MODO CARREIRA", true)) {
        PlaySound(selectSound);
        vsBot = false;
        currentScreen = MONSTER_SELECTION;
        teamSelectionCount = 0;
        scrollOffset = 0;

        if (playerTeam != NULL) freeMonsterList(playerTeam);
        if (opponentTeam != NULL) freeMonsterList(opponentTeam);

        playerTeam = createMonsterList();
        opponentTeam = createMonsterList();
    }

    // Caixa de ajuda (sem ícones)
    const char* helpText = NULL;
    if (hoveringFree) {
        helpText = "Selecione 3 monstros e batalhe contra um bot com 3 monstros aleatórios.";
    } else if (hoveringCareer) {
        helpText = "Enfrente uma sequência de 9 batalhas contra monstros chefes exclusivos.";
    }

    if (helpText != NULL) {
        float boxWidth = 600 * GetScaleX();
        float boxHeight = 80 * GetScaleY();
        float boxX = GetScreenWidth() / 2 - boxWidth / 2;
        float boxY = careerBtn.y + buttonHeight + 30 * GetScaleY();

        Rectangle helpBox = { boxX, boxY, boxWidth, boxHeight };
        DrawRectangleRec(helpBox, (Color){ 248, 248, 255, 255 });
        DrawRectangleLinesEx(helpBox, 2, (Color){ 40, 40, 40, 255 });

        float helpFontSize = ScaleFontSize(18);
        Vector2 textSize = MeasureTextEx(gameFont, helpText, helpFontSize, 1);
        Vector2 textPos = {
            helpBox.x + (helpBox.width - textSize.x) / 2,
            helpBox.y + (helpBox.height - textSize.y) / 2
        };
        DrawTextEx(gameFont, helpText, textPos, helpFontSize, 1, DARKGRAY);
    }

    // Botão "Voltar"
    Rectangle backBtnRect = {
        20 * GetScaleX(),
        GetScreenHeight() - 70 * GetScaleY(),
        150 * GetScaleX(),
        50 * GetScaleY()
    };

    if (GuiPokemonButton(backBtnRect, "VOLTAR", true)) {
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
    ClearBackground((Color){240, 248, 255, 255});

    // Título escalado
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

    float titleFontSize = ScaleFontSize(30);
    Vector2 instrSize = MeasureTextEx(gameFont, instruction, titleFontSize, 2);
    float instrX = (GetScreenWidth() - instrSize.x) / 2;
    float instrY = 20 * GetScaleY();
    DrawTextEx(gameFont, instruction, (Vector2){instrX, instrY}, titleFontSize, 2, DARKGRAY);


    // Se estiver visualizando estatísticas de um monstro
    if (viewingStats && currentViewedMonster != NULL) {
        // Área de estatísticas
        Rectangle statsRect = ScaleRectangle(50, 80, GetScreenWidth()/GetScaleX() - 100, GetScreenHeight()/GetScaleY() - 160);

        // Desenhar fundo
        DrawRectangleRec(statsRect, (Color){ 248, 248, 255, 255 });
        DrawRectangleLinesEx(statsRect, 3, (Color){ 40, 40, 40, 255 });

        // Nome e tipos
        DrawText(currentViewedMonster->name, statsRect.x + 20, statsRect.y + 20, 30, BLACK);

        // Ícones de tipo
        Rectangle type1Rect = {
            statsRect.x + 20,
            statsRect.y + 60,
            80,
            30
        };
        GuiPokemonTypeIcon(type1Rect, currentViewedMonster->type1);

        if (currentViewedMonster->type2 != TYPE_NONE) {
            Rectangle type2Rect = {
                statsRect.x + 110,
                statsRect.y + 60,
                80,
                30
            };
            GuiPokemonTypeIcon(type2Rect, currentViewedMonster->type2);
        }

        // Estatísticas
        DrawText("Estatísticas:", statsRect.x + 20, statsRect.y + 100, 25, BLACK);

        // Desenhar barras de stats
        const char* statNames[] = {"HP", "Ataque", "Defesa", "Velocidade"};
        int statValues[] = {
            currentViewedMonster->maxHp,
            currentViewedMonster->attack,
            currentViewedMonster->defense,
            currentViewedMonster->speed
        };

        for (int i = 0; i < 4; i++) {
            DrawText(statNames[i], statsRect.x + 30, statsRect.y + 130 + i * 40, 20, BLACK);

            Rectangle statBar = {
                statsRect.x + 120,
                statsRect.y + 130 + i * 40,
                200,
                25
            };

            // Calcular valor relativo para a barra (baseado em um máximo de 150)
            float relativeValue = (float)statValues[i] / 150.0f;
            if (relativeValue > 1.0f) relativeValue = 1.0f;

            // Cor específica para cada stat
            Color statColor;
            switch(i) {
                case 0: statColor = (Color){ 255, 50, 50, 255 }; break;   // HP
                case 1: statColor = (Color){ 240, 128, 48, 255 }; break;  // Ataque
                case 2: statColor = (Color){ 48, 96, 240, 255 }; break;   // Defesa
                case 3: statColor = (Color){ 48, 240, 160, 255 }; break;  // Velocidade
                default: statColor = BLACK;
            }

            GuiPokemonStatusBar(statBar, statValues[i], 150, NULL, statColor);

            // Texto do valor
            char valueText[8];
            sprintf(valueText, "%d", statValues[i]);
            DrawText(valueText, statBar.x + statBar.width + 10, statBar.y + 5, 18, BLACK);
        }

        // Ataques
        DrawText("Ataques:", statsRect.x + statsRect.width/2 + 20, statsRect.y + 100, 25, BLACK);

        for (int i = 0; i < 4; i++) {
            Rectangle attackRect = {
                statsRect.x + statsRect.width/2 + 20,
                statsRect.y + 130 + i * 60,
                statsRect.width/2 - 40,
                50
            };

            Color attackColor = getTypeColor(currentViewedMonster->attacks[i].type);

            // Desenhar retângulo do ataque
            DrawRectangleRec(attackRect, attackColor);
            DrawRectangleLinesEx(attackRect, 2, BLACK);

            // Nome do ataque
            DrawText(currentViewedMonster->attacks[i].name,
                    attackRect.x + 10,
                    attackRect.y + 10,
                    20,
                    WHITE);

            // Poder e precisão
            char attackStats[32];
            if (currentViewedMonster->attacks[i].power > 0) {
                sprintf(attackStats, "Poder: %d  Precisão: %d%%",
                       currentViewedMonster->attacks[i].power,
                       currentViewedMonster->attacks[i].accuracy);
            } else {
                sprintf(attackStats, "Status  Precisão: %d%%",
                       currentViewedMonster->attacks[i].accuracy);
            }

            DrawText(attackStats, attackRect.x + 10, attackRect.y + 30, 15, WHITE);
        }

        // Botão voltar
        Rectangle backBtnRect = {
            statsRect.x + statsRect.width/2 - 75,
            statsRect.y + statsRect.height - 60,
            150,
            50
        };

        if (GuiPokemonButton(backBtnRect, "VOLTAR", true)) {
            PlaySound(selectSound);
            viewingStats = false;
        }
    } else {
        // Obter número total de monstros
        int monsterCount = getMonsterCount();

        // Definir tamanhos e espaçamentos com escala
        float cardWidth = 230 * GetScaleX();
        float cardHeight = 180 * GetScaleY();
        float spacingX = 20 * GetScaleX();
        float spacingY = 20 * GetScaleY();

        // Calcular quantos cards cabem em uma linha
        int columns = (int)((GetScreenWidth() - spacingX) / (cardWidth + spacingX));
        if (columns < 1) columns = 1;

        float startX = (GetScreenWidth() - (cardWidth * columns + spacingX * (columns - 1))) / 2;
        float startY = 80 * GetScaleY();

        // Calcular área visível para os monstros
        float contentHeight = GetScreenHeight() - startY - 80 * GetScaleY();
        int monstersPerColumn = (int)(contentHeight / (cardHeight + spacingY));
        int visibleMonsters = monstersPerColumn * columns;
        int totalRows = (monsterCount + columns - 1) / columns;

        // Área de grade rolável
        BeginScissorMode(startX, startY,
                        columns * cardWidth + (columns-1) * spacingX,
                        contentHeight);

        // Calcular linha inicial baseada no scroll
        int startRow = scrollOffset / (int)(cardHeight + spacingY);
        int startIndex = startRow * columns;
        int endIndex = min(monsterCount, startIndex + visibleMonsters + columns);

        float scrollRemainder = scrollOffset - (startRow * (cardHeight + spacingY));

        // Desenhar cards de monstros
        for (int i = startIndex; i < endIndex; i++) {
            int row = i / columns - startRow;
            int col = i % columns;

            Rectangle bounds = {
                startX + col * (cardWidth + spacingX),
                startY + row * (cardHeight + spacingY) - scrollRemainder,
                cardWidth,
                cardHeight
            };

            if (bounds.y + bounds.height < startY || bounds.y > startY + contentHeight) {
                continue;
            }

            PokeMonster* monster = getMonsterByIndex(i);
            if (monster == NULL) continue;

            // Fundo do card
            DrawRectangleRec(bounds, getTypeColor(monster->type1));
            DrawRectangleLinesEx(bounds, 3, BLACK);

            // Área interna do card
            Rectangle innerBounds = {
                bounds.x + 5,
                bounds.y + 5,
                bounds.width - 10,
                bounds.height - 10
            };
            DrawRectangleRec(innerBounds, (Color){ 248, 248, 255, 255 });

            // Nome
            DrawText(monster->name,
                    innerBounds.x + 10,
                    innerBounds.y + 10,
                    24,
                    BLACK);

            // Tipos
            Rectangle type1Rect = {
                innerBounds.x + 10,
                innerBounds.y + 40,
                60,
                25
            };
            GuiPokemonTypeIcon(type1Rect, monster->type1);

            if (monster->type2 != TYPE_NONE) {
                Rectangle type2Rect = {
                    innerBounds.x + 80,
                    innerBounds.y + 40,
                    60,
                    25
                };
                GuiPokemonTypeIcon(type2Rect, monster->type2);
            }

            // Estatísticas básicas
            char hpText[32], atkText[32], defText[32], spdText[32];
            sprintf(hpText, "HP: %d", monster->maxHp);
            sprintf(atkText, "ATK: %d", monster->attack);
            sprintf(defText, "DEF: %d", monster->defense);
            sprintf(spdText, "SPD: %d", monster->speed);

            DrawText(hpText, innerBounds.x + 10, innerBounds.y + 75, 16, BLACK);
            DrawText(atkText, innerBounds.x + 110, innerBounds.y + 75, 16, BLACK);
            DrawText(defText, innerBounds.x + 10, innerBounds.y + 95, 16, BLACK);
            DrawText(spdText, innerBounds.x + 110, innerBounds.y + 95, 16, BLACK);

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

            // Botões de ação
            Rectangle selectBounds = {
                innerBounds.x + 10,
                innerBounds.y + innerBounds.height - 45,
                innerBounds.width / 2 - 20,
                35
            };

            Rectangle detailsBounds = {
                innerBounds.x + innerBounds.width / 2 + 10,
                innerBounds.y + innerBounds.height - 45,
                innerBounds.width / 2 - 20,
                35
            };

            // Desenhar botão de seleção (desabilitado se já selecionado)
            if (!alreadySelected) {
                if (GuiPokemonButton(selectBounds, "SELECIONAR", true)) {
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
                                scrollOffset = 0;
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
            } else {
                // Mostrar que já está selecionado
                DrawRectangleRec(selectBounds, GRAY);
                DrawRectangleLinesEx(selectBounds, 1, BLACK);
                DrawText("SELECIONADO",
                        selectBounds.x + selectBounds.width/2 - MeasureText("SELECIONADO", 16)/2,
                        selectBounds.y + selectBounds.height/2 - 8,
                        16,
                        WHITE);
            }

            // Botão de detalhes
            if (GuiPokemonButton(detailsBounds, "DETALHES", true)) {
                PlaySound(selectSound);
                viewingStats = true;
                currentViewedMonster = monster;
            }
        }

        EndScissorMode();

        // Desenhar barra de rolagem se necessário
        float contentTotalHeight = totalRows * (cardHeight + spacingY);
        if (contentTotalHeight > contentHeight) {
            float scrollbarWidth = 15 * GetScaleX();
            float scrollbarX = startX + columns * (cardWidth + spacingX) + 5;
            float scrollbarHeight = contentHeight;

            // Trilho da barra
            DrawRectangleV(
                (Vector2){ scrollbarX, startY },
                (Vector2){ scrollbarWidth, scrollbarHeight },
                (Color){ 200, 200, 200, 150 }
            );

            // Alça da barra
            float handleHeight = max(30 * GetScaleY(), scrollbarHeight * (contentHeight / contentTotalHeight));
            float scrollRatio = min(1.0f, scrollOffset / (contentTotalHeight - contentHeight));
            float handleY = startY + scrollRatio * (scrollbarHeight - handleHeight);

            DrawRectangleV(
                (Vector2){ scrollbarX, handleY },
                (Vector2){ scrollbarWidth, handleHeight },
                (Color){ 80, 80, 80, 200 }
            );

            // Interação com a barra
            Rectangle handleRect = { scrollbarX, handleY, scrollbarWidth, handleHeight };
            Rectangle trackRect = { scrollbarX, startY, scrollbarWidth, scrollbarHeight };

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (CheckCollisionPointRec(GetMousePosition(), handleRect)) {
                    draggingScrollbar = true;
                    dragStartY = GetMousePosition().y;
                    initialScrollOffset = scrollOffset;
                }
                else if (CheckCollisionPointRec(GetMousePosition(), trackRect)) {
                    float clickPos = (GetMousePosition().y - startY) / scrollbarHeight;
                    scrollOffset = clickPos * (contentTotalHeight - contentHeight);
                }
            }

            if (draggingScrollbar && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                float dragDelta = GetMousePosition().y - dragStartY;
                float dragRatio = dragDelta / (scrollbarHeight - handleHeight);
                scrollOffset = initialScrollOffset + dragRatio * (contentTotalHeight - contentHeight);

                // Limites
                if (scrollOffset < 0) scrollOffset = 0;
                if (scrollOffset > contentTotalHeight - contentHeight)
                    scrollOffset = contentTotalHeight - contentHeight;
            }

            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                draggingScrollbar = false;
            }
        }

        // Botão voltar
        Rectangle backBtnRect = ScaleRectangle(20, GetScreenHeight()/GetScaleY() - 70, 150, 50);

        if (GuiPokemonButton(backBtnRect, "VOLTAR", true)) {
            PlaySound(selectSound);

            if (teamSelectionCount == 1 && !vsBot) {
                // Voltar para a seleção do jogador 1
                teamSelectionCount = 0;
                scrollOffset = 0;

                // Limpar o time do jogador 2
                if (opponentTeam != NULL) {
                    freeMonsterList(opponentTeam);
                    opponentTeam = createMonsterList();
                }
            } else {
                // Voltar para a seleção de adversário
                currentScreen = OPPONENT_SELECTION;
                scrollOffset = 0;

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

        // Contador de seleção
        if (teamSelectionCount == 0 && playerTeam != NULL) {
            char countText[32];
            sprintf(countText, "Selecionados: %d/3", playerTeam->count);
            DrawText(countText,
                   GetScreenWidth() - MeasureText(countText, 24) - 20,
                   GetScreenHeight() - 50,
                   24,
                   BLACK);
        } else if (teamSelectionCount == 1 && opponentTeam != NULL) {
            char countText[32];
            sprintf(countText, "Selecionados: %d/3", opponentTeam->count);
            DrawText(countText,
                   GetScreenWidth() - MeasureText(countText, 24) - 20,
                   GetScreenHeight() - 50,
                   24,
                   BLACK);
        }
    }
}

void updateMonsterSelection(void) {
    if (!viewingStats) {
        float wheelMove = GetMouseWheelMove();
        if (wheelMove != 0) {
            scrollOffset -= wheelMove * 30 * GetScaleY(); // 30 pixels por tick da roda

            // Limites do scroll
            int monsterCount = getMonsterCount();
            float cardHeight = 180 * GetScaleY();
            float spacingY = 20 * GetScaleY();
            int columns = (int)((GetScreenWidth() - spacingY) / (230 * GetScaleX() + spacingY));
            if (columns < 1) columns = 1;

            int totalRows = (monsterCount + columns - 1) / columns;
            float contentHeight = GetScreenHeight() - (80 * GetScaleY()) - (80 * GetScaleY());
            float contentTotalHeight = totalRows * (cardHeight + spacingY);

            if (scrollOffset < 0) scrollOffset = 0;
            if (scrollOffset > contentTotalHeight - contentHeight)
                scrollOffset = contentTotalHeight - contentHeight;
        }
    }
}
