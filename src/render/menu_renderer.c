#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#endif

#include "menu_renderer.h"
#include "hud_elements.h"
#include "resources.h"
#include "globals.h"
#include "scaling.h"
#include <math.h>

static const GameState menuOptionStates[] = {
    OPPONENT_SELECTION,  // "JOGAR"
    SETTINGS,           // "CONFIGURAÇÕES"
    TYPES_TABLE,        // "TIPOS"
    CREDITS,            // "CRÉDITOS"
    EXIT                // "SAIR"
};



// Variáveis locais para animações
static float logoScale = 0.0f;
static float logoAlpha = 0.0f;
static float menuAlpha = 0.0f;
static float particleTimer = 0.0f;
static float backgroundScroll = 0.0f;
static float titleBobTimer = 0.0f;
static int selectedOption = 0;
static int animationState = 0; // 0=intro, 1=menu normal
static float introTimer = 0.0f;

#define MAX_PARTICLES 50
typedef struct {
    Vector2 position;
    Vector2 velocity;
    float radius;
    float alpha;
    Color color;
    bool active;
} Particle;

static Particle particles[MAX_PARTICLES];

float EaseElasticOut(float t) {
    float p = 0.3f;
    return powf(2.0f, -10.0f * t) * sinf((t - p / 4.0f) * (2.0f * PI) / p) + 1.0f;
}
// Inicializa partículas decorativas
static void initParticles(void) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        particles[i].position = (Vector2){
            GetRandomValue(0, GetScreenWidth()),
            GetRandomValue(0, GetScreenHeight())
        };
        particles[i].velocity = (Vector2){
            GetRandomValue(-50, 50) / 100.0f,
            GetRandomValue(-50, 50) / 100.0f
        };
        particles[i].radius = GetRandomValue(2, 6);
        particles[i].alpha = GetRandomValue(50, 200) / 255.0f;
        particles[i].color = (Color){
            GetRandomValue(100, 255),
            GetRandomValue(200, 255),
            GetRandomValue(200, 255),
            255
        };
        particles[i].active = true;
    }
}

// Atualiza as partículas
static void updateParticles(void) {
    float deltaTime = GetFrameTime();

    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[i].active) continue;

        // Mover partícula
        particles[i].position.x += particles[i].velocity.x * deltaTime * 60.0f;
        particles[i].position.y += particles[i].velocity.y * deltaTime * 60.0f;

        // Fazer partícula flutuar com alguma aleatoriedade
        particles[i].position.x += sinf(particleTimer * 2.0f + i) * 0.5f;
        particles[i].position.y += cosf(particleTimer * 1.5f + i) * 0.5f;

        // Verificar limites da tela
        if (particles[i].position.x < -10 ||
            particles[i].position.x > GetScreenWidth() + 10 ||
            particles[i].position.y < -10 ||
            particles[i].position.y > GetScreenHeight() + 10) {

            // Reposicionar partícula
            if (GetRandomValue(0, 1) == 0) {
                // Entrar pelo lado
                particles[i].position.x = GetRandomValue(0, 1) == 0 ? -5 : GetScreenWidth() + 5;
                particles[i].position.y = GetRandomValue(0, GetScreenHeight());
            } else {
                // Entrar por cima ou por baixo
                particles[i].position.x = GetRandomValue(0, GetScreenWidth());
                particles[i].position.y = GetRandomValue(0, 1) == 0 ? -5 : GetScreenHeight() + 5;
            }

            // Nova velocidade
            particles[i].velocity = (Vector2){
                GetRandomValue(-60, 60) / 100.0f,
                GetRandomValue(-60, 60) / 100.0f
            };
        }
    }
}

// Desenha as partículas
static void drawParticles(void) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[i].active) continue;

        // Tamanho e opacidade
        float sizeScale = 0.8f + sinf(particleTimer * 2.0f + i * 0.3f) * 0.2f;
        float opacityScale = 0.8f + sinf(particleTimer * 1.5f + i * 0.3f) * 0.2f;

        Color particleColor = particles[i].color;
        particleColor.a = (unsigned char)(particles[i].alpha * opacityScale * 255);

        DrawCircleV(particles[i].position,
                    particles[i].radius * sizeScale,
                    particleColor);
    }
}

void drawMainMenu(void) {
    // Atualizar a música do menu
    UpdateMusicStream(menuMusic);


    // Fundo azul degradê
    for (int i = 0; i < GetScreenHeight(); i++) {
        float factor = (float)i / GetScreenHeight();
        Color lineColor = (Color){
            (unsigned char)(50 * (1.0f - factor) + 10 * factor),
            (unsigned char)(100 * (1.0f - factor) + 50 * factor),
            (unsigned char)(200 * (1.0f - factor) + 150 * factor),
            255
        };
        DrawRectangle(0, i, GetScreenWidth(), 1, lineColor);
    }

    // Desenhar padrão de listras horizontais
    backgroundScroll -= GetFrameTime() * 20.0f;
    if (backgroundScroll < -20.0f) backgroundScroll += 20.0f;

    for (int i = 0; i < GetScreenHeight(); i += 20) {
        int yPos = i + (int)backgroundScroll;
        if (yPos < 0) yPos += 20;
        DrawRectangle(0, yPos, GetScreenWidth(), 2, (Color){255, 255, 255, 20});
    }

    // Atualizar temporizadores de animação
    particleTimer += GetFrameTime();
    titleBobTimer += GetFrameTime() * 2.0f;

    // Estado de animação de introdução
    if (animationState == 0) {
        introTimer += GetFrameTime();

        // Crescimento do logo
        if (introTimer < 1.5f) {
            logoScale = EaseElasticOut(introTimer / 1.5f) * 1.0f;
            logoAlpha = introTimer / 1.5f;
        }

        // Aparecimento do menu
        if (introTimer > 1.5f && introTimer < 2.5f) {
            menuAlpha = (introTimer - 1.5f) / 1.0f;
            if (menuAlpha > 1.0f) menuAlpha = 1.0f;
        }

        // Finalizar animação
        if (introTimer >= 2.5f) {
            animationState = 1;
            logoScale = 1.0f;
            logoAlpha = 1.0f;
            menuAlpha = 1.0f;
        }
    }

    // Atualizar e desenhar partículas
    updateParticles();
    drawParticles();

    // Desenhar título com animação de "flutuação"
    const char* title = "PokeBattle";
    int fontSize = (int)(ScaleFontSize(60));
    float yOffset = sinf(titleBobTimer) * 8.0f;

    // Título com efeito de sombra e cor
    Vector2 titleSize = MeasureTextEx(gameFont, title, fontSize, 1);
    Vector2 titlePos = {
        GetScreenWidth() / 2 - titleSize.x / 2,
        GetScreenHeight() / 4 - titleSize.y / 2 + yOffset
    };

    // Sombra
    DrawTextEx(gameFont, title,
               (Vector2){titlePos.x + 4, titlePos.y + 4},
               fontSize, 1,
               Fade(BLACK, 0.7f * logoAlpha));

    // Contorno
    for (int i = 0; i < 360; i += 45) {
        float rad = i * DEG2RAD;
        DrawTextEx(gameFont, title,
                  (Vector2){titlePos.x + cosf(rad) * 2, titlePos.y + sinf(rad) * 2},
                  fontSize, 1,
                  Fade(DARKBLUE, logoAlpha));
    }

    // Texto principal com gradiente
    for (int i = 0; i < titleSize.y; i++) {
        float factor = (float)i / titleSize.y;
        Color textColor = (Color){
            255,  // R
            (unsigned char)(220 + sinf(titleBobTimer + factor * 5) * 35),  // G
            (unsigned char)(50 + factor * 150),  // B
            (unsigned char)(255 * logoAlpha)  // A
        };

        Rectangle scissorRect = {
            titlePos.x, titlePos.y + i, titleSize.x, 1
        };
        BeginScissorMode(scissorRect.x, scissorRect.y, scissorRect.width, scissorRect.height);
        DrawTextEx(gameFont, title, titlePos, fontSize, 1, textColor);
        EndScissorMode();
    }

    // Desenhar ícone de Pokébola embaixo do título
    float pokeScale = 0.8f + sinf(titleBobTimer * 1.2f) * 0.1f;
    int pokeSize = (int)(60 * pokeScale);
    DrawCircle(
        GetScreenWidth() / 2,
        GetScreenHeight() / 4 + 70 + (int)yOffset,
        pokeSize,
        Fade(RED, 0.8f * logoAlpha)
    );
    DrawCircle(
        GetScreenWidth() / 2,
        GetScreenHeight() / 4 + 70 + (int)yOffset,
        pokeSize - 5,
        Fade(WHITE, 0.9f * logoAlpha)
    );
    DrawRectangle(
        GetScreenWidth() / 2 - pokeSize,
        GetScreenHeight() / 4 + 70 - 3 + (int)yOffset,
        pokeSize * 2,
        6,
        Fade(BLACK, 0.8f * logoAlpha)
    );
    DrawCircle(
        GetScreenWidth() / 2,
        GetScreenHeight() / 4 + 70 + (int)yOffset,
        10,
        Fade(BLACK, 0.8f * logoAlpha)
    );
    DrawCircle(
        GetScreenWidth() / 2,
        GetScreenHeight() / 4 + 70 + (int)yOffset,
        6,
        Fade(WHITE, 0.8f * logoAlpha)
    );

    // Desenhar opções do menu
    const char* options[] = {
        "JOGAR",
        "CONFIGURAÇÕES",
        "TIPOS",
        "CRÉDITOS",
        "SAIR"
    };
    int optionCount = 5;
    int menuStart = GetScreenHeight() / 2;

    for (int i = 0; i < optionCount; i++) {
        Rectangle optionRect = {
            GetScreenWidth() / 2 - 150,
            menuStart + i * 60,
            300,
            50
        };

        // Verificar o hover do mouse
        bool isHovered = CheckCollisionPointRec(GetMousePosition(), optionRect);
        if (isHovered) {
            selectedOption = i;
        }

        // Cor do botão com base na seleção
        Color buttonColor = (selectedOption == i) ?
                           (Color){0, 102, 204, (unsigned char)(255 * menuAlpha)} :
                           (Color){0, 51, 102, (unsigned char)(255 * menuAlpha)};

        // Efeito de pulso para o botão selecionado
        float pulseScale = 1.0f;
        if (selectedOption == i) {
            pulseScale = 1.0f + sinf(particleTimer * 5.0f) * 0.05f;

            // Desenhar Pokébola ao lado da opção selecionada
            float pokeX = optionRect.x - 40;
            DrawCircle(pokeX, optionRect.y + optionRect.height/2, 12, RED);
            DrawCircle(pokeX, optionRect.y + optionRect.height/2, 10, WHITE);
            DrawRectangle(pokeX - 12, optionRect.y + optionRect.height/2 - 2, 24, 4, BLACK);
            DrawCircle(pokeX, optionRect.y + optionRect.height/2, 3, BLACK);
        }

        // Ajustar retângulo com pulso
        Rectangle pulsedRect = {
            optionRect.x - (optionRect.width * pulseScale - optionRect.width) / 2,
            optionRect.y - (optionRect.height * pulseScale - optionRect.height) / 2,
            optionRect.width * pulseScale,
            optionRect.height * pulseScale
        };

        // Desenha o fundo do botão
        DrawRectangleRounded(pulsedRect, 0.2f, 10, buttonColor);
        DrawRectangleRoundedLines(pulsedRect, 0.2f, 10,Fade(BLACK, menuAlpha));

        // Desenha o texto
        DrawText(options[i],
                pulsedRect.x + pulsedRect.width/2 - MeasureText(options[i], 20)/2,
                pulsedRect.y + pulsedRect.height/2 - 10,
                20,
                Fade(WHITE, menuAlpha));

        // Verifica o clique do mouse em seleção
        if (isHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            PlaySound(selectSound);
            currentScreen = menuOptionStates[i];  // Use o mapeamento correto
        }
    }

    // Desenha as informações na parte inferior da tela
    DrawText("© 2025 - CESAR School",
            10,
            GetScreenHeight() - 30,
            16,
            Fade(WHITE, menuAlpha * 0.8f));

    // Indicador de versão
    DrawText("v1.0",
            GetScreenWidth() - 40,
            GetScreenHeight() - 30,
            16,
            Fade(WHITE, menuAlpha * 0.8f));

    // Status da IA
    drawAIIndicator();
}

void updateMainMenu(void) {
    // Inicializar as partículas na primeira vez
    static bool particlesInitialized = false;
    if (!particlesInitialized) {
        initParticles();
        particlesInitialized = true;
    }

    // Controle de teclado para navegação
    if (IsKeyPressed(KEY_UP)) {
        selectedOption = (selectedOption - 1 + 5) % 5;
        PlaySound(selectSound);
    }
    else if (IsKeyPressed(KEY_DOWN)) {
        selectedOption = (selectedOption + 1) % 5;
        PlaySound(selectSound);
    }
    else if (IsKeyPressed(KEY_ENTER)) {
        PlaySound(selectSound);
        currentScreen = menuOptionStates[selectedOption];
    }

    // Verificar se uma tecla F11 foi pressionada para alternar tela cheia
    if (IsKeyPressed(KEY_F11)) {
        pendingFullscreen = !pendingFullscreen;
    }
}