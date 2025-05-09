#ifndef BATTLE_RENDERER_H
#define BATTLE_RENDERER_H

#include "raylib.h"
#include "../structures.h"
#include "../battle.h"

// Constantes e estruturas para efeitos
#define MAX_EFFECTS 10

// Tipos de efeito
enum {
    EFFECT_NONE = 0,
    EFFECT_FLASH,
    EFFECT_SHAKE,
    EFFECT_PARTICLES,
    EFFECT_SLASH,
    EFFECT_FIRE,
    EFFECT_WATER,
    EFFECT_ELECTRIC,
    EFFECT_NATURE
};

// Estrutura para efeitos visuais na batalha
typedef struct {
    bool active;
    float timer;
    float duration;
    int type;
    Rectangle bounds;
    Color color;
    Vector2 origin;
    Vector2 target;
} BattleEffect;

// Funções para renderização da batalha
void drawBattleScreen(void);
void drawMonsterInBattle(PokeMonster* monster, Rectangle bounds, bool isPlayer);
void drawBattleHUD(void);
void updateBattleScreen(void);
void initBattleEffects(void);
void updateBattleEffects(void);
void drawBattleEffects(void);
void createBattleEffect(int type, Rectangle bounds, Color color, Vector2 origin, Vector2 target, float duration);
void createAttackEffect(MonsterType attackType, Vector2 origin, Vector2 target);
void executeAttackWithEffects(PokeMonster* attacker, PokeMonster* defender, int attackIndex);

#endif