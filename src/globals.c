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
float musicVolume = 0.7f;
float soundVolume = 0.8f;


void initializeGlobals(void)
{
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

void cleanupGlobals(void)
{
    // Liberação de recursos
    if (playerTeam != NULL)
    {
        freeMonsterList(playerTeam);
        playerTeam = NULL;
    }

    if (opponentTeam != NULL)
    {
        freeMonsterList(opponentTeam);
        opponentTeam = NULL;
    }
}
