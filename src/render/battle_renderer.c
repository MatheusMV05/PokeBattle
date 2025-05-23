/**
 * PokeBattle - Renderização da tela de batalha
 */
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
#include "gui.h"
#include "hp_bar.h"

// Sistema de typewriter para textos
typedef struct
{
    char fullText[256]; // Texto completo
    char displayText[256]; // Texto atualmente exibido
    float charTimer; // Timer para próximo caractere
    int currentChar; // Índice do caractere atual
    bool isComplete; // Se terminou de digitar
    float charDelay; // Delay entre caracteres
    bool waitingForInput; // Se está esperando input do usuário
    float blinkTimer; // Timer para piscar o indicador de continuar
} TypewriterText;

static TypewriterText typewriter = {0};
static float platformYOffset1 = 0.0f;
static float platformYOffset2 = 0.0f;
static float battleTimer = 0.0f;
static float hpAnimTimer = 0.0f;
static bool isHpAnimationActive = false;

// Velocidade do Typewriter (menor = mais rápido)
#define TYPEWRITER_SPEED 0.02f

// Limite de efeitos visuais
#define MAX_EFFECTS 10
BattleEffect effects[MAX_EFFECTS] = {0};

// Estrutura para sprite de um pokémon que pode ser animado
typedef struct
{
    Texture2D texture; // Textura do sprite
    float scale; // Escala do sprite
    Rectangle frameRect; // Retângulo do frame atual
    int frameCount; // Número total de frames
    int currentFrame; // Frame atual
    float frameTime; // Tempo de cada frame
    float timer; // Timer acumulado
    bool isAnimated; // Se é um sprite animado (GIF)
    // Efeitos visuais opcionais
    float bounceHeight; // Altura do efeito de "bounce"
    float bounceSpeed; // Velocidade do bounce
    float flashAlpha; // Alpha para efeito de flash (0-1)
    Color tint; // Cor de matiz
} AnimatedSprite;

static AnimatedSprite playerSprite = {0};
static AnimatedSprite enemySprite = {0};

/**
 * Inicia o efeito typewriter para um novo texto
 */
void startTypewriter(const char* text, bool waitForInput)
{
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
void updateTypewriter(void)
{
    if (typewriter.isComplete)
    {
        // Atualizar o timer de piscar
        typewriter.blinkTimer += GetFrameTime() * 3.0f;
        return;
    }

    typewriter.charTimer += GetFrameTime();

    // Avançar para o próximo caractere
    while (typewriter.charTimer >= typewriter.charDelay && !typewriter.isComplete)
    {
        if (typewriter.currentChar < strlen(typewriter.fullText))
        {
            // Adicionar próximo caractere
            typewriter.displayText[typewriter.currentChar] = typewriter.fullText[typewriter.currentChar];
            typewriter.displayText[typewriter.currentChar + 1] = '\0';
            typewriter.currentChar++;

            // Resetar timer
            typewriter.charTimer -= typewriter.charDelay;

            // Se for espaço ou pontuação, adicionar uma pequena pausa
            char lastChar = typewriter.fullText[typewriter.currentChar - 1];
            if (lastChar == ' ' || lastChar == ',' || lastChar == '.')
            {
                typewriter.charTimer -= typewriter.charDelay * 0.5f;
            }
        }
        else
        {
            // Texto completo
            typewriter.isComplete = true;
            typewriter.blinkTimer = 0.0f;
        }
    }

    // Permitir pular o efeito com clique ou tecla
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER))
    {
        if (!typewriter.isComplete)
        {
            // Completar instantaneamente
            strcpy(typewriter.displayText, typewriter.fullText);
            typewriter.currentChar = strlen(typewriter.fullText);
            typewriter.isComplete = true;
            typewriter.blinkTimer = 0.0f;
        }
    }
}

/**
 * Desenha um monstro na batalha
 */
void drawMonsterInBattle(PokeMonster* monster, bool isPlayer) {
    if (monster == NULL) return;

    Vector2 platformPos, monsterPos;

    Animation* currentAnim = NULL;
    float baseScale = isPlayer ? 3.0f : 2.5f;

    // Posicionamento ajustado para melhor composição na tela
    if (isPlayer) {
        // Posição do jogador (um pouco mais à direita e mais baixo)

        monsterPos = (Vector2){GetScreenWidth() / 3, GetScreenHeight() / 1.8f - 20};

        currentAnim = &monster->backAnimation;
    } else {
        // Posição do inimigo (um pouco mais à esquerda e mais alto)

        monsterPos = (Vector2){GetScreenWidth() * 2 / 3, GetScreenHeight() / 2.6f - 20};

        currentAnim = &monster->frontAnimation;
    }





    // Atualizar e desenhar animação
    if (currentAnim->frameCount > 0) {
        UpdateAnimation(currentAnim);

        Texture2D currentFrame = currentAnim->frames[currentAnim->currentFrame];
        float scale = baseScale * (isPlayer ? 1.0f : 0.9f);

        // Efeito de animação de respiração
        float breatheEffect = sinf(battleTimer * 1.2f) * 0.03f;
        scale += breatheEffect;

        DrawTextureEx(
            currentFrame,
            (Vector2){
                monsterPos.x - (currentFrame.width * scale) / 2,
                monsterPos.y - (currentFrame.height * scale) / 2
            },
            0.0f,
            scale,
            WHITE
        );
    } else {
        // Fallback estático caso a animação falhe
        Texture2D fallback = isPlayer ? monster->backAnimation.frames[0] : monster->frontAnimation.frames[0];
        if (fallback.id != 0) {
            DrawTextureEx(
                fallback,
                (Vector2){
                    monsterPos.x - (fallback.width * baseScale) / 2,
                    monsterPos.y - (fallback.height * baseScale) / 2
                },
                0.0f,
                baseScale,
                WHITE
            );
        }
    }

    // Desenhar indicador de status (se tiver)
    if (monster->statusCondition != STATUS_NONE) {
        Vector2 statusPos = isPlayer
                                ? (Vector2){monsterPos.x - 50, monsterPos.y - 80}  // Ajustado para mais visível
                                : (Vector2){monsterPos.x + 50, monsterPos.y - 70}; // Ajustado para mais visível

        Color statusColor;
        const char* statusText;

        switch (monster->statusCondition) {
        case STATUS_PARALYZED:
            statusColor = (Color){240, 208, 48, 255}; // Amarelo Pokémon
            statusText = "PAR";
            break;
        case STATUS_SLEEPING:
            statusColor = (Color){112, 88, 152, 255}; // Roxo escuro Pokémon
            statusText = "SLP";
            break;
        case STATUS_BURNING:
            statusColor = (Color){240, 128, 48, 255}; // Laranja Pokémon
            statusText = "BRN";
            break;
        case STATUS_ATK_DOWN:
            statusColor = (Color){192, 48, 40, 255}; // Vermelho escuro Pokémon
            statusText = "ATK↓";
            break;
        case STATUS_DEF_DOWN:
            statusColor = (Color){48, 96, 240, 255}; // Azul Pokémon
            statusText = "DEF↓";
            break;
        case STATUS_SPD_DOWN:
            statusColor = (Color){120, 200, 80, 255}; // Verde Pokémon
            statusText = "SPD↓";
            break;
        default:
            statusColor = (Color){168, 168, 120, 255}; // Bege Pokémon
            statusText = "???";
            break;
        }

        // Desenhar bolha de status estilo BW
        DrawCircle(
            statusPos.x,
            statusPos.y,
            22,
            statusColor
        );

        // Borda da bolha
        DrawCircleLines(
            statusPos.x,
            statusPos.y,
            22,
            BLACK
        );

        // Destacar texto
        DrawText(
            statusText,
            statusPos.x - MeasureText(statusText, 14) / 2,
            statusPos.y - 7,
            14,
            WHITE
        );
    }
}


/**
 * Desenha a caixa de status de um monstro no estilo Pokémon Black/White
 */
void drawMonsterStatusBox(PokeMonster* monster, Rectangle bounds, bool isPlayer) {
    if (monster == NULL) return;

    // Cores do estilo Black/White
    Color bgColor = (Color){40, 40, 40, 230};
    Color frameColor = (Color){10, 10, 10, 255};
    Color textColor = (Color){255, 255, 255, 255};
    Color nameBoxColor = isPlayer ? (Color){64, 159, 92, 255} : (Color){190, 52, 52, 255};

    // Desenhar sombra sutil
    DrawRectangleRounded(
        (Rectangle){bounds.x + 3, bounds.y + 3, bounds.width, bounds.height},
        0.3f, 8,
        (Color){0, 0, 0, 100}
    );

    // Desenhar fundo principal
    DrawRectangleRounded(bounds, 0.3f, 8, bgColor);
    DrawRectangleRoundedLines(bounds, 0.3f, 8, frameColor);

    // Desenhar caixa de nome (mais estreita)
    Rectangle nameBox = {
        bounds.x - 10,
        bounds.y - 15,
        bounds.width * 0.65f,
        40
    };

    // Sombra da caixa de nome
    DrawRectangleRounded(
        (Rectangle){nameBox.x + 2, nameBox.y + 2, nameBox.width, nameBox.height},
        0.3f, 8,
        (Color){0, 0, 0, 100}
    );

    DrawRectangleRounded(nameBox, 0.3f, 8, nameBoxColor);
    DrawRectangleRoundedLines(nameBox, 0.3f, 8, frameColor);

    // Nome do monstro na caixa
    DrawText(monster->name,
            nameBox.x + 15,
            nameBox.y + 10,
            20,
            textColor);

    // Se for o jogador, mostrar info de HP em texto
    if (isPlayer) {
        char hpText[32];
        sprintf(hpText, "%d/%d", monster->hp, monster->maxHp);
        DrawText(hpText,
                bounds.x + bounds.width - 80,
                bounds.y + 25,
                16,
                textColor);
    }

    // Texto "HP" no estilo BW (mais destacado)
    DrawText("HP",
            bounds.x + 15,
            bounds.y + 35,
            18,
            textColor);

    // Barra de HP no estilo Black/White (mais fina, com borda)
    Rectangle hpBarOutline = {
        bounds.x + 50,
        bounds.y + 40,
        bounds.width - 100,
        12
    };

    // Usar nossa função avançada de barra de HP em vez da implementação estática
    DrawHealthBar(hpBarOutline, monster->hp, monster->maxHp, monster);
}

/**
 * Desenha o menu de ações da batalha no estilo Pokémon Black/White
 */
void drawBattleActionMenu(Rectangle bounds) {
    // Cores do estilo BW
    Color bgColor = (Color){40, 40, 40, 230};
    Color frameColor = (Color){0, 0, 0, 255};
    Color textColor = (Color){255, 255, 255, 255};

    // Fundo principal
    DrawRectangleRounded(bounds, 0.3f, 8, bgColor);
    DrawRectangleRoundedLines(bounds, 0.3f, 8, frameColor);

    // Título no estilo BW
    DrawText("O que você vai fazer?",
            bounds.x + 20,
            bounds.y + 15,
            20,
            textColor);

    const char* options[] = {
        "LUTAR",
        "MOCHILA",
        "POKÉMON",
        "FUGIR"
    };

    // Calcular o tamanho adequado para os botões para não vazar
    // Deixar margens adequadas de todos os lados
    float marginX = 30;    // Margens laterais
    float marginY = 60;    // Margem do topo (maior para acomodar o título) + margem inferior
    float spacingX = 20;   // Espaço entre botões horizontalmente
    float spacingY = 15;   // Espaço entre botões verticalmente

    // Calcular o tamanho de cada botão
    float buttonWidth = (bounds.width - 2 * marginX - spacingX) / 2;    // 2 colunas
    float buttonHeight = (bounds.height - marginY - spacingY) / 2;      // 2 linhas

    // Posição inicial dos botões
    float startX = bounds.x + marginX;
    float startY = bounds.y + 50;  // Deslocado para dar espaço para o título

    // Cores dos botões no estilo BW
    Color buttonColors[] = {
        (Color){180, 50, 50, 255},  // LUTAR - Vermelho
        (Color){50, 110, 180, 255}, // MOCHILA - Azul
        (Color){60, 170, 60, 255},  // POKÉMON - Verde
        (Color){180, 140, 40, 255}  // FUGIR - Amarelo
    };

    // Desenhar os 4 botões em grid 2x2
    for (int i = 0; i < 4; i++) {
        int row = i / 2;
        int col = i % 2;

        Rectangle optionRect = {
            startX + col * (buttonWidth + spacingX),
            startY + row * (buttonHeight + spacingY),
            buttonWidth,
            buttonHeight
        };

        // Verificar se a opção está disponível (MOCHILA só disponível se item não foi usado)
        bool isEnabled = true;
        if (i == 1 && battleSystem->playerItemUsed) {
            isEnabled = false;
            buttonColors[i] = GRAY;
        }

        // Desenhar botão com esquinas mais arredondadas
        if (GuiPokemonButton(optionRect, options[i], isEnabled)) {
            PlaySound(selectSound);

            // Ação com base na opção
            battleSystem->selectedAction = i;

            switch (i) {
                case 0: // LUTAR
                    battleSystem->battleState = BATTLE_SELECT_ATTACK;
                    break;
                case 1: // MOCHILA
                    if (!battleSystem->playerItemUsed) {
                        battleSystem->battleState = BATTLE_ITEM_MENU;
                    }
                    break;
                case 2: // POKÉMON
                    battleSystem->battleState = BATTLE_SELECT_MONSTER;
                    break;
                case 3: // FUGIR
                    battleSystem->battleState = BATTLE_CONFIRM_QUIT;
                    break;
            }
        }
    }
}

/**
 * Desenha os ataques disponíveis
 */
void drawBattleAttackMenu(Rectangle bounds) {
    // Cores do estilo BW
    Color bgColor = (Color){40, 40, 40, 230};
    Color frameColor = (Color){0, 0, 0, 255};
    Color textColor = (Color){255, 255, 255, 255};

    // Fundo da caixa de ataques
    DrawRectangleRounded(bounds, 0.3f, 8, bgColor);
    DrawRectangleRoundedLines(bounds, 0.3f, 8, frameColor);

    // Título
    DrawText("SELECIONE UM ATAQUE",
             bounds.x + 20,
             bounds.y + 15,
             20,
             textColor);

    // Botão de voltar no estilo BW
    Rectangle backBtn = {
        bounds.x + bounds.width - 100,
        bounds.y + 10,
        80,
        30
    };

    if (GuiPokemonButton(backBtn, "VOLTAR", true)) {
        PlaySound(selectSound);
        battleSystem->battleState = BATTLE_SELECT_ACTION;
    }

    // Mostrar ataques disponíveis
    PokeMonster* monster = battleSystem->playerTeam->current;

    // Calcular o tamanho adequado para os botões de ataque
    float marginX = 25;    // Margens laterais
    float marginY = 55;    // Margem superior (para acomodar título) + inferior
    float spacingX = 20;   // Espaço entre botões horizontalmente
    float spacingY = 15;   // Espaço entre botões verticalmente

    float attackWidth = (bounds.width - 2 * marginX - spacingX) / 2;    // 2 colunas
    float attackHeight = (bounds.height - marginY - spacingY) / 2;     // 2 linhas

    float startX = bounds.x + marginX;
    float startY = bounds.y + marginY;  // Posicionado abaixo do título

    for (int i = 0; i < 4; i++) {
        int row = i / 2;
        int col = i % 2;

        Rectangle attackRect = {
            startX + col * (attackWidth + spacingX),
            startY + row * (attackHeight + spacingY),
            attackWidth,
            attackHeight
        };

        // Cor baseada no tipo
        Color attackColor = getTypeColor(monster->attacks[i].type);

        // Ajustar cor para o estilo BW
        // Tornar cores menos saturadas e mais padronizadas
        attackColor.r = (attackColor.r * 3 + 40) / 4;
        attackColor.g = (attackColor.g * 3 + 40) / 4;
        attackColor.b = (attackColor.b * 3 + 40) / 4;

        // Desativar se não tiver PP
        bool canUse = monster->attacks[i].ppCurrent > 0;
        if (!canUse) {
            attackColor.r = (attackColor.r + 200) / 3;
            attackColor.g = (attackColor.g + 200) / 3;
            attackColor.b = (attackColor.b + 200) / 3;
            attackColor.a = 180;
        }

        if (GuiPokemonButton(attackRect, monster->attacks[i].name, canUse)) {
            PlaySound(selectSound);
            battleSystem->selectedAttack = i;

            // Enfileirar ação de ataque
            enqueue(battleSystem->actionQueue, 0, i, battleSystem->playerTeam->current);

            // Passar o turno para o bot escolher
            battleSystem->playerTurn = false;
            battleSystem->battleState = BATTLE_SELECT_ACTION;
        }

        // Desenhar informações do ataque em posições calculadas para caber bem
        Rectangle infoRect = {
            attackRect.x + 10,
            attackRect.y + attackRect.height - 22,
            attackRect.width - 20,
            18
        };

        // PP à esquerda
        char ppText[20];
        sprintf(ppText, "PP: %d/%d",
                monster->attacks[i].ppCurrent,
                monster->attacks[i].ppMax);

        DrawText(ppText,
                 infoRect.x,
                 infoRect.y,
                 14,
                 WHITE);

        // Poder à direita (se tiver)
        if (monster->attacks[i].power > 0) {
            char powerText[20];
            sprintf(powerText, "Poder: %d", monster->attacks[i].power);

            // Calcular posição para alinhar à direita
            int powerWidth = MeasureText(powerText, 14);
            DrawText(powerText,
                     infoRect.x + infoRect.width - powerWidth,
                     infoRect.y,
                     14,
                     WHITE);
        } else {
            DrawText("Status",
                     infoRect.x + infoRect.width - MeasureText("Status", 14),
                     infoRect.y,
                     14,
                     WHITE);
        }
    }
}

/**
 * Desenha o menu de seleção de monstro
 */
void drawMonsterSelectionMenu(Rectangle bounds)
{
    // Fundo da caixa
    DrawRectangleRounded(bounds, 0.2f, 6, (Color){240, 240, 240, 230});
    DrawRectangleRoundedLines(bounds, 0.2f, 6, BLACK);

    // Título
    DrawText("SELECIONE UM POKÉMON",
             bounds.x + 20,
             bounds.y + 15,
             20,
             BLACK);

    // Botão de voltar - só visível se não for troca forçada
    if (battleSystem->battleState != BATTLE_FORCED_SWITCH)
    {
        Rectangle backBtn = {
            bounds.x + bounds.width - 100,
            bounds.y + 10,
            80,
            30
        };

        if (GuiPokemonButton(backBtn, "VOLTAR", true))
        {
            PlaySound(selectSound);
            battleSystem->battleState = BATTLE_SELECT_ACTION;
        }
    }

    // Mostrar monstros do time
    MonsterList* team = battleSystem->playerTeam;
    if (team == NULL || team->first == NULL) return;

    // Configurações de layout
    const float cardWidth = bounds.width - 40;
    const float cardHeight = 70;
    const float startX = bounds.x + 20;
    const float startY = bounds.y + 50;
    const float spacing = 15;
    const int maxVisible = 3; // Número máximo de cards visíveis

    // Contar quantos monstros existem
    int totalMonsters = 0;
    PokeMonster* counter = team->first;
    while (counter != NULL)
    {
        totalMonsters++;
        counter = counter->next;
    }

    // Ajustar posição inicial para centralizar se necessário
    float adjustedStartY = startY;
    if (totalMonsters < maxVisible)
    {
        float totalHeight = (cardHeight + spacing) * totalMonsters - spacing;
        adjustedStartY = bounds.y + (bounds.height - totalHeight) / 2;
    }

    // Variáveis de desenho
    int index = 0;
    PokeMonster* current = team->first;
    float scrollOffset = 0; // Implementar lógica de scroll se necessário

    while (current != NULL && index < maxVisible)
    {
        // Calcular posição do card com offset
        Rectangle cardRect = {
            startX,
            adjustedStartY + index * (cardHeight + spacing) - scrollOffset,
            cardWidth,
            cardHeight
        };

        // Não desenhar se estiver fora da área visível
        if (cardRect.y + cardHeight < bounds.y || cardRect.y > bounds.y + bounds.height)
        {
            current = current->next;
            index++;
            continue;
        }

        // Cor de fundo
        Color cardColor = getTypeColor(current->type1);
        cardColor.a = 150;

        // Verificar status do monstro
        bool isCurrentMonster = (current == team->current);
        bool isFainted = isMonsterFainted(current);

        // Destacar monstro atual
        if (isCurrentMonster)
        {
            DrawRectangleRounded(
                (Rectangle){cardRect.x - 5, cardRect.y - 5, cardRect.width + 10, cardRect.height + 10},
                0.2f, 6, YELLOW
            );
        }

        // Escurecer desmaiados
        if (isFainted)
        {
            cardColor = ColorAlpha(cardColor, 0.4f);
        }

        // Desenhar card
        DrawRectangleRounded(cardRect, 0.2f, 6, cardColor);
        DrawRectangleRoundedLines(cardRect, 0.2f, 6, BLACK);

        // Informações do monstro
        DrawText(current->name, cardRect.x + 10, cardRect.y + 10, 20, WHITE);

        // Status
        if (current->statusCondition != STATUS_NONE)
        {
            const char* statusText = "---";
            switch (current->statusCondition)
            {
            case STATUS_PARALYZED: statusText = "PAR";
                break;
            case STATUS_SLEEPING: statusText = "SLP";
                break;
            case STATUS_BURNING: statusText = "BRN";
                break;
            }
            DrawText(statusText, cardRect.x + cardRect.width - 60, cardRect.y + 10, 18, WHITE);
        }

        // HP
        char hpText[32];
        sprintf(hpText, "HP: %d/%d", current->hp, current->maxHp);
        DrawText(hpText, cardRect.x + 10, cardRect.y + 40, 16, WHITE);

        // Barra de HP
        Rectangle hpBar = {cardRect.x + 130, cardRect.y + 45, cardRect.width - 140, 10};
        float hpRatio = (float)current->hp / current->maxHp;
        hpRatio = fmaxf(fminf(hpRatio, 1.0f), 0.0f);
        Color hpColor = hpRatio > 0.5f ? GREEN : (hpRatio > 0.2f ? YELLOW : RED);

        DrawRectangleRec(hpBar, GRAY);
        DrawRectangle(hpBar.x, hpBar.y, hpBar.width * hpRatio, hpBar.height, hpColor);

        // Interação
        if (!isFainted && !isCurrentMonster &&
            CheckCollisionPointRec(GetMousePosition(), cardRect) &&
            IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {
            PlaySound(selectSound);
            switchMonster(team, current);
            enqueue(battleSystem->actionQueue, 1, index, team->current);

            if (battleSystem->battleState == BATTLE_FORCED_SWITCH)
            {
                actionQueueReady = true;
                battleSystem->battleState = BATTLE_PREPARING_ACTIONS;
            }
            else
            {
                battleSystem->playerTurn = false;
                battleSystem->battleState = BATTLE_SELECT_ACTION;
            }
        }

        current = current->next;
        index++;
    }

    // Overlay escuro para troca forçada
    if (battleSystem->battleState == BATTLE_FORCED_SWITCH)
    {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), ColorAlpha(BLACK, 0.5f));
    }
}


/**
 * Desenha o menu de itens com múltiplos itens disponíveis
 */
void drawItemMenu(Rectangle bounds) {
    // Variáveis para controlar os itens

    // Cores do estilo BW
    Color bgColor = (Color){40, 40, 40, 230};
    Color frameColor = (Color){0, 0, 0, 255};
    Color textColor = (Color){255, 255, 255, 255};

    // Fundo principal
    DrawRectangleRounded(bounds, 0.3f, 8, bgColor);
    DrawRectangleRoundedLines(bounds, 0.3f, 8, frameColor);

    // Título no estilo BW
    DrawText("USAR ITEM",
            bounds.x + 20,
            bounds.y + 15,
            20,
            textColor);

    // Botão de voltar
    Rectangle backBtn = {
        bounds.x + bounds.width - 100,
        bounds.y + 10,
        80,
        30
    };

    if (GuiPokemonButton(backBtn, "VOLTAR", true)) {
        PlaySound(selectSound);
        battleSystem->battleState = BATTLE_SELECT_ACTION;
    }

    // Calcular o layout similar ao menu de ações
    float marginX = 20;       // Margem lateral
    float marginY = 60;       // Margem superior (para o título)
    float buttonWidth = 150;  // Largura do botão
    float buttonHeight = 15;  // Altura do botão
    float spacingY = 15;      // Espaçamento vertical entre botões

    // Posição inicial dos botões (alinhados à esquerda)
    float startX = bounds.x + marginX;
    float startY = bounds.y + marginY;

    // Variáveis para controlar os itens
    static bool potionUsed = false;       // Controla se a poção foi usada
    static bool randomItemUsed = false;   // Controla se o item aleatório foi usado

    // Se voltar ao menu, resetar os itens (isso reflete que um novo turno começou)
    if (battleSystem->turn == 1 && !battleSystem->playerItemUsed) {
        potionUsed = false;
        randomItemUsed = false;
    }

    // NOVO: Sincronizar com a flag global
    if (battleSystem->playerItemUsed) {
        // Se o jogador já usou um item neste turno, ambos os itens
        // devem estar desabilitados
        potionUsed = true;
        randomItemUsed = true;
    }

    // Se o jogador já usou qualquer item neste turno, verificar qual
    if (battleSystem->playerItemUsed) {
        // O estado específico precisa ser mantido na tela de itens
        // Não resetamos aqui, apenas na mudança de turno
    }

    // Definir retângulos para os botões
    Rectangle item1Rect = {
        startX,
        startY,
        buttonWidth,
        buttonHeight
    };

    Rectangle item2Rect = {
        startX,
        startY + buttonHeight + spacingY,
        buttonWidth,
        buttonHeight
    };

    // Área de descrição (à direita dos botões)
    Rectangle descArea = {
        startX + buttonWidth + 20,
        startY,
        bounds.width - buttonWidth - marginX * 3,
        bounds.height - marginY - 30
    };

    // Nomes e descrições dos itens
    const char* potionName = "POÇÃO";
    const char* potionDesc = "Restaura 20 pontos de HP do seu Pokémon.";

    // Obter informações sobre o item aleatório para esta batalha
    const char* randomItemName;
    const char* randomItemDesc;
    Color randomItemColor;
    ItemType randomItemType;

    // Determinar o item aleatório com base no valor definido no início da batalha
    if (battleSystem->itemType == ITEM_RED_CARD) {
        randomItemName = "CARTÃO VERMELHO";
        randomItemDesc = "Força o oponente a trocar de Pokémon imediatamente.";
        randomItemColor = (Color){180, 50, 50, 255}; // Vermelho
        randomItemType = ITEM_RED_CARD;
    } else {
        randomItemName = "MOEDA DA SORTE";
        randomItemDesc = "Cara: Restaura HP total do seu Pokémon / Coroa: Seu Pokémon desmaia.";
        randomItemColor = (Color){180, 140, 40, 255}; // Amarelo
        randomItemType = ITEM_COIN;
    }

    // Elemento selecionado atualmente
    static int selectedItem = -1;

    // Verificar hover para seleção (apenas para itens disponíveis)
    selectedItem = -1; // Reset inicial

    if (!potionUsed && CheckCollisionPointRec(GetMousePosition(), item1Rect)) {
        selectedItem = 0;
    }
    else if (!randomItemUsed && CheckCollisionPointRec(GetMousePosition(), item2Rect)) {
        selectedItem = 1;
    }

    // Cores para os botões
    Color btn1Color = (Color){60, 170, 60, 255}; // Verde para Poção
    Color btn2Color = battleSystem->itemType == ITEM_RED_CARD ?
                    (Color){180, 50, 50, 255} :  // Vermelho para Cartão Vermelho
                    (Color){180, 140, 40, 255};  // Amarelo para Moeda da Sorte

    // Botão 1: Poção
    if (selectedItem == 0) {
        // Destacar seleção
        DrawRectangleRounded(
            (Rectangle){item1Rect.x - 5, item1Rect.y - 5, item1Rect.width + 10, item1Rect.height + 10},
            0.3f, 8, WHITE
        );
    }

    // Desenhar retângulo do botão com base no estado
    if (potionUsed) {
        // Botão desativado com cor cinza
        DrawRectangleRounded(item1Rect, 0.3f, 8, (Color){100, 100, 100, 150});
        DrawRectangleRoundedLines(item1Rect, 0.3f, 8, frameColor);
    } else {
        // Botão normal com cor verde
        DrawRectangleRounded(item1Rect, 0.3f, 8, btn1Color);
        DrawRectangleRoundedLines(item1Rect, 0.3f, 8, frameColor);
    }

    // Texto do botão 1
    DrawText(potionName,
            item1Rect.x + item1Rect.width/2 - MeasureText(potionName, 20)/2,
            item1Rect.y + item1Rect.height/2 - 10,
            20,
            potionUsed ? (Color){200, 200, 200, 150} : WHITE);

    // Botão 2: Item aleatório
    if (selectedItem == 1) {
        // Destacar seleção
        DrawRectangleRounded(
            (Rectangle){item2Rect.x - 5, item2Rect.y - 5, item2Rect.width + 10, item2Rect.height + 10},
            0.3f, 8, WHITE
        );
    }

    // Desenhar retângulo do botão com base no estado
    if (randomItemUsed) {
        // Botão desativado com cor cinza
        DrawRectangleRounded(item2Rect, 0.3f, 8, (Color){100, 100, 100, 150});
        DrawRectangleRoundedLines(item2Rect, 0.3f, 8, frameColor);
    } else {
        // Botão normal com cor apropriada para o item
        DrawRectangleRounded(item2Rect, 0.3f, 8, btn2Color);
        DrawRectangleRoundedLines(item2Rect, 0.3f, 8, frameColor);
    }

    // Texto do botão 2
    DrawText(randomItemName,
            item2Rect.x + item2Rect.width/2 - MeasureText(randomItemName, 20)/2,
            item2Rect.y + item2Rect.height/2 - 10,
            20,
            randomItemUsed ? (Color){200, 200, 200, 150} : WHITE);

    // Lidar com interação para cada item individualmente

    // Verificar clique no botão 1 (Poção) se não foi usada
    if (!potionUsed && CheckCollisionPointRec(GetMousePosition(), item1Rect) &&
        IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        PlaySound(selectSound);
        // Usar poção
        enqueue(battleSystem->actionQueue, 2, ITEM_POTION, battleSystem->playerTeam->current);
        potionUsed = true;
        battleSystem->playerItemUsed = true;
        battleSystem->playerTurn = false;
        battleSystem->battleState = BATTLE_SELECT_ACTION;
    }

    // Verificar clique no botão 2 (Item aleatório) se não foi usado
    if (!randomItemUsed && CheckCollisionPointRec(GetMousePosition(), item2Rect) &&
        IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        PlaySound(selectSound);
        // Usar o item aleatório
        enqueue(battleSystem->actionQueue, 2, randomItemType, battleSystem->playerTeam->current);
        randomItemUsed = true;
        battleSystem->playerItemUsed = true;
        battleSystem->playerTurn = false;
        battleSystem->battleState = BATTLE_SELECT_ACTION;
    }

    // Desenhar descrição do item selecionado à direita
    if (selectedItem >= 0) {
        const char* desc = selectedItem == 0 ? potionDesc : randomItemDesc;

        // Dividir o texto para caber na área de descrição
        int fontSize = 20;
        int lineHeight = fontSize + 5;
        int maxWidth = descArea.width;

        // Variáveis para processamento do texto
        int curTextY = descArea.y;
        char textBuffer[256] = "";
        int bufferWidth = 0;

        // Processar cada palavra
        char wordBuffer[64];
        int wordIndex = 0;

        for (int i = 0; i <= strlen(desc); i++) {
            if (desc[i] == ' ' || desc[i] == '\0') {
                // Finalizar palavra atual
                wordBuffer[wordIndex] = '\0';

                // Calcular tamanho
                int wordWidth = MeasureText(wordBuffer, fontSize);

                // Verificar se a palavra cabe na linha atual
                if (bufferWidth + wordWidth > maxWidth) {
                    // Desenhar linha atual e começar nova linha
                    DrawText(textBuffer, descArea.x, curTextY, fontSize, textColor);
                    curTextY += lineHeight;
                    strcpy(textBuffer, "");
                    bufferWidth = 0;
                }

                // Adicionar palavra ao buffer
                strcat(textBuffer, wordBuffer);
                bufferWidth += wordWidth;

                // Adicionar espaço após a palavra (exceto se for o final)
                if (desc[i] != '\0') {
                    strcat(textBuffer, " ");
                    bufferWidth += MeasureText(" ", fontSize);
                }

                // Resetar buffer de palavra
                wordIndex = 0;
            } else {
                // Adicionar caractere à palavra atual
                wordBuffer[wordIndex++] = desc[i];
            }
        }

        // Desenhar qualquer texto restante
        if (strlen(textBuffer) > 0) {
            DrawText(textBuffer, descArea.x, curTextY, fontSize, textColor);
        }
    }

    // Mensagem de item usado (se aplicável) - mudando para refletir o estado correto
    if (potionUsed && randomItemUsed) {
        const char* usedMsg = "Ambos os itens já foram usados";
        DrawText(usedMsg,
                bounds.x + bounds.width/2 - MeasureText(usedMsg, 18)/2,
                bounds.y + bounds.height - 30,
                18,
                (Color){255, 100, 100, 255});
    }
    else if (potionUsed) {
        const char* usedMsg = "Poção já foi usada neste turno";
        DrawText(usedMsg,
                bounds.x + bounds.width/2 - MeasureText(usedMsg, 18)/2,
                bounds.y + bounds.height - 30,
                18,
                (Color){255, 100, 100, 255});
    }
    else if (randomItemUsed) {
        const char* usedMsg = "Item especial já foi usado neste turno";
        DrawText(usedMsg,
                bounds.x + bounds.width/2 - MeasureText(usedMsg, 18)/2,
                bounds.y + bounds.height - 30,
                18,
                (Color){255, 100, 100, 255});
    }
}

// Divide texto em linhas para caber no limite de largura
int wrapTextLines(const char* text, char lines[][256], int maxLines, int maxWidth, int fontSize)
{
    int lineCount = 0;
    const char* ptr = text;
    char buffer[256] = "";
    int bufferLen = 0;

    while (*ptr && lineCount < maxLines)
    {
        buffer[bufferLen++] = *ptr;
        buffer[bufferLen] = '\0';

        if (*ptr == ' ' || *(ptr + 1) == '\0')
        {
            int width = MeasureText(buffer, fontSize);
            if (width >= maxWidth)
            {
                if (lineCount < maxLines)
                {
                    buffer[bufferLen - 1] = '\0'; // remove space
                    strcpy(lines[lineCount++], buffer);
                    bufferLen = 0;
                    buffer[0] = '\0';
                }
            }
        }

        ptr++;
    }

    if (bufferLen > 0 && lineCount < maxLines)
    {
        strcpy(lines[lineCount++], buffer);
    }

    return lineCount;
}


/**
 * Desenha a mensagem de batalha no estilo Pokémon Black/White
 */
void drawBattleMessage(Rectangle bounds) {
    // Cores do estilo BW
    Color bgColor = (Color){40, 40, 40, 230};
    Color frameColor = (Color){0, 0, 0, 255};
    Color textColor = (Color){255, 255, 255, 255};

    // Fundo da caixa com esquinas mais arredondadas
    DrawRectangleRounded(bounds, 0.3f, 8, bgColor);
    DrawRectangleRoundedLines(bounds, 0.3f, 8, frameColor);

    // Verificar se temos uma mensagem atual
    if (strlen(currentMessage.message) > 0) {
        // Inicializar typewriter se necessário
        static bool textInitialized = false;
        static char lastMessage[256] = "";

        if (!textInitialized || strcmp(lastMessage, currentMessage.message) != 0) {
            startTypewriter(currentMessage.message, currentMessage.waitingForInput);
            strcpy(lastMessage, currentMessage.message);
            textInitialized = true;
        }

        // Atualizar efeito de typing
        updateTypewriter();

        // Renderizar texto com quebras manuais
        char wrappedLines[5][256]; // Até 5 linhas de texto
        int fontSize = 24;
        int lineCount = wrapTextLines(typewriter.displayText, wrappedLines, 5, bounds.width - 40, fontSize);

        for (int i = 0; i < lineCount; i++) {
            DrawText(wrappedLines[i],
                    bounds.x + 20,
                    bounds.y + 20 + i * (fontSize + 8),
                    fontSize,
                    textColor);
        }

        // Indicador de continuar se o texto estiver completo
        if (typewriter.isComplete && typewriter.waitingForInput) {
            float blinkValue = sinf(typewriter.blinkTimer);
            if (blinkValue > 0) {
                // Triângulo no estilo BW (mais destacado)
                DrawTriangle(
                    (Vector2){bounds.x + bounds.width - 35, bounds.y + bounds.height - 25},
                    (Vector2){bounds.x + bounds.width - 15, bounds.y + bounds.height - 35},
                    (Vector2){bounds.x + bounds.width - 15, bounds.y + bounds.height - 15},
                    WHITE
                );
            }
        }
    } else {
        // Mensagem padrão se não tivermos uma específica
        DrawText("...",
                bounds.x + 20,
                bounds.y + 20,
                24,
                textColor);
    }
}

/**
 * Desenha o diálogo de confirmação
 */
void drawConfirmDialog(const char* message, const char* yesText, const char* noText)
{
    // Fundo semi-transparente
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){0, 0, 0, 150});

    // Caixa de diálogo
    Rectangle dialogBox = {
        GetScreenWidth() / 2 - 200,
        GetScreenHeight() / 2 - 100,
        400,
        200
    };

    DrawRectangleRounded(dialogBox, 0.2f, 6, (Color){240, 240, 240, 255});
    DrawRectangleRoundedLines(dialogBox, 0.2f, 6, BLACK);

    // Dividir texto longo manualmente para caber na caixa
    const char* line1 = "Tem certeza que deseja";
    const char* line2 = "fugir da batalha?";

    DrawText(line1,
             dialogBox.x + dialogBox.width / 2 - MeasureText(line1, 24) / 2,
             dialogBox.y + 35,
             24,
             BLACK);

    DrawText(line2,
             dialogBox.x + dialogBox.width / 2 - MeasureText(line2, 24) / 2,
             dialogBox.y + 65,
             24,
             BLACK);


    // Botões
    Rectangle yesBtn = {
        dialogBox.x + 50,
        dialogBox.y + 120,
        120,
        40
    };

    Rectangle noBtn = {
        dialogBox.x + 230,
        dialogBox.y + 120,
        120,
        40
    };

    if (GuiPokemonButton(yesBtn, yesText, true))
    {
        PlaySound(selectSound);
        // Voltar ao menu principal
        StopMusicStream(battleMusic);
        PlayMusicStream(menuMusic);
        currentScreen = MAIN_MENU;
        resetBattle();
    }

    if (GuiPokemonButton(noBtn, noText, true))
    {
        PlaySound(selectSound);
        // Continuar a batalha
        battleSystem->battleState = BATTLE_SELECT_ACTION;
    }
}

/**
 * Função principal para desenhar a tela de batalha
 */
void drawBattleScreen(void) {
    // Atualizar música
    UpdateMusicStream(battleMusic);

    // Fundo de batalha texturizado em tela inteira
    DrawTexturePro(
        battleBackground,
        (Rectangle){0, 0, (float)battleBackground.width, (float)battleBackground.height},
        (Rectangle){0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()},
        (Vector2){0, 0},
        0.0f,
        WHITE
    );

    // Verificar se a batalha está inicializada
    if (battleSystem == NULL ||
        battleSystem->playerTeam == NULL ||
        battleSystem->opponentTeam == NULL) {
        DrawText("ERRO: Sistema de batalha não inicializado!",
                GetScreenWidth()/2 - MeasureText("ERRO: Sistema de batalha não inicializado!", 30)/2,
                GetScreenHeight()/2,
                30,
                RED);
        return;
    }

    // Monstros ativos
    PokeMonster* playerMonster = battleSystem->playerTeam->current;
    PokeMonster* opponentMonster = battleSystem->opponentTeam->current;


    // Desenhar monstros
    drawMonsterInBattle(playerMonster, true);
    drawMonsterInBattle(opponentMonster, false);


    // Caixa do inimigo no topo
    Rectangle enemyStatusBox = {
        20,
        20,
        250,
        80
    };

    // Caixa do jogador mais para cima para evitar a sobreposição com a caixa de texto
    Rectangle playerStatusBox = {
        GetScreenWidth() - 280,
        GetScreenHeight() - 230, // Movido para cima, longe da caixa de texto
        250,
        80
    };

    drawMonsterStatusBox(playerMonster, playerStatusBox, true);
    drawMonsterStatusBox(opponentMonster, enemyStatusBox, false);

    // Caixa de mensagem ou menu de ações (maior e mais baixa)
    Rectangle actionBox = {
        20,
        GetScreenHeight() - 140,
        GetScreenWidth() - 40,
        120
    };

    // Desenhar interface baseada no estado atual
    switch (battleSystem->battleState) {
        case BATTLE_INTRO:
            // Apenas exibir a mensagem durante a introdução
            drawBattleMessage(actionBox);
            break;

        case BATTLE_SELECT_ACTION:
            if (isMonsterFainted(battleSystem->playerTeam->current)) {
                battleSystem->battleState = BATTLE_FORCED_SWITCH;
                battleSystem->playerTurn = true;
                return; // Sair imediatamente
            }
            if (battleSystem->playerTurn) {
                drawBattleActionMenu(actionBox);
            } else {
                // Mensagem de espera pelo bot
                strcpy(currentMessage.message, "O oponente está escolhendo sua ação...");
                currentMessage.displayTime = 0.5f;
                currentMessage.elapsedTime = 0.0f;
                currentMessage.waitingForInput = false;
                currentMessage.autoAdvance = false;
                drawBattleMessage(actionBox);
            }
            break;

        case BATTLE_SELECT_ATTACK:
            drawBattleAttackMenu(actionBox);
            break;

        case BATTLE_SELECT_MONSTER:
            // Ajuste de posicionamento para o menu de seleção de monstro
            actionBox = (Rectangle){
                50,
                GetScreenHeight() - 320,
                GetScreenWidth() - 100,
                300
            };
            drawMonsterSelectionMenu(actionBox);
            break;

        case BATTLE_FORCED_SWITCH:
            // Também ajustando para a troca forçada
            actionBox = (Rectangle){
                50,
                GetScreenHeight() - 320,
                GetScreenWidth() - 100,
                300
            };
            drawMonsterSelectionMenu(actionBox);
            break;

        case BATTLE_ITEM_MENU:
            drawItemMenu(actionBox);
            break;

        case BATTLE_MESSAGE_DISPLAY:
            drawBattleMessage(actionBox);
            break;

        case BATTLE_CONFIRM_QUIT:
            drawConfirmDialog("Tem certeza que deseja fugir da batalha?", "Sim", "Não");
            break;

        case BATTLE_OVER:
            // Determinar vencedor
            {
                int winner = getBattleWinner();
                const char* resultMsg;

                if (winner == 1) {
                    resultMsg = "Você venceu a batalha!";
                } else if (winner == 2) {
                    resultMsg = "Você perdeu a batalha!";
                } else {
                    resultMsg = "A batalha terminou em empate!";
                }

                strcpy(currentMessage.message, resultMsg);
                drawBattleMessage(actionBox);

                // Botão para voltar ao menu
                Rectangle menuBtn = {
                    GetScreenWidth() / 2 - 100,
                    GetScreenHeight() - 60,
                    200,
                    50
                };

                if (GuiPokemonButton(menuBtn, "MENU PRINCIPAL", true)) {
                    PlaySound(selectSound);
                    StopMusicStream(battleMusic);
                    PlayMusicStream(menuMusic);
                    currentScreen = MAIN_MENU;
                    resetBattle();
                }
            }
            break;

        default:
            // Outros estados, mostrar mensagem atual
            drawBattleMessage(actionBox);
            break;
    }

    // Desenhar indicador da API
    drawAIIndicator();
}

/**
 * Atualiza a lógica da tela de batalha
 */
void updateBattleScreen(void)
{
    // Atualizar temporizadores e efeitos
    float deltaTime = GetFrameTime();
    battleTimer += deltaTime;

    // Animar plataformas
    platformYOffset1 = sinf(battleTimer * 0.5f) * 5.0f;
    platformYOffset2 = cosf(battleTimer * 0.6f) * 5.0f;

    // Atualizar efeitos de flash e animação para os sprites
    if (playerSprite.flashAlpha > 0.0f)
    {
        playerSprite.flashAlpha -= deltaTime * 2.0f;
        if (playerSprite.flashAlpha < 0.0f) playerSprite.flashAlpha = 0.0f;
    }

    if (enemySprite.flashAlpha > 0.0f)
    {
        enemySprite.flashAlpha -= deltaTime * 2.0f;
        if (enemySprite.flashAlpha < 0.0f) enemySprite.flashAlpha = 0.0f;
    }

    // Atualizar animação de HP
    if (isHpAnimationActive)
    {
        hpAnimTimer += deltaTime;
        if (hpAnimTimer >= 1.0f)
        {
            isHpAnimationActive = false;
            hpAnimTimer = 0.0f;
        }
    }
    // Chamada para atualizar a lógica de batalha
    updateBattle();
}

// Função para aplicar efeito de flash ao receber dano
void applyDamageEffect(AnimatedSprite* sprite)
{
    if (sprite == NULL) return;
    sprite->flashAlpha = 1.0f;
}

void resetBattleSprites(void)
{
    // Resetar sprite do jogador
    if (playerSprite.texture.id != 0)
    {
        playerSprite.texture.id = 0;
    }

    // Resetar sprite do inimigo
    if (enemySprite.texture.id != 0)
    {
        enemySprite.texture.id = 0;
    }

    // Limpar outras propriedades se necessário
    playerSprite.frameCount = 0;
    playerSprite.currentFrame = 0;
    enemySprite.frameCount = 0;
    enemySprite.currentFrame = 0;
}

int countMonsters(MonsterList* team)
{
    int count = 0;
    PokeMonster* current = team->first;
    while (current != NULL)
    {
        count++;
        current = current->next;
    }
    return count;
}


