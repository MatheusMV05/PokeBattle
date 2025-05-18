#ifndef HP_BAR_H
#define HP_BAR_H

#include <stdint.h> // Para uintptr_t
#include "raylib.h" // Supondo uso de Raylib

typedef struct PokeMonster PokeMonster; // Forward declaration

// Estrutura para armazenar o estado de animação da barra de vida
typedef struct {
    uintptr_t monsterId;      // ID único baseado no endereço do monstro
    float animatedFillRatio;  // Valor atual da animação
    int lastHP;               // Último valor de HP registrado
    float timer;              // Timer para animação
    bool needsReset;          // Flag para controle de reset
} HPBarAnimation;

// Interface das funções
void InitHPBarSystem(void);
void ResetHPBarAnimations(void);
void DrawHealthBar(Rectangle bounds, int currentHP, int maxHP, const PokeMonster* monster);

#endif // HP_BAR_H
