// Define macros de prevenção de conflitos antes de qualquer include
#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #define NOGDI
    #define NOUSER
#endif

#include "globals.h"
#include "monsters.h"
#include <string.h>
#include <curl/curl.h>
#include "decision_tree.h"
#include "hud_elements.h"
#include "resources.h"

// Variáveis de estado do jogo
GameState currentScreen = MAIN_MENU;
bool gameRunning = true;
bool vsBot = true;
bool playerTurn = true;
bool gameInitialized = false;

// Sistema de batalha
BattleSystem* battleSystem = NULL;
bool actionQueueReady = false;
char battleMessage[256] = "";
BattleMessage currentMessage = {0};
BattleAnimation currentAnimation = {0};
float stateTransitionDelay = 0.0f;

// Variáveis de configuração
bool fullscreen = false;
int currentResolutionIndex = 0;
int pendingResolutionIndex = 0;
bool hasUnsavedChanges = false;
float pendingMusicVolume = 0.7f;
float pendingSoundVolume = 0.8f;
bool pendingFullscreen = false;
int pendingDifficultyIndex = 1;
int pendingAnimSpeedIndex = 1;
DecisionNode* botDecisionTree = NULL;

// Integração com IA
bool initialized = false;
CURL* curl_handle = NULL;

// Teams
MonsterList* playerTeam = NULL;
MonsterList* opponentTeam = NULL;

// Recursos visuais e sonoros
Font gameFont;
Texture2D backgroundTexture;
Texture2D menuBackground;
Texture2D battleBackground;
Texture2D monsterSelectBackground;
Texture2D typeIcons[TYPE_COUNT];
Sound selectSound;
Sound attackSound;
Sound hitSound;
Sound faintSound;
Music menuMusic;
Music battleMusic;
int currentBattleBackground = 0;
char errorBuffer[256] = {0};
Texture2D battleBackgrounds[BATTLE_BACKGROUNDS_COUNT] = {0};


void initializeGlobals(void) {
    // Inicializar valores padrão
    currentScreen = MAIN_MENU;
    gameRunning = true;
    vsBot = true;
    playerTurn = true;
    gameInitialized = false;

    // Limpar strings
    memset(battleMessage, 0, sizeof(battleMessage));
    memset(&currentMessage, 0, sizeof(currentMessage));
    memset(&currentAnimation, 0, sizeof(currentAnimation));

    // Inicializar sistema de batalha
    actionQueueReady = false;
    stateTransitionDelay = 0.0f;

    // Configurações
    musicVolume = 0.7f;
    soundVolume = 0.8f;
    fullscreen = false;
    pendingMusicVolume = musicVolume;
    pendingSoundVolume = soundVolume;
    pendingFullscreen = fullscreen;
    pendingDifficultyIndex = 1;
    pendingAnimSpeedIndex = 1;
    hasUnsavedChanges = false;

    // Teams
    playerTeam = NULL;
    opponentTeam = NULL;

    // Recursos (serão carregados posteriormente)
    currentBattleBackground = 0;
}

void cleanupGlobals(void) {
    // Liberação de recursos
    if (playerTeam != NULL) {
        freeMonsterList(playerTeam);
        playerTeam = NULL;
    }

    if (opponentTeam != NULL) {
        freeMonsterList(opponentTeam);
        opponentTeam = NULL;
    }
}




 // Tabela de tipos
 void drawTypesTable(void) {
     // Desenhar fundo
     ClearBackground(RAYWHITE);

     // Desenhar título
     const char* title = "Tabela de Tipos";
     Vector2 titleSize = MeasureTextEx(gameFont, title, 40, 2);
     DrawTextEx(gameFont, title, (Vector2){ GetScreenWidth()/2 - titleSize.x/2, 20 }, 40, 2, BLACK);

     // Desenhar tabela de efetividade de tipos
     int cellSize = 50;
     int startX = (GetScreenWidth() - (cellSize * (TYPE_COUNT + 1))) / 2;
     int startY = 80;

     // Desenhar cabeçalhos de coluna (tipos atacantes)
     for (int i = 0; i < TYPE_COUNT; i++) {
         Rectangle cell = { startX + (i + 1) * cellSize, startY, cellSize, cellSize };
         DrawRectangleRec(cell, getTypeColor(i));
         DrawText(getTypeName(i), cell.x + 5, cell.y + 15, 10, WHITE);
     }

     // Desenhar cabeçalhos de linha (tipos defensores)
     for (int i = 0; i < TYPE_COUNT; i++) {
         Rectangle cell = { startX, startY + (i + 1) * cellSize, cellSize, cellSize };
         DrawRectangleRec(cell, getTypeColor(i));
         DrawText(getTypeName(i), cell.x + 5, cell.y + 15, 10, WHITE);
     }

     // Desenhar células de efetividade
     for (int i = 0; i < TYPE_COUNT; i++) {
         for (int j = 0; j < TYPE_COUNT; j++) {
             Rectangle cell = {
                 startX + (j + 1) * cellSize,
                 startY + (i + 1) * cellSize,
                 cellSize,
                 cellSize
             };

             float effectiveness = typeEffectiveness[j][i];
             Color cellColor;

             if (effectiveness > 1.5f) {
                 cellColor = GREEN;
             } else if (effectiveness < 0.5f) {
                 cellColor = RED;
             } else if (effectiveness == 0.0f) {
                 cellColor = BLACK;
             } else {
                 cellColor = LIGHTGRAY;
             }

             DrawRectangleRec(cell, cellColor);

             char effText[10];
             if (effectiveness == 0.0f) {
                 strcpy(effText, "0");
             } else {
                 sprintf(effText, "%.1fx", effectiveness);
             }

             DrawText(effText, cell.x + cell.width/2 - MeasureText(effText, 20)/2,
                     cell.y + cell.height/2 - 10, 20, WHITE);
         }
     }

     // Legenda
     DrawText("Efetividade:", startX, startY + (TYPE_COUNT + 1) * cellSize + 20, 20, BLACK);

     DrawRectangle(startX, startY + (TYPE_COUNT + 1) * cellSize + 50, 20, 20, GREEN);
     DrawText("Super efetivo (>1.5x)", startX + 30, startY + (TYPE_COUNT + 1) * cellSize + 50, 20, BLACK);

     DrawRectangle(startX, startY + (TYPE_COUNT + 1) * cellSize + 80, 20, 20, LIGHTGRAY);
     DrawText("Normal (0.5x - 1.5x)", startX + 30, startY + (TYPE_COUNT + 1) * cellSize + 80, 20, BLACK);

     DrawRectangle(startX, startY + (TYPE_COUNT + 1) * cellSize + 110, 20, 20, RED);
     DrawText("Pouco efetivo (<0.5x)", startX + 30, startY + (TYPE_COUNT + 1) * cellSize + 110, 20, BLACK);

     DrawRectangle(startX, startY + (TYPE_COUNT + 1) * cellSize + 140, 20, 20, BLACK);
     DrawText("Sem efeito (0x)", startX + 30, startY + (TYPE_COUNT + 1) * cellSize + 140, 20, BLACK);

     // Botão de voltar
     if (drawButton((Rectangle){ 20, GetScreenHeight() - 70, 150, 50 }, "Voltar", BLUE)) {
         PlaySound(selectSound);
         currentScreen = MAIN_MENU;
     }
 }

 void updateTypesTable(void) {
     // Atualização da lógica da tabela de tipos, se necessário
 }



 // Créditos
 void drawCredits(void) {
     // Desenhar fundo
     ClearBackground(BLACK);

     // Desenhar título
     const char* title = "Créditos";
     Vector2 titleSize = MeasureTextEx(gameFont, title, 40, 2);
     DrawTextEx(gameFont, title, (Vector2){ GetScreenWidth()/2 - titleSize.x/2, 50 }, 40, 2, WHITE);

     // Texto dos créditos
     const char* credits[] = {
         "PokeBattle",
         "Um jogo inspirado em Pokémon Stadium",
         "",
         "Desenvolvido por:",
         "Julia Torres, Fatima Beatriz, Maria Claudia, Matheus Martins, Vinicius Jose - 2025",
         "",
         "Projeto para a disciplina de",
         "Algoritmos e Estruturas de Dados",
         "",
         "Agradecimentos:",
         "A todos os professores e colegas",
         "que tornaram este projeto possível.",
         "",
         "Recursos utilizados:",
         "Raylib - Biblioteca gráfica",
         "libcurl - Integração com API",
         "Gemini AI - API de IA para comportamento do bot"
     };

     int creditCount = sizeof(credits) / sizeof(credits[0]);
     int startY = 120;

     for (int i = 0; i < creditCount; i++) {
         int fontSize = (i == 0) ? 30 : 20;
         Color textColor = (i == 0 || i == 3 || i == 9 || i == 13) ? GOLD : WHITE;

         Vector2 textSize = MeasureTextEx(gameFont, credits[i], fontSize, 1);
         DrawTextEx(gameFont, credits[i], (Vector2){ GetScreenWidth()/2 - textSize.x/2, startY }, fontSize, 1, textColor);

         startY += fontSize + 10;
     }

     // Botão de voltar
     if (drawButton((Rectangle){ GetScreenWidth()/2 - 75, GetScreenHeight() - 70, 150, 50 }, "Voltar", BLUE)) {
         PlaySound(selectSound);
         currentScreen = MAIN_MENU;
     }
 }

 void updateCredits(void) {
     // Atualização da lógica dos créditos, se necessário
 }



