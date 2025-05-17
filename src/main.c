// main.c - VERSÃO CORRIGIDA COM TELA DE CARREGAMENTO
#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #define NOGDI
    #define NOUSER
#endif

#include "raygui.h"
#include <stdio.h>
#include <stdlib.h>
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
#include "render/loading_renderer.h"
#include "types_table_renderer.h"
#include "render/credits_renderer.h"
#include "main.h"
#include "resources.h"
#include "gui.h"

// Etapas de carregamento do jogo
typedef enum {
    LOAD_INIT = 0,
    LOAD_TEXTURES,
    LOAD_SOUNDS,
    LOAD_MONSTER_DB,
    LOAD_MONSTER_SPRITES,
    LOAD_BATTLE_SYSTEM,
    LOAD_IA,
    LOAD_COMPLETE
} LoadingStage;

static LoadingStage currentLoadingStage = LOAD_INIT;
static int totalLoadingSteps = 7;  // Número total de etapas
static float loadProgress = 0.0f;
static bool aiInitialized = false;
static int aiInitAttempts = 0;
static double aiInitLastTime = 0.0;
static const double AI_RETRY_DELAY = 1.0; // 1 segundo entre tentativas

// Função auxiliar para avançar o progresso de carregamento
void advanceLoadingProgress(void) {
    currentLoadingStage++;
    loadProgress = (float)currentLoadingStage / totalLoadingSteps;
}

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
    const int screenWidth = 1280;
    const int screenHeight = 720;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);

    InitWindow(screenWidth, screenHeight, "PokeBattle");
    InitAudioDevice();
    SetTargetFPS(60);

    // Inicializando o gerador de números aleatórios
    srand(time(NULL));

    // Inicializar recursos globais básicos
    initializeGlobals();

    // Definir estado inicial como tela de carregamento
    currentScreen = LOADING_SCREEN;
    currentLoadingStage = LOAD_INIT;
    loadProgress = 0.0f;

    // Inicializar tela de carregamento
    initLoadingScreen();

    // Loop principal do jogo
    while (!WindowShouldClose() && gameRunning) {
        // Atualização
        if (currentScreen == LOADING_SCREEN) {
            // Lógica de carregamento progressivo
            switch (currentLoadingStage) {
                case LOAD_INIT:
                    printf("Iniciando carregamento do jogo...\n");
                    advanceLoadingProgress();
                    break;

                case LOAD_TEXTURES:
                    printf("Carregando texturas...\n");
                    loadTextures();
                    advanceLoadingProgress();
                    break;

                case LOAD_SOUNDS:
                    printf("Carregando sons...\n");
                    loadSounds(musicVolume, soundVolume);
                    advanceLoadingProgress();
                    break;

                case LOAD_MONSTER_DB:
                    printf("Inicializando banco de monstros...\n");
                    initializeMonsterDatabase();
                    advanceLoadingProgress();
                    break;

                case LOAD_MONSTER_SPRITES:
                    printf("Carregando sprites dos monstros...\n");
                    verifyMonsterSprites();
                    loadMonsterTextures();
                    advanceLoadingProgress();
                    break;

                case LOAD_BATTLE_SYSTEM:
                    printf("Inicializando sistema de batalha...\n");
                    initializeBattleSystem();
                    initBattleEffects();
                    LoadPokemonTheme(); // Carregar tema antes de inicializar IA
                    advanceLoadingProgress();
                    break;

                case LOAD_IA:
                    // Lógica melhorada para inicialização da IA
                    if (!aiInitialized) {
                        double currentTime = GetTime();

                        // Controlar tempo entre tentativas
                        if (aiInitAttempts == 0 || currentTime - aiInitLastTime >= AI_RETRY_DELAY) {
                            aiInitAttempts++;
                            aiInitLastTime = currentTime;

                            printf("Tentativa %d de inicializar IA...\n", aiInitAttempts);

                            // Temos 3 abordagens possíveis:
                            // 1. Criar uma função de pré-inicialização que não faz conexão completa ainda
                            // 2. Usar a função existente mas com espera significativa
                            // 3. Mover a inicialização para depois de carregar tudo
                            // Vamos usar a abordagem 2:

                            curl_global_init(CURL_GLOBAL_ALL); // Garantir que libcurl esteja inicializada

                            // Tenta inicializar com tempo suficiente
                            aiInitialized = initializeAI();

                            if (aiInitialized) {
                                printf("✓ IA inicializada com sucesso na tentativa %d!\n", aiInitAttempts);

                                // Testar conexão após inicialização bem-sucedida
                                printf("\n=== TESTANDO CONEXÃO COM IA ===\n");
                                if (testAIConnection()) {
                                    printf("✓ Conexão com IA estabelecida com sucesso!\n");
                                }
                                printf("==============================\n\n");

                                // Avançar para a próxima etapa
                                advanceLoadingProgress();
                            } else if (aiInitAttempts >= 3) {
                                // Desistir após 3 tentativas
                                printf("\n=== IA INDISPONÍVEL APÓS 3 TENTATIVAS ===\n");
                                printf("Usando sistema de IA local (simples) para o bot.\n");
                                printf("======================\n\n");

                                // Avançar mesmo sem IA
                                advanceLoadingProgress();
                            }
                        }
                    }
                    break;

                case LOAD_COMPLETE:
                    // Verificar conclusão do carregamento
                    if (updateLoadingScreen()) {
                        // Completar inicialização
                        gameInitialized = true;

                        // Avançar para o menu principal
                        currentScreen = MAIN_MENU;
                        PlayMusicStream(menuMusic);

                        // Descarregar recursos da tela de carregamento
                        unloadLoadingScreen();
                    }
                    break;
            }
        } else {
            // Lógica normal do jogo
            updateGame();
        }

        // Renderização
        BeginDrawing();

        if (currentScreen == LOADING_SCREEN) {
            // Desenhar tela de carregamento
            drawLoadingScreen(loadProgress);
        } else {
            // Lógica de renderização normal
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
        }

        EndDrawing();
    }

     // Limpeza e encerramento
     cleanupGame();
     cleanupGlobals();
     CloseAudioDevice();
     CloseWindow();

     return 0;
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
    UnloadPokemonTheme();

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