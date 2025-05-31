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
#include "globals.h"
#include "gui.h"
#include "hp_bar.h"
#include "battle_effects.h"


// Estados da animação de introdução
typedef enum {
    INTRO_WAITING = 0,
    INTRO_THROW_PLAYER,      // Lançar pokébola do jogador
    INTRO_THROW_OPPONENT,    // Lançar pokébola do oponente
    INTRO_FLYING,            // Pokébolas voando
    INTRO_LANDING,           // Pokébolas pousando
    INTRO_OPENING,           // Pokébolas se abrindo
    INTRO_REVEALING,         // Pokémons aparecendo
    INTRO_COMPLETE           // Animação completa
} BattleIntroState;



// Variáveis globais para a animação de introdução
static BattleIntroState introState = INTRO_WAITING;
static float introTimer = 0.0f;
static PokeballAnimation playerPokeball = {0};
static PokeballAnimation opponentPokeball = {0};
static bool introAnimationActive = false;
static float pokemonRevealAlpha = 0.0f;


// Sistema de partículas para cada pokébola
#define MAX_POKEBALL_PARTICLES 20
static PokeballParticle playerParticles[MAX_POKEBALL_PARTICLES] = {0};
static PokeballParticle opponentParticles[MAX_POKEBALL_PARTICLES] = {0};

// Mensagens personalizadas durante a introdução
const char* GetIntroMessage(BattleIntroState state, bool isPlayerPokeball) {
    switch (state) {
        case INTRO_WAITING:
            return "Preparando para a batalha...";
        case INTRO_THROW_PLAYER:
            return "Vai, meu Pokémon!";
        case INTRO_THROW_OPPONENT:
            return "O oponente enviou seu Pokémon!";
        case INTRO_FLYING:
            return "Pokébolas voando pelo ar...";
        case INTRO_LANDING:
            return "As Pokébolas estão pousando!";
        case INTRO_OPENING:
            return "As Pokébolas estão se abrindo!";
        case INTRO_REVEALING:
            return "Os Pokémons estão aparecendo!";
        case INTRO_COMPLETE:
            return "Que a batalha comece!";
        default:
            return "";
    }
}

// Sistema de sons melhorado para a introdução
void PlayIntroSound(BattleIntroState state) {
    switch (state) {
        case INTRO_THROW_PLAYER:
        case INTRO_THROW_OPPONENT:
            PlaySound(attackSound); // Som de lançamento
            break;
        case INTRO_LANDING:
            // Som já tocado no UpdatePokeballAnimation
            break;
        case INTRO_OPENING:
            // Som já tocado no UpdatePokeballAnimation
            break;
        case INTRO_REVEALING:
            PlaySound(selectSound); // Som de revelação
            break;
        case INTRO_COMPLETE:
            PlaySound(selectSound); // Som de confirmação
            break;
        default:
            break;
    }
}

// Função para criar efeito de "vapor" quando pokémon aparece
void CreatePokemonRevealEffect(Vector2 position, bool isPlayer) {
    // Criar efeito de vapor/névoa
    for (int i = 0; i < 15; i++) {
        float angle = ((float)i / 15) * 2 * PI;
        Vector2 velocity = {
            cosf(angle) * (30 + rand() % 40),
            sinf(angle) * (30 + rand() % 40) - 20 // Tendência para cima
        };

        Color vaporColor = isPlayer ?
            (Color){100, 150, 255, 180} :
            (Color){255, 150, 100, 180};

        // Usar sistema de partículas existente ou criar novo efeito
        CreateHealEffect(position, 0); // Por enquanto usar efeito de cura
    }
}


// Função para inicializar a animação de introdução
void StartBattleIntroAnimation(void) {
    printf("[INTRO] Iniciando animação de introdução da batalha\n");

    introAnimationActive = true;
    introState = INTRO_WAITING;
    introTimer = 0.0f;
    pokemonRevealAlpha = 0.0f;

    // Configurar pokébola do jogador
    playerPokeball.startPos = (Vector2){-100, GetScreenHeight() - 200}; // Vem da esquerda
    playerPokeball.targetPos = (Vector2){GetScreenWidth() / 3, GetScreenHeight() / 1.8f + 50}; // Posição final
    playerPokeball.currentPos = playerPokeball.startPos;
    playerPokeball.velocity = (Vector2){0, 0};
    playerPokeball.rotation = 0.0f;
    playerPokeball.rotationSpeed = 720.0f; // 2 rotações por segundo
    playerPokeball.scale = 1.0f;
    playerPokeball.bounceHeight = 0.0f;
    playerPokeball.bounceTimer = 0.0f;
    playerPokeball.openTimer = 0.0f;
    playerPokeball.isLanded = false;
    playerPokeball.isOpening = false;
    playerPokeball.isOpen = false;
    playerPokeball.tint = (Color){255, 100, 100, 255}; // Vermelho

    // Configurar pokébola do oponente
    opponentPokeball.startPos = (Vector2){GetScreenWidth() + 100, GetScreenHeight() - 150}; // Vem da direita
    opponentPokeball.targetPos = (Vector2){GetScreenWidth() * 2 / 3, GetScreenHeight() / 2.6f + 50}; // Posição final
    opponentPokeball.currentPos = opponentPokeball.startPos;
    opponentPokeball.velocity = (Vector2){0, 0};
    opponentPokeball.rotation = 0.0f;
    opponentPokeball.rotationSpeed = -650.0f; // Rotação oposta
    opponentPokeball.scale = 1.0f;
    opponentPokeball.bounceHeight = 0.0f;
    opponentPokeball.bounceTimer = 0.0f;
    opponentPokeball.openTimer = 0.0f;
    opponentPokeball.isLanded = false;
    opponentPokeball.isOpening = false;
    opponentPokeball.isOpen = false;
    opponentPokeball.tint = (Color){100, 100, 255, 255}; // Azul

    printf("[INTRO] Pokébolas configuradas - Jogador: (%.1f,%.1f) -> (%.1f,%.1f)\n",
           playerPokeball.startPos.x, playerPokeball.startPos.y,
           playerPokeball.targetPos.x, playerPokeball.targetPos.y);
    printf("[INTRO] Oponente: (%.1f,%.1f) -> (%.1f,%.1f)\n",
           opponentPokeball.startPos.x, opponentPokeball.startPos.y,
           opponentPokeball.targetPos.x, opponentPokeball.targetPos.y);
}

// Função para atualizar a animação de uma pokébola
void UpdatePokeballAnimation(PokeballAnimation* pokeball, float deltaTime) {
    if (pokeball == NULL) return;

    // Atualizar rotação
    pokeball->rotation += pokeball->rotationSpeed * deltaTime;

    // Atualizar posição baseada na velocidade
    pokeball->currentPos.x += pokeball->velocity.x * deltaTime;
    pokeball->currentPos.y += pokeball->velocity.y * deltaTime;

    // Verificar se pousou
    if (!pokeball->isLanded && pokeball->currentPos.y >= pokeball->targetPos.y) {
        pokeball->isLanded = true;
        pokeball->currentPos.y = pokeball->targetPos.y;
        pokeball->bounceTimer = 0.0f;
        pokeball->velocity = (Vector2){0, 0};

        // Efeito de pouso MELHORADO
        CreateDamageEffect(pokeball->currentPos, 0, false, false);
        TriggerScreenShake(4.0f, 0.4f);
        PlaySound(hitSound);

        // Criar partículas de impacto
        bool isPlayer = (pokeball == &playerPokeball);
        CreatePokeballOpeningEffect(pokeball->currentPos, isPlayer);

        printf("[INTRO] Pokébola pousou com efeitos melhorados\n");
    }

    // Animação de bounce após pousar
    if (pokeball->isLanded && !pokeball->isOpening) {
        pokeball->bounceTimer += deltaTime * 8.0f; // Frequência do bounce
        pokeball->bounceHeight = sinf(pokeball->bounceTimer) * 10.0f * expf(-pokeball->bounceTimer * 0.3f); // Decaimento

        if (pokeball->bounceTimer > 6.0f) { // Após 6 unidades de tempo, parar de bouncing
            pokeball->bounceHeight = 0.0f;
        }
    }

    // Animação de abertura
    if (pokeball->isOpening) {
        pokeball->openTimer += deltaTime * 3.0f;
        pokeball->scale = 1.0f + sinf(pokeball->openTimer * 2.0f) * 0.4f; // Pulsar mais intenso

        // Criar partículas durante abertura
        if (fmodf(pokeball->openTimer, 0.1f) < deltaTime) { // A cada 0.1s
            bool isPlayer = (pokeball == &playerPokeball);
            CreatePokeballOpeningEffect(pokeball->currentPos, isPlayer);
        }

        if (pokeball->openTimer >= 2.0f) {
            pokeball->isOpen = true;
            pokeball->isOpening = false;
            pokeball->scale = 0.3f; // Encolher muito após abrir

            // Efeito de abertura FINAL
            CreateHealEffect(pokeball->currentPos, 0);
            TriggerScreenShake(6.0f, 0.5f);
            PlaySound(selectSound);

            // Explosão final de partículas
            bool isPlayer = (pokeball == &playerPokeball);
            for (int i = 0; i < 5; i++) {
                CreatePokeballOpeningEffect(pokeball->currentPos, isPlayer);
            }

            printf("[INTRO] Pokébola abriu com efeito final!\n");
        }
    }
}

// Função para desenhar uma pokébola
void DrawPokeball(PokeballAnimation* pokeball) {
    if (pokeball == NULL) return;

    Vector2 drawPos = {
        pokeball->currentPos.x,
        pokeball->currentPos.y - pokeball->bounceHeight
    };

    float drawRadius = 25.0f * pokeball->scale;

    // Desenhar sombra
    Color shadowColor = (Color){0, 0, 0, 100};
    DrawCircle((int)pokeball->currentPos.x, (int)pokeball->currentPos.y + 5, drawRadius * 0.8f, shadowColor);

    // Corpo principal da pokébola
    DrawCircle((int)drawPos.x, (int)drawPos.y, drawRadius, pokeball->tint);

    // Metade superior (mais clara)
    Color topColor = (Color){
        (unsigned char)fminf(255, pokeball->tint.r + 50),
        (unsigned char)fminf(255, pokeball->tint.g + 50),
        (unsigned char)fminf(255, pokeball->tint.b + 50),
        255
    };
    DrawCircle((int)drawPos.x, (int)drawPos.y - drawRadius * 0.3f, drawRadius * 0.8f, topColor);

    // Linha divisória
    DrawRectangle(
        (int)(drawPos.x - drawRadius),
        (int)(drawPos.y - drawRadius * 0.1f),
        (int)(drawRadius * 2),
        (int)(drawRadius * 0.2f),
        BLACK
    );

    // Botão central
    DrawCircle((int)drawPos.x, (int)drawPos.y, drawRadius * 0.2f, BLACK);
    DrawCircle((int)drawPos.x, (int)drawPos.y, drawRadius * 0.15f, WHITE);

    // Efeito de brilho
    if (!pokeball->isOpen) {
        float brightX = drawPos.x + cosf(pokeball->rotation * DEG2RAD) * (drawRadius * 0.4f);
        float brightY = drawPos.y + sinf(pokeball->rotation * DEG2RAD) * (drawRadius * 0.4f);
        DrawCircle((int)brightX, (int)brightY, drawRadius * 0.08f, (Color){255, 255, 255, 180});
    }

    // Efeito de abertura (flash)
    if (pokeball->isOpening) {
        Color flashColor = (Color){255, 255, 255, (unsigned char)(100 * sinf(pokeball->openTimer * 4.0f))};
        DrawCircle((int)drawPos.x, (int)drawPos.y, drawRadius * 1.5f, flashColor);
    }
}


// Função principal para atualizar a animação de introdução
void UpdateBattleIntroAnimation(void) {
    if (!introAnimationActive) return;

    float deltaTime = GetFrameTime();
    introTimer += deltaTime;

    // Atualizar partículas
    UpdatePokeballParticles(playerParticles, deltaTime);
    UpdatePokeballParticles(opponentParticles, deltaTime);

    BattleIntroState previousState = introState;

    switch (introState) {
        case INTRO_WAITING:
            if (introTimer >= 0.8f) { // Tempo maior para apreciar
                introState = INTRO_THROW_PLAYER;
                introTimer = 0.0f;
                printf("[INTRO] Estado: THROW_PLAYER\n");
            }
            break;

        case INTRO_THROW_PLAYER:
            {
                Vector2 direction = {
                    playerPokeball.targetPos.x - playerPokeball.startPos.x,
                    playerPokeball.targetPos.y - playerPokeball.startPos.y
                };
                float distance = sqrtf(direction.x * direction.x + direction.y * direction.y);

                // Velocidade do lançamento (mais realista)
                playerPokeball.velocity.x = (direction.x / distance) * 450.0f;
                playerPokeball.velocity.y = (direction.y / distance) * 350.0f - 150.0f; // Arco mais alto

                introState = INTRO_THROW_OPPONENT;
                introTimer = 0.0f;
                printf("[INTRO] Estado: THROW_OPPONENT\n");
            }
            break;

        case INTRO_THROW_OPPONENT:
            if (introTimer >= 0.5f) { // Delay maior entre lançamentos
                Vector2 direction = {
                    opponentPokeball.targetPos.x - opponentPokeball.startPos.x,
                    opponentPokeball.targetPos.y - opponentPokeball.startPos.y
                };
                float distance = sqrtf(direction.x * direction.x + direction.y * direction.y);

                opponentPokeball.velocity.x = (direction.x / distance) * 420.0f;
                opponentPokeball.velocity.y = (direction.y / distance) * 320.0f - 130.0f;

                introState = INTRO_FLYING;
                introTimer = 0.0f;
                printf("[INTRO] Estado: FLYING\n");
            }
            break;

        case INTRO_FLYING:
            // Aplicar gravidade mais realista
            playerPokeball.velocity.y += 450.0f * deltaTime;
            opponentPokeball.velocity.y += 450.0f * deltaTime;

            // Resistência do ar
            playerPokeball.velocity.x *= 0.995f;
            opponentPokeball.velocity.x *= 0.995f;

            if (playerPokeball.isLanded && opponentPokeball.isLanded) {
                introState = INTRO_LANDING;
                introTimer = 0.0f;
                printf("[INTRO] Estado: LANDING\n");
            }
            break;

        case INTRO_LANDING:
            if (introTimer >= 2.0f) { // Tempo maior para bounce
                playerPokeball.isOpening = true;
                opponentPokeball.isOpening = true;
                introState = INTRO_OPENING;
                introTimer = 0.0f;
                printf("[INTRO] Estado: OPENING\n");
            }
            break;

        case INTRO_OPENING:
            if (playerPokeball.isOpen && opponentPokeball.isOpen) {
                // Criar efeitos de revelação
                CreatePokemonRevealEffect(playerPokeball.currentPos, true);
                CreatePokemonRevealEffect(opponentPokeball.currentPos, false);

                introState = INTRO_REVEALING;
                introTimer = 0.0f;
                pokemonRevealAlpha = 0.0f;
                printf("[INTRO] Estado: REVEALING\n");
            }
            break;

        case INTRO_REVEALING:
            // Revelar pokémons mais lentamente para mais dramatismo
            pokemonRevealAlpha += deltaTime * 1.5f; // Mais lento
            if (pokemonRevealAlpha >= 1.0f) {
                pokemonRevealAlpha = 1.0f;
                introState = INTRO_COMPLETE;
                introTimer = 0.0f;
                printf("[INTRO] Estado: COMPLETE\n");
            }
            break;

        case INTRO_COMPLETE:
            if (introTimer >= 1.0f) { // Tempo maior para apreciar
                introAnimationActive = false;
                printf("[INTRO] Animação de introdução completa!\n");
            }
            break;
    }

    // Tocar som se mudou de estado
    if (previousState != introState) {
        PlayIntroSound(introState);
    }

    // Atualizar pokébolas
    UpdatePokeballAnimation(&playerPokeball, deltaTime);
    UpdatePokeballAnimation(&opponentPokeball, deltaTime);
}


// Função para desenhar a animação de introdução
void DrawBattleIntroAnimation(void) {
    if (!introAnimationActive) return;

    // Desenhar fundo escurecido para dar foco
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){0, 0, 0, 50});

    // Desenhar partículas primeiro (atrás das pokébolas)
    DrawPokeballParticles(playerParticles);
    DrawPokeballParticles(opponentParticles);

    // Desenhar pokébolas melhoradas
    if (!playerPokeball.isOpen) {
        DrawEnhancedPokeball(&playerPokeball);
    }

    if (!opponentPokeball.isOpen) {
        DrawEnhancedPokeball(&opponentPokeball);
    }

    // Desenhar pokémons sendo revelados (se estiver na fase de revelação)
    if (introState >= INTRO_REVEALING && pokemonRevealAlpha > 0.0f &&
        battleSystem && battleSystem->playerTeam && battleSystem->opponentTeam) {

        PokeMonster* playerMonster = battleSystem->playerTeam->current;
        PokeMonster* opponentMonster = battleSystem->opponentTeam->current;

        // Desenhar com alpha crescente
        drawMonsterInBattleWithAlpha(playerMonster, true, pokemonRevealAlpha);
        drawMonsterInBattleWithAlpha(opponentMonster, false, pokemonRevealAlpha);
    }

    // Caixa de mensagem estilizada
    Rectangle messageBox = {
        GetScreenWidth() / 2 - 300,
        GetScreenHeight() - 100,
        600,
        80
    };

    // Fundo da mensagem
    DrawRectangleRounded(messageBox, 0.3f, 8, (Color){0, 0, 0, 200});
    DrawRectangleRoundedLines(messageBox, 0.3f, 8, WHITE);

    // Texto da mensagem
    const char* message = GetIntroMessage(introState, true);
    int fontSize = 24;
    int textWidth = MeasureText(message, fontSize);

    DrawText(message,
            messageBox.x + (messageBox.width - textWidth) / 2,
            messageBox.y + (messageBox.height - fontSize) / 2,
            fontSize,
            WHITE);

    // Indicador de progresso
    Rectangle progressBar = {
        messageBox.x + 50,
        messageBox.y + messageBox.height - 20,
        messageBox.width - 100,
        8
    };

    float progress = 0.0f;
    switch (introState) {
        case INTRO_WAITING: progress = 0.1f; break;
        case INTRO_THROW_PLAYER: progress = 0.2f; break;
        case INTRO_THROW_OPPONENT: progress = 0.3f; break;
        case INTRO_FLYING: progress = 0.5f; break;
        case INTRO_LANDING: progress = 0.6f; break;
        case INTRO_OPENING: progress = 0.8f; break;
        case INTRO_REVEALING: progress = 0.9f + pokemonRevealAlpha * 0.1f; break;
        case INTRO_COMPLETE: progress = 1.0f; break;
    }

    DrawRectangleRec(progressBar, (Color){50, 50, 50, 255});
    DrawRectangle(
        progressBar.x,
        progressBar.y,
        progressBar.width * progress,
        progressBar.height,
        (Color){100, 200, 255, 255}
    );

    // Instrução para pular (apenas após primeira fase)
    if (introState > INTRO_WAITING) {
        const char* skipText = "Pressione qualquer tecla para pular";
        DrawText(skipText,
                GetScreenWidth() / 2 - MeasureText(skipText, 16) / 2,
                GetScreenHeight() - 25,
                16,
                (Color){255, 255, 255, 150});
    }

    // Nome dos pokémons aparecendo durante revelação
    if (introState == INTRO_REVEALING && pokemonRevealAlpha > 0.5f &&
        battleSystem && battleSystem->playerTeam && battleSystem->opponentTeam) {

        PokeMonster* playerMonster = battleSystem->playerTeam->current;
        PokeMonster* opponentMonster = battleSystem->opponentTeam->current;

        Color nameColor = (Color){255, 255, 255, (unsigned char)(255 * pokemonRevealAlpha)};

        // Nome do pokémon do jogador
        DrawText(playerMonster->name,
                GetScreenWidth() / 3 - MeasureText(playerMonster->name, 20) / 2,
                GetScreenHeight() / 1.5f,
                20,
                nameColor);

        // Nome do pokémon oponente
        DrawText(opponentMonster->name,
                GetScreenWidth() * 2 / 3 - MeasureText(opponentMonster->name, 20) / 2,
                GetScreenHeight() / 3,
                20,
                nameColor);
    }
}

void SetupCustomBattleIntro(const char* playerPokeballColor, const char* opponentPokeballColor) {
    // Permitir personalizar cores das pokébolas
    if (strcmp(playerPokeballColor, "red") == 0) {
        playerPokeball.tint = (Color){255, 100, 100, 255};
    } else if (strcmp(playerPokeballColor, "blue") == 0) {
        playerPokeball.tint = (Color){100, 100, 255, 255};
    } else if (strcmp(playerPokeballColor, "green") == 0) {
        playerPokeball.tint = (Color){100, 255, 100, 255};
    }

    if (strcmp(opponentPokeballColor, "red") == 0) {
        opponentPokeball.tint = (Color){255, 100, 100, 255};
    } else if (strcmp(opponentPokeballColor, "blue") == 0) {
        opponentPokeball.tint = (Color){100, 100, 255, 255};
    } else if (strcmp(opponentPokeballColor, "purple") == 0) {
        opponentPokeball.tint = (Color){200, 100, 255, 255};
    }
}

// Verificar se a animação de introdução está ativa
bool IsBattleIntroActive(void) {
    return introAnimationActive;
}

// Pular a animação de introdução
void SkipBattleIntro(void) {
    if (introAnimationActive) {
        introAnimationActive = false;
        pokemonRevealAlpha = 1.0f;
        printf("[INTRO] Animação pulada pelo usuário\n");
    }
}

// Função para criar partículas de abertura da pokébola
void CreatePokeballOpeningEffect(Vector2 position, bool isPlayer) {
    PokeballParticle* particles = isPlayer ? playerParticles : opponentParticles;

    for (int i = 0; i < MAX_POKEBALL_PARTICLES; i++) {
        if (!particles[i].active) {
            // Configurar partícula
            particles[i].position = position;

            // Velocidade em círculo
            float angle = ((float)i / MAX_POKEBALL_PARTICLES) * 2 * PI;
            float speed = 100.0f + (rand() % 50);
            particles[i].velocity = (Vector2){
                cosf(angle) * speed,
                sinf(angle) * speed - 50.0f // Tendência para cima
            };

            particles[i].life = 0.0f;
            particles[i].maxLife = 1.5f + (rand() % 100) * 0.01f;
            particles[i].color = isPlayer ?
                (Color){255, 150, 150, 255} :
                (Color){150, 150, 255, 255};
            particles[i].size = 6.0f + (rand() % 4);
            particles[i].active = true;

            break;
        }
    }
}

// Função para atualizar partículas da pokébola
void UpdatePokeballParticles(PokeballParticle* particles, float deltaTime) {
    for (int i = 0; i < MAX_POKEBALL_PARTICLES; i++) {
        if (!particles[i].active) continue;

        // Atualizar posição
        particles[i].position.x += particles[i].velocity.x * deltaTime;
        particles[i].position.y += particles[i].velocity.y * deltaTime;

        // Aplicar gravidade
        particles[i].velocity.y += 100.0f * deltaTime;

        // Reduzir velocidade (atrito)
        particles[i].velocity.x *= 0.98f;
        particles[i].velocity.y *= 0.98f;

        // Atualizar vida
        particles[i].life += deltaTime;

        // Fade out
        float t = particles[i].life / particles[i].maxLife;
        particles[i].color.a = (unsigned char)(255 * (1.0f - t));
        particles[i].size *= 0.99f;

        // Desativar se morreu
        if (particles[i].life >= particles[i].maxLife) {
            particles[i].active = false;
        }
    }
}

// Função para desenhar partículas da pokébola
void DrawPokeballParticles(PokeballParticle* particles) {
    for (int i = 0; i < MAX_POKEBALL_PARTICLES; i++) {
        if (!particles[i].active) continue;

        DrawCircle(
            (int)particles[i].position.x,
            (int)particles[i].position.y,
            particles[i].size,
            particles[i].color
        );
    }
}

// Função melhorada para desenhar pokébola com mais detalhes
void DrawEnhancedPokeball(PokeballAnimation* pokeball) {
    if (pokeball == NULL) return;
    float deltaTime = GetFrameTime();
    introTimer += deltaTime;

    Vector2 drawPos = {
        pokeball->currentPos.x,
        pokeball->currentPos.y - pokeball->bounceHeight
    };

    float drawRadius = 30.0f * pokeball->scale; // Pokébola maior

    // Desenhar sombra melhorada
    Color shadowColor = (Color){0, 0, 0, (unsigned char)(80 * pokeball->scale)};
    DrawCircle((int)pokeball->currentPos.x + 3, (int)pokeball->currentPos.y + 8, drawRadius * 0.9f, shadowColor);

    // Brilho de fundo (aura)
    if (pokeball->isOpening) {
        Color auraColor = (Color){255, 255, 255, (unsigned char)(50 * sinf(pokeball->openTimer * 8.0f))};
        DrawCircle((int)drawPos.x, (int)drawPos.y, drawRadius * 1.8f, auraColor);
    }

    // Corpo principal da pokébola
    DrawCircle((int)drawPos.x, (int)drawPos.y, drawRadius, pokeball->tint);

    // Metade superior (branco/prata)
    Color topColor = WHITE;
    for (int i = 3; i >= 0; i--) {
        Color ringColor = (Color){255, 255, 255, (unsigned char)(255 - i * 30)};
        DrawCircle((int)drawPos.x, (int)drawPos.y - drawRadius * 0.15f, drawRadius * (0.85f - i * 0.05f), ringColor);
    }

    // Metade inferior (cor do time)
    Color bottomColor = pokeball->tint;
    DrawCircle((int)drawPos.x, (int)drawPos.y + drawRadius * 0.15f, drawRadius * 0.85f, bottomColor);

    // Linha divisória (mais detalhada)
    float lineThickness = drawRadius * 0.15f;
    DrawRectangle(
        (int)(drawPos.x - drawRadius),
        (int)(drawPos.y - lineThickness/2),
        (int)(drawRadius * 2),
        (int)lineThickness,
        BLACK
    );

    // Detalhes metálicos na linha
    DrawRectangle(
        (int)(drawPos.x - drawRadius * 0.8f),
        (int)(drawPos.y - lineThickness/2 + 2),
        (int)(drawRadius * 1.6f),
        2,
        (Color){150, 150, 150, 255}
    );

    // Botão central (mais detalhado)
    float buttonRadius = drawRadius * 0.25f;

    // Borda do botão
    DrawCircle((int)drawPos.x, (int)drawPos.y, buttonRadius + 3, BLACK);

    // Corpo do botão
    DrawCircle((int)drawPos.x, (int)drawPos.y, buttonRadius, (Color){200, 200, 200, 255});

    // Centro do botão
    DrawCircle((int)drawPos.x, (int)drawPos.y, buttonRadius * 0.7f, WHITE);

    // Luz no botão (se não estiver abrindo)
    if (!pokeball->isOpening) {
        Color lightColor = (Color){255, 255, 255, (unsigned char)(150 + 50 * sinf(pokeball->rotation * 0.1f))};
        DrawCircle((int)drawPos.x - 2, (int)drawPos.y - 2, buttonRadius * 0.3f, lightColor);
    }

    // Efeito de brilho móvel
    if (!pokeball->isOpen) {
        float brightAngle = pokeball->rotation * DEG2RAD;
        float brightX = drawPos.x + cosf(brightAngle) * (drawRadius * 0.6f);
        float brightY = drawPos.y + sinf(brightAngle) * (drawRadius * 0.6f);

        Color brightColor = (Color){255, 255, 255, 220};
        DrawCircle((int)brightX, (int)brightY, drawRadius * 0.12f, brightColor);

        // Segundo brilho menor
        brightColor.a = 120;
        DrawCircle((int)brightX + 5, (int)brightY + 5, drawRadius * 0.06f, brightColor);
    }

    // Efeito de abertura (múltiplos flashes)
    if (pokeball->isOpening) {
        for (int i = 1; i <= 3; i++) {
            float flashIntensity = sinf(pokeball->openTimer * (4.0f + i));
            if (flashIntensity > 0) {
                Color flashColor = (Color){255, 255, 255, (unsigned char)(80 * flashIntensity)};
                DrawCircle((int)drawPos.x, (int)drawPos.y, drawRadius * (1.0f + i * 0.3f), flashColor);
            }
        }

        // Raios de luz saindo da pokébola
        for (int i = 0; i < 8; i++) {
            float rayAngle = (i * PI / 4) + pokeball->openTimer;
            float rayLength = drawRadius * (2.0f + sinf(pokeball->openTimer * 3.0f));

            Vector2 rayEnd = {
                drawPos.x + cosf(rayAngle) * rayLength,
                drawPos.y + sinf(rayAngle) * rayLength
            };

            Color rayColor = (Color){255, 255, 255, (unsigned char)(100 * sinf(pokeball->openTimer * 2.0f))};
            DrawLineEx(drawPos, rayEnd, 3.0f, rayColor);
        }
    }

    // Trilha durante voo
    if (!pokeball->isLanded && (pokeball->velocity.x != 0 || pokeball->velocity.y != 0)) {
        for (int i = 1; i <= 5; i++) {
            Vector2 trailPos = {
                drawPos.x - pokeball->velocity.x * deltaTime * i * 0.1f,
                drawPos.y - pokeball->velocity.y * deltaTime * i * 0.1f
            };

            Color trailColor = pokeball->tint;
            trailColor.a = (unsigned char)(255 * (1.0f - (float)i / 6.0f));
            float trailSize = drawRadius * (1.0f - (float)i / 8.0f);

            DrawCircle((int)trailPos.x, (int)trailPos.y, trailSize, trailColor);
        }
    }
}

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

// Sistema de shake individual para cada Pokémon
typedef struct {
    bool isShaking;
    float shakeTimer;
    float shakeDuration;
    float shakeIntensity;
    Vector2 shakeOffset;
} PokemonShakeData;

static PokemonShakeData playerShake = {0};
static PokemonShakeData enemyShake = {0};

// Função para ativar shake em um Pokémon específico
void TriggerPokemonShake(bool isPlayerPokemon, float intensity, float duration) {
    PokemonShakeData* shake = isPlayerPokemon ? &playerShake : &enemyShake;

    shake->isShaking = true;
    shake->shakeTimer = 0.0f;
    shake->shakeDuration = duration;
    shake->shakeIntensity = intensity;
    shake->shakeOffset = (Vector2){0, 0};

    printf("[POKEMON SHAKE] Iniciando shake no %s: intensidade=%.1f, duração=%.1f\n",
           isPlayerPokemon ? "jogador" : "oponente", intensity, duration);
}

// Função para atualizar o shake dos Pokémons
void UpdatePokemonShakes(void) {
    float deltaTime = GetFrameTime();

    // Atualizar shake do jogador
    if (playerShake.isShaking) {
        playerShake.shakeTimer += deltaTime;

        if (playerShake.shakeTimer >= playerShake.shakeDuration) {
            playerShake.isShaking = false;
            playerShake.shakeOffset = (Vector2){0, 0};
        } else {
            float progress = playerShake.shakeTimer / playerShake.shakeDuration;
            float currentIntensity = playerShake.shakeIntensity * (1.0f - progress); // Diminui com o tempo

            playerShake.shakeOffset.x = (float)(rand() % 21 - 10) * currentIntensity * 0.1f;
            playerShake.shakeOffset.y = (float)(rand() % 21 - 10) * currentIntensity * 0.1f;
        }
    }

    // Atualizar shake do oponente
    if (enemyShake.isShaking) {
        enemyShake.shakeTimer += deltaTime;

        if (enemyShake.shakeTimer >= enemyShake.shakeDuration) {
            enemyShake.isShaking = false;
            enemyShake.shakeOffset = (Vector2){0, 0};
        } else {
            float progress = enemyShake.shakeTimer / enemyShake.shakeDuration;
            float currentIntensity = enemyShake.shakeIntensity * (1.0f - progress); // Diminui com o tempo

            enemyShake.shakeOffset.x = (float)(rand() % 21 - 10) * currentIntensity * 0.1f;
            enemyShake.shakeOffset.y = (float)(rand() % 21 - 10) * currentIntensity * 0.1f;
        }
    }
}

static TypewriterText typewriter = {0};
static float platformYOffset1 = 0.0f;
static float platformYOffset2 = 0.0f;
static float battleTimer = 0.0f;
static float hpAnimTimer = 0.0f;
static bool isHpAnimationActive = false;

static float damageShakeTimer = 0.0f;
static bool isShaking = false;
static float flashTimer = 0.0f;
static bool isFlashing = false;


// Velocidade do Typewriter (menor = mais rápido)
#define TYPEWRITER_SPEED 0.04f

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
        if (typewriter.currentChar < strlen(typewriter.fullText)) {
            typewriter.displayText[typewriter.currentChar] = typewriter.fullText[typewriter.currentChar];
            typewriter.displayText[typewriter.currentChar + 1] = '\0';
            typewriter.currentChar++;

            typewriter.charTimer -= typewriter.charDelay;

            // Pausas mais longas para pontuação (estilo Pokémon)
            char lastChar = typewriter.fullText[typewriter.currentChar - 1];
            if (lastChar == '.') {
                typewriter.charTimer -= typewriter.charDelay * 3.0f; // Pausa longa
            } else if (lastChar == ',' || lastChar == '!' || lastChar == '?') {
                typewriter.charTimer -= typewriter.charDelay * 2.0f; // Pausa média
            } else if (lastChar == ' ') {
                typewriter.charTimer -= typewriter.charDelay * 0.3f; // Pausa pequena
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
void drawMonsterInBattleWithAlpha(PokeMonster* monster, bool isPlayer, float alpha) {
    if (monster == NULL || alpha <= 0.0f) return;

    Vector2 platformPos, monsterPos;
    Animation* currentAnim = NULL;
    float baseScale = isPlayer ? 3.0f : 2.5f;

    // Posicionamento base
    if (isPlayer) {
        monsterPos = (Vector2){GetScreenWidth() / 3, GetScreenHeight() / 1.8f - 20};
        currentAnim = &monster->backAnimation;
    } else {
        monsterPos = (Vector2){GetScreenWidth() * 2 / 3, GetScreenHeight() / 2.6f - 20};
        currentAnim = &monster->frontAnimation;
    }

    // APLICAR O SHAKE INDIVIDUAL (se houver)
    Vector2 finalPos = monsterPos;
    extern PokemonShakeData playerShake, enemyShake; // Usar variáveis globais
    PokemonShakeData* shake = isPlayer ? &playerShake : &enemyShake;

    if (shake->isShaking) {
        finalPos.x += shake->shakeOffset.x;
        finalPos.y += shake->shakeOffset.y;
    }

    // Cor com alpha aplicado
    Color drawColor = (Color){255, 255, 255, (unsigned char)(255 * alpha)};

    // Atualizar e desenhar animação
    if (currentAnim->frameCount > 0) {
        UpdateAnimation(currentAnim);

        Texture2D currentFrame = currentAnim->frames[currentAnim->currentFrame];
        float scale = baseScale * (isPlayer ? 1.0f : 0.9f);

        // Efeito de animação de respiração
        float breatheEffect = sinf(battleTimer * 1.2f) * 0.03f;
        scale += breatheEffect;

        // Desenhar com alpha aplicado
        DrawTextureEx(
            currentFrame,
            (Vector2){
                finalPos.x - (currentFrame.width * scale) / 2,
                finalPos.y - (currentFrame.height * scale) / 2
            },
            0.0f,
            scale,
            drawColor
        );
    } else {
        // Fallback estático
        Texture2D fallback = isPlayer ? monster->backAnimation.frames[0] : monster->frontAnimation.frames[0];
        if (fallback.id != 0) {
            DrawTextureEx(
                fallback,
                (Vector2){
                    finalPos.x - (fallback.width * baseScale) / 2,
                    finalPos.y - (fallback.height * baseScale) / 2
                },
                0.0f,
                baseScale,
                drawColor
            );
        }
    }

    // Desenhar indicador de status (apenas se totalmente visível)
    if (monster->statusCondition != STATUS_NONE && alpha >= 0.8f) {
        Vector2 statusPos = isPlayer
                                ? (Vector2){finalPos.x - 50, finalPos.y - 80}
                                : (Vector2){finalPos.x + 50, finalPos.y - 70};

        Color statusColor;
        const char* statusText;

        switch (monster->statusCondition) {
        case STATUS_PARALYZED:
            statusColor = (Color){240, 208, 48, (unsigned char)(255 * alpha)};
            statusText = "PAR";
            break;
        case STATUS_SLEEPING:
            statusColor = (Color){112, 88, 152, (unsigned char)(255 * alpha)};
            statusText = "SLP";
            break;
        case STATUS_BURNING:
            statusColor = (Color){240, 128, 48, (unsigned char)(255 * alpha)};
            statusText = "BRN";
            break;
        case STATUS_ATK_DOWN:
            statusColor = (Color){192, 48, 40, (unsigned char)(255 * alpha)};
            statusText = "ATK↓";
            break;
        case STATUS_DEF_DOWN:
            statusColor = (Color){48, 96, 240, (unsigned char)(255 * alpha)};
            statusText = "DEF↓";
            break;
        case STATUS_SPD_DOWN:
            statusColor = (Color){120, 200, 80, (unsigned char)(255 * alpha)};
            statusText = "SPD↓";
            break;
        default:
            statusColor = (Color){168, 168, 120, (unsigned char)(255 * alpha)};
            statusText = "???";
            break;
        }

        DrawCircle(statusPos.x, statusPos.y, 22, statusColor);
        DrawCircleLines(statusPos.x, statusPos.y, 22, (Color){0, 0, 0, (unsigned char)(255 * alpha)});
        DrawText(statusText,
                 statusPos.x - MeasureText(statusText, 14) / 2,
                 statusPos.y - 7,
                 14,
                 (Color){255, 255, 255, (unsigned char)(255 * alpha)});
    }
}

// Função para ativar efeitos visuais de dano (adicionar ao header também)
void triggerDamageEffects(bool isPlayer) {
    // Esta função precisa ser implementada com variáveis globais ou
    // um sistema de efeitos mais robusto
    printf("Efeito de dano ativado para %s\n", isPlayer ? "jogador" : "oponente");
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

    // SE ESTAMOS NA ANIMAÇÃO DE INTRODUÇÃO
    if (battleSystem->battleState == BATTLE_INTRO_ANIMATION) {
        // Desenhar apenas a animação de pokébolas
        DrawBattleIntroAnimation();

        // Texto de instrução
        const char* skipText = "Pressione qualquer tecla para pular";
        DrawText(skipText,
                GetScreenWidth() / 2 - MeasureText(skipText, 20) / 2,
                GetScreenHeight() - 30,
                20,
                (Color){255, 255, 255, 180});

        return; // Não desenhar mais nada durante a animação
    }

    // Monstros ativos
    PokeMonster* playerMonster = battleSystem->playerTeam->current;
    PokeMonster* opponentMonster = battleSystem->opponentTeam->current;

    // DESENHAR MONSTROS (com alpha se ainda estão sendo revelados)
    float monsterAlpha = 1.0f;

    // Se acabamos de sair da animação de introdução, usar alpha gradual
    if (battleSystem->battleState == BATTLE_INTRO) {
        extern float pokemonRevealAlpha; // Usar variável global da animação
        monsterAlpha = pokemonRevealAlpha;
    }

    drawMonsterInBattleWithAlpha(playerMonster, true, monsterAlpha);
    drawMonsterInBattleWithAlpha(opponentMonster, false, monsterAlpha);

    // Desenhar efeitos APÓS os monstros mas ANTES da UI
    DrawBattleEffects();

    // Caixas de status
    Rectangle enemyStatusBox = {20, 20, 250, 80};
    Rectangle playerStatusBox = {
        GetScreenWidth() - 280,
        GetScreenHeight() - 230,
        250,
        80
    };

    drawMonsterStatusBox(playerMonster, playerStatusBox, true);
    drawMonsterStatusBox(opponentMonster, enemyStatusBox, false);

    // Caixa de mensagem ou menu de ações
    Rectangle actionBox = {
        20,
        GetScreenHeight() - 140,
        GetScreenWidth() - 40,
        120
    };

    // Desenhar interface baseada no estado atual
    switch (battleSystem->battleState) {
        case BATTLE_INTRO:
            // Durante a introdução, apenas mostrar mensagem
            drawBattleMessage(actionBox);
            break;

        case BATTLE_SELECT_ACTION:
            if (isMonsterFainted(battleSystem->playerTeam->current)) {
                battleSystem->battleState = BATTLE_FORCED_SWITCH;
                battleSystem->playerTurn = true;
                return;
            }
            if (battleSystem->playerTurn) {
                drawBattleActionMenu(actionBox);
            } else {
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
            actionBox = (Rectangle){
                50,
                GetScreenHeight() - 320,
                GetScreenWidth() - 100,
                300
            };
            drawMonsterSelectionMenu(actionBox);
            break;

        case BATTLE_FORCED_SWITCH:
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
            drawBattleMessage(actionBox);
            break;
    }

    // Desenhar indicador da API
    drawAIIndicator();
}

/**
 * Atualiza a lógica da tela de batalha
 */

void updateBattleScreen(void) {
    // Atualizar temporizadores e efeitos
    float deltaTime = GetFrameTime();
    battleTimer += deltaTime;

    UpdateBattleEffects();
    UpdatePokemonShakes();

    // Animar plataformas
    platformYOffset1 = sinf(battleTimer * 0.5f) * 5.0f;
    platformYOffset2 = cosf(battleTimer * 0.6f) * 5.0f;

    // Atualizar efeitos de flash e animação para os sprites
    if (playerSprite.flashAlpha > 0.0f) {
        playerSprite.flashAlpha -= deltaTime * 2.0f;
        if (playerSprite.flashAlpha < 0.0f) playerSprite.flashAlpha = 0.0f;
    }

    if (enemySprite.flashAlpha > 0.0f) {
        enemySprite.flashAlpha -= deltaTime * 2.0f;
        if (enemySprite.flashAlpha < 0.0f) enemySprite.flashAlpha = 0.0f;
    }

    // Atualizar animação de HP
    if (isHpAnimationActive) {
        hpAnimTimer += deltaTime;
        if (hpAnimTimer >= 1.0f) {
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


