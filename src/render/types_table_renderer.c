#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#endif

#include "types_table_renderer.h"
#include "raylib.h"
#include "monsters.h"
#include "globals.h"
#include "gui.h"
#include <math.h>

// Variáveis de animação
static float tableTimer = 0.0f;
static float rowHighlight = -1.0f;
static float colHighlight = -1.0f;
static Vector2 mouseCell = {-1, -1};
static int animationState = 0; // 0 = intro, 1 = normal
static float introTimer = 0.0f;
static float bgScroll = 0.0f;

// Função para desenhar a tabela de tipos
void drawTypesTable(void) {
    // Atualizar temporizadores
    tableTimer += GetFrameTime();
    bgScroll -= GetFrameTime() * 15.0f;
    if (bgScroll < -20.0f) bgScroll += 20.0f;

    // Animação de introdução
    if (animationState == 0) {
        introTimer += GetFrameTime();
        if (introTimer >= 2.0f) {
            animationState = 1;
        }
    }

    // Desenhar fundo
    ClearBackground((Color){16, 32, 64, 255});

    // Desenhar padrão de listras horizontais
    for (int i = 0; i < GetScreenHeight(); i += 20) {
        int yPos = i + (int)bgScroll;
        if (yPos < 0) yPos += 20;
        DrawRectangle(0, yPos, GetScreenWidth(), 2, (Color){255, 255, 255, 10});
    }

    // Título com animação
    const char* title = "TABELA DE TIPOS";
    float titleScale = animationState == 0 ?
                      0.5f + introTimer * 0.5f / 1.0f :
                      1.0f + sinf(tableTimer * 1.5f) * 0.05f;

    if (titleScale > 1.0f) titleScale = 1.0f;

    int titleFontSize = (int)(40 * titleScale);
    int titleAlpha = animationState == 0 ?
                    (int)(255 * (introTimer / 1.0f)) :
                    255;

    // Sombra do título
    DrawText(title,
            GetScreenWidth()/2 - MeasureText(title, titleFontSize)/2 + 3,
            30 + 3,
            titleFontSize,
            (Color){0, 0, 0, titleAlpha / 2});

    // Título
    DrawText(title,
            GetScreenWidth()/2 - MeasureText(title, titleFontSize)/2,
            30,
            titleFontSize,
            (Color){255, 255, 255, titleAlpha});

    // Determinar tamanho e posição da tabela
    int cellSize = 32;
    int headerSize = 80;
    int tableWidth = headerSize + cellSize * TYPE_COUNT;
    int tableHeight = headerSize + cellSize * TYPE_COUNT;

    int startX = GetScreenWidth()/2 - tableWidth/2;
    int startY = 100;

    // Desenhar tabela com animação de entrada
    float tableAlpha = animationState == 0 ?
                      introTimer / 2.0f * 255 :
                      255;

    if (tableAlpha > 255) tableAlpha = 255;

    // Desenhar fundo da tabela
    DrawRectangle(startX - 10, startY - 10,
                  tableWidth + 20, tableHeight + 20,
                  (Color){30, 60, 90, (unsigned char)tableAlpha});

    DrawRectangleLines(startX - 10, startY - 10,
                      tableWidth + 20, tableHeight + 20,
                      (Color){100, 150, 200, (unsigned char)tableAlpha});

    // Obter célula sob o mouse
    Vector2 mouse = GetMousePosition();
    int mouseRow = -1;
    int mouseCol = -1;

    if (mouse.x >= startX + headerSize && mouse.x < startX + tableWidth &&
        mouse.y >= startY + headerSize && mouse.y < startY + tableHeight) {
        mouseCol = (int)((mouse.x - (startX + headerSize)) / cellSize);
        mouseRow = (int)((mouse.y - (startY + headerSize)) / cellSize);

        if (mouseCol >= 0 && mouseCol < TYPE_COUNT &&
            mouseRow >= 0 && mouseRow < TYPE_COUNT) {
            mouseCell = (Vector2){mouseCol, mouseRow};
            rowHighlight = mouseRow;
            colHighlight = mouseCol;
        }
    } else {
        mouseCell = (Vector2){-1, -1};

        // Resetar highlights gradualmente
        if (rowHighlight >= 0) {
            rowHighlight += ((-1.0f) - rowHighlight) * GetFrameTime() * 10.0f;
            if (fabsf(rowHighlight - (-1.0f)) < 0.1f) {
                rowHighlight = -1.0f;
            }
        }

        if (colHighlight >= 0) {
            colHighlight += ((-1.0f) - colHighlight) * GetFrameTime() * 10.0f;
            if (fabsf(colHighlight - (-1.0f)) < 0.1f) {
                colHighlight = -1.0f;
            }
        }
    }

    // Desenhar headers com ícones de tipo
    for (int i = 0; i < TYPE_COUNT; i++) {
        // Header horizontal (tipos atacantes)
        Rectangle headerRect = {
            startX + headerSize + i * cellSize,
            startY,
            cellSize,
            headerSize
        };

        // Highlight da coluna
        if ((int)colHighlight == i || fabsf(colHighlight - i) < 1.0f) {
            float highlightAlpha = colHighlight == i ? 0.3f : 0.3f * (1.0f - fabsf(colHighlight - i));
            DrawRectangle(headerRect.x, startY,
                         cellSize, tableHeight,
                         (Color){255, 255, 255, (unsigned char)(highlightAlpha * 255)});
        }

        // Fundo do header
        Color typeColor = getTypeColor(i);
        typeColor.a = (unsigned char)tableAlpha;
        DrawRectangle(headerRect.x, headerRect.y, headerRect.width, headerRect.height, typeColor);

        // Rotacionar texto de tipo
        Vector2 textPos = {
            headerRect.x + headerRect.width/2,
            headerRect.y + headerRect.height - 10
        };

        // Nome do tipo em posição vertical
        DrawText(getTypeName(i),
                textPos.x - MeasureText(getTypeName(i), 12)/2,
                headerRect.y + 5,
                12,
                WHITE);

        // Header vertical (tipos defensores)
        Rectangle defHeaderRect = {
            startX,
            startY + headerSize + i * cellSize,
            headerSize,
            cellSize
        };

        // Highlight da linha
        if ((int)rowHighlight == i || fabsf(rowHighlight - i) < 1.0f) {
            float highlightAlpha = rowHighlight == i ? 0.3f : 0.3f * (1.0f - fabsf(rowHighlight - i));
            DrawRectangle(startX, defHeaderRect.y,
                         tableWidth, cellSize,
                         (Color){255, 255, 255, (unsigned char)(highlightAlpha * 255)});
        }

        // Fundo do header
        DrawRectangle(defHeaderRect.x, defHeaderRect.y,
                     defHeaderRect.width, defHeaderRect.height,
                     typeColor);

        // Nome do tipo horizontal
        DrawText(getTypeName(i),
                defHeaderRect.x + 5,
                defHeaderRect.y + defHeaderRect.height/2 - 8,
                16,
                WHITE);
    }

    // Desenhar células de efetividade
    for (int row = 0; row < TYPE_COUNT; row++) {
        for (int col = 0; col < TYPE_COUNT; col++) {
            Rectangle cellRect = {
                startX + headerSize + col * cellSize,
                startY + headerSize + row * cellSize,
                cellSize,
                cellSize
            };

            // Obter valor de efetividade
            float effectiveness = typeEffectiveness[col][row];

            // Escolher cor com base na efetividade
            Color cellColor;
            if (effectiveness > 1.5f) {
                cellColor = (Color){100, 200, 100, (unsigned char)tableAlpha}; // Super efetivo
            } else if (effectiveness < 0.5f && effectiveness > 0.0f) {
                cellColor = (Color){200, 100, 100, (unsigned char)tableAlpha}; // Não muito efetivo
            } else if (effectiveness == 0.0f) {
                cellColor = (Color){60, 60, 60, (unsigned char)tableAlpha}; // Sem efeito
            } else {
                cellColor = (Color){100, 100, 100, (unsigned char)tableAlpha}; // Normal
            }

            // Desenhar célula com animação de pulso para a célula sob o mouse
            float cellScale = 1.0f;

            if (mouseCell.x == col && mouseCell.y == row) {
                cellScale = 1.0f + sinf(tableTimer * 5.0f) * 0.1f;

                // Também desenhar um retângulo destacado
                DrawRectangle(cellRect.x, cellRect.y,
                             cellRect.width, cellRect.height,
                             (Color){255, 255, 255, 100});
            }

            // Aplicar escala ao desenhar
            float scaleDiff = (cellScale - 1.0f) * cellSize;
            Rectangle scaledRect = {
                cellRect.x - scaleDiff / 2,
                cellRect.y - scaleDiff / 2,
                cellRect.width + scaleDiff,
                cellRect.height + scaleDiff
            };

            DrawRectangleRec(scaledRect, cellColor);
            DrawRectangleLinesEx(scaledRect, 1, (Color){0, 0, 0, (unsigned char)tableAlpha});

            // Formatação do texto para mostrar multiplicador
            char effText[8];
            if (effectiveness == 0.0f) {
                strcpy(effText, "0");
            } else if (effectiveness == 0.25f) {
                strcpy(effText, "¼");
            } else if (effectiveness == 0.5f) {
                strcpy(effText, "½");
            } else {
                sprintf(effText, "%.1f", effectiveness);
                // Remover o .0 para efetividade inteira
                if (effectiveness == 1.0f || effectiveness == 2.0f) {
                    effText[1] = '\0';
                }
            }

            // Desenhar texto com tamanho que se ajusta à escala
            float fontSize = 16 * cellScale;
            DrawText(effText,
                    scaledRect.x + scaledRect.width/2 - MeasureText(effText, fontSize)/2,
                    scaledRect.y + scaledRect.height/2 - fontSize/2,
                    fontSize,
                    WHITE);
        }
    }

    // Informações da célula selecionada
    if (mouseCell.x >= 0 && mouseCell.y >= 0) {
        int attackType = mouseCell.x;
        int defenseType = mouseCell.y;
        float effectValue = typeEffectiveness[attackType][defenseType];

        char infoText[100];

        if (effectValue > 1.0f) {
            sprintf(infoText, "%s é SUPER EFETIVO contra %s (%.1fx)",
                   getTypeName(attackType), getTypeName(defenseType), effectValue);
        } else if (effectValue < 1.0f && effectValue > 0.0f) {
            sprintf(infoText, "%s é POUCO EFETIVO contra %s (%.1fx)",
                   getTypeName(attackType), getTypeName(defenseType), effectValue);
        } else if (effectValue == 0.0f) {
            sprintf(infoText, "%s não tem EFEITO contra %s (0x)",
                   getTypeName(attackType), getTypeName(defenseType));
        } else {
            sprintf(infoText, "%s tem efetividade NORMAL contra %s (1x)",
                   getTypeName(attackType), getTypeName(defenseType));
        }

        int infoFontSize = 20;
        DrawRectangle(
            GetScreenWidth()/2 - MeasureText(infoText, infoFontSize)/2 - 10,
            startY + tableHeight + 20 - 5,
            MeasureText(infoText, infoFontSize) + 20,
            30,
            (Color){30, 60, 90, 200}
        );

        DrawText(infoText,
                GetScreenWidth()/2 - MeasureText(infoText, infoFontSize)/2,
                startY + tableHeight + 20,
                infoFontSize,
                WHITE);
    }

    // Desenhar legenda
    int legendY = startY + tableHeight + 60;
    int legendX = startX + 50;
    int legendSpacing = 40;
    int legendSize = 20;

    DrawText("Legenda:", legendX - 40, legendY, 20, WHITE);

    // Super efetivo
    DrawRectangle(legendX, legendY + legendSpacing, legendSize, legendSize,
                 (Color){100, 200, 100, 255});
    DrawText("Super efetivo (>1x)",
            legendX + legendSize + 10,
            legendY + legendSpacing,
            18,
            WHITE);

    // Normal
    DrawRectangle(legendX, legendY + legendSpacing * 2, legendSize, legendSize,
                 (Color){100, 100, 100, 255});
    DrawText("Efetividade normal (1x)",
            legendX + legendSize + 10,
            legendY + legendSpacing * 2,
            18,
            WHITE);

    // Não muito efetivo
    DrawRectangle(legendX, legendY + legendSpacing * 3, legendSize, legendSize,
                 (Color){200, 100, 100, 255});
    DrawText("Pouco efetivo (<1x)",
            legendX + legendSize + 10,
            legendY + legendSpacing * 3,
            18,
            WHITE);

    // Sem efeito
    DrawRectangle(legendX, legendY + legendSpacing * 4, legendSize, legendSize,
                 (Color){60, 60, 60, 255});
    DrawText("Sem efeito (0x)",
            legendX + legendSize + 10,
            legendY + legendSpacing * 4,
            18,
            WHITE);

    // Botão de voltar
    Rectangle backBtn = {20, GetScreenHeight() - 70, 150, 50};

    if (GuiPokemonButton(backBtn, "VOLTAR", true)) {
        PlaySound(selectSound);
        currentScreen = MAIN_MENU;
        animationState = 0; // Resetar animação para próxima vez
        introTimer = 0.0f;
    }
}

void updateTypesTable(void) {
    // Toda a lógica já está dentro da função de desenho
}