#ifndef RESOURCES_H
#define RESOURCES_H

#include "raylib.h"
#include "../structures.h"

// Gerenciamento de recursos (texturas, sons, fontes)
void loadTextures(void);
void unloadTextures(void);
void loadSounds(float musicVol, float soundVol);
void unloadSounds(void);

#define BATTLE_BACKGROUNDS_COUNT 13
extern Texture2D battleBackgrounds[BATTLE_BACKGROUNDS_COUNT];
extern int currentBattleBackground;

// Variáveis globais de recursos
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

#endif