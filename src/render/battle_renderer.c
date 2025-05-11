// battle_renderer.c - Sistema completo de renderização de batalha estilo Pokémon FireRed
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#endif

#include "battle_renderer.h"
#include "hud_elements.h"
#include "resources.h"
#include "monsters.h"
#include "game_state.h"
#include "ia_integration.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

// Declaração de funções de sistema de batalha
void displayBattleMessage(const char* message, float duration, bool waitForInput, bool autoAdvance);

// Declaração de variáveis globais externas
extern BattleMessage currentMessage;
extern BattleAnimation currentAnimation;
extern bool actionQueueReady;
extern BattleSystem* battleSystem;

// Variáveis locais
static char battleMessage[256] = "";
static BattleEffect effects[MAX_EFFECTS] = {0};

// Sistema de typewriter para textos
typedef struct {
    char fullText[256];         // Texto completo
    char displayText[256];      // Texto atualmente exibido
    float charTimer;            // Timer para próximo caractere
    int currentChar;            // Índice do caractere atual
    bool isComplete;            // Se terminou de digitar
    float charDelay;            // Delay entre caracteres
    bool waitingForInput;       // Se está esperando input do usuário
    float blinkTimer;           // Timer para piscar o indicador de continuar
} TypewriterText;

static TypewriterText typewriter = {0};

// Velocidade do Typewriter ajustada para FireRed
#define TYPEWRITER_SPEED 0.02f

// Escalas e configurações visuais estilo FireRed
#define UI_SCALE 2.0f
#define FONT_SIZE_SMALL (8 * UI_SCALE)
#define FONT_SIZE_LARGE (10 * UI_SCALE)
#define PADDING (4 * UI_SCALE)

// Cores do estilo FireRed
#define COLOR_UI_BG (Color){248, 248, 248, 255}
#define COLOR_BOX_BG (Color){255, 255, 255, 255}
#define COLOR_BOX_BORDER (Color){40, 40, 40, 255}
#define COLOR_BOX_SHADOW (Color){200, 200, 200, 255}
#define COLOR_TEXT_MAIN (Color){32, 32, 32, 255}
#define COLOR_HP_GREEN (Color){32, 200, 48, 255}
#define COLOR_HP_YELLOW (Color){200, 168, 8, 255}
#define COLOR_HP_RED (Color){200, 0, 0, 255}
#define COLOR_EXP_BLUE (Color){80, 184, 248, 255}
#define COLOR_PLATFORM_ENEMY (Color){208, 208, 200, 255}
#define COLOR_PLATFORM_PLAYER (Color){168, 168, 160, 255}
#define COLOR_MALE_GENDER (Color){0, 144, 240, 255}
#define COLOR_FEMALE_GENDER (Color){248, 96, 104, 255}

// Backgrounds disponíveis para batalha
extern Texture2D battleBackgrounds[BATTLE_BACKGROUNDS_COUNT];
extern int currentBattleBackground;

/**
 * Inicia o efeito typewriter para um novo texto
 * @param text Texto a ser exibido
 * @param waitForInput Se deve esperar input do usuário após terminar
 */
void startTypewriter(const char* text, bool waitForInput) {
    if (text == NULL) return;

    strncpy(typewriter.fullText, text, sizeof(typewriter.fullText) - 1);
    typewriter.fullText[sizeof(typewriter.fullText) - 1] = '\0';

    typewriter.displayText[0] = '\0';
    typewriter.charTimer = 0.0f;
    typewriter.currentChar = 0;
    typewriter.isComplete = false;
    typewriter.charDelay = TYPEWRITER_SPEED;
    typewriter.waitingForInput = waitForInput;
    typewriter.blinkTimer = 0.0f;
}

/**
 * Atualiza o efeito typewriter
 */
void updateTypewriter(void) {
    if (typewriter.isComplete) {
        // Atualizar o timer de piscar
        typewriter.blinkTimer += GetFrameTime() * 3.0f;
        return;
    }

    typewriter.charTimer += GetFrameTime();

    // Avançar para o próximo caractere
    while (typewriter.charTimer >= typewriter.charDelay && !typewriter.isComplete) {
        if (typewriter.currentChar < strlen(typewriter.fullText)) {
            // Adicionar próximo caractere
            typewriter.displayText[typewriter.currentChar] = typewriter.fullText[typewriter.currentChar];
            typewriter.displayText[typewriter.currentChar + 1] = '\0';
            typewriter.currentChar++;

            // Resetar timer
            typewriter.charTimer -= typewriter.charDelay;

            // Se for espaço ou pontuação, adicionar uma pequena pausa
            char lastChar = typewriter.fullText[typewriter.currentChar - 1];
            if (lastChar == ' ' || lastChar == ',' || lastChar == '.') {
                typewriter.charTimer -= typewriter.charDelay * 0.5f;
            }
        } else {
            // Texto completo
            typewriter.isComplete = true;
            typewriter.blinkTimer = 0.0f;
        }
    }

    // Permitir pular o efeito com clique ou tecla
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) {
        if (!typewriter.isComplete) {
            // Completar instantaneamente
            strcpy(typewriter.displayText, typewriter.fullText);
            typewriter.currentChar = strlen(typewriter.fullText);
            typewriter.isComplete = true;
            typewriter.blinkTimer = 0.0f;
        } else if (typewriter.waitingForInput) {
            // Se já completou e está esperando input, avançar
            // Isso deve ser tratado pela lógica de batalha
        }
    }
}

/**
 * Desenha o texto com efeito typewriter
 * @param position Posição do texto
 * @param fontSize Tamanho da fonte
 * @param color Cor do texto
 */
void drawTypewriterText(Vector2 position, float fontSize, Color color) {
    // Desenhar o texto atual
    DrawText(typewriter.displayText, position.x, position.y, fontSize, color);

    // Se o texto estiver completo e esperando input, desenhar indicador de continuar
    if (typewriter.isComplete && typewriter.waitingForInput) {
        if (sinf(typewriter.blinkTimer) > 0.0f) {
            // Triângulo pequeno piscando
            Vector2 v1 = {position.x + MeasureText(typewriter.displayText, fontSize) + 10, position.y + fontSize/2};
            Vector2 v2 = {v1.x + 8, v1.y};
            Vector2 v3 = {v1.x + 4, v1.y + 6};
            DrawTriangle(v1, v2, v3, color);
        }
    }
}

/**
 * Verifica se o efeito typewriter está completo
 */
bool isTypewriterComplete(void) {
    return typewriter.isComplete;
}

/**
 * Verifica se o typewriter está esperando input
 */
bool isTypewriterWaitingInput(void) {
    return typewriter.isComplete && typewriter.waitingForInput;
}

/**
 * Desenha uma caixa estilo FireRed com bordas e sombra
 * @param bounds Retângulo da caixa
 * @param isSelected Se a caixa está selecionada (adiciona destaque)
 */
void drawFireRedBox(Rectangle bounds, bool isSelected) {
    // Sombra inferior direita
    Rectangle shadowRect = {
        bounds.x + 2,
        bounds.y + 2,
        bounds.width,
        bounds.height
    };
    DrawRectangleRec(shadowRect, COLOR_BOX_SHADOW);

    // Borda principal preta
    DrawRectangleRec(bounds, COLOR_BOX_BORDER);

    // Fundo branco interno
    Rectangle innerRect = {
        bounds.x + 2,
        bounds.y + 2,
        bounds.width - 4,
        bounds.height - 4
    };
    DrawRectangleRec(innerRect, COLOR_BOX_BG);

    // Destaque se selecionado
    if (isSelected) {
        DrawRectangleLinesEx(bounds, 2, COLOR_TEXT_MAIN);
    }
}

/**
 * Desenha a barra de HP estilo FireRed
 * @param position Posição da barra
 * @param width Largura da barra
 * @param currentHP HP atual
 * @param maxHP HP máximo
 */
void drawFireRedHPBar(Vector2 position, float width, float currentHP, float maxHP) {
    float hpPercentage = (maxHP > 0) ? (currentHP / maxHP) : 0;

    // Borda da barra
    Rectangle borderRect = {position.x, position.y, width, 6 * UI_SCALE};
    DrawRectangleRec(borderRect, COLOR_BOX_BORDER);

    // Fundo branco
    Rectangle bgRect = {
        position.x + 1,
        position.y + 1,
        width - 2,
        4 * UI_SCALE
    };
    DrawRectangleRec(bgRect, COLOR_BOX_BG);

    // Cor baseada na porcentagem de HP
    Color hpColor = COLOR_HP_GREEN;
    if (hpPercentage <= 0.2f) hpColor = COLOR_HP_RED;
    else if (hpPercentage <= 0.5f) hpColor = COLOR_HP_YELLOW;

    // Preenchimento da HP
    if (hpPercentage > 0) {
        Rectangle hpRect = {
            position.x + 1,
            position.y + 1,
            (width - 2) * hpPercentage,
            4 * UI_SCALE
        };
        DrawRectangleRec(hpRect, hpColor);
    }
}

/**
 * Desenha a barra de EXP estilo FireRed
 * @param position Posição da barra
 * @param width Largura da barra
 * @param expPercentage Porcentagem de experiência
 */
void drawFireRedEXPBar(Vector2 position, float width, float expPercentage) {
    // Borda da barra
    Rectangle borderRect = {position.x, position.y, width, 4 * UI_SCALE};
    DrawRectangleRec(borderRect, COLOR_BOX_BORDER);

    // Fundo branco
    Rectangle bgRect = {
        position.x + 1,
        position.y + 1,
        width - 2,
        2 * UI_SCALE
    };
    DrawRectangleRec(bgRect, COLOR_BOX_BG);

    // Preenchimento da EXP
    if (expPercentage > 0) {
        Rectangle expRect = {
            position.x + 1,
            position.y + 1,
            (width - 2) * expPercentage,
            2 * UI_SCALE
        };
        DrawRectangleRec(expRect, COLOR_EXP_BLUE);
    }
}

/**
 * Desenha a caixa de status do oponente
 * @param monster Monstro do oponente
 */
void drawOpponentStatusBox(PokeMonster* monster) {
    if (monster == NULL) return;

    // Posição e tamanho da caixa
    Rectangle statusBox = {
        8 * UI_SCALE,
        8 * UI_SCALE,
        96 * UI_SCALE,
        30 * UI_SCALE
    };

    drawFireRedBox(statusBox, false);

    // Nome do Pokémon
    Vector2 namePos = {statusBox.x + 4 * UI_SCALE, statusBox.y + 4 * UI_SCALE};
    DrawText(monster->name, namePos.x, namePos.y, FONT_SIZE_SMALL, COLOR_TEXT_MAIN);

    // Símbolo de gênero (exemplo - sempre macho por enquanto)
    Vector2 genderPos = {namePos.x + MeasureText(monster->name, FONT_SIZE_SMALL) + 2 * UI_SCALE, namePos.y};
    DrawText("♂", genderPos.x, genderPos.y, FONT_SIZE_SMALL, COLOR_MALE_GENDER);

    // Nível
    Vector2 levelPos = {statusBox.x + statusBox.width - 25 * UI_SCALE, namePos.y};
    DrawText("Lv5", levelPos.x, levelPos.y, FONT_SIZE_SMALL, COLOR_TEXT_MAIN);

    // Label HP
    Vector2 hpLabelPos = {statusBox.x + 4 * UI_SCALE, statusBox.y + 16 * UI_SCALE};
    DrawText("HP", hpLabelPos.x, hpLabelPos.y, FONT_SIZE_SMALL, COLOR_TEXT_MAIN);

    // Barra de HP
    Vector2 hpBarPos = {hpLabelPos.x + 16 * UI_SCALE, hpLabelPos.y + UI_SCALE};
    drawFireRedHPBar(hpBarPos, 48 * UI_SCALE, monster->hp, monster->maxHp);
}

/**
 * Desenha a caixa de status do jogador
 * @param monster Monstro do jogador
 */
void drawPlayerStatusBox(PokeMonster* monster) {
    if (monster == NULL) return;

    // Posição e tamanho da caixa
    Rectangle statusBox = {
        GetScreenWidth() - (100 * UI_SCALE) - (8 * UI_SCALE),
        GetScreenHeight() - (40 * UI_SCALE) - (48 * UI_SCALE) - (12 * UI_SCALE),
        100 * UI_SCALE,
        40 * UI_SCALE
    };

    drawFireRedBox(statusBox, false);

    // Nome do Pokémon
    Vector2 namePos = {statusBox.x + 4 * UI_SCALE, statusBox.y + 4 * UI_SCALE};
    DrawText(monster->name, namePos.x, namePos.y, FONT_SIZE_SMALL, COLOR_TEXT_MAIN);

    // Símbolo de gênero
    Vector2 genderPos = {namePos.x + MeasureText(monster->name, FONT_SIZE_SMALL) + 2 * UI_SCALE, namePos.y};
    DrawText("♂", genderPos.x, genderPos.y, FONT_SIZE_SMALL, COLOR_MALE_GENDER);

    // Nível
    Vector2 levelPos = {statusBox.x + statusBox.width - 25 * UI_SCALE, namePos.y};
    DrawText("Lv5", levelPos.x, levelPos.y, FONT_SIZE_SMALL, COLOR_TEXT_MAIN);

    // Barra de HP
    Vector2 hpBarPos = {statusBox.x + 20 * UI_SCALE, statusBox.y + 16 * UI_SCALE};
    drawFireRedHPBar(hpBarPos, 48 * UI_SCALE, monster->hp, monster->maxHp);

    // Texto de HP
    char hpText[16];
    sprintf(hpText, "%d/%d", monster->hp, monster->maxHp);
    Vector2 hpTextPos = {hpBarPos.x + 48 * UI_SCALE + 4 * UI_SCALE, hpBarPos.y};
    DrawText(hpText, hpTextPos.x, hpTextPos.y, FONT_SIZE_SMALL, COLOR_TEXT_MAIN);

    // Label EXP
    Vector2 expLabelPos = {statusBox.x + 4 * UI_SCALE, statusBox.y + 28 * UI_SCALE};
    DrawText("EXP", expLabelPos.x, expLabelPos.y, 6 * UI_SCALE, COLOR_TEXT_MAIN);

    // Barra de EXP
    Vector2 expBarPos = {expLabelPos.x + 18 * UI_SCALE, expLabelPos.y + UI_SCALE};
    drawFireRedEXPBar(expBarPos, 64 * UI_SCALE, 0.1f); // 10% de exemplo
}

/**
 * Função principal para desenhar a tela de batalha
 * Gerencia todos os estados visuais da batalha
 */
void drawBattleScreen(void) {
    // Atualizar música da batalha
    UpdateMusicStream(battleMusic);

    // Desenhar background
    if (currentBattleBackground >= 0 &&
        currentBattleBackground < BATTLE_BACKGROUNDS_COUNT &&
        battleBackgrounds[currentBattleBackground].id != 0) {

        DrawTexturePro(
            battleBackgrounds[currentBattleBackground],
            (Rectangle){ 0, 0,
                        (float)battleBackgrounds[currentBattleBackground].width,
                        (float)battleBackgrounds[currentBattleBackground].height },
            (Rectangle){ 0, 0,
                        (float)GetScreenWidth(),
                        (float)GetScreenHeight() },
            (Vector2){ 0, 0 },
            0.0f,
            WHITE
        );
    } else {
        // Fundo padrão caso não tenha background
        ClearBackground(COLOR_UI_BG);
    }

    // Verificar se o sistema de batalha está válido
    if (battleSystem != NULL &&
        battleSystem->playerTeam != NULL && battleSystem->playerTeam->current != NULL &&
        battleSystem->opponentTeam != NULL && battleSystem->opponentTeam->current != NULL) {

        PokeMonster* playerMonster = battleSystem->playerTeam->current;
        PokeMonster* opponentMonster = battleSystem->opponentTeam->current;

        // Desenhar plataformas estilo FireRed
        // Plataforma do oponente (elipse cinza clara)
        Vector2 enemyPlatformCenter = {
            GetScreenWidth() * 0.72f,
            GetScreenHeight() * 0.35f
        };
        DrawEllipse(enemyPlatformCenter.x, enemyPlatformCenter.y,
                   100 * UI_SCALE, 20 * UI_SCALE, COLOR_PLATFORM_ENEMY);

        // Plataforma do jogador (elipse cinza mais escura)
        Vector2 playerPlatformCenter = {
            GetScreenWidth() * 0.30f,
            GetScreenHeight() * 0.75f
        };
        DrawEllipse(playerPlatformCenter.x, playerPlatformCenter.y,
                   90 * UI_SCALE, 15 * UI_SCALE, COLOR_PLATFORM_PLAYER);

        // Posições dos sprites
        Rectangle opponentRect = {
            enemyPlatformCenter.x - 64,
            enemyPlatformCenter.y - 140,
            128,
            128
        };

        Rectangle playerRect = {
            playerPlatformCenter.x - 64,
            playerPlatformCenter.y - 140,
            128,
            128
        };

        // Desenhar os monstros
        drawMonsterInBattle(opponentMonster, opponentRect, false);
        drawMonsterInBattle(playerMonster, playerRect, true);

        // Desenhar caixas de status
        drawOpponentStatusBox(opponentMonster);
        drawPlayerStatusBox(playerMonster);

        // Caixa principal de ação/mensagem
        Rectangle actionBox = {
            4 * UI_SCALE,
            GetScreenHeight() - (48 * UI_SCALE) - (4 * UI_SCALE),
            GetScreenWidth() - (8 * UI_SCALE),
            48 * UI_SCALE
        };

        drawFireRedBox(actionBox, false);

        // Atualizar o typewriter para mensagens de batalha
        static BattleState lastStateForTypewriter = BATTLE_IDLE;

        // Verificar se mudou de estado e precisa iniciar novo texto
        if (battleSystem->battleState != lastStateForTypewriter) {
            switch (battleSystem->battleState) {
                case BATTLE_INTRO:
                    startTypewriter("Uma batalha selvagem começou!", true);
                    break;
                case BATTLE_MESSAGE_DISPLAY:
                    if (strlen(currentMessage.message) > 0) {
                        startTypewriter(currentMessage.message, currentMessage.waitingForInput);
                    }
                    break;
                // Outros estados podem iniciar typewriter conforme necessário
            }
            lastStateForTypewriter = battleSystem->battleState;
        }

        // Desenhar conteúdo baseado no estado atual da batalha
        switch (battleSystem->battleState) {
            case BATTLE_INTRO:
                // Usar typewriter para mensagem de introdução
                updateTypewriter();
                drawTypewriterText((Vector2){actionBox.x + 12 * UI_SCALE, actionBox.y + 16 * UI_SCALE},
                                  FONT_SIZE_SMALL, COLOR_TEXT_MAIN);

                // Se o texto terminou, aguardar um momento antes de prosseguir
                if (isTypewriterComplete() && typewriter.blinkTimer > 2.0f) {
                    battleSystem->battleState = BATTLE_SELECT_ACTION;
                }
                break;

            case BATTLE_SELECT_ACTION:
                // Menu principal de ação do jogador
                if (battleSystem->playerTurn) {
                    // Verificar se o monstro do jogador desmaiou
                    if (isMonsterFainted(battleSystem->playerTeam->current)) {
                        battleSystem->battleState = BATTLE_FORCED_SWITCH;
                        return;
                    }

                    // Variável estática para controlar o início do texto
                    static bool actionTextStarted = false;
                    static BattleState lastActionState = BATTLE_IDLE;

                    // Reset quando o estado muda
                    if (lastActionState != battleSystem->battleState) {
                        actionTextStarted = false;
                        lastActionState = battleSystem->battleState;
                    }

                    // Texto com typewriter
                    if (!actionTextStarted) {
                        char actionText[128];
                        sprintf(actionText, "O que %s irá fazer?", playerMonster->name);
                        startTypewriter(actionText, false);
                        actionTextStarted = true;
                    }

                    updateTypewriter();
                    drawTypewriterText((Vector2){actionBox.x + 8 * UI_SCALE, actionBox.y + 16 * UI_SCALE},
                                      FONT_SIZE_SMALL, COLOR_TEXT_MAIN);

                    // Menu de opções (FIGHT, BAG, POKEMON, RUN) - só aparece após o texto
                    if (isTypewriterComplete()) {
                        Rectangle menuSection = {
                            actionBox.x + 140 * UI_SCALE,
                            actionBox.y + 4 * UI_SCALE,
                            90 * UI_SCALE,
                            actionBox.height - 8 * UI_SCALE
                        };

                        // Opções do menu em layout 2x2
                        const char* options[] = {"FIGHT", "BAG", "POKéMON", "RUN"};
                        Color optionColors[] = {
                            (Color){ 240, 80, 80, 255 },   // FIGHT - Vermelho
                            (Color){ 120, 120, 255, 255 }, // BAG - Azul
                            (Color){ 120, 200, 80, 255 },  // POKEMON - Verde
                            (Color){ 200, 120, 200, 255 }  // RUN - Roxo
                        };

                        // Posições das opções
                        Vector2 positions[] = {
                            {menuSection.x + 4 * UI_SCALE, menuSection.y + 8 * UI_SCALE},
                            {menuSection.x + 50 * UI_SCALE, menuSection.y + 8 * UI_SCALE},
                            {menuSection.x + 4 * UI_SCALE, menuSection.y + 28 * UI_SCALE},
                            {menuSection.x + 50 * UI_SCALE, menuSection.y + 28 * UI_SCALE}
                        };

                        // Desenhar opções e processar cliques
                        for (int i = 0; i < 4; i++) {
                            Rectangle optionRect = {
                                positions[i].x - 2 * UI_SCALE,
                                positions[i].y - 2 * UI_SCALE,
                                45 * UI_SCALE,
                                18 * UI_SCALE
                            };

                            bool isHovered = CheckCollisionPointRec(GetMousePosition(), optionRect);

                            // Verificar se a opção está desabilitada
                            bool isDisabled = false;
                            if (i == 1 && battleSystem->itemUsed) { // BAG button quando item já foi usado
                                isDisabled = true;
                            }

                            if (isHovered && !isDisabled) {
                                DrawRectangleRec(optionRect, Fade(optionColors[i], 0.3f));

                                // Cursor triangular
                                Vector2 v1 = {positions[i].x - 10 * UI_SCALE, positions[i].y + 4 * UI_SCALE};
                                Vector2 v2 = {v1.x + 6 * UI_SCALE, v1.y + 4 * UI_SCALE};
                                Vector2 v3 = {v1.x, v1.y + 8 * UI_SCALE};
                                DrawTriangle(v1, v2, v3, COLOR_TEXT_MAIN);
                            }

                            // Desenhar texto com cor diferente se desabilitado
                            Color textColor = isDisabled ? GRAY : COLOR_TEXT_MAIN;
                            DrawText(options[i], positions[i].x, positions[i].y,
                                    FONT_SIZE_SMALL, textColor);

                            // Processar clique apenas se não estiver desabilitado
                            if (isHovered && !isDisabled && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                                PlaySound(selectSound);
                                actionTextStarted = false;
                                switch (i) {
                                    case 0: // FIGHT
                                        battleSystem->battleState = BATTLE_SELECT_ATTACK;
                                        break;
                                    case 1: // BAG
                                        battleSystem->battleState = BATTLE_ITEM_MENU;
                                        break;
                                    case 2: // POKEMON
                                        battleSystem->battleState = BATTLE_SELECT_MONSTER;
                                        break;
                                    case 3: // RUN
                                        battleSystem->battleState = BATTLE_CONFIRM_QUIT;
                                        break;
                                }
                            }
                        }
                    }
                } else {
                    // Mensagem de espera enquanto o oponente decide
                    startTypewriter("Oponente está pensando...", false);
                    updateTypewriter();
                    drawTypewriterText((Vector2){actionBox.x + 12 * UI_SCALE, actionBox.y + 16 * UI_SCALE},
                                      FONT_SIZE_SMALL, COLOR_TEXT_MAIN);
                }
                break;

            case BATTLE_SELECT_ATTACK:
                // Seleção de ataque
                if (battleSystem->playerTurn) {
                    DrawText("ATAQUES:", actionBox.x + 8 * UI_SCALE,
                            actionBox.y + 4 * UI_SCALE, FONT_SIZE_SMALL, COLOR_TEXT_MAIN);

                    // Grid 2x2 para ataques
                    int attackWidth = (actionBox.width - 24 * UI_SCALE) / 2;

                    for (int i = 0; i < 4; i++) {
                        int row = i / 2;
                        int col = i % 2;

                        Rectangle attackRect = {
                            actionBox.x + 8 * UI_SCALE + col * (attackWidth + 8 * UI_SCALE),
                            actionBox.y + 20 * UI_SCALE + row * 15 * UI_SCALE,
                            attackWidth,
                            12 * UI_SCALE
                        };

                        Color attackColor = getTypeColor(playerMonster->attacks[i].type);
                        bool canUse = playerMonster->attacks[i].ppCurrent > 0;

                        if (!canUse) {
                            attackColor = GRAY;
                        }

                        // Background do ataque
                        DrawRectangleRec(attackRect, Fade(attackColor, 0.7f));
                        DrawRectangleLinesEx(attackRect, 1, COLOR_BOX_BORDER);

                        // Nome do ataque
                        DrawText(playerMonster->attacks[i].name,
                                attackRect.x + 2 * UI_SCALE,
                                attackRect.y + 1 * UI_SCALE,
                                FONT_SIZE_SMALL,
                                canUse ? WHITE : LIGHTGRAY);

                        // PP
                        char ppText[16];
                        sprintf(ppText, "PP %d/%d",
                               playerMonster->attacks[i].ppCurrent,
                               playerMonster->attacks[i].ppMax);
                        DrawText(ppText,
                                attackRect.x + attackRect.width - MeasureText(ppText, 6 * UI_SCALE) - 2 * UI_SCALE,
                                attackRect.y + 1 * UI_SCALE,
                                6 * UI_SCALE,
                                canUse ? WHITE : LIGHTGRAY);

                        // Interação com mouse
                        if (canUse && CheckCollisionPointRec(GetMousePosition(), attackRect)) {
                            DrawRectangleLinesEx(attackRect, 2, WHITE);

                            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                                PlaySound(selectSound);
                                battleSystem->selectedAttack = i;

                                // Enfileirar ação de ataque
                                enqueue(battleSystem->actionQueue, 0, i, battleSystem->playerTeam->current);

                                // Passar turno para o bot
                                battleSystem->playerTurn = false;
                                battleSystem->battleState = BATTLE_SELECT_ACTION;
                            }
                        }
                    }

                    // Botão voltar
                    Rectangle backBtn = {actionBox.x + actionBox.width - 50 * UI_SCALE,
                                       actionBox.y + 2 * UI_SCALE,
                                       48 * UI_SCALE,
                                       10 * UI_SCALE};

                    if (drawButton(backBtn, "VOLTAR", GRAY)) {
                        PlaySound(selectSound);
                        battleSystem->battleState = BATTLE_SELECT_ACTION;
                    }
                }
                break;

            case BATTLE_MESSAGE_DISPLAY:
                // Exibir mensagem atual com typewriter
                updateTypewriter();
                drawTypewriterText((Vector2){actionBox.x + 8 * UI_SCALE, actionBox.y + 16 * UI_SCALE},
                                  FONT_SIZE_SMALL, COLOR_TEXT_MAIN);

                // Se está esperando input e o usuário pressionou algo, avançar
                if (isTypewriterWaitingInput() &&
                    (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) ||
                     IsKeyPressed(KEY_SPACE) ||
                     IsKeyPressed(KEY_ENTER))) {
                    // Notificar o sistema de batalha que a mensagem foi lida
                    messageDisplayComplete();
                }
                break;

            case BATTLE_FORCED_SWITCH:
                // Troca forçada com mensagem animada
                startTypewriter("Escolha outro monstro!", false);
                updateTypewriter();
                drawTypewriterText((Vector2){actionBox.x + 8 * UI_SCALE, actionBox.y + 4 * UI_SCALE},
                                  FONT_SIZE_SMALL, RED);

                // Similar ao SELECT_MONSTER mas sem opção de voltar
                int forcedCount = 0;
                PokeMonster* forcedCurrent = battleSystem->playerTeam->first;

                while (forcedCurrent != NULL && forcedCount < 3) {
                    if (!isMonsterFainted(forcedCurrent)) {
                        Rectangle monsterRect = {
                            actionBox.x + 8 * UI_SCALE,
                            actionBox.y + 16 * UI_SCALE + forcedCount * 10 * UI_SCALE,
                            actionBox.width - 16 * UI_SCALE,
                            9 * UI_SCALE
                        };

                        DrawRectangleRec(monsterRect, LIGHTGRAY);
                        DrawRectangleLinesEx(monsterRect, 1, COLOR_BOX_BORDER);

                        DrawText(forcedCurrent->name,
                                monsterRect.x + 2 * UI_SCALE,
                                monsterRect.y + 1 * UI_SCALE,
                                6 * UI_SCALE,
                                COLOR_TEXT_MAIN);

                        char hpText[16];
                        sprintf(hpText, "HP:%d/%d", forcedCurrent->hp, forcedCurrent->maxHp);
                        DrawText(hpText,
                                monsterRect.x + monsterRect.width - MeasureText(hpText, 6 * UI_SCALE) - 2 * UI_SCALE,
                                monsterRect.y + 1 * UI_SCALE,
                                6 * UI_SCALE,
                                COLOR_TEXT_MAIN);

                        if (CheckCollisionPointRec(GetMousePosition(), monsterRect)) {
                            DrawRectangleLinesEx(monsterRect, 2, COLOR_TEXT_MAIN);

                            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                                PlaySound(selectSound);

                                // Fazer a troca
                                switchMonster(battleSystem->playerTeam, forcedCurrent);

                                // Voltar para a batalha
                                if (!isQueueEmpty(battleSystem->actionQueue)) {
                                    battleSystem->battleState = BATTLE_EXECUTING_ACTIONS;
                                } else {
                                    battleSystem->battleState = BATTLE_SELECT_ACTION;
                                }
                            }
                        }

                        forcedCount++;
                    }
                    forcedCurrent = forcedCurrent->next;
                }
                break;

            case BATTLE_SELECT_MONSTER:
                // Seleção de monstros para troca
                if (battleSystem->playerTurn) {
                    // Título
                    DrawText("POKÉMON", actionBox.x + 8 * UI_SCALE,
                            actionBox.y + 4 * UI_SCALE, FONT_SIZE_SMALL, COLOR_TEXT_MAIN);

                    // Listar monstros disponíveis
                    int monsterCount = 0;
                    PokeMonster* current = battleSystem->playerTeam->first;

                    while (current != NULL && monsterCount < 3) {
                        Rectangle monsterRect = {
                            actionBox.x + 8 * UI_SCALE,
                            actionBox.y + 18 * UI_SCALE + monsterCount * 12 * UI_SCALE,
                            160 * UI_SCALE,
                            10 * UI_SCALE
                        };

                        // Cor de fundo baseada no estado
                        Color bgColor = LIGHTGRAY;
                        if (current == battleSystem->playerTeam->current) {
                            bgColor = Fade(BLUE, 0.3f);  // Monstro atual
                        } else if (isMonsterFainted(current)) {
                            bgColor = Fade(RED, 0.3f);    // Monstro desmaiado
                        }

                        DrawRectangleRec(monsterRect, bgColor);
                        DrawRectangleLinesEx(monsterRect, 1, COLOR_BOX_BORDER);

                        // Nome do monstro
                        DrawText(current->name,
                                monsterRect.x + 2 * UI_SCALE,
                                monsterRect.y + 1 * UI_SCALE,
                                FONT_SIZE_SMALL,
                                COLOR_TEXT_MAIN);

                        // HP
                        char hpText[32];
                        sprintf(hpText, "HP:%d/%d", current->hp, current->maxHp);
                        DrawText(hpText,
                                monsterRect.x + 80 * UI_SCALE,
                                monsterRect.y + 1 * UI_SCALE,
                                FONT_SIZE_SMALL,
                                COLOR_TEXT_MAIN);

                        // Status
                        if (current->statusCondition > STATUS_NONE) {
                            const char* statusText = "";
                            Color statusColor = GRAY;

                            switch (current->statusCondition) {
                                case STATUS_PARALYZED: statusText = "PAR"; statusColor = YELLOW; break;
                                case STATUS_SLEEPING: statusText = "SLP"; statusColor = PURPLE; break;
                                case STATUS_BURNING: statusText = "BRN"; statusColor = RED; break;
                            }

                            if (strlen(statusText) > 0) {
                                DrawText(statusText,
                                        monsterRect.x + 140 * UI_SCALE,
                                        monsterRect.y + 1 * UI_SCALE,
                                        FONT_SIZE_SMALL,
                                        statusColor);
                            }
                        }

                        // Interação
                        if (!isMonsterFainted(current) && current != battleSystem->playerTeam->current) {
                            if (CheckCollisionPointRec(GetMousePosition(), monsterRect)) {
                                DrawRectangleLinesEx(monsterRect, 2, COLOR_TEXT_MAIN);

                                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                                    PlaySound(selectSound);

                                    // Enfileirar ação de troca
                                    enqueue(battleSystem->actionQueue, 1, monsterCount, battleSystem->playerTeam->current);

                                    // Passar turno para o bot
                                    battleSystem->playerTurn = false;
                                    battleSystem->battleState = BATTLE_SELECT_ACTION;
                                }
                            }
                        }

                        monsterCount++;
                        current = current->next;
                    }

                    // Botão voltar
                    Rectangle backBtn = {
                        actionBox.x + actionBox.width - 50 * UI_SCALE,
                        actionBox.y + 2 * UI_SCALE,
                        48 * UI_SCALE,
                        10 * UI_SCALE
                    };

                    if (drawButton(backBtn, "VOLTAR", GRAY)) {
                        PlaySound(selectSound);
                        battleSystem->battleState = BATTLE_SELECT_ACTION;
                    }
                }
                break;

            case BATTLE_ITEM_MENU:
                // Menu de itens
                if (battleSystem->playerTurn) {
                    // Título
                    DrawText("MOCHILA", actionBox.x + 8 * UI_SCALE,
                            actionBox.y + 4 * UI_SCALE, FONT_SIZE_SMALL, COLOR_TEXT_MAIN);

                    // Item disponível
                    Rectangle itemRect = {
                        actionBox.x + 8 * UI_SCALE,
                        actionBox.y + 18 * UI_SCALE,
                        actionBox.width - 16 * UI_SCALE,
                        15 * UI_SCALE
                    };

                    // Cor e nome baseado no tipo de item
                    Color itemColor = GRAY;
                    const char* itemName = "";
                    const char* itemDesc = "";

                    switch (battleSystem->itemType) {
                        case ITEM_POTION:
                            itemColor = PURPLE;
                            itemName = "POÇÃO";
                            itemDesc = "Restaura 20 HP";
                            break;
                        case ITEM_RED_CARD:
                            itemColor = RED;
                            itemName = "CARTÃO VERMELHO";
                            itemDesc = "Força troca do oponente";
                            break;
                        case ITEM_COIN:
                            itemColor = GOLD;
                            itemName = "MOEDA DA SORTE";
                            itemDesc = "50% cura total, 50% desmaia";
                            break;
                    }

                    // Desenhar item
                    DrawRectangleRec(itemRect, Fade(itemColor, 0.3f));
                    DrawRectangleLinesEx(itemRect, 1, COLOR_BOX_BORDER);

                    DrawText(itemName,
                            itemRect.x + 4 * UI_SCALE,
                            itemRect.y + 2 * UI_SCALE,
                            FONT_SIZE_SMALL,
                            COLOR_TEXT_MAIN);

                    DrawText(itemDesc,
                            itemRect.x + 60 * UI_SCALE,
                            itemRect.y + 2 * UI_SCALE,
                            6 * UI_SCALE,
                            COLOR_TEXT_MAIN);

                    // Interação - usar item
                    if (CheckCollisionPointRec(GetMousePosition(), itemRect)) {
                        DrawRectangleLinesEx(itemRect, 2, COLOR_TEXT_MAIN);

                        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                            PlaySound(selectSound);

                            // Enfileirar ação de usar item
                            enqueue(battleSystem->actionQueue, 2, battleSystem->itemType, battleSystem->playerTeam->current);

                            // Passar turno para o bot
                            battleSystem->playerTurn = false;
                            battleSystem->battleState = BATTLE_SELECT_ACTION;
                        }
                    }

                    // Texto de instruções
                    DrawText("Clique no item para usar",
                            actionBox.x + 8 * UI_SCALE,
                            actionBox.y + 35 * UI_SCALE,
                            6 * UI_SCALE,
                            DARKGRAY);

                    // Botão voltar
                    Rectangle backBtn = {
                        actionBox.x + actionBox.width - 50 * UI_SCALE,
                        actionBox.y + 2 * UI_SCALE,
                        48 * UI_SCALE,
                        10 * UI_SCALE
                    };

                    if (drawButton(backBtn, "VOLTAR", GRAY)) {
                        PlaySound(selectSound);
                        battleSystem->battleState = BATTLE_SELECT_ACTION;
                    }
                }
                break;

            case BATTLE_CONFIRM_QUIT:
                // Confirmação para desistir da batalha
                {
                    // Fundo escurecido
                    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(),
                                 (Color){0, 0, 0, 150});

                    // Caixa de diálogo
                    Rectangle dialogBox = {
                        GetScreenWidth()/2 - 100 * UI_SCALE,
                        GetScreenHeight()/2 - 50 * UI_SCALE,
                        200 * UI_SCALE,
                        100 * UI_SCALE
                    };

                    drawFireRedBox(dialogBox, false);

                    // Texto da confirmação
                    const char* confirmText = "Deseja fugir da batalha?";
                    int textWidth = MeasureText(confirmText, FONT_SIZE_SMALL);
                    DrawText(confirmText,
                            dialogBox.x + dialogBox.width/2 - textWidth/2,
                            dialogBox.y + 20 * UI_SCALE,
                            FONT_SIZE_SMALL,
                            COLOR_TEXT_MAIN);

                    // Texto adicional
                    const char* warningText = "Você perderá a batalha!";
                    int warningWidth = MeasureText(warningText, 6 * UI_SCALE);
                    DrawText(warningText,
                            dialogBox.x + dialogBox.width/2 - warningWidth/2,
                            dialogBox.y + 35 * UI_SCALE,
                            6 * UI_SCALE,
                            RED);

                    // Botões
                    Rectangle yesBtn = {
                        dialogBox.x + 30 * UI_SCALE,
                        dialogBox.y + 60 * UI_SCALE,
                        50 * UI_SCALE,
                        20 * UI_SCALE
                    };

                    Rectangle noBtn = {
                        dialogBox.x + 120 * UI_SCALE,
                        dialogBox.y + 60 * UI_SCALE,
                        50 * UI_SCALE,
                        20 * UI_SCALE
                    };

                    if (drawButton(yesBtn, "SIM", RED)) {
                        PlaySound(selectSound);
                        StopMusicStream(battleMusic);
                        PlayMusicStream(menuMusic);
                        currentScreen = MAIN_MENU;
                        resetBattle();
                    }

                    if (drawButton(noBtn, "NÃO", GREEN)) {
                        PlaySound(selectSound);
                        battleSystem->battleState = BATTLE_SELECT_ACTION;
                    }
                }
                break;

            case BATTLE_OVER:
                // Batalha terminada
                {
                    int winner = getBattleWinner();
                    const char* resultText = "";
                    Color resultColor = BLACK;

                    if (winner == 1) {
                        resultText = "VOCÊ VENCEU!";
                        resultColor = GREEN;
                    } else if (winner == 2) {
                        resultText = "VOCÊ PERDEU!";
                        resultColor = RED;
                    } else {
                        resultText = "EMPATE!";
                        resultColor = ORANGE;
                    }

                    DrawText(resultText,
                            actionBox.x + actionBox.width/2 - MeasureText(resultText, FONT_SIZE_LARGE)/2,
                            actionBox.y + 10 * UI_SCALE,
                            FONT_SIZE_LARGE,
                            resultColor);

                    Rectangle menuBtn = {
                        actionBox.x + actionBox.width/2 - 60 * UI_SCALE,
                        actionBox.y + 30 * UI_SCALE,
                        120 * UI_SCALE,
                        15 * UI_SCALE
                    };

                    if (drawButton(menuBtn, "VOLTAR AO MENU", BLUE)) {
                        PlaySound(selectSound);
                        StopMusicStream(battleMusic);
                        PlayMusicStream(menuMusic);
                        currentScreen = MAIN_MENU;
                        resetBattle();
                    }
                }
                break;

            default:
                // Mensagem padrão para outros estados
                if (strlen(battleMessage) > 0) {
                    DrawText(battleMessage,
                            actionBox.x + 8 * UI_SCALE,
                            actionBox.y + 16 * UI_SCALE,
                            FONT_SIZE_SMALL,
                            COLOR_TEXT_MAIN);
                } else {
                    DrawText(getBattleDescription(),
                            actionBox.x + 8 * UI_SCALE,
                            actionBox.y + 16 * UI_SCALE,
                            FONT_SIZE_SMALL,
                            COLOR_TEXT_MAIN);
                }
                break;
        }
    }

    // Sempre desenhar efeitos visuais da batalha
    drawBattleEffects();

    // Indicador de IA
    drawAIIndicator();
}

/**
 * Atualiza a lógica da batalha
 */
void updateBattleScreen(void) {
    if (battleSystem != NULL) {
        // Atualizar música
        UpdateMusicStream(battleMusic);

        // Atualizar lógica de batalha
        updateBattle();

        // Atualizar efeitos visuais
        updateBattleEffects();

        // Detectar mudanças de estado para efeitos sonoros
        static BattleState lastState = BATTLE_IDLE;

        if (lastState != battleSystem->battleState) {
            switch (battleSystem->battleState) {
                case BATTLE_ATTACK_ANIMATION:
                    PlaySound(attackSound);
                    break;
                case BATTLE_DAMAGE_ANIMATION:
                    PlaySound(hitSound);
                    break;
                case BATTLE_FORCED_SWITCH:
                    if (isMonsterFainted(battleSystem->playerTeam->current)) {
                        PlaySound(faintSound);
                    }
                    break;
            }

            lastState = battleSystem->battleState;
        }
    }
}

/**
 * Função para desenhar um monstro na batalha
 */
void drawMonsterInBattle(PokeMonster* monster, Rectangle bounds, bool isPlayer) {
    if (monster == NULL) return;

    // Verificar se a textura foi carregada
    if (monster->texture.id == 0) {
        // Desenhar apenas uma silhueta se não houver textura
        Color monsterColor = getTypeColor(monster->type1);
        monsterColor.a = 200; // Semitransparente

        if (isPlayer) {
            // Silhueta traseira para o jogador
            DrawRectangleRounded((Rectangle){ bounds.x + 30, bounds.y + 20,
                               bounds.width - 60, bounds.height - 40 }, 0.5f, 10, monsterColor);
        } else {
            // Silhueta frontal para o oponente
            DrawRectangleRounded(bounds, 0.3f, 10, monsterColor);
        }
        return;
    }

    // Calcular tamanho e posição para desenhar a textura
    float scale = fmin(
        bounds.width / monster->texture.width,
        bounds.height / monster->texture.height
    ) * 0.9f; // Ligeiramente menor para deixar espaço

    float width = monster->texture.width * scale;
    float height = monster->texture.height * scale;

    float x = bounds.x + (bounds.width - width) / 2;
    float y = bounds.y + (bounds.height - height) / 2;

    // Se for o monstro do jogador, aplicar efeito espelhado
    if (isPlayer) {
        // Desenhar com efeito espelhado (virado de costas)
        DrawTexturePro(
            monster->texture,
            (Rectangle){ 0, 0, -monster->texture.width, monster->texture.height }, // Inverter horizontalmente
            (Rectangle){ x, y, width, height },
            (Vector2){ 0, 0 },
            0.0f,  // Rotação
            WHITE
        );
    } else {
        // Desenhar normalmente com um efeito de "flutuação"
        static float animTimer = 0;
        animTimer += GetFrameTime() * 2.0f;
        float offsetY = sinf(animTimer) * 3.0f; // Movimento suave para cima e para baixo

        DrawTextureEx(
            monster->texture,
            (Vector2){ x, y + offsetY },
            0.0f,
            scale,
            WHITE
        );
    }

    // Adicionar efeitos baseados no status do monstro
    if (monster->statusCondition != STATUS_NONE) {
        Rectangle statusRect = {
            isPlayer ? bounds.x - 30 : bounds.x + bounds.width - 50,
            bounds.y - 20,
            80,
            20
        };

        Color statusColor;
        const char* statusText;

        switch (monster->statusCondition) {
            case STATUS_PARALYZED:
                statusColor = YELLOW;
                statusText = "PAR";
                // Efeito visual adicional (pequenos raios)
                if ((int)(GetTime() * 4) % 2 == 0) {
                    DrawLine(
                        x + width/2,
                        y + height/4,
                        x + width/2 + 10 * cosf(GetTime() * 8),
                        y + height/4 + 10 * sinf(GetTime() * 8),
                        YELLOW
                    );
                }
                break;

            case STATUS_SLEEPING:
                statusColor = DARKPURPLE;
                statusText = "SLP";
                // Efeito visual adicional ("Z"s flutuando)
                for (int i = 0; i < 2; i++) {
                    float time = GetTime() + i * 0.7f;
                    float zX = x + width/2 + 15 * cosf(time * 1.5f);
                    float zY = y + height/4 - 5 * time + 10 * sinf(time);
                    if (fmodf(time, 3.0f) < 2.0f) {
                        DrawText("z", zX, zY, 20, DARKPURPLE);
                    }
                }
                break;

            case STATUS_BURNING:
                statusColor = RED;
                statusText = "BRN";
                // Efeito visual adicional (pequenas chamas)
                for (int i = 0; i < 3; i++) {
                    float time = GetTime() * 3 + i * 2.1f;
                    float fireX = x + width/4 + width/2 * (i * 0.3f) + 5 * sinf(time);
                    float fireY = y + height/2 - 10 * fabs(sinf(time));

                    Color fireColor = (Color){ 230, 150 + i * 30, 50, 200 };
                    DrawCircle(fireX, fireY, 5 + 3 * sinf(time), fireColor);
                }
                break;

            default:
                statusColor = GRAY;
                statusText = "";
                break;
        }

        // Desenhar indicador de status
        if (statusText[0] != '\0') {
            DrawRectangleRounded(statusRect, 0.5f, 8, statusColor);
            DrawRectangleRoundedLines(statusRect, 0.5f, 8, BLACK);
            DrawText(statusText, statusRect.x + statusRect.width/2 - MeasureText(statusText, 16)/2,
                    statusRect.y + 2, 16, WHITE);
        }
    }

    // Adicionar efeito de "dano" quando o HP estiver baixo
    if (monster->hp < monster->maxHp * 0.25f) {
        // Efeito de pulso vermelho para indicar HP baixo
        static float lowHpTimer = 0;
        lowHpTimer += GetFrameTime() * 2.0f;

        if (sinf(lowHpTimer) > 0.2f) {
            DrawRectangleRec(bounds, (Color){ 255, 0, 0, 50 });
        }
    }
}

// Restante das funções de efeitos visuais (mantidas como no código original)

/**
 * Inicializa o sistema de efeitos visuais
 */
void initBattleEffects(void) {
    for (int i = 0; i < MAX_EFFECTS; i++) {
        effects[i].active = false;
    }
}

/**
 * Atualiza os efeitos visuais ativos
 */
void updateBattleEffects(void) {
    for (int i = 0; i < MAX_EFFECTS; i++) {
        if (effects[i].active) {
            effects[i].timer += GetFrameTime();

            if (effects[i].timer >= effects[i].duration) {
                effects[i].active = false;
            }
        }
    }
}

/**
 * Desenha todos os efeitos visuais ativos
 */
void drawBattleEffects(void) {
    for (int i = 0; i < MAX_EFFECTS; i++) {
        if (!effects[i].active) continue;

        float progress = effects[i].timer / effects[i].duration;

        switch (effects[i].type) {
            case EFFECT_FLASH:
                // Efeito de flash na tela
                DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(),
                             Fade(effects[i].color, (1.0f - progress) * 0.4f));
                break;

            case EFFECT_PARTICLES:
                // Efeito de partículas
                for (int j = 0; j < 20; j++) {
                    float angle = j * (2.0f * PI / 20.0f);
                    float distance = progress * 100.0f;
                    float x = effects[i].origin.x + cosf(angle) * distance;
                    float y = effects[i].origin.y + sinf(angle) * distance - progress * progress * 50.0f;
                    float size = (1.0f - progress) * 10.0f;

                    DrawCircle(x, y, size, effects[i].color);
                }
                break;

            case EFFECT_SLASH:
                // Efeito de corte
                {
                    float thickness = (1.0f - progress) * 5.0f;
                    for (int j = 0; j < 3; j++) {
                        Vector2 start = {
                            effects[i].origin.x + (effects[i].target.x - effects[i].origin.x) * (progress - 0.1f + j * 0.05f),
                            effects[i].origin.y + (effects[i].target.y - effects[i].origin.y) * (progress - 0.1f + j * 0.05f)
                        };
                        Vector2 end = {
                            effects[i].origin.x + (effects[i].target.x - effects[i].origin.x) * (progress + j * 0.05f),
                            effects[i].origin.y + (effects[i].target.y - effects[i].origin.y) * (progress + j * 0.05f)
                        };

                        DrawLineEx(start, end, thickness, effects[i].color);
                    }
                }
                break;

            case EFFECT_FIRE:
                // Efeito de fogo
                for (int j = 0; j < 15; j++) {
                    float randAngle = j * 0.1f + GetRandomValue(0, 100) / 100.0f;
                    float randDist = GetRandomValue(0, 100) / 100.0f;
                    float x = effects[i].target.x + sinf(randAngle * 10.0f + progress * 5.0f) * 30.0f * randDist;
                    float y = effects[i].target.y - progress * 80.0f * randDist - j * 2.0f;
                    float size = (1.0f - progress) * 15.0f * randDist;

                    Color fireColor = (Color){
                        255,
                        (unsigned char)(100 + randDist * 155),
                        0,
                        (unsigned char)(255 * (1.0f - progress))
                    };
                    DrawCircle(x, y, size, fireColor);
                }
                break;

            case EFFECT_WATER:
                // Efeito de água
                for (int j = 0; j < 20; j++) {
                    float angle = j * (2.0f * PI / 20) + GetRandomValue(0, 100) / 100.0f;
                    float dist = progress * 80.0f;
                    float x = effects[i].target.x + cosf(angle) * dist * (1.0f + sinf(progress * 10.0f) * 0.2f);
                    float y = effects[i].target.y + sinf(angle) * dist * (1.0f + cosf(progress * 10.0f) * 0.2f);
                    float size = (1.0f - progress) * 8.0f;

                    Color waterColor = (Color){
                        0,
                        100,
                        (unsigned char)(200 + sinf(progress * 5.0f) * 55),
                        (unsigned char)(255 * (1.0f - progress))
                    };
                    DrawCircle(x, y, size, waterColor);
                }
                break;

            case EFFECT_ELECTRIC:
                // Efeito elétrico
                for (int j = 0; j < 5; j++) {
                    float offsetX = GetRandomValue(-30, 30);
                    float offsetY = GetRandomValue(-30, 30);
                    Vector2 start = {
                        effects[i].target.x + offsetX,
                        effects[i].target.y + offsetY
                    };

                    // Desenhar raios
                    for (int k = 0; k < 5; k++) {
                        Vector2 end = {
                            start.x + GetRandomValue(-20, 20),
                            start.y + GetRandomValue(-20, 20) - 10.0f
                        };

                        DrawLineEx(start, end, 3.0f * (1.0f - progress), YELLOW);
                        start = end;
                    }
                }
                break;

            case EFFECT_NATURE:
                // Efeito de natureza
                for (int j = 0; j < 10; j++) {
                    float angle = j * (2.0f * PI / 10) + GetRandomValue(0, 100) / 100.0f;
                    float dist = progress * 60.0f;
                    float x = effects[i].target.x + cosf(angle) * dist;
                    float y = effects[i].target.y + sinf(angle) * dist;

                    // Desenhar folhas
                    Vector2 leaf[] = {
                        {x, y},
                        {x + 10.0f * (1.0f - progress), y + 5.0f * (1.0f - progress)},
                        {x, y + 10.0f * (1.0f - progress)},
                        {x - 10.0f * (1.0f - progress), y + 5.0f * (1.0f - progress)},
                        {x, y}
                    };

                    // Rotacionar a folha
                    float rotation = GetRandomValue(0, 360) * DEG2RAD;
                    for (int v = 0; v < 5; v++) {
                        float tempX = leaf[v].x - x;
                        float tempY = leaf[v].y - y;
                        leaf[v].x = x + tempX * cosf(rotation) - tempY * sinf(rotation);
                        leaf[v].y = y + tempX * sinf(rotation) + tempY * cosf(rotation);
                    }

                    // Desenhar a folha
                    for (int v = 0; v < 4; v++) {
                        DrawLineEx(leaf[v], leaf[v+1], 2.0f * (1.0f - progress), GREEN);
                    }
                }
                break;
        }
    }
}

/**
 * Cria um novo efeito visual
 */
void createBattleEffect(int type, Rectangle bounds, Color color, Vector2 origin, Vector2 target, float duration) {
    // Encontrar um slot livre
    for (int i = 0; i < MAX_EFFECTS; i++) {
        if (!effects[i].active) {
            effects[i].active = true;
            effects[i].type = type;
            effects[i].bounds = bounds;
            effects[i].color = color;
            effects[i].origin = origin;
            effects[i].target = target;
            effects[i].duration = duration;
            effects[i].timer = 0;
            break;
        }
    }
}

/**
 * Cria efeito visual baseado no tipo do ataque
 */
void createAttackEffect(MonsterType attackType, Vector2 origin, Vector2 target) {
    switch (attackType) {
        case TYPE_FIRE:
            createBattleEffect(EFFECT_FIRE, (Rectangle){0}, RED, origin, target, 0.5f);
            createBattleEffect(EFFECT_FLASH, (Rectangle){0}, (Color){255, 100, 0, 128}, origin, target, 0.2f);
            break;

        case TYPE_WATER:
            createBattleEffect(EFFECT_WATER, (Rectangle){0}, BLUE, origin, target, 0.6f);
            createBattleEffect(EFFECT_FLASH, (Rectangle){0}, (Color){0, 100, 255, 128}, origin, target, 0.2f);
            break;

        case TYPE_ELECTRIC:
            createBattleEffect(EFFECT_ELECTRIC, (Rectangle){0}, YELLOW, origin, target, 0.5f);
            createBattleEffect(EFFECT_FLASH, (Rectangle){0}, (Color){255, 255, 0, 128}, origin, target, 0.2f);
            break;

        case TYPE_GRASS:
            createBattleEffect(EFFECT_NATURE, (Rectangle){0}, GREEN, origin, target, 0.7f);
            createBattleEffect(EFFECT_FLASH, (Rectangle){0}, (Color){50, 200, 50, 128}, origin, target, 0.2f);
            break;

        case TYPE_METAL:
        case TYPE_FLYING:
            createBattleEffect(EFFECT_SLASH, (Rectangle){0}, WHITE, origin, target, 0.4f);
            createBattleEffect(EFFECT_SHAKE, (Rectangle){0}, WHITE, origin, target, 0.3f);
            break;

        case TYPE_DRAGON:
            createBattleEffect(EFFECT_FLASH, (Rectangle){0}, (Color){100, 50, 200, 100}, origin, target, 0.15f);
            createBattleEffect(EFFECT_PARTICLES, (Rectangle){0}, (Color){150, 80, 220, 255}, origin, target, 0.5f);
            break;

        case TYPE_GHOST:
            createBattleEffect(EFFECT_FLASH, (Rectangle){0}, (Color){100, 50, 150, 128}, origin, target, 0.4f);
            createBattleEffect(EFFECT_PARTICLES, (Rectangle){0}, (Color){120, 60, 170, 255}, origin, target, 0.6f);
            break;

        case TYPE_FAIRY:
            createBattleEffect(EFFECT_PARTICLES, (Rectangle){0}, (Color){255, 180, 200, 255}, origin, target, 0.7f);
            createBattleEffect(EFFECT_FLASH, (Rectangle){0}, (Color){255, 150, 180, 128}, origin, target, 0.3f);
            break;

        default:
            createBattleEffect(EFFECT_PARTICLES, (Rectangle){0}, WHITE, origin, target, 0.5f);
            break;
    }
}

// Funções auxiliares do sistema de batalha

/**
 * Executa a troca de monstro
 */
void executeMonsterSwitch(PokeMonster* monster, int targetIndex) {
    MonsterList* team = NULL;
    bool isPlayer = false;

    // Determinar qual time
    if (monster == battleSystem->playerTeam->current) {
        team = battleSystem->playerTeam;
        isPlayer = true;
    } else {
        team = battleSystem->opponentTeam;
    }

    // Encontrar o monstro alvo
    PokeMonster* newMonster = NULL;
    PokeMonster* current = team->first;
    int count = 0;

    while (current != NULL && count < targetIndex) {
        current = current->next;
        count++;
    }

    if (current != NULL && !isMonsterFainted(current)) {
        newMonster = current;
        switchMonster(team, newMonster);

        // Mensagem de troca
        char switchMsg[256];
        if (isPlayer) {
            sprintf(switchMsg, "Volte, %s! Vai, %s!",
                    monster->name, newMonster->name);
        } else {
            sprintf(switchMsg, "O oponente trocou para %s!", newMonster->name);
        }

        displayBattleMessage(switchMsg, 2.0f, false, true);
    }
}

/**
 * Executa o uso de um item
 */
void executeItemUse(PokeMonster* user, ItemType itemType) {
    useItem(itemType, user);

    // A mensagem já está definida em useItem
    displayBattleMessage(battleMessage, 2.0f, false, true);

    battleSystem->itemUsed = true;
}