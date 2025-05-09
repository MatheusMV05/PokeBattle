#ifndef HUD_ELEMENTS_H
#define HUD_ELEMENTS_H

#include "raylib.h"
#include "../structures.h"

// Elementos de interface compartilhados
bool drawButton(Rectangle bounds, const char* text, Color color);
void drawMonsterCard(Rectangle bounds, PokeMonster* monster, bool selected);
void drawTypeIcon(Rectangle bounds, MonsterType type);
void drawHealthBar(Rectangle bounds, int currentHP, int maxHP, PokeMonster* monster);
void drawMonsterStats(Rectangle bounds, PokeMonster* monster);
void drawAttackList(Rectangle bounds, PokeMonster* monster, int selectedAttack);
void drawMessageBox(Rectangle bounds, const char* message);
void drawConfirmationDialog(const char* message, const char* yesText, const char* noText);
void drawAIIndicator(void);

#endif