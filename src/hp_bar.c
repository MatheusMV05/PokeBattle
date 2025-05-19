#include "hp_bar.h"
#include <math.h> // Para funções min/max
#include <stdio.h>
#include "structures.h"

// Armazenamento para animações de várias barras de HP
#define MAX_BARS 10
static HPBarAnimation hpBars[MAX_BARS] = {0};

// Inicializa o sistema de barras de vida
void InitHPBarSystem(void) {
    for (int i = 0; i < MAX_BARS; i++) {
        hpBars[i].monsterId = 0;
        hpBars[i].animatedFillRatio = 0.0f;
        hpBars[i].lastHP = 0;
        hpBars[i].timer = 0.0f;
        hpBars[i].needsReset = true;
    }
}

// Marca todas as animações para reset
void ResetHPBarAnimations(void) {
    for (int i = 0; i < MAX_BARS; i++) {
        hpBars[i].needsReset = true;
    }
}

// Obtém ou cria uma animação para o monstro específico
static HPBarAnimation* GetHPBarAnimation(const PokeMonster* monster) {
    if (monster == NULL) return NULL;

    // Usamos o endereço do monstro como identificador único
    const uintptr_t monsterId = (uintptr_t)monster;

    // Primeira passada: procurar por entradas existentes
    for (int i = 0; i < MAX_BARS; i++) {
        if (hpBars[i].monsterId == monsterId) {
            if (hpBars[i].needsReset) {
                // Resetar valores se necessário
                hpBars[i].animatedFillRatio = (float)monster->hp / monster->maxHp;
                hpBars[i].lastHP = monster->hp;
                hpBars[i].timer = 0.0f;
                hpBars[i].needsReset = false;
            }
            return &hpBars[i];
        }
    }

    // Segunda passada: procurar por slots vazios ou para reset
    for (int i = 0; i < MAX_BARS; i++) {
        if (hpBars[i].monsterId == 0 || hpBars[i].needsReset) {
            hpBars[i].monsterId = monsterId;
            hpBars[i].animatedFillRatio = (float)monster->hp / monster->maxHp;
            hpBars[i].lastHP = monster->hp;
            hpBars[i].timer = 0.0f;
            hpBars[i].needsReset = false;
            return &hpBars[i];
        }
    }

    return NULL; // Nenhum slot disponível
}

// Função principal de desenho da barra de vida
void DrawHealthBar(Rectangle bounds, int currentHP, int maxHP, const PokeMonster* monster) {
    if (maxHP <= 0) return; // Prevenir divisão por zero

    // Debug para verificar os valores


    // Obter animação específica para este monstro
    HPBarAnimation* anim = GetHPBarAnimation(monster);
    if (!anim) return;

    // Atualizar valores da animação
    const float targetFillRatio = (float)currentHP / maxHP;

    // Detectar mudança de HP
    if (anim->lastHP != currentHP) {
        anim->lastHP = currentHP;
        anim->timer = 0.0f; // Reiniciar timer para nova animação
    }

    // Atualizar timer
    anim->timer += GetFrameTime();

    // Suavizar animação
    const float animationSpeed = 0.5f; // Ajuste para velocidade desejada
    const float delta = targetFillRatio - anim->animatedFillRatio;

    if (fabsf(delta) > 0.001f) {
        anim->animatedFillRatio += delta * animationSpeed * GetFrameTime() * 4.0f;

        // Clamping para evitar overshooting
        if ((delta > 0 && anim->animatedFillRatio > targetFillRatio) ||
            (delta < 0 && anim->animatedFillRatio < targetFillRatio)) {
            anim->animatedFillRatio = targetFillRatio;
        }
    }

    // Desenhar fundo
    DrawRectangleRec(bounds, BLACK);

    // Borda interna
    const Rectangle innerBounds = {
        .x = bounds.x + 1,
        .y = bounds.y + 1,
        .width = bounds.width - 2,
        .height = bounds.height - 2
    };
    DrawRectangleRec(innerBounds, WHITE);

    // Calcular dimensões do preenchimento
    const float fillWidth = innerBounds.width * anim->animatedFillRatio;
    const Rectangle fillRect = {
        .x = innerBounds.x,
        .y = innerBounds.y,
        .width = fillWidth > 0 ? fillWidth : 0,
        .height = innerBounds.height
    };

    // Escolher cor baseada no HP
    Color fillColor;
    if (anim->animatedFillRatio > 0.5f) {
        fillColor = (Color){0, 200, 80, 255};    // Verde
    } else if (anim->animatedFillRatio > 0.2f) {
        fillColor = (Color){255, 180, 0, 255};   // Amarelo
    } else {
        fillColor = (Color){200, 0, 0, 255};     // Vermelho
    }

    // Desenhar preenchimento
    DrawRectangleRec(fillRect, fillColor);

    // Efeito de gradiente
    for (int i = 0; i < fillRect.height; i += 2) {
        const Color gradientColor = {
            .r = (uint8_t)fminf(255, fillColor.r + 40),
            .g = (uint8_t)fminf(255, fillColor.g + 40),
            .b = (uint8_t)fminf(255, fillColor.b + 40),
            .a = fillColor.a
        };
        DrawLineEx(
            (Vector2){fillRect.x, fillRect.y + i},
            (Vector2){fillRect.x + fillRect.width, fillRect.y + i},
            1.0f,
            gradientColor
        );
    }

    // Efeito de piscar quando HP está crítico
    if (anim->animatedFillRatio <= 0.2f) {
        const float blinkRate = 3.0f;
        if (sinf(anim->timer * blinkRate * 2 * PI) > 0.0f) {
            DrawRectangleRec(
                fillRect,
                Fade(WHITE, 0.3f)
            );
        }
    }
}
