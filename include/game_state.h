#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <stdbool.h> // Adicionado para tipo bool

// Estados do jogo
typedef enum {
    MAIN_MENU = 0,
    OPPONENT_SELECTION = 1,
    MONSTER_SELECTION = 2,
    BATTLE_SCREEN = 3,
    TYPES_TABLE = 4,
    SETTINGS = 5,
    CREDITS = 6,
    EXIT = 7
} GameState;

// Declaração de variáveis externas
extern GameState currentScreen;
extern bool gameRunning;
extern bool vsBot;
extern bool playerTurn;

#endif // GAME_STATE_H