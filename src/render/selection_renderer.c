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

// Variáveis estáticas para seleção
static MonsterList* playerTeam = NULL;
static MonsterList* opponentTeam = NULL;
static int selectedMonsterIndex = 0;
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
    // Desenhar fundo
    ClearBackground(DARKBLUE);

    // Desenhar título com escala
    const char* title = "Selecione seu Adversário";
    float fontSize = ScaleFontSize(40);
    Vector2 titleSize = MeasureTextEx(gameFont, title, fontSize, 2);
    Vector2 titlePos = ScalePosition(GetScreenWidth()/2 - titleSize.x/2, 50);
    DrawTextEx(gameFont, title, titlePos, fontSize, 2, WHITE);

    // Desenhar botões de seleção com escala
    float buttonWidth = 250 * GetScaleX();
    float buttonHeight = 80 * GetScaleY();
    float buttonSpacing = 40 * GetScaleY();
    float startY = GetScreenHeight()/2 - buttonHeight - buttonSpacing/2;

    Rectangle botBtnRect = ScaleRectangle(
        GetScreenWidth()/(2*GetScaleX()) - 125,
        (startY/GetScaleY()),
        250,
        80
    );

    if (drawButton(botBtnRect, "Computador (Bot)", DARKGREEN)) {
        PlaySound(selectSound);
        vsBot = true;
        currentScreen = MONSTER_SELECTION;
        teamSelectionCount = 0;
        scrollOffset = 0; // Resetar posição da barra de rolagem

        // Liberar time anterior se existir
        if (playerTeam != NULL) {
            freeMonsterList(playerTeam);
        }

        // Criar novo time do jogador
        playerTeam = createMonsterList();
    }

    Rectangle humanBtnRect = ScaleRectangle(
        GetScreenWidth()/(2*GetScaleX()) - 125,
        (startY/GetScaleY()) + 120,
        250,
        80
    );

    if (drawButton(humanBtnRect, "Outro Jogador", MAROON)) {
        PlaySound(selectSound);
        vsBot = false;
        currentScreen = MONSTER_SELECTION;
        teamSelectionCount = 0;
        scrollOffset = 0; // Resetar posição da barra de rolagem

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

    // Botão de voltar com escala
    Rectangle backBtnRect = ScaleRectangle(20, GetScreenHeight()/GetScaleY() - 70, 150, 50);

    if (drawButton(backBtnRect, "Voltar", GRAY)) {
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

    // Desenhar instruções com escala
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
    Vector2 instrPos = ScalePosition(GetScreenWidth()/2 - instrSize.x/2, 20);
    DrawTextEx(gameFont, instruction, instrPos, titleFontSize, 2, DARKGRAY);

    // Se estiver visualizando estatísticas de um monstro
    if (viewingStats && currentViewedMonster != NULL) {
        // Desenhar detalhes do monstro
        Rectangle statsRect = ScaleRectangle(50, 80, GetScreenWidth()/GetScaleX() - 100, GetScreenHeight()/GetScaleY() - 160);
        drawMonsterStats(statsRect, currentViewedMonster);

        // Botão de voltar
        Rectangle backBtnRect = ScaleRectangle(
            GetScreenWidth()/(2*GetScaleX()) - 75,
            GetScreenHeight()/GetScaleY() - 70,
            150,
            50
        );

        if (drawButton(backBtnRect, "Voltar", GRAY)) {
            PlaySound(selectSound);
            viewingStats = false;
        }
    } else {
        // Obter número total de monstros
        int monsterCount = getMonsterCount();
        int columns = 3;

        // Definir tamanhos e espaçamentos com escala
        float cardWidth = 230 * GetScaleX();
        float cardHeight = 160 * GetScaleY();
        float spacingX = 20 * GetScaleX();
        float spacingY = 20 * GetScaleY();
        float startX = (GetScreenWidth() - (cardWidth * columns + spacingX * (columns - 1))) / 2;
        float startY = 80 * GetScaleY();

        // Calcular área visível para os monstros
        float contentHeight = GetScreenHeight() - startY - 80 * GetScaleY(); // Altura disponível para cards
        int monstersPerColumn = (int)(contentHeight / (cardHeight + spacingY));
        int visibleMonsters = monstersPerColumn * columns;
        int totalRows = (monsterCount + columns - 1) / columns; // Arredondar para cima

        // Calcular altura total do conteúdo
        float totalContentHeight = totalRows * (cardHeight + spacingY);
        float maxScrollOffset = fmaxf(0, totalContentHeight - contentHeight);

        // Limitar scrollOffset
        if (scrollOffset < 0) scrollOffset = 0;
        if (scrollOffset > maxScrollOffset) scrollOffset = maxScrollOffset;

        // Desenhar área dos cards com clipping
        BeginScissorMode(startX, startY,
                        columns * cardWidth + (columns-1) * spacingX,
                        contentHeight);

        // Calcular linha inicial baseada no scroll
        int startRow = scrollOffset / (cardHeight + spacingY);
        int startIndex = startRow * columns;
        int endIndex = fminf(monsterCount, startIndex + visibleMonsters + columns); // +columns para garantir que mostra uma linha extra se preciso

        // Offset ajustado para o scroll parcial da primeira linha
        float scrollRemainder = scrollOffset - (startRow * (cardHeight + spacingY));

        for (int i = startIndex; i < endIndex; i++) {
            int row = i / columns - startRow;
            int col = i % columns;

            // Calcular posição real considerando o scrollOffset
            Rectangle bounds = {
                startX + col * (cardWidth + spacingX),
                startY + row * (cardHeight + spacingY) - scrollRemainder,
                cardWidth,
                cardHeight
            };

            // Se estiver fora da área visível, não desenhar
            if (bounds.y + bounds.height < startY || bounds.y > startY + contentHeight) {
                continue;
            }

            PokeMonster* monster = getMonsterByIndex(i);
            if (monster != NULL) {
                // Escalar o retângulo para o drawMonsterCard
                Rectangle scaledBounds = {
                    bounds.x / GetScaleX(),
                    bounds.y / GetScaleY(),
                    bounds.width / GetScaleX(),
                    bounds.height / GetScaleY()
                };

                // Desenhar card do monstro
                drawMonsterCard(scaledBounds, monster, i == selectedMonsterIndex);

                // Botões de ação
                Rectangle selectBounds = {
                    scaledBounds.x + 10,
                    scaledBounds.y + scaledBounds.height - 60,
                    scaledBounds.width / 2 - 15,
                    40
                };

                Rectangle statsBounds = {
                    scaledBounds.x + scaledBounds.width / 2 + 5,
                    scaledBounds.y + scaledBounds.height - 60,
                    scaledBounds.width / 2 - 15,
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
                               scrollOffset = 0; // Reset scroll para jogador 2
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

        EndScissorMode();

        // Desenhar barra de rolagem se necessário
        if (totalContentHeight > contentHeight) {
            // Cálculos para a barra de rolagem
            float scrollbarWidth = 20 * GetScaleX();
            float scrollbarX = startX + columns * cardWidth + (columns-1) * spacingX + 10 * GetScaleX();
            float scrollbarHeight = contentHeight;
            float handleSize = fmaxf(30 * GetScaleY(), scrollbarHeight * (contentHeight / totalContentHeight));
            float handleY = startY + (scrollOffset / maxScrollOffset) * (scrollbarHeight - handleSize);

            // Desenhar trilho da barra
            DrawRectangleV(
                (Vector2){ scrollbarX, startY },
                (Vector2){ scrollbarWidth, scrollbarHeight },
                ColorAlpha(GRAY, 0.3f)
            );

            // Desenhar alça da barra
            DrawRectangleV(
                (Vector2){ scrollbarX, handleY },
                (Vector2){ scrollbarWidth, handleSize },
                ColorAlpha(BLUE, 0.7f)
            );

            // Interação com a barra de rolagem
            Vector2 mousePos = GetMousePosition();
            Rectangle handleRect = { scrollbarX, handleY, scrollbarWidth, handleSize };
            Rectangle trackRect = { scrollbarX, startY, scrollbarWidth, scrollbarHeight };

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (CheckCollisionPointRec(mousePos, handleRect)) {
                    draggingScrollbar = true;
                    dragStartY = mousePos.y;
                    initialScrollOffset = scrollOffset;
                }
                else if (CheckCollisionPointRec(mousePos, trackRect)) {
                    // Clicar na trilha posiciona a alça naquele local
                    float clickPos = (mousePos.y - startY) / scrollbarHeight;
                    scrollOffset = clickPos * maxScrollOffset;
                }
            }

            if (draggingScrollbar) {
                if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                    float dragDelta = mousePos.y - dragStartY;
                    float dragRatio = dragDelta / (scrollbarHeight - handleSize);
                    scrollOffset = initialScrollOffset + dragRatio * maxScrollOffset;

                    // Limitar o scroll
                    if (scrollOffset < 0) scrollOffset = 0;
                    if (scrollOffset > maxScrollOffset) scrollOffset = maxScrollOffset;
                }
                else {
                    // Mouse solto, parar de arrastar
                    draggingScrollbar = false;
                }
            }
        }

        // Botão de voltar
        Rectangle backBtnRect = ScaleRectangle(20, GetScreenHeight()/GetScaleY() - 70, 150, 50);

        if (drawButton(backBtnRect, "Voltar", GRAY)) {
            PlaySound(selectSound);

            if (teamSelectionCount == 1 && !vsBot) {
                // Voltar para a seleção do jogador 1
                teamSelectionCount = 0;
                scrollOffset = 0; // Reset scroll

                // Limpar o time do jogador 2
                if (opponentTeam != NULL) {
                    freeMonsterList(opponentTeam);
                    opponentTeam = createMonsterList();
                }
            } else {
                // Voltar para a seleção de adversário
                currentScreen = OPPONENT_SELECTION;
                scrollOffset = 0; // Reset scroll

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
    // Processar rolagem com a roda do mouse
    if (!viewingStats) {
        float wheelMove = GetMouseWheelMove();
        if (wheelMove != 0) {
            scrollOffset -= wheelMove * 30 * GetScaleY(); // 30 pixels por tick da roda
        }
    }
}