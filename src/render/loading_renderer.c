#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#endif

#include "loading_renderer.h"
#include "raygui.h"
#include "resources.h"
#include "monsters.h"
#include "globals.h"
#include <math.h>
#include <stdio.h>

// Variáveis para a tela de carregamento
static Texture2D loadingTexture;         // Logo do jogo
static Texture2D pokeballTexture;        // Textura da Pokebola
static float loadingTime = 0.0f;         // Tempo decorrido
static float pulsatingFactor = 0.0f;     // Fator para efeito de pulsação
static float rotationAngle = 0.0f;       // Ângulo para rotação da pokebola
static float bgScroll = 0.0f;            // Deslocamento do fundo
static bool finishedLoading = false;     // Flag para indicar fim do carregamento
static float loadingProgress = 0.0f;     // Progresso do carregamento (0.0 a 1.0)
static float artificialDelay = 0.0f;     // Atraso artificial após carregamento

// Texto para mostrar de acordo com o progresso
static char* loadingTexts[] = {
    "Iniciando sistema...",
    "Carregando Pokédex...",
    "Carregando sprites...",
    "Inicializando batalha...",
    "Conectando à IA...",
    "Preparando criaturas...",
    "Quase pronto..."
};

// Partículas para efeitos visuais
#define MAX_PARTICLES 30

typedef struct {
    Vector2 position;
    float speed;
    float radius;
    float alpha;
    Color color;
} LoadingParticle;

static LoadingParticle particles[MAX_PARTICLES];

// Inicializa a tela de carregamento
void initLoadingScreen(void) {
    loadingTexture = LoadTexture("resources/loading_logo.png");
    pokeballTexture = LoadTexture("resources/pokeball.png");

    // Inicializar partículas
    for (int i = 0; i < MAX_PARTICLES; i++) {
        particles[i].position = (Vector2){
            GetRandomValue(0, GetScreenWidth()),
            GetRandomValue(0, GetScreenHeight())
        };
        particles[i].speed = GetRandomValue(30, 100) / 100.0f;
        particles[i].radius = GetRandomValue(2, 6);
        particles[i].alpha = GetRandomValue(50, 200) / 255.0f;
        particles[i].color = (Color){
            GetRandomValue(180, 255),
            GetRandomValue(180, 255),
            GetRandomValue(200, 255),
            255
        };
    }

    // Usar fallbacks se as texturas não carregarem
    if (loadingTexture.id == 0) {
        printf("Aviso: Textura do logo não encontrada. Usando fallback.\n");
    }

    if (pokeballTexture.id == 0) {
        printf("Aviso: Textura da pokébola não encontrada. Usando fallback.\n");
    }

    loadingTime = 0.0f;
    finishedLoading = false;
    initialized = true;
    loadingProgress = 0.0f;
    artificialDelay = 0.0f;
}

// Atualiza e desenha as partículas
static void updateAndDrawParticles(void) {
    float deltaTime = GetFrameTime();

    for (int i = 0; i < MAX_PARTICLES; i++) {
        // Atualizar posição
        particles[i].position.y -= particles[i].speed * deltaTime * 60.0f;

        // Resetar partícula quando sair da tela
        if (particles[i].position.y < -10) {
            particles[i].position.y = GetScreenHeight() + 10;
            particles[i].position.x = GetRandomValue(0, GetScreenWidth());
        }

        // Desenhar partícula
        Color particleColor = particles[i].color;
        particleColor.a = (unsigned char)(particles[i].alpha * 255 * (0.5f + 0.5f * sinf(loadingTime * 2.0f)));

        DrawCircleV(particles[i].position, particles[i].radius, particleColor);
    }
}

// Desenha a tela de carregamento com o progresso
void drawLoadingScreen(float progress) {
    if (!initialized) {
        initLoadingScreen();
    }

    // Atualizar temporizadores
    float deltaTime = GetFrameTime();
    loadingTime += deltaTime;
    pulsatingFactor = sinf(loadingTime * 2.0f) * 0.1f + 1.0f;
    rotationAngle += deltaTime * 60.0f;
    bgScroll += deltaTime * 20.0f;

    // Atualizar progresso (suavizar a transição)
    loadingProgress = loadingProgress * 0.9f + progress * 0.1f;

    // Verificar se carregou tudo
    if (progress >= 1.0f && !finishedLoading) {
        // Adicionar um pequeno atraso após tudo estar carregado
        artificialDelay += deltaTime;
        if (artificialDelay >= 1.5f) {
            finishedLoading = true;
        }
    }

    // Desenhar fundo estilo Pokémon
    ClearBackground(RAYWHITE);

    // Gradiente de fundo
    for (int i = 0; i < GetScreenHeight(); i++) {
        float factor = (float)i / GetScreenHeight();
        Color lineColor = (Color){
            (unsigned char)(50 * (1.0f - factor) + 20 * factor),
            (unsigned char)(100 * (1.0f - factor) + 70 * factor),
            (unsigned char)(200 * (1.0f - factor) + 150 * factor),
            255
        };
        DrawRectangle(0, i, GetScreenWidth(), 1, lineColor);
    }

    // Desenhar linhas horizontais para efeito
    for (int i = 0; i < GetScreenHeight(); i += 20) {
        int yPos = i - ((int)bgScroll % 20);
        DrawRectangle(0, yPos, GetScreenWidth(), 2, (Color){255, 255, 255, 30});
    }

    // Desenhar partículas
    updateAndDrawParticles();

    // Desenhar título do jogo
    const char* title = "PokeBattle";
    Vector2 titlePos = { GetScreenWidth() / 2 - MeasureText(title, 60) / 2, 100 };

    // Sombra do título
    DrawText(title, titlePos.x + 4, titlePos.y + 4, 60, (Color){0, 0, 0, 150});

    // Título com efeito de pulso
    float pulseScale = 1.0f + sinf(loadingTime * 2.0f) * 0.05f;
    DrawText(title, titlePos.x, titlePos.y, 60, (Color){255, 255, 0, 255});

    // Desenhar logo se disponível, ou pokébola como fallback
    int logoY = 180;
    if (loadingTexture.id != 0) {
        float logoScale = 0.5f * pulsatingFactor;
        DrawTextureEx(
            loadingTexture,
            (Vector2){
                GetScreenWidth() / 2 - (loadingTexture.width * logoScale) / 2,
                logoY
            },
            0.0f,
            logoScale,
            WHITE
        );
    } else {
        // Desenhar pokébola grande se o logo não estiver disponível
        int pokeSize = 100;
        DrawCircle(GetScreenWidth() / 2, logoY + 80, pokeSize, RED);
        DrawCircle(GetScreenWidth() / 2, logoY + 80, pokeSize - 5, WHITE);
        DrawRectangle(GetScreenWidth() / 2 - pokeSize, logoY + 80 - 3, pokeSize * 2, 6, BLACK);
        DrawCircle(GetScreenWidth() / 2, logoY + 80, 15, BLACK);
        DrawCircle(GetScreenWidth() / 2, logoY + 80, 10, WHITE);
    }

    // Área para barra de progresso
    Rectangle progressBarBg = {
        GetScreenWidth() / 2 - 200,
        GetScreenHeight() - 150,
        400,
        30
    };

    // Desenhar área de fundo da barra
    DrawRectangleRec(progressBarBg, (Color){0, 0, 0, 150});

    // Desenhar barra de progresso
    Rectangle progressBarFg = {
        progressBarBg.x + 3,
        progressBarBg.y + 3,
        (progressBarBg.width - 6) * loadingProgress,
        progressBarBg.height - 6
    };

    // Cor da barra baseada no progresso
    Color barColor;
    if (loadingProgress < 0.33f) {
        barColor = RED;
    } else if (loadingProgress < 0.66f) {
        barColor = YELLOW;
    } else {
        barColor = GREEN;
    }

    DrawRectangleRec(progressBarFg, barColor);

    // Desenhar pokébolas na barra
    if (pokeballTexture.id != 0) {
        float pokeScale = 0.2f;
        int pokeSize = pokeballTexture.width * pokeScale;
        int numPokeballs = 8;

        for (int i = 0; i < numPokeballs; i++) {
            float progress = (float)i / (numPokeballs - 1);
            if (progress <= loadingProgress) {
                DrawTextureEx(
                    pokeballTexture,
                    (Vector2){
                        progressBarBg.x + (progressBarBg.width * progress) - pokeSize/2,
                        progressBarBg.y + progressBarBg.height/2 - pokeSize/2
                    },
                    rotationAngle * (i % 2 == 0 ? 1 : -1),
                    pokeScale,
                    WHITE
                );
            }
        }
    }

    // Exibir porcentagem
    char progressText[16];
    sprintf(progressText, "%d%%", (int)(loadingProgress * 100));
    DrawText(
        progressText,
        progressBarBg.x + progressBarBg.width + 10,
        progressBarBg.y + 5,
        20,
        WHITE
    );

    // Exibir texto baseado no progresso
    int textIndex = (int)(loadingProgress * (sizeof(loadingTexts)/sizeof(loadingTexts[0])));
    if (textIndex >= sizeof(loadingTexts)/sizeof(loadingTexts[0])) {
        textIndex = sizeof(loadingTexts)/sizeof(loadingTexts[0]) - 1;
    }

    DrawText(
        loadingTexts[textIndex],
        progressBarBg.x,
        progressBarBg.y - 30,
        20,
        WHITE
    );

    // Desenhar dica na parte inferior
    const char* tip = "Dica: Pokémons com tipo vantajoso causam 2x de dano!";
    DrawText(
        tip,
        GetScreenWidth() / 2 - MeasureText(tip, 16) / 2,
        GetScreenHeight() - 50,
        16,
        (Color){255, 255, 255, 180}
    );

    // Desenhar avisos de carregamento
    if (finishedLoading) {
        const char* readyText = "Carregamento concluído! Pressione ENTER para continuar";

        // Texto piscante
        if (sinf(loadingTime * 4.0f) > 0) {
            DrawText(
                readyText,
                GetScreenWidth() / 2 - MeasureText(readyText, 24) / 2,
                GetScreenHeight() - 100,
                24,
                YELLOW
            );
        }
    }
}

// Atualiza a lógica da tela de carregamento
// Retorna true quando a tela deve terminar
bool updateLoadingScreen(void) {
    if (finishedLoading && (IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON))) {
        return true;
    }

    return false;
}

// Descarrega recursos utilizados pela tela de carregamento
void unloadLoadingScreen(void) {
    if (initialized) {
        UnloadTexture(loadingTexture);
        UnloadTexture(pokeballTexture);
        initialized = false;
    }
}