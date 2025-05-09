#include "hud_elements.h"
#include "resources.h"
#include "../monsters.h"
#include "../battle.h"
#include "../game_state.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <curl/curl.h>

extern bool initialized;
extern CURL* curl_handle;

extern BattleSystem* battleSystem;

// Desenha um botão e retorna true se foi clicado
bool drawButton(Rectangle bounds, const char* text, Color color) {
    bool clicked = false;
    Vector2 mousePoint = GetMousePosition();
    
    // Verificar hover
    if (CheckCollisionPointRec(mousePoint, bounds)) {
        // Clarear a cor quando hover
        Color hoverColor = (Color){ 
            (unsigned char)fmin(255, color.r + 40),
            (unsigned char)fmin(255, color.g + 40),
            (unsigned char)fmin(255, color.b + 40),
            color.a
        };
        
        DrawRectangleRounded(bounds, 0.2f, 10, hoverColor);
        
        // Verificar clique
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            clicked = true;
        }
    } else {
        DrawRectangleRounded(bounds, 0.2f, 10, color);
    }
    
    // Desenhar borda
    DrawRectangleRoundedLines(bounds, 0.2f, 10, BLACK);
    
    // Desenhar texto
    Vector2 textSize = MeasureTextEx(gameFont, text, 20, 1);
    DrawTextEx(gameFont, text, (Vector2){ 
        bounds.x + bounds.width/2 - textSize.x/2,
        bounds.y + bounds.height/2 - textSize.y/2
    }, 20, 1, WHITE);
    
    return clicked;
}

// Desenha um card de monstro
void drawMonsterCard(Rectangle bounds, PokeMonster* monster, bool selected) {
    // Desenhar fundo do card
    Color cardColor = selected ? SKYBLUE : LIGHTGRAY;
    DrawRectangleRounded(bounds, 0.2f, 10, cardColor);
    
    // Desenhar borda
    DrawRectangleRoundedLines(bounds, 0.2f, 10, selected ? BLUE : BLACK);
    
    // Desenhar nome
    DrawText(monster->name, bounds.x + 10, bounds.y + 10, 20, BLACK);
    
    // Desenhar tipos
    Rectangle type1Rect = { bounds.x + 10, bounds.y + 40, 60, 25 };
    DrawRectangleRec(type1Rect, getTypeColor(monster->type1));
    DrawText(getTypeName(monster->type1), type1Rect.x + 5, type1Rect.y + 5, 15, WHITE);
    
    if (monster->type2 != TYPE_NONE) {
        Rectangle type2Rect = { bounds.x + 80, bounds.y + 40, 60, 25 };
        DrawRectangleRec(type2Rect, getTypeColor(monster->type2));
        DrawText(getTypeName(monster->type2), type2Rect.x + 5, type2Rect.y + 5, 15, WHITE);
    }
    
    // Desenhar estatísticas básicas
    char stats[100];
    sprintf(stats, "HP: %d  ATK: %d", monster->maxHp, monster->attack);
    DrawText(stats, bounds.x + 10, bounds.y + 70, 15, BLACK);
    
    sprintf(stats, "DEF: %d  SPD: %d", monster->defense, monster->speed);
    DrawText(stats, bounds.x + 10, bounds.y + 90, 15, BLACK);
}

// Desenha ícone de tipo
void drawTypeIcon(Rectangle bounds, MonsterType type) {
    DrawRectangleRec(bounds, getTypeColor(type));
    DrawText(getTypeName(type), bounds.x + 5, bounds.y + 5, 15, WHITE);
}
// Estrutura para armazenar o estado de animação da barra de vida
typedef struct {
   void* monsterId;          // ID único para o monstro (endereço de memória)
   float animatedFillRatio;  // Valor atual da animação
   int lastHP;               // Último valor de HP registrado
   float timer;              // Timer para animação
} HPBarAnimation;

// Array para armazenar animações de múltiplas barras de HP
#define MAX_BARS 10
static HPBarAnimation hpBars[MAX_BARS] = {0};

// Encontrar ou criar uma animação para um monstro específico
HPBarAnimation* getHPBarAnimation(PokeMonster* monster) {
   if (monster == NULL) return NULL;
   
   // Usar endereço do monstro como ID
   void* monsterId = (void*)monster;
   
   // Procurar animação existente
   for (int i = 0; i < MAX_BARS; i++) {
       if (hpBars[i].monsterId == monsterId) {
           return &hpBars[i];
       }
   }
   
   // Criar nova animação
   for (int i = 0; i < MAX_BARS; i++) {
       if (hpBars[i].monsterId == NULL) {
           hpBars[i].monsterId = monsterId;
           hpBars[i].animatedFillRatio = (float)monster->hp / monster->maxHp;
           hpBars[i].lastHP = monster->hp;
           hpBars[i].timer = 0;
           return &hpBars[i];
       }
   }
   
   return NULL; // Não deveria acontecer a menos que MAX_BARS seja muito pequeno
}

// Limpar animações antigas (chame isto quando iniciar uma nova batalha)
void clearHPBarAnimations(void) {
   for (int i = 0; i < MAX_BARS; i++) {
       hpBars[i].monsterId = NULL;
       hpBars[i].animatedFillRatio = 0;
       hpBars[i].lastHP = 0;
       hpBars[i].timer = 0;
   }
}

void drawHealthBar(Rectangle bounds, int currentHP, int maxHP, PokeMonster* monster) {
   static bool battleStarted = false;
   if (!battleStarted && battleSystem != NULL) {
       clearHPBarAnimations();
       battleStarted = true;
   }
   // Obter o estado de animação para este monstro específico
   HPBarAnimation* anim = getHPBarAnimation(monster);
   if (anim == NULL) {
       // Fallback caso não consiga obter a animação
       float fillRatio = (float)currentHP / maxHP;
       if (fillRatio < 0) fillRatio = 0;
       if (fillRatio > 1) fillRatio = 1;
       
       DrawRectangleRec(bounds, BLACK);
       Rectangle innerBounds = {
           bounds.x + 1,
           bounds.y + 1,
           bounds.width - 2,
           bounds.height - 2
       };
       DrawRectangleRec(innerBounds, WHITE);
       
       Color fillColor = fillRatio > 0.5f ? GREEN : (fillRatio > 0.2f ? YELLOW : RED);
       DrawRectangleRec((Rectangle){ 
           innerBounds.x, innerBounds.y, 
           innerBounds.width * fillRatio, innerBounds.height 
       }, fillColor);
       return;
   }
   
   // Calcular preenchimento baseado no HP atual
   float targetFillRatio = (float)currentHP / maxHP;
   if (targetFillRatio < 0) targetFillRatio = 0;
   if (targetFillRatio > 1) targetFillRatio = 1;
   
   // Detectar mudança de HP
   if (anim->lastHP != currentHP) {
       anim->lastHP = currentHP;
       anim->timer = 0;  // Resetar timer quando o HP muda
   }
   
   // Avançar o timer
   anim->timer += GetFrameTime();
   
   // Animação mais lenta e suave (0.5 unidades por segundo)
   float animationSpeed = 0.5f;
   
   // Animação suave da barra
   float delta = targetFillRatio - anim->animatedFillRatio;
   if (fabs(delta) > 0.001f) {
       anim->animatedFillRatio += delta * animationSpeed * GetFrameTime() * 4.0f;
       
       // Garantir que não ultrapasse o objetivo
       if (delta > 0 && anim->animatedFillRatio > targetFillRatio) {
           anim->animatedFillRatio = targetFillRatio;
       } else if (delta < 0 && anim->animatedFillRatio < targetFillRatio) {
           anim->animatedFillRatio = targetFillRatio;
       }
   }
   
   // Desenhar fundo da barra (borda preta)
   DrawRectangleRec(bounds, BLACK);
   
   // Borda interna (espaço para o preenchimento)
   Rectangle innerBounds = {
       bounds.x + 1,
       bounds.y + 1,
       bounds.width - 2,
       bounds.height - 2
   };
   DrawRectangleRec(innerBounds, WHITE);
   
   // Determinar cor baseada no HP - estilo Pokémon
   Color fillColor;
   if (anim->animatedFillRatio > 0.5f) {
       fillColor = (Color){ 0, 200, 80, 255 }; // Verde
   } else if (anim->animatedFillRatio > 0.2f) {
       fillColor = (Color){ 255, 180, 0, 255 }; // Amarelo
   } else {
       fillColor = (Color){ 200, 0, 0, 255 }; // Vermelho
   }
   
   // Desenhar preenchimento com animação
   DrawRectangleRec((Rectangle){ 
       innerBounds.x, innerBounds.y, 
       innerBounds.width * anim->animatedFillRatio, innerBounds.height 
   }, fillColor);
   
   // Adicionar efeito de gradiente sutil na barra de HP
   for (int i = 0; i < innerBounds.height; i += 2) {
       DrawLine(
           innerBounds.x, 
           innerBounds.y + i,
           innerBounds.x + innerBounds.width * anim->animatedFillRatio,
           innerBounds.y + i,
           (Color){ 
               (unsigned char)fmin(255, fillColor.r + 40),
               (unsigned char)fmin(255, fillColor.g + 40),
               (unsigned char)fmin(255, fillColor.b + 40),
               fillColor.a
           }
       );
   }
   
   // Adicionar efeito de "piscada" quando o HP estiver muito baixo
   if (anim->animatedFillRatio <= 0.2f) {
       float blinkRate = 3.0f;  // Frequência de piscada mais lenta
       
       if (sinf(anim->timer * blinkRate) > 0.0f) {
           DrawRectangleRec(
               (Rectangle){ 
                   innerBounds.x, innerBounds.y, 
                   innerBounds.width * anim->animatedFillRatio, innerBounds.height 
               }, 
               (Color){ 255, 255, 255, 100 }
           );
       }
   }
}

// Desenha estatísticas detalhadas de um monstro
void drawMonsterStats(Rectangle bounds, PokeMonster* monster) {
    // Desenhar fundo
    DrawRectangleRounded(bounds, 0.2f, 10, LIGHTGRAY);
    
    // Desenhar nome
    DrawText(monster->name, bounds.x + 20, bounds.y + 20, 30, BLACK);
    
    // Desenhar tipos
    Rectangle type1Rect = { bounds.x + 20, bounds.y + 60, 80, 30 };
    DrawRectangleRec(type1Rect, getTypeColor(monster->type1));
    DrawText(getTypeName(monster->type1), type1Rect.x + 10, type1Rect.y + 5, 20, WHITE);
    
    if (monster->type2 != TYPE_NONE) {
        Rectangle type2Rect = { bounds.x + 110, bounds.y + 60, 80, 30 };
        DrawRectangleRec(type2Rect, getTypeColor(monster->type2));
        DrawText(getTypeName(monster->type2), type2Rect.x + 10, type2Rect.y + 5, 20, WHITE);
    }
    
    // Desenhar estatísticas
    DrawText("Estatísticas:", bounds.x + 20, bounds.y + 100, 25, BLACK);
    
    char statText[50];
    sprintf(statText, "HP: %d", monster->maxHp);
    DrawText(statText, bounds.x + 30, bounds.y + 130, 20, BLACK);
    
    sprintf(statText, "Ataque: %d", monster->attack);
    DrawText(statText, bounds.x + 30, bounds.y + 160, 20, BLACK);
    
    sprintf(statText, "Defesa: %d", monster->defense);
    DrawText(statText, bounds.x + 30, bounds.y + 190, 20, BLACK);
    
    sprintf(statText, "Velocidade: %d", monster->speed);
    DrawText(statText, bounds.x + 30, bounds.y + 220, 20, BLACK);
    
    // Desenhar ataques
    DrawText("Ataques:", bounds.x + bounds.width/2, bounds.y + 100, 25, BLACK);
    
    for (int i = 0; i < 4; i++) {
        Rectangle attackRect = { 
            bounds.x + bounds.width/2, 
            bounds.y + 130 + i * 40, 
            bounds.width/2 - 30, 
            35 
        };
        
        DrawRectangleRec(attackRect, getTypeColor(monster->attacks[i].type));
        DrawText(monster->attacks[i].name, attackRect.x + 10, attackRect.y + 8, 20, WHITE);
        
        char attackInfo[50];
        if (monster->attacks[i].power > 0) {
            sprintf(attackInfo, "Poder: %d  Precisão: %d", 
                   monster->attacks[i].power, monster->attacks[i].accuracy);
        } else {
            sprintf(attackInfo, "Status  Precisão: %d", monster->attacks[i].accuracy);
        }
        
        DrawText(attackInfo, attackRect.x + 10, attackRect.y + 30, 15, BLACK);
    }
}

// Desenha lista de ataques
void drawAttackList(Rectangle bounds, PokeMonster* monster, int selectedAttack) {
   int attackWidth = (bounds.width - 130) / 4;
   
   for (int i = 0; i < 4; i++) {
       Rectangle attackBounds = {
           bounds.x + 10 + i * (attackWidth + 10),
           bounds.y + 10,
           attackWidth,
           60
       };
       
       Color attackColor = getTypeColor(monster->attacks[i].type);
       
       if (monster->attacks[i].ppCurrent <= 0) {
           attackColor.r = attackColor.r / 2;
           attackColor.g = attackColor.g / 2;
           attackColor.b = attackColor.b / 2;
       }
       
       if (monster->attacks[i].ppCurrent > 0 && drawButton(attackBounds, monster->attacks[i].name, attackColor)) {
           PlaySound(selectSound);
           battleSystem->selectedAttack = i;
           
           // Enfileirar ação de ataque
           enqueue(battleSystem->actionQueue, 0, i, battleSystem->playerTeam->current);
           
           // Passar o turno para o bot escolher
           battleSystem->playerTurn = false;
           battleSystem->battleState = BATTLE_SELECT_ACTION;
       } else if (monster->attacks[i].ppCurrent <= 0) {
           DrawRectangleRounded(attackBounds, 0.2f, 10, attackColor);
           DrawRectangleRoundedLines(attackBounds, 0.2f, 10, BLACK);
           
           Vector2 textSize = MeasureTextEx(gameFont, monster->attacks[i].name, 20, 1);
           DrawTextEx(gameFont, monster->attacks[i].name, (Vector2){ 
               attackBounds.x + attackBounds.width/2 - textSize.x/2,
               attackBounds.y + 10
           }, 20, 1, WHITE);
           
           char ppText[20];
           sprintf(ppText, "PP: %d/%d", monster->attacks[i].ppCurrent, monster->attacks[i].ppMax);
           DrawText(ppText, attackBounds.x + 10, attackBounds.y + 35, 15, WHITE);
       } else {
           char ppText[20];
           sprintf(ppText, "PP: %d/%d", monster->attacks[i].ppCurrent, monster->attacks[i].ppMax);
           DrawText(ppText, attackBounds.x + 10, attackBounds.y + 35, 15, WHITE);
       }
   }
}

// Desenha caixa de mensagem
void drawMessageBox(Rectangle bounds, const char* message) {
    // Desenhar fundo
    DrawRectangleRounded(bounds, 0.2f, 10, WHITE);
    DrawRectangleRoundedLines(bounds, 0.2f, 10, BLACK);
    
    // Desenhar texto
    DrawText(message, bounds.x + 15, bounds.y + bounds.height/2 - 10, 20, BLACK);
}

// Desenha diálogo de confirmação
void drawConfirmationDialog(const char* message, const char* yesText, const char* noText) {
    // Desenhar fundo semi-transparente
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){ 0, 0, 0, 150 });
    
    // Desenhar caixa de diálogo
    Rectangle dialogBox = {
        GetScreenWidth()/2 - 200,
        GetScreenHeight()/2 - 100,
        400,
        200
    };
    
    DrawRectangleRounded(dialogBox, 0.2f, 10, WHITE);
    DrawRectangleRoundedLines(dialogBox, 0.2f, 10, BLACK);
    
    // Desenhar mensagem
    Vector2 textSize = MeasureTextEx(gameFont, message, 20, 1);
    DrawTextEx(gameFont, message, (Vector2){ 
        dialogBox.x + dialogBox.width/2 - textSize.x/2,
        dialogBox.y + 40
    }, 20, 1, BLACK);
    
    // Desenhar botões
    Rectangle yesButton = {
        dialogBox.x + 50,
        dialogBox.y + 120,
        100,
        40
    };
    
    Rectangle noButton = {
        dialogBox.x + 250,
        dialogBox.y + 120,
        100,
        40
    };
    
    if (drawButton(yesButton, yesText, RED)) {
        PlaySound(selectSound);
        // Voltar ao menu principal
        StopMusicStream(battleMusic);
        PlayMusicStream(menuMusic);
        currentScreen = MAIN_MENU;
        resetBattle();
    }
    
    if (drawButton(noButton, noText, GREEN)) {
        PlaySound(selectSound);
        // Continuar a batalha
        battleSystem->battleState = BATTLE_SELECT_ACTION;
    }
}

// Função auxiliar para desenhar o indicador de IA
void drawAIIndicator(void) {
   Rectangle indicator = { GetScreenWidth() - 80, 5, 75, 25 };
   Color indicatorColor;
   const char* text;
   
   // Verificar se a IA está disponível
   if (initialized && curl_handle != NULL) {
       indicatorColor = GREEN;
       text = "IA ON";
   } else {
       indicatorColor = YELLOW;
       text = "IA OFF";
   }
   
   DrawRectangleRounded(indicator, 0.3f, 8, indicatorColor);
   DrawRectangleRoundedLines(indicator, 0.3f, 8, BLACK);
   DrawText(text, indicator.x + 8, indicator.y + 4, 18, WHITE);
}