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
void drawMonsterInBattle(PokeMonster* monster, bool isPlayer)
{
    if (monster == NULL) return;

    Vector2 platformPos, monsterPos;
    float platformScale;
    Animation* currentAnim = NULL;
    float baseScale = isPlayer ? 3.0f : 2.5f;

    if (isPlayer)
    {
        platformPos = (Vector2){GetScreenWidth() / 4, GetScreenHeight() / 1.5f + 50};
        monsterPos = (Vector2){GetScreenWidth() / 4, GetScreenHeight() / 1.5f - 20};
        platformScale = 1.5f;
        currentAnim = &monster->backAnimation;
    }
    else
    {
        platformPos = (Vector2){GetScreenWidth() * 3 / 4, GetScreenHeight() / 3 + 50};
        monsterPos = (Vector2){GetScreenWidth() * 3 / 4, GetScreenHeight() / 3 - 20};
        platformScale = 1.2f;
        currentAnim = &monster->frontAnimation;
    }

    // Desenhar plataforma
    DrawEllipse(
        platformPos.x,
        platformPos.y,
        (isPlayer ? 100 : 80) * platformScale,
        (isPlayer ? 30 : 25) * platformScale,
        (Color){100, 120, 140, 200}
    );

    // Atualizar e desenhar animação
    if (currentAnim->frameCount > 0)
    {
        UpdateAnimation(currentAnim);

        Texture2D currentFrame = currentAnim->frames[currentAnim->currentFrame];
        float scale = baseScale * (isPlayer ? 1.0f : 0.9f);

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
    }
    else
    {
        // Fallback estático caso a animação falhe
        Texture2D fallback = isPlayer ? monster->backAnimation.frames[0] : monster->frontAnimation.frames[0];
        if (fallback.id != 0)
        {
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
    if (monster->statusCondition != STATUS_NONE)
    {
        Vector2 statusPos = isPlayer
                                ? (Vector2){monsterPos.x - 50, monsterPos.y - 70}
                                : (Vector2){monsterPos.x + 50, monsterPos.y - 60};

        Color statusColor;
        const char* statusText;

        switch (monster->statusCondition)
        {
        case STATUS_PARALYZED:
            statusColor = YELLOW;
            statusText = "PAR";
            break;
        case STATUS_SLEEPING:
            statusColor = DARKPURPLE;
            statusText = "SLP";
            break;
        case STATUS_BURNING:
            statusColor = RED;
            statusText = "BRN";
            break;
        case STATUS_ATK_DOWN:
            statusColor = MAROON;
            statusText = "ATK↓";
            break;
        case STATUS_DEF_DOWN:
            statusColor = DARKBLUE;
            statusText = "DEF↓";
            break;
        case STATUS_SPD_DOWN:
            statusColor = DARKGREEN;
            statusText = "SPD↓";
            break;
        default:
            statusColor = GRAY;
            statusText = "???";
            break;
        }

        // Desenhar bolha de status
        DrawCircle(
            statusPos.x,
            statusPos.y,
            20,
            statusColor
        );

        DrawCircleLines(
            statusPos.x,
            statusPos.y,
            20,
            BLACK
        );

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
 * Desenha a caixa de status de um monstro
 */
void drawMonsterStatusBox(PokeMonster* monster, Rectangle bounds, bool isPlayer)
{
    if (monster == NULL) return;

    // Desenhar fundo da caixa
    DrawRectangleRounded(bounds, 0.2f, 6, (Color){240, 240, 240, 230});
    DrawRectangleRoundedLines(bounds, 0.2f, 6, BLACK);

    // Nome e nível
    DrawText(monster->name,
             bounds.x + 15,
             bounds.y + 10,
             20,
             BLACK);

    DrawText("Nv.50",
             bounds.x + bounds.width - 70,
             bounds.y + 10,
             20,
             BLACK);

    // Tipos
    Rectangle type1Rect = {
        bounds.x + 15,
        bounds.y + 40,
        60,
        20
    };

    GuiPokemonTypeIcon(type1Rect, monster->type1);

    if (monster->type2 != TYPE_NONE)
    {
        Rectangle type2Rect = {
            bounds.x + 85,
            bounds.y + 40,
            60,
            20
        };

        GuiPokemonTypeIcon(type2Rect, monster->type2);
    }

    // HP atual e máximo
    char hpText[32];
    sprintf(hpText, "HP: %d/%d", monster->hp, monster->maxHp);

    if (isPlayer)
    {
        DrawText(hpText,
                 bounds.x + 15,
                 bounds.y + bounds.height - 30,
                 18,
                 BLACK);
    }

    // Barra de HP
    Rectangle hpBarBounds = {
        bounds.x + 15,
        bounds.y + 70,
        bounds.width - 30,
        15
    };

    // Desenhar a barra de HP
    drawHealthBar(hpBarBounds, monster->hp, monster->maxHp, monster);
}

/**
 * Desenha o menu de ações da batalha
 */
void drawBattleActionMenu(Rectangle bounds)
{
    // Fundo da caixa de ações
    DrawRectangleRounded(bounds, 0.2f, 6, (Color){240, 240, 240, 230});
    DrawRectangleRoundedLines(bounds, 0.2f, 6, BLACK);

    const char* options[] = {
        "LUTAR",
        "MOCHILA",
        "POKÉMON",
        "FUGIR"
    };

    int cols = 2;
    int rows = 2;
    float width = (bounds.width - 60) / cols;
    float height = (bounds.height - 40) / rows;
    float startX = bounds.x + 20;
    float startY = bounds.y + 20;

    for (int i = 0; i < 4; i++)
    {
        int row = i / cols;
        int col = i % cols;

        Rectangle optionRect = {
            startX + col * (width + 20),
            startY + row * (height + 20),
            width,
            height
        };

        // Cores diferentes para cada opção
        Color buttonColor;
        switch (i)
        {
        case 0: buttonColor = RED;
            break; // LUTAR
        case 1: buttonColor = BLUE;
            break; // MOCHILA
        case 2: buttonColor = GREEN;
            break; // POKÉMON
        case 3: buttonColor = YELLOW;
            break; // FUGIR
        default: buttonColor = GRAY;
        }

        // Verificar se a opção está disponível (MOCHILA só disponível se item não foi usado)
        bool isEnabled = true;
        if (i == 1 && battleSystem->itemUsed)
        {
            isEnabled = false;
            buttonColor = GRAY;
        }

        if (GuiPokemonButton(optionRect, options[i], isEnabled))
        {
            PlaySound(selectSound);

            // Ação com base na opção
            battleSystem->selectedAction = i;

            switch (i)
            {
            case 0: // LUTAR
                battleSystem->battleState = BATTLE_SELECT_ATTACK;
                break;
            case 1: // MOCHILA
                if (!battleSystem->itemUsed)
                {
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
void drawBattleAttackMenu(Rectangle bounds)
{
    // Fundo da caixa de ataques
    DrawRectangleRounded(bounds, 0.2f, 6, (Color){240, 240, 240, 230});
    DrawRectangleRoundedLines(bounds, 0.2f, 6, BLACK);

    // Título
    DrawText("SELECIONE UM ATAQUE",
             bounds.x + 20,
             bounds.y + 15,
             20,
             BLACK);

    // Botão de voltar
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

    // Mostrar ataques disponíveis
    PokeMonster* monster = battleSystem->playerTeam->current;

    float attackWidth = (bounds.width - 60) / 2;
    float attackHeight = 50;
    float startX = bounds.x + 20;
    float startY = bounds.y + 50;

    for (int i = 0; i < 4; i++)
    {
        int row = i / 2;
        int col = i % 2;

        Rectangle attackRect = {
            startX + col * (attackWidth + 20),
            startY + row * (attackHeight + 10),
            attackWidth,
            attackHeight
        };

        // Cor baseada no tipo
        Color attackColor = getTypeColor(monster->attacks[i].type);

        // Desativar se não tiver PP
        bool canUse = monster->attacks[i].ppCurrent > 0;
        if (!canUse)
        {
            attackColor.r = (attackColor.r + 200) / 3;
            attackColor.g = (attackColor.g + 200) / 3;
            attackColor.b = (attackColor.b + 200) / 3;
        }

        if (GuiPokemonButton(attackRect, monster->attacks[i].name, canUse))
        {
            PlaySound(selectSound);
            battleSystem->selectedAttack = i;

            // Enfileirar ação de ataque
            enqueue(battleSystem->actionQueue, 0, i, battleSystem->playerTeam->current);

            // Passar o turno para o bot escolher
            battleSystem->playerTurn = false;
            battleSystem->battleState = BATTLE_SELECT_ACTION;
        }

        // Informações do ataque
        char ppText[20];
        sprintf(ppText, "PP: %d/%d",
                monster->attacks[i].ppCurrent,
                monster->attacks[i].ppMax);

        DrawText(ppText,
                 attackRect.x + 10,
                 attackRect.y + 30,
                 14,
                 WHITE);

        // Poder do ataque
        if (monster->attacks[i].power > 0)
        {
            char powerText[20];
            sprintf(powerText, "Poder: %d", monster->attacks[i].power);

            DrawText(powerText,
                     attackRect.x + attackRect.width - 100,
                     attackRect.y + 30,
                     14,
                     WHITE);
        }
        else
        {
            DrawText("Status",
                     attackRect.x + attackRect.width - 70,
                     attackRect.y + 30,
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
 * Desenha o menu de itens
 */
void drawItemMenu(Rectangle bounds)
{
    // Fundo da caixa
    DrawRectangleRounded(bounds, 0.2f, 6, (Color){240, 240, 240, 230});
    DrawRectangleRoundedLines(bounds, 0.2f, 6, BLACK);

    // Título
    DrawText("USAR ITEM",
             bounds.x + 20,
             bounds.y + 15,
             20,
             BLACK);

    // Botão de voltar
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

    // Mostrar item disponível
    Rectangle itemRect = {
        bounds.x + 20,
        bounds.y + 50,
        bounds.width - 40,
        70
    };

    Color itemColor;
    const char* itemName;
    const char* itemDesc;

    switch (battleSystem->itemType)
    {
    case ITEM_POTION:
        itemColor = GREEN;
        itemName = "Poção";
        itemDesc = "Restaura 20 pontos de HP";
        break;
    case ITEM_RED_CARD:
        itemColor = RED;
        itemName = "Cartão Vermelho";
        itemDesc = "Força o oponente a trocar de Pokémon";
        break;
    case ITEM_COIN:
        itemColor = YELLOW;
        itemName = "Moeda da Sorte";
        itemDesc = "Cara: HP total / Coroa: Desmaio";
        break;
    default:
        itemColor = GRAY;
        itemName = "Item Desconhecido";
        itemDesc = "???";
    }

    DrawRectangleRounded(itemRect, 0.2f, 6, itemColor);
    DrawRectangleRoundedLines(itemRect, 0.2f, 6, BLACK);

    DrawText(itemName,
             itemRect.x + 10,
             itemRect.y + 10,
             24,
             WHITE);

    DrawText(itemDesc,
             itemRect.x + 10,
             itemRect.y + 40,
             18,
             WHITE);

    // Desenhar botão de usar
    Rectangle useBtn = {
        bounds.x + bounds.width / 2 - 75,
        bounds.y + bounds.height - 60,
        150,
        40
    };

    if (GuiPokemonButton(useBtn, "USAR", true))
    {
        PlaySound(selectSound);

        // Enfileirar ação de usar item
        enqueue(battleSystem->actionQueue, 2, battleSystem->itemType, battleSystem->playerTeam->current);

        // Passar o turno para o bot escolher
        battleSystem->playerTurn = false;
        battleSystem->battleState = BATTLE_SELECT_ACTION;
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
 * Desenha a mensagem de batalha
 */
void drawBattleMessage(Rectangle bounds)
{
    // Fundo da caixa
    DrawRectangleRounded(bounds, 0.2f, 6, (Color){240, 240, 240, 230});
    DrawRectangleRoundedLines(bounds, 0.2f, 6, BLACK);

    // Verificar se temos uma mensagem atual
    if (strlen(currentMessage.message) > 0)
    {
        // Inicializar typewriter se necessário
        static bool textInitialized = false;
        static char lastMessage[256] = "";

        if (!textInitialized || strcmp(lastMessage, currentMessage.message) != 0)
        {
            startTypewriter(currentMessage.message, currentMessage.waitingForInput);
            strcpy(lastMessage, currentMessage.message);
            textInitialized = true;
        }

        // Atualizar efeito de typing
        updateTypewriter();

        // Desenhar texto
        Vector2 textPos = {bounds.x + 20, bounds.y + 20};

        // Desenhar texto com efeito typewriter
        int fontSize = 24;
        // Renderizar texto com quebras manuais
        char wrappedLines[5][256]; // Até 5 linhas de texto
        int lineCount = wrapTextLines(typewriter.displayText, wrappedLines, 5, bounds.width - 40, fontSize);

        for (int i = 0; i < lineCount; i++)
        {
            DrawText(wrappedLines[i],
                     bounds.x + 20,
                     bounds.y + 20 + i * (fontSize + 8),
                     fontSize,
                     BLACK);
        }


        // Indicador de continuar se o texto estiver completo
        if (typewriter.isComplete && typewriter.waitingForInput)
        {
            float blinkValue = sinf(typewriter.blinkTimer);
            if (blinkValue > 0)
            {
                DrawTriangle(
                    (Vector2){bounds.x + bounds.width - 30, bounds.y + bounds.height - 20},
                    (Vector2){bounds.x + bounds.width - 10, bounds.y + bounds.height - 30},
                    (Vector2){bounds.x + bounds.width - 10, bounds.y + bounds.height - 10},
                    BLACK
                );
            }
        }
    }
    else
    {
        // Mensagem padrão se não tivermos uma específica
        DrawText("...",
                 bounds.x + 20,
                 bounds.y + 20,
                 24,
                 BLACK);
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
void drawBattleScreen(void)
{
    // Atualizar música
    UpdateMusicStream(battleMusic);

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
        battleSystem->opponentTeam == NULL)
    {
        DrawText("ERRO: Sistema de batalha não inicializado!",
                 GetScreenWidth() / 2 - MeasureText("ERRO: Sistema de batalha não inicializado!", 30) / 2,
                 GetScreenHeight() / 2,
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

    // Desenhar caixas de status
    Rectangle playerStatusBox = {
        GetScreenWidth() - 250,
        GetScreenHeight() - 120,
        240,
        110
    };

    Rectangle enemyStatusBox = {
        10,
        10,
        240,
        100
    };

    drawMonsterStatusBox(playerMonster, playerStatusBox, true);
    drawMonsterStatusBox(opponentMonster, enemyStatusBox, false);

    // Caixa de mensagem ou menu de ações
    Rectangle actionBox = {
        GetScreenWidth() / 2 - 350,
        GetScreenHeight() - 150,
        700,
        140
    };

    // Desenhar interface baseada no estado atual
    switch (battleSystem->battleState)
    {
    case BATTLE_INTRO:
        // Apenas exibir a mensagem durante a introdução
        drawBattleMessage(actionBox);
        break;

    case BATTLE_SELECT_ACTION:
        if (isMonsterFainted(battleSystem->playerTeam->current))
        {
            battleSystem->battleState = BATTLE_FORCED_SWITCH;
            battleSystem->playerTurn = true;
            return; // Sair imediatamente
        }
        if (battleSystem->playerTurn)
        {
            // CORREÇÃO: Imprimir informação de debug para verificar se está entrando aqui
            printf("[DEBUG RENDER] Desenhando menu de ações para o jogador\n");
            drawBattleActionMenu(actionBox);
        }
        else
        {
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
        // Reduzir a área do menu para garantir que caiba na tela
        actionBox = (Rectangle){50, GetScreenHeight() - 320, GetScreenWidth() - 100, 300};
        drawMonsterSelectionMenu(actionBox);


    case BATTLE_FORCED_SWITCH:
        // Reduzir a área do menu para garantir que caiba na tela
        actionBox = (Rectangle){50, GetScreenHeight() - 320, GetScreenWidth() - 100, 300};
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

            if (winner == 1)
            {
                resultMsg = "Você venceu a batalha!";
            }
            else if (winner == 2)
            {
                resultMsg = "Você perdeu a batalha!";
            }
            else
            {
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

            if (GuiPokemonButton(menuBtn, "MENU PRINCIPAL", true))
            {
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
