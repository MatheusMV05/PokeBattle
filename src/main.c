// main.c - ATUALIZADO
#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #define NOGDI
    #define NOUSER
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "raylib.h"
#include "globals.h"
#include "monsters.h"
#include "battle.h"
#include "ia_integration.h"
#include "render/menu_renderer.h"
#include "render/battle_renderer.h"
#include "render/selection_renderer.h"
#include "render/settings_renderer.h"
#include "types_table_renderer.h" // Adicionado para resolver o drawTypesTable()
#include "render/credits_renderer.h"     // Adicionado para resolver o drawCredits()
#include "main.h"
#include "resources.h"


// Declarações das funções
void initializeGame(void);
void updateGame(void);
void cleanupGame(void);
void changeScreen(GameState newScreen);
void loadMonsterTextures(void);
void unloadMonsterTextures(void);
void initBattleEffects(void);
bool testAIConnection(void);

// Função principal
int main(void) {
    // Inicialização do Raylib
    const int screenWidth = 1920;
    const int screenHeight =  1080;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);

    InitWindow(screenWidth, screenHeight, "PokeBattle");
    InitAudioDevice();
    SetTargetFPS(60);

    // Inicializando o gerador de números aleatórios
    srand(time(NULL));

    // Inicializar recursos do jogo
    initializeGlobals();  // Inicialização das variáveis globais
    initializeGame();

    // Loop principal do jogo
    while (!WindowShouldClose() && gameRunning) {
        // Atualização
        updateGame();

        // Renderização
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // Debug para verificar o estado atual
        if (currentScreen == EXIT) {
            printf("[DEBUG] currentScreen é EXIT, saindo do jogo\n");
        }

        // Renderizar a tela atual
        switch (currentScreen) {
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
     cleanupGlobals();  // Limpeza das variáveis globais
     CloseAudioDevice();
     CloseWindow();

     return 0;
 }

 // Inicializa todos os recursos do jogo
 void initializeGame(void) {
    // Inicializar banco de monstros
    initializeMonsterDatabase();

    // Inicializar estruturas de batalha
    initializeBattleSystem();

    // Inicializar efeitos de batalha
    initBattleEffects();

    //Inicializar o menu de configurações
    initializeSettings();

    // Inicializar recursos visuais
    loadTextures();
    loadSounds(musicVolume, soundVolume);

    // Carregar texturas dos monstros
    loadMonsterTextures();

    // Inicializar API de IA
    if (initializeAI()) {
        printf("\n=== TESTANDO CONEXÃO COM IA ===\n");
        testAIConnection();
        printf("==============================\n\n");
    } else {
        printf("\n=== IA INDISPONÍVEL ===\n");
        printf("Usando sistema de IA local (simples) para o bot.\n");
        printf("======================\n\n");
    }

    gameInitialized = true;
}

 // Atualiza o estado do jogo
 void updateGame(void) {
     // Processamento específico para cada tela
     switch (currentScreen) {
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
 void cleanupGame(void) {
    // Descarregar texturas dos monstros
    unloadMonsterTextures();

    // Liberar banco de monstros
    freeMonsterDatabase();

    // Liberar estruturas de batalha
    freeBattleSystem();

    // Liberar recursos visuais
    unloadTextures();
    unloadSounds();

    // Encerrar API de IA
    shutdownAI();
}

 // Função para alterar a tela atual
 void changeScreen(GameState newScreen) {
     currentScreen = newScreen;
 }