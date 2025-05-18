#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#endif

#include "credits_renderer.h"
#include "raylib.h"
#include "globals.h"
#include "gui.h"
#include <math.h>
#include "settings_renderer.h"

static float creditsTimer = 0.0f;
static float creditsScroll = 0.0f;
static float scrollSpeed = 40.0f;
static bool autoScroll = true;
static float starTimer = 0.0f;

// Estrutura para estrela animada
typedef struct
{
    Vector2 position;
    float scale;
    float rotation;
    float speed;
    Color color;
} AnimStar;

#define MAX_STARS 30
static AnimStar stars[MAX_STARS];

typedef struct {
    Texture2D texture;
    Vector2 basePosition;
    float scale;
    bool enableRotation;
    float timeOffset;
} CreditSprite;

#define MAX_CREDIT_SPRITES 10
static CreditSprite creditSprites[MAX_CREDIT_SPRITES];
static int creditSpriteCount = 0;


// Inicializar estrelas
static void initStars(void)
{
    for (int i = 0; i < MAX_STARS; i++)
    {
        stars[i].position = (Vector2){
            (float)GetRandomValue(0, GetScreenWidth()),
            (float)GetRandomValue(0, GetScreenHeight())
        };
        stars[i].scale = GetRandomValue(5, 20) / 10.0f;
        stars[i].rotation = GetRandomValue(0, 360);
        stars[i].speed = GetRandomValue(10, 50) / 100.0f;

        stars[i].color = (Color){
            (unsigned char)GetRandomValue(180, 255),
            (unsigned char)GetRandomValue(180, 255),
            (unsigned char)GetRandomValue(200, 255),
            (unsigned char)GetRandomValue(100, 200)
        };
    }
}

// Desenhar uma estrela
static void drawStar(Vector2 position, float scale, float rotation, Color color)
{
    const int points = 5;
    const float innerRadius = 10 * scale;
    const float outerRadius = 20 * scale;
    const float angle = 2.0f * PI / points;

    Vector2 center = position;

    for (int i = 0; i < points * 2; i++)
    {
        float radius = i % 2 == 0 ? outerRadius : innerRadius;
        float a = rotation + i * angle / 2.0f;
        float a2 = rotation + (i + 1) * angle / 2.0f;

        Vector2 p1 = {
            center.x + cosf(a) * radius,
            center.y + sinf(a) * radius
        };

        Vector2 p2 = {
            center.x + cosf(a2) * (i % 2 == 0 ? innerRadius : outerRadius),
            center.y + sinf(a2) * (i % 2 == 0 ? innerRadius : outerRadius)
        };

        DrawTriangle(center, p1, p2, color);
    }
}

void initCreditSprites(void)
{
    creditSprites[0] = (CreditSprite){ LoadTexture("resources/spritesNeto/grilitron.png"), { 150, 300 }, 1.0f, true, 0.0f };
    creditSprites[1] = (CreditSprite){ LoadTexture("resources/spritesNeto/aquariah.png"), { 990, 500 }, 1.0f, false, 0.5f };
    creditSprites[2] = (CreditSprite){ LoadTexture("resources/spritesNeto/gambiarra.png"), { 150, 700 }, 0.9f, true, 1.0f };
    creditSprites[3] = (CreditSprite){ LoadTexture("resources/spritesNeto/mandragoiaba.png"), { 990, 900 }, 1.1f, false, 0.3f };
    creditSprites[4] = (CreditSprite){ LoadTexture("resources/spritesNeto/netomon.png"), { 150, 1100 }, 1.0f, true, 0.7f };
    creditSprites[5] = (CreditSprite){ LoadTexture("resources/spritesNeto/pyromula.png"), { 1000, 1297 }, 1.0f, false, 1.2f };
    creditSprites[6] = (CreditSprite){ LoadTexture("resources/spritesNeto/tatarion.png"), { 120, 1400 }, 1.0f, true, 0.4f };
    creditSprites[7] = (CreditSprite){ LoadTexture("resources/spritesNeto/ventaforte.png"), { 990, 1600 }, 1.0f, true, 0.8f };
    creditSpriteCount = 8;
}

void unloadCreditSprites(void)
{
    for (int i = 0; i < creditSpriteCount; i++) {
        UnloadTexture(creditSprites[i].texture);
    }
    creditSpriteCount = 0;
}

void drawCredits(void)
{
    // Atualizar temporizadores
    float deltaTime = GetFrameTime();
    creditsTimer += deltaTime;
    starTimer += deltaTime;

    // Fundo gradiente
    for (int i = 0; i < GetScreenHeight(); i++)
    {
        float factor = (float)i / GetScreenHeight();
        Color lineColor = (Color){
            (unsigned char)(20 * (1.0f - factor) + 10 * factor),
            (unsigned char)(20 * (1.0f - factor) + 30 * factor),
            (unsigned char)(50 * (1.0f - factor) + 100 * factor),
            255
        };
        DrawRectangle(0, i, GetScreenWidth(), 1, lineColor);
    }

    // Atualizar e desenhar estrelas
    for (int i = 0; i < MAX_STARS; i++)
    {
        // Animar estrelas
        stars[i].rotation += stars[i].speed * deltaTime * 60.0f;

        // Pulsar tamanho
        float scaleModifier = sinf(starTimer * 2.0f + i * 0.1f) * 0.2f + 1.0f;

        // Desenhar estrela
        drawStar(
            stars[i].position,
            stars[i].scale * scaleModifier,
            stars[i].rotation * DEG2RAD,
            stars[i].color
        );
    }

    for (int i = 0; i < creditSpriteCount; i++)
    {
        CreditSprite *sprite = &creditSprites[i];
        float animTime = creditsTimer + sprite->timeOffset;

        float rotation = sprite->enableRotation ? sinf(animTime) * 5.0f : 0.0f;
        float xOffset = sinf(animTime * 1.5f) * 5.0f; // oscilação horizontal opcional

        Vector2 drawPos = {
            sprite->basePosition.x + xOffset,
            sprite->basePosition.y - creditsScroll
        };

        if (drawPos.y + sprite->texture.height * sprite->scale >= 0 &&
            drawPos.y <= GetScreenHeight())
        {
            DrawTextureEx(
                sprite->texture,
                drawPos,
                rotation,
                sprite->scale,
                WHITE);
        }
    }

    // Auto-scroll
    if (autoScroll)
    {
        creditsScroll += scrollSpeed * deltaTime;
    }

    // Calcular layout baseado na resolução
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    int contentWidth = 600;
    int centerX = screenWidth / 2;

    // Calcular altura total do conteúdo (para auto-scroll)
    int totalHeight = 1200; // Altura aproximada de todo conteúdo

    // Limitar scroll
    if (creditsScroll > totalHeight)
    {
        creditsScroll = totalHeight;
        autoScroll = false; // Parar quando chegar ao fim
    }

    if (creditsScroll < 0)
    {
        creditsScroll = 0;
    }

    // Desenhar título principal
    const char* title = "CRÉDITOS";
    float titleScale = 1.0f + sinf(creditsTimer * 1.5f) * 0.05f;
    int titleSize = (int)(50 * titleScale);

    // Sombra do título
    DrawText(title,
             centerX - MeasureText(title, titleSize) / 2 + 3,
             50 + 3,
             titleSize,
             (Color){0, 0, 0, 150});

    // Título
    Color titleColor = (Color){
        (unsigned char)(220 + sinf(creditsTimer * 2.0f) * 35),
        (unsigned char)(220 + sinf(creditsTimer * 2.5f) * 35),
        255,
        255
    };
    DrawText(title,
             centerX - MeasureText(title, titleSize) / 2,
             50,
             titleSize,
             titleColor);

    // Começar a desenhar conteúdo dos créditos
    int startY = 150 - (int)creditsScroll;
    int sectionY = startY;

    // Função para centralizar texto
#define DRAW_CENTERED_TEXT(text, y, size, color) \
        DrawText((text), centerX - MeasureText((text), (size)) / 2, (y), (size), (color))

    // Função para desenhar cabeçalho de seção
#define DRAW_SECTION_HEADER(text, y) \
        do { \
            int size = 30; \
            Color bgColor = (Color){0, 102, 204, 150}; \
            int width = MeasureText((text), size) + 40; \
            DrawRectangle(centerX - width / 2, (y) - 5, width, 40, bgColor); \
            DrawRectangleLinesEx( \
                (Rectangle){centerX - width / 2, (y) - 5, width, 40}, \
                2, \
                WHITE \
            ); \
            DRAW_CENTERED_TEXT((text), (y), size, WHITE); \
            sectionY = (y) + 50; \
        } while(0)

    // Seção 1: Título do Jogo
    DRAW_SECTION_HEADER("PokeBattle", sectionY);

    DRAW_CENTERED_TEXT("Uma batalha de monstros inspirada em Pokémon",
                       sectionY + 10, 20, RAYWHITE);

    // Logo estilizado
    DrawCircle(centerX, sectionY + 90, 40, RED);
    DrawCircle(centerX, sectionY + 90, 35, WHITE);
    DrawRectangle(centerX - 40, sectionY + 87, 80, 6, BLACK);
    DrawCircle(centerX, sectionY + 90, 10, BLACK);
    DrawCircle(centerX, sectionY + 90, 6, WHITE);

    // Descrição
    const char* descText = "Um jogo de batalhas por turnos com monstros elementais";
    DRAW_CENTERED_TEXT(descText, sectionY + 140, 18, RAYWHITE);

    sectionY += 200;

    // Seção 2: Equipe de Desenvolvimento
    DRAW_SECTION_HEADER("Desenvolvido por", sectionY);

    const char* devNames[] = {
        "Julia Torres",
        "Fatima Beatriz",
        "Maria Claudia",
        "Matheus Martins",
        "Vinicius Jose"
    };

    for (int i = 0; i < 5; i++)
    {
        Color nameColor = (Color){
            (unsigned char)(180 + sinf(creditsTimer + i * 0.5f) * 75),
            (unsigned char)(180 + sinf(creditsTimer * 1.2f + i * 0.2f) * 75),
            (unsigned char)(230 + sinf(creditsTimer * 0.8f + i * 0.3f) * 25),
            255
        };
        DRAW_CENTERED_TEXT(devNames[i], sectionY + i * 30, 24, nameColor);
    }

    sectionY += 180;

    // Seção 3: Disciplina
    DRAW_SECTION_HEADER("Projeto da Disciplina", sectionY);

    const char* projectInfo[] = {
        "Algoritmos e Estruturas de Dados",
        "CESAR School - 2025",
        "Módulo 2 - Turma AED2025.1"
    };

    for (int i = 0; i < 3; i++)
    {
        DRAW_CENTERED_TEXT(projectInfo[i], sectionY + i * 30, 20, RAYWHITE);
    }

    sectionY += 120;

    // Seção 4: Tecnologias
    DRAW_SECTION_HEADER("Tecnologias Utilizadas", sectionY);

    const char* techInfo[] = {
        "Linguagem C",
        "Biblioteca gráfica Raylib",
        "Integração com API Gemini AI",
        "RayGUI para interface"
    };

    for (int i = 0; i < 4; i++)
    {
        DRAW_CENTERED_TEXT(techInfo[i], sectionY + i * 30, 20, RAYWHITE);
    }

    sectionY += 150;

    // Seção 5: Estruturas de Dados
    DRAW_SECTION_HEADER("Estruturas de Dados Implementadas", sectionY);

    const char* dataStructures[] = {
        "Lista Duplamente Encadeada para os times",
        "Fila para ordenação de ações",
        "Pilha para efeitos de status",
        "Árvore de Decisão para IA do oponente",
        "Quick Sort para ordenação de velocidade"
    };

    for (int i = 0; i < 5; i++)
    {
        DRAW_CENTERED_TEXT(dataStructures[i], sectionY + i * 30, 18, RAYWHITE);
    }

    sectionY += 180;

    // Seção 6: Agradecimentos
    DRAW_SECTION_HEADER("Agradecimentos", sectionY);

    const char* thanks[] = {
        "Professores e monitores",
        "Colegas de classe",
        "Nintendo e Game Freak (inspiração)",
        "Raylib e comunidade de código aberto"
    };

    for (int i = 0; i < 4; i++)
    {
        DRAW_CENTERED_TEXT(thanks[i], sectionY + i * 30, 20, RAYWHITE);
    }

    sectionY += 150;

    // Seção de versículos
    DRAW_CENTERED_TEXT("Assim, quer comais, quer bebais, quer façais qualquer outra coisa, façam tudo para a glória de Deus.", sectionY, 16, YELLOW);
    DRAW_CENTERED_TEXT("1 Coríntios 10.31", sectionY + 30, 16, YELLOW);
    DRAW_CENTERED_TEXT("Em tudo dai graças, porque esta é a vontade de Deus para vocês em Cristo Jesus.", sectionY + 70, 16, YELLOW);
    DRAW_CENTERED_TEXT("1 Tessalonicenses 5:18", sectionY + 100, 16, YELLOW);
    DRAW_CENTERED_TEXT("Não temas, porque eu sou contigo; não te assombres, porque eu sou teu Deus; eu te fortaleço, e te ajudo, e te sustento com a destra da minha justiça.", sectionY + 140, 16, YELLOW);
    DRAW_CENTERED_TEXT("Isaías 41:10", sectionY + 170, 16, YELLOW);

    // Seção final
    DRAW_CENTERED_TEXT("© 2025 - Todos os direitos reservados", sectionY + 270, 16, LIGHTGRAY);
    DRAW_CENTERED_TEXT("Feito com <3 e muito café", sectionY + 300, 16, PINK);

    // Desenhar barra de scroll na lateral
    if (totalHeight > screenHeight)
    {
        float scrollBarHeight = screenHeight * 0.7f;
        float scrollBarY = screenHeight * 0.15f;

        // Trilho da barra
        DrawRectangle(
            screenWidth - 20,
            (int)scrollBarY,
            10,
            (int)scrollBarHeight,
            (Color){100, 100, 100, 100}
        );

        // Proporção de quanto foi scrollado
        float scrollRatio = creditsScroll / totalHeight;
        float handleHeight = scrollBarHeight * (screenHeight / totalHeight);

        // Manopla da barra
        DrawRectangle(
            screenWidth - 20,
            (int)(scrollBarY + scrollRatio * (scrollBarHeight - handleHeight)),
            10,
            (int)handleHeight,
            (Color){200, 200, 255, 200}
        );
    }

    // Botão de auto-scroll
    Rectangle autoScrollBtn = {20, 20, 180, 40};
    if (GuiPokemonButton(autoScrollBtn, autoScroll ? "Parar Auto-Scroll" : "Ativar Auto-Scroll", true))
    {
        PlaySound(selectSound);
        autoScroll = !autoScroll;
    }

    // Botão de voltar
    Rectangle backBtn = {20, screenHeight - 70, 150, 50};
    if (GuiPokemonButton(backBtn, "VOLTAR", true))
    {
        PlaySound(selectSound);
        currentScreen = MAIN_MENU;
        unloadCreditSprites();

        // Resetar scroll para próxima visualização
        creditsScroll = 0;
        autoScroll = true;
    }

    // Remover macros temporárias
#undef DRAW_CENTERED_TEXT
#undef DRAW_SECTION_HEADER
}

void updateCredits(void)
{
    // Inicializar estrelas na primeira vez
    static bool starsInitialized = false;
    if (!starsInitialized)
    {
        initStars();
        starsInitialized = true;
    }

    static bool spritesInitialized = false;
    if (!spritesInitialized) {
        initCreditSprites();
        spritesInitialized = true;
    }

    // Verificar scroll manual
    int wheel = GetMouseWheelMove();
    if (wheel != 0)
    {
        autoScroll = false;
        creditsScroll -= wheel * 60.0f;
    }

    // Verificar teclas de navegação
    if (IsKeyDown(KEY_UP))
    {
        autoScroll = false;
        creditsScroll -= GetFrameTime() * 300.0f;
    }
    else if (IsKeyDown(KEY_DOWN))
    {
        autoScroll = false;
        creditsScroll += GetFrameTime() * 300.0f;
    }

    // Verificar se uma tecla F11 foi pressionada para alternar tela cheia
    if (IsKeyPressed(KEY_F11))
    {
        pendingFullscreen = !pendingFullscreen;
    }
}
