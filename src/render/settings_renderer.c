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


static bool fullscreen = false;
float musicVolume = 0.7f;
float soundVolume = 0.8f;
static int currentResolutionIndex = 0;
static int pendingResolutionIndex = 0;
static bool hasUnsavedChanges = false;
static float pendingMusicVolume = 0.7f;
static float pendingSoundVolume = 0.8f;
static bool pendingFullscreen = false;
static int pendingDifficultyIndex = 1;
static int pendingAnimSpeedIndex = 1;


static Resolution availableResolutions[] = {
    { 1920, 1080, "1920x1080" },
    { 1600, 900,  "1600x900" },
    { 1366, 768,  "1366x768" },
    { 1280, 720,  "1280x720" },
    { 1024, 768,  "1024x768" }
};
static int numResolutions = 5;

// Função para desenhar a tela de configurações
void drawSettings(void) {
    // Calcular escala baseada em 1920x1080
    float scaleX = GetScreenWidth() / 1920.0f;
    float scaleY = GetScreenHeight() / 1080.0f;
    float scale = fmin(scaleX, scaleY);
    
    // Cores do tema (mantendo consistência com o menu principal)
    Color primaryColor = (Color){ 255, 51, 51, 255 };      
    Color secondaryColor = (Color){ 51, 153, 255, 255 };   
    Color bgColor = (Color){ 245, 245, 245, 255 };         
    Color accentColor = (Color){ 255, 204, 0, 255 };       
    
    // Fundo gradiente similar ao menu principal
    static float bgTimer = 0.0f;
    bgTimer += GetFrameTime() * 0.5f;
    
    // Desenhar fundo gradiente dinâmico
    for (int i = 0; i < GetScreenHeight(); i += 5) {
        float factor = (float)i / GetScreenHeight();
        
        Color lineColor = (Color){
            (unsigned char)(bgColor.r * (1.0f - factor) + secondaryColor.r * factor),
            (unsigned char)(bgColor.g * (1.0f - factor) + secondaryColor.g * factor),
            (unsigned char)(bgColor.b * (1.0f - factor) + secondaryColor.b * factor),
            255
        };
        
        DrawRectangle(0, i, GetScreenWidth(), 5, lineColor);
    }
    
    // Título principal
    const char* title = "Configurações";
    int titleFontSize = (int)(90 * scale);
    
    // Título - Sombra
    Vector2 titleShadowPos = { 
        GetScreenWidth()/2 - MeasureText(title, titleFontSize)/2 + (int)(3 * scale), 
        (int)(83 * scale) 
    };
    DrawText(title, titleShadowPos.x, titleShadowPos.y, titleFontSize, (Color){ 40, 40, 40, 200 });
    
    // Título - Contorno
    Vector2 titleOutlinePos = { 
        GetScreenWidth()/2 - MeasureText(title, titleFontSize)/2, 
        (int)(80 * scale) 
    };
    
    int outlineOffset = (int)(3 * scale);
    DrawText(title, titleOutlinePos.x - outlineOffset, titleOutlinePos.y - outlineOffset, titleFontSize, BLACK);
    DrawText(title, titleOutlinePos.x + outlineOffset, titleOutlinePos.y - outlineOffset, titleFontSize, BLACK);
    DrawText(title, titleOutlinePos.x - outlineOffset, titleOutlinePos.y + outlineOffset, titleFontSize, BLACK);
    DrawText(title, titleOutlinePos.x + outlineOffset, titleOutlinePos.y + outlineOffset, titleFontSize, BLACK);
    
    // Título - Texto principal
    DrawText(title, titleOutlinePos.x, titleOutlinePos.y, titleFontSize, primaryColor);
    
    // Área principal de configurações
    int settingsWidth = (int)(900 * scale);
    int settingsHeight = (int)(700 * scale);
    Rectangle settingsArea = {
        GetScreenWidth()/2 - settingsWidth/2,
        (int)(220 * scale),
        settingsWidth,
        settingsHeight
    };
    
    // Painel de configurações
    DrawRectangleRounded(settingsArea, 0.05f, 8, ColorAlpha(WHITE, 0.95f));
    DrawRectangleRoundedLines(settingsArea, 0.05f, 8, DARKGRAY);
    
    // Desenhar categorias de configuração em abas
    const char* tabs[] = { "ÁUDIO", "VÍDEO", "CONTROLES", "JOGO" };
    static int activeTab = 0;
    
    int tabWidth = settingsWidth / 4;
    int tabHeight = (int)(60 * scale);
    
    for (int i = 0; i < 4; i++) {
        Rectangle tabRect = {
            settingsArea.x + i * tabWidth,
            settingsArea.y,
            tabWidth,
            tabHeight
        };
        
        Color tabColor = (activeTab == i) ? primaryColor : LIGHTGRAY;
        if (activeTab == i) {
            DrawRectangleRounded(tabRect, 0.2f, 8, tabColor);
        } else {
            DrawRectangleRec(tabRect, tabColor);
        }
        
        // Hover effect
        if (CheckCollisionPointRec(GetMousePosition(), tabRect) && activeTab != i) {
            DrawRectangleRec(tabRect, ColorAlpha(primaryColor, 0.3f));
            
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                activeTab = i;
                PlaySound(selectSound);
            }
        }
        
        int tabFontSize = (int)(30 * scale);
        Color textColor = (activeTab == i) ? WHITE : DARKGRAY;
        DrawText(tabs[i], 
                tabRect.x + tabRect.width/2 - MeasureText(tabs[i], tabFontSize)/2,
                tabRect.y + tabRect.height/2 - tabFontSize/2,
                tabFontSize,
                textColor);
    }
    
    // Conteúdo da aba ativa
    Rectangle contentArea = {
        settingsArea.x + (int)(50 * scale),
        settingsArea.y + tabHeight + (int)(50 * scale),
        settingsArea.width - (int)(100 * scale),
        settingsArea.height - tabHeight - (int)(150 * scale)
    };
    
    switch (activeTab) {
        case 0: // ÁUDIO
            {
                int yOffset = contentArea.y;
                int itemSpacing = (int)(100 * scale);
                
                // Volume da Música
                int labelFontSize = (int)(32 * scale);
                DrawText("Volume da Música", contentArea.x, yOffset, labelFontSize, BLACK);
                
                // Slider para volume da música
                Rectangle musicSlider = { 
                    contentArea.x, 
                    yOffset + (int)(40 * scale), 
                    contentArea.width - (int)(100 * scale), // Espaço para o texto da porcentagem
                    (int)(40 * scale) 
                };
                
                // Fundo do slider
                DrawRectangleRounded(musicSlider, 0.5f, 8, LIGHTGRAY);
                
                // Preenchimento do slider
                Rectangle musicFill = {
                    musicSlider.x,
                    musicSlider.y,
                    musicSlider.width * pendingMusicVolume,
                    musicSlider.height
                };
                DrawRectangleRounded(musicFill, 0.5f, 8, secondaryColor);
                
                // Controle deslizante (thumb)
                int thumbRadius = (int)(20 * scale);
                float thumbX = musicSlider.x + musicSlider.width * pendingMusicVolume;
                float thumbY = musicSlider.y + musicSlider.height/2;
                
                DrawCircle(thumbX, thumbY, thumbRadius, WHITE);
                DrawCircleLines(thumbX, thumbY, thumbRadius, DARKGRAY);
                
                // Valor atual
                char volumeText[10];
                sprintf(volumeText, "%.0f%%", pendingMusicVolume * 100);
                DrawText(volumeText, 
                        musicSlider.x + musicSlider.width + (int)(20 * scale), 
                        musicSlider.y + (int)(5 * scale), 
                        (int)(28 * scale), 
                        BLACK);
                
                // Interação com o slider de música
                if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                    Vector2 mousePos = GetMousePosition();
                    Rectangle expandedSlider = {
                        musicSlider.x - thumbRadius,
                        musicSlider.y - thumbRadius,
                        musicSlider.width + thumbRadius * 2,
                        musicSlider.height + thumbRadius * 2
                    };
                    
                    if (CheckCollisionPointRec(mousePos, expandedSlider)) {
                        pendingMusicVolume = (mousePos.x - musicSlider.x) / musicSlider.width;
                        pendingMusicVolume = pendingMusicVolume < 0 ? 0 : (pendingMusicVolume > 1 ? 1 : pendingMusicVolume);
                        hasUnsavedChanges = true;
                    }
                }
                
                yOffset += itemSpacing;
                
                // Volume dos Efeitos
                DrawText("Volume dos Efeitos", contentArea.x, yOffset, labelFontSize, BLACK);
                
                // Slider para volume dos efeitos
                Rectangle soundSlider = { 
                    contentArea.x, 
                    yOffset + (int)(40 * scale), 
                    contentArea.width - (int)(100 * scale),
                    (int)(40 * scale) 
                };
                
                DrawRectangleRounded(soundSlider, 0.5f, 8, LIGHTGRAY);
                
                Rectangle soundFill = {
                    soundSlider.x,
                    soundSlider.y,
                    soundSlider.width * pendingSoundVolume,
                    soundSlider.height
                };
                DrawRectangleRounded(soundFill, 0.5f, 8, secondaryColor);
                
                // Controle deslizante
                thumbX = soundSlider.x + soundSlider.width * pendingSoundVolume;
                thumbY = soundSlider.y + soundSlider.height/2;
                
                DrawCircle(thumbX, thumbY, thumbRadius, WHITE);
                DrawCircleLines(thumbX, thumbY, thumbRadius, DARKGRAY);
                
                // Valor atual
                sprintf(volumeText, "%.0f%%", pendingSoundVolume * 100);
                DrawText(volumeText, 
                        soundSlider.x + soundSlider.width + (int)(20 * scale), 
                        soundSlider.y + (int)(5 * scale), 
                        (int)(28 * scale), 
                        BLACK);
                
                // Interação com o slider de som
                if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                    Vector2 mousePos = GetMousePosition();
                    Rectangle expandedSlider = {
                        soundSlider.x - thumbRadius,
                        soundSlider.y - thumbRadius,
                        soundSlider.width + thumbRadius * 2,
                        soundSlider.height + thumbRadius * 2
                    };
                    
                    if (CheckCollisionPointRec(mousePos, expandedSlider)) {
                        pendingSoundVolume = (mousePos.x - soundSlider.x) / soundSlider.width;
                        pendingSoundVolume = pendingSoundVolume < 0 ? 0 : (pendingSoundVolume > 1 ? 1 : pendingSoundVolume);
                        hasUnsavedChanges = true;
                    }
                }
                
                yOffset += itemSpacing;
                
                // Opção Mute
                static bool muteAll = false;
                Rectangle muteButton = {
                    contentArea.x,
                    yOffset,
                    (int)(300 * scale),
                    (int)(60 * scale)
                };
                
                Color muteColor = muteAll ? primaryColor : LIGHTGRAY;
                DrawRectangleRounded(muteButton, 0.3f, 8, muteColor);
                DrawRectangleRoundedLines(muteButton, 0.3f, 8, DARKGRAY);
                
                const char* muteText = muteAll ? "SOM DESLIGADO" : "SOM LIGADO";
                DrawText(muteText,
                        muteButton.x + muteButton.width/2 - MeasureText(muteText, (int)(28 * scale))/2,
                        muteButton.y + muteButton.height/2 - (int)(14 * scale),
                        (int)(28 * scale),
                        muteAll ? WHITE : BLACK);
                
                if (CheckCollisionPointRec(GetMousePosition(), muteButton) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    muteAll = !muteAll;
                    PlaySound(selectSound);
                    
                    if (muteAll) {
                        SetMasterVolume(0.0f);
                    } else {
                        SetMasterVolume(1.0f);
                    }
                }
            }
            break;
            
        case 1: // VÍDEO
            {
                int yOffset = contentArea.y;
                int itemSpacing = (int)(100 * scale);
                int labelFontSize = (int)(32 * scale);
                
                // Tela cheia
                DrawText("Modo Tela Cheia", contentArea.x, yOffset, labelFontSize, BLACK);
                
                Rectangle fullscreenButton = {
                    contentArea.x,
                    yOffset + (int)(40 * scale),
                    (int)(300 * scale),
                    (int)(60 * scale)
                };
                
                Color fullscreenColor = pendingFullscreen ? primaryColor : LIGHTGRAY;
                DrawRectangleRounded(fullscreenButton, 0.3f, 8, fullscreenColor);
                DrawRectangleRoundedLines(fullscreenButton, 0.3f, 8, DARKGRAY);
                
                const char* fullscreenText = pendingFullscreen ? "ATIVADO" : "DESATIVADO";
                DrawText(fullscreenText,
                        fullscreenButton.x + fullscreenButton.width/2 - MeasureText(fullscreenText, (int)(28 * scale))/2,
                        fullscreenButton.y + fullscreenButton.height/2 - (int)(14 * scale),
                        (int)(28 * scale),
                        pendingFullscreen ? WHITE : BLACK);
                
                if (CheckCollisionPointRec(GetMousePosition(), fullscreenButton) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    pendingFullscreen = !pendingFullscreen;
                    PlaySound(selectSound);
                    hasUnsavedChanges = true;
                }
                
                yOffset += itemSpacing;
                
                // Resolução
                DrawText("Resolução", contentArea.x, yOffset, labelFontSize, BLACK);
                
                // Botão anterior
                Rectangle prevButton = {
                    contentArea.x,
                    yOffset + (int)(40 * scale),
                    (int)(60 * scale),
                    (int)(60 * scale)
                };
                
                DrawRectangleRounded(prevButton, 0.3f, 8, primaryColor);
                DrawText("<",
                        prevButton.x + prevButton.width/2 - MeasureText("<", (int)(32 * scale))/2,
                        prevButton.y + prevButton.height/2 - (int)(16 * scale),
                        (int)(32 * scale),
                        WHITE);
                
                // Display da resolução atual
                Rectangle resolutionDisplay = {
                    prevButton.x + prevButton.width + (int)(20 * scale),
                    yOffset + (int)(40 * scale),
                    (int)(200 * scale),
                    (int)(60 * scale)
                };
                
                DrawRectangleRounded(resolutionDisplay, 0.3f, 8, LIGHTGRAY);
                
                // Mostrar resolução atual e pendente se diferentes
                if (pendingResolutionIndex != currentResolutionIndex) {
                    DrawText(availableResolutions[pendingResolutionIndex].description,
                            resolutionDisplay.x + resolutionDisplay.width/2 - MeasureText(availableResolutions[pendingResolutionIndex].description, (int)(24 * scale))/2,
                            resolutionDisplay.y + resolutionDisplay.height/2 - (int)(20 * scale),
                            (int)(24 * scale),
                            secondaryColor);
                    
                    // Mostrar resolução atual abaixo em menor tamanho
                    char currentText[32];
                    sprintf(currentText, "(atual: %s)", availableResolutions[currentResolutionIndex].description);
                    DrawText(currentText,
                            resolutionDisplay.x + resolutionDisplay.width/2 - MeasureText(currentText, (int)(16 * scale))/2,
                            resolutionDisplay.y + resolutionDisplay.height/2 + (int)(5 * scale),
                            (int)(16 * scale),
                            GRAY);
                } else {
                    DrawText(availableResolutions[pendingResolutionIndex].description,
                            resolutionDisplay.x + resolutionDisplay.width/2 - MeasureText(availableResolutions[pendingResolutionIndex].description, (int)(28 * scale))/2,
                            resolutionDisplay.y + resolutionDisplay.height/2 - (int)(14 * scale),
                            (int)(28 * scale),
                            BLACK);
                }
                
                // Botão próximo
                Rectangle nextButton = {
                    resolutionDisplay.x + resolutionDisplay.width + (int)(20 * scale),
                    yOffset + (int)(40 * scale),
                    (int)(60 * scale),
                    (int)(60 * scale)
                };
                
                DrawRectangleRounded(nextButton, 0.3f, 8, primaryColor);
                DrawText(">",
                        nextButton.x + nextButton.width/2 - MeasureText(">", (int)(32 * scale))/2,
                        nextButton.y + nextButton.height/2 - (int)(16 * scale),
                        (int)(32 * scale),
                        WHITE);
                
                // Interação com botões de resolução
                if (CheckCollisionPointRec(GetMousePosition(), prevButton) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    pendingResolutionIndex = (pendingResolutionIndex - 1 + numResolutions) % numResolutions;
                    PlaySound(selectSound);
                    hasUnsavedChanges = true;
                }
                
                if (CheckCollisionPointRec(GetMousePosition(), nextButton) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    pendingResolutionIndex = (pendingResolutionIndex + 1) % numResolutions;
                    PlaySound(selectSound);
                    hasUnsavedChanges = true;
                }
            }
            break;
            
        case 2: // CONTROLES
            {
                int yOffset = contentArea.y;
                int labelFontSize = (int)(32 * scale);
                
                DrawText("Controles de Batalha", contentArea.x, yOffset, labelFontSize, BLACK);
                
                yOffset += (int)(60 * scale);
                
                // Lista de controles
                const char* controls[] = {
                    "Selecionar Ação: Clique do Mouse / Enter",
                    "Navegar Menu: Setas / Mouse",
                    "Cancelar: ESC / Botão Voltar",
                    "Confirmar: Espaço / Enter / Clique",
                    "Alternar Tela Cheia: F11"
                };
                
                for (int i = 0; i < 5; i++) {
                    DrawText(controls[i], 
                            contentArea.x + (int)(20 * scale), 
                            yOffset + i * (int)(50 * scale), 
                            (int)(26 * scale), 
                            DARKGRAY);
                }
            }
            break;
            
        case 3: // JOGO
            {
                int yOffset = contentArea.y;
                int itemSpacing = (int)(100 * scale);
                int labelFontSize = (int)(32 * scale);
                
                // Dificuldade
                DrawText("Dificuldade", contentArea.x, yOffset, labelFontSize, BLACK);
                
                const char* difficulties[] = { "FÁCIL", "NORMAL", "DIFÍCIL" };
                Color difficultyColors[] = { GREEN, BLUE, RED };
                
                for (int i = 0; i < 3; i++) {
                    Rectangle diffButton = {
                        contentArea.x + i * (int)(220 * scale),
                        yOffset + (int)(40 * scale),
                        (int)(200 * scale),
                        (int)(60 * scale)
                    };
                    
                    Color buttonColor = (pendingDifficultyIndex == i) ? difficultyColors[i] : LIGHTGRAY;
                    DrawRectangleRounded(diffButton, 0.3f, 8, buttonColor);
                    DrawRectangleRoundedLines(diffButton, 0.3f, 8, DARKGRAY);
                    
                    DrawText(difficulties[i],
                            diffButton.x + diffButton.width/2 - MeasureText(difficulties[i], (int)(28 * scale))/2,
                            diffButton.y + diffButton.height/2 - (int)(14 * scale),
                            (int)(28 * scale),
                            (pendingDifficultyIndex == i) ? WHITE : BLACK);
                    
                    if (CheckCollisionPointRec(GetMousePosition(), diffButton) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        pendingDifficultyIndex = i;
                        PlaySound(selectSound);
                        hasUnsavedChanges = true;
                    }
                }
                
                yOffset += itemSpacing;
                
                // Velocidade de Animação
                DrawText("Velocidade de Animação", contentArea.x, yOffset, labelFontSize, BLACK);
                
                const char* animSpeeds[] = { "LENTA", "NORMAL", "RÁPIDA" };
                
                for (int i = 0; i < 3; i++) {
                    Rectangle speedButton = {
                        contentArea.x + i * (int)(220 * scale),
                        yOffset + (int)(40 * scale),
                        (int)(200 * scale),
                        (int)(60 * scale)
                    };
                    
                    Color buttonColor = (pendingAnimSpeedIndex == i) ? secondaryColor : LIGHTGRAY;
                    DrawRectangleRounded(speedButton, 0.3f, 8, buttonColor);
                    DrawRectangleRoundedLines(speedButton, 0.3f, 8, DARKGRAY);
                    
                    DrawText(animSpeeds[i],
                            speedButton.x + speedButton.width/2 - MeasureText(animSpeeds[i], (int)(28 * scale))/2,
                            speedButton.y + speedButton.height/2 - (int)(14 * scale),
                            (int)(28 * scale),
                            (pendingAnimSpeedIndex == i) ? WHITE : BLACK);
                    
                    if (CheckCollisionPointRec(GetMousePosition(), speedButton) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        pendingAnimSpeedIndex = i;
                        PlaySound(selectSound);
                        hasUnsavedChanges = true;
                    }
                }
            }
            break;
    }
    
    // Botões de ação na parte inferior
    int buttonWidth = (int)(220 * scale);
    int buttonHeight = (int)(60 * scale);
    int buttonSpacing = (int)(20 * scale);
    int bottomY = settingsArea.y + settingsArea.height - (int)(80 * scale);
    
    // Botão Aplicar (só aparece se houver mudanças)
    if (hasUnsavedChanges) {
        Rectangle applyButton = {
            settingsArea.x + settingsArea.width/2 - buttonWidth - buttonSpacing/2,
            bottomY,
            buttonWidth,
            buttonHeight
        };
        
        Color applyButtonColor = CheckCollisionPointRec(GetMousePosition(), applyButton) ? 
                               ColorAlpha(GREEN, 0.8f) : GREEN;
        
        DrawRectangleRounded(applyButton, 0.3f, 8, applyButtonColor);
        DrawRectangleRoundedLines(applyButton, 0.3f, 8, DARKGRAY);
        
        const char* applyText = "APLICAR";
        int applyFontSize = (int)(32 * scale);
        DrawText(applyText,
                applyButton.x + applyButton.width/2 - MeasureText(applyText, applyFontSize)/2,
                applyButton.y + applyButton.height/2 - applyFontSize/2,
                applyFontSize,
                WHITE);
        
        if (CheckCollisionPointRec(GetMousePosition(), applyButton) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            PlaySound(selectSound);
            applySettings();
        }
    }
    
    // Botão Voltar/Cancelar
    Rectangle backButton = {
        hasUnsavedChanges ? 
            settingsArea.x + settingsArea.width/2 + buttonSpacing/2 :
            settingsArea.x + settingsArea.width/2 - buttonWidth/2,
        bottomY,
        buttonWidth,
        buttonHeight
    };
    
    Color backButtonColor = CheckCollisionPointRec(GetMousePosition(), backButton) ? 
                           ColorAlpha(primaryColor, 0.8f) : primaryColor;
    
    DrawRectangleRounded(backButton, 0.3f, 8, backButtonColor);
    DrawRectangleRoundedLines(backButton, 0.3f, 8, DARKGRAY);
    
    const char* backText = hasUnsavedChanges ? "CANCELAR" : "VOLTAR";
    int backFontSize = (int)(32 * scale);
    DrawText(backText,
            backButton.x + backButton.width/2 - MeasureText(backText, backFontSize)/2,
            backButton.y + backButton.height/2 - backFontSize/2,
            backFontSize,
            WHITE);
    
            if (CheckCollisionPointRec(GetMousePosition(), backButton) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                PlaySound(selectSound);
                
                // Se houver mudanças não salvas, reverter para os valores atuais
                if (hasUnsavedChanges) {
                    pendingMusicVolume = musicVolume;
                    pendingSoundVolume = soundVolume;
                    pendingFullscreen = fullscreen;
                    pendingResolutionIndex = currentResolutionIndex;
                    pendingDifficultyIndex = 1;
                    pendingAnimSpeedIndex = 1;
                    hasUnsavedChanges = false;
                }
                
                currentScreen = MAIN_MENU;
            }
    
    // Indicador de mudanças não salvas
    if (hasUnsavedChanges) {
        const char* unsavedText = "* Mudanças não salvas";
        int unsavedFontSize = (int)(20 * scale);
        DrawText(unsavedText,
                settingsArea.x + (int)(20 * scale),
                bottomY + (int)(10 * scale),
                unsavedFontSize,
                ORANGE);
    }
    
    // Efeitos decorativos
    static float particleTimer = 0.0f;
    particleTimer += GetFrameTime();
    
    // Partículas flutuantes (similar ao menu principal)
    for (int i = 0; i < 15; i++) {
        float x = sinf(particleTimer * 0.3f + i * 0.4f) * GetScreenWidth() * 0.3f + GetScreenWidth() * 0.5f;
        float y = cosf(particleTimer * 0.2f + i * 0.5f) * (int)(120 * scale) + GetScreenHeight() * 0.15f;
        float size = sinf(particleTimer + i) * (int)(2 * scale) + (int)(3 * scale);
        
        DrawCircle(x, y, size, ColorAlpha(accentColor, 0.5f));
    }
}

// Funções auxiliares que precisam ser adicionadas:

void applySettings(void) {
    // Aplicar resolução se mudou
    if (pendingResolutionIndex != currentResolutionIndex) {
        SetWindowSize(availableResolutions[pendingResolutionIndex].width, 
                     availableResolutions[pendingResolutionIndex].height);
        
        // Centralizar a janela na tela
        int monitorWidth = GetMonitorWidth(GetCurrentMonitor());
        int monitorHeight = GetMonitorHeight(GetCurrentMonitor());
        int windowWidth = availableResolutions[pendingResolutionIndex].width;
        int windowHeight = availableResolutions[pendingResolutionIndex].height;
        
        SetWindowPosition((monitorWidth - windowWidth) / 2, 
                         (monitorHeight - windowHeight) / 2);
        
        currentResolutionIndex = pendingResolutionIndex;
    }
    
    // Aplicar modo tela cheia
    if (pendingFullscreen != fullscreen) {
        fullscreen = pendingFullscreen;
        ToggleFullscreen();
    }
    
    // Aplicar volumes
    musicVolume = pendingMusicVolume;
    soundVolume = pendingSoundVolume;
    SetMusicVolume(menuMusic, musicVolume);
    SetMusicVolume(battleMusic, musicVolume);
    SetSoundVolume(selectSound, soundVolume);
    SetSoundVolume(attackSound, soundVolume);
    SetSoundVolume(hitSound, soundVolume);
    SetSoundVolume(faintSound, soundVolume);
   
   hasUnsavedChanges = false;
}

void detectCurrentResolution(void) {
   int currentWidth = GetScreenWidth();
   int currentHeight = GetScreenHeight();
   
   for (int i = 0; i < numResolutions; i++) {
       if (availableResolutions[i].width == currentWidth && 
           availableResolutions[i].height == currentHeight) {
           currentResolutionIndex = i;
           pendingResolutionIndex = i;
           break;
       }
   }
}

void initializeSettings(void) {
   detectCurrentResolution();
   pendingMusicVolume = musicVolume;
   pendingSoundVolume = soundVolume;
   pendingFullscreen = fullscreen;
   pendingDifficultyIndex = 1; // Normal por padrão
   pendingAnimSpeedIndex = 1;  // Normal por padrão
   hasUnsavedChanges = false;
}
 
 void updateSettings(void) {
     // Atualização da lógica de configurações, se necessário
 }