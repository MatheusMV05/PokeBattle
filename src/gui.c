#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#endif


// gui.c
#include "gui.h"
#include "monsters.h"
#include "game_state.h"
#include <string.h>
#include <math.h>

#include "globals.h"

// Cores do tema
#define PKMN_LIGHT_BLUE    (Color){ 95, 205, 255, 255 }
#define PKMN_DARK_BLUE     (Color){ 16, 144, 192, 255 }
#define PKMN_RED           (Color){ 248, 88, 96, 255 }
#define PKMN_GREEN         (Color){ 64, 200, 88, 255 }
#define PKMN_YELLOW        (Color){ 248, 216, 104, 255 }
#define PKMN_BLACK         (Color){ 48, 48, 48, 255 }
#define PKMN_WHITE         (Color){ 248, 248, 248, 255 }

// Texturas e recursos para o tema
static Texture2D pokemonFrameTexture;
static Texture2D pokemonButtonTexture;
static Texture2D pokemonDialogTexture;

// Carrega o tema
void LoadPokemonTheme(void) {
    GuiSetStyle(DEFAULT, BACKGROUND_COLOR, ColorToInt(PKMN_LIGHT_BLUE));
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(PKMN_BLACK));
    GuiSetStyle(DEFAULT, TEXT_COLOR_FOCUSED, ColorToInt(PKMN_DARK_BLUE));
    GuiSetStyle(DEFAULT, TEXT_COLOR_PRESSED, ColorToInt(PKMN_WHITE));

    GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, ColorToInt(PKMN_DARK_BLUE));
    GuiSetStyle(BUTTON, BASE_COLOR_FOCUSED, ColorToInt((Color){ 36, 164, 212, 255 }));
    GuiSetStyle(BUTTON, BASE_COLOR_PRESSED, ColorToInt(PKMN_RED));

    GuiSetStyle(BUTTON, BORDER_COLOR_NORMAL, ColorToInt(PKMN_BLACK));
    GuiSetStyle(BUTTON, BORDER_COLOR_FOCUSED, ColorToInt(PKMN_WHITE));
    GuiSetStyle(BUTTON, BORDER_COLOR_PRESSED, ColorToInt(PKMN_BLACK));

    GuiSetStyle(BUTTON, TEXT_COLOR_NORMAL, ColorToInt(PKMN_WHITE));
    GuiSetStyle(BUTTON, TEXT_COLOR_FOCUSED, ColorToInt(PKMN_WHITE));
    GuiSetStyle(BUTTON, TEXT_COLOR_PRESSED, ColorToInt(PKMN_WHITE));

    GuiSetStyle(BUTTON, BORDER_WIDTH, 2);
    GuiSetStyle(BUTTON, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);

}

// Descarrega recursos do tema
void UnloadPokemonTheme(void) {

}

// Botão estilizado
bool GuiPokemonButton(Rectangle bounds, const char *text, bool active) {
    bool pressed = false;
    int state = GuiGetState();

    // Se não está ativo, forçar estado inativo
    if (!active) state = STATE_DISABLED;

    // Arredondar posição para alinhamento com pixel
    bounds.x = (int)bounds.x;
    bounds.y = (int)bounds.y;
    bounds.width = (int)bounds.width;
    bounds.height = (int)bounds.height;

    // Efeito de hover
    bool mouseHover = CheckCollisionPointRec(GetMousePosition(), bounds);
    if (active && mouseHover) {
        state = STATE_FOCUSED;
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            state = STATE_PRESSED;
            pressed = true;
        }
    }

    // Definir cores no estilo BW
    Color baseColor = (Color){60, 60, 60, 255}; // Cinza escuro base
    Color borderColor = BLACK;
    Color textColor = WHITE;

    // Modificar baseado no estado
    if (state == STATE_FOCUSED) {
        baseColor = (Color){80, 80, 80, 255}; // Cinza um pouco mais claro quando hover
    }
    if (state == STATE_PRESSED) {
        baseColor = (Color){100, 100, 100, 255}; // Ainda mais claro quando pressionado
    }
    if (!active) {
        baseColor = (Color){40, 40, 40, 180}; // Mais escuro quando inativo
        textColor = (Color){200, 200, 200, 180}; // Texto cinza claro quando inativo
    }

    // Desenhar sombra sutil
    DrawRectangleRounded(
        (Rectangle){bounds.x + 2, bounds.y + 2, bounds.width, bounds.height},
        0.3f, 8,
        (Color){0, 0, 0, 100}
    );

    // Botão principal com bordas mais consistentes
    DrawRectangleRounded(bounds, 0.3f, 8, baseColor);

    // Adicionar brilho sutil no topo (estilo BW)
    DrawRectangleRounded(
        (Rectangle){bounds.x + 2, bounds.y + 2, bounds.width - 4, bounds.height/3},
        0.3f, 4,
        (Color){255, 255, 255, 30}
    );

    // Borda mais visível e consistente
    DrawRectangleRoundedLines(bounds, 0.3f, 8, borderColor);

    // Texto centralizado verticalmente e horizontalmente
    int fontSize = 18;
    if (bounds.width < 100 || bounds.height < 40) {
        fontSize = 16; // Texto um pouco menor para botões pequenos
    }

    Vector2 textSize = MeasureTextEx(GetFontDefault(), text, fontSize, 1);

    // Garantir que o texto fique centralizado e não transborde
    float textX = bounds.x + bounds.width/2 - textSize.x/2;
    float textY = bounds.y + bounds.height/2 - textSize.y/2;

    // Verificar se o texto cabe na largura do botão
    if (textSize.x > bounds.width - 10) {
        // Reduzir tamanho da fonte se texto é muito grande
        fontSize -= 2;
        textSize = MeasureTextEx(GetFontDefault(), text, fontSize, 1);
        textX = bounds.x + bounds.width/2 - textSize.x/2;
        textY = bounds.y + bounds.height/2 - textSize.y/2;
    }

    DrawText(text, textX, textY, fontSize, textColor);

    return pressed;
}

// Barra de status estilo Pokémon (HP, etc)
void GuiPokemonStatusBar(Rectangle bounds, int value, int maxValue, const char *text, Color color) {
    // Calcular preenchimento baseado no valor
    float fillRatio = (float)value / maxValue;
    if (fillRatio < 0.0f) fillRatio = 0.0f;
    if (fillRatio > 1.0f) fillRatio = 1.0f;

    // Determinar a cor com base no preenchimento
    Color fillColor = color;
    if (color.r == 0 && color.g == 0 && color.b == 0) { // Se não for especificada, usar cor padrão de HP
        if (fillRatio > 0.5f) fillColor = PKMN_GREEN;
        else if (fillRatio > 0.2f) fillColor = PKMN_YELLOW;
        else fillColor = PKMN_RED;
    }

    // Desenhar fundo
    DrawRectangleRec(bounds, PKMN_BLACK);

    // Desenhar preenchimento
    Rectangle fillRect = {
        bounds.x + 2,
        bounds.y + 2,
        (bounds.width - 4) * fillRatio,
        bounds.height - 4
    };
    DrawRectangleRec(fillRect, fillColor);

    // Desenhar texto
    if (text) {
        int fontSize = 18;
        Vector2 textSize = MeasureTextEx(GetFontDefault(), text, fontSize, 1);
        DrawText(text,
                bounds.x + bounds.width/2 - textSize.x/2,
                bounds.y + bounds.height/2 - textSize.y/2,
                fontSize,
                PKMN_WHITE);
    }
}

// Caixa de mensagem estilo Pokémon
void GuiPokemonMessageBox(Rectangle bounds, const char *message) {
    // Desenhar caixa de fundo
    DrawRectangleRec(bounds, PKMN_WHITE);
    DrawRectangleLinesEx(bounds, 2, PKMN_BLACK);

    // Desenhar bordas internas
    DrawRectangle(bounds.x + 2, bounds.y + 2, bounds.width - 4, 4, PKMN_LIGHT_BLUE);
    DrawRectangle(bounds.x + 2, bounds.y + bounds.height - 6, bounds.width - 4, 4, PKMN_LIGHT_BLUE);
    DrawRectangle(bounds.x + 2, bounds.y + 6, 4, bounds.height - 12, PKMN_LIGHT_BLUE);
    DrawRectangle(bounds.x + bounds.width - 6, bounds.y + 6, 4, bounds.height - 12, PKMN_LIGHT_BLUE);

    // Desenhar texto
    DrawText(message, bounds.x + 20, bounds.y + 20, 20, PKMN_BLACK);

    // Indicador de "continuar (triangulozinho piscando)"
    static float blinkTimer = 0.0f;
    blinkTimer += GetFrameTime() * 4.0f;

    if (sinf(blinkTimer) > 0.0f) {
        DrawTriangle(
            (Vector2){ bounds.x + bounds.width - 30, bounds.y + bounds.height - 25 },
            (Vector2){ bounds.x + bounds.width - 20, bounds.y + bounds.height - 20 },
            (Vector2){ bounds.x + bounds.width - 30, bounds.y + bounds.height - 15 },
            PKMN_BLACK
        );
    }
}

// Ícone de tipo de Pokémon
void GuiPokemonTypeIcon(Rectangle bounds, int typeIndex) {
    // Obter cor do tipo
    Color typeColor = getTypeColor(typeIndex);

    // Desenhar retângulo do tipo
    DrawRectangleRec(bounds, typeColor);
    DrawRectangleLinesEx(bounds, 1, PKMN_BLACK);

    // Obter nome do tipo
    const char* typeName = getTypeName(typeIndex);

    // Desenhar nome do tipo
    int fontSize = 14;
    Vector2 textSize = MeasureTextEx(GetFontDefault(), typeName, fontSize, 1);
    DrawText(typeName,
             bounds.x + bounds.width/2 - textSize.x/2,
             bounds.y + bounds.height/2 - textSize.y/2,
             fontSize,
             PKMN_WHITE);
}

// Menu principal estilo Pokémon
void GuiPokemonMainMenu(Rectangle bounds, int *currentScreen) {
    static int selectedOption = 0;
    const char* options[] = {
        "JOGAR",
        "OPÇÕES",
        "TIPOS",
        "CRÉDITOS",
        "SAIR"
    };
    int optionCount = 5;

    // Desenhar título
    DrawText("PokeBattle", bounds.x + bounds.width/2 - MeasureText("PokeBattle", 40)/2, bounds.y + 30, 40, PKMN_BLACK);

    // Desenhar opções
    for (int i = 0; i < optionCount; i++) {
        Rectangle optionBounds = {
            bounds.x + bounds.width/2 - 150,
            bounds.y + 150 + i * 70,
            300,
            50
        };

        bool isSelected = (selectedOption == i);

        // Desenhar cursor de seleção (Pokébola)
        if (isSelected) {
            DrawCircle(optionBounds.x - 30, optionBounds.y + optionBounds.height/2, 12, PKMN_RED);
            DrawCircle(optionBounds.x - 30, optionBounds.y + optionBounds.height/2, 10, PKMN_WHITE);
            DrawCircle(optionBounds.x - 30, optionBounds.y + optionBounds.height/2, 4, PKMN_BLACK);
            DrawLine(optionBounds.x - 40, optionBounds.y + optionBounds.height/2,
                     optionBounds.x - 20, optionBounds.y + optionBounds.height/2,
                     PKMN_BLACK);
        }

        // Desenhar botão
        if (GuiPokemonButton(optionBounds, options[i], true)) {
            *currentScreen = i + 1; // +1 porque os valores da enum começam com MAIN_MENU=0
        }

        // Verificar hover para mudança de seleção
        if (CheckCollisionPointRec(GetMousePosition(), optionBounds)) {
            selectedOption = i;
        }
    }

    // Controle de teclado
    if (IsKeyPressed(KEY_UP)) {
        selectedOption = (selectedOption - 1 + optionCount) % optionCount;
    }
    else if (IsKeyPressed(KEY_DOWN)) {
        selectedOption = (selectedOption + 1) % optionCount;
    }
    else if (IsKeyPressed(KEY_ENTER)) {
        *currentScreen = selectedOption + 1;
    }
}

// Menu de batalha estilo Pokémon
void GuiPokemonBattleMenu(Rectangle bounds, int *currentOption) {
    static int selectedOption = 0;

    const char* options[] = {
        "LUTAR",
        "MOCHILA",
        "POKÉMON",
        "FUGIR"
    };

    // Desenhar caixa de fundo
    DrawRectangleRec(bounds, PKMN_WHITE);
    DrawRectangleLinesEx(bounds, 2, PKMN_BLACK);

    // Desenhar grid 2x2 de opções
    int cols = 2;
    int rows = 2;
    int buttonWidth = (bounds.width - 40) / cols;
    int buttonHeight = (bounds.height - 40) / rows;

    for (int i = 0; i < 4; i++) {
        int col = i % cols;
        int row = i / cols;

        Rectangle buttonBounds = {
            bounds.x + 20 + col * (buttonWidth + 10),
            bounds.y + 20 + row * (buttonHeight + 10),
            buttonWidth,
            buttonHeight
        };

        // Cores diferentes para cada botão
        Color buttonColor;
        switch(i) {
            case 0: buttonColor = PKMN_RED; break;        // LUTAR
            case 1: buttonColor = PKMN_LIGHT_BLUE; break; // MOCHILA
            case 2: buttonColor = PKMN_GREEN; break;      // POKÉMON
            case 3: buttonColor = PKMN_YELLOW; break;     // FUGIR
            default: buttonColor = PKMN_DARK_BLUE;
        }

        // Desenhar botão customizado
        DrawRectangleRec(buttonBounds, buttonColor);
        DrawRectangleLinesEx(buttonBounds, 2, PKMN_BLACK);

        // Texto
        DrawText(options[i],
                buttonBounds.x + buttonBounds.width/2 - MeasureText(options[i], 20)/2,
                buttonBounds.y + buttonBounds.height/2 - 10,
                20,
                PKMN_WHITE);

        // Verificar clique
        if (CheckCollisionPointRec(GetMousePosition(), buttonBounds) &&
            IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            *currentOption = i;
            PlaySound(selectSound); // Som ao clicar
        }
    }
}

// Diálogo estilo Pokémon
bool GuiPokemonDialog(Rectangle bounds, const char *title, const char *message, const char *buttons) {
    static bool result = false;
    static bool active = true;

    // Desenhar fundo semi-transparente
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){ 0, 0, 0, 150 });

    // Desenhar caixa de diálogo
    DrawRectangleRec(bounds, PKMN_WHITE);
    DrawRectangleLinesEx(bounds, 2, PKMN_BLACK);

    // Título
    if (title) {
        DrawText(title,
                bounds.x + bounds.width/2 - MeasureText(title, 24)/2,
                bounds.y + 20,
                24,
                PKMN_BLACK);
    }

    // Mensagem
    if (message) {
        DrawText(message,
                bounds.x + 30,
                bounds.y + 60,
                20,
                PKMN_BLACK);
    }

    // Botões
    if (buttons) {
        char buttonsSeparated[256];
        strcpy(buttonsSeparated, buttons);

        // Parse dos botões separados por ";"
        char *button = strtok(buttonsSeparated, ";");
        int buttonCount = 0;
        int btnWidth = 120;
        int spacing = 30;
        int startX = bounds.x + bounds.width/2 - (btnWidth + spacing/2);

        while (button != NULL) {
            Rectangle btnBounds = {
                startX + buttonCount * (btnWidth + spacing),
                bounds.y + bounds.height - 70,
                btnWidth,
                40
            };

            if (GuiPokemonButton(btnBounds, button, active)) {
                result = (buttonCount == 0); // true para o primeiro botão (geralmente "Sim")
                active = false;
                return result;
            }

            button = strtok(NULL, ";");
            buttonCount++;
        }
    }

    return false;
}