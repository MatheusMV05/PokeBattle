#ifndef SETTINGS_RENDERER_H
#define SETTINGS_RENDERER_H

#include "raylib.h"

// Funções para renderização de configurações
void drawSettings(void);
void updateSettings(void);
void initializeSettings(void);
void applySettings(void);
void detectCurrentResolution(void);

#endif