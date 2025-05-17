#ifndef LOADING_RENDERER_H
#define LOADING_RENDERER_H

#include "raylib.h"

// Funções para renderização da tela de carregamento
void initLoadingScreen(void);
void drawLoadingScreen(float progress);
bool updateLoadingScreen(void);
void unloadLoadingScreen(void);

#endif // LOADING_RENDERER_H