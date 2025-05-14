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

// Opções de dificuldade e velocidade de animação
static const char* difficultyOptions[] = {"FÁCIL", "NORMAL", "DIFÍCIL"};
static const char* animSpeedOptions[] = {"LENTA", "NORMAL", "RÁPIDA"};

// Animações
static float settingsTimer = 0.0f;
static float bgScroll = 0.0f;

// Função para desenhar a tela de configurações
void drawSettings(void) {
    // Atualizar temporizador
    settingsTimer += GetFrameTime();
    bgScroll += GetFrameTime() * 30.0f;
    if (bgScroll > 40.0f) bgScroll -= 40.0f;

    // Desenhar fundo estilo Pokémon
    // Fundo azul gradiente
    for (int i = 0; i < GetScreenHeight(); i++) {
        float factor = (float)i / GetScreenHeight();
        Color lineColor = (Color){
            (unsigned char)(60 * (1.0f - factor) + 30 * factor),
            (unsigned char)(120 * (1.0f - factor) + 80 * factor),
            (unsigned char)(180 * (1.0f - factor) + 140 * factor),
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

    // Título principal
    const char* title = "CONFIGURAÇÕES";
    int titleFontSize = 40;

    // Sombra do título
    DrawText(title,
            GetScreenWidth()/2 - MeasureText(title, titleFontSize)/2 + 2,
            40 + 2,
            titleFontSize,
            (Color){0, 0, 0, 120});

    // Título
    DrawText(title,
            GetScreenWidth()/2 - MeasureText(title, titleFontSize)/2,
            40,
            titleFontSize,
            WHITE);

    // Área principal de configurações
    int settingsWidth = 800;
    int settingsHeight = GetScreenHeight() - 150;
    Rectangle settingsArea = {
        GetScreenWidth()/2 - settingsWidth/2,
        100,
        settingsWidth,
        settingsHeight
    };

    // Painel de configurações
    DrawRectangleRounded(settingsArea, 0.05f, 8, (Color){30, 60, 90, 230});
    DrawRectangleRoundedLines(settingsArea, 0.05f, 8, WHITE);

    // Desenhar abas de configuração
    const char* tabs[] = { "ÁUDIO", "VÍDEO", "CONTROLES", "JOGO" };
    static int activeTab = 0;

    int tabWidth = settingsWidth / 4;
    int tabHeight = 50;

    for (int i = 0; i < 4; i++) {
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
                int yPos = contentArea.y;
                int itemHeight = 80;

                // Volume da música
                DrawText("Volume da Música", contentArea.x, yPos, 24, WHITE);

                Rectangle musicSlider = {
                    contentArea.x + 50,
                    yPos + 40,
                    contentArea.width - 150,
                    30
                };

                // Barra de fundo
                DrawRectangleRec(musicSlider, (Color){100, 100, 100, 150});

                // Barra de valor
                DrawRectangleRec(
                    (Rectangle){
                        musicSlider.x,
                        musicSlider.y,
                        musicSlider.width * pendingMusicVolume,
                        musicSlider.height
                    },
                    (Color){0, 150, 220, 255}
                );

                // Manopla
                float handleX = musicSlider.x + musicSlider.width * pendingMusicVolume;
                DrawRectangleRec(
                    (Rectangle){
                        handleX - 5,
                        musicSlider.y - 5,
                        10,
                        musicSlider.height + 10
                    },
                    WHITE
                );

                // Texto do valor
                char musicText[16];
                sprintf(musicText, "%.0f%%", pendingMusicVolume * 100);
                DrawText(musicText,
                        musicSlider.x + musicSlider.width + 20,
                        musicSlider.y + 5,
                        20,
                        WHITE);

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

                yPos += itemHeight + 20;

                // Volume dos efeitos
                DrawText("Volume dos Efeitos", contentArea.x, yPos, 24, WHITE);

                Rectangle soundSlider = {
                    contentArea.x + 50,
                    yPos + 40,
                    contentArea.width - 150,
                    30
                };

                // Barra de fundo
                DrawRectangleRec(soundSlider, (Color){100, 100, 100, 150});

                // Barra de valor
                DrawRectangleRec(
                    (Rectangle){
                        soundSlider.x,
                        soundSlider.y,
                        soundSlider.width * pendingSoundVolume,
                        soundSlider.height
                    },
                    (Color){0, 150, 220, 255}
                );

                // Manopla
                handleX = soundSlider.x + soundSlider.width * pendingSoundVolume;
                DrawRectangleRec(
                    (Rectangle){
                        handleX - 5,
                        soundSlider.y - 5,
                        10,
                        soundSlider.height + 10
                    },
                    WHITE
                );

                // Texto do valor
                char soundText[16];
                sprintf(soundText, "%.0f%%", pendingSoundVolume * 100);
                DrawText(soundText,
                        soundSlider.x + soundSlider.width + 20,
                        soundSlider.y + 5,
                        20,
                        WHITE);

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
            }
            break;

        case 1: // VÍDEO
            {
                int yPos = contentArea.y;
                int itemHeight = 70;

                // Tela cheia
                DrawText("Modo Tela Cheia", contentArea.x, yPos, 24, WHITE);

                Rectangle fullscreenToggle = {
                    contentArea.x + 50,
                    yPos + 35,
                    80,
                    30
                };

                // Desenhar toggle
                DrawRectangleRec(fullscreenToggle, (Color){100, 100, 100, 150});

                // Desenhar indicador
                DrawRectangleRec(
                    (Rectangle){
                        pendingFullscreen ?
                            fullscreenToggle.x + fullscreenToggle.width - 30 :
                            fullscreenToggle.x,
                        fullscreenToggle.y,
                        30,
                        fullscreenToggle.height
                    },
                    pendingFullscreen ? (Color){0, 200, 100, 255} : (Color){150, 150, 150, 255}
                );

                // Texto do estado
                DrawText(pendingFullscreen ? "ATIVADO" : "DESATIVADO",
                        fullscreenToggle.x + 100,
                        fullscreenToggle.y + 5,
                        20,
                        WHITE);

                // Interação com o toggle
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    if (CheckCollisionPointRec(GetMousePosition(), fullscreenToggle)) {
                        pendingFullscreen = !pendingFullscreen;
                        PlaySound(selectSound);
                        hasUnsavedChanges = true;
                    }
                }

                yPos += itemHeight + 20;

                // Resolução
                DrawText("Resolução", contentArea.x, yPos, 24, WHITE);

                // Botão anterior
                Rectangle prevResButton = {
                    contentArea.x + 50,
                    yPos + 35,
                    40,
                    40
                };

                // Botão próximo
                Rectangle nextResButton = {
                    contentArea.x + 50 + 40 + 200 + 20,
                    yPos + 35,
                    40,
                    40
                };

                // Resolução atual
                Rectangle currentResRect = {
                    contentArea.x + 50 + 40 + 10,
                    yPos + 35,
                    200,
                    40
                };

                // Desenhar botões e display
                DrawRectangleRounded(prevResButton, 0.5f, 6, (Color){0, 102, 204, 255});
                DrawRectangleRounded(nextResButton, 0.5f, 6, (Color){0, 102, 204, 255});
                DrawRectangleRounded(currentResRect, 0.2f, 6, (Color){60, 60, 60, 200});

                DrawText("<",
                        prevResButton.x + prevResButton.width/2 - 5,
                        prevResButton.y + prevResButton.height/2 - 12,
                        24,
                        WHITE);

                DrawText(">",
                        nextResButton.x + nextResButton.width/2 - 5,
                        nextResButton.y + nextResButton.height/2 - 12,
                        24,
                        WHITE);

                DrawText(availableResolutions[pendingResolutionIndex].description,
                        currentResRect.x + currentResRect.width/2 -
                        MeasureText(availableResolutions[pendingResolutionIndex].description, 20)/2,
                        currentResRect.y + currentResRect.height/2 - 10,
                        20,
                        WHITE);

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
            }
            break;

        case 2: // CONTROLES
            {
                int yPos = contentArea.y;

                DrawText("Controles do Jogo", contentArea.x, yPos, 24, WHITE);

                yPos += 50;

                // Lista de controles
                const char* controlLabels[] = {
                    "Confirmar/Selecionar:",
                    "Cancelar/Voltar:",
                    "Navegar no Menu:",
                    "Navegar em Batalha:",
                    "Alternar Tela Cheia:",
                    "Pausar o Jogo:"
                };

                const char* controlBindings[] = {
                    "Enter, Clique Esquerdo",
                    "Esc, Backspace",
                    "Setas, Mouse",
                    "Setas, 1-4, Mouse",
                    "F11",
                    "P, Esc"
                };

                int controlCount = 6;

                for (int i = 0; i < controlCount; i++) {
                    DrawText(controlLabels[i],
                            contentArea.x + 20,
                            yPos + i * 40,
                            20,
                            WHITE);

                    DrawText(controlBindings[i],
                            contentArea.x + 250,
                            yPos + i * 40,
                            20,
                            (Color){200, 255, 200, 255});
                }

                yPos += controlCount * 40 + 30;

                // Dica adicional
                DrawText("Dica: Use o mouse para a maioria das interações",
                        contentArea.x + 20,
                        yPos,
                        18,
                        (Color){180, 180, 255, 255});
            }
            break;

        case 3: // JOGO
            {
                int yPos = contentArea.y;

                // Dificuldade
                DrawText("Dificuldade do Bot", contentArea.x, yPos, 24, WHITE);

                int buttonWidth = 140;
                int buttonHeight = 40;
                int spacing = 20;

                for (int i = 0; i < 3; i++) {
                    Rectangle diffBtn = {
                        contentArea.x + 50 + i * (buttonWidth + spacing),
                        yPos + 40,
                        buttonWidth,
                        buttonHeight
                    };

                    Color btnColor;
                    if (pendingDifficultyIndex == i) {
                        btnColor = i == 0 ? (Color){0, 180, 100, 255} :
                                   i == 1 ? (Color){0, 120, 200, 255} :
                                            (Color){200, 60, 60, 255};
                    } else {
                        btnColor = (Color){80, 80, 80, 200};
                    }

                    // Desenhar botão
                    DrawRectangleRounded(diffBtn, 0.3f, 6, btnColor);
                    DrawRectangleRoundedLines(diffBtn, 0.3f, 6, WHITE);

                    // Texto
                    DrawText(difficultyOptions[i],
                            diffBtn.x + diffBtn.width/2 - MeasureText(difficultyOptions[i], 20)/2,
                            diffBtn.y + diffBtn.height/2 - 10,
                            20,
                            WHITE);

                    // Interação
                    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
                        CheckCollisionPointRec(GetMousePosition(), diffBtn)) {
                        pendingDifficultyIndex = i;
                        PlaySound(selectSound);
                        hasUnsavedChanges = true;
                    }
                }

                yPos += 100;

                // Velocidade de animação
                DrawText("Velocidade de Animação", contentArea.x, yPos, 24, WHITE);

                for (int i = 0; i < 3; i++) {
                    Rectangle speedBtn = {
                        contentArea.x + 50 + i * (buttonWidth + spacing),
                        yPos + 40,
                        buttonWidth,
                        buttonHeight
                    };

                    Color btnColor = pendingAnimSpeedIndex == i ?
                                 (Color){0, 150, 200, 255} : (Color){80, 80, 80, 200};

                    // Desenhar botão
                    DrawRectangleRounded(speedBtn, 0.3f, 6, btnColor);
                    DrawRectangleRoundedLines(speedBtn, 0.3f, 6, WHITE);

                    // Texto
                    DrawText(animSpeedOptions[i],
                            speedBtn.x + speedBtn.width/2 - MeasureText(animSpeedOptions[i], 20)/2,
                            speedBtn.y + speedBtn.height/2 - 10,
                            20,
                            WHITE);

                    // Interação
                    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
                        CheckCollisionPointRec(GetMousePosition(), speedBtn)) {
                        pendingAnimSpeedIndex = i;
                        PlaySound(selectSound);
                        hasUnsavedChanges = true;
                    }
                }

                // Verificar conexão com API - Botão de teste
                yPos += 100;
                DrawText("Integração com IA", contentArea.x, yPos, 24, WHITE);

                Rectangle testApiBtn = {
                    contentArea.x + 50,
                    yPos + 40,
                    200,
                    40
                };

                DrawRectangleRounded(testApiBtn, 0.3f, 6, (Color){100, 50, 200, 255});
                DrawRectangleRoundedLines(testApiBtn, 0.3f, 6, WHITE);

                DrawText("Testar Conexão",
                        testApiBtn.x + testApiBtn.width/2 - MeasureText("Testar Conexão", 20)/2,
                        testApiBtn.y + testApiBtn.height/2 - 10,
                        20,
                        WHITE);

                // Verificar atual do IA
                if (initialized) {
                    DrawText("Status: Conectado",
                            testApiBtn.x + testApiBtn.width + 20,
                            testApiBtn.y + 10,
                            20,
                            GREEN);
                } else {
                    DrawText("Status: Desconectado",
                            testApiBtn.x + testApiBtn.width + 20,
                            testApiBtn.y + 10,
                            20,
                            RED);
                }

                // Interação com botão de teste
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
                    CheckCollisionPointRec(GetMousePosition(), testApiBtn)) {
                    PlaySound(selectSound);
                    testAIConnection();
                }
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

        DrawRectangleRounded(applyBtn, 0.3f, 6, (Color){0, 180, 90, 255});
        DrawRectangleRoundedLines(applyBtn, 0.3f, 6, WHITE);

        DrawText("APLICAR",
                applyBtn.x + applyBtn.width/2 - MeasureText("APLICAR", 24)/2,
                applyBtn.y + applyBtn.height/2 - 12,
                24,
                WHITE);

        // Interação
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
            CheckCollisionPointRec(GetMousePosition(), applyBtn)) {
            PlaySound(selectSound);
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

    DrawRectangleRounded(backBtn, 0.3f, 6, (Color){100, 100, 100, 255});
    DrawRectangleRoundedLines(backBtn, 0.3f, 6, WHITE);

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
        DrawText("* Mudanças não salvas",
                settingsArea.x + 20,
                bottomY + 15,
                18,
                (Color){255, 200, 100, 255});
    }

    // Desenhar partículas decorativas
    for (int i = 0; i < 10; i++) {
        float time = settingsTimer + i * 0.5f;
        float x = sinf(time * 0.2f + i) * GetScreenWidth() * 0.4f + GetScreenWidth() * 0.5f;
        float y = cosf(time * 0.3f + i) * GetScreenHeight() * 0.3f + GetScreenHeight() * 0.5f;

        float size = 3 + sinf(time) * 2;

        Color particleColor = (Color){
            (unsigned char)(100 + sinf(time * 0.3f) * 70),
            (unsigned char)(150 + sinf(time * 0.5f) * 50),
            (unsigned char)(200 + sinf(time * 0.7f) * 55),
            150
        };

        DrawCircle(x, y, size, particleColor);
    }
}

void updateSettings(void) {
    // Atualização da lógica de configurações (nada adicional aqui, tudo já é feito dentro de drawSettings)
}