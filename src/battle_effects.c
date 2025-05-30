// battle_effects.c - Sistema Completo de Efeitos de Batalha
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#endif

#include "battle_effects.h"
#include "monsters.h"
#include "resources.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "battle.h"

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

// Variáveis para efeitos de status contínuos
static float statusEffectTimer = 0.0f;
static bool statusEffectsActive[MAX_BATTLE_EFFECTS] = {false};

// Inicializa o sistema de efeitos
void InitBattleEffectsSystem(void) {
    printf("Inicializando sistema completo de efeitos de batalha...\n");

    // Limpar todos os efeitos
    for (int i = 0; i < MAX_BATTLE_EFFECTS; i++) {
        battleEffects[i].active = false;
        battleEffects[i].type = EFFECT_NONE;
        battleEffects[i].particleCount = 0;
        statusEffectsActive[i] = false;

        // Limpar partículas
        for (int j = 0; j < MAX_PARTICLES_PER_EFFECT; j++) {
            battleEffects[i].particles[j].active = false;
        }
    }

    activeEffectCount = 0;
    statusEffectTimer = 0.0f;

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
    statusEffectTimer += deltaTime;

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
        effect->scale = 1.0f + EaseOut(progress) * 0.5f;

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
            statusEffectsActive[i] = false;
            activeEffectCount--;
        }
    }
}

// Desenha todos os efeitos ativos
void DrawBattleEffects(void) {
    // Desenhar efeitos individuais
    for (int i = 0; i < MAX_BATTLE_EFFECTS; i++) {
        if (!battleEffects[i].active) continue;

        BattleEffectSystem* effect = &battleEffects[i];

        // Desenhar partículas primeiro
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

            // Efeitos de ataque específicos por tipo
            case EFFECT_ATTACK_FIRE:
                DrawFireAttackEffect(effect);
                break;
            case EFFECT_ATTACK_WATER:
                DrawWaterAttackEffect(effect);
                break;
            case EFFECT_ATTACK_ELECTRIC:
                DrawElectricAttackEffect(effect);
                break;
            case EFFECT_ATTACK_GRASS:
                DrawGrassAttackEffect(effect);
                break;
            case EFFECT_ATTACK_ICE:
                DrawIceAttackEffect(effect);
                break;
            case EFFECT_ATTACK_FIGHTING:
                DrawFightingAttackEffect(effect);
                break;
            case EFFECT_ATTACK_POISON:
                DrawPoisonAttackEffect(effect);
                break;
            case EFFECT_ATTACK_GROUND:
                DrawGroundAttackEffect(effect);
                break;
            case EFFECT_ATTACK_FLYING:
                DrawFlyingAttackEffect(effect);
                break;
            case EFFECT_ATTACK_PSYCHIC:
                DrawPsychicAttackEffect(effect);
                break;
            case EFFECT_ATTACK_BUG:
                DrawBugAttackEffect(effect);
                break;
            case EFFECT_ATTACK_ROCK:
                DrawRockAttackEffect(effect);
                break;
            case EFFECT_ATTACK_GHOST:
                DrawGhostAttackEffect(effect);
                break;
            case EFFECT_ATTACK_DRAGON:
                DrawDragonAttackEffect(effect);
                break;
            case EFFECT_ATTACK_DARK:
                DrawDarkAttackEffect(effect);
                break;
            case EFFECT_ATTACK_STEEL:
                DrawSteelAttackEffect(effect);
                break;
            case EFFECT_ATTACK_FAIRY:
                DrawFairyAttackEffect(effect);
                break;

            default:
                // Efeitos básicos são principalmente partículas
                break;
        }
    }

    // Desenhar flash de tela por último
    if (screenFlashIntensity > 0.0f) {
        Color flashColor = screenFlashColor;
        flashColor.a = (unsigned char)(255 * screenFlashIntensity);
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), flashColor);
    }

    // Desenhar efeitos de shake de tela se necessário
    if (screenShakeIntensity > 0.0f) {
        // O shake é aplicado na matriz de transformação globalmente
        // Aqui apenas disponibilizamos o offset para outras funções
    }
}

// Limpa todos os efeitos ativos
void ClearAllBattleEffects(void) {
    for (int i = 0; i < MAX_BATTLE_EFFECTS; i++) {
        battleEffects[i].active = false;
        statusEffectsActive[i] = false;
    }
    activeEffectCount = 0;

    screenShakeTimer = 0.0f;
    screenFlashTimer = 0.0f;
    statusEffectTimer = 0.0f;
}

// Encontra um slot livre para um novo efeito
int FindFreeEffectSlot(void) {
    for (int i = 0; i < MAX_BATTLE_EFFECTS; i++) {
        if (!battleEffects[i].active) {
            return i;
        }
    }
    return -1;
}

// Cria um efeito de ataque baseado no tipo
void CreateAttackEffect(MonsterType attackType, Vector2 origin, Vector2 target, bool isPlayerAttacking) {
    printf("Criando efeito de ataque tipo %d de %s\n", attackType, isPlayerAttacking ? "jogador" : "oponente");

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

// Cria efeito de cura
void CreateHealEffect(Vector2 position, int healAmount) {
    int slot = FindFreeEffectSlot();
    if (slot == -1) return;

    BattleEffectSystem* effect = &battleEffects[slot];
    effect->active = true;
    effect->type = EFFECT_HEAL;
    effect->timer = 0.0f;
    effect->duration = 2.0f;
    effect->origin = position;
    effect->target = (Vector2){position.x, position.y - 30};
    effect->currentPos = position;
    effect->color = (Color){100, 255, 100, 255};

    // Criar partículas de cura subindo
    for (int i = 0; i < 20; i++) {
        float angle = -PI/2 + ((float)(rand() % 60 - 30) * DEG2RAD);
        float speed = (float)(rand() % 80 + 40);
        Vector2 velocity = {cosf(angle) * speed, sinf(angle) * speed};

        Color healColor = (Color){100 + rand() % 155, 255, 100 + rand() % 155, 200 + rand() % 55};
        Vector2 startPos = {
            position.x + (rand() % 40 - 20),
            position.y + (rand() % 20)
        };

        InitParticle(&effect->particles[i], startPos, velocity, healColor, 1.5f);
        effect->particleCount++;
    }

    activeEffectCount++;
    printf("Efeito de cura criado: %d HP restaurado\n", healAmount);
}

// Cria efeito de aplicação de status
void CreateStatusEffect(Vector2 position, int statusType) {
    int slot = FindFreeEffectSlot();
    if (slot == -1) return;

    BattleEffectSystem* effect = &battleEffects[slot];
    effect->active = true;
    effect->type = EFFECT_STATUS_APPLIED;
    effect->timer = 0.0f;
    effect->duration = 1.5f;
    effect->origin = position;
    effect->currentPos = position;
    statusEffectsActive[slot] = true;

    Color statusColor;
    switch (statusType) {
        case STATUS_PARALYZED:
            statusColor = (Color){255, 255, 100, 255}; // Amarelo
            break;
        case STATUS_SLEEPING:
            statusColor = (Color){100, 100, 255, 255}; // Azul
            break;
        case STATUS_BURNING:
            statusColor = (Color){255, 100, 100, 255}; // Vermelho
            break;
        default:
            statusColor = (Color){200, 200, 200, 255}; // Cinza
            break;
    }

    effect->color = statusColor;

    // Criar partículas de status em círculo
    for (int i = 0; i < 12; i++) {
        float angle = ((float)i / 12) * 2 * PI;
        Vector2 velocity = {
            cosf(angle) * 60,
            sinf(angle) * 60
        };

        InitParticle(&effect->particles[i], position, velocity, statusColor, 1.2f);
        effect->particleCount++;
    }

    activeEffectCount++;
    printf("Efeito de status criado: tipo %d\n", statusType);
}

// Cria efeito de desmaiado
void CreateFaintEffect(Vector2 position) {
    int slot = FindFreeEffectSlot();
    if (slot == -1) return;

    BattleEffectSystem* effect = &battleEffects[slot];
    effect->active = true;
    effect->type = EFFECT_FAINT;
    effect->timer = 0.0f;
    effect->duration = 2.0f;
    effect->origin = position;
    effect->currentPos = position;
    effect->color = (Color){100, 100, 100, 255};

    // Criar partículas caindo
    for (int i = 0; i < 25; i++) {
        float angle = (float)(rand() % 180 + 90) * DEG2RAD; // Para baixo
        float speed = (float)(rand() % 100 + 50);
        Vector2 velocity = {cosf(angle) * speed, sinf(angle) * speed};

        Color faintColor = (Color){100 + rand() % 100, 100 + rand() % 100, 100 + rand() % 100, 255};
        InitParticle(&effect->particles[i], position, velocity, faintColor, 1.5f);
        effect->particleCount++;
    }

    activeEffectCount++;
    PlaySound(faintSound);
    TriggerScreenFlash((Color){100, 100, 100, 120}, 0.8f, 0.5f);
    printf("Efeito de desmaiado criado\n");
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
        if (length > 0) {
            direction.x /= length;
            direction.y /= length;
        }

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
        if (length > 0) {
            direction.x /= length;
            direction.y /= length;
        }

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
        if (length > 0) {
            direction.x /= length;
            direction.y /= length;
        }

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
    Color healColor = (Color){100, 255, 100, (unsigned char)(255 * effect->alpha)};

    // Círculo pulsante
    float pulseSize = 20 + sinf(effect->timer * 8.0f) * 5;
    DrawCircleV(effect->currentPos, pulseSize * effect->intensity, healColor);

    // Anel externo
    DrawCircleLinesV(effect->currentPos, pulseSize * effect->intensity + 10, healColor);
}

void DrawStatusEffect(BattleEffectSystem* effect) {
    Color statusColor = effect->color;
    statusColor.a = (unsigned char)(255 * effect->alpha);

    // Múltiplos anéis pulsantes
    for (int i = 0; i < 3; i++) {
        float ringSize = 15 + i * 10 + sinf(effect->timer * 6.0f + i) * 3;
        DrawCircleLinesV(effect->currentPos, ringSize * effect->intensity, statusColor);
    }
}

void DrawFaintEffect(BattleEffectSystem* effect) {
    Color faintColor = (Color){100, 100, 100, (unsigned char)(255 * effect->alpha)};

    // X gigante indicando desmaiado
    float size = 30 * effect->intensity;
    Vector2 center = effect->currentPos;

    DrawLineEx(
        (Vector2){center.x - size, center.y - size},
        (Vector2){center.x + size, center.y + size},
        3.0f,
        faintColor
    );
    DrawLineEx(
        (Vector2){center.x + size, center.y - size},
        (Vector2){center.x - size, center.y + size},
        3.0f,
        faintColor
    );
}

void DrawCriticalEffect(BattleEffectSystem* effect) {
    Color critColor = (Color){255, 255, 100, (unsigned char)(255 * effect->alpha)};

    // Estrela brilhante
    float starSize = 25 * effect->intensity;
    Vector2 center = effect->currentPos;

    for (int i = 0; i < 8; i++) {
        float angle = (float)i * PI / 4;
        Vector2 point = {
            center.x + cosf(angle) * starSize,
            center.y + sinf(angle) * starSize
        };
        DrawLineEx(center, point, 2.0f, critColor);
    }

    DrawText("CRITICAL!",
             (int)(center.x - 40),
             (int)(center.y - 20),
             20,
             critColor);
}

// Funções de desenho específicas para cada tipo de ataque
void DrawFireAttackEffect(BattleEffectSystem* effect) {
    Color fireColor = (Color){255, 150, 50, (unsigned char)(200 * effect->alpha)};

    // Chamas ondulantes
    for (int i = 0; i < 5; i++) {
        float waveHeight = sinf(effect->timer * 8.0f + i) * 10;
        Vector2 flamePos = {
            effect->currentPos.x + (i - 2) * 15,
            effect->currentPos.y + waveHeight
        };
        DrawCircleV(flamePos, 8 * effect->intensity, fireColor);
    }
}

void DrawWaterAttackEffect(BattleEffectSystem* effect) {
    Color waterColor = (Color){100, 200, 255, (unsigned char)(180 * effect->alpha)};

    // Ondas de água
    for (int i = 0; i < 3; i++) {
        float waveRadius = 20 + i * 15 + effect->timer * 50;
        DrawCircleLinesV(effect->currentPos, waveRadius * effect->intensity, waterColor);
    }
}

void DrawElectricAttackEffect(BattleEffectSystem* effect) {
    Color electricColor = (Color){255, 255, 150, (unsigned char)(255 * effect->alpha)};

    // Raios em zigue-zague
    Vector2 start = effect->origin;
    Vector2 end = effect->target;
    int segments = 8;

    for (int i = 0; i < segments; i++) {
        float t1 = (float)i / segments;
        float t2 = (float)(i + 1) / segments;

        Vector2 p1 = LerpVector2(start, end, t1);
        Vector2 p2 = LerpVector2(start, end, t2);

        // Adicionar variação nos pontos
        p1.x += sinf(effect->timer * 20.0f + i) * 10;
        p2.x += sinf(effect->timer * 20.0f + i + 1) * 10;

        DrawLineEx(p1, p2, 3.0f, electricColor);
    }
}

void DrawGrassAttackEffect(BattleEffectSystem* effect) {
    Color grassColor = (Color){150, 255, 150, (unsigned char)(200 * effect->alpha)};

    // Folhas girando
    for (int i = 0; i < 6; i++) {
        float angle = effect->timer * 4.0f + i * PI / 3;
        float radius = 20 + i * 5;
        Vector2 leafPos = {
            effect->currentPos.x + cosf(angle) * radius,
            effect->currentPos.y + sinf(angle) * radius
        };

        DrawRectanglePro(
            (Rectangle){leafPos.x, leafPos.y, 12, 6},
            (Vector2){6, 3},
            angle * 180 / PI,
            grassColor
        );
    }
}

void DrawIceAttackEffect(BattleEffectSystem* effect) {
    Color iceColor = (Color){200, 230, 255, (unsigned char)(220 * effect->alpha)};

    // Cristais de gelo
    for (int i = 0; i < 4; i++) {
        float angle = (float)i * PI / 2 + effect->timer * 2.0f;
        float size = 15 * effect->intensity;
        Vector2 crystalPos = {
            effect->currentPos.x + cosf(angle) * 25,
            effect->currentPos.y + sinf(angle) * 25
        };

        // Desenhar cristal como losango
        Vector2 points[4] = {
            {crystalPos.x, crystalPos.y - size},
            {crystalPos.x + size/2, crystalPos.y},
            {crystalPos.x, crystalPos.y + size},
            {crystalPos.x - size/2, crystalPos.y}
        };

        for (int j = 0; j < 4; j++) {
            DrawLineEx(points[j], points[(j + 1) % 4], 2.0f, iceColor);
        }
    }
}

void DrawFightingAttackEffect(BattleEffectSystem* effect) {
    Color fightColor = (Color){255, 200, 150, (unsigned char)(255 * effect->alpha)};

    // Ondas de choque concêntricas
    for (int i = 0; i < 4; i++) {
        float shockRadius = i * 20 + effect->timer * 100;
        if (shockRadius > 0 && shockRadius < 100) {
            DrawCircleLinesV(effect->currentPos, shockRadius, fightColor);
        }
    }
}

void DrawPoisonAttackEffect(BattleEffectSystem* effect) {
    Color poisonColor = (Color){200, 100, 200, (unsigned char)(180 * effect->alpha)};

    // Bolhas de veneno subindo
    for (int i = 0; i < 8; i++) {
        float bubbleY = effect->currentPos.y - fmodf(effect->timer * 50 + i * 10, 80);
        Vector2 bubblePos = {
            effect->currentPos.x + sinf(effect->timer * 3.0f + i) * 20,
            bubbleY
        };

        float bubbleSize = 3 + sinf(effect->timer * 5.0f + i) * 2;
        DrawCircleV(bubblePos, bubbleSize, poisonColor);
    }
}

void DrawGroundAttackEffect(BattleEffectSystem* effect) {
    Color groundColor = (Color){200, 150, 100, (unsigned char)(200 * effect->alpha)};

    // Rachadura no chão
    Vector2 start = {effect->currentPos.x - 40, effect->currentPos.y};
    Vector2 end = {effect->currentPos.x + 40, effect->currentPos.y};

    // Linha principal da rachadura
    DrawLineEx(start, end, 4.0f, groundColor);

    // Rachaduras menores
    for (int i = 0; i < 5; i++) {
        float t = (float)i / 5;
        Vector2 crackStart = LerpVector2(start, end, t);
        Vector2 crackEnd = {
            crackStart.x + (rand() % 20 - 10),
            crackStart.y + (rand() % 15 + 5)
        };
        DrawLineEx(crackStart, crackEnd, 2.0f, groundColor);
    }
}

void DrawFlyingAttackEffect(BattleEffectSystem* effect) {
    Color flyingColor = (Color){220, 240, 255, (unsigned char)(150 * effect->alpha)};

    // Rajadas de vento
    for (int i = 0; i < 6; i++) {
        float windSpeed = effect->timer * 200 + i * 30;
        Vector2 windPos = {
            effect->currentPos.x + sinf(windSpeed * 0.01f) * 40,
            effect->currentPos.y + cosf(windSpeed * 0.01f) * 20
        };

        DrawLineEx(
            windPos,
            (Vector2){windPos.x + 20, windPos.y},
            2.0f,
            flyingColor
        );
    }
}

void DrawPsychicAttackEffect(BattleEffectSystem* effect) {
    Color psychicColor = (Color){255, 150, 255, (unsigned char)(200 * effect->alpha)};

    // Ondas psíquicas espirais
    for (int i = 0; i < 20; i++) {
        float angle = (float)i / 20 * 2 * PI + effect->timer * 4.0f;
        float radius = 10 + fmodf(effect->timer * 30, 50);
        Vector2 spiralPos = {
            effect->currentPos.x + cosf(angle) * radius,
            effect->currentPos.y + sinf(angle) * radius
        };

        DrawCircleV(spiralPos, 3, psychicColor);
    }
}

void DrawBugAttackEffect(BattleEffectSystem* effect) {
    Color bugColor = (Color){150, 255, 100, (unsigned char)(180 * effect->alpha)};

    // Enxame de pequenos pontos
    for (int i = 0; i < 15; i++) {
        float speed = effect->timer * 150 + i * 20;
        Vector2 bugPos = {
            effect->currentPos.x + sinf(speed * 0.02f + i) * 30,
            effect->currentPos.y + cosf(speed * 0.03f + i) * 25
        };

        DrawCircleV(bugPos, 2, bugColor);
    }
}

void DrawRockAttackEffect(BattleEffectSystem* effect) {
    Color rockColor = (Color){180, 150, 120, (unsigned char)(220 * effect->alpha)};

    // Pedras caindo
    for (int i = 0; i < 6; i++) {
        float fallY = effect->currentPos.y - 50 + fmodf(effect->timer * 100 + i * 15, 60);
        Vector2 rockPos = {
            effect->currentPos.x + (i - 3) * 15,
            fallY
        };

        DrawRectangleV(rockPos, (Vector2){8, 8}, rockColor);
    }
}

void DrawGhostAttackEffect(BattleEffectSystem* effect) {
    Color ghostColor = (Color){150, 100, 255, (unsigned char)(120 * effect->alpha)};

    // Formas espectrais ondulantes
    for (int i = 0; i < 5; i++) {
        float wave = sinf(effect->timer * 6.0f + i) * 15;
        Vector2 ghostPos = {
            effect->currentPos.x + wave,
            effect->currentPos.y + (i - 2) * 8
        };

        DrawCircleV(ghostPos, 6 + sinf(effect->timer * 4.0f + i) * 2, ghostColor);
    }
}

void DrawDragonAttackEffect(BattleEffectSystem* effect) {
    Color dragonColor = (Color){255, 100, 200, (unsigned char)(200 * effect->alpha)};

    // Chamas dracônicas em espiral
    for (int i = 0; i < 10; i++) {
        float angle = (float)i / 10 * 2 * PI + effect->timer * 3.0f;
        float radius = 20 + sinf(effect->timer * 2.0f + i) * 10;
        Vector2 flamePos = {
            effect->currentPos.x + cosf(angle) * radius,
            effect->currentPos.y + sinf(angle) * radius
        };

        DrawCircleV(flamePos, 5, dragonColor);
    }
}

void DrawDarkAttackEffect(BattleEffectSystem* effect) {
    Color darkColor = (Color){100, 50, 100, (unsigned char)(180 * effect->alpha)};

    // Sombras se expandindo
    for (int i = 0; i < 8; i++) {
        float angle = (float)i / 8 * 2 * PI;
        float length = 30 * effect->intensity;
        Vector2 shadowEnd = {
            effect->currentPos.x + cosf(angle) * length,
            effect->currentPos.y + sinf(angle) * length
        };

        DrawLineEx(effect->currentPos, shadowEnd, 3.0f, darkColor);
    }
}

void DrawSteelAttackEffect(BattleEffectSystem* effect) {
    Color steelColor = (Color){220, 220, 220, (unsigned char)(255 * effect->alpha)};

    // Faíscas metálicas
    for (int i = 0; i < 12; i++) {
        float sparkAngle = (float)(rand() % 360) * DEG2RAD;
        float sparkDistance = (rand() % 30) + 10;
        Vector2 sparkPos = {
            effect->currentPos.x + cosf(sparkAngle) * sparkDistance,
            effect->currentPos.y + sinf(sparkAngle) * sparkDistance
        };

        DrawLineEx(
            sparkPos,
            (Vector2){sparkPos.x + cosf(sparkAngle) * 8, sparkPos.y + sinf(sparkAngle) * 8},
            1.0f,
            steelColor
        );
    }
}

void DrawFairyAttackEffect(BattleEffectSystem* effect) {
    Color fairyColor = (Color){255, 200, 255, (unsigned char)(150 * effect->alpha)};

    // Brilhos de fada
    for (int i = 0; i < 10; i++) {
        float twinkleAngle = (float)i / 10 * 2 * PI + effect->timer * 5.0f;
        float twinkleRadius = 15 + sinf(effect->timer * 3.0f + i) * 8;
        Vector2 twinklePos = {
            effect->currentPos.x + cosf(twinkleAngle) * twinkleRadius,
            effect->currentPos.y + sinf(twinkleAngle) * twinkleRadius
        };

        // Desenhar pequena estrela
        for (int j = 0; j < 4; j++) {
            float starAngle = (float)j * PI / 2;
            Vector2 starPoint = {
                twinklePos.x + cosf(starAngle) * 3,
                twinklePos.y + sinf(starAngle) * 3
            };
            DrawLineEx(twinklePos, starPoint, 1.0f, fairyColor);
        }
    }
}

// Função para criar efeitos contínuos de status
void CreateContinuousStatusEffect(Vector2 position, int statusType, float duration) {
    int slot = FindFreeEffectSlot();
    if (slot == -1) return;

    BattleEffectSystem* effect = &battleEffects[slot];
    effect->active = true;
    effect->type = EFFECT_STATUS_APPLIED;
    effect->timer = 0.0f;
    effect->duration = duration;
    effect->origin = position;
    effect->currentPos = position;
    statusEffectsActive[slot] = true;

    // Configurar cor e comportamento baseado no status
    switch (statusType) {
        case STATUS_PARALYZED:
            effect->color = (Color){255, 255, 100, 255};
            // Criar raios pequenos ao redor
            for (int i = 0; i < 8; i++) {
                float angle = ((float)i / 8) * 2 * PI;
                Vector2 velocity = {cosf(angle) * 30, sinf(angle) * 30};
                InitParticle(&effect->particles[i], position, velocity, effect->color, 0.5f);
                effect->particleCount++;
            }
            break;

        case STATUS_SLEEPING:
            effect->color = (Color){150, 150, 255, 255};
            // Criar bolhas de sono
            for (int i = 0; i < 5; i++) {
                Vector2 velocity = {(rand() % 20 - 10), -(rand() % 50 + 20)};
                InitParticle(&effect->particles[i], position, velocity, effect->color, 2.0f);
                effect->particleCount++;
            }
            break;

        case STATUS_BURNING:
            effect->color = (Color){255, 150, 100, 255};
            // Criar chamas pequenas
            for (int i = 0; i < 10; i++) {
                Vector2 velocity = {(rand() % 30 - 15), -(rand() % 40 + 20)};
                Color fireColor = (i % 2) ? (Color){255, 100, 50, 255} : (Color){255, 150, 0, 255};
                InitParticle(&effect->particles[i], position, velocity, fireColor, 1.0f);
                effect->particleCount++;
            }
            break;
    }

    activeEffectCount++;
    printf("Efeito contínuo de status criado: tipo %d por %.1f segundos\n", statusType, duration);
}