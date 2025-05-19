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

// Efeitos visuais para o novo estilo
static float selectionTimer = 0.0f;
static float cardPulseAnimations[50] = {0}; // Para animar os cards

// Seleção de adversário
void drawOpponentSelection(void)
{
    // Atualizar temporizador para animações
    selectionTimer += GetFrameTime();
    float bgScroll = selectionTimer * 15.0f;

    // Fundo com gradiente azul
    for (int i = 0; i < GetScreenHeight(); i++)
    {
        float factor = (float)i / GetScreenHeight();
        Color lineColor = (Color){
            (unsigned char)(50 * (1.0f - factor) + 10 * factor),
            (unsigned char)(100 * (1.0f - factor) + 50 * factor),
            (unsigned char)(200 * (1.0f - factor) + 150 * factor),
            255
        };
        DrawRectangle(0, i, GetScreenWidth(), 1, lineColor);
    }

    // Desenhar padrão de listras horizontais
    for (int i = 0; i < GetScreenHeight(); i += 20)
    {
        int yPos = i + (int)bgScroll % 20;
        if (yPos < 0) yPos += 20;
        DrawRectangle(0, yPos, GetScreenWidth(), 2, (Color){255, 255, 255, 20});
    }

    // Título com animação de pulso
    const char* title = "Selecione o Modo de Jogo";
    float titleScale = 1.0f + sinf(selectionTimer * 1.5f) * 0.05f;
    int titleFontSize = (int)(40 * titleScale);

    // Sombra do título
    DrawText(title,
             GetScreenWidth() / 2 - MeasureText(title, titleFontSize) / 2 + 3,
             50 + 3,
             titleFontSize,
             (Color){0, 0, 0, 150});

    // Título
    Color titleColor = (Color){
        (unsigned char)(220 + sinf(selectionTimer * 2.0f) * 35),
        (unsigned char)(220 + sinf(selectionTimer * 2.5f) * 35),
        255,
        255
    };
    DrawText(title,
             GetScreenWidth() / 2 - MeasureText(title, titleFontSize) / 2,
             50,
             titleFontSize,
             titleColor);

    // Dimensões
    float buttonWidth = 300 * GetScaleX();
    float buttonHeight = 60 * GetScaleY();
    float spacing = 30 * GetScaleY();
    float titleSpacing = 40 * GetScaleY();

    float totalHeight = titleSpacing + buttonHeight * 2 + spacing;
    float startY = 150;
    float centerX = GetScreenWidth() / 2 - buttonWidth / 2;

    // Botão "Batalha Livre"
    Rectangle freeBattleBtn = {centerX, startY, buttonWidth, buttonHeight};

    // Efeito de pulso para o botão
    float pulseFactor = 1.0f + sinf(selectionTimer * 3.0f) * 0.05f;
    Rectangle pulsedFreeBattleBtn = {
        freeBattleBtn.x - (freeBattleBtn.width * pulseFactor - freeBattleBtn.width) / 2,
        freeBattleBtn.y - (freeBattleBtn.height * pulseFactor - freeBattleBtn.height) / 2,
        freeBattleBtn.width * pulseFactor,
        freeBattleBtn.height * pulseFactor
    };

    if (GuiPokemonButton(pulsedFreeBattleBtn, "BATALHA LIVRE", true))
    {
        PlaySound(selectSound);
        vsBot = true;
        currentScreen = MONSTER_SELECTION;
        teamSelectionCount = 0;
        scrollOffset = 0;

        if (playerTeam != NULL) freeMonsterList(playerTeam);
        playerTeam = createMonsterList();
    }

    // Botão "Modo Carreira" (desabilitado com W.I.P)
    Rectangle careerBtn = {centerX, freeBattleBtn.y + buttonHeight + spacing, buttonWidth, buttonHeight};

    // Desenhar o botão desabilitado manualmente
    Color disabledColor = (Color){100, 100, 100, 200}; // Cinza para botão desabilitado
    DrawRectangleRounded(careerBtn, 0.3f, 8, disabledColor);
    DrawRectangleRoundedLines(careerBtn, 0.3f, 8, BLACK);

    // Texto com W.I.P.
    const char* careerText = "MODO CARREIRA";
    DrawText(careerText,
             careerBtn.x + careerBtn.width / 2 - MeasureText(careerText, 20) / 2,
             careerBtn.y + careerBtn.height / 2 - 15,
             20,
             (Color){200, 200, 200, 200});

    // Rótulo W.I.P.
    const char* wipText = "W.I.P.";
    float wipScale = 0.8f + sinf(selectionTimer * 4.0f) * 0.2f; // Fazendo o W.I.P. pulsar
    DrawText(wipText,
             careerBtn.x + careerBtn.width / 2 - MeasureText(wipText, 16 * wipScale) / 2,
             careerBtn.y + careerBtn.height / 2 + 5,
             16 * wipScale,
             (Color){255, 200, 50, (unsigned char)(150 + 100 * fabsf(sinf(selectionTimer * 2.0f)))});

    // Adicionar descrição do W.I.P.
    const char* wipDesc = "Em desenvolvimento";
    DrawText(wipDesc,
             careerBtn.x + careerBtn.width + 10,
             careerBtn.y + careerBtn.height / 2 - 8,
             16,
             (Color){255, 200, 50, 200});

    // Caixa de ajuda para Batalha Livre
    Vector2 mouse = GetMousePosition();
    bool hoveringFree = CheckCollisionPointRec(mouse, freeBattleBtn);

    if (hoveringFree)
    {
        const char* helpText = "Selecione 3 monstros e batalhe contra um bot com 3 monstros aleatórios.";

        float boxWidth = 600 * GetScaleX();
        float boxHeight = 80 * GetScaleY();
        float boxX = GetScreenWidth() / 2 - boxWidth / 2;
        float boxY = careerBtn.y + buttonHeight + 30 * GetScaleY();

        Rectangle helpBox = {boxX, boxY, boxWidth, boxHeight};

        // Estilo consistente com o menu principal
        DrawRectangleRounded(helpBox, 0.3f, 8, (Color){40, 40, 40, 230});
        DrawRectangleRoundedLines(helpBox, 0.3f, 8, (Color){255, 255, 255, 150});

        float helpFontSize = ScaleFontSize(18);
        Vector2 textSize = MeasureTextEx(gameFont, helpText, helpFontSize, 1);
        Vector2 textPos = {
            helpBox.x + (helpBox.width - textSize.x) / 2,
            helpBox.y + (helpBox.height - textSize.y) / 2
        };
        DrawTextEx(gameFont, helpText, textPos, helpFontSize, 1, WHITE);
    }

    // Desenhar pequenas Pokébolas animadas como decoração
    for (int i = 0; i < 5; i++)
    {
        float pokeX = (GetScreenWidth() / 6) * (i + 1);
        float pokeY = GetScreenHeight() - 100 + sinf(selectionTimer * 1.5f + i * 0.5f) * 20;
        float pokeSize = 12 + sinf(selectionTimer * 2.0f + i * 0.7f) * 3;

        // Desenhar Pokébola
        DrawCircle(pokeX, pokeY, pokeSize, RED);
        DrawCircle(pokeX, pokeY, pokeSize - 2, WHITE);
        DrawRectangle(pokeX - pokeSize, pokeY - 2, pokeSize * 2, 4, BLACK);
        DrawCircle(pokeX, pokeY, 4, BLACK);
        DrawCircle(pokeX, pokeY, 2, WHITE);
    }

    // Botão "Voltar"
    Rectangle backBtnRect = {
        20 * GetScaleX(),
        GetScreenHeight() - 70 * GetScaleY(),
        150 * GetScaleX(),
        50 * GetScaleY()
    };

    if (GuiPokemonButton(backBtnRect, "VOLTAR", true))
    {
        PlaySound(selectSound);
        currentScreen = MAIN_MENU;
    }
}

void updateOpponentSelection(void)
{
    // Atualização da lógica de seleção de adversário, se necessário
}

// Seleção de monstros
void drawMonsterSelection(void)
{
    // Atualizar timer
    selectionTimer += GetFrameTime();
    float bgScroll = selectionTimer * 15.0f;

    // Fundo com gradiente azul
    for (int i = 0; i < GetScreenHeight(); i++)
    {
        float factor = (float)i / GetScreenHeight();
        Color lineColor = (Color){
            (unsigned char)(50 * (1.0f - factor) + 10 * factor),
            (unsigned char)(100 * (1.0f - factor) + 50 * factor),
            (unsigned char)(200 * (1.0f - factor) + 150 * factor),
            255
        };
        DrawRectangle(0, i, GetScreenWidth(), 1, lineColor);
    }

    // Desenhar padrão de listras horizontais
    for (int i = 0; i < GetScreenHeight(); i += 20)
    {
        int yPos = i + (int)bgScroll % 20;
        if (yPos < 0) yPos += 20;
        DrawRectangle(0, yPos, GetScreenWidth(), 2, (Color){255, 255, 255, 20});
    }

    // Título com animação
    const char* instruction = "Selecione 3 monstros para seu time";
    float titleScale = 1.0f + sinf(selectionTimer * 1.5f) * 0.05f;
    int titleFontSize = (int)(35 * titleScale);

    // Sombra do título
    DrawText(instruction,
             GetScreenWidth() / 2 - MeasureText(instruction, titleFontSize) / 2 + 3,
             25 + 3,
             titleFontSize,
             (Color){0, 0, 0, 150});

    // Título
    Color titleColor = (Color){
        (unsigned char)(220 + sinf(selectionTimer * 2.0f) * 35),
        (unsigned char)(220 + sinf(selectionTimer * 2.5f) * 35),
        255,
        255
    };
    DrawText(instruction,
             GetScreenWidth() / 2 - MeasureText(instruction, titleFontSize) / 2,
             25,
             titleFontSize,
             titleColor);

    // Se estiver visualizando estatísticas de um monstro
    if (viewingStats && currentViewedMonster != NULL)
    {
        // Área de estatísticas
        Rectangle statsRect = {
            GetScreenWidth() / 2 - 400,
            80,
            800,
            GetScreenHeight() - 160
        };

        // Desenhar fundo com estilo consistente
        DrawRectangleRounded(statsRect, 0.3f, 8, (Color){40, 40, 40, 230});
        DrawRectangleRoundedLines(statsRect, 0.3f, 8, (Color){255, 255, 255, 150});

        // Nome do Pokémon com efeito de título
        float nameScale = 1.0f + sinf(selectionTimer * 2.0f) * 0.05f;
        int nameFontSize = (int)(30 * nameScale);
        DrawText(currentViewedMonster->name,
                 statsRect.x + statsRect.width / 2 - MeasureText(currentViewedMonster->name, nameFontSize) / 2,
                 statsRect.y + 20,
                 nameFontSize,
                 WHITE);


        // Usar a animação frontal do Pokémon
        if (currentViewedMonster->frontAnimation.frameCount > 0)
        {
            UpdateAnimation(&currentViewedMonster->frontAnimation);
            Texture2D currentFrame = currentViewedMonster->frontAnimation.frames[currentViewedMonster->frontAnimation.currentFrame];

            // Calcular posição central para o sprite
            float scale = 3.0f; // Escala maior para melhor visualização
            float spriteX = statsRect.x + 200 - (currentFrame.width * scale) / 2;
            float spriteY = statsRect.y + 140;

            // Efeito de flutuação suave
            float floatEffect = sinf(selectionTimer * 1.2f) * 5.0f;

            // Desenhar o sprite com efeito
            DrawTextureEx(
                currentFrame,
                (Vector2){spriteX, spriteY + floatEffect},
                0.0f,
                scale,
                WHITE
            );

            // Adicionar sombra sob o sprite
            DrawEllipse(
                spriteX + currentFrame.width * scale / 2,
                spriteY + currentFrame.height * scale + 10,
                currentFrame.width * scale / 2.5f,
                10,
                (Color){0, 0, 0, 100}
            );
        }
        else
        {
            // Fallback caso a animação não esteja disponível
            DrawText("Sprite indisponível", statsRect.x + 200 - MeasureText("Sprite indisponível", 20) / 2,
                     statsRect.y + 140, 20, GRAY);
        }

        // Ícones de tipo
        DrawText("Tipo:", statsRect.x + 450, statsRect.y + 90, 25, WHITE);

        Rectangle type1Rect = {
            statsRect.x + 520,
            statsRect.y + 90,
            80,
            30
        };
        GuiPokemonTypeIcon(type1Rect, currentViewedMonster->type1);

        if (currentViewedMonster->type2 != TYPE_NONE)
        {
            Rectangle type2Rect = {
                statsRect.x + 610,
                statsRect.y + 90,
                80,
                30
            };
            GuiPokemonTypeIcon(type2Rect, currentViewedMonster->type2);
        }

        // Estatísticas
        DrawText("Estatísticas:", statsRect.x + 450, statsRect.y + 130, 25, WHITE);

        // Desenhar barras de stats
        const char* statNames[] = {"HP", "Ataque", "Defesa", "Velocidade"};
        int statValues[] = {
            currentViewedMonster->maxHp,
            currentViewedMonster->attack,
            currentViewedMonster->defense,
            currentViewedMonster->speed
        };

        for (int i = 0; i < 4; i++)
        {
            DrawText(statNames[i], statsRect.x + 450, statsRect.y + 170 + i * 40, 20, WHITE);

            Rectangle statBar = {
                statsRect.x + 550,
                statsRect.y + 170 + i * 40,
                200,
                25
            };

            // Cor específica para cada stat
            Color statColor;
            switch (i)
            {
            case 0: statColor = (Color){255, 50, 50, 255};
                break; // HP
            case 1: statColor = (Color){240, 128, 48, 255};
                break; // Ataque
            case 2: statColor = (Color){48, 96, 240, 255};
                break; // Defesa
            case 3: statColor = (Color){48, 240, 160, 255};
                break; // Velocidade
            default: statColor = WHITE;
            }

            GuiPokemonStatusBar(statBar, statValues[i], 150, NULL, statColor);

            // Texto do valor
            char valueText[8];
            sprintf(valueText, "%d", statValues[i]);
            DrawText(valueText, statBar.x + statBar.width + 10, statBar.y + 5, 18, WHITE);
        }

        // Ataques
        DrawText("Ataques:", statsRect.x + 125, statsRect.y + 310, 25, WHITE);

        for (int i = 0; i < 4; i++)
        {
            int col = i % 2;
            int row = i / 2;
            Rectangle attackRect = {
                statsRect.x + 50 + col * 350,
                statsRect.y + 350 + row * 70,
                300,
                60
            };

            Color attackColor = getTypeColor(currentViewedMonster->attacks[i].type);

            // Desenhar retângulo do ataque
            DrawRectangleRounded(attackRect, 0.3f, 8, attackColor);
            DrawRectangleRoundedLines(attackRect, 0.3f, 8, BLACK);

            // Nome do ataque
            DrawText(currentViewedMonster->attacks[i].name,
                     attackRect.x + 10,
                     attackRect.y + 10,
                     20,
                     WHITE);

            // Poder e precisão
            char attackStats[32];
            if (currentViewedMonster->attacks[i].power > 0)
            {
                sprintf(attackStats, "Poder: %d  Precisão: %d%%",
                        currentViewedMonster->attacks[i].power,
                        currentViewedMonster->attacks[i].accuracy);
            }
            else
            {
                sprintf(attackStats, "Status  Precisão: %d%%",
                        currentViewedMonster->attacks[i].accuracy);
            }

            DrawText(attackStats, attackRect.x + 10, attackRect.y + 35, 15, WHITE);
        }

        // Botão voltar
        Rectangle backBtnRect = {
            statsRect.x + statsRect.width / 2 - 75,
            statsRect.y + statsRect.height - 60,
            150,
            50
        };

        if (GuiPokemonButton(backBtnRect, "VOLTAR", true))
        {
            PlaySound(selectSound);
            viewingStats = false;
        }
    }
    else
    {
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
                         columns * cardWidth + (columns - 1) * spacingX,
                         contentHeight);

        // Calcular linha inicial baseada no scroll
        int startRow = scrollOffset / (int)(cardHeight + spacingY);
        int startIndex = startRow * columns;
        int endIndex = min(monsterCount, startIndex + visibleMonsters + columns);

        float scrollRemainder = scrollOffset - (startRow * (cardHeight + spacingY));

        // Desenhar cards de monstros
        for (int i = startIndex; i < endIndex; i++)
        {
            int row = i / columns - startRow;
            int col = i % columns;

            // Inicializar animação de pulso se necessário
            if (cardPulseAnimations[i] == 0)
            {
                cardPulseAnimations[i] = (float)i * 0.2f; // Defasar as animações
            }
            cardPulseAnimations[i] += GetFrameTime();

            // Calcular pulso para o card
            float pulse = 1.0f + sinf(cardPulseAnimations[i] * 2.0f) * 0.02f;

            Rectangle bounds = {
                startX + col * (cardWidth + spacingX),
                startY + row * (cardHeight + spacingY) - scrollRemainder,
                cardWidth * pulse,
                cardHeight * pulse
            };

            // Centralizar o card pulsante
            bounds.x -= (bounds.width - cardWidth) / 2;
            bounds.y -= (bounds.height - cardHeight) / 2;

            if (bounds.y + bounds.height < startY || bounds.y > startY + contentHeight)
            {
                continue;
            }

            PokeMonster* monster = getMonsterByIndex(i);
            if (monster == NULL) continue;

            // Verificar se o mouse está sobre o card
            Vector2 mousePoint = GetMousePosition();
            bool isHovered = CheckCollisionPointRec(mousePoint, bounds);

            // Fundo do card com efeito hover
            Color cardBgColor = getTypeColor(monster->type1);
            if (isHovered)
            {
                // Destacar quando hover - evitando overflow com verificação explícita
                cardBgColor.r = (cardBgColor.r + 40 > 255) ? 255 : cardBgColor.r + 40;
                cardBgColor.g = (cardBgColor.g + 40 > 255) ? 255 : cardBgColor.g + 40;
                cardBgColor.b = (cardBgColor.b + 40 > 255) ? 255 : cardBgColor.b + 40;
            }

            DrawRectangleRounded(bounds, 0.2f, 8, cardBgColor);
            DrawRectangleRoundedLines(bounds, 0.3f, 8, BLACK);

            // Área interna do card com efeito
            Rectangle innerBounds = {
                bounds.x + 5,
                bounds.y + 5,
                bounds.width - 10,
                bounds.height - 10
            };
            DrawRectangleRounded(innerBounds, 0.2f, 6, (Color){40, 40, 40, 230});

            // Nome com efeito de sombra
            DrawText(monster->name,
                     innerBounds.x + 10 + 1,
                     innerBounds.y + 10 + 1,
                     24,
                     BLACK);
            DrawText(monster->name,
                     innerBounds.x + 10,
                     innerBounds.y + 10,
                     24,
                     WHITE);

            // Tipos com estilo Pokémon
            Rectangle type1Rect = {
                innerBounds.x + 10,
                innerBounds.y + 40,
                60,
                25
            };
            GuiPokemonTypeIcon(type1Rect, monster->type1);

            if (monster->type2 != TYPE_NONE)
            {
                Rectangle type2Rect = {
                    innerBounds.x + 80,
                    innerBounds.y + 40,
                    60,
                    25
                };
                GuiPokemonTypeIcon(type2Rect, monster->type2);
            }

            // Estatísticas básicas com cores distintas
            char hpText[32], atkText[32], defText[32], spdText[32];
            sprintf(hpText, "HP: %d", monster->maxHp);
            sprintf(atkText, "ATK: %d", monster->attack);
            sprintf(defText, "DEF: %d", monster->defense);
            sprintf(spdText, "SPD: %d", monster->speed);

            DrawText(hpText, innerBounds.x + 10, innerBounds.y + 75, 16, (Color){255, 100, 100, 255});
            DrawText(atkText, innerBounds.x + 110, innerBounds.y + 75, 16, (Color){255, 150, 50, 255});
            DrawText(defText, innerBounds.x + 10, innerBounds.y + 95, 16, (Color){100, 100, 255, 255});
            DrawText(spdText, innerBounds.x + 110, innerBounds.y + 95, 16, (Color){100, 255, 100, 255});

            // Verificar se o monstro já está no time
            bool alreadySelected = false;
            if (teamSelectionCount == 0)
            {
                // Verificar no time do jogador 1
                PokeMonster* current = playerTeam->first;
                while (current != NULL)
                {
                    if (strcmp(current->name, monster->name) == 0)
                    {
                        alreadySelected = true;
                        break;
                    }
                    current = current->next;
                }
            }
            else
            {
                // Verificar no time do jogador 2
                PokeMonster* current = opponentTeam->first;
                while (current != NULL)
                {
                    if (strcmp(current->name, monster->name) == 0)
                    {
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
            if (alreadySelected)
            {
                if (GuiPokemonButton(selectBounds, "REMOVER", true))
                {
                    PlaySound(selectSound);

                    if (teamSelectionCount == 0)
                    {
                        removeMonsterByName(playerTeam, monster->name);
                    }
                    else
                    {
                        removeMonsterByName(opponentTeam, monster->name);
                    }
                }
            }
            else
            {
                if (GuiPokemonButton(selectBounds, "SELECIONAR", true))
                {
                    PlaySound(selectSound);

                    PokeMonster* newMonster = createMonsterCopy(monster);

                    if (teamSelectionCount == 0)
                    {
                        addMonster(playerTeam, newMonster);

                        if (playerTeam->count >= 3)
                        {
                            if (vsBot)
                            {
                                opponentTeam = generateRandomTeam(3);
                                startNewBattle(playerTeam, opponentTeam);
                                currentScreen = BATTLE_SCREEN;
                                StopMusicStream(menuMusic);
                                PlayMusicStream(battleMusic);
                            }
                            else
                            {
                                teamSelectionCount = 1;
                                scrollOffset = 0;
                            }
                        }
                    }
                    else
                    {
                        addMonster(opponentTeam, newMonster);

                        if (opponentTeam->count >= 3)
                        {
                            startNewBattle(playerTeam, opponentTeam);
                            currentScreen = BATTLE_SCREEN;
                            StopMusicStream(menuMusic);
                            PlayMusicStream(battleMusic);
                        }
                    }
                }
            }

            // Botão de detalhes
            if (GuiPokemonButton(detailsBounds, "DETALHES", true))
            {
                PlaySound(selectSound);
                viewingStats = true;
                currentViewedMonster = monster;
            }
        }

        EndScissorMode();

        // Desenhar barra de rolagem se necessário
        float contentTotalHeight = totalRows * (cardHeight + spacingY);
        if (contentTotalHeight > contentHeight)
        {
            float scrollbarWidth = 15 * GetScaleX();
            float scrollbarX = startX + columns * (cardWidth + spacingX) + 5;
            float scrollbarHeight = contentHeight;

            // Trilho da barra
            DrawRectangleRounded(
                (Rectangle){scrollbarX, startY, scrollbarWidth, scrollbarHeight},
                0.3f, 6,
                (Color){40, 40, 40, 150}
            );

            // Alça da barra
            float handleHeight = (handleHeight = 30 * GetScaleY()) > (scrollbarHeight * (contentHeight /
                                     contentTotalHeight))
                                     ? handleHeight
                                     : (scrollbarHeight * (contentHeight / contentTotalHeight));
            float scrollRatio = scrollOffset / (contentTotalHeight - contentHeight);
            if (scrollRatio > 1.0f) scrollRatio = 1.0f;
            float handleY = startY + scrollRatio * (scrollbarHeight - handleHeight);

            // Alça com animação de pulso
            float handlePulse = 1.0f + (isQueueEmpty(battleSystem ? battleSystem->actionQueue : NULL)
                                            ? sinf(selectionTimer * 3.0f) * 0.1f
                                            : 0.0f);

            DrawRectangleRounded(
                (Rectangle){scrollbarX, handleY, scrollbarWidth, handleHeight},
                0.3f, 6,
                (Color){100, 150, 250, 200}
            );

            // Interação com a barra
            Rectangle handleRect = {scrollbarX, handleY, scrollbarWidth, handleHeight};
            Rectangle trackRect = {scrollbarX, startY, scrollbarWidth, scrollbarHeight};

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                if (CheckCollisionPointRec(GetMousePosition(), handleRect))
                {
                    draggingScrollbar = true;
                    dragStartY = GetMousePosition().y;
                    initialScrollOffset = scrollOffset;
                }
                else if (CheckCollisionPointRec(GetMousePosition(), trackRect))
                {
                    float clickPos = (GetMousePosition().y - startY) / scrollbarHeight;
                    scrollOffset = clickPos * (contentTotalHeight - contentHeight);
                }
            }

            if (draggingScrollbar && IsMouseButtonDown(MOUSE_LEFT_BUTTON))
            {
                float dragDelta = GetMousePosition().y - dragStartY;
                float dragRatio = dragDelta / (scrollbarHeight - handleHeight);
                scrollOffset = initialScrollOffset + dragRatio * (contentTotalHeight - contentHeight);

                // Limites
                if (scrollOffset < 0) scrollOffset = 0;
                if (scrollOffset > contentTotalHeight - contentHeight)
                    scrollOffset = contentTotalHeight - contentHeight;
            }

            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
            {
                draggingScrollbar = false;
            }
        }

        // Botão voltar com estilo consistente
        Rectangle backBtnRect = {
            20,
            GetScreenHeight() - 70,
            150,
            50
        };

        if (GuiPokemonButton(backBtnRect, "VOLTAR", true))
        {
            PlaySound(selectSound);

            if (teamSelectionCount == 1 && !vsBot)
            {
                // Voltar para a seleção do jogador 1
                teamSelectionCount = 0;
                scrollOffset = 0;

                // Limpar o time do jogador 2
                if (opponentTeam != NULL)
                {
                    freeMonsterList(opponentTeam);
                    opponentTeam = createMonsterList();
                }
            }
            else
            {
                // Voltar para a seleção de adversário
                currentScreen = OPPONENT_SELECTION;
                scrollOffset = 0;

                // Limpar os times
                if (playerTeam != NULL)
                {
                    freeMonsterList(playerTeam);
                    playerTeam = NULL;
                }

                if (opponentTeam != NULL)
                {
                    freeMonsterList(opponentTeam);
                    opponentTeam = NULL;
                }
            }
        }

        // Contador de seleção com visual melhorado
        if (teamSelectionCount == 0 && playerTeam != NULL)
        {
            // Fundo para o contador
            Rectangle countRect = {
                GetScreenWidth() - 220,
                GetScreenHeight() - 70,
                200,
                50
            };

            DrawRectangleRounded(countRect, 0.3f, 8, (Color){40, 40, 40, 230});
            DrawRectangleRoundedLines(countRect, 0.3f, 8, (Color){255, 255, 255, 150});

            // Texto
            char countText[32];
            sprintf(countText, "Selecionados: %d/3", playerTeam->count);

            DrawText(countText,
                     countRect.x + countRect.width / 2 - MeasureText(countText, 24) / 2,
                     countRect.y + countRect.height / 2 - 12,
                     24,
                     WHITE);

            // Pokébolas representando o progresso
            for (int i = 0; i < 3; i++)
            {
                float pokeX = countRect.x + 50 + i * 50;
                float pokeY = countRect.y + 70;
                float pokeSize = 12;

                if (i < playerTeam->count)
                {
                    // Pokébola preenchida (selecionada)
                    DrawCircle(pokeX, pokeY, pokeSize, RED);
                    DrawCircle(pokeX, pokeY, pokeSize - 2, WHITE);
                }
                else
                {
                    // Pokébola vazia (não selecionada)
                    DrawCircleLines(pokeX, pokeY, pokeSize, (Color){150, 150, 150, 200});
                }

                DrawRectangle(pokeX - pokeSize, pokeY - 2, pokeSize * 2, 4,
                              i < playerTeam->count ? BLACK : (Color){150, 150, 150, 200});
                DrawCircle(pokeX, pokeY, 3, i < playerTeam->count ? BLACK : (Color){150, 150, 150, 200});
            }
        }
        else if (teamSelectionCount == 1 && opponentTeam != NULL)
        {
            // Mesmo visual para o time 2
            Rectangle countRect = {
                GetScreenWidth() - 220,
                GetScreenHeight() - 70,
                200,
                50
            };

            DrawRectangleRounded(countRect, 0.3f, 8, (Color){40, 40, 40, 230});
            DrawRectangleRoundedLines(countRect, 0.3f, 8, (Color){255, 255, 255, 150});

            char countText[32];
            sprintf(countText, "Selecionados: %d/3", opponentTeam->count);

            DrawText(countText,
                     countRect.x + countRect.width / 2 - MeasureText(countText, 24) / 2,
                     countRect.y + countRect.height / 2 - 12,
                     24,
                     WHITE);

            for (int i = 0; i < 3; i++)
            {
                float pokeX = countRect.x + 50 + i * 50;
                float pokeY = countRect.y + 70;
                float pokeSize = 12;

                if (i < opponentTeam->count)
                {
                    DrawCircle(pokeX, pokeY, pokeSize, RED);
                    DrawCircle(pokeX, pokeY, pokeSize - 2, WHITE);
                }
                else
                {
                    DrawCircleLines(pokeX, pokeY, pokeSize, (Color){150, 150, 150, 200});
                }

                DrawRectangle(pokeX - pokeSize, pokeY - 2, pokeSize * 2, 4,
                              i < opponentTeam->count ? BLACK : (Color){150, 150, 150, 200});
                DrawCircle(pokeX, pokeY, 3, i < opponentTeam->count ? BLACK : (Color){150, 150, 150, 200});
            }
        }
    }
}

void updateMonsterSelection(void)
{
    // Processar rolagem com a roda do mouse
    if (!viewingStats)
    {
        float wheelMove = GetMouseWheelMove();
        if (wheelMove != 0)
        {
            scrollOffset -= wheelMove * 30 * GetScaleY(); // 30 pixels por tick da roda
            if (scrollOffset < 0) scrollOffset = 0;
        }
    }
}
