// battle_effects.h
#ifndef BATTLE_EFFECTS_H
#define BATTLE_EFFECTS_H

#include "raylib.h"
#include "structures.h"
#include <stdbool.h>

// Tipos de efeitos expandidos
typedef enum {
    EFFECT_NONE = 0,
    EFFECT_ATTACK_NORMAL,
    EFFECT_ATTACK_FIRE,
    EFFECT_ATTACK_WATER,
    EFFECT_ATTACK_GRASS,
    EFFECT_ATTACK_ELECTRIC,
    EFFECT_ATTACK_ICE,
    EFFECT_ATTACK_FIGHTING,
    EFFECT_ATTACK_POISON,
    EFFECT_ATTACK_GROUND,
    EFFECT_ATTACK_FLYING,
    EFFECT_ATTACK_PSYCHIC,
    EFFECT_ATTACK_BUG,
    EFFECT_ATTACK_ROCK,
    EFFECT_ATTACK_GHOST,
    EFFECT_ATTACK_DRAGON,
    EFFECT_ATTACK_DARK,
    EFFECT_ATTACK_STEEL,
    EFFECT_ATTACK_FAIRY,
    EFFECT_DAMAGE_PLAYER,
    EFFECT_DAMAGE_OPPONENT,
    EFFECT_HEAL,
    EFFECT_STATUS_APPLIED,
    EFFECT_FAINT,
    EFFECT_CRITICAL_HIT,
    EFFECT_TYPE_COUNT
} EffectType;

// Estrutura expandida para partículas
typedef struct {
    Vector2 position;
    Vector2 velocity;
    Vector2 acceleration;
    float size;
    float life;
    float maxLife;
    Color color;
    Color targetColor;
    float rotation;
    float rotationSpeed;
    bool active;
    int type; // Tipo específico da partícula
} Particle;

// Estrutura para efeito de batalha completo
typedef struct {
    bool active;
    EffectType type;
    float timer;
    float duration;
    Vector2 origin;
    Vector2 target;
    Vector2 currentPos;
    Rectangle bounds;
    Color color;

    // Propriedades para animação
    float intensity;
    float scale;
    float rotation;
    float alpha;

    // Sistema de partículas
    Particle particles[50];
    int particleCount;

    // Propriedades específicas do efeito
    bool isPlayerTarget;
    float shakeIntensity;
    float flashIntensity;

    // Callback quando o efeito termina
    void (*onComplete)(void);
} BattleEffectSystem;

// Configurações globais de efeitos
#define MAX_BATTLE_EFFECTS 20
#define MAX_PARTICLES_PER_EFFECT 50

// Sistema principal de efeitos
extern BattleEffectSystem battleEffects[MAX_BATTLE_EFFECTS];
extern int activeEffectCount;

// Variáveis para efeitos de tela
extern float screenShakeTimer;
extern float screenShakeIntensity;
extern Vector2 screenShakeOffset;

extern float screenFlashTimer;
extern float screenFlashIntensity;
extern Color screenFlashColor;

// Funções principais
void InitBattleEffectsSystem(void);
void UpdateBattleEffects(void);
void DrawBattleEffects(void);
void ClearAllBattleEffects(void);

// Funções para criar efeitos específicos
void CreateAttackEffect(MonsterType attackType, Vector2 origin, Vector2 target, bool isPlayerAttacking);
void CreateDamageEffect(Vector2 position, int damage, bool isPlayerTarget, bool isCritical);
void CreateHealEffect(Vector2 position, int healAmount);
void CreateStatusEffect(Vector2 position, int statusType);
void CreateFaintEffect(Vector2 position);

// Efeitos de tela
void TriggerScreenShake(float intensity, float duration);
void TriggerScreenFlash(Color color, float intensity, float duration);

// Funções auxiliares para partículas
void InitParticle(Particle* particle, Vector2 pos, Vector2 vel, Color color, float life);
void UpdateParticle(Particle* particle);
void DrawParticle(Particle* particle);

// Funções para efeitos específicos por tipo
void CreateFireEffect(Vector2 origin, Vector2 target);
void CreateWaterEffect(Vector2 origin, Vector2 target);
void CreateGrassEffect(Vector2 origin, Vector2 target);
void CreateElectricEffect(Vector2 origin, Vector2 target);
void CreateIceEffect(Vector2 origin, Vector2 target);
void CreateFightingEffect(Vector2 origin, Vector2 target);
void CreatePoisonEffect(Vector2 origin, Vector2 target);
void CreateGroundEffect(Vector2 origin, Vector2 target);
void CreateFlyingEffect(Vector2 origin, Vector2 target);
void CreatePsychicEffect(Vector2 origin, Vector2 target);
void CreateBugEffect(Vector2 origin, Vector2 target);
void CreateRockEffect(Vector2 origin, Vector2 target);
void CreateGhostEffect(Vector2 origin, Vector2 target);
void CreateDragonEffect(Vector2 origin, Vector2 target);
void CreateDarkEffect(Vector2 origin, Vector2 target);
void CreateSteelEffect(Vector2 origin, Vector2 target);
void CreateFairyEffect(Vector2 origin, Vector2 target);
void CreateNormalEffect(Vector2 origin, Vector2 target);

// Funções utilitárias
Vector2 LerpVector2(Vector2 start, Vector2 end, float t);
Color LerpColor(Color start, Color end, float t);
float EaseInOut(float t);
float EaseOut(float t);
float EaseIn(float t);

// Funções de renderização para efeitos básicos
void DrawDamageNumbers(BattleEffectSystem* effect);
void DrawHealEffect(BattleEffectSystem* effect);
void DrawStatusEffect(BattleEffectSystem* effect);
void DrawFaintEffect(BattleEffectSystem* effect);
void DrawCriticalEffect(BattleEffectSystem* effect);

// Funções de desenho específicas para cada tipo de ataque
void DrawFireAttackEffect(BattleEffectSystem* effect);
void DrawWaterAttackEffect(BattleEffectSystem* effect);
void DrawElectricAttackEffect(BattleEffectSystem* effect);
void DrawGrassAttackEffect(BattleEffectSystem* effect);
void DrawIceAttackEffect(BattleEffectSystem* effect);
void DrawFightingAttackEffect(BattleEffectSystem* effect);
void DrawPoisonAttackEffect(BattleEffectSystem* effect);
void DrawGroundAttackEffect(BattleEffectSystem* effect);
void DrawFlyingAttackEffect(BattleEffectSystem* effect);
void DrawPsychicAttackEffect(BattleEffectSystem* effect);
void DrawBugAttackEffect(BattleEffectSystem* effect);
void DrawRockAttackEffect(BattleEffectSystem* effect);
void DrawGhostAttackEffect(BattleEffectSystem* effect);
void DrawDragonAttackEffect(BattleEffectSystem* effect);
void DrawDarkAttackEffect(BattleEffectSystem* effect);
void DrawSteelAttackEffect(BattleEffectSystem* effect);
void DrawFairyAttackEffect(BattleEffectSystem* effect);
void DrawNormalAttackEffect(BattleEffectSystem* effect);

// Função para efeitos contínuos de status
void CreateContinuousStatusEffect(Vector2 position, int statusType, float duration);

void TriggerPokemonShake(bool isPlayerPokemon, float intensity, float duration);
void UpdatePokemonShakes(void);

#endif // BATTLE_EFFECTS_H