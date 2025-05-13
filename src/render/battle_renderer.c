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
#include "scaling.h"
#include "globals.h"

// Declaração de funções de sistema de batalha
void displayBattleMessage(const char* message, float duration, bool waitForInput, bool autoAdvance);

// Variáveis locais
BattleEffect effects[MAX_EFFECTS] = {0};

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

// Cores da UI
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

#define REF_ENEMY_POS_X     520
#define REF_ENEMY_POS_Y     180
#define REF_PLAYER_POS_X    200
#define REF_PLAYER_POS_Y    360
#define REF_MONSTER_SIZE    150
#define REF_ACTION_BOX_X    10
#define REF_ACTION_BOX_Y    440
#define REF_ACTION_BOX_W    670
#define REF_ACTION_BOX_H    110
#define REF_ENEMY_STATUS_X  480
#define REF_ENEMY_STATUS_Y  20
#define REF_ENEMY_STATUS_W  200
#define REF_ENEMY_STATUS_H  65
#define REF_PLAYER_STATUS_X 480
#define REF_PLAYER_STATUS_Y 390
#define REF_PLAYER_STATUS_W 200
#define REF_PLAYER_STATUS_H 80

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
    // Escalar o tamanho da fonte
    float scaledFontSize = ScaleFontSize(fontSize);

    // Desenhar o texto atual
    DrawText(typewriter.displayText, position.x, position.y, scaledFontSize, color);

    // Se o texto estiver completo e esperando input, desenhar indicador de continuar
    if (typewriter.isComplete && typewriter.waitingForInput) {
        if (sinf(typewriter.blinkTimer) > 0.0f) {
            // Triângulo pequeno piscando
            float triangleSize = ScaleFontSize(8);
            Vector2 v1 = {position.x + MeasureText(typewriter.displayText, scaledFontSize) + 10 * GetScaleX(),
                          position.y + scaledFontSize/2};
            Vector2 v2 = {v1.x + triangleSize, v1.y};
            Vector2 v3 = {v1.x + triangleSize/2, v1.y + triangleSize};
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
    float borderThickness = 2.0f * ((GetScaleX() + GetScaleY()) / 2.0f);
    float shadowOffset = 2.0f * ((GetScaleX() + GetScaleY()) / 2.0f);

    // Sombra inferior direita
    Rectangle shadowRect = {
        bounds.x + shadowOffset,
        bounds.y + shadowOffset,
        bounds.width,
        bounds.height
    };
    DrawRectangleRec(shadowRect, COLOR_BOX_SHADOW);

    // Borda principal preta
    DrawRectangleRec(bounds, COLOR_BOX_BORDER);

    // Fundo branco interno
    Rectangle innerRect = {
        bounds.x + borderThickness,
        bounds.y + borderThickness,
        bounds.width - (borderThickness * 2),
        bounds.height - (borderThickness * 2)
    };
    DrawRectangleRec(innerRect, COLOR_BOX_BG);

    // Destaque se selecionado
    if (isSelected) {
        DrawRectangleLinesEx(bounds, borderThickness, COLOR_TEXT_MAIN);
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

    // Escalas para a altura da barra e bordas
    float barHeight = 6 * ((GetScaleX() + GetScaleY()) / 2.0f);
    float borderThickness = 1 * ((GetScaleX() + GetScaleY()) / 2.0f);

    // Borda da barra
    Rectangle borderRect = {
        position.x,
        position.y,
        width,
        barHeight
    };
    DrawRectangleRec(borderRect, COLOR_BOX_BORDER);

    // Fundo branco
    Rectangle bgRect = {
        position.x + borderThickness,
        position.y + borderThickness,
        width - (borderThickness * 2),
        barHeight - (borderThickness * 2)
    };
    DrawRectangleRec(bgRect, COLOR_BOX_BG);

    // Cor baseada na porcentagem de HP
    Color hpColor = COLOR_HP_GREEN;
    if (hpPercentage <= 0.2f) hpColor = COLOR_HP_RED;
    else if (hpPercentage <= 0.5f) hpColor = COLOR_HP_YELLOW;

    // Preenchimento da HP
    if (hpPercentage > 0) {
        Rectangle hpRect = {
            position.x + borderThickness,
            position.y + borderThickness,
            (width - (borderThickness * 2)) * hpPercentage,
            barHeight - (borderThickness * 2)
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

    // Posição e tamanho da caixa escalados
    Rectangle statusBox = ScaleRectangle(
        REF_ENEMY_STATUS_X,
        REF_ENEMY_STATUS_Y,
        REF_ENEMY_STATUS_W,
        REF_ENEMY_STATUS_H
    );

    drawFireRedBox(statusBox, false);

    // Nome do Pokémon - fonte escalada
    float fontSize = ScaleFontSize(18);
    Vector2 namePos = {statusBox.x + 8 * GetScaleX(), statusBox.y + 8 * GetScaleY()};
    DrawText(monster->name, namePos.x, namePos.y, fontSize, COLOR_TEXT_MAIN);

    // Nível
    Vector2 levelPos = {statusBox.x + statusBox.width - 50 * GetScaleX(), namePos.y};
    DrawText("Lv5", levelPos.x, levelPos.y, fontSize, COLOR_TEXT_MAIN);

    // Barra de HP
    Vector2 hpBarPos = {statusBox.x + 45 * GetScaleX(), statusBox.y + 35 * GetScaleY()};
    drawFireRedHPBar(
        hpBarPos,
        statusBox.width - 55 * GetScaleX(),
        monster->hp,
        monster->maxHp
    );
}

/**
 * Desenha a caixa de status do jogador
 * @param monster Monstro do jogador
 */

void drawPlayerStatusBox(PokeMonster* monster) {
    if (monster == NULL) return;

    // Posição e tamanho da caixa escalados
    Rectangle statusBox = ScaleRectangle(
        REF_PLAYER_STATUS_X,
        REF_PLAYER_STATUS_Y,
        REF_PLAYER_STATUS_W,
        REF_PLAYER_STATUS_H
    );

    drawFireRedBox(statusBox, false);

    // Nome do Pokémon - fonte escalada
    float fontSize = ScaleFontSize(18);
    Vector2 namePos = {statusBox.x + 8 * GetScaleX(), statusBox.y + 8 * GetScaleY()};
    DrawText(monster->name, namePos.x, namePos.y, fontSize, COLOR_TEXT_MAIN);

    // Nível
    Vector2 levelPos = {statusBox.x + statusBox.width - 50 * GetScaleX(), namePos.y};
    DrawText("Lv5", levelPos.x, levelPos.y, fontSize, COLOR_TEXT_MAIN);

    // Barra de HP
    Vector2 hpBarPos = {statusBox.x + 45 * GetScaleX(), statusBox.y + 35 * GetScaleY()};
    drawFireRedHPBar(
        hpBarPos,
        statusBox.width - 55 * GetScaleX(),
        monster->hp,
        monster->maxHp
    );

    // Texto de HP
    char hpText[16];
    sprintf(hpText, "%d/%d", monster->hp, monster->maxHp);
    Vector2 hpTextPos = {statusBox.x + 8 * GetScaleX(), statusBox.y + 55 * GetScaleY()};
    DrawText(hpText, hpTextPos.x, hpTextPos.y, ScaleFontSize(16), COLOR_TEXT_MAIN);
}

/**
 * Função principal para desenhar a tela de batalha
 * Gerencia todos os estados visuais da batalha
 */
void drawBattleScreen(void) {
      // Atualizar música da batalha
    UpdateMusicStream(battleMusic);

    // Desenhar fundo simples (plano ou listrado horizontal como no Crystal)
    ClearBackground(RAYWHITE);

    // Desenhar listras horizontais sutis como no Pokémon Crystal
    for (int i = 0; i < GetScreenHeight(); i += (int)(8 * GetScaleY())) {
        DrawRectangle(0, i, GetScreenWidth(), (int)(4 * GetScaleY()), (Color){200, 230, 220, 80});
    }

    // Verificar se o sistema de batalha está válido
    if (battleSystem != NULL &&
        battleSystem->playerTeam != NULL && battleSystem->playerTeam->current != NULL &&
        battleSystem->opponentTeam != NULL && battleSystem->opponentTeam->current != NULL) {

        PokeMonster* playerMonster = battleSystem->playerTeam->current;
        PokeMonster* opponentMonster = battleSystem->opponentTeam->current;

        // Obter posições escaladas
        Vector2 enemyPosition = ScalePosition(REF_ENEMY_POS_X, REF_ENEMY_POS_Y);
        Vector2 playerPosition = ScalePosition(REF_PLAYER_POS_X, REF_PLAYER_POS_Y);
        float monsterSize = REF_MONSTER_SIZE * ((GetScaleX() + GetScaleY()) / 2.0f);

        // Definir retângulos para posicionar os sprites
        Rectangle opponentRect = {
            enemyPosition.x - monsterSize/2,
            enemyPosition.y - monsterSize/2,
            monsterSize,
            monsterSize
        };

        Rectangle playerRect = {
            playerPosition.x - monsterSize/2,
            playerPosition.y - monsterSize/2,
            monsterSize,
            monsterSize
        };

        // Desenhar os monstros
        drawMonsterInBattle(opponentMonster, opponentRect, false);
        drawMonsterInBattle(playerMonster, playerRect, true);

        //Desenha a box de status dos monstros
        drawOpponentStatusBox(opponentMonster);
        drawPlayerStatusBox(playerMonster);

        // Caixa principal de ação/mensagem - Usar valores escalados
        Rectangle actionBox = ScaleRectangle(
            REF_ACTION_BOX_X,
            REF_ACTION_BOX_Y,
            REF_ACTION_BOX_W,
            REF_ACTION_BOX_H
        );

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
    // Definir constantes para o menu
    #define REF_MENU_X 360         // X inicial do menu
    #define REF_MENU_Y 460         // Y inicial do menu
    #define REF_BUTTON_WIDTH 145   // Largura de cada botão
    #define REF_BUTTON_HEIGHT 40   // Altura de cada botão
    #define REF_BUTTON_SPACING 15  // Espaçamento entre botões

    // Opções do menu em layout 2x2
    const char* options[] = {"FIGHT", "BAG", "POKéMON", "RUN"};
    Color optionColors[] = {
        (Color){ 240, 80, 80, 255 },   // FIGHT - Vermelho
        (Color){ 120, 120, 255, 255 }, // BAG - Azul
        (Color){ 120, 200, 80, 255 },  // POKEMON - Verde
        (Color){ 200, 120, 200, 255 }  // RUN - Roxo
    };

    // Desenhar opções e processar cliques
    for (int i = 0; i < 4; i++) {
        int row = i / 2;
        int col = i % 2;

        // Calcular posição escalada para cada botão
        Rectangle optionRect = ScaleRectangle(
            REF_MENU_X + col * (REF_BUTTON_WIDTH + REF_BUTTON_SPACING),
            REF_MENU_Y + row * (REF_BUTTON_HEIGHT + REF_BUTTON_SPACING),
            REF_BUTTON_WIDTH,
            REF_BUTTON_HEIGHT
        );

        bool isHovered = CheckCollisionPointRec(GetMousePosition(), optionRect);

        // Verificar se a opção está desabilitada
        bool isDisabled = false;
        if (i == 1 && battleSystem->itemUsed) { // BAG button quando item já foi usado
            isDisabled = true;
        }

        // Desenhar o botão com fundo colorido
        Color buttonColor = isDisabled ? GRAY : optionColors[i];
        if (isHovered && !isDisabled) {
            buttonColor = (Color){
                (unsigned char)fmin(255, buttonColor.r + 40),
                (unsigned char)fmin(255, buttonColor.g + 40),
                (unsigned char)fmin(255, buttonColor.b + 40),
                buttonColor.a
            };
        }

        // Desenhar botão com borda arredondada
        DrawRectangleRounded(optionRect, 0.3f, 10 * ((GetScaleX() + GetScaleY()) / 2.0f),
                            Fade(buttonColor, isHovered ? 0.8f : 0.6f));
        DrawRectangleRoundedLines(optionRect, 0.3f, 10 * ((GetScaleX() + GetScaleY()) / 2.0f),
                                 isDisabled ? DARKGRAY : BLACK);

        // Desenhar texto com cor diferente se desabilitado
        Color textColor = isDisabled ? DARKGRAY : WHITE;
        float fontSize = ScaleFontSize(20);

        // Centralizar o texto no botão
        Vector2 textSize = MeasureTextEx(gameFont, options[i], fontSize, 1);
        Vector2 textPos = {
            optionRect.x + (optionRect.width - textSize.x) / 2,
            optionRect.y + (optionRect.height - textSize.y) / 2
        };

        DrawTextEx(gameFont, options[i], textPos, fontSize, 1, textColor);

        // Desenhar cursor triangular quando hover
        if (isHovered && !isDisabled) {
            float triangleSize = ScaleFontSize(12);
            Vector2 v1 = {optionRect.x - triangleSize*1.5f, optionRect.y + optionRect.height/2};
            Vector2 v2 = {v1.x + triangleSize, v1.y - triangleSize};
            Vector2 v3 = {v1.x + triangleSize, v1.y + triangleSize};
            DrawTriangle(v1, v2, v3, WHITE);
        }

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

    #undef REF_MENU_X
    #undef REF_MENU_Y
    #undef REF_BUTTON_WIDTH
    #undef REF_BUTTON_HEIGHT
    #undef REF_BUTTON_SPACING

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
        float fontSize = ScaleFontSize(10); // FONT_SIZE_SMALL
        Vector2 titlePos = ScalePosition(actionBox.x + 8, actionBox.y + 4);
        DrawText("ATAQUES:", titlePos.x, titlePos.y, fontSize, COLOR_TEXT_MAIN);

        // Grid 2x2 para ataques
        float scaledWidth = actionBox.width - ScalePosition(24, 0).x;
        float attackWidth = scaledWidth / 2;

        for (int i = 0; i < 4; i++) {
            int row = i / 2;
            int col = i % 2;

            Rectangle attackRect = ScaleRectangle(
                actionBox.x/GetScaleX() + 8 + col * (attackWidth/GetScaleX() + 8),
                actionBox.y/GetScaleY() + 20 + row * 15,
                attackWidth/GetScaleX(),
                12
            );

            Color attackColor = getTypeColor(playerMonster->attacks[i].type);
            bool canUse = playerMonster->attacks[i].ppCurrent > 0;

            if (!canUse) {
                attackColor = GRAY;
            }

            // Background do ataque
            DrawRectangleRec(attackRect, Fade(attackColor, 0.7f));
            DrawRectangleLinesEx(attackRect, ((GetScaleX() + GetScaleY()) / 2.0f), COLOR_BOX_BORDER);

            // Nome do ataque
            DrawText(
                playerMonster->attacks[i].name,
                attackRect.x + ScalePosition(2, 0).x,
                attackRect.y + ScalePosition(0, 1).y,
                fontSize,
                canUse ? WHITE : LIGHTGRAY
            );

            // PP
            char ppText[16];
            sprintf(ppText, "PP %d/%d",
                   playerMonster->attacks[i].ppCurrent,
                   playerMonster->attacks[i].ppMax);

            float smallFontSize = ScaleFontSize(6);
            float textWidth = MeasureText(ppText, smallFontSize);
            DrawText(ppText,
                    attackRect.x + attackRect.width - textWidth - ScalePosition(2, 0).x,
                    attackRect.y + ScalePosition(0, 1).y,
                    smallFontSize,
                    canUse ? WHITE : LIGHTGRAY);

            // Interação com mouse
            if (canUse && CheckCollisionPointRec(GetMousePosition(), attackRect)) {
                DrawRectangleLinesEx(attackRect, 2 * ((GetScaleX() + GetScaleY()) / 2.0f), WHITE);

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
        Rectangle backBtn = ScaleRectangle(
            actionBox.x/GetScaleX() + actionBox.width/GetScaleX() - 50,
            actionBox.y/GetScaleY() + 2,
            48,
            10
        );

        if (drawButton(backBtn, "VOLTAR", GRAY)) {
            PlaySound(selectSound);
            battleSystem->battleState = BATTLE_SELECT_ACTION;
        }
    }
    break;

case BATTLE_MESSAGE_DISPLAY:
    // Exibir mensagem atual com typewriter
    updateTypewriter();
    Vector2 msgPos = ScalePosition(actionBox.x + 8, actionBox.y + 16);
    drawTypewriterText(msgPos, ScaleFontSize(10), COLOR_TEXT_MAIN);

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
    Vector2 forcedMsgPos = ScalePosition(actionBox.x + 8, actionBox.y + 4);
    drawTypewriterText(forcedMsgPos, ScaleFontSize(10), RED);

    // Similar ao SELECT_MONSTER mas sem opção de voltar
    int forcedCount = 0;
    PokeMonster* forcedCurrent = battleSystem->playerTeam->first;

    while (forcedCurrent != NULL && forcedCount < 3) {
        if (!isMonsterFainted(forcedCurrent)) {
            Rectangle monsterRect = ScaleRectangle(
                actionBox.x/GetScaleX() + 8,
                actionBox.y/GetScaleY() + 16 + forcedCount * 10,
                actionBox.width/GetScaleX() - 16,
                9
            );

            DrawRectangleRec(monsterRect, LIGHTGRAY);
            DrawRectangleLinesEx(monsterRect, ((GetScaleX() + GetScaleY()) / 2.0f), COLOR_BOX_BORDER);

            DrawText(
                forcedCurrent->name,
                monsterRect.x + ScalePosition(2, 0).x,
                monsterRect.y + ScalePosition(0, 1).y,
                ScaleFontSize(6),
                COLOR_TEXT_MAIN
            );

            char hpText[16];
            sprintf(hpText, "HP:%d/%d", forcedCurrent->hp, forcedCurrent->maxHp);
            float smallFontSize = ScaleFontSize(6);
            float hpTextWidth = MeasureText(hpText, smallFontSize);

            DrawText(
                hpText,
                monsterRect.x + monsterRect.width - hpTextWidth - ScalePosition(2, 0).x,
                monsterRect.y + ScalePosition(0, 1).y,
                smallFontSize,
                COLOR_TEXT_MAIN
            );

            if (CheckCollisionPointRec(GetMousePosition(), monsterRect)) {
                DrawRectangleLinesEx(monsterRect, 2 * ((GetScaleX() + GetScaleY()) / 2.0f), COLOR_TEXT_MAIN);

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
        Vector2 monsterTitlePos = ScalePosition(actionBox.x + 8, actionBox.y + 4);
        DrawText("POKÉMON", monsterTitlePos.x, monsterTitlePos.y, ScaleFontSize(10), COLOR_TEXT_MAIN);

        // Listar monstros disponíveis
        int monsterCount = 0;
        PokeMonster* current = battleSystem->playerTeam->first;

        while (current != NULL && monsterCount < 3) {
            Rectangle monsterRect = ScaleRectangle(
                actionBox.x/GetScaleX() + 8,
                actionBox.y/GetScaleY() + 18 + monsterCount * 12,
                160,
                10
            );

            // Cor de fundo baseada no estado
            Color bgColor = LIGHTGRAY;
            if (current == battleSystem->playerTeam->current) {
                bgColor = Fade(BLUE, 0.3f);  // Monstro atual
            } else if (isMonsterFainted(current)) {
                bgColor = Fade(RED, 0.3f);    // Monstro desmaiado
            }

            DrawRectangleRec(monsterRect, bgColor);
            DrawRectangleLinesEx(monsterRect, ((GetScaleX() + GetScaleY()) / 2.0f), COLOR_BOX_BORDER);

            // Nome do monstro
            DrawText(
                current->name,
                monsterRect.x + ScalePosition(2, 0).x,
                monsterRect.y + ScalePosition(0, 1).y,
                ScaleFontSize(10),
                COLOR_TEXT_MAIN
            );

            // HP
            char hpText[32];
            sprintf(hpText, "HP:%d/%d", current->hp, current->maxHp);
            Vector2 hpPos = ScalePosition(monsterRect.x/GetScaleX() + 80, monsterRect.y/GetScaleY() + 1);
            DrawText(
                hpText,
                hpPos.x,
                hpPos.y,
                ScaleFontSize(10),
                COLOR_TEXT_MAIN
            );

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
                    Vector2 statusPos = ScalePosition(monsterRect.x/GetScaleX() + 140, monsterRect.y/GetScaleY() + 1);
                    DrawText(
                        statusText,
                        statusPos.x,
                        statusPos.y,
                        ScaleFontSize(10),
                        statusColor
                    );
                }
            }

            // Interação
            if (!isMonsterFainted(current) && current != battleSystem->playerTeam->current) {
                if (CheckCollisionPointRec(GetMousePosition(), monsterRect)) {
                    DrawRectangleLinesEx(monsterRect, 2 * ((GetScaleX() + GetScaleY()) / 2.0f), COLOR_TEXT_MAIN);

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
        Rectangle backBtn = ScaleRectangle(
            actionBox.x/GetScaleX() + actionBox.width/GetScaleX() - 50,
            actionBox.y/GetScaleY() + 2,
            48,
            10
        );

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
        Vector2 titlePos = ScalePosition(actionBox.x + 8, actionBox.y + 4);
        DrawText("MOCHILA", titlePos.x, titlePos.y, ScaleFontSize(10), COLOR_TEXT_MAIN);

        // Item disponível
        Rectangle itemRect = ScaleRectangle(
            actionBox.x/GetScaleX() + 8,
            actionBox.y/GetScaleY() + 18,
            actionBox.width/GetScaleX() - 16,
            15
        );

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
        DrawRectangleLinesEx(itemRect, ((GetScaleX() + GetScaleY()) / 2.0f), COLOR_BOX_BORDER);

        Vector2 itemNamePos = ScalePosition(itemRect.x/GetScaleX() + 4, itemRect.y/GetScaleY() + 2);
        DrawText(
            itemName,
            itemNamePos.x,
            itemNamePos.y,
            ScaleFontSize(10),
            COLOR_TEXT_MAIN
        );

        Vector2 itemDescPos = ScalePosition(itemRect.x/GetScaleX() + 60, itemRect.y/GetScaleY() + 2);
        DrawText(
            itemDesc,
            itemDescPos.x,
            itemDescPos.y,
            ScaleFontSize(6),
            COLOR_TEXT_MAIN
        );

        // Interação - usar item
        if (CheckCollisionPointRec(GetMousePosition(), itemRect)) {
            DrawRectangleLinesEx(itemRect, 2 * ((GetScaleX() + GetScaleY()) / 2.0f), COLOR_TEXT_MAIN);

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
        Vector2 instrPos = ScalePosition(actionBox.x + 8, actionBox.y + 35);
        DrawText(
            "Clique no item para usar",
            instrPos.x,
            instrPos.y,
            ScaleFontSize(6),
            DARKGRAY
        );

        // Botão voltar
        Rectangle backBtn = ScaleRectangle(
            actionBox.x/GetScaleX() + actionBox.width/GetScaleX() - 50,
            actionBox.y/GetScaleY() + 2,
            48,
            10
        );

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
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){0, 0, 0, 150});

        // Caixa de diálogo
        Rectangle dialogBox = ScaleRectangle(
            GetScreenWidth()/(2*GetScaleX()) - 100,
            GetScreenHeight()/(2*GetScaleY()) - 50,
            200,
            100
        );

        drawFireRedBox(dialogBox, false);

        // Texto da confirmação
        const char* confirmText = "Deseja fugir da batalha?";
        float fontSize = ScaleFontSize(10);
        int textWidth = MeasureText(confirmText, fontSize);
        Vector2 textPos = {
            dialogBox.x + dialogBox.width/2 - textWidth/2,
            dialogBox.y + ScalePosition(0, 20).y
        };
        DrawText(confirmText, textPos.x, textPos.y, fontSize, COLOR_TEXT_MAIN);

        // Texto adicional
        const char* warningText = "Você perderá a batalha!";
        float smallFontSize = ScaleFontSize(6);
        int warningWidth = MeasureText(warningText, smallFontSize);
        Vector2 warningPos = {
            dialogBox.x + dialogBox.width/2 - warningWidth/2,
            dialogBox.y + ScalePosition(0, 35).y
        };
        DrawText(warningText, warningPos.x, warningPos.y, smallFontSize, RED);

        // Botões
        Rectangle yesBtn = ScaleRectangle(
            dialogBox.x/GetScaleX() + 30,
            dialogBox.y/GetScaleY() + 60,
            50,
            20
        );

        Rectangle noBtn = ScaleRectangle(
            dialogBox.x/GetScaleX() + 120,
            dialogBox.y/GetScaleY() + 60,
            50,
            20
        );

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
 * Função atualizada para desenhar um monstro na batalha,
 * usando a textura frontal ou traseira dependendo de quem é o dono
 */
void drawMonsterInBattle(PokeMonster* monster, Rectangle bounds, bool isPlayer) {
    if (monster == NULL) return;

    // Escolher a textura apropriada (traseira para jogador, frontal para oponente)
    Texture2D texture = isPlayer ? monster->backTexture : monster->frontTexture;

    // Verificar se a textura foi carregada
    if (texture.id == 0) {
        // Desenhar apenas uma silhueta se não houver textura
        Color monsterColor = getTypeColor(monster->type1);
        monsterColor.a = 200; // Semitransparente

        // Desenhar silhueta simples sem plataformas
        DrawRectangleRounded(bounds, 0.3f, 10, monsterColor);
        return;
    }

    // Calcular tamanho e posição para desenhar a textura
    float scale = fmin(
        bounds.width / texture.width,
        bounds.height / texture.height
    ) * 1.2f; // Um pouco maior para ficar mais próximo da tela

    float width = texture.width * scale;
    float height = texture.height * scale;

    float x = bounds.x + (bounds.width - width) / 2;
    float y = bounds.y + (bounds.height - height) / 2;

    // Desenhar a textura
    DrawTextureEx(
        texture,
        (Vector2){ x, y },
        0.0f,  // Rotação
        scale,
        WHITE
    );

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

        case TYPE_STEEL:
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