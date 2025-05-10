#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#endif

#include "menu_renderer.h"
#include "hud_elements.h"
#include "resources.h"
#include "../game_state.h"
#include <math.h>
#include "screens.h"

static float pendingMusicVolume = 0.7f;
static float pendingSoundVolume = 0.8f;
static bool pendingFullscreen = false;
static int pendingDifficultyIndex = 1;
static int pendingAnimSpeedIndex = 1;

// Menu principal
void drawMainMenu(void) {
    // Atualizar música do menu
    UpdateMusicStream(menuMusic);
    
    // Calcular escala baseada em 1920x1080
    float scaleX = GetScreenWidth() / 1920.0f;
    float scaleY = GetScreenHeight() / 1080.0f;
    float scale = fmin(scaleX, scaleY);
    
    // Cores principais do tema
    Color primaryColor = (Color){ 255, 51, 51, 255 };      // Vermelho Fire Red
    Color secondaryColor = (Color){ 51, 153, 255, 255 };   // Azul complementar
    Color bgColor = (Color){ 245, 245, 245, 255 };         // Fundo claro
    Color accentColor = (Color){ 255, 204, 0, 255 };       // Amarelo Pikachu
    
    // Efeito de fundo animado
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
    
    // Título principal estilo Pokémon
    const char* title = "PokeBattle";
    int titleFontSize = (int)(110 * scale); // Aumentado proporcionalmente
    
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
    
    // Desenhar contorno
    int outlineOffset = (int)(3 * scale);
    DrawText(title, titleOutlinePos.x - outlineOffset, titleOutlinePos.y - outlineOffset, titleFontSize, BLACK);
    DrawText(title, titleOutlinePos.x + outlineOffset, titleOutlinePos.y - outlineOffset, titleFontSize, BLACK);
    DrawText(title, titleOutlinePos.x - outlineOffset, titleOutlinePos.y + outlineOffset, titleFontSize, BLACK);
    DrawText(title, titleOutlinePos.x + outlineOffset, titleOutlinePos.y + outlineOffset, titleFontSize, BLACK);
    
    // Título - Texto principal
    DrawText(title, titleOutlinePos.x, titleOutlinePos.y, titleFontSize, primaryColor);
    
    // Subtítulo
    const char* subtitle = "Uma aventura de monstros de batalha";
    int subtitleFontSize = (int)(32 * scale);
    Vector2 subtitlePos = { 
        GetScreenWidth()/2 - MeasureText(subtitle, subtitleFontSize)/2, 
        (int)(200 * scale) 
    };
    DrawText(subtitle, subtitlePos.x, subtitlePos.y, subtitleFontSize, DARKGRAY);
    
    // Desenhar PokeBall decorativa
    int pokeballSize = (int)(64 * scale); // Proporcional ao tamanho da tela
    int pokeballX = GetScreenWidth()/2;
    int pokeballY = subtitlePos.y + (int)(64 * scale);
    
    DrawCircle(pokeballX, pokeballY, pokeballSize, RED);
    DrawCircle(pokeballX, pokeballY, pokeballSize - (int)(8 * scale), WHITE);
    DrawCircle(pokeballX, pokeballY, pokeballSize/4, WHITE);
    DrawCircle(pokeballX, pokeballY, pokeballSize/4 - (int)(3 * scale), DARKGRAY);
    DrawLine(pokeballX - pokeballSize, pokeballY, pokeballX + pokeballSize, pokeballY, BLACK);
    
    // Área de menu
    int menuWidth = (int)(800 * scale); // Aumentado proporcionalmente
    int menuHeight = (int)(512 * scale); // Aumentado proporcionalmente
    Rectangle menuArea = {
        GetScreenWidth()/2 - menuWidth/2,
        pokeballY + pokeballSize + (int)(48 * scale),
        menuWidth,
        menuHeight
    };
    
    // Painel do menu com efeito decorativo
    DrawRectangleRounded(menuArea, 0.05f, 8, ColorAlpha(WHITE, 0.9f));
    DrawRectangleRoundedLines(menuArea, 0.05f, 8, DARKGRAY);
    
    // Desenhar decoração na borda do menu
    int decorSize = (int)(4 * scale);
    int decorSpacing = (int)(8 * scale);
    
    for (int i = 0; i < menuArea.width - (int)(16 * scale); i += decorSpacing) {
        DrawRectangle(menuArea.x + decorSpacing + i, menuArea.y + decorSpacing, decorSize, decorSize, 
                       (i % (decorSpacing * 2) == 0) ? primaryColor : secondaryColor);
        
        DrawRectangle(menuArea.x + decorSpacing + i, menuArea.y + menuArea.height - (int)(12 * scale), decorSize, decorSize, 
                       (i % (decorSpacing * 2) == 0) ? primaryColor : secondaryColor);
    }
    
    // Desenhar botões do menu
    int buttonWidth = (int)(480 * scale); // Aumentado proporcionalmente
    int buttonHeight = (int)(80 * scale); // Aumentado proporcionalmente
    int buttonSpacing = (int)(24 * scale); // Aumentado proporcionalmente
    int startY = menuArea.y + (int)(48 * scale);
    
    // Opções do menu principal
    const char* menuOptions[] = {
        "INICIAR JOGO",
        "CONFIGURAÇÕES",
        "TABELA DE TIPOS",
        "CRÉDITOS",
        "SAIR"
    };
    
    // Cores para cada botão
    Color menuColors[] = {
        (Color){ 220, 60, 60, 255 },    // Vermelho - Jogar
        (Color){ 60, 180, 120, 255 },   // Verde - Configurações
        (Color){ 80, 100, 220, 255 },   // Azul - Tabela de Tipos
        (Color){ 220, 140, 60, 255 },   // Laranja - Créditos
        (Color){ 150, 60, 150, 255 }    // Roxo - Sair
    };
    
    // Ícones para os botões (representados por formas simples)
    for (int i = 0; i < 5; i++) {
        Rectangle buttonBounds = {
            menuArea.x + menuArea.width/2 - buttonWidth/2,
            startY,
            buttonWidth,
            buttonHeight
        };
        
        // Verificar se o mouse está sobre o botão
        bool isHovering = CheckCollisionPointRec(GetMousePosition(), buttonBounds);
        
        // Cores dinâmicas com base no hover
        Color currentColor = menuColors[i];
        if (isHovering) {
            // Brilho ao passar o mouse
            currentColor.r = (currentColor.r + 30) > 255 ? 255 : (currentColor.r + 30);
            currentColor.g = (currentColor.g + 30) > 255 ? 255 : (currentColor.g + 30);
            currentColor.b = (currentColor.b + 30) > 255 ? 255 : (currentColor.b + 30);
            
            // Destacar com linha pontilhada
            float time = GetTime() * 10.0f;
            int dotSize = (int)(2 * scale);
            int dotSpacing = (int)(4 * scale);
            
            for (int j = 0; j < buttonBounds.width; j += dotSpacing) {
                if ((int)(j + time) % (dotSpacing * 2) < dotSpacing) {
                    DrawRectangle(buttonBounds.x + j, buttonBounds.y - (int)(5 * scale), dotSize, dotSize, WHITE);
                    DrawRectangle(buttonBounds.x + j, buttonBounds.y + buttonBounds.height + (int)(3 * scale), dotSize, dotSize, WHITE);
                }
            }
        }
        
        // Desenhar botão com efeito de profundidade
        DrawRectangleRounded(buttonBounds, 0.2f, 8, currentColor);
        
        // Sombra na parte inferior
        DrawRectangleRounded(
            (Rectangle){ buttonBounds.x, buttonBounds.y + buttonBounds.height - (int)(8 * scale), buttonBounds.width, (int)(8 * scale) },
            0.2f, 8, ColorAlpha(BLACK, 0.3f)
        );
        
        // Brilho na parte superior
        DrawRectangleRounded(
            (Rectangle){ buttonBounds.x, buttonBounds.y, buttonBounds.width, (int)(8 * scale) },
            0.2f, 8, ColorAlpha(WHITE, 0.3f)
        );
        
        // Desenhar ícone para cada botão
        int iconSize = (int)(30 * scale);
        Rectangle iconRect = { 
            buttonBounds.x + (int)(24 * scale), 
            buttonBounds.y + buttonBounds.height/2 - iconSize/2, 
            iconSize, 
            iconSize 
        };
        
        switch (i) {
            case 0: // Jogar - Triângulo (Play)
                DrawPoly((Vector2){iconRect.x + iconSize/2, iconRect.y + iconSize/2}, 3, iconSize/2, 0, WHITE);
                break;
            case 1: // Configurações - Engrenagem
                DrawCircle(iconRect.x + iconSize/2, iconRect.y + iconSize/2, (int)(13 * scale), WHITE);
                DrawCircle(iconRect.x + iconSize/2, iconRect.y + iconSize/2, (int)(7 * scale), currentColor);
                for (int j = 0; j < 8; j++) {
                    float angle = j * PI / 4.0f;
                    DrawLine(
                        iconRect.x + iconSize/2 + cosf(angle) * (int)(12 * scale),
                        iconRect.y + iconSize/2 + sinf(angle) * (int)(12 * scale),
                        iconRect.x + iconSize/2 + cosf(angle) * (int)(20 * scale),
                        iconRect.y + iconSize/2 + sinf(angle) * (int)(20 * scale),
                        WHITE
                    );
                }
                break;
            case 2: // Tabela - Grid
                DrawRectangleLines(iconRect.x, iconRect.y, iconSize, iconSize, WHITE);
                DrawLine(iconRect.x, iconRect.y + iconSize/3, iconRect.x + iconSize, iconRect.y + iconSize/3, WHITE);
                DrawLine(iconRect.x, iconRect.y + iconSize*2/3, iconRect.x + iconSize, iconRect.y + iconSize*2/3, WHITE);
                DrawLine(iconRect.x + iconSize/3, iconRect.y, iconRect.x + iconSize/3, iconRect.y + iconSize, WHITE);
                DrawLine(iconRect.x + iconSize*2/3, iconRect.y, iconRect.x + iconSize*2/3, iconRect.y + iconSize, WHITE);
                break;
            case 3: // Créditos - Estrela
                DrawPoly((Vector2){iconRect.x + iconSize/2, iconRect.y + iconSize/2}, 5, iconSize/2, 0, WHITE);
                break;
            case 4: // Sair - X
                DrawLine(iconRect.x + (int)(5 * scale), iconRect.y + (int)(5 * scale), 
                        iconRect.x + iconSize - (int)(5 * scale), iconRect.y + iconSize - (int)(5 * scale), WHITE);
                DrawLine(iconRect.x + iconSize - (int)(5 * scale), iconRect.y + (int)(5 * scale), 
                        iconRect.x + (int)(5 * scale), iconRect.y + iconSize - (int)(5 * scale), WHITE);
                break;
        }
        
        // Texto do botão
        int fontSize = (int)(38 * scale); // Aumentado proporcionalmente
        DrawText(
            menuOptions[i], 
            buttonBounds.x + (int)(96 * scale), 
            buttonBounds.y + buttonBounds.height/2 - fontSize/2, 
            fontSize, 
            WHITE
        );
        
        // Verificar clique
        if (isHovering && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            PlaySound(selectSound);
            
            // Mudança de tela com base na opção escolhida
            switch (i) {
                case 0: currentScreen = OPPONENT_SELECTION; break;
                case 1: currentScreen = SETTINGS; break;
                case 2: currentScreen = TYPES_TABLE; break;
                case 3: currentScreen = CREDITS; break;
                case 4: currentScreen = EXIT; break;
            }
        }
        
        startY += buttonHeight + buttonSpacing;
    }
    
    // Rodapé com versão do jogo
    const char* version = "v1.0.0 (2025)";
    int versionFontSize = (int)(25 * scale);
    DrawText(version, 
             GetScreenWidth() - MeasureText(version, versionFontSize) - (int)(16 * scale), 
             GetScreenHeight() - (int)(40 * scale), 
             versionFontSize, 
             DARKGRAY);
             
    // Desenhar efeitos de partículas (pequenos pontos flutuando)
    static float particleTimer = 0.0f;
    particleTimer += GetFrameTime();
    
    for (int i = 0; i < 20; i++) {
        float x = sinf(particleTimer * 0.5f + i * 0.3f) * GetScreenWidth() * 0.4f + GetScreenWidth() * 0.5f;
        float y = cosf(particleTimer * 0.2f + i * 0.7f) * (int)(160 * scale) + GetScreenHeight() * 0.2f;
        float size = sinf(particleTimer + i) * (int)(3 * scale) + (int)(5 * scale);
        
        DrawCircle(x, y, size, ColorAlpha(accentColor, 0.7f));
    }
}

void updateMainMenu(void) {
    // Adicionar suporte para F11 (tela cheia)
    if (IsKeyPressed(KEY_F11)) {
        pendingFullscreen = !pendingFullscreen;
        applySettings();
    }
}
 