/**
 * PokeBattle - Implementação das telas do jogo
 * 
 * Este arquivo contém as implementações das funções para as diferentes telas do jogo.
 */
#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #define NOGDI
    #define NOUSER
#endif


 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include "raylib.h"
 #include "structures.h"
 #include "monsters.h"
 #include "battle.h"
 #include "ia_integration.h"
 #include "screens.h"
 #include <math.h>  // Para a função fmin
 #include "game_state.h"
 #include <stdbool.h>
 #include <curl/curl.h>
 
 // Variáveis externas do main.c
 extern BattleSystem* battleSystem;
 extern GameState currentScreen;
 extern bool gameRunning;
 extern bool vsBot;
 extern bool playerTurn;
 // Variáveis externas para tabela de tipos
 extern float typeEffectiveness[TYPE_COUNT][TYPE_COUNT];
 
 // Recursos visuais
 static Font gameFont;
 static Texture2D backgroundTexture;
 static Texture2D menuBackground;
 static Texture2D battleBackground;
 static Texture2D monsterSelectBackground;
 static Texture2D typeIcons[TYPE_COUNT];
 static Sound selectSound;
 static Sound attackSound;
 static Sound hitSound;
 static Sound faintSound;
 static Music menuMusic;
 static Music battleMusic;
 
 // Variáveis para a seleção de monstros
 static MonsterList* playerTeam = NULL;
 static MonsterList* opponentTeam = NULL;
 static int selectedMonsterIndex = 0;
 static int teamSelectionCount = 0;
 static bool viewingStats = false;
 static PokeMonster* currentViewedMonster = NULL;
 
 // Variáveis de configuração
 static bool fullscreen = false;
 static float musicVolume = 0.7f;
 static float soundVolume = 0.8f;
 
 // Funções auxiliares de desenho
 bool drawButton(Rectangle bounds, const char* text, Color color);
 void drawMonsterCard(Rectangle bounds, PokeMonster* monster, bool selected);
 void drawTypeIcon(Rectangle bounds, MonsterType type);
 void drawHealthBar(Rectangle bounds, int currentHP, int maxHP, PokeMonster* monster);
 void drawMonsterStats(Rectangle bounds, PokeMonster* monster);
 void drawAttackList(Rectangle bounds, PokeMonster* monster, int selectedAttack);
 void drawMessageBox(Rectangle bounds, const char* message);
 void drawBattleHUD(void);
 void drawConfirmationDialog(const char* message, const char* yesText, const char* noText);
 void drawMonsterInBattle(PokeMonster* monster, Rectangle bounds, bool isPlayer);
 
 static int currentResolutionIndex = 0;  
static int pendingResolutionIndex = 0;  
static bool hasUnsavedChanges = false;  

// Configurações pendentes
static float pendingMusicVolume = 0.7f;
static float pendingSoundVolume = 0.8f;
static bool pendingFullscreen = false;
static int pendingDifficultyIndex = 1;
static int pendingAnimSpeedIndex = 1;

// Estrutura para efeitos visuais na batalha
typedef struct {
    bool active;
    float timer;
    float duration;
    int type;
    Rectangle bounds;
    Color color;
    Vector2 origin;
    Vector2 target;
} BattleEffect;

// Variáveis para efeitos
#define MAX_EFFECTS 10
static BattleEffect effects[MAX_EFFECTS] = {0};

// Tipos de efeito
enum {
    EFFECT_NONE = 0,
    EFFECT_FLASH,
    EFFECT_SHAKE,
    EFFECT_PARTICLES,
    EFFECT_SLASH,
    EFFECT_FIRE,
    EFFECT_WATER,
    EFFECT_ELECTRIC,
    EFFECT_NATURE
};

// Estrutura para resoluções
typedef struct {
    int width;
    int height;
    const char* description;
} Resolution;

static Resolution availableResolutions[] = {
    { 1920, 1080, "1920x1080" },
    { 1600, 900,  "1600x900" },
    { 1366, 768,  "1366x768" },
    { 1280, 720,  "1280x720" },
    { 1024, 768,  "1024x768" }
};
static int numResolutions = 5;
 
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
 
 // Funções auxiliares de desenho
 
 

