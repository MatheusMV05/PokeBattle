#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdbool.h> // Adicionado para tipo bool
#include "game_state.h"
#include "raylib.h"
#include "battle.h"
#include "structures.h"
#include <curl/curl.h>

// Definições para evitar conflitos com Windows
#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #define NOGDI
    #define NOUSER
#endif

// Declaração CURL sem conflitos

typedef void CURL;  // Forward declaration simplificada para evitar conflitos


// Variáveis de estado do jogo
extern GameState currentScreen;
extern bool gameRunning;
extern bool vsBot;
extern bool playerTurn;
extern bool gameInitialized;

// Sistema de batalha
extern BattleSystem* battleSystem;
extern bool actionQueueReady;
extern char battleMessage[256];
extern BattleMessage currentMessage;
extern BattleAnimation currentAnimation;
extern float stateTransitionDelay;
extern MessageSequence currentSequence;

// Variáveis de configuração
extern bool fullscreen;
extern float musicVolume;
extern float soundVolume;
extern int currentResolutionIndex;
extern int pendingResolutionIndex;
extern bool hasUnsavedChanges;
extern float pendingMusicVolume;
extern float pendingSoundVolume;
extern bool pendingFullscreen;
extern int pendingDifficultyIndex;
extern int pendingAnimSpeedIndex;

// Integração com IA
extern bool initialized;
extern CURL* curl_handle;

// Teams
extern MonsterList* playerTeam;
extern MonsterList* opponentTeam;

// Recursos visuais e sonoros
extern Font gameFont;
extern Texture2D backgroundTexture;
extern Texture2D menuBackground;
extern Texture2D battleBackground;
extern Texture2D monsterSelectBackground;
extern Texture2D typeIcons[TYPE_COUNT];
extern Sound selectSound;
extern Sound attackSound;
extern Sound hitSound;
extern Sound faintSound;
extern Music menuMusic;
extern Music battleMusic;
extern int currentBattleBackground;
extern BattleSystem* battleSystem;
extern bool actionQueueReady;
extern char battleMessage[256];
extern BattleMessage currentMessage;
extern BattleAnimation currentAnimation;
extern float stateTransitionDelay;
extern char errorBuffer[256];

// Funções de inicialização/finalização globais
void initializeGlobals(void);
void cleanupGlobals(void);

#endif // GLOBALS_H