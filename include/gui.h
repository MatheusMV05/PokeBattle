#ifndef GUI_H
#define GUI_H

#include "raylib.h"
#include "raygui.h"

// Estilos e temas
void LoadPokemonTheme(void);
void UnloadPokemonTheme(void);

// Componentes de interface
bool GuiPokemonButton(Rectangle bounds, const char *text, bool active);
void GuiPokemonStatusBar(Rectangle bounds, int value, int maxValue, const char *text, Color color);
void GuiPokemonMessageBox(Rectangle bounds, const char *message);
void GuiPokemonTypeIcon(Rectangle bounds, int typeIndex);
void GuiPokemonMainMenu(Rectangle bounds, int *currentScreen);
void GuiPokemonBattleMenu(Rectangle bounds, int *currentOption);
bool GuiPokemonDialog(Rectangle bounds, const char *title, const char *message, const char *buttons);

#endif // GUI_H