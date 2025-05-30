// battle_effects.c
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#endif


#include "monsters.h"
#include "battle_effects.h"
#include "resources.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

// Variáveis globais do sistema de efeitos
BattleEffectSystem battleEffects[MAX_BATTLE_EFFECTS] = {0};
int activeEffectCount = 0;

// Variáveis para efeitos de tela
float screenShakeTimer = 0.0f;
float screenShakeIntensity = 0.0f;
Vector2 screenShakeOffset = {0, 0};

float screenFlashTimer = 0.0f;
float screenFlashIntensity = 0.0f;
Color screenFlashColor = WHITE;

// Inicializa o sistema de efeitos
void InitBattleEffectsSystem(void) {
    printf("Inicializando sistema de efeitos de batalha...\n");

    // Limpar todos os efeitos
    for (int i = 0; i < MAX_BATTLE_EFFECTS; i++) {
        battleEffects[i].active = false;
        battleEffects[i].type = EFFECT_NONE;
        battleEffects[i].particleCount = 0;

        // Limpar partículas
        for (int j = 0; j < MAX_PARTICLES_PER_EFFECT; j++) {
            battleEffects[i].particles[j].active = false;
        }
    }

    activeEffectCount = 0;

    // Resetar efeitos de tela
    screenShakeTimer = 0.0f;
    screenShakeIntensity = 0.0f;
    screenShakeOffset = (Vector2){0, 0};

    screenFlashTimer = 0.0f;
    screenFlashIntensity = 0.0f;
    screenFlashColor = WHITE;

    printf("Sistema de efeitos inicializado com sucesso!\n");
}

// Atualiza todos os efeitos ativos
void UpdateBattleEffects(void) {
    float deltaTime = GetFrameTime();

    // Atualizar efeitos de tela
    if (screenShakeTimer > 0.0f) {
        screenShakeTimer -= deltaTime;
        float intensity = screenShakeIntensity * (screenShakeTimer / 1.0f);
        screenShakeOffset.x = (float)(rand() % 21 - 10) * intensity * 0.1f;
        screenShakeOffset.y = (float)(rand() % 21 - 10) * intensity * 0.1f;

        if (screenShakeTimer <= 0.0f) {
            screenShakeOffset = (Vector2){0, 0};
        }
    }

    if (screenFlashTimer > 0.0f) {
        screenFlashTimer -= deltaTime;
        screenFlashIntensity = screenFlashTimer / 0.3f;

        if (screenFlashTimer <= 0.0f) {
            screenFlashIntensity = 0.0f;
        }
    }

    // Atualizar efeitos individuais
    for (int i = 0; i < MAX_BATTLE_EFFECTS; i++) {
        if (!battleEffects[i].active) continue;

        BattleEffectSystem* effect = &battleEffects[i];
        effect->timer += deltaTime;

        // Calcular progresso (0.0 - 1.0)
        float progress = effect->timer / effect->duration;
        if (progress > 1.0f) progress = 1.0f;

        // Atualizar propriedades baseadas no progresso
        effect->alpha = 1.0f - EaseOut(progress);
        effect->intensity = 1.0f - EaseInOut(progress);

        // Atualizar posição (para efeitos que se movem)
        if (effect->type >= EFFECT_ATTACK_NORMAL && effect->type <= EFFECT_ATTACK_FAIRY) {
            effect->currentPos = LerpVector2(effect->origin, effect->target, EaseOut(progress));
        }

        // Atualizar partículas
        for (int j = 0; j < effect->particleCount; j++) {
            if (effect->particles[j].active) {
                UpdateParticle(&effect->particles[j]);
            }
        }

        // Verificar se o efeito terminou
        if (effect->timer >= effect->duration) {
            if (effect->onComplete) {
                effect->onComplete();
            }
            effect->active = false;
            activeEffectCount--;
        }
    }
}

// Desenha todos os efeitos ativos
void DrawBattleEffects(void) {
    // Aplicar offset de shake na tela
    if (screenShakeIntensity > 0.0f) {
        // O shake será aplicado externamente usando rlPushMatrix/rlPopMatrix
        // Aqui apenas disponibilizamos o offset
    }

    // Desenhar efeitos individuais
    for (int i = 0; i < MAX_BATTLE_EFFECTS; i++) {
        if (!battleEffects[i].active) continue;

        BattleEffectSystem* effect = &battleEffects[i];

        // Desenhar partículas
        for (int j = 0; j < effect->particleCount; j++) {
            if (effect->particles[j].active) {
                DrawParticle(&effect->particles[j]);
            }
        }

        // Desenhar efeitos específicos baseados no tipo
        switch (effect->type) {
            case EFFECT_DAMAGE_PLAYER:
            case EFFECT_DAMAGE_OPPONENT:
                DrawDamageNumbers(effect);
                break;

            case EFFECT_HEAL:
                DrawHealEffect(effect);
                break;

            case EFFECT_STATUS_APPLIED:
                DrawStatusEffect(effect);
                break;

            case EFFECT_FAINT:
                DrawFaintEffect(effect);
                break;

            case EFFECT_CRITICAL_HIT:
                DrawCriticalEffect(effect);
                break;

            default:
                // Efeitos de ataque são principalmente partículas
                break;
        }
    }

    // Desenhar flash de tela por último
    if (screenFlashIntensity > 0.0f) {
        Color flashColor = screenFlashColor;
        flashColor.a = (unsigned char)(255 * screenFlashIntensity);
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), flashColor);
    }
}

// Limpa todos os efeitos ativos
void ClearAllBattleEffects(void) {
    for (int i = 0; i < MAX_BATTLE_EFFECTS; i++) {
        battleEffects[i].active = false;
    }
    activeEffectCount = 0;

    screenShakeTimer = 0.0f;
    screenFlashTimer = 0.0f;
}

// Encontra um slot livre para um novo efeito
int FindFreeEffectSlot(void) {
    for (int i = 0; i < MAX_BATTLE_EFFECTS; i++) {
        if (!battleEffects[i].active) {
            return i;
        }
    }
    return -1; // Nenhum slot livre
}

// Cria um efeito de ataque baseado no tipo
void CreateAttackEffect(MonsterType attackType, Vector2 origin, Vector2 target, bool isPlayerAttacking) {
    printf("Criando efeito de ataque tipo %d\n", attackType);

    // Tocar som de ataque
    PlaySound(attackSound);

    // Criar efeito visual baseado no tipo
    switch (attackType) {
        case TYPE_FIRE:
            CreateFireEffect(origin, target);
            TriggerScreenFlash((Color){255, 100, 50, 100}, 0.8f, 0.2f);
            break;
        case TYPE_WATER:
            CreateWaterEffect(origin, target);
            TriggerScreenFlash((Color){50, 150, 255, 80}, 0.6f, 0.25f);
            break;
        case TYPE_GRASS:
            CreateGrassEffect(origin, target);
            TriggerScreenFlash((Color){100, 255, 100, 70}, 0.5f, 0.2f);
            break;
        case TYPE_ELECTRIC:
            CreateElectricEffect(origin, target);
            TriggerScreenFlash((Color){255, 255, 100, 120}, 1.0f, 0.15f);
            TriggerScreenShake(8.0f, 0.3f);
            break;
        case TYPE_ICE:
            CreateIceEffect(origin, target);
            TriggerScreenFlash((Color){150, 200, 255, 90}, 0.7f, 0.3f);
            break;
        case TYPE_FIGHTING:
            CreateFightingEffect(origin, target);
            TriggerScreenShake(6.0f, 0.4f);
            break;
        case TYPE_POISON:
            CreatePoisonEffect(origin, target);
            TriggerScreenFlash((Color){150, 50, 150, 85}, 0.6f, 0.25f);
            break;
        case TYPE_GROUND:
            CreateGroundEffect(origin, target);
            TriggerScreenShake(10.0f, 0.5f);
            break;
        case TYPE_FLYING:
            CreateFlyingEffect(origin, target);
            break;
        case TYPE_PSYCHIC:
            CreatePsychicEffect(origin, target);
            TriggerScreenFlash((Color){255, 100, 255, 90}, 0.8f, 0.4f);
            break;
        case TYPE_BUG:
            CreateBugEffect(origin, target);
            break;
        case TYPE_ROCK:
            CreateRockEffect(origin, target);
            TriggerScreenShake(8.0f, 0.4f);
            break;
        case TYPE_GHOST:
            CreateGhostEffect(origin, target);
            TriggerScreenFlash((Color){100, 50, 200, 100}, 0.9f, 0.3f);
            break;
        case TYPE_DRAGON:
            CreateDragonEffect(origin, target);
            TriggerScreenFlash((Color){200, 100, 255, 110}, 1.0f, 0.3f);
            TriggerScreenShake(7.0f, 0.4f);
            break;
        case TYPE_DARK:
            CreateDarkEffect(origin, target);
            TriggerScreenFlash((Color){50, 50, 50, 120}, 0.8f, 0.4f);
            break;
        case TYPE_STEEL:
            CreateSteelEffect(origin, target);
            TriggerScreenFlash((Color){200, 200, 200, 100}, 0.7f, 0.2f);
            break;
        case TYPE_FAIRY:
            CreateFairyEffect(origin, target);
            TriggerScreenFlash((Color){255, 150, 200, 85}, 0.6f, 0.3f);
            break;
        default:
            CreateNormalEffect(origin, target);
            break;
    }
}

// Cria efeito de dano
void CreateDamageEffect(Vector2 position, int damage, bool isPlayerTarget, bool isCritical) {
    int slot = FindFreeEffectSlot();
    if (slot == -1) return;

    BattleEffectSystem* effect = &battleEffects[slot];
    effect->active = true;
    effect->type = isPlayerTarget ? EFFECT_DAMAGE_PLAYER : EFFECT_DAMAGE_OPPONENT;
    effect->timer = 0.0f;
    effect->duration = isCritical ? 1.5f : 1.0f;
    effect->origin = position;
    effect->target = (Vector2){position.x, position.y - 50};
    effect->currentPos = position;
    effect->intensity = isCritical ? 1.5f : 1.0f;
    effect->isPlayerTarget = isPlayerTarget;

    // Configurar cor baseada no tipo de dano
    if (isCritical) {
        effect->color = (Color){255, 255, 100, 255}; // Amarelo para crítico
        TriggerScreenFlash((Color){255, 255, 150, 150}, 1.0f, 0.3f);
        TriggerScreenShake(12.0f, 0.5f);
    } else {
        effect->color = isPlayerTarget ? (Color){255, 100, 100, 255} : (Color){255, 150, 150, 255};
        TriggerScreenFlash((Color){255, 200, 200, 100}, 0.6f, 0.2f);
        TriggerScreenShake(5.0f, 0.3f);
    }

    // Criar partículas de impacto
    for (int i = 0; i < 15; i++) {
        float angle = (float)(rand() % 360) * DEG2RAD;
        float speed = (float)(rand() % 100 + 50);
        Vector2 velocity = {cosf(angle) * speed, sinf(angle) * speed};

        Color particleColor = effect->color;
        particleColor.a = (unsigned char)(rand() % 100 + 155);

        InitParticle(&effect->particles[i], position, velocity, particleColor, 0.8f);
        effect->particleCount++;
    }

    activeEffectCount++;
    PlaySound(hitSound);

    printf("Efeito de dano criado: %d de dano, crítico: %s\n", damage, isCritical ? "sim" : "não");
}

// Implementações dos efeitos específicos por tipo

void CreateFireEffect(Vector2 origin, Vector2 target) {
    int slot = FindFreeEffectSlot();
    if (slot == -1) return;

    BattleEffectSystem* effect = &battleEffects[slot];
    effect->active = true;
    effect->type = EFFECT_ATTACK_FIRE;
    effect->timer = 0.0f;
    effect->duration = 1.2f;
    effect->origin = origin;
    effect->target = target;
    effect->currentPos = origin;
    effect->color = (Color){255, 100, 50, 255};

    // Criar partículas de fogo
    for (int i = 0; i < 25; i++) {
        Vector2 direction = {target.x - origin.x, target.y - origin.y};
        float length = sqrtf(direction.x * direction.x + direction.y * direction.y);
        direction.x /= length;
        direction.y /= length;

        float spread = 30.0f;
        Vector2 velocity = {
            direction.x * (200 + rand() % 100) + (rand() % (int)(spread * 2) - spread),
            direction.y * (200 + rand() % 100) + (rand() % (int)(spread * 2) - spread)
        };

        Color fireColor = (rand() % 2) ? (Color){255, 150, 50, 255} : (Color){255, 100, 0, 255};
        InitParticle(&effect->particles[i], origin, velocity, fireColor, 1.0f + (rand() % 50) * 0.01f);
        effect->particleCount++;
    }

    activeEffectCount++;
}

void CreateWaterEffect(Vector2 origin, Vector2 target) {
    int slot = FindFreeEffectSlot();
    if (slot == -1) return;

    BattleEffectSystem* effect = &battleEffects[slot];
    effect->active = true;
    effect->type = EFFECT_ATTACK_WATER;
    effect->timer = 0.0f;
    effect->duration = 1.0f;
    effect->origin = origin;
    effect->target = target;
    effect->currentPos = origin;
    effect->color = (Color){50, 150, 255, 255};

    // Criar partículas de água
    for (int i = 0; i < 20; i++) {
        Vector2 direction = {target.x - origin.x, target.y - origin.y};
        float length = sqrtf(direction.x * direction.x + direction.y * direction.y);
        direction.x /= length;
        direction.y /= length;

        Vector2 velocity = {
            direction.x * (180 + rand() % 80),
            direction.y * (180 + rand() % 80)
        };

        Color waterColor = (Color){50 + rand() % 100, 150 + rand() % 50, 255, 200 + rand() % 55};
        InitParticle(&effect->particles[i], origin, velocity, waterColor, 0.8f + (rand() % 40) * 0.01f);
        effect->particleCount++;
    }

    activeEffectCount++;
}

void CreateElectricEffect(Vector2 origin, Vector2 target) {
    int slot = FindFreeEffectSlot();
    if (slot == -1) return;

    BattleEffectSystem* effect = &battleEffects[slot];
    effect->active = true;
    effect->type = EFFECT_ATTACK_ELECTRIC;
    effect->timer = 0.0f;
    effect->duration = 0.8f;
    effect->origin = origin;
    effect->target = target;
    effect->currentPos = origin;
    effect->color = (Color){255, 255, 100, 255};

    // Criar raios elétricos (partículas em linha reta com variação)
    Vector2 direction = {target.x - origin.x, target.y - origin.y};
    float length = sqrtf(direction.x * direction.x + direction.y * direction.y);
    int segments = (int)(length / 20);

    for (int i = 0; i < segments && i < MAX_PARTICLES_PER_EFFECT; i++) {
        float t = (float)i / segments;
        Vector2 pos = {
            origin.x + direction.x * t + (rand() % 20 - 10),
            origin.y + direction.y * t + (rand() % 20 - 10)
        };

        Vector2 velocity = {(rand() % 40 - 20), (rand() % 40 - 20)};
        Color electricColor = (Color){255, 255, 100 + rand() % 155, 255};

        InitParticle(&effect->particles[i], pos, velocity, electricColor, 0.6f);
        effect->particleCount++;
    }

    activeEffectCount++;
}

void CreateGrassEffect(Vector2 origin, Vector2 target) {
    int slot = FindFreeEffectSlot();
    if (slot == -1) return;

    BattleEffectSystem* effect = &battleEffects[slot];
    effect->active = true;
    effect->type = EFFECT_ATTACK_GRASS;
    effect->timer = 0.0f;
    effect->duration = 1.3f;
    effect->origin = origin;
    effect->target = target;
    effect->currentPos = origin;
    effect->color = (Color){100, 255, 100, 255};

    // Criar folhas voando
    for (int i = 0; i < 18; i++) {
        Vector2 velocity = {
            (float)(rand() % 200 - 100),
            (float)(rand() % 150 - 200) // Movimento para cima
        };

        Color leafColor = (rand() % 2) ? (Color){100, 255, 100, 255} : (Color){50, 200, 50, 255};
        Vector2 startPos = {
            origin.x + (rand() % 80 - 40),
            origin.y + (rand() % 40 - 20)
        };

        InitParticle(&effect->particles[i], startPos, velocity, leafColor, 1.0f + (rand() % 60) * 0.01f);
        effect->particles[i].rotation = (float)(rand() % 360);
        effect->particles[i].rotationSpeed = (float)(rand() % 200 - 100);
        effect->particleCount++;
    }

    activeEffectCount++;
}

void CreateIceEffect(Vector2 origin, Vector2 target) {
    int slot = FindFreeEffectSlot();
    if (slot == -1) return;

    BattleEffectSystem* effect = &battleEffects[slot];
    effect->active = true;
    effect->type = EFFECT_ATTACK_ICE;
    effect->timer = 0.0f;
    effect->duration = 1.1f;
    effect->origin = origin;
    effect->target = target;
    effect->currentPos = origin;
    effect->color = (Color){150, 200, 255, 255};

    // Criar cristais de gelo
    for (int i = 0; i < 15; i++) {
        Vector2 direction = {target.x - origin.x, target.y - origin.y};
        float length = sqrtf(direction.x * direction.x + direction.y * direction.y);
        direction.x /= length;
        direction.y /= length;

        Vector2 velocity = {
            direction.x * (150 + rand() % 100),
            direction.y * (150 + rand() % 100)
        };

        Color iceColor = (Color){150 + rand() % 105, 200 + rand() % 55, 255, 200 + rand() % 55};
        InitParticle(&effect->particles[i], origin, velocity, iceColor, 0.9f + (rand() % 40) * 0.01f);
        effect->particleCount++;
    }

    activeEffectCount++;
}

void CreateFightingEffect(Vector2 origin, Vector2 target) {
    int slot = FindFreeEffectSlot();
    if (slot == -1) return;

    BattleEffectSystem* effect = &battleEffects[slot];
    effect->active = true;
    effect->type = EFFECT_ATTACK_FIGHTING;
    effect->timer = 0.0f;
    effect->duration = 0.8f;
    effect->origin = origin;
    effect->target = target;
    effect->currentPos = origin;
    effect->color = (Color){255, 150, 100, 255};

    // Criar efeito de impacto físico
    for (int i = 0; i < 12; i++) {
        float angle = (float)(rand() % 360) * DEG2RAD;
        Vector2 velocity = {
            cosf(angle) * (100 + rand() % 150),
            sinf(angle) * (100 + rand() % 150)
        };

        Color fightColor = (Color){255, 150 + rand() % 105, 100, 255};
        InitParticle(&effect->particles[i], target, velocity, fightColor, 0.6f);
        effect->particleCount++;
    }

    activeEffectCount++;
}

void CreatePoisonEffect(Vector2 origin, Vector2 target) {
    int slot = FindFreeEffectSlot();
    if (slot == -1) return;

    BattleEffectSystem* effect = &battleEffects[slot];
    effect->active = true;
    effect->type = EFFECT_ATTACK_POISON;
    effect->timer = 0.0f;
    effect->duration = 1.4f;
    effect->origin = origin;
    effect->target = target;
    effect->currentPos = origin;
    effect->color = (Color){150, 50, 150, 255};

    // Criar bolhas de veneno
    for (int i = 0; i < 20; i++) {
        Vector2 velocity = {
            (float)(rand() % 120 - 60),
            (float)(rand() % 100 - 150) // Movimento ascendente
        };

        Color poisonColor = (rand() % 2) ? (Color){150, 50, 150, 200} : (Color){100, 0, 100, 180};
        Vector2 startPos = {
            target.x + (rand() % 60 - 30),
            target.y + (rand() % 30)
        };

        InitParticle(&effect->particles[i], startPos, velocity, poisonColor, 1.2f + (rand() % 40) * 0.01f);
        effect->particleCount++;
    }

    activeEffectCount++;
}

void CreateGroundEffect(Vector2 origin, Vector2 target) {
    int slot = FindFreeEffectSlot();
    if (slot == -1) return;

    BattleEffectSystem* effect = &battleEffects[slot];
    effect->active = true;
    effect->type = EFFECT_ATTACK_GROUND;
    effect->timer = 0.0f;
    effect->duration = 1.0f;
    effect->origin = origin;
    effect->target = target;
    effect->currentPos = origin;
    effect->color = (Color){150, 100, 50, 255};

    // Criar partículas de terra e rochas
    for (int i = 0; i < 25; i++) {
        Vector2 velocity = {
            (float)(rand() % 200 - 100),
            (float)(rand() % 100 - 200)
        };

        Color earthColor = (rand() % 2) ? (Color){150, 100, 50, 255} : (Color){100, 70, 30, 255};
        Vector2 startPos = {
            target.x + (rand() % 100 - 50),
            target.y + (rand() % 20)
        };

        InitParticle(&effect->particles[i], startPos, velocity, earthColor, 0.8f + (rand() % 50) * 0.01f);
        effect->particleCount++;
    }

    activeEffectCount++;
}

void CreatePsychicEffect(Vector2 origin, Vector2 target) {
    int slot = FindFreeEffectSlot();
    if (slot == -1) return;

    BattleEffectSystem* effect = &battleEffects[slot];
    effect->active = true;
    effect->type = EFFECT_ATTACK_PSYCHIC;
    effect->timer = 0.0f;
    effect->duration = 1.6f;
    effect->origin = origin;
    effect->target = target;
    effect->currentPos = origin;
    effect->color = (Color){255, 100, 255, 255};

    // Criar ondas psíquicas em círculo
    for (int i = 0; i < 30; i++) {
        float angle = ((float)i / 30) * 2 * PI;
        Vector2 velocity = {
            cosf(angle) * 80,
            sinf(angle) * 80
        };

        Color psychicColor = (Color){255, 100 + rand() % 155, 255, 150 + rand() % 105};
        InitParticle(&effect->particles[i], target, velocity, psychicColor, 1.4f);
        effect->particleCount++;
    }

    activeEffectCount++;
}

// Implementações das demais funções específicas seguem padrão similar...
void CreateBugEffect(Vector2 origin, Vector2 target) {
    int slot = FindFreeEffectSlot();
    if (slot == -1) return;

    BattleEffectSystem* effect = &battleEffects[slot];
    effect->active = true;
    effect->type = EFFECT_ATTACK_BUG;
    effect->timer = 0.0f;
    effect->duration = 1.0f;
    effect->origin = origin;
    effect->target = target;
    effect->color = (Color){100, 200, 50, 255};

    // Criar pequenas partículas rápidas (insetos)
    for (int i = 0; i < 15; i++) {
        Vector2 velocity = {(float)(rand() % 300 - 150), (float)(rand() % 300 - 150)};
        Color bugColor = (Color){100 + rand() % 100, 200, 50 + rand() % 100, 255};
        InitParticle(&effect->particles[i], origin, velocity, bugColor, 0.8f);
        effect->particleCount++;
    }
    activeEffectCount++;
}

void CreateRockEffect(Vector2 origin, Vector2 target) {
    int slot = FindFreeEffectSlot();
    if (slot == -1) return;

    BattleEffectSystem* effect = &battleEffects[slot];
    effect->active = true;
    effect->type = EFFECT_ATTACK_ROCK;
    effect->timer = 0.0f;
    effect->duration = 1.2f;
    effect->origin = origin;
    effect->target = target;
    effect->color = (Color){150, 150, 100, 255};

    for (int i = 0; i < 20; i++) {
        Vector2 velocity = {(float)(rand() % 200 - 100), (float)(rand() % 150 - 200)};
        Color rockColor = (Color){150, 150, 100 + rand() % 50, 255};
        InitParticle(&effect->particles[i], target, velocity, rockColor, 1.0f);
        effect->particleCount++;
    }
    activeEffectCount++;
}

void CreateGhostEffect(Vector2 origin, Vector2 target) {
    int slot = FindFreeEffectSlot();
    if (slot == -1) return;

    BattleEffectSystem* effect = &battleEffects[slot];
    effect->active = true;
    effect->type = EFFECT_ATTACK_GHOST;
    effect->timer = 0.0f;
    effect->duration = 1.5f;
    effect->origin = origin;
    effect->target = target;
    effect->color = (Color){100, 50, 200, 180};

    for (int i = 0; i < 25; i++) {
        Vector2 velocity = {(float)(rand() % 120 - 60), (float)(rand() % 120 - 60)};
        Color ghostColor = (Color){100, 50 + rand() % 100, 200, 100 + rand() % 100};
        InitParticle(&effect->particles[i], target, velocity, ghostColor, 1.3f);
        effect->particleCount++;
    }
    activeEffectCount++;
}

void CreateDragonEffect(Vector2 origin, Vector2 target) {
    int slot = FindFreeEffectSlot();
    if (slot == -1) return;

    BattleEffectSystem* effect = &battleEffects[slot];
    effect->active = true;
    effect->type = EFFECT_ATTACK_DRAGON;
    effect->timer = 0.0f;
    effect->duration = 1.5f;
    effect->origin = origin;
    effect->target = target;
    effect->color = (Color){200, 100, 255, 255};

    for (int i = 0; i < 30; i++) {
        Vector2 velocity = {(float)(rand() % 250 - 125), (float)(rand() % 250 - 125)};
        Color dragonColor = (Color){200 + rand() % 55, 100 + rand() % 100, 255, 200 + rand() % 55};
        InitParticle(&effect->particles[i], target, velocity, dragonColor, 1.2f);
        effect->particleCount++;
    }
    activeEffectCount++;
}

void CreateDarkEffect(Vector2 origin, Vector2 target) {
    int slot = FindFreeEffectSlot();
    if (slot == -1) return;

    BattleEffectSystem* effect = &battleEffects[slot];
    effect->active = true;
    effect->type = EFFECT_ATTACK_DARK;
    effect->timer = 0.0f;
    effect->duration = 1.3f;
    effect->origin = origin;
    effect->target = target;
    effect->color = (Color){50, 50, 50, 200};

    for (int i = 0; i < 20; i++) {
        Vector2 velocity = {(float)(rand() % 180 - 90), (float)(rand() % 180 - 90)};
        Color darkColor = (Color){50 + rand() % 50, 50 + rand() % 50, 50 + rand() % 50, 150 + rand() % 105};
        InitParticle(&effect->particles[i], target, velocity, darkColor, 1.1f);
        effect->particleCount++;
    }
    activeEffectCount++;
}

void CreateSteelEffect(Vector2 origin, Vector2 target) {
    int slot = FindFreeEffectSlot();
    if (slot == -1) return;

    BattleEffectSystem* effect = &battleEffects[slot];
    effect->active = true;
    effect->type = EFFECT_ATTACK_STEEL;
    effect->timer = 0.0f;
    effect->duration = 1.0f;
    effect->origin = origin;
    effect->target = target;
    effect->color = (Color){200, 200, 200, 255};

    for (int i = 0; i < 18; i++) {
        Vector2 velocity = {(float)(rand() % 200 - 100), (float)(rand() % 200 - 100)};
        Color steelColor = (Color){200, 200, 200 + rand() % 55, 255};
        InitParticle(&effect->particles[i], target, velocity, steelColor, 0.8f);
        effect->particleCount++;
    }
    activeEffectCount++;
}

void CreateFairyEffect(Vector2 origin, Vector2 target) {
    int slot = FindFreeEffectSlot();
    if (slot == -1) return;

    BattleEffectSystem* effect = &battleEffects[slot];
    effect->active = true;
    effect->type = EFFECT_ATTACK_FAIRY;
    effect->timer = 0.0f;
    effect->duration = 1.4f;
    effect->origin = origin;
    effect->target = target;
    effect->color = (Color){255, 150, 200, 255};

    for (int i = 0; i < 25; i++) {
        Vector2 velocity = {(float)(rand() % 150 - 75), (float)(rand() % 150 - 75)};
        Color fairyColor = (Color){255, 150 + rand() % 105, 200 + rand() % 55, 200 + rand() % 55};
        InitParticle(&effect->particles[i], target, velocity, fairyColor, 1.2f);
        effect->particleCount++;
    }
    activeEffectCount++;
}

void CreateFlyingEffect(Vector2 origin, Vector2 target) {
    int slot = FindFreeEffectSlot();
    if (slot == -1) return;

    BattleEffectSystem* effect = &battleEffects[slot];
    effect->active = true;
    effect->type = EFFECT_ATTACK_FLYING;
    effect->timer = 0.0f;
    effect->duration = 1.0f;
    effect->origin = origin;
    effect->target = target;
    effect->color = (Color){200, 220, 255, 200};

    for (int i = 0; i < 16; i++) {
        Vector2 velocity = {(float)(rand() % 300 - 150), (float)(rand() % 200 - 250)};
        Color flyingColor = (Color){200 + rand() % 55, 220 + rand() % 35, 255, 150 + rand() % 105};
        InitParticle(&effect->particles[i], origin, velocity, flyingColor, 0.9f);
        effect->particleCount++;
    }
    activeEffectCount++;
}

void CreateNormalEffect(Vector2 origin, Vector2 target) {
    int slot = FindFreeEffectSlot();
    if (slot == -1) return;

    BattleEffectSystem* effect = &battleEffects[slot];
    effect->active = true;
    effect->type = EFFECT_ATTACK_NORMAL;
    effect->timer = 0.0f;
    effect->duration = 0.8f;
    effect->origin = origin;
    effect->target = target;
    effect->color = (Color){200, 200, 200, 255};

    for (int i = 0; i < 12; i++) {
        Vector2 velocity = {(float)(rand() % 180 - 90), (float)(rand() % 180 - 90)};
        Color normalColor = (Color){200 + rand() % 55, 200 + rand() % 55, 200 + rand() % 55, 255};
        InitParticle(&effect->particles[i], target, velocity, normalColor, 0.7f);
        effect->particleCount++;
    }
    activeEffectCount++;
}

// Efeitos de tela
void TriggerScreenShake(float intensity, float duration) {
    screenShakeIntensity = intensity;
    screenShakeTimer = duration;
    printf("Screen shake: intensidade %.1f por %.1f segundos\n", intensity, duration);
}

void TriggerScreenFlash(Color color, float intensity, float duration) {
    screenFlashColor = color;
    screenFlashIntensity = intensity;
    screenFlashTimer = duration;
    printf("Screen flash: cor (%d,%d,%d) intensidade %.1f por %.1f segundos\n",
           color.r, color.g, color.b, intensity, duration);
}

// Funções auxiliares para partículas
void InitParticle(Particle* particle, Vector2 pos, Vector2 vel, Color color, float life) {
    particle->position = pos;
    particle->velocity = vel;
    particle->acceleration = (Vector2){0, 50}; // Gravidade leve
    particle->size = (float)(rand() % 8 + 4);
    particle->life = 0.0f;
    particle->maxLife = life;
    particle->color = color;
    particle->targetColor = (Color){color.r, color.g, color.b, 0}; // Fade para transparente
    particle->rotation = 0.0f;
    particle->rotationSpeed = (float)(rand() % 200 - 100);
    particle->active = true;
    particle->type = 0;
}

void UpdateParticle(Particle* particle) {
    if (!particle->active) return;

    float deltaTime = GetFrameTime();

    // Atualizar física
    particle->velocity.x += particle->acceleration.x * deltaTime;
    particle->velocity.y += particle->acceleration.y * deltaTime;

    particle->position.x += particle->velocity.x * deltaTime;
    particle->position.y += particle->velocity.y * deltaTime;

    particle->rotation += particle->rotationSpeed * deltaTime;

    // Atualizar vida
    particle->life += deltaTime;

    if (particle->life >= particle->maxLife) {
        particle->active = false;
        return;
    }

    // Interpolar cor (fade out)
    float t = particle->life / particle->maxLife;
    particle->color = LerpColor(particle->color, particle->targetColor, EaseOut(t));

    // Reduzir tamanho gradualmente
    particle->size *= (1.0f - deltaTime * 0.5f);
    if (particle->size < 1.0f) particle->size = 1.0f;
}

void DrawParticle(Particle* particle) {
    if (!particle->active || particle->color.a < 10) return;

    // Desenhar partícula como círculo
    DrawCircleV(particle->position, particle->size, particle->color);

    // Adicionar brilho para alguns tipos
    if (particle->type == EFFECT_ATTACK_ELECTRIC) {
        Color glowColor = particle->color;
        glowColor.a = (unsigned char)(glowColor.a * 0.3f);
        DrawCircleV(particle->position, particle->size * 1.5f, glowColor);
    }
}

// Funções utilitárias
Vector2 LerpVector2(Vector2 start, Vector2 end, float t) {
    return (Vector2){
        start.x + (end.x - start.x) * t,
        start.y + (end.y - start.y) * t
    };
}

Color LerpColor(Color start, Color end, float t) {
    return (Color){
        (unsigned char)(start.r + (end.r - start.r) * t),
        (unsigned char)(start.g + (end.g - start.g) * t),
        (unsigned char)(start.b + (end.b - start.b) * t),
        (unsigned char)(start.a + (end.a - start.a) * t)
    };
}

float EaseInOut(float t) {
    return t * t * (3.0f - 2.0f * t);
}

float EaseOut(float t) {
    return 1.0f - (1.0f - t) * (1.0f - t);
}

float EaseIn(float t) {
    return t * t;
}

// Funções de renderização para efeitos específicos
void DrawDamageNumbers(BattleEffectSystem* effect) {
    // Esta seria implementada para mostrar números de dano flutuando
    if (effect->alpha < 0.1f) return;

    Color textColor = effect->color;
    textColor.a = (unsigned char)(255 * effect->alpha);

    char damageText[32] = "HIT!";
    int fontSize = (int)(24 * effect->intensity);

    DrawText(damageText,
             (int)(effect->currentPos.x - MeasureText(damageText, fontSize) / 2),
             (int)effect->currentPos.y,
             fontSize,
             textColor);
}

void DrawHealEffect(BattleEffectSystem* effect) {
    // Implementação para efeito de cura
    Color healColor = (Color){100, 255, 100, (unsigned char)(255 * effect->alpha)};
    DrawCircleV(effect->currentPos, 20 * effect->intensity, healColor);
}

void DrawStatusEffect(BattleEffectSystem* effect) {
    // Implementação para efeito de status
    Color statusColor = (Color){255, 255, 100, (unsigned char)(255 * effect->alpha)};
    DrawCircleLinesV(effect->currentPos, 25 * effect->intensity, statusColor);
}

void DrawFaintEffect(BattleEffectSystem* effect) {
    // Implementação para efeito de desmaiado
    Color faintColor = (Color){255, 100, 100, (unsigned char)(255 * effect->alpha)};
    DrawCircleV(effect->currentPos, 30 * effect->intensity, faintColor);
}

void DrawCriticalEffect(BattleEffectSystem* effect) {
    // Implementação para efeito de crítico
    Color critColor = (Color){255, 255, 100, (unsigned char)(255 * effect->alpha)};
    DrawText("CRITICAL!",
             (int)(effect->currentPos.x - 40),
             (int)(effect->currentPos.y - 20),
             20,
             critColor);
}