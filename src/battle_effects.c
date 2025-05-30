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

    // Atualizar efeitos de tela (reduzir intensidade)
    if (screenShakeTimer > 0.0f) {
        screenShakeTimer -= deltaTime;
        float intensity = screenShakeIntensity * (screenShakeTimer / 1.0f) * 0.5f;
        screenShakeOffset.x = (float)(rand() % 21 - 10) * intensity * 0.1f;
        screenShakeOffset.y = (float)(rand() % 21 - 10) * intensity * 0.1f;

        if (screenShakeTimer <= 0.0f) {
            screenShakeOffset = (Vector2){0, 0};
        }
    }

    // Remover completamente o sistema de flash de tela
    screenFlashTimer = 0.0f;
    screenFlashIntensity = 0.0f;

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

        // NOVO SISTEMA DE MOVIMENTO PARA ATAQUES
        if (effect->type >= EFFECT_ATTACK_NORMAL && effect->type <= EFFECT_ATTACK_FAIRY) {
            // Dividir em duas fases: viagem (0-60%) e impacto (60-100%)
            if (progress <= 0.6f) {
                // Fase de viagem: movimento suave do atacante para o defensor
                float travelProgress = progress / 0.6f; // Normalizar para 0-1
                effect->currentPos = LerpVector2(effect->origin, effect->target, EaseInOut(travelProgress));

                printf("[ATTACK MOVEMENT] Progresso: %.1f%%, Pos: (%.1f,%.1f) -> (%.1f,%.1f)\n",
                       travelProgress * 100, effect->currentPos.x, effect->currentPos.y,
                       effect->target.x, effect->target.y);
            } else {
                // Fase de impacto: permanecer no alvo
                effect->currentPos = effect->target;
            }
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

            // Efeitos de ataque específicos por tipo (MELHORADOS)
            case EFFECT_ATTACK_NORMAL:
                DrawNormalAttackEffect(effect);
                break;
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
                // Para tipos não implementados, usar efeito básico
                if (effect->type >= EFFECT_ATTACK_NORMAL && effect->type <= EFFECT_ATTACK_FAIRY) {
                    DrawNormalAttackEffect(effect);
                }
                break;
        }
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

    // Criar efeito visual baseado no tipo (SEM FLASH!)
    switch (attackType) {
        case TYPE_FIRE:
            CreateFireEffect(origin, target);
            // TriggerScreenFlash removido!
            break;
        case TYPE_WATER:
            CreateWaterEffect(origin, target);
            // TriggerScreenFlash removido!
            break;
        case TYPE_GRASS:
            CreateGrassEffect(origin, target);
            // TriggerScreenFlash removido!
            break;
        case TYPE_ELECTRIC:
            CreateElectricEffect(origin, target);
            // TriggerScreenFlash removido!
            TriggerScreenShake(6.0f, 0.3f); // Manter apenas shake leve
            break;
        case TYPE_ICE:
            CreateIceEffect(origin, target);
            // TriggerScreenFlash removido!
            break;
        case TYPE_FIGHTING:
            CreateFightingEffect(origin, target);
            TriggerScreenShake(4.0f, 0.3f); // Shake mais suave
            break;
        case TYPE_POISON:
            CreatePoisonEffect(origin, target);
            // TriggerScreenFlash removido!
            break;
        case TYPE_GROUND:
            CreateGroundEffect(origin, target);
            TriggerScreenShake(6.0f, 0.4f); // Manter shake para ataques de terra
            break;
        case TYPE_FLYING:
            CreateFlyingEffect(origin, target);
            break;
        case TYPE_PSYCHIC:
            CreatePsychicEffect(origin, target);
            // TriggerScreenFlash removido!
            break;
        case TYPE_BUG:
            CreateBugEffect(origin, target);
            break;
        case TYPE_ROCK:
            CreateRockEffect(origin, target);
            TriggerScreenShake(5.0f, 0.3f); // Manter shake para ataques de pedra
            break;
        case TYPE_GHOST:
            CreateGhostEffect(origin, target);
            // TriggerScreenFlash removido!
            break;
        case TYPE_DRAGON:
            CreateDragonEffect(origin, target);
            // TriggerScreenFlash removido!
            TriggerScreenShake(4.0f, 0.3f); // Shake leve
            break;
        case TYPE_DARK:
            CreateDarkEffect(origin, target);
            // TriggerScreenFlash removido!
            break;
        case TYPE_STEEL:
            CreateSteelEffect(origin, target);
            // TriggerScreenFlash removido!
            break;
        case TYPE_FAIRY:
            CreateFairyEffect(origin, target);
            // TriggerScreenFlash removido!
            break;
        default:
            CreateNormalEffect(origin, target);
            break;
    }
}

void CreateAttackProjectile(BattleEffectSystem* effect, MonsterType attackType) {
    // Criar partículas que formam um "projétil" visível
    Vector2 direction = {effect->target.x - effect->origin.x, effect->target.y - effect->origin.y};
    float length = sqrtf(direction.x * direction.x + direction.y * direction.y);

    if (length > 0) {
        direction.x /= length;
        direction.y /= length;
    }

    // Cores baseadas no tipo
    Color projectileColor;
    int particleCount;
    float particleSize;

    switch (attackType) {
        case TYPE_FIRE:
            projectileColor = (Color){255, 150, 50, 255};
            particleCount = 15;
            particleSize = 8.0f;
            break;
        case TYPE_WATER:
            projectileColor = (Color){100, 180, 255, 255};
            particleCount = 12;
            particleSize = 6.0f;
            break;
        case TYPE_ELECTRIC:
            projectileColor = (Color){255, 255, 150, 255};
            particleCount = 10;
            particleSize = 5.0f;
            break;
        case TYPE_GRASS:
            projectileColor = (Color){150, 255, 150, 255};
            particleCount = 8;
            particleSize = 7.0f;
            break;
        default:
            projectileColor = (Color){255, 255, 255, 255};
            particleCount = 10;
            particleSize = 6.0f;
            break;
    }

    // Criar partículas em formação para formar o projétil
    for (int i = 0; i < particleCount && i < MAX_PARTICLES_PER_EFFECT; i++) {
        // Posição inicial com pequena variação
        Vector2 startPos = {
            effect->origin.x + (rand() % 20 - 10),
            effect->origin.y + (rand() % 20 - 10)
        };

        // Velocidade em direção ao alvo com pequena variação
        Vector2 velocity = {
            direction.x * (200 + rand() % 100) + (rand() % 40 - 20),
            direction.y * (200 + rand() % 100) + (rand() % 40 - 20)
        };

        // Aumentar o tamanho das partículas
        effect->particles[i].position = startPos;
        effect->particles[i].velocity = velocity;
        effect->particles[i].acceleration = (Vector2){0, 20}; // Gravidade leve
        effect->particles[i].size = particleSize + (rand() % 4); // TAMANHO MAIOR
        effect->particles[i].life = 0.0f;
        effect->particles[i].maxLife = 2.0f;
        effect->particles[i].color = projectileColor;
        effect->particles[i].targetColor = (Color){projectileColor.r, projectileColor.g, projectileColor.b, 0};
        effect->particles[i].rotation = 0.0f;
        effect->particles[i].rotationSpeed = (float)(rand() % 200 - 100);
        effect->particles[i].active = true;
        effect->particles[i].type = 0;

        effect->particleCount++;
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
    effect->duration = isCritical ? 2.0f : 1.5f; // Duração mais longa
    effect->origin = position;
    effect->target = (Vector2){position.x, position.y - 50};
    effect->currentPos = position;
    effect->intensity = isCritical ? 1.5f : 1.0f;
    effect->isPlayerTarget = isPlayerTarget;

    // Configurar cor baseada no tipo de dano
    if (isCritical) {
        effect->color = (Color){255, 255, 100, 255}; // Amarelo para crítico
        // REMOVER: TriggerScreenFlash((Color){255, 255, 150, 150}, 1.0f, 0.3f);
        // REMOVER: TriggerScreenShake(12.0f, 0.5f);
    } else {
        effect->color = isPlayerTarget ? (Color){255, 100, 100, 255} : (Color){255, 150, 150, 255};
        // REMOVER: TriggerScreenFlash((Color){255, 200, 200, 100}, 0.6f, 0.2f);
        // REMOVER: TriggerScreenShake(5.0f, 0.3f);
    }

    // Criar partículas de impacto (mais duradouras)
    for (int i = 0; i < 15; i++) {
        float angle = (float)(rand() % 360) * DEG2RAD;
        float speed = (float)(rand() % 80 + 30); // Velocidade reduzida

        Vector2 velocity = {cosf(angle) * speed, sinf(angle) * speed};

        Color particleColor = effect->color;
        particleColor.a = (unsigned char)(rand() % 100 + 155);

        InitParticle(&effect->particles[i], position, velocity, particleColor, 1.2f); // Vida mais longa
        effect->particleCount++;
    }

    activeEffectCount++;
    PlaySound(hitSound);

    printf("Efeito de dano criado: %d de dano, crítico: %s (SEM FLASH)\n", damage, isCritical ? "sim" : "não");
}


// Cria efeito de cura
void CreateHealEffect(Vector2 position, int healAmount) {
    int slot = FindFreeEffectSlot();
    if (slot == -1) return;

    BattleEffectSystem* effect = &battleEffects[slot];
    effect->active = true;
    effect->type = EFFECT_HEAL;
    effect->timer = 0.0f;
    effect->duration = 2.5f; // Era 2.0f, agora 2.5f
    effect->origin = position;
    effect->target = (Vector2){position.x, position.y - 30};
    effect->currentPos = position;
    effect->color = (Color){100, 255, 100, 255};

    // Criar partículas de cura subindo (movimento mais lento)
    for (int i = 0; i < 20; i++) {
        float angle = -PI/2 + ((float)(rand() % 60 - 30) * DEG2RAD);
        float speed = (float)(rand() % 50 + 25); // Velocidade reduzida

        Vector2 velocity = {cosf(angle) * speed, sinf(angle) * speed};

        Color healColor = (Color){100 + rand() % 155, 255, 100 + rand() % 155, 200 + rand() % 55};
        Vector2 startPos = {
            position.x + (rand() % 40 - 20),
            position.y + (rand() % 20)
        };

        InitParticle(&effect->particles[i], startPos, velocity, healColor, 2.0f); // Vida mais longa
        effect->particleCount++;
    }

    activeEffectCount++;
    printf("Efeito de cura criado: %d HP restaurado (duração ajustada)\n", healAmount);
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
    effect->duration = 2.5f; // Duração mais longa
    effect->origin = position;
    effect->currentPos = position;
    effect->color = (Color){100, 100, 100, 255};

    // Criar partículas caindo
    for (int i = 0; i < 25; i++) {
        float angle = (float)(rand() % 180 + 90) * DEG2RAD; // Para baixo
        float speed = (float)(rand() % 80 + 30); // Velocidade reduzida
        Vector2 velocity = {cosf(angle) * speed, sinf(angle) * speed};

        Color faintColor = (Color){100 + rand() % 100, 100 + rand() % 100, 100 + rand() % 100, 255};
        InitParticle(&effect->particles[i], position, velocity, faintColor, 2.0f); // Vida mais longa
        effect->particleCount++;
    }

    activeEffectCount++;
    PlaySound(faintSound);
    // REMOVER: TriggerScreenFlash((Color){100, 100, 100, 120}, 0.8f, 0.5f);
    printf("Efeito de desmaiado criado (SEM FLASH)\n");
}


// Implementações dos efeitos específicos por tipo



void CreateFireEffect(Vector2 origin, Vector2 target) {
    int slot = FindFreeEffectSlot();
    if (slot == -1) return;

    BattleEffectSystem* effect = &battleEffects[slot];
    effect->active = true;
    effect->type = EFFECT_ATTACK_FIRE;
    effect->timer = 0.0f;
    effect->duration = 2.0f;
    effect->origin = origin;
    effect->target = target;
    effect->currentPos = origin; // Começar na origem
    effect->color = (Color){255, 100, 50, 255};

    printf("[FIRE EFFECT] Criando de (%.1f,%.1f) para (%.1f,%.1f)\n",
           origin.x, origin.y, target.x, target.y);

    // Criar um projétil de fogo MAIOR que viaja
    Vector2 direction = {target.x - origin.x, target.y - origin.y};
    float length = sqrtf(direction.x * direction.x + direction.y * direction.y);
    if (length > 0) {
        direction.x /= length;
        direction.y /= length;
    }

    // Criar partículas MAIORES em formação concentrada para formar projétil visível
    for (int i = 0; i < 20; i++) {
        Vector2 velocity = {
            direction.x * (150 + rand() % 50),
            direction.y * (150 + rand() % 50)
        };

        // Adicionar pequena variação na posição inicial para formar um "grupo"
        Vector2 startPos = {
            origin.x + (rand() % 30 - 15),
            origin.y + (rand() % 30 - 15)
        };

        Color fireColor = (rand() % 2) ? (Color){255, 150, 50, 255} : (Color){255, 100, 0, 255};

        effect->particles[i].position = startPos;
        effect->particles[i].velocity = velocity;
        effect->particles[i].acceleration = (Vector2){0, 30};
        effect->particles[i].size = 12.0f + (rand() % 8); // MUITO MAIOR: 12-20 pixels
        effect->particles[i].life = 0.0f;
        effect->particles[i].maxLife = 1.8f;
        effect->particles[i].color = fireColor;
        effect->particles[i].targetColor = (Color){fireColor.r, fireColor.g, fireColor.b, 0};
        effect->particles[i].rotation = 0.0f;
        effect->particles[i].rotationSpeed = (float)(rand() % 200 - 100);
        effect->particles[i].active = true;
        effect->particles[i].type = 0;

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
    effect->duration = 1.8f;
    effect->origin = origin;
    effect->target = target;
    effect->currentPos = origin;
    effect->color = (Color){50, 150, 255, 255};

    printf("[WATER EFFECT] Criando de (%.1f,%.1f) para (%.1f,%.1f)\n",
           origin.x, origin.y, target.x, target.y);

    Vector2 direction = {target.x - origin.x, target.y - origin.y};
    float length = sqrtf(direction.x * direction.x + direction.y * direction.y);
    if (length > 0) {
        direction.x /= length;
        direction.y /= length;
    }

    // Criar jato de água concentrado e MAIOR
    for (int i = 0; i < 18; i++) {
        Vector2 velocity = {
            direction.x * (180 + rand() % 60),
            direction.y * (180 + rand() % 60)
        };

        Vector2 startPos = {
            origin.x + (rand() % 20 - 10),
            origin.y + (rand() % 20 - 10)
        };

        Color waterColor = (Color){50 + rand() % 100, 150 + rand() % 50, 255, 200 + rand() % 55};

        effect->particles[i].position = startPos;
        effect->particles[i].velocity = velocity;
        effect->particles[i].acceleration = (Vector2){0, 25};
        effect->particles[i].size = 10.0f + (rand() % 6); // MAIOR: 10-16 pixels
        effect->particles[i].life = 0.0f;
        effect->particles[i].maxLife = 1.6f;
        effect->particles[i].color = waterColor;
        effect->particles[i].targetColor = (Color){waterColor.r, waterColor.g, waterColor.b, 0};
        effect->particles[i].rotation = 0.0f;
        effect->particles[i].rotationSpeed = (float)(rand() % 100 - 50);
        effect->particles[i].active = true;
        effect->particles[i].type = 0;

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
    effect->duration = 1.5f;
    effect->origin = origin;
    effect->target = target;
    effect->currentPos = origin;
    effect->color = (Color){255, 255, 100, 255};

    printf("[ELECTRIC EFFECT] Criando de (%.1f,%.1f) para (%.1f,%.1f)\n",
           origin.x, origin.y, target.x, target.y);

    // Criar raio elétrico que viaja em linha reta
    Vector2 direction = {target.x - origin.x, target.y - origin.y};
    float length = sqrtf(direction.x * direction.x + direction.y * direction.y);

    // Criar segmentos de raio ao longo do caminho
    int segments = (int)(length / 15); // Um segmento a cada 15 pixels
    if (segments > MAX_PARTICLES_PER_EFFECT) segments = MAX_PARTICLES_PER_EFFECT;

    for (int i = 0; i < segments; i++) {
        float t = (float)i / segments;
        Vector2 segmentPos = {
            origin.x + direction.x * t + (rand() % 40 - 20), // Variação para zigue-zague
            origin.y + direction.y * t + (rand() % 40 - 20)
        };

        Vector2 velocity = {
            (rand() % 60 - 30),
            (rand() % 60 - 30)
        };

        Color electricColor = (Color){255, 255, 100 + rand() % 155, 255};

        effect->particles[i].position = segmentPos;
        effect->particles[i].velocity = velocity;
        effect->particles[i].acceleration = (Vector2){0, 0}; // Sem gravidade para raios
        effect->particles[i].size = 8.0f + (rand() % 6); // MAIOR: 8-14 pixels
        effect->particles[i].life = 0.0f;
        effect->particles[i].maxLife = 1.2f;
        effect->particles[i].color = electricColor;
        effect->particles[i].targetColor = (Color){electricColor.r, electricColor.g, electricColor.b, 0};
        effect->particles[i].rotation = 0.0f;
        effect->particles[i].rotationSpeed = 0.0f; // Sem rotação para raios
        effect->particles[i].active = true;
        effect->particles[i].type = 0;

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
    effect->duration = 1.8f;
    effect->origin = origin;
    effect->target = target;
    effect->currentPos = origin;
    effect->color = (Color){100, 255, 100, 255};

    printf("[GRASS EFFECT] Criando de (%.1f,%.1f) para (%.1f,%.1f)\n",
           origin.x, origin.y, target.x, target.y);

    Vector2 direction = {target.x - origin.x, target.y - origin.y};
    float length = sqrtf(direction.x * direction.x + direction.y * direction.y);
    if (length > 0) {
        direction.x /= length;
        direction.y /= length;
    }

    // Criar "folhas" voando em direção ao alvo
    for (int i = 0; i < 15; i++) {
        Vector2 velocity = {
            direction.x * (120 + rand() % 80),
            direction.y * (120 + rand() % 80)
        };

        Vector2 startPos = {
            origin.x + (rand() % 40 - 20),
            origin.y + (rand() % 40 - 20)
        };

        Color leafColor = (rand() % 2) ? (Color){100, 255, 100, 255} : (Color){50, 200, 50, 255};

        effect->particles[i].position = startPos;
        effect->particles[i].velocity = velocity;
        effect->particles[i].acceleration = (Vector2){0, 20};
        effect->particles[i].size = 14.0f + (rand() % 8); // MAIOR: 14-22 pixels (folhas maiores)
        effect->particles[i].life = 0.0f;
        effect->particles[i].maxLife = 1.6f;
        effect->particles[i].color = leafColor;
        effect->particles[i].targetColor = (Color){leafColor.r, leafColor.g, leafColor.b, 0};
        effect->particles[i].rotation = (float)(rand() % 360);
        effect->particles[i].rotationSpeed = (float)(rand() % 300 - 150); // Rotação das folhas
        effect->particles[i].active = true;
        effect->particles[i].type = 0;

        effect->particleCount++;
    }

    activeEffectCount++;
}

// Criar efeito normal maior também
void CreateNormalEffect(Vector2 origin, Vector2 target) {
    int slot = FindFreeEffectSlot();
    if (slot == -1) return;

    BattleEffectSystem* effect = &battleEffects[slot];
    effect->active = true;
    effect->type = EFFECT_ATTACK_NORMAL;
    effect->timer = 0.0f;
    effect->duration = 1.5f;
    effect->origin = origin;
    effect->target = target;
    effect->currentPos = origin;
    effect->color = (Color){200, 200, 200, 255};

    printf("[NORMAL EFFECT] Criando de (%.1f,%.1f) para (%.1f,%.1f)\n",
           origin.x, origin.y, target.x, target.y);

    Vector2 direction = {target.x - origin.x, target.y - origin.y};
    float length = sqrtf(direction.x * direction.x + direction.y * direction.y);
    if (length > 0) {
        direction.x /= length;
        direction.y /= length;
    }

    for (int i = 0; i < 12; i++) {
        Vector2 velocity = {
            direction.x * (140 + rand() % 60),
            direction.y * (140 + rand() % 60)
        };

        Vector2 startPos = {
            origin.x + (rand() % 25 - 12),
            origin.y + (rand() % 25 - 12)
        };

        Color normalColor = (Color){200 + rand() % 55, 200 + rand() % 55, 200 + rand() % 55, 255};

        effect->particles[i].position = startPos;
        effect->particles[i].velocity = velocity;
        effect->particles[i].acceleration = (Vector2){0, 25};
        effect->particles[i].size = 9.0f + (rand() % 6); // MAIOR: 9-15 pixels
        effect->particles[i].life = 0.0f;
        effect->particles[i].maxLife = 1.3f;
        effect->particles[i].color = normalColor;
        effect->particles[i].targetColor = (Color){normalColor.r, normalColor.g, normalColor.b, 0};
        effect->particles[i].rotation = 0.0f;
        effect->particles[i].rotationSpeed = (float)(rand() % 200 - 100);
        effect->particles[i].active = true;
        effect->particles[i].type = 0;

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

    // Atualizar física com menos gravidade
    particle->velocity.x += particle->acceleration.x * deltaTime;
    particle->velocity.y += particle->acceleration.y * deltaTime * 0.3f; // Gravidade reduzida

    particle->position.x += particle->velocity.x * deltaTime;
    particle->position.y += particle->velocity.y * deltaTime;

    particle->rotation += particle->rotationSpeed * deltaTime * 0.5f; // Rotação mais lenta

    // Atualizar vida
    particle->life += deltaTime;

    if (particle->life >= particle->maxLife) {
        particle->active = false;
        return;
    }

    // Interpolar cor (fade out mais suave)
    float t = particle->life / particle->maxLife;
    particle->color = LerpColor(particle->color, particle->targetColor, EaseOut(t) * 0.8f);

    // Reduzir tamanho gradualmente (mais lento)
    particle->size *= (1.0f - deltaTime * 0.3f); // Era 0.5f, agora 0.3f
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
    Color fireColor = (Color){255, 150, 50, (unsigned char)(255 * effect->alpha)};

    // DESENHAR PROJÉTIL PRINCIPAL GRANDE
    if (effect->alpha > 0.1f) {
        // Núcleo do projétil de fogo
        float coreSize = 25.0f * effect->intensity; // Tamanho grande
        DrawCircle((int)effect->currentPos.x, (int)effect->currentPos.y, coreSize, fireColor);

        // Anel externo para mais impacto visual
        Color outerFire = (Color){255, 100, 0, (unsigned char)(180 * effect->alpha)};
        DrawCircle((int)effect->currentPos.x, (int)effect->currentPos.y, coreSize * 1.3f, outerFire);

        // Trilha atrás do projétil
        Vector2 direction = {effect->target.x - effect->origin.x, effect->target.y - effect->origin.y};
        float length = sqrtf(direction.x * direction.x + direction.y * direction.y);
        if (length > 0) {
            direction.x /= length;
            direction.y /= length;
        }

        // Desenhar trilha de fogo
        for (int i = 1; i <= 5; i++) {
            Vector2 trailPos = {
                effect->currentPos.x - direction.x * i * 15,
                effect->currentPos.y - direction.y * i * 15
            };

            float trailAlpha = effect->alpha * (1.0f - (float)i / 6.0f);
            Color trailColor = (Color){255, 150, 50, (unsigned char)(255 * trailAlpha)};
            float trailSize = coreSize * (1.0f - (float)i / 8.0f);

            DrawCircle((int)trailPos.x, (int)trailPos.y, trailSize, trailColor);
        }
    }

    printf("[DRAW FIRE] Pos: (%.1f,%.1f), Alpha: %.2f, Tamanho: %.1f\n",
           effect->currentPos.x, effect->currentPos.y, effect->alpha, 25.0f * effect->intensity);
}

void DrawWaterAttackEffect(BattleEffectSystem* effect) {
    Color waterColor = (Color){100, 200, 255, (unsigned char)(255 * effect->alpha)};

    // DESENHAR JATO DE ÁGUA PRINCIPAL
    if (effect->alpha > 0.1f) {
        // Núcleo do jato de água
        float coreSize = 20.0f * effect->intensity;
        DrawCircle((int)effect->currentPos.x, (int)effect->currentPos.y, coreSize, waterColor);

        // Gotas ao redor
        Color dropColor = (Color){50, 150, 255, (unsigned char)(200 * effect->alpha)};
        DrawCircle((int)effect->currentPos.x, (int)effect->currentPos.y, coreSize * 1.2f, dropColor);

        // Trilha de água
        Vector2 direction = {effect->target.x - effect->origin.x, effect->target.y - effect->origin.y};
        float length = sqrtf(direction.x * direction.x + direction.y * direction.y);
        if (length > 0) {
            direction.x /= length;
            direction.y /= length;
        }

        for (int i = 1; i <= 4; i++) {
            Vector2 trailPos = {
                effect->currentPos.x - direction.x * i * 12,
                effect->currentPos.y - direction.y * i * 12
            };

            float trailAlpha = effect->alpha * (1.0f - (float)i / 5.0f);
            Color trailColor = (Color){100, 200, 255, (unsigned char)(255 * trailAlpha)};
            float trailSize = coreSize * (1.0f - (float)i / 6.0f);

            DrawCircle((int)trailPos.x, (int)trailPos.y, trailSize, trailColor);
        }
    }

    printf("[DRAW WATER] Pos: (%.1f,%.1f), Alpha: %.2f, Tamanho: %.1f\n",
           effect->currentPos.x, effect->currentPos.y, effect->alpha, 20.0f * effect->intensity);
}

void DrawElectricAttackEffect(BattleEffectSystem* effect) {
    Color electricColor = (Color){255, 255, 150, (unsigned char)(255 * effect->alpha)};

    // DESENHAR RAIO PRINCIPAL
    if (effect->alpha > 0.1f) {
        // Linha principal do raio (mais grossa)
        DrawLineEx(effect->origin, effect->currentPos, 8.0f * effect->intensity, electricColor);

        // Linha secundária (brilho)
        Color glowColor = (Color){255, 255, 255, (unsigned char)(200 * effect->alpha)};
        DrawLineEx(effect->origin, effect->currentPos, 4.0f * effect->intensity, glowColor);

        // Raios secundários (zigue-zague)
        Vector2 direction = {effect->target.x - effect->origin.x, effect->target.y - effect->origin.y};
        float length = sqrtf(direction.x * direction.x + direction.y * direction.y);

        if (length > 0) {
            direction.x /= length;
            direction.y /= length;

            // Criar zigue-zague
            for (int i = 0; i < 3; i++) {
                float t = (float)i / 3.0f;
                Vector2 zigPos = {
                    effect->origin.x + direction.x * length * t + (rand() % 30 - 15),
                    effect->origin.y + direction.y * length * t + (rand() % 30 - 15)
                };

                Vector2 zagPos = {
                    zigPos.x + direction.x * length * 0.1f + (rand() % 30 - 15),
                    zigPos.y + direction.y * length * 0.1f + (rand() % 30 - 15)
                };

                DrawLineEx(zigPos, zagPos, 3.0f, electricColor);
            }
        }

        // Núcleo brilhante na posição atual
        DrawCircle((int)effect->currentPos.x, (int)effect->currentPos.y, 15.0f * effect->intensity, electricColor);
    }

    printf("[DRAW ELECTRIC] Linha de (%.1f,%.1f) para (%.1f,%.1f), Alpha: %.2f\n",
           effect->origin.x, effect->origin.y, effect->currentPos.x, effect->currentPos.y, effect->alpha);
}

void DrawGrassAttackEffect(BattleEffectSystem* effect) {
    Color grassColor = (Color){150, 255, 150, (unsigned char)(255 * effect->alpha)};

    // DESENHAR TORNADO DE FOLHAS
    if (effect->alpha > 0.1f) {
        // Núcleo do tornado
        float coreSize = 18.0f * effect->intensity;
        DrawCircle((int)effect->currentPos.x, (int)effect->currentPos.y, coreSize, grassColor);

        // Folhas girando ao redor
        for (int i = 0; i < 6; i++) {
            float angle = effect->timer * 4.0f + i * PI / 3;
            float radius = coreSize * 1.5f;
            Vector2 leafPos = {
                effect->currentPos.x + cosf(angle) * radius,
                effect->currentPos.y + sinf(angle) * radius
            };

            Color leafColor = (Color){100 + rand() % 100, 255, 100 + rand() % 100, (unsigned char)(200 * effect->alpha)};

            // Desenhar folha como retângulo rotacionado
            DrawRectanglePro(
                (Rectangle){leafPos.x, leafPos.y, 16, 8},
                (Vector2){8, 4},
                angle * 180 / PI,
                leafColor
            );
        }

        // Trilha de folhas
        Vector2 direction = {effect->target.x - effect->origin.x, effect->target.y - effect->origin.y};
        float length = sqrtf(direction.x * direction.x + direction.y * direction.y);
        if (length > 0) {
            direction.x /= length;
            direction.y /= length;
        }

        for (int i = 1; i <= 3; i++) {
            Vector2 trailPos = {
                effect->currentPos.x - direction.x * i * 20,
                effect->currentPos.y - direction.y * i * 20
            };

            float trailAlpha = effect->alpha * (1.0f - (float)i / 4.0f);
            Color trailColor = (Color){150, 255, 150, (unsigned char)(255 * trailAlpha)};
            float trailSize = coreSize * (1.0f - (float)i / 5.0f);

            DrawCircle((int)trailPos.x, (int)trailPos.y, trailSize, trailColor);
        }
    }

    printf("[DRAW GRASS] Pos: (%.1f,%.1f), Alpha: %.2f, Tamanho: %.1f\n",
           effect->currentPos.x, effect->currentPos.y, effect->alpha, 18.0f * effect->intensity);
}

void DrawIceAttackEffect(BattleEffectSystem* effect) {
    Color iceColor = (Color){150, 200, 255, (unsigned char)(255 * effect->alpha)};

    if (effect->alpha > 0.1f) {
        // Projétil de gelo
        float coreSize = 22.0f * effect->intensity;
        DrawCircle((int)effect->currentPos.x, (int)effect->currentPos.y, coreSize, iceColor);

        // Cristais ao redor
        for (int i = 0; i < 6; i++) {
            float angle = (float)i * PI / 3 + effect->timer * 2.0f;
            Vector2 crystalPos = {
                effect->currentPos.x + cosf(angle) * coreSize * 0.8f,
                effect->currentPos.y + sinf(angle) * coreSize * 0.8f
            };

            Color crystalColor = (Color){200, 230, 255, (unsigned char)(200 * effect->alpha)};
            DrawCircle((int)crystalPos.x, (int)crystalPos.y, 4, crystalColor);
        }
    }
}

void DrawFightingAttackEffect(BattleEffectSystem* effect) {
    Color fightColor = (Color){255, 150, 100, (unsigned char)(255 * effect->alpha)};

    if (effect->alpha > 0.1f) {
        // Punho de energia
        float coreSize = 20.0f * effect->intensity;
        DrawCircle((int)effect->currentPos.x, (int)effect->currentPos.y, coreSize, fightColor);

        // Ondas de choque
        for (int i = 1; i <= 3; i++) {
            float waveSize = coreSize + i * 8;
            Color waveColor = (Color){255, 150, 100, (unsigned char)(100 * effect->alpha / i)};
            DrawCircleLines((int)effect->currentPos.x, (int)effect->currentPos.y, waveSize, waveColor);
        }
    }
}

void DrawPoisonAttackEffect(BattleEffectSystem* effect) {
    Color poisonColor = (Color){150, 50, 150, (unsigned char)(255 * effect->alpha)};

    if (effect->alpha > 0.1f) {
        // Bolha venenosa
        float coreSize = 19.0f * effect->intensity;
        DrawCircle((int)effect->currentPos.x, (int)effect->currentPos.y, coreSize, poisonColor);

        // Bolhas menores ao redor
        for (int i = 0; i < 4; i++) {
            float angle = (float)i * PI / 2 + effect->timer * 3.0f;
            Vector2 bubblePos = {
                effect->currentPos.x + cosf(angle) * coreSize * 1.2f,
                effect->currentPos.y + sinf(angle) * coreSize * 1.2f
            };

            Color bubbleColor = (Color){100, 0, 100, (unsigned char)(150 * effect->alpha)};
            DrawCircle((int)bubblePos.x, (int)bubblePos.y, 6, bubbleColor);
        }
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

void DrawNormalAttackEffect(BattleEffectSystem* effect) {
    Color normalColor = (Color){200, 200, 200, (unsigned char)(255 * effect->alpha)};

    if (effect->alpha > 0.1f) {
        // Projétil normal (energia neutra)
        float coreSize = 18.0f * effect->intensity;
        DrawCircle((int)effect->currentPos.x, (int)effect->currentPos.y, coreSize, normalColor);

        // Anel de energia
        Color energyColor = (Color){255, 255, 255, (unsigned char)(180 * effect->alpha)};
        DrawCircleLines((int)effect->currentPos.x, (int)effect->currentPos.y, coreSize * 1.2f, energyColor);

        // Trilha simples
        Vector2 direction = {effect->target.x - effect->origin.x, effect->target.y - effect->origin.y};
        float length = sqrtf(direction.x * direction.x + direction.y * direction.y);
        if (length > 0) {
            direction.x /= length;
            direction.y /= length;
        }

        for (int i = 1; i <= 3; i++) {
            Vector2 trailPos = {
                effect->currentPos.x - direction.x * i * 15,
                effect->currentPos.y - direction.y * i * 15
            };

            float trailAlpha = effect->alpha * (1.0f - (float)i / 4.0f);
            Color trailColor = (Color){200, 200, 200, (unsigned char)(255 * trailAlpha)};
            float trailSize = coreSize * (1.0f - (float)i / 5.0f);

            DrawCircle((int)trailPos.x, (int)trailPos.y, trailSize, trailColor);
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

void CreateDelayedDamageEffect(Vector2 position, int damage, bool isPlayerTarget, bool isCritical, float delay) {
    // Esta função pode ser usada para criar o efeito de dano com delay
    // Por enquanto, mantemos o efeito imediato, mas com duração ajustada
    CreateDamageEffect(position, damage, isPlayerTarget, isCritical);
}