#ifndef MAIN_H
#define MAIN_H

#include "game_state.h"

void initializeGame(void);
void updateGame(void);
void cleanupGame(void);
void changeScreen(GameState newScreen);

#endif