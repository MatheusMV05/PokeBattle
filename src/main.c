
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#endif

#include "raygui.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>  // Para funções seno/cosseno
#include "raylib.h"
#include "globals.h"
#include "monsters.h"
#include "battle.h"
#include "ia_integration.h"
#include "render/menu_renderer.h"
#include "render/battle_renderer.h"
#include "render/selection_renderer.h"
#include "render/settings_renderer.h"
#include "types_table_renderer.h"
#include "render/credits_renderer.h"
#include "main.h"
#include "resources.h"
#include "gui.h"
#include "battle_effects.h"

// Declarações das funções
void initializeGame(void);
void updateGame(void);
void cleanupGame(void);
void changeScreen(GameState newScreen);
void loadMonsterTextures(void);
void unloadMonsterTextures(void);
bool testAIConnection(void);

// Desenha uma Pokébola estilizada
void DrawStylizedPokeball(int centerX, int centerY, float radius, float rotation) {
    // Círculo externo vermelho
    DrawCircle(centerX, centerY, radius, RED);

    // Círculo interno branco
    DrawCircle(centerX, centerY, radius * 0.9f, WHITE);

    // Linha divisória
    DrawRectangle(
        centerX - radius,
        centerY - radius * 0.1f,
        radius * 2,
        radius * 0.2f,
        BLACK
    );

    // Botão central
    DrawCircle(centerX, centerY, radius * 0.15f, BLACK);
    DrawCircle(centerX, centerY, radius * 0.1f, WHITE);

    // Adicionar efeito de brilho com rotação
    float brightX = centerX + cosf(rotation) * (radius * 0.4f);
    float brightY = centerY + sinf(rotation) * (radius * 0.4f);
    DrawCircle(brightX, brightY, radius * 0.05f, (Color){255, 255, 255, 150});
}

// Função para desenhar uma estrela
void DrawStar(float x, float y, float outerRadius, float innerRadius, int points, Color color, float rotation) {
    float angle = 0.0f;
    float angleStep = 2.0f * PI / points;

    for (int i = 0; i < points; i++) {
        float nextAngle = angle + angleStep / 2.0f;

        Vector2 p1 = {x + cosf(angle + rotation) * outerRadius, y + sinf(angle + rotation) * outerRadius};
        Vector2 p2 = {x + cosf(nextAngle + rotation) * innerRadius, y + sinf(nextAngle + rotation) * innerRadius};
        Vector2 p3 = {x, y};

        DrawTriangle(p1, p2, p3, color);

        angle = nextAngle + angleStep / 2.0f;
    }
}

// Função para desenhar uma tela de carregamento estilizada
void drawEnhancedLoadingScreen(const char* message, float progress, float timer) {
    BeginDrawing();

    // Fundo com gradiente estilo Pokémon
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

    // Padrão de linhas decorativas (estilo grid)
    for (int i = 0; i < GetScreenHeight(); i += 20) {
        int yPos = i - (int)(timer * 30) % 20;
        if (yPos < 0) yPos += 20;
        DrawRectangle(0, yPos, GetScreenWidth(), 2, (Color){255, 255, 255, 30});
    }

    for (int i = 0; i < GetScreenWidth(); i += 20) {
        int xPos = i - (int)(timer * 15) % 20;
        if (xPos < 0) xPos += 20;
        DrawRectangle(xPos, 0, 2, GetScreenHeight(), (Color){255, 255, 255, 20});
    }

    // Título pulsante
    const char* title = "PokeBattle";
    float pulseFactor = 1.0f + sinf(timer * 3.0f) * 0.05f;
    int titleFontSize = (int)(60 * pulseFactor);

    // Sombra do título
    DrawText(title,
             GetScreenWidth()/2 - MeasureText(title, titleFontSize)/2 + 4,
             GetScreenHeight()/4 + 4,
             titleFontSize,
             (Color){0, 0, 0, 120});

    // Título com cores de gradiente
    float hue = timer * 30.0f;
    Color titleColor = (Color){
        (unsigned char)(220 + sinf(timer * 2.0f) * 35),
        (unsigned char)(220 + cosf(timer * 2.5f) * 35),
        255,
        255
    };

    DrawText(title,
             GetScreenWidth()/2 - MeasureText(title, titleFontSize)/2,
             GetScreenHeight()/4,
             titleFontSize,
             titleColor);

    // Desenhar decoração com PokéBolas
    int ballSize = 50;
    float ballRotation = timer * 2.0f;

    // Desenhar Pokébolas ao redor do título
    for (int i = 0; i < 4; i++) {
        float angle = timer + (float)i * (PI/2);
        float offsetX = sinf(angle) * 250.0f;
        float offsetY = cosf(angle) * 100.0f;
        float localRotation = ballRotation + i * 0.5f;

        DrawStylizedPokeball(
            GetScreenWidth()/2 + offsetX,
            GetScreenHeight()/4 + offsetY,
            ballSize - 10 * i % 3,
            localRotation
        );
    }

    // Mensagem de carregamento com efeito de digitação
    int displayChars = (int)(strlen(message) * fminf(1.0f, timer * 4.0f));
    char displayMessage[256] = {0};
    strncpy(displayMessage, message, displayChars);
    displayMessage[displayChars] = '\0';

    DrawText(displayMessage,
             GetScreenWidth()/2 - MeasureText(message, 30)/2,
             GetScreenHeight()/2 + 20,
             30,
             WHITE);

    // Cursor piscante ao final do texto
    if (fmodf(timer, 1.0f) > 0.5f && displayChars < strlen(message)) {
        int textWidth = MeasureText(displayMessage, 30);
        DrawRectangle(
            GetScreenWidth()/2 - MeasureText(message, 30)/2 + textWidth,
            GetScreenHeight()/2 + 20,
            15,
            30,
            WHITE
        );
    }

    // Barra de progresso estilizada
    Rectangle progressBarBg = {
        GetScreenWidth()/2 - 200,
        GetScreenHeight()/2 + 80,
        400,
        30
    };

    // Fundo da barra com estilo 3D
    DrawRectangleRec(progressBarBg, (Color){40, 40, 40, 200});
    DrawRectangleLinesEx(progressBarBg, 2, (Color){100, 100, 100, 255});

    // Cor da barra baseada no progresso
    Color progressColor;
    if (progress < 0.3f) {
        progressColor = RED;
    } else if (progress < 0.7f) {
        progressColor = YELLOW;
    } else {
        progressColor = GREEN;
    }

    // Barra de progresso com efeito de gradiente
    Rectangle progressBarFg = {
        progressBarBg.x + 4,
        progressBarBg.y + 4,
        (progressBarBg.width - 8) * progress,
        progressBarBg.height - 8
    };

    DrawRectangleGradientH(
        progressBarFg.x,
        progressBarFg.y,
        progressBarFg.width,
        progressBarFg.height,
        progressColor,
        (Color){255, 255, 255, 200}
    );

    // Adicionar pequenas Pokébolas na barra de progresso
    int numBalls = 8;
    for (int i = 0; i < numBalls; i++) {
        float ballProgress = (float)i / (numBalls - 1);
        if (ballProgress <= progress) {
            DrawStylizedPokeball(
                progressBarBg.x + 20 + ballProgress * (progressBarBg.width - 40),
                progressBarBg.y + progressBarBg.height/2,
                8,
                timer * (i % 2 == 0 ? 1.0f : -1.0f)
            );
        }
    }

    // Porcentagem
    char percentText[10];
    sprintf(percentText, "%d%%", (int)(progress * 100));
    DrawText(percentText,
             GetScreenWidth()/2 - MeasureText(percentText, 24)/2,
             progressBarBg.y + progressBarBg.height + 15,
             24,
             WHITE);

    // Adicionar estrelas decorativas animadas
    for (int i = 0; i < 8; i++) {
        float starX = 100 + (GetScreenWidth() - 200) * ((float)i / 8 + sinf(timer + i) * 0.05f);
        float starY = GetScreenHeight() - 100 + cosf(timer * 1.5f + i) * 30;
        float starSize = 8 + sinf(timer * 2.0f + i) * 3;

        Color starColor = (Color){
            (unsigned char)(180 + sinf(timer + i) * 75),
            (unsigned char)(180 + sinf(timer * 1.2f + i) * 75),
            255,
            (unsigned char)(120 + sinf(timer * 2.0f + i) * 60)
        };

        DrawStar(starX, starY, starSize, starSize * 0.4f, 5, starColor, timer * 0.5f + i);
    }

    // Dica na parte inferior
    const char* tip;
    int tipIndex = (int)(timer / 2.5f) % 4; // Alternar entre diferentes dicas

    switch (tipIndex) {
        case 0:
            tip = "Dica: Pokémons com tipo vantajoso causam 2x de dano!";
            break;
        case 1:
            tip = "Dica: Monstros dormindo não podem atacar!";
            break;
        case 2:
            tip = "Dica: Escolha seus ataques baseados no tipo do oponente!";
            break;
        case 3:
            tip = "Dica: A velocidade determina quem ataca primeiro!";
            break;
        default:
            tip = "Carregando...";
    }

    // Efeito de fade-in/fade-out suave
    float tipTimer = fmodf(timer, 2.5f); // Timer específico para cada dica
    float fadeEffect;

    // Primeiros 20% do tempo: fade-in
    if (tipTimer < 0.5f) {
        fadeEffect = tipTimer / 0.5f; // 0.0 a 1.0
    }
    // Últimos 20% do tempo: fade-out
    else if (tipTimer > 2.0f) {
        fadeEffect = 1.0f - ((tipTimer - 2.0f) / 0.5f); // 1.0 a 0.0
    }
    // Meio do tempo: totalmente visível
    else {
        fadeEffect = 1.0f;
    }


    DrawText(tip,
             GetScreenWidth()/2 - MeasureText(tip, 20)/2,
             GetScreenHeight() - 50,
             20,
             (Color){255, 255, 255, (unsigned char)(180 * fadeEffect)});

    // Texto de "Pressione qualquer tecla" no final do carregamento
    if (progress >= 1.0f) {
        const char* readyText = "Carregamento concluído! Pressione qualquer tecla para continuar...";

        // Texto piscante
        if (sinf(timer * 4.0f) > 0.0f) {
            DrawText(readyText,
                     GetScreenWidth()/2 - MeasureText(readyText, 22)/2,
                     GetScreenHeight() - 90,
                     22,
                     YELLOW);
        }
    }

    EndDrawing();
}

// Função principal
int main(void)
{
    // Inicialização do Raylib
    const int screenWidth = 1280;
    const int screenHeight = 720;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);

    InitWindow(screenWidth, screenHeight, "PokeBattle");
    InitAudioDevice();
    SetTargetFPS(60);

    // Inicializando o gerador de números aleatórios
    srand(time(NULL));

    // Variáveis para controle do carregamento
    float loadingTimer = 0.0f;
    float loadingProgress = 0.0f;
    bool loadingComplete = false;
    int loadingStage = 0;

    // Primeiras inicializações básicas sem exibição
    initializeGlobals();

    // Loop de carregamento
    while (!loadingComplete && !WindowShouldClose()) {
        // Atualizar timer
        float deltaTime = GetFrameTime();
        loadingTimer += deltaTime;

        // Atualizar o estágio de carregamento e o progresso
        switch (loadingStage) {
            case 0:
                drawEnhancedLoadingScreen("Inicializando sistema...", 0.05f, loadingTimer);
                loadingStage = 1;
                loadingProgress = 0.05f;
                break;

            case 1:
                drawEnhancedLoadingScreen("Verificando recursos...", 0.15f, loadingTimer);
                verifyMonsterSprites();
                loadingStage = 2;
                loadingProgress = 0.15f;
                break;

            case 2:
                drawEnhancedLoadingScreen("Carregando Pokédex...", 0.25f, loadingTimer);
                initializeMonsterDatabase();
                loadingStage = 3;
                loadingProgress = 0.25f;
                break;

            case 3:
                drawEnhancedLoadingScreen("Inicializando sistema de batalha...", 0.35f, loadingTimer);
                initializeBattleSystem();
                InitBattleEffectsSystem();
                loadingStage = 4;
                loadingProgress = 0.35f;
                break;

            case 4:
                drawEnhancedLoadingScreen("Carregando interface...", 0.50f, loadingTimer);
                LoadPokemonTheme();
                loadTextures();
                battleBackground = LoadTexture("resources/battle_background.png");
                loadingStage = 5;
                loadingProgress = 0.50f;
                break;

            case 5:
                drawEnhancedLoadingScreen("Carregando sons e música...", 0.65f, loadingTimer);
                loadSounds(musicVolume, soundVolume);
                loadingStage = 6;
                loadingProgress = 0.65f;
                break;

            case 6:
                drawEnhancedLoadingScreen("Carregando sprites dos Pokémons...", 0.75f, loadingTimer);
                loadMonsterTextures();
                loadingStage = 7;
                loadingProgress = 0.75f;
                break;

            case 7:
                drawEnhancedLoadingScreen("Conectando à Inteligência Artificial...", 0.90f, loadingTimer);
                if (initializeAI()) {
                    printf("\n=== TESTANDO CONEXÃO COM IA ===\n");
                    testAIConnection();
                    printf("==============================\n\n");
                } else {
                    printf("\n=== IA INDISPONÍVEL ===\n");
                    printf("Usando sistema de IA local (simples) para o bot.\n");
                    printf("======================\n\n");
                }
                loadingStage = 8;
                loadingProgress = 1.0f;
                break;

            case 8:
                // Carregamento completo, aguardar input
                drawEnhancedLoadingScreen("Tudo pronto para começar!", 1.0f, loadingTimer);
                gameInitialized = true;

                // Verificar se alguma tecla foi pressionada
                if (IsKeyReleased(KEY_ENTER) || IsKeyReleased(KEY_SPACE) ||
                    IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                    loadingComplete = true;
                    currentScreen = MAIN_MENU;
                    PlayMusicStream(menuMusic);
                }
                break;
        }
    }

    // Loop principal do jogo (após o carregamento)
    while (!WindowShouldClose() && gameRunning)
    {
        // Atualização
        updateGame();

        // Renderização
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // Renderizar a tela atual
        switch (currentScreen)
        {
        case MAIN_MENU:
            drawMainMenu();
            break;
        case OPPONENT_SELECTION:
            drawOpponentSelection();
            break;
        case MONSTER_SELECTION:
            drawMonsterSelection();
            break;
        case BATTLE_SCREEN:
            drawBattleScreen();
            break;
        case TYPES_TABLE:
            drawTypesTable();
            break;
        case SETTINGS:
            drawSettings();
            break;
        case CREDITS:
            drawCredits();
            break;
        case EXIT:
            gameRunning = false;
            break;
        default:
            printf("[DEBUG] Estado desconhecido: %d\n", currentScreen);
            break;
        }

        EndDrawing();
    }

    // Limpeza e encerramento
    cleanupGame();
    cleanupGlobals();
    CloseAudioDevice();
    ClearAllBattleEffects();
    CloseWindow();

    return 0;
}

// Atualiza o estado do jogo
void updateGame(void)
{
    // Processamento específico para cada tela
    switch (currentScreen)
    {
    case MAIN_MENU:
        updateMainMenu();
        break;
    case OPPONENT_SELECTION:
        updateOpponentSelection();
        break;
    case MONSTER_SELECTION:
        updateMonsterSelection();
        break;
    case BATTLE_SCREEN:
        updateBattleScreen();
        break;
    case TYPES_TABLE:
        updateTypesTable();
        break;
    case SETTINGS:
        updateSettings();
        break;
    case CREDITS:
        updateCredits();
        break;
    default:
        break;
    }
}

// Limpa recursos alocados
void cleanupGame(void)
{
    // Descarregar texturas dos monstros
    unloadMonsterTextures();

    // Liberar banco de monstros
    freeMonsterDatabase();

    // Liberar estruturas de batalha
    freeBattleSystem();
    UnloadPokemonTheme();
    UnloadTexture(battleBackground);

    // Liberar recursos visuais
    unloadTextures();
    unloadSounds();

    // Encerrar API de IA
    shutdownAI();
}

// Função para alterar a tela atual
void changeScreen(GameState newScreen)
{
    currentScreen = newScreen;
}