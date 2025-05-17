#ifndef BATTLE_RENDERER_H
#define BATTLE_RENDERER_H

#include "raylib.h"
#include "structures.h"
#include "../battle.h"

// Constantes e estruturas para efeitos
#define MAX_EFFECTS 10
// Configuração de estilo
#define UI_PANEL_COLOR     CLITERAL(Color){ 50, 70, 100, 255 }   // Azul escuro
#define UI_ACCENT_COLOR    CLITERAL(Color){ 230, 50, 70, 255 }   // Vermelho pokémon
#define UI_TEXT_COLOR      WHITE
#define HEALTH_BAR_WIDTH   200
#define HEALTH_BAR_HEIGHT  20

// Elementos de interface
typedef enum {
    BATTLE_BTN_FIGHT = 0,
    BATTLE_BTN_BAG,
    BATTLE_BTN_POKEMON,
    BATTLE_BTN_RUN,
    BATTLE_BTN_COUNT
} BattleButtons;


// Funções para renderização da batalha
void drawBattleScreen(void);
void drawMonsterInBattle(PokeMonster* monster, bool isPlayer);
void drawBattleHUD(void);
void updateBattleScreen(void);
void initBattleEffects(void);
void updateBattleEffects(void);
void drawBattleEffects(void);
void createBattleEffect(int type, Rectangle bounds, Color color, Vector2 origin, Vector2 target, float duration);
void createAttackEffect(MonsterType attackType, Vector2 origin, Vector2 target);
void executeAttackWithEffects(PokeMonster* attacker, PokeMonster* defender, int attackIndex);
void startTypewriter(const char* text, bool waitForInput);
void updateTypewriter(void);
void resetBattleSprites(void);
bool isTypewriterComplete(void);
bool isTypewriterWaitingInput(void);
void drawTypewriterText(Vector2 position, float fontSize, Color color);

#endif