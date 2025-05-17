#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <stdbool.h> // Adicionado para tipo bool

// Estados do jogo
typedef enum {
    LOADING_SCREEN = 0,
    MAIN_MENU = 1,
    OPPONENT_SELECTION = 2,
    MONSTER_SELECTION = 3,
    BATTLE_SCREEN = 4,
    TYPES_TABLE = 5,
    SETTINGS = 6,
    CREDITS = 7,
    EXIT = 8
} GameState;

// Declaração de variáveis externas
extern GameState currentScreen;
extern bool gameRunning;
extern bool vsBot;
extern bool playerTurn;

#endif // GAME_STATE_H