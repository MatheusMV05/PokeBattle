// settings_renderer.c (corrigido)
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#endif

#include "settings_renderer.h"
#include "hud_elements.h"
#include "resources.h"
#include "../game_state.h"
#include <math.h>
#include <stdio.h>
#include "structures.h"
#include "gui.h"
#include "globals.h"
#include "ia_integration.h"

// Definição de resoluções disponíveis
static Resolution availableResolutions[] = {
    { 1920, 1080, "1920x1080" },
    { 1600, 900,  "1600x900" },
    { 1366, 768,  "1366x768" },
    { 1280, 720,  "1280x720" },
    { 1024, 768,  "1024x768" }
};
static int numResolutions = 5;

// Animações
static float settingsTimer = 0.0f;
static float bgScroll = 0.0f;
static float pokeballSpinAngle = 0.0f;

// Função para desenhar uma Pokébola
static void DrawPokeball(float x, float y, float size, float angle) {
    // Círculo externo
    DrawCircle(x, y, size, RED);

    // Círculo interno
    DrawCircle(x, y, size * 0.85f, WHITE);

    // Linha horizontal
    DrawRectangle(x - size, y - size * 0.15f, size * 2, size * 0.3f, BLACK);

    // Botão central
    DrawCircle(x, y, size * 0.15f, BLACK);
    DrawCircle(x, y, size * 0.1f, (Color){100, 100, 100, 255});

    // Brilho animado
    float shineX = x + cosf(angle) * (size * 0.5f);
    float shineY = y + sinf(angle) * (size * 0.5f);
    DrawCircle(shineX, shineY, size * 0.08f, (Color){255, 255, 255, 130});
}

// Função para desenhar a tela de configurações
void drawSettings(void) {
    // Atualizar temporizadores
    settingsTimer += GetFrameTime();
    bgScroll += GetFrameTime() * 30.0f;
    pokeballSpinAngle += GetFrameTime() * 1.2f;

    if (bgScroll > 40.0f) bgScroll -= 40.0f;

    // Desenhar fundo estilo Pokémon
    // Fundo azul gradiente
    for (int i = 0; i < GetScreenHeight(); i++) {
        float factor = (float)i / GetScreenHeight();
        Color lineColor = (Color){
            (unsigned char)(50 * (1.0f - factor) + 20 * factor),
            (unsigned char)(100 * (1.0f - factor) + 70 * factor),
            (unsigned char)(200 * (1.0f - factor) + 150 * factor),
            255
        };
        DrawRectangle(0, i, GetScreenWidth(), 1, lineColor);
    }

    // Desenhar padrão de linhas horizontais
    for (int i = 0; i < GetScreenHeight(); i += 40) {
        int yPos = i + (int)bgScroll;
        if (yPos < 0) yPos += 40;
        DrawRectangle(0, yPos, GetScreenWidth(), 3, (Color){255, 255, 255, 20});
    }

    // Título principal com animação pulsante
    const char* title = "CONFIGURAÇÕES";
    float titlePulse = 1.0f + sinf(settingsTimer * 3.0f) * 0.05f;
    int titleFontSize = (int)(40 * titlePulse);

    // Sombra do título
    DrawText(title,
            GetScreenWidth()/2 - MeasureText(title, titleFontSize)/2 + 3,
            40 + 3,
            titleFontSize,
            (Color){0, 0, 0, 120});

    // Título com cor dinâmica
    Color titleColor = (Color){
        (unsigned char)(220 + sinf(settingsTimer * 2.0f) * 35),
        (unsigned char)(220 + sinf(settingsTimer * 2.5f) * 35),
        255,
        255
    };

    DrawText(title,
            GetScreenWidth()/2 - MeasureText(title, titleFontSize)/2,
            40,
            titleFontSize,
            titleColor);

    // Área principal de configurações
    int settingsWidth = GetScreenWidth() * 0.75f;
    int settingsHeight = GetScreenHeight() - 150;
    Rectangle settingsArea = {
        GetScreenWidth()/2 - settingsWidth/2,
        100,
        settingsWidth,
        settingsHeight
    };

    // Desenhar Pokébola decorativa nos cantos do painel
    float pokeSize = 15 + sinf(settingsTimer * 2.0f) * 2.0f;
    DrawPokeball(settingsArea.x + 20, settingsArea.y + 20, pokeSize, pokeballSpinAngle);
    DrawPokeball(settingsArea.x + settingsArea.width - 20, settingsArea.y + 20, pokeSize, pokeballSpinAngle + PI/2);
    DrawPokeball(settingsArea.x + 20, settingsArea.y + settingsArea.height - 20, pokeSize, pokeballSpinAngle + PI);
    DrawPokeball(settingsArea.x + settingsArea.width - 20, settingsArea.y + settingsArea.height - 20, pokeSize, pokeballSpinAngle + 3*PI/2);

    // Painel de configurações com sombra
    DrawRectangleRounded(
        (Rectangle){settingsArea.x + 4, settingsArea.y + 4, settingsArea.width, settingsArea.height},
        0.05f, 8,
        (Color){0, 0, 0, 100}
    );

    DrawRectangleRounded(settingsArea, 0.05f, 8, (Color){30, 60, 90, 230});
    DrawRectangleRoundedLines(settingsArea, 0.05f, 8, WHITE);

    // Desenhar abas de configuração
    const char* tabs[] = { "ÁUDIO", "VÍDEO", "IA" };
    static int activeTab = 0;

    int tabWidth = settingsWidth / 3;
    int tabHeight = 50;

    for (int i = 0; i < 3; i++) {
        Rectangle tabRect = {
            settingsArea.x + i * tabWidth,
            settingsArea.y,
            tabWidth,
            tabHeight
        };

        Color tabColor = (activeTab == i) ?
                        (Color){0, 102, 204, 255} :
                        (Color){30, 60, 90, 180};

        // Desenhar aba
        if (activeTab == i) {
            DrawRectangleRounded(tabRect, 0.5f, 6, tabColor);
        } else {
            DrawRectangleRec(tabRect, tabColor);
        }

        // Efeito de hover
        if (CheckCollisionPointRec(GetMousePosition(), tabRect) && activeTab != i) {
            DrawRectangleRec(tabRect, (Color){0, 102, 204, 100});

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                activeTab = i;
                PlaySound(selectSound);
            }
        }

        // Texto da aba
        DrawText(tabs[i],
                tabRect.x + tabRect.width/2 - MeasureText(tabs[i], 20)/2,
                tabRect.y + tabRect.height/2 - 10,
                20,
                activeTab == i ? WHITE : (Color){200, 200, 200, 200});
    }

    // Área de conteúdo da aba
    Rectangle contentArea = {
        settingsArea.x + 20,
        settingsArea.y + tabHeight + 20,
        settingsArea.width - 40,
        settingsArea.height - tabHeight - 80
    };

    // Desenhar conteúdo com base na aba ativa
    switch (activeTab) {
        case 0: // ÁUDIO
            {
                // Seção de Áudio com decoração
                const char* audioTitle = "CONFIGURAÇÕES DE ÁUDIO";
                DrawText(audioTitle,
                        contentArea.x + contentArea.width/2 - MeasureText(audioTitle, 28)/2,
                        contentArea.y + 10,
                        28,
                        (Color){220, 220, 255, 255});

                // Linha decorativa
                DrawRectangle(
                    contentArea.x + 30,
                    contentArea.y + 50,
                    contentArea.width - 60,
                    2,
                    (Color){100, 150, 255, 150}
                );

                int yPos = contentArea.y + 70;
                int itemHeight = 90;
                int iconSize = 24;

                // Volume da música
                DrawText("Volume da Música", contentArea.x + iconSize + 15, yPos, 24, WHITE);

                Rectangle musicSlider = {
                    contentArea.x + 50,
                    yPos + 40,
                    contentArea.width - 150,
                    30
                };

                // Barra de fundo com degradê
                DrawRectangleGradientH(
                    musicSlider.x, musicSlider.y, musicSlider.width, musicSlider.height,
                    (Color){40, 60, 100, 150}, (Color){60, 100, 150, 150}
                );

                // Barra de valor
                DrawRectangleGradientH(
                    musicSlider.x,
                    musicSlider.y,
                    musicSlider.width * pendingMusicVolume,
                    musicSlider.height,
                    (Color){0, 150, 220, 255},
                    (Color){100, 200, 255, 255}
                );

                // Manopla
                float handleX = musicSlider.x + musicSlider.width * pendingMusicVolume;
                DrawRectangleRounded(
                    (Rectangle){
                        handleX - 5,
                        musicSlider.y - 5,
                        10,
                        musicSlider.height + 10
                    },
                    0.5f, 5, WHITE
                );

                // Texto do valor com brilho
                char musicText[16];
                sprintf(musicText, "%.0f%%", pendingMusicVolume * 100);
                DrawText(musicText,
                        musicSlider.x + musicSlider.width + 20,
                        musicSlider.y + 5,
                        20,
                        (Color){150 + (int)(sinf(settingsTimer * 5.0f) * 50),
                                220,
                                255,
                                255});

                // Interação com o slider
                if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                    Vector2 mouse = GetMousePosition();
                    if (CheckCollisionPointRec(mouse,
                            (Rectangle){
                                musicSlider.x - 10,
                                musicSlider.y - 10,
                                musicSlider.width + 20,
                                musicSlider.height + 20
                            })) {
                        pendingMusicVolume = (mouse.x - musicSlider.x) / musicSlider.width;
                        if (pendingMusicVolume < 0) pendingMusicVolume = 0;
                        if (pendingMusicVolume > 1) pendingMusicVolume = 1;
                        hasUnsavedChanges = true;
                    }
                }

                yPos += itemHeight;

                // Volume dos efeitos
                DrawTexture(typeIcons[TYPE_ELECTRIC], contentArea.x, yPos + 10, WHITE); // Ícone temático
                DrawText("Volume dos Efeitos Sonoros", contentArea.x + iconSize + 15, yPos, 24, WHITE);

                Rectangle soundSlider = {
                    contentArea.x + 50,
                    yPos + 40,
                    contentArea.width - 150,
                    30
                };

                // Barra de fundo com degradê
                DrawRectangleGradientH(
                    soundSlider.x, soundSlider.y, soundSlider.width, soundSlider.height,
                    (Color){40, 60, 100, 150}, (Color){60, 100, 150, 150}
                );

                // Barra de valor
                DrawRectangleGradientH(
                    soundSlider.x,
                    soundSlider.y,
                    soundSlider.width * pendingSoundVolume,
                    soundSlider.height,
                    (Color){255, 150, 50, 255},
                    (Color){255, 200, 100, 255}
                );

                // Manopla
                handleX = soundSlider.x + soundSlider.width * pendingSoundVolume;
                DrawRectangleRounded(
                    (Rectangle){
                        handleX - 5,
                        soundSlider.y - 5,
                        10,
                        soundSlider.height + 10
                    },
                    0.5f, 5, WHITE
                );

                // Texto do valor com brilho
                char soundText[16];
                sprintf(soundText, "%.0f%%", pendingSoundVolume * 100);
                DrawText(soundText,
                        soundSlider.x + soundSlider.width + 20,
                        soundSlider.y + 5,
                        20,
                        (Color){255,
                                200 + (int)(sinf(settingsTimer * 5.0f) * 50),
                                100,
                                255});

                // Interação com o slider
                if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                    Vector2 mouse = GetMousePosition();
                    if (CheckCollisionPointRec(mouse,
                            (Rectangle){
                                soundSlider.x - 10,
                                soundSlider.y - 10,
                                soundSlider.width + 20,
                                soundSlider.height + 20
                            })) {
                        pendingSoundVolume = (mouse.x - soundSlider.x) / soundSlider.width;
                        if (pendingSoundVolume < 0) pendingSoundVolume = 0;
                        if (pendingSoundVolume > 1) pendingSoundVolume = 1;
                        hasUnsavedChanges = true;
                    }
                }

                // Dica ao usuário no rodapé
                DrawText("Dica: Ajuste o volume para uma melhor experiência de jogo!",
                         contentArea.x + 20,
                         contentArea.y + contentArea.height - 40,
                         18,
                         (Color){200, 200, 255, (unsigned char)(150 + sinf(settingsTimer * 3.0f) * 50)});
            }
            break;

        case 1: // VÍDEO
            {
                // Título da seção
                const char* videoTitle = "CONFIGURAÇÕES DE VÍDEO";
                DrawText(videoTitle,
                        contentArea.x + contentArea.width/2 - MeasureText(videoTitle, 28)/2,
                        contentArea.y + 10,
                        28,
                        (Color){220, 255, 220, 255});

                // Linha decorativa
                DrawRectangle(
                    contentArea.x + 30,
                    contentArea.y + 50,
                    contentArea.width - 60,
                    2,
                    (Color){100, 255, 150, 150}
                );

                int yPos = contentArea.y + 70;
                int itemHeight = 90;
                int iconSize = 24;

                // Tela cheia
                DrawTexture(typeIcons[TYPE_NORMAL], contentArea.x, yPos + 10, WHITE); // Ícone temático
                DrawText("Modo Tela Cheia", contentArea.x + iconSize + 15, yPos, 24, WHITE);

                // Botão de tela cheia mais estilizado
                Rectangle fullscreenToggle = {
                    contentArea.x + 50,
                    yPos + 35,
                    100,
                    40
                };

                // Desenhar toggle estilizado
                DrawRectangleRounded(fullscreenToggle, 0.5f, 8, (Color){60, 80, 120, 180});
                DrawRectangleRoundedLines(fullscreenToggle, 0.5f, 8, (Color){100, 120, 160, 255});

                // Desenhar indicador
                float togglePosX = pendingFullscreen ?
                        fullscreenToggle.x + fullscreenToggle.width - 45 :
                        fullscreenToggle.x + 5;

                Rectangle toggleIndicator = {
                    togglePosX,
                    fullscreenToggle.y + 5,
                    30,
                    30
                };

                // Cor do indicador com brilho
                Color indicatorColor = pendingFullscreen ?
                        (Color){0, 200 + (int)(sinf(settingsTimer * 5.0f) * 50), 100, 255} :
                        (Color){150, 150, 150, 255};

                DrawRectangleRounded(toggleIndicator, 0.5f, 8, indicatorColor);
                DrawRectangleRoundedLines(toggleIndicator, 0.5f, 8, WHITE);

                // Texto do estado
                DrawText(pendingFullscreen ? "ATIVADO" : "DESATIVADO",
                        fullscreenToggle.x + 120,
                        fullscreenToggle.y + 10,
                        20,
                        pendingFullscreen ? GREEN : GRAY);

                // Interação com o toggle
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    if (CheckCollisionPointRec(GetMousePosition(), fullscreenToggle)) {
                        pendingFullscreen = !pendingFullscreen;
                        PlaySound(selectSound);
                        hasUnsavedChanges = true;
                    }
                }

                yPos += itemHeight;

                // Resolução
                DrawTexture(typeIcons[TYPE_PSYCHIC], contentArea.x, yPos + 10, WHITE); // Ícone temático
                DrawText("Resolução de Tela", contentArea.x + iconSize + 15, yPos, 24, WHITE);

                // Botão anterior
                Rectangle prevResButton = {
                    contentArea.x + 50,
                    yPos + 35,
                    40,
                    40
                };

                // Botão próximo
                Rectangle nextResButton = {
                    contentArea.x + 50 + 40 + 220 + 20,
                    yPos + 35,
                    40,
                    40
                };

                // Resolução atual
                Rectangle currentResRect = {
                    contentArea.x + 50 + 40 + 10,
                    yPos + 35,
                    220,
                    40
                };

                // Desenhar botões e display com efeitos
                Color buttonGlow = (Color){
                    100,
                    150,
                    255,
                    (unsigned char)(200 + sinf(settingsTimer * 4.0f) * 55)
                };

                DrawRectangleRounded(prevResButton, 0.5f, 6, buttonGlow);
                DrawRectangleRoundedLines(prevResButton, 0.5f, 6, WHITE);

                DrawRectangleRounded(nextResButton, 0.5f, 6, buttonGlow);
                DrawRectangleRoundedLines(nextResButton, 0.5f, 6, WHITE);

                // Display de resolução com degradê
                DrawRectangleGradientH(
                    currentResRect.x, currentResRect.y, currentResRect.width, currentResRect.height,
                    (Color){30, 45, 80, 200}, (Color){60, 90, 120, 200}
                );
                DrawRectangleRoundedLines(currentResRect, 0.3f, 6, WHITE);

                // Setas de navegação com efeito
                float arrowScale = 1.0f + sinf(settingsTimer * 5.0f) * 0.1f;
                DrawText("<",
                        prevResButton.x + prevResButton.width/2 - 6 * arrowScale,
                        prevResButton.y + prevResButton.height/2 - 12 * arrowScale,
                        (int)(24 * arrowScale),
                        WHITE);

                DrawText(">",
                        nextResButton.x + nextResButton.width/2 - 6 * arrowScale,
                        nextResButton.y + nextResButton.height/2 - 12 * arrowScale,
                        (int)(24 * arrowScale),
                        WHITE);

                // Resolução atual com efeito de brilho
                Color resTextColor = (Color){
                    (unsigned char)(200 + sinf(settingsTimer * 3.0f) * 55),
                    (unsigned char)(200 + sinf(settingsTimer * 3.5f) * 55),
                    255,
                    255
                };

                DrawText(availableResolutions[pendingResolutionIndex].description,
                        currentResRect.x + currentResRect.width/2 -
                        MeasureText(availableResolutions[pendingResolutionIndex].description, 20)/2,
                        currentResRect.y + currentResRect.height/2 - 10,
                        20,
                        resTextColor);

                // Interação com os botões de resolução
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    if (CheckCollisionPointRec(GetMousePosition(), prevResButton)) {
                        pendingResolutionIndex = (pendingResolutionIndex - 1 + numResolutions) % numResolutions;
                        PlaySound(selectSound);
                        hasUnsavedChanges = true;
                    }
                    else if (CheckCollisionPointRec(GetMousePosition(), nextResButton)) {
                        pendingResolutionIndex = (pendingResolutionIndex + 1) % numResolutions;
                        PlaySound(selectSound);
                        hasUnsavedChanges = true;
                    }
                }

                // Dica sobre reinicialização
                const char* resolutionNote = "Nota: Alterações na resolução serão aplicadas imediatamente.";
                DrawText(resolutionNote,
                         contentArea.x + 50,
                         yPos + 90,
                         18,
                         (Color){255, 255, 200, (unsigned char)(150 + sinf(settingsTimer * 3.0f) * 50)});

            }
            break;

        case 2: // IA
            {
                // Título da seção
                const char* iaTitle = "CONFIGURAÇÕES DE INTELIGÊNCIA ARTIFICIAL";
                DrawText(iaTitle,
                        contentArea.x + contentArea.width/2 - MeasureText(iaTitle, 28)/2,
                        contentArea.y + 10,
                        28,
                        (Color){255, 220, 255, 255});

                // Linha decorativa
                DrawRectangle(
                    contentArea.x + 30,
                    contentArea.y + 50,
                    contentArea.width - 60,
                    2,
                    (Color){255, 150, 200, 150}
                );

                int yPos = contentArea.y + 70;

                // Estado atual da IA
                Rectangle statusBox = {
                    contentArea.x + 50,
                    yPos,
                    contentArea.width - 100,
                    80
                };

                // Fundo da caixa de status com degradê
                DrawRectangleGradientH(
                    statusBox.x, statusBox.y, statusBox.width, statusBox.height,
                    (Color){40, 40, 60, 200}, (Color){60, 40, 80, 200}
                );
                DrawRectangleRoundedLines(statusBox, 0.2f, 6, (Color){100, 100, 140, 255});

                // Status da IA
                const char* statusPrefix = "Status Atual da IA: ";
                DrawText(statusPrefix,
                         statusBox.x + 20,
                         statusBox.y + 20,
                         24,
                         WHITE);

                // Texto do status com cor dependendo do estado
                const char* statusText = initialized ? "CONECTADO" : "DESCONECTADO";
                Color statusColor = initialized ?
                    (Color){100, 255, 100, 255} :
                    (Color){255, 100, 100, 255};

                // Efeito de pulsação no status
                float statusPulse = 1.0f + sinf(settingsTimer * 4.0f) * 0.1f;
                DrawText(statusText,
                         statusBox.x + 20 + MeasureText(statusPrefix, 24),
                         statusBox.y + 20,
                         24 * statusPulse,
                         statusColor);

                // Descrição do status
                const char* statusDesc = initialized ?
                    "O bot está usando IA para tomar decisões inteligentes." :
                    "O bot está usando algoritmo simples (offline).";

                DrawText(statusDesc,
                         statusBox.x + 20,
                         statusBox.y + 50,
                         18,
                         (Color){200, 200, 200, 255});

                yPos += 100;

                // Teste de conexão
                DrawText("Teste de Conexão com IA",
                         contentArea.x + 50,
                         yPos + 20,
                         24,
                         WHITE);

                // Botão de teste estilizado
                Rectangle testApiBtn = {
                    contentArea.x + 50,
                    yPos + 60,
                    250,
                    50
                };

                // Efeito de brilho no botão
                Color testBtnColor = (Color){
                    (unsigned char)(100 + sinf(settingsTimer * 2.5f) * 30),
                    (unsigned char)(50 + sinf(settingsTimer * 3.0f) * 20),
                    (unsigned char)(200 + sinf(settingsTimer * 3.5f) * 30),
                    255
                };

                DrawRectangleGradientH(
                    testApiBtn.x, testApiBtn.y, testApiBtn.width, testApiBtn.height,
                    testBtnColor, (Color){80, 30, 150, 255}
                );
                DrawRectangleRoundedLines(testApiBtn, 0.3f, 8, WHITE);

                // Ícone de conexão animado
                float iconScale = 1.0f + sinf(settingsTimer * 5.0f) * 0.2f;
                DrawPokeball(
                    testApiBtn.x + 25,
                    testApiBtn.y + testApiBtn.height/2,
                    10 * iconScale,
                    settingsTimer * 3.0f
                );

                DrawText("Testar Conexão",
                        testApiBtn.x + 50,
                        testApiBtn.y + testApiBtn.height/2 - 10,
                        20,
                        WHITE);

                // Interação com botão de teste
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
                    CheckCollisionPointRec(GetMousePosition(), testApiBtn)) {
                    PlaySound(selectSound);
                    testAIConnection();
                }

                yPos += 130;

                // Caixa informativa sobre a IA
                Rectangle infoBox = {
                    contentArea.x + 50,
                    yPos,
                    contentArea.width - 100,
                    150
                };

                DrawRectangleRounded(infoBox, 0.3f, 8, (Color){50, 30, 70, 180});
                DrawRectangleRoundedLines(infoBox, 0.3f, 8, (Color){150, 100, 200, 255});

                const char* infoTitle = "Sobre a IA no PokeBattle";
                DrawText(infoTitle,
                         infoBox.x + infoBox.width/2 - MeasureText(infoTitle, 20)/2,
                         infoBox.y + 15,
                         20,
                         (Color){220, 180, 255, 255});

                const char* infoText1 = " A inteligência artificial melhora as decisões do bot";
                const char* infoText2 = " Escolhe ataques baseados em efetividade de tipo";
                const char* infoText3 = " Avalia situações estratégicas durante a batalha";
                const char* infoText4 = " Funciona online via API do Gemini (Google)";

                DrawText(infoText1, infoBox.x + 20, infoBox.y + 45, 16, WHITE);
                DrawText(infoText2, infoBox.x + 20, infoBox.y + 70, 16, WHITE);
                DrawText(infoText3, infoBox.x + 20, infoBox.y + 95, 16, WHITE);
                DrawText(infoText4, infoBox.x + 20, infoBox.y + 120, 16, WHITE);
            }
            break;
    }

    // Botões inferiores
    int buttonWidth = 180;
    int buttonHeight = 50;
    int bottomY = settingsArea.y + settingsArea.height - buttonHeight - 15;

    // Botão Aplicar (só aparece se tiver mudanças)
    if (hasUnsavedChanges) {
        Rectangle applyBtn = {
            settingsArea.x + settingsArea.width/2 - buttonWidth - 10,
            bottomY,
            buttonWidth,
            buttonHeight
        };

        // Efeito de brilho no botão de aplicar
        Color applyBtnColor = (Color){
            (unsigned char)(0 + sinf(settingsTimer * 2.0f) * 20),
            (unsigned char)(180 + sinf(settingsTimer * 2.5f) * 20),
            (unsigned char)(90 + sinf(settingsTimer * 3.0f) * 20),
            255
        };

        DrawRectangleGradientH(
            applyBtn.x, applyBtn.y, applyBtn.width, applyBtn.height,
            applyBtnColor, (Color){0, 140, 60, 255}
        );
        DrawRectangleRoundedLines(applyBtn, 0.3f, 8, WHITE);

        float applyScale = 1.0f + sinf(settingsTimer * 4.0f) * 0.05f;
        DrawText("APLICAR",
                applyBtn.x + applyBtn.width/2 - MeasureText("APLICAR", 24 * applyScale)/2,
                applyBtn.y + applyBtn.height/2 - 12 * applyScale,
                24 * applyScale,
                WHITE);

        // Interação
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
            CheckCollisionPointRec(GetMousePosition(), applyBtn)) {
            PlaySound(selectSound);
            applySettings();
            hasUnsavedChanges = false;
        }
    }

    // Botão Voltar
    Rectangle backBtn = {
        hasUnsavedChanges ?
            settingsArea.x + settingsArea.width/2 + 10 :
            settingsArea.x + settingsArea.width/2 - buttonWidth/2,
        bottomY,
        buttonWidth,
        buttonHeight
    };

    // Gradiente para o botão de voltar
    DrawRectangleGradientH(
        backBtn.x, backBtn.y, backBtn.width, backBtn.height,
        (Color){120, 120, 120, 255}, (Color){80, 80, 80, 255}
    );
    DrawRectangleRoundedLines(backBtn, 0.3f, 8, WHITE);

    DrawText(hasUnsavedChanges ? "CANCELAR" : "VOLTAR",
            backBtn.x + backBtn.width/2 - MeasureText(hasUnsavedChanges ? "CANCELAR" : "VOLTAR", 24)/2,
            backBtn.y + backBtn.height/2 - 12,
            24,
            WHITE);

    // Interação
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
        CheckCollisionPointRec(GetMousePosition(), backBtn)) {
        PlaySound(selectSound);

        // Se tiver mudanças não salvas, reverter para valores atuais
        if (hasUnsavedChanges) {
            pendingMusicVolume = musicVolume;
            pendingSoundVolume = soundVolume;
            pendingFullscreen = fullscreen;
            pendingResolutionIndex = currentResolutionIndex;
            hasUnsavedChanges = false;
        }

        currentScreen = MAIN_MENU;
    }

    // Se tiver mudanças não salvas, mostrar indicador
    if (hasUnsavedChanges) {
        // Indicador com efeito pulsante
        Color changeColor = (Color){
            255,
            (unsigned char)(200 + sinf(settingsTimer * 4.0f) * 55),
            (unsigned char)(100 + sinf(settingsTimer * 4.0f) * 55),
            255
        };

        DrawText("* Mudanças não salvas",
                settingsArea.x + 20,
                bottomY + 15,
                18,
                changeColor);
    }

    // Decorações - pequenas Pokébolas flutuando
    for (int i = 0; i < 10; i++) {
        float time = settingsTimer + i * 0.5f;
        float x = sinf(time * 0.2f + i) * GetScreenWidth() * 0.4f + GetScreenWidth() * 0.5f;
        float y = cosf(time * 0.3f + i) * GetScreenHeight() * 0.3f + GetScreenHeight() * 0.5f;
        float size = 5 + sinf(time) * 2;

        if (i % 3 == 0) {
            // Algumas Pokébolas
            DrawPokeball(x, y, size, time * 1.5f);
        } else {
            // Algumas partículas brilhantes
            Color particleColor = (Color){
                (unsigned char)(100 + sinf(time * 0.3f) * 70),
                (unsigned char)(150 + sinf(time * 0.5f) * 50),
                (unsigned char)(200 + sinf(time * 0.7f) * 55),
                150
            };

            DrawCircle(x, y, size, particleColor);
        }
    }
}

void applySettings(void) {
    // Atualizar volumes
    if (musicVolume != pendingMusicVolume) {
        musicVolume = pendingMusicVolume;

        // Aplicar volume da música imediatamente
        SetMusicVolume(menuMusic, musicVolume);
        SetMusicVolume(battleMusic, musicVolume);
    }

    if (soundVolume != pendingSoundVolume) {
        soundVolume = pendingSoundVolume;

        // Aplicar volume dos efeitos sonoros
        SetSoundVolume(selectSound, soundVolume);
        SetSoundVolume(attackSound, soundVolume);
        SetSoundVolume(hitSound, soundVolume);
        SetSoundVolume(faintSound, soundVolume);
    }

    // Aplicar resolução somente se mudou
    if (currentResolutionIndex != pendingResolutionIndex) {
        currentResolutionIndex = pendingResolutionIndex;

        // Atualizar resolução
        int width = availableResolutions[currentResolutionIndex].width;
        int height = availableResolutions[currentResolutionIndex].height;
        SetWindowSize(width, height);
    }

    // Atualizar modo tela cheia
    if (fullscreen != pendingFullscreen) {
        fullscreen = pendingFullscreen;
        ToggleFullscreen();
    }

    // Feedback visual
    printf("Configurações aplicadas com sucesso!\n");
}

void updateSettings(void) {
    // Atualização de efeitos animados pode ser colocada aqui se necessário
}