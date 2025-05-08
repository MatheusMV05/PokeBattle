/**
 * PokeBattle - Sistema de game states
 * 
 * Este arquivo contém as declarações para o sistema de gamestates.
 */

#ifndef GAME_STATE_H
#define GAME_STATE_H

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

// Declaração de variáveis externas
extern GameState currentScreen;
extern bool gameRunning;
extern bool vsBot;
extern bool playerTurn;

#endif // GAME_STATE_H