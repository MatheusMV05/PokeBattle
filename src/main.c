/**
 * PokeBattle - Um jogo de batalha por turnos inspirado em Pokémon
 * Autor: Claude
 * Data: Abril/2025
 * 
 * Este arquivo contém o ponto de entrada principal do jogo.
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <time.h>
 #include "raylib.h"
 #include "structures.h"
 #include "monsters.h"
 #include "battle.h"
 #include "ai_integration.h"
 #include "screens.h"
 
 // Estados do jogo
 typedef enum {
     MAIN_MENU = 0,
     OPPONENT_SELECTION,
     MONSTER_SELECTION,
     BATTLE_SCREEN,
     TYPES_TABLE,
     SETTINGS,
     CREDITS,
     EXIT
 } GameState;
 
 // Variáveis globais
 GameState currentScreen = MAIN_MENU;
 bool gameRunning = true;
 bool vsBot = true;
 bool playerTurn = true;
 bool gameInitialized = false;
 
 // Função principal
 int main(void) {
     // Inicialização do Raylib
     const int screenWidth = 800;
     const int screenHeight = 600;
     
     InitWindow(screenWidth, screenHeight, "PokeBattle");
     InitAudioDevice();
     SetTargetFPS(60);
     
     // Inicializando o gerador de números aleatórios
     srand(time(NULL));
     
     // Inicializar recursos do jogo
     initializeGame();
     
     // Loop principal do jogo
     while (!WindowShouldClose() && gameRunning) {
         // Atualização
         updateGame();
         
         // Renderização
         BeginDrawing();
         ClearBackground(RAYWHITE);
         
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
         }
         
         EndDrawing();
     }
     
     // Limpeza e encerramento
     cleanupGame();
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
     
     // Inicializar recursos visuais
     loadTextures();
     loadSounds();
     
     // Inicializar API de IA
     initializeAI();
     
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