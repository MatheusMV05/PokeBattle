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

void displayBattleMessage(const char* message, float duration, bool waitForInput, bool autoAdvance);

// Declaração de variáveis globais
extern BattleMessage currentMessage;
extern BattleAnimation currentAnimation;
extern bool actionQueueReady;
extern BattleSystem* battleSystem;

// Adicione a declaração da variável battleMessage se não existir
static char battleMessage[256] = "";

// ADICIONE A DECLARAÇÃO DA VARIÁVEL EFFECTS
static BattleEffect effects[MAX_EFFECTS] = {0};


void executeMonsterSwitch(PokeMonster* monster, int targetIndex) {
    MonsterList* team = NULL;
    bool isPlayer = false;

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

void executeItemUse(PokeMonster* user, ItemType itemType) {
    useItem(itemType, user);

    // A mensagem já é definida em useItem, apenas mudar para o novo sistema
    displayBattleMessage(battleMessage, 2.0f, false, true);

    battleSystem->itemUsed = true;
}


// Função para desenhar a tela de batalha
void drawBattleScreen(void) {
    // Atualizar música da batalha
    UpdateMusicStream(battleMusic);

    // Desenhar background selecionado
    if (currentBattleBackground >= 0 &&
        currentBattleBackground < BATTLE_BACKGROUNDS_COUNT &&
        battleBackgrounds[currentBattleBackground].id != 0) {

        // Desenhar o background em tela cheia
        Rectangle source = {
            0, 0,
            (float)battleBackgrounds[currentBattleBackground].width,
            (float)battleBackgrounds[currentBattleBackground].height
        };

        Rectangle dest = {
            0, 0,
            (float)GetScreenWidth(),
            (float)GetScreenHeight()
        };

        DrawTexturePro(
            battleBackgrounds[currentBattleBackground],
            source,
            dest,
            (Vector2){ 0, 0 },
            0.0f,
            WHITE
        );
        } else {
            // Fallback caso o background não carregue
            ClearBackground((Color){ 120, 200, 255, 255 });
            DrawRectangle(0, GetScreenHeight()/2, GetScreenWidth(), GetScreenHeight()/2,
                          (Color){ 120, 180, 100, 255 });
        }
    // Desenhar monstros em batalha e suas informações
    if (battleSystem != NULL &&
        battleSystem->playerTeam != NULL && battleSystem->playerTeam->current != NULL &&
        battleSystem->opponentTeam != NULL && battleSystem->opponentTeam->current != NULL) {

        PokeMonster* playerMonster = battleSystem->playerTeam->current;
        PokeMonster* opponentMonster = battleSystem->opponentTeam->current;

        // Definir retângulos para os monstros
        Rectangle opponentRect = { GetScreenWidth()/4 - 80, GetScreenHeight()/2 - 130, 160, 160 };
        Rectangle playerRect = { GetScreenWidth() - GetScreenWidth()/4 - 80, GetScreenHeight()/2 - 50, 160, 160 };

        // Boxes de informação estilo Pokémon
        // Box do oponente (superior direito)
        DrawRectangleRounded((Rectangle){ GetScreenWidth() - 280, 20, 260, 100 }, 0.2f, 10, (Color){ 240, 240, 240, 255 });
        DrawRectangleRoundedLines((Rectangle){ GetScreenWidth() - 280, 20, 260, 100 }, 0.2f, 10, BLACK);

        // Nome do monstro oponente
        DrawText(opponentMonster->name, GetScreenWidth() - 270, 30, 24, BLACK);

        // Barra de HP
        DrawText("HP:", GetScreenWidth() - 270, 60, 20, BLACK);
        drawHealthBar((Rectangle){ GetScreenWidth() - 230, 65, 200, 15 },
              opponentMonster->hp, opponentMonster->maxHp, opponentMonster);

        // Box do jogador (inferior esquerdo)
        DrawRectangleRounded((Rectangle){ 20, GetScreenHeight() - 120, 260, 100 }, 0.2f, 10, (Color){ 240, 240, 240, 255 });
        DrawRectangleRoundedLines((Rectangle){ 20, GetScreenHeight() - 120, 260, 100 }, 0.2f, 10, BLACK);

        // Nome do monstro do jogador
        DrawText(playerMonster->name, 30, GetScreenHeight() - 110, 24, BLACK);

        // Barra de HP com números
        DrawText("HP:", 30, GetScreenHeight() - 80, 20, BLACK);
        drawHealthBar((Rectangle){ 70, GetScreenHeight() - 75, 200, 15 },
              playerMonster->hp, playerMonster->maxHp, playerMonster);

        // Exibir os valores numéricos de HP
        char hpText[32];
        sprintf(hpText, "%d/%d", playerMonster->hp, playerMonster->maxHp);
        DrawText(hpText, 120, GetScreenHeight() - 50, 20, BLACK);

        // Desenhar os monstros com suas texturas
        drawMonsterInBattle(opponentMonster, opponentRect, false);
        drawMonsterInBattle(playerMonster, playerRect, true);

        // NOVA SEÇÃO: Desenhar mensagens com o novo sistema
        if (battleSystem->battleState == BATTLE_MESSAGE_DISPLAY ||
            battleSystem->battleState == BATTLE_INTRO ||
            strlen(currentMessage.message) > 0) {

            // Caixa de mensagem estilo Pokémon
            Rectangle messageBox = { 20, GetScreenHeight() - 180, GetScreenWidth() - 40, 50 };
            DrawRectangleRounded(messageBox, 0.2f, 10, (Color){ 240, 240, 240, 250 });
            DrawRectangleRoundedLines(messageBox, 0.2f, 10, BLACK);

            // Desenhar mensagem com efeito de digitação
            int visibleChars = strlen(currentMessage.message);

            if (currentMessage.elapsedTime < 0.5f) {
                // Efeito de digitação
                float typingProgress = currentMessage.elapsedTime / 0.5f;
                visibleChars = (int)(strlen(currentMessage.message) * typingProgress);
            }

            char displayText[256] = {0};
            strncpy(displayText, currentMessage.message, visibleChars);

            DrawText(displayText, messageBox.x + 15, messageBox.y + 15, 20, BLACK);

            // Indicador de continuar (apenas se esperando input)
            if (currentMessage.waitingForInput && currentMessage.elapsedTime > 0.5f) {
                static float blinkTimer = 0.0f;
                blinkTimer += GetFrameTime() * 3.0f;

                if (sinf(blinkTimer) > 0.0f) {
                    // Triângulo pequeno no canto inferior direito
                    Vector2 v1 = {messageBox.x + messageBox.width - 30, messageBox.y + messageBox.height - 15};
                    Vector2 v2 = {v1.x + 10, v1.y};
                    Vector2 v3 = {v1.x + 5, v1.y + 8};
                    DrawTriangle(v1, v2, v3, BLACK);
                }
            }
        } else {
            // Mensagem padrão quando não há mensagem específica
            Rectangle messageBox = { 20, GetScreenHeight() - 180, GetScreenWidth() - 40, 50 };
            DrawRectangleRounded(messageBox, 0.2f, 10, (Color){ 240, 240, 240, 250 });
            DrawRectangleRoundedLines(messageBox, 0.2f, 10, BLACK);
            DrawText(getBattleDescription(), messageBox.x + 15, messageBox.y + 15, 20, BLACK);
        }

        // Desenhar menu de batalha estilo Pokémon
        switch (battleSystem->battleState) {
            case BATTLE_INTRO:
                // Durante a introdução, apenas mostrar a mensagem
                break;

            case BATTLE_SELECT_ACTION:
                // Mostrar opções de ação se for o turno do jogador
                if (battleSystem->playerTurn) {
                    if (isMonsterFainted(battleSystem->playerTeam->current)) {
                        // Forçar o estado de troca
                        battleSystem->battleState = BATTLE_FORCED_SWITCH;
                        return;
                    }

                    // Caixa de menu principal
                    Rectangle menuBox = { GetScreenWidth() - 280, GetScreenHeight() - 180, 260, 160 };
                    DrawRectangleRounded(menuBox, 0.2f, 10, (Color){ 240, 240, 240, 250 });
                    DrawRectangleRoundedLines(menuBox, 0.2f, 10, BLACK);

                    // Divisória central
                    DrawLine(menuBox.x + menuBox.width/2, menuBox.y,
                             menuBox.x + menuBox.width/2, menuBox.y + menuBox.height,
                             BLACK);

                    // Divisória horizontal
                    DrawLine(menuBox.x, menuBox.y + menuBox.height/2,
                             menuBox.x + menuBox.width, menuBox.y + menuBox.height/2,
                             BLACK);

                    // Opções de menu (4 quadrantes)
                    int padding = 15;
                    Rectangle fightBtn = { menuBox.x + padding, menuBox.y + padding,
                                          menuBox.width/2 - padding*2, menuBox.height/2 - padding*2 };
                    Rectangle bagBtn = { menuBox.x + menuBox.width/2 + padding, menuBox.y + padding,
                                        menuBox.width/2 - padding*2, menuBox.height/2 - padding*2 };
                    Rectangle monsterBtn = { menuBox.x + padding, menuBox.y + menuBox.height/2 + padding,
                                           menuBox.width/2 - padding*2, menuBox.height/2 - padding*2 };
                    Rectangle runBtn = { menuBox.x + menuBox.width/2 + padding, menuBox.y + menuBox.height/2 + padding,
                                        menuBox.width/2 - padding*2, menuBox.height/2 - padding*2 };

                    // Botão Lutar
                    if (drawButton(fightBtn, "LUTAR", (Color){ 240, 80, 80, 255 })) {
                        printf("[DEBUG] Botão LUTAR clicado\n");
                        PlaySound(selectSound);
                        battleSystem->battleState = BATTLE_SELECT_ATTACK;
                    }

                    // Botão Mochila/Item
                    Color itemColor = battleSystem->itemUsed ? GRAY : (Color){ 120, 120, 255, 255 };
                    if (!battleSystem->itemUsed && drawButton(bagBtn, "ITEM", itemColor)) {
                        PlaySound(selectSound);
                        battleSystem->battleState = BATTLE_ITEM_MENU;
                    } else if (battleSystem->itemUsed) {
                        DrawRectangleRounded(bagBtn, 0.2f, 10, itemColor);
                        DrawRectangleRoundedLines(bagBtn, 0.2f, 10, BLACK);
                        DrawText("ITEM", bagBtn.x + bagBtn.width/2 - MeasureText("ITEM", 20)/2,
                                bagBtn.y + bagBtn.height/2 - 10, 20, WHITE);
                    }

                    // Botão Monstros
                    if (drawButton(monsterBtn, "TROCAR", (Color){ 120, 200, 80, 255 })) {
                        PlaySound(selectSound);
                        battleSystem->battleState = BATTLE_SELECT_MONSTER;
                    }

                    // Botão Fugir/Desistir
                    if (drawButton(runBtn, "FUGIR", (Color){ 200, 120, 200, 255 })) {
                        PlaySound(selectSound);
                        battleSystem->battleState = BATTLE_CONFIRM_QUIT;
                    }
                } else {
                    // Se for o turno do bot, mostrar mensagem de espera
                    Rectangle waitBox = { GetScreenWidth() - 280, GetScreenHeight() - 180, 260, 160 };
                    DrawRectangleRounded(waitBox, 0.2f, 10, (Color){ 240, 240, 240, 250 });
                    DrawRectangleRoundedLines(waitBox, 0.2f, 10, BLACK);

                    DrawText("Oponente está", waitBox.x + 20, waitBox.y + 60, 24, BLACK);
                    DrawText("pensando...", waitBox.x + 20, waitBox.y + 90, 24, BLACK);
                }
                break;

            case BATTLE_SELECT_ATTACK:
                if (battleSystem->playerTurn) {
                    // Menu de ataques estilo Fire Red
                    Rectangle attackBox = { 20, GetScreenHeight() - 120, GetScreenWidth() - 40, 100 };
                    DrawRectangleRounded(attackBox, 0.2f, 10, (Color){ 240, 240, 240, 250 });
                    DrawRectangleRoundedLines(attackBox, 0.2f, 10, BLACK);

                    // Título
                    DrawText("ATAQUES:", attackBox.x + 15, attackBox.y + 10, 20, BLACK);

                    // Divisória após o título
                    DrawLine(attackBox.x, attackBox.y + 35,
                             attackBox.x + attackBox.width, attackBox.y + 35,
                             (Color){ 200, 200, 200, 255 });

                    // Grid 2x2 para ataques
                    int btnWidth = (attackBox.width - 60) / 2;
                    int btnHeight = 25;

                    for (int i = 0; i < 4; i++) {
                        int row = i / 2;
                        int col = i % 2;

                        Rectangle attackBtn = {
                            attackBox.x + 15 + col * (btnWidth + 30),
                            attackBox.y + 45 + row * (btnHeight + 15),
                            btnWidth,
                            btnHeight
                        };

                        Color attackBtnColor = getTypeColor(playerMonster->attacks[i].type);

                        // Deixar mais claro se o PP estiver zero
                        if (playerMonster->attacks[i].ppCurrent <= 0) {
                            attackBtnColor.r = (attackBtnColor.r + 200) / 2;
                            attackBtnColor.g = (attackBtnColor.g + 200) / 2;
                            attackBtnColor.b = (attackBtnColor.b + 200) / 2;
                        }

                        if (playerMonster->attacks[i].ppCurrent > 0 &&
                            drawButton(attackBtn, playerMonster->attacks[i].name, attackBtnColor)) {

                            PlaySound(selectSound);
                            battleSystem->selectedAttack = i;

                            // Enfileirar ação de ataque
                            enqueue(battleSystem->actionQueue, 0, i, battleSystem->playerTeam->current);

                            // Mostrar mensagem de confirmação usando o novo sistema
                            sprintf(battleMessage, "%s vai usar %s!",
                                    playerMonster->name,
                                    playerMonster->attacks[i].name);
                            displayBattleMessage(battleMessage, 1.0f, false, true);

                            // Agora é a vez do bot
                            battleSystem->playerTurn = false;
                            battleSystem->battleState = BATTLE_SELECT_ACTION;
                            actionQueueReady = false;
                        } else if (playerMonster->attacks[i].ppCurrent <= 0) {
                            // Ataque sem PP (desabilitado)
                            DrawRectangleRounded(attackBtn, 0.2f, 10, attackBtnColor);
                            DrawRectangleRoundedLines(attackBtn, 0.2f, 10, BLACK);
                            DrawText(playerMonster->attacks[i].name,
                                    attackBtn.x + 10,
                                    attackBtn.y + btnHeight/2 - 10,
                                    18, (Color){ 80, 80, 80, 255 });
                        }

                        // PP do ataque
                        char ppText[20];
                        sprintf(ppText, "PP: %d/%d",
                               playerMonster->attacks[i].ppCurrent,
                               playerMonster->attacks[i].ppMax);

                        DrawText(ppText,
                                attackBtn.x + btnWidth - MeasureText(ppText, 15) - 5,
                                attackBtn.y + btnHeight + 2,
                                15, DARKGRAY);
                    }

                    // Botão de voltar
                    if (drawButton((Rectangle){ attackBox.x + attackBox.width - 80, attackBox.y + 10, 70, 25 },
                                  "VOLTAR", GRAY)) {
                        PlaySound(selectSound);
                        battleSystem->battleState = BATTLE_SELECT_ACTION;
                    }
                }
                break;

            case BATTLE_PREPARING_ACTIONS:
                // Não mostrar menu durante preparação
                break;

            case BATTLE_ATTACK_ANIMATION:
                // Durante animação de ataque, apenas mostrar o efeito visual
                if (currentAnimation.isAnimating) {
                    // O efeito de ataque é desenhado pela função drawBattleEffects()
                    // Talvez adicionar algum indicador visual adicional aqui
                }
                break;

            case BATTLE_DAMAGE_ANIMATION:
                // Durante animação de dano, possivelmente adicionar efeito de screen shake
                break;

            case BATTLE_MESSAGE_DISPLAY:
                // Mensagem já é mostrada no topo da função
                break;

            case BATTLE_TURN_END:
                // Processando fim de turno
                break;

            case BATTLE_ITEM_MENU:
                // Confirmar uso do item
                if (battleSystem->playerTurn) {
                    // Caixa de menu de item estilo Fire Red
                    Rectangle itemBox = { 20, GetScreenHeight() - 180, GetScreenWidth() - 40, 160 };
                    DrawRectangleRounded(itemBox, 0.2f, 10, (Color){ 240, 240, 240, 250 });
                    DrawRectangleRoundedLines(itemBox, 0.2f, 10, BLACK);

                    // Título
                    DrawText("MOCHILA", itemBox.x + 15, itemBox.y + 10, 24, BLACK);

                    // Divisória após o título
                    DrawLine(itemBox.x, itemBox.y + 40,
                             itemBox.x + itemBox.width, itemBox.y + 40,
                             (Color){ 200, 200, 200, 255 });

                    // Zona de visualização do item
                    Rectangle itemDisplayArea = { itemBox.x + 20, itemBox.y + 50, 100, 100 };
                    Rectangle itemIconArea = { itemDisplayArea.x + 25, itemDisplayArea.y + 20, 50, 50 };

                    // Desenhar ícone e fundo do item
                    Color itemBgColor = (Color){ 230, 230, 230, 255 };
                    DrawRectangleRounded(itemDisplayArea, 0.2f, 10, itemBgColor);

                    // Ícone do item (representação simplificada)
                    switch (battleSystem->itemType) {
                        case ITEM_POTION:
                            // Poção (roxo)
                            DrawRectangleRounded(itemIconArea, 0.5f, 10, (Color){ 180, 100, 230, 255 });
                            DrawRectangleRoundedLines(itemIconArea, 0.5f, 10, BLACK);
                            // Detalhes da poção
                            DrawRectangle(itemIconArea.x + 15, itemIconArea.y - 5, 20, 10, (Color){ 220, 220, 220, 255 });
                            DrawRectangleLines(itemIconArea.x + 15, itemIconArea.y - 5, 20, 10, BLACK);
                            break;

                        case ITEM_RED_CARD:
                            // Cartão vermelho
                            DrawRectangleRounded(itemIconArea, 0.1f, 5, RED);
                            DrawRectangleRoundedLines(itemIconArea, 0.1f, 5, BLACK);
                            // Detalhes do cartão
                            DrawLine(itemIconArea.x + 10, itemIconArea.y + 10,
                                     itemIconArea.x + 40, itemIconArea.y + 40, WHITE);
                            DrawLine(itemIconArea.x + 40, itemIconArea.y + 10,
                                     itemIconArea.x + 10, itemIconArea.y + 40, WHITE);
                            break;

                        case ITEM_COIN:
                            // Moeda (dourada)
                            DrawCircle(itemIconArea.x + 25, itemIconArea.y + 25, 20, (Color){ 230, 190, 40, 255 });
                            DrawCircleLines(itemIconArea.x + 25, itemIconArea.y + 25, 20, BLACK);
                            // Detalhes da moeda
                            DrawText("$", itemIconArea.x + 20, itemIconArea.y + 15, 24, (Color){ 200, 160, 30, 255 });
                            break;
                    }

                    // Nome do item abaixo do ícone
                    char itemName[32];
                    switch (battleSystem->itemType) {
                        case ITEM_POTION: strcpy(itemName, "POÇÃO"); break;
                        case ITEM_RED_CARD: strcpy(itemName, "CARTÃO VERMELHO"); break;
                        case ITEM_COIN: strcpy(itemName, "MOEDA DA SORTE"); break;
                        default: strcpy(itemName, "ITEM"); break;
                    }

                    DrawText(itemName,
                            itemDisplayArea.x + 50 - MeasureText(itemName, 16)/2,
                            itemDisplayArea.y + 80,
                            16, BLACK);

                    // Descrição do item
                    Rectangle descArea = { itemBox.x + 140, itemBox.y + 50, itemBox.width - 160, 70 };
                    DrawRectangleRounded(descArea, 0.2f, 10, (Color){ 230, 230, 230, 255 });
                    DrawRectangleRoundedLines(descArea, 0.2f, 10, BLACK);

                    char itemDesc[128];
                    switch (battleSystem->itemType) {
                        case ITEM_POTION:
                            strcpy(itemDesc, "Restaura 20 pontos de HP do seu monstro.");
                            break;
                        case ITEM_RED_CARD:
                            strcpy(itemDesc, "Força o oponente a trocar de monstro.");
                            break;
                        case ITEM_COIN:
                            strcpy(itemDesc, "50% chance de curar todo HP, 50% chance de desmaiar.");
                            break;
                        default:
                            strcpy(itemDesc, "Item desconhecido.");
                            break;
                    }

                    // Desenhar texto com quebra de linha
                    int yPos = descArea.y + 10;
                    int maxWidth = descArea.width - 20;
                    int len = strlen(itemDesc);
                    int startChar = 0;

                    for (int i = 0; i < len; i++) {
                        if (itemDesc[i] == ' ' || i == len - 1) {
                            if (MeasureText(itemDesc + startChar, i - startChar + 1) > maxWidth) {
                                DrawText(itemDesc + startChar, descArea.x + 10, yPos, 18, BLACK);
                                yPos += 22;
                                startChar = i + 1;
                            }
                        }
                    }

                    // Desenhar o restante do texto
                    if (startChar < len) {
                        DrawText(itemDesc + startChar, descArea.x + 10, yPos, 18, BLACK);
                    }

                    // Botões de confirmação
                    Rectangle useBtn = { itemBox.x + 140, itemBox.y + 130, 120, 20 };
                    Rectangle cancelBtn = { itemBox.x + itemBox.width - 160, itemBox.y + 130, 120, 20 };

                    if (drawButton(useBtn, "USAR", (Color){ 120, 200, 80, 255 })) {
                        PlaySound(selectSound);

                        // Enfileirar ação de usar item
                        enqueue(battleSystem->actionQueue, 2, battleSystem->itemType, battleSystem->playerTeam->current);

                        // Passar o turno para o bot
                        battleSystem->playerTurn = false;
                        battleSystem->battleState = BATTLE_SELECT_ACTION;
                    }

                    if (drawButton(cancelBtn, "CANCELAR", (Color){ 200, 100, 100, 255 })) {
                        PlaySound(selectSound);
                        battleSystem->battleState = BATTLE_SELECT_ACTION;
                    }
                }
                break;

            case BATTLE_SELECT_MONSTER:
                // Mostrar lista de monstros para troca estilo Pokémon
                if (battleSystem->playerTurn) {
                    // Caixa principal para seleção de monstros
                    Rectangle monsterBox = { 20, GetScreenHeight() - 200, GetScreenWidth() - 40, 180 };
                    DrawRectangleRounded(monsterBox, 0.2f, 10, (Color){ 240, 240, 240, 250 });
                    DrawRectangleRoundedLines(monsterBox, 0.2f, 10, BLACK);

                    // Título
                    DrawText("TROCAR MONSTRO", monsterBox.x + 15, monsterBox.y + 10, 24, BLACK);

                    // Divisória após o título
                    DrawLine(monsterBox.x, monsterBox.y + 40,
                             monsterBox.x + monsterBox.width, monsterBox.y + 40,
                             (Color){ 200, 200, 200, 255 });

                    // Desenhar slots para os monstros
                    int monsterCount = 0;
                    PokeMonster* current = battleSystem->playerTeam->first;

                    // Calcular dimensões para os slots
                    int slotWidth = (monsterBox.width - 40) / 3;  // 3 slots por linha
                    int slotHeight = 60;
                    int slotSpacing = 10;

                    while (current != NULL) {
                        int row = monsterCount / 3;
                        int col = monsterCount % 3;

                        Rectangle slotRect = {
                            monsterBox.x + 10 + col * (slotWidth + slotSpacing),
                            monsterBox.y + 50 + row * (slotHeight + slotSpacing),
                            slotWidth,
                            slotHeight
                        };

                        // Cor de fundo do slot baseada no estado do monstro
                        Color slotColor;

                        if (current == battleSystem->playerTeam->current) {
                            // Monstro atual (destacado)
                            slotColor = (Color){ 200, 230, 255, 255 };
                        } else if (isMonsterFainted(current)) {
                            // Monstro desmaiado
                            slotColor = (Color){ 230, 200, 200, 255 };
                        } else {
                            // Monstro disponível
                            slotColor = (Color){ 230, 255, 230, 255 };
                        }

                        // Desenhar o slot
                        DrawRectangleRounded(slotRect, 0.2f, 10, slotColor);
                        DrawRectangleRoundedLines(slotRect, 0.2f, 10, BLACK);

                        // Nome do monstro
                        DrawText(current->name, slotRect.x + 10, slotRect.y + 10, 20, BLACK);

                        // Ícone de tipo
                        Rectangle typeIconRect = { slotRect.x + 10, slotRect.y + 35, 40, 15 };
                        DrawRectangleRounded(typeIconRect, 0.3f, 5, getTypeColor(current->type1));
                        DrawText(getTypeName(current->type1),
                                typeIconRect.x + 5,
                                typeIconRect.y + 1,
                                12, WHITE);

                        // Barra de HP
                        Rectangle hpBarRect = { slotRect.x + 60, slotRect.y + 35, slotWidth - 70, 15 };
                        drawHealthBar(hpBarRect, current->hp, current->maxHp, current);

                        // Status (se houver)
                        if (current->statusCondition > STATUS_SPD_DOWN) {
                            char statusText[4] = "";
                            switch (current->statusCondition) {
                                case STATUS_PARALYZED: strcpy(statusText, "PAR"); break;
                                case STATUS_SLEEPING: strcpy(statusText, "SLP"); break;
                                case STATUS_BURNING: strcpy(statusText, "BRN"); break;
                                default: break;
                            }

                            Rectangle statusRect = { slotRect.x + slotWidth - 50, slotRect.y + 10, 40, 20 };
                            Color statusColor;

                            switch (current->statusCondition) {
                                case STATUS_PARALYZED: statusColor = YELLOW; break;
                                case STATUS_SLEEPING: statusColor = DARKPURPLE; break;
                                case STATUS_BURNING: statusColor = RED; break;
                                default: statusColor = GRAY; break;
                            }

                            DrawRectangleRounded(statusRect, 0.5f, 5, statusColor);
                            DrawText(statusText,
                                    statusRect.x + 5,
                                    statusRect.y + 2,
                                    16, WHITE);
                        }

                        // Verificar interação
                        if (CheckCollisionPointRec(GetMousePosition(), slotRect)) {
                            // Destacar slot ao passar o mouse
                            DrawRectangleRoundedLines(slotRect, 0.2f, 10, (Color){ 0, 120, 255, 255 });

                            // Verificar clique
                            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                                if (!isMonsterFainted(current) && current != battleSystem->playerTeam->current) {
                                   PlaySound(selectSound);

                                   // Enfileirar ação de troca
                                   enqueue(battleSystem->actionQueue, 1, monsterCount, battleSystem->playerTeam->current);

                                   // Passar o turno para o bot
                                   battleSystem->playerTurn = false;
                                   battleSystem->battleState = BATTLE_SELECT_ACTION;
                               }
                           }
                       }

                       monsterCount++;
                       current = current->next;
                   }

                   // Botão de voltar
                   Rectangle backBtn = { monsterBox.x + monsterBox.width - 100, monsterBox.y + 10, 80, 25 };
                   if (drawButton(backBtn, "VOLTAR", DARKGRAY)) {
                       PlaySound(selectSound);
                       battleSystem->battleState = BATTLE_SELECT_ACTION;
                   }
               }
               break;

           case BATTLE_FORCED_SWITCH:
               // Mostrar lista de monstros para troca forçada estilo Pokémon
               if (battleSystem->playerTurn) {
                   // Mensagem de ação obrigatória
                   Rectangle msgBox = { 20, GetScreenHeight() - 240, GetScreenWidth() - 40, 40 };
                   DrawRectangleRounded(msgBox, 0.2f, 10, (Color){ 255, 200, 200, 250 });
                   DrawRectangleRoundedLines(msgBox, 0.2f, 10, BLACK);
                   DrawText("Seu monstro desmaiou! Escolha outro para continuar.",
                           msgBox.x + 20, msgBox.y + 10, 20, MAROON);

                   // Caixa principal para seleção de monstros
                   Rectangle monsterBox = { 20, GetScreenHeight() - 190, GetScreenWidth() - 40, 170 };
                   DrawRectangleRounded(monsterBox, 0.2f, 10, (Color){ 240, 240, 240, 250 });
                   DrawRectangleRoundedLines(monsterBox, 0.2f, 10, BLACK);

                   // Título
                   DrawText("MONSTROS DISPONÍVEIS", monsterBox.x + 15, monsterBox.y + 10, 24, BLACK);

                   // Divisória após o título
                   DrawLine(monsterBox.x, monsterBox.y + 40,
                            monsterBox.x + monsterBox.width, monsterBox.y + 40,
                            (Color){ 200, 200, 200, 255 });

                   // Desenhar slots para os monstros disponíveis
                   int monsterCount = 0;
                   PokeMonster* current = battleSystem->playerTeam->first;

                   // Calcular dimensões para os slots
                   int slotWidth = (monsterBox.width - 40) / 3;  // 3 slots por linha
                   int slotHeight = 110;
                   int slotSpacing = 10;
                   int activeMonstersCount = 0;

                   // Primeiro, contar monstros não desmaiados
                   PokeMonster* tempCurrent = battleSystem->playerTeam->first;
                   while (tempCurrent != NULL) {
                       if (!isMonsterFainted(tempCurrent)) activeMonstersCount++;
                       tempCurrent = tempCurrent->next;
                   }

                   // Se houver apenas um monstro, centralizar
                   int startCol = (activeMonstersCount < 3) ? (3 - activeMonstersCount) / 2 : 0;

                   while (current != NULL) {
                       if (!isMonsterFainted(current)) {
                           int col = startCol + monsterCount % 3;

                           Rectangle slotRect = {
                               monsterBox.x + 10 + col * (slotWidth + slotSpacing),
                               monsterBox.y + 50,
                               slotWidth,
                               slotHeight
                           };

                           // Slot destacado
                           Color slotColor = (Color){ 230, 255, 230, 255 };

                           // Desenhar o slot
                           DrawRectangleRounded(slotRect, 0.2f, 10, slotColor);
                           DrawRectangleRoundedLines(slotRect, 0.2f, 10, BLACK);

                           // Mini prévia do sprite
                           Rectangle spriteRect = { slotRect.x + slotWidth/2 - 25, slotRect.y + 10, 50, 50 };

                           if (current->texture.id != 0) {
                               // Usar a textura
                               float scale = fmin(
                                   spriteRect.width / current->texture.width,
                                   spriteRect.height / current->texture.height
                               );

                               DrawTextureEx(
                                   current->texture,
                                   (Vector2){
                                       spriteRect.x + (spriteRect.width - current->texture.width * scale) / 2,
                                       spriteRect.y + (spriteRect.height - current->texture.height * scale) / 2
                                   },
                                   0.0f,
                                   scale,
                                   WHITE
                               );
                           } else {
                               // Desenhar silhueta
                               Color monsterColor = getTypeColor(current->type1);
                               monsterColor.a = 200;
                               DrawRectangleRounded(spriteRect, 0.3f, 10, monsterColor);
                           }

                           // Nome do monstro
                           DrawText(current->name,
                                   slotRect.x + slotWidth/2 - MeasureText(current->name, 18)/2,
                                   slotRect.y + 65,
                                   18, BLACK);

                           // Barra de HP
                           Rectangle hpBarRect = { slotRect.x + 10, slotRect.y + 85, slotWidth - 20, 10 };
                           drawHealthBar(hpBarRect, current->hp, current->maxHp, current);

                           // Verificar interação
                           if (CheckCollisionPointRec(GetMousePosition(), slotRect)) {
                               // Destacar slot ao passar o mouse
                               DrawRectangleRoundedLines(slotRect, 0.2f, 10, (Color){ 0, 120, 255, 255 });

                               // Verificar clique
                               if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                                   PlaySound(selectSound);

                                   // Trocar para o monstro escolhido
                                   switchMonster(battleSystem->playerTeam, current);

                                   // Voltar ao estado de execução de ações
                                   if (!isQueueEmpty(battleSystem->actionQueue)) {
                                       battleSystem->battleState = BATTLE_EXECUTING_ACTIONS;
                                   } else {
                                       // Se não, volta para seleção de ação
                                       battleSystem->battleState = BATTLE_SELECT_ACTION;
                                   }
                               }
                           }

                           monsterCount++;
                       }
                       current = current->next;
                   }
               }
               break;

           case BATTLE_CONFIRM_QUIT:
               // Confirmação para desistir/fugir da batalha
               {
                   // Fundo semi-transparente para destacar o diálogo
                   DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){ 0, 0, 0, 150 });

                   // Caixa de diálogo central
                   Rectangle dialogBox = { GetScreenWidth()/2 - 200, GetScreenHeight()/2 - 100, 400, 200 };
                   DrawRectangleRounded(dialogBox, 0.2f, 10, (Color){ 240, 240, 240, 250 });
                   DrawRectangleRoundedLines(dialogBox, 0.2f, 10, BLACK);

                   // Título da caixa de diálogo
                   DrawText("DESISTIR DA BATALHA",
                           dialogBox.x + 200 - MeasureText("DESISTIR DA BATALHA", 24)/2,
                           dialogBox.y + 30,
                           24, (Color){ 200, 80, 80, 255 });

                   // Mensagem de confirmação
                   const char* confirmMsg = "Tem certeza que deseja fugir?";
                   DrawText(confirmMsg,
                           dialogBox.x + 200 - MeasureText(confirmMsg, 20)/2,
                           dialogBox.y + 70,
                           20, BLACK);

                   const char* subMsg = "Você perderá esta batalha.";
                   DrawText(subMsg,
                           dialogBox.x + 200 - MeasureText(subMsg, 18)/2,
                           dialogBox.y + 100,
                           18, DARKGRAY);

                   // Botões de SIM e NÃO
                   Rectangle yesBtn = { dialogBox.x + 50, dialogBox.y + 140, 100, 40 };
                   Rectangle noBtn = { dialogBox.x + 250, dialogBox.y + 140, 100, 40 };

                   if (drawButton(yesBtn, "SIM", (Color){ 200, 80, 80, 255 })) {
                       PlaySound(selectSound);
                       // Terminar a batalha e voltar ao menu principal
                       StopMusicStream(battleMusic);
                       PlayMusicStream(menuMusic);
                       currentScreen = MAIN_MENU;
                       resetBattle();
                   }

                   if (drawButton(noBtn, "NÃO", (Color){ 80, 180, 80, 255 })) {
                       PlaySound(selectSound);
                       // Continuar a batalha
                       battleSystem->battleState = BATTLE_SELECT_ACTION;
                   }
               }
               break;

           case BATTLE_OVER:
               // Mostrar resultado da batalha estilo Pokémon
               {
                   // Caixa de resultado
                   Rectangle resultBox = { GetScreenWidth()/2 - 200, GetScreenHeight()/2 - 100, 400, 200 };
                   DrawRectangleRounded(resultBox, 0.2f, 10, (Color){ 240, 240, 240, 250 });
                   DrawRectangleRoundedLines(resultBox, 0.2f, 10, BLACK);

                   int winner = getBattleWinner();

                   // Título
                   const char* resultTitle;
                   Color titleColor;

                   if (winner == 1) {
                       resultTitle = "VITÓRIA!";
                       titleColor = (Color){ 0, 150, 50, 255 };
                   } else if (winner == 2) {
                       resultTitle = "DERROTA!";
                       titleColor = (Color){ 200, 30, 30, 255 };
                   } else {
                       resultTitle = "EMPATE!";
                       titleColor = (Color){ 50, 100, 200, 255 };
                   }

                   int titleWidth = MeasureText(resultTitle, 40);
                   DrawText(resultTitle,
                           resultBox.x + resultBox.width/2 - titleWidth/2,
                           resultBox.y + 30,
                           40, titleColor);

                   // Descrição
                   const char* resultDesc;

                   if (winner == 1) {
                       resultDesc = "Todos os monstros do oponente desmaiaram!";
                   } else if (winner == 2) {
                       resultDesc = "Todos os seus monstros desmaiaram!";
                   } else {
                       resultDesc = "A batalha terminou em empate!";
                   }

                   int descWidth = MeasureText(resultDesc, 20);
                   DrawText(resultDesc,
                           resultBox.x + resultBox.width/2 - descWidth/2,
                           resultBox.y + 90,
                           20, BLACK);

                   // Desenhar prêmio (simulado) se venceu
                   if (winner == 1) {
                       DrawText("Ganhou 500 PokeDólares!",
                               resultBox.x + resultBox.width/2 - MeasureText("Ganhou 500 PokeDólares!", 18)/2,
                               resultBox.y + 120,
                               18, (Color){ 230, 180, 30, 255 });
                   }

                   // Botão para voltar ao menu
                   Rectangle menuBtn = { resultBox.x + resultBox.width/2 - 100, resultBox.y + 150, 200, 30 };
                   if (drawButton(menuBtn, "VOLTAR AO MENU", (Color){ 100, 150, 240, 255 })) {
                       PlaySound(selectSound);
                       StopMusicStream(battleMusic);
                       PlayMusicStream(menuMusic);
                       currentScreen = MAIN_MENU;
                       resetBattle();
                   }
               }
               break;

           default:
               break;
       }
   }

   // Sempre desenhar efeitos visuais de batalha
   drawBattleEffects();

   // Indicador de IA
   drawAIIndicator();
}

// Função para atualizar a lógica da batalha
void updateBattleScreen(void) {
    if (battleSystem != NULL) {
        // Atualizar música da batalha
        UpdateMusicStream(battleMusic);

        // Atualizar a lógica da batalha
        updateBattle();

        // Atualizar efeitos visuais
        updateBattleEffects();

        // Sons de batalha baseados no estado
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

// Desenha um monstro na tela de batalha
void drawMonsterInBattle(PokeMonster* monster, Rectangle bounds, bool isPlayer) {
    if (monster == NULL) return;
    
    // Verificar se a textura foi carregada
    if (monster->texture.id == 0) {
        // Desenhar apenas uma silhueta se não houver textura
        Color monsterColor = getTypeColor(monster->type1);
        monsterColor.a = 200; // Semitransparente
        
        if (isPlayer) {
            // Silhueta traseira para o jogador
            DrawRectangleRounded((Rectangle){ bounds.x + 30, bounds.y + 20, bounds.width - 60, bounds.height - 40 }, 0.5f, 10, monsterColor);
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
    
    // Aplicar efeito de sombra (oval abaixo do monstro)
    DrawEllipse(
        x + width/2, 
        y + height - 10, 
        width/2, 
        20, 
        (Color){ 0, 0, 0, 100 }
    );
    
    // Se for o monstro do jogador, aplicar efeito espelhado
    if (isPlayer) {
        // Desenhar com efeito espelhado (virado de costas)
        DrawTexturePro(
            monster->texture,
            (Rectangle){ 0, 0, -monster->texture.width, monster->texture.height }, // Inverter a textura horizontalmente
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

void drawBattleHUD(void) {
    // Implementação básica
    if (battleSystem == NULL) return;
    
    // Área para HUD
    Rectangle hudArea = { 0, GetScreenHeight() - 150, GetScreenWidth(), 150 };
    DrawRectangleRec(hudArea, RAYWHITE);
    DrawRectangleLines(hudArea.x, hudArea.y, hudArea.width, hudArea.height, BLACK);
    
    // Exibir informações básicas
    char turnText[32];
    sprintf(turnText, "Turno: %d", battleSystem->turn);
    DrawText(turnText, hudArea.x + 20, hudArea.y + 20, 20, BLACK);
}

// Inicializar sistema de efeitos
void initBattleEffects(void) {
    for (int i = 0; i < MAX_EFFECTS; i++) {
        effects[i].active = false;
    }
}

// Atualizar efeitos de batalha
void updateBattleEffects(void) {
    for (int i = 0; i < MAX_EFFECTS; i++) {
        if (effects[i].active) {
            effects[i].timer += GetFrameTime();
            
            // Verificar se o efeito terminou
            if (effects[i].timer >= effects[i].duration) {
                effects[i].active = false;
            }
        }
    }
}

// Desenhar efeitos de batalha
void drawBattleEffects(void) {
    for (int i = 0; i < MAX_EFFECTS; i++) {
        if (!effects[i].active) continue;
        
        // Normalizar o timer (0.0 - 1.0)
        float progress = effects[i].timer / effects[i].duration;
        
        switch (effects[i].type) {
            case EFFECT_FLASH: {
                // Efeito de flash (pisca a tela com a cor)
                float alpha = (1.0f - progress) * 0.4f; // Reduzir de 0.6f para 0.4f
                DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), 
                             (Color){effects[i].color.r, effects[i].color.g, effects[i].color.b, 
                                    (unsigned char)(effects[i].color.a * alpha)}); // Multiplicar pelo alpha da cor original
            } break;
                
            case EFFECT_SHAKE: {
                // Efeito de chacoalho (não implementado diretamente, afeta a câmera)
            } break;
                
            case EFFECT_PARTICLES: {
                // Efeito de partículas
                int particleCount = 20;
                for (int j = 0; j < particleCount; j++) {
                    float angle = j * (2.0f * PI / particleCount);
                    float distance = progress * 100.0f;
                    float x = effects[i].origin.x + cosf(angle) * distance;
                    float y = effects[i].origin.y + sinf(angle) * distance - progress * progress * 50.0f;
                    float size = (1.0f - progress) * 10.0f;
                    
                    DrawCircle(x, y, size, effects[i].color);
                }
            } break;
                
            case EFFECT_SLASH: {
                // Efeito de corte
                float thickness = (1.0f - progress) * 5.0f;
                for (int j = 0; j < 3; j++) {
                    float offset = j * 10.0f * progress;
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
            } break;
                
            case EFFECT_FIRE: {
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
            } break;
                
            case EFFECT_WATER: {
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
            } break;
                
            case EFFECT_ELECTRIC: {
                // Efeito elétrico
                for (int j = 0; j < 5; j++) {
                    float offsetX = GetRandomValue(-30, 30);
                    float offsetY = GetRandomValue(-30, 30);
                    Vector2 start = {
                        effects[i].target.x + offsetX,
                        effects[i].target.y + offsetY
                    };
                    
                    // Desenhar raios (ziguezague)
                    for (int k = 0; k < 5; k++) {
                        float progress2 = progress + k * 0.05f;
                        if (progress2 > 1.0f) progress2 = 1.0f;
                        
                        Vector2 end = {
                            start.x + GetRandomValue(-20, 20),
                            start.y + GetRandomValue(-20, 20) - 10.0f
                        };
                        
                        DrawLineEx(start, end, 3.0f * (1.0f - progress), YELLOW);
                        start = end;
                    }
                }
            } break;
                
            case EFFECT_NATURE: {
                // Efeito de natureza
                for (int j = 0; j < 10; j++) {
                    float angle = j * (2.0f * PI / 10) + GetRandomValue(0, 100) / 100.0f;
                    float dist = progress * 60.0f;
                    float x = effects[i].target.x + cosf(angle) * dist;
                    float y = effects[i].target.y + sinf(angle) * dist;
                    
                    // Desenhar pequenas folhas
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
            } break;
        }
    }
}

// Criar efeito baseado no tipo do ataque
void createAttackEffect(MonsterType attackType, Vector2 origin, Vector2 target) {
    switch (attackType) {
        case TYPE_FIRE:
            createBattleEffect(EFFECT_FIRE, (Rectangle){0, 0, 0, 0}, RED, origin, target, 0.5f);
            createBattleEffect(EFFECT_FLASH, (Rectangle){0, 0, 0, 0}, (Color){255, 100, 0, 255}, origin, target, 0.2f);
            break;
            
        case TYPE_WATER:
            createBattleEffect(EFFECT_WATER, (Rectangle){0, 0, 0, 0}, BLUE, origin, target, 0.6f);
            createBattleEffect(EFFECT_FLASH, (Rectangle){0, 0, 0, 0}, (Color){0, 100, 255, 255}, origin, target, 0.2f);
            break;
            
        case TYPE_ELECTRIC:
            createBattleEffect(EFFECT_ELECTRIC, (Rectangle){0, 0, 0, 0}, YELLOW, origin, target, 0.5f);
            createBattleEffect(EFFECT_FLASH, (Rectangle){0, 0, 0, 0}, (Color){255, 255, 0, 255}, origin, target, 0.2f);
            break;
            
        case TYPE_GRASS:
            createBattleEffect(EFFECT_NATURE, (Rectangle){0, 0, 0, 0}, GREEN, origin, target, 0.7f);
            createBattleEffect(EFFECT_FLASH, (Rectangle){0, 0, 0, 0}, (Color){50, 200, 50, 255}, origin, target, 0.2f);
            break;
            
        case TYPE_METAL:
        case TYPE_FLYING:
            createBattleEffect(EFFECT_SLASH, (Rectangle){0, 0, 0, 0}, WHITE, origin, target, 0.4f);
            createBattleEffect(EFFECT_SHAKE, (Rectangle){0, 0, 0, 0}, WHITE, origin, target, 0.3f);
            break;
            
        case TYPE_DRAGON:
            // Reduzir a intensidade e duração do flash roxo
            createBattleEffect(EFFECT_FLASH, (Rectangle){0, 0, 0, 0}, 
                              (Color){100, 50, 200, 100}, // Reduzir opacidade para 100 (era 255)
                              origin, target, 0.15f);     // Reduzir duração de 0.3f para 0.15f
            
            createBattleEffect(EFFECT_PARTICLES, (Rectangle){0, 0, 0, 0}, 
                              (Color){150, 80, 220, 255}, 
                              origin, target, 0.5f);
            break;
            
        case TYPE_GHOST:
            createBattleEffect(EFFECT_FLASH, (Rectangle){0, 0, 0, 0}, (Color){100, 50, 150, 255}, origin, target, 0.4f);
            createBattleEffect(EFFECT_PARTICLES, (Rectangle){0, 0, 0, 0}, (Color){120, 60, 170, 255}, origin, target, 0.6f);
            break;
            
        case TYPE_FAIRY:
            createBattleEffect(EFFECT_PARTICLES, (Rectangle){0, 0, 0, 0}, (Color){255, 180, 200, 255}, origin, target, 0.7f);
            createBattleEffect(EFFECT_FLASH, (Rectangle){0, 0, 0, 0}, (Color){255, 150, 180, 255}, origin, target, 0.3f);
            break;
            
        default:
            createBattleEffect(EFFECT_PARTICLES, (Rectangle){0, 0, 0, 0}, WHITE, origin, target, 0.5f);
            break;
    }
}

// Modificar a função executeAttack para criar efeitos visuais
void executeAttackWithEffects(PokeMonster* attacker, PokeMonster* defender, int attackIndex) {
    // Chamamos a função original primeiro
    executeAttack(attacker, defender, attackIndex);
    
    // Posições para o efeito (baseadas no desenho dos monstros na tela)
    Vector2 attackerPos, defenderPos;
    
    if (attacker == battleSystem->playerTeam->current) {
        // Jogador atacando
        attackerPos = (Vector2){ 
            GetScreenWidth() - GetScreenWidth()/4, 
            GetScreenHeight()/2 + 40
        };
        
        defenderPos = (Vector2){ 
            GetScreenWidth()/4, 
            GetScreenHeight()/2 - 40
        };
    } else {
        // Oponente atacando
        attackerPos = (Vector2){ 
            GetScreenWidth()/4, 
            GetScreenHeight()/2 - 40
        };
        
        defenderPos = (Vector2){ 
            GetScreenWidth() - GetScreenWidth()/4, 
            GetScreenHeight()/2 + 40
        };
    }
    
    // Criar o efeito baseado no tipo do ataque
    createAttackEffect(attacker->attacks[attackIndex].type, attackerPos, defenderPos);
}

// Criar um novo efeito de batalha
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