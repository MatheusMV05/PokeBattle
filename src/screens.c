/**
 * PokeBattle - Implementação das telas do jogo
 * 
 * Este arquivo contém as implementações das funções para as diferentes telas do jogo.
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include "raylib.h"
 #include "structures.h"
 #include "monsters.h"
 #include "battle.h"
 #include "ia_integration.h"
 #include "screens.h"
 #include <math.h>  // Para a função fmin
 #include "game_state.h"
 #include <stdbool.h>
 #include <curl/curl.h>
 
 // Variáveis externas do main.c
 extern BattleSystem* battleSystem;
 extern GameState currentScreen;
 extern bool gameRunning;
 extern bool vsBot;
 extern bool playerTurn;
 // Variáveis externas para tabela de tipos
extern float typeEffectiveness[TYPE_COUNT][TYPE_COUNT];
 
 // Recursos visuais
 static Font gameFont;
 static Texture2D backgroundTexture;
 static Texture2D menuBackground;
 static Texture2D battleBackground;
 static Texture2D monsterSelectBackground;
 static Texture2D typeIcons[TYPE_COUNT];
 static Sound selectSound;
 static Sound attackSound;
 static Sound hitSound;
 static Sound faintSound;
 static Music menuMusic;
 static Music battleMusic;
 
 // Variáveis para a seleção de monstros
 static MonsterList* playerTeam = NULL;
 static MonsterList* opponentTeam = NULL;
 static int selectedMonsterIndex = 0;
 static int teamSelectionCount = 0;
 static bool viewingStats = false;
 static PokeMonster* currentViewedMonster = NULL;
 
 // Variáveis de configuração
 static bool fullscreen = false;
 static float musicVolume = 0.7f;
 static float soundVolume = 0.8f;
 
 // Funções auxiliares de desenho
 bool drawButton(Rectangle bounds, const char* text, Color color);
 void drawMonsterCard(Rectangle bounds, PokeMonster* monster, bool selected);
 void drawTypeIcon(Rectangle bounds, MonsterType type);
 void drawHealthBar(Rectangle bounds, int currentHP, int maxHP, PokeMonster* monster);
 void drawMonsterStats(Rectangle bounds, PokeMonster* monster);
 void drawAttackList(Rectangle bounds, PokeMonster* monster, int selectedAttack);
 void drawMessageBox(Rectangle bounds, const char* message);
 void drawBattleHUD(void);
 void drawConfirmationDialog(const char* message, const char* yesText, const char* noText);
 void drawMonsterInBattle(PokeMonster* monster, Rectangle bounds, bool isPlayer);
 
 // Carrega texturas, fontes e recursos visuais
 void loadTextures(void) {
     // Carregar fonte
     gameFont = GetFontDefault(); // Em um jogo real, carregaria uma fonte personalizada
     
     // Carregar texturas
     backgroundTexture = LoadTexture("resources/background.png");
     menuBackground = LoadTexture("resources/menu_bg.png");
     battleBackground = LoadTexture("resources/battle_bg.png");
     monsterSelectBackground = LoadTexture("resources/select_bg.png");
     
     // Carregar ícones de tipo
     for (int i = 0; i < TYPE_COUNT; i++) {
         char filename[50];
         sprintf(filename, "resources/type_%s.png", getTypeName(i));
         typeIcons[i] = LoadTexture(filename);
     }
 }
 
 void unloadTextures(void) {
     // Descarregar fonte
     // UnloadFont(gameFont); // Se fosse uma fonte carregada
     
     // Descarregar texturas
     UnloadTexture(backgroundTexture);
     UnloadTexture(menuBackground);
     UnloadTexture(battleBackground);
     UnloadTexture(monsterSelectBackground);
     
     // Descarregar ícones de tipo
     for (int i = 0; i < TYPE_COUNT; i++) {
         UnloadTexture(typeIcons[i]);
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
 
 // Carrega sons e música
void loadSounds(void) {
    // Tentar reinicializar o áudio se falhar
    if (!IsAudioDeviceReady()) {
        printf("Iniciando dispositivo de áudio...\n");
        InitAudioDevice();
    }
    
    // Verificar se o áudio está disponível
    if (!IsAudioDeviceReady()) {
        printf("Aviso: Dispositivo de áudio não está disponível. Sons desativados.\n");
        return;
    }
    
    // Carregar efeitos sonoros
    selectSound = LoadSound("resources/select.wav");
    attackSound = LoadSound("resources/attack.wav");
    hitSound = LoadSound("resources/hit.wav");
    faintSound = LoadSound("resources/faint.wav");
    
    // Carregar músicas
    menuMusic = LoadMusicStream("resources/menu_music.mp3");
    battleMusic = LoadMusicStream("resources/battle_music.mp3");
    
    // Configurar volume
    SetMusicVolume(menuMusic, musicVolume);
    SetMusicVolume(battleMusic, musicVolume);
    SetSoundVolume(selectSound, soundVolume);
    SetSoundVolume(attackSound, soundVolume);
    SetSoundVolume(hitSound, soundVolume);
    SetSoundVolume(faintSound, soundVolume);
    
    // Iniciar música do menu
    PlayMusicStream(menuMusic);
}
 
 void unloadSounds(void) {
     // Parar músicas
     StopMusicStream(menuMusic);
     StopMusicStream(battleMusic);
     
     // Descarregar efeitos sonoros
     UnloadSound(selectSound);
     UnloadSound(attackSound);
     UnloadSound(hitSound);
     UnloadSound(faintSound);
     
     // Descarregar músicas
     UnloadMusicStream(menuMusic);
     UnloadMusicStream(battleMusic);
     
     // Encerrar áudio
     CloseAudioDevice();
 }
 
 // Menu principal
 void drawMainMenu(void) {
    // Atualizar música do menu
    UpdateMusicStream(menuMusic);
    
    // Cores principais do tema
    Color primaryColor = (Color){ 255, 51, 51, 255 };      // Vermelho Fire Red
    Color secondaryColor = (Color){ 51, 153, 255, 255 };   // Azul complementar
    Color bgColor = (Color){ 245, 245, 245, 255 };         // Fundo claro
    Color accentColor = (Color){ 255, 204, 0, 255 };       // Amarelo Pikachu
    
    // Efeito de fundo animado
    static float bgTimer = 0.0f;
    bgTimer += GetFrameTime() * 0.5f;
    
    // Desenhar fundo gradiente dinâmico
    for (int i = 0; i < GetScreenHeight(); i += 5) {
        float factor = (float)i / GetScreenHeight();
        // float offset = sinf(bgTimer + factor * 5.0f) * 0.05f; // Removido pois não estava sendo usado
        
        Color lineColor = (Color){
            (unsigned char)(bgColor.r * (1.0f - factor) + secondaryColor.r * factor),
            (unsigned char)(bgColor.g * (1.0f - factor) + secondaryColor.g * factor),
            (unsigned char)(bgColor.b * (1.0f - factor) + secondaryColor.b * factor),
            255
        };
        
        DrawRectangle(0, i, GetScreenWidth(), 5, lineColor);
    }
    
    // Título principal estilo Pokémon
    const char* title = "PokeBattle";
    
    // Título - Sombra
    Vector2 titleShadowPos = { GetScreenWidth()/2 - MeasureText(title, 70)/2 + 3, 53 };
    DrawText(title, titleShadowPos.x, titleShadowPos.y, 70, (Color){ 40, 40, 40, 200 });
    
    // Título - Contorno
    Vector2 titleOutlinePos = { GetScreenWidth()/2 - MeasureText(title, 70)/2, 50 };
    DrawText(title, titleOutlinePos.x - 2, titleOutlinePos.y - 2, 70, BLACK);
    DrawText(title, titleOutlinePos.x + 2, titleOutlinePos.y - 2, 70, BLACK);
    DrawText(title, titleOutlinePos.x - 2, titleOutlinePos.y + 2, 70, BLACK);
    DrawText(title, titleOutlinePos.x + 2, titleOutlinePos.y + 2, 70, BLACK);
    
    // Título - Texto principal
    DrawText(title, titleOutlinePos.x, titleOutlinePos.y, 70, primaryColor);
    
    // Subtítulo
    const char* subtitle = "Uma aventura de monstros de batalha";
    Vector2 subtitlePos = { GetScreenWidth()/2 - MeasureText(subtitle, 20)/2, 125 };
    DrawText(subtitle, subtitlePos.x, subtitlePos.y, 20, DARKGRAY);
    
    // Desenhar PokeBall decorativa
    int pokeballSize = 40;
    int pokeballX = GetScreenWidth()/2;
    int pokeballY = subtitlePos.y + 40;
    
    DrawCircle(pokeballX, pokeballY, pokeballSize, RED);
    DrawCircle(pokeballX, pokeballY, pokeballSize - 5, WHITE);
    DrawCircle(pokeballX, pokeballY, pokeballSize/4, WHITE);
    DrawCircle(pokeballX, pokeballY, pokeballSize/4 - 2, DARKGRAY);
    DrawLine(pokeballX - pokeballSize, pokeballY, pokeballX + pokeballSize, pokeballY, BLACK);
    
    // Área de menu
    Rectangle menuArea = {
        GetScreenWidth()/2 - 250,
        pokeballY + pokeballSize + 30,
        500,
        320
    };
    
    // Painel do menu com efeito decorativo
    DrawRectangleRounded(menuArea, 0.05f, 8, ColorAlpha(WHITE, 0.9f));
    DrawRectangleRoundedLines(menuArea, 0.05f, 8, DARKGRAY);
    
    // Desenhar decoração na borda do menu
    for (int i = 0; i < menuArea.width - 16; i += 8) {
        DrawRectangle(menuArea.x + 8 + i, menuArea.y + 8, 4, 4, 
                       (i % 16 == 0) ? primaryColor : secondaryColor);
        
        DrawRectangle(menuArea.x + 8 + i, menuArea.y + menuArea.height - 12, 4, 4, 
                       (i % 16 == 0) ? primaryColor : secondaryColor);
    }
    
    // Desenhar botões do menu
    int buttonWidth = 300;
    int buttonHeight = 50;
    int buttonSpacing = 15;
    int startY = menuArea.y + 30;
    
    // Opções do menu principal
    const char* menuOptions[] = {
        "INICIAR JOGO",
        "CONFIGURAÇÕES",
        "TABELA DE TIPOS",
        "CRÉDITOS",
        "SAIR"
    };
    
    // Cores para cada botão
    Color menuColors[] = {
        (Color){ 220, 60, 60, 255 },    // Vermelho - Jogar
        (Color){ 60, 180, 120, 255 },   // Verde - Configurações
        (Color){ 80, 100, 220, 255 },   // Azul - Tabela de Tipos
        (Color){ 220, 140, 60, 255 },   // Laranja - Créditos
        (Color){ 150, 60, 150, 255 }    // Roxo - Sair
    };
    
    // Ícones para os botões (representados por formas simples)
    for (int i = 0; i < 5; i++) {
        Rectangle buttonBounds = {
            menuArea.x + menuArea.width/2 - buttonWidth/2,
            startY,
            buttonWidth,
            buttonHeight
        };
        
        // Verificar se o mouse está sobre o botão
        bool isHovering = CheckCollisionPointRec(GetMousePosition(), buttonBounds);
        
        // Cores dinâmicas com base no hover
        Color currentColor = menuColors[i];
        if (isHovering) {
            // Brilho ao passar o mouse
            // Substituindo Clamp() por uma solução simples
            currentColor.r = (currentColor.r + 30) > 255 ? 255 : (currentColor.r + 30);
            currentColor.g = (currentColor.g + 30) > 255 ? 255 : (currentColor.g + 30);
            currentColor.b = (currentColor.b + 30) > 255 ? 255 : (currentColor.b + 30);
            
            // Destacar com linha pontilhada
            float time = GetTime() * 10.0f;
            for (int j = 0; j < buttonBounds.width; j += 4) {
                if ((int)(j + time) % 8 < 4) {
                    DrawRectangle(buttonBounds.x + j, buttonBounds.y - 5, 2, 2, WHITE);
                    DrawRectangle(buttonBounds.x + j, buttonBounds.y + buttonBounds.height + 3, 2, 2, WHITE);
                }
            }
        }
        
        // Desenhar botão com efeito de profundidade
        DrawRectangleRounded(buttonBounds, 0.2f, 8, currentColor);
        
        // Sombra na parte inferior - substitui DrawRectangleRoundedEx
        DrawRectangleRounded(
            (Rectangle){ buttonBounds.x, buttonBounds.y + buttonBounds.height - 5, buttonBounds.width, 5 },
            0.2f, 8, ColorAlpha(BLACK, 0.3f)
        );
        
        // Brilho na parte superior - substitui DrawRectangleRoundedEx
        DrawRectangleRounded(
            (Rectangle){ buttonBounds.x, buttonBounds.y, buttonBounds.width, 5 },
            0.2f, 8, ColorAlpha(WHITE, 0.3f)
        );
        
        // Desenhar ícone para cada botão
        Rectangle iconRect = { buttonBounds.x + 15, buttonBounds.y + buttonBounds.height/2 - 15, 30, 30 };
        switch (i) {
            case 0: // Jogar - Triângulo (Play)
                DrawPoly((Vector2){iconRect.x + 15, iconRect.y + 15}, 3, 15, 0, WHITE);
                break;
            case 1: // Configurações - Engrenagem
                DrawCircle(iconRect.x + 15, iconRect.y + 15, 13, WHITE);
                DrawCircle(iconRect.x + 15, iconRect.y + 15, 7, currentColor);
                for (int j = 0; j < 8; j++) {
                    float angle = j * PI / 4.0f;
                    DrawLine(
                        iconRect.x + 15 + cosf(angle) * 12,
                        iconRect.y + 15 + sinf(angle) * 12,
                        iconRect.x + 15 + cosf(angle) * 20,
                        iconRect.y + 15 + sinf(angle) * 20,
                        WHITE
                    );
                }
                break;
            case 2: // Tabela - Grid
                DrawRectangleLines(iconRect.x, iconRect.y, 30, 30, WHITE);
                DrawLine(iconRect.x, iconRect.y + 10, iconRect.x + 30, iconRect.y + 10, WHITE);
                DrawLine(iconRect.x, iconRect.y + 20, iconRect.x + 30, iconRect.y + 20, WHITE);
                DrawLine(iconRect.x + 10, iconRect.y, iconRect.x + 10, iconRect.y + 30, WHITE);
                DrawLine(iconRect.x + 20, iconRect.y, iconRect.x + 20, iconRect.y + 30, WHITE);
                break;
            case 3: // Créditos - Estrela
                DrawPoly((Vector2){iconRect.x + 15, iconRect.y + 15}, 5, 15, 0, WHITE);
                break;
            case 4: // Sair - X
                DrawLine(iconRect.x + 5, iconRect.y + 5, iconRect.x + 25, iconRect.y + 25, WHITE);
                DrawLine(iconRect.x + 25, iconRect.y + 5, iconRect.x + 5, iconRect.y + 25, WHITE);
                break;
        }
        
        // Texto do botão
        int fontSize = 24;
        DrawText(
            menuOptions[i], 
            buttonBounds.x + 60, 
            buttonBounds.y + buttonBounds.height/2 - fontSize/2, 
            fontSize, 
            WHITE
        );
        
        // Verificar clique
        if (isHovering && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            PlaySound(selectSound);
            
            // Mudança de tela com base na opção escolhida
            switch (i) {
                case 0: currentScreen = OPPONENT_SELECTION; break;
                case 1: currentScreen = SETTINGS; break;
                case 2: currentScreen = TYPES_TABLE; break;
                case 3: currentScreen = CREDITS; break;
                case 4: currentScreen = EXIT; break;
            }
        }
        
        startY += buttonHeight + buttonSpacing;
    }
    
    // Rodapé com versão do jogo
    const char* version = "v1.0.0 (2025)";
    DrawText(version, 
             GetScreenWidth() - MeasureText(version, 16) - 10, 
             GetScreenHeight() - 25, 
             16, 
             DARKGRAY);
             
    // Desenhar efeitos de partículas (pequenos pontos flutuando)
    static float particleTimer = 0.0f;
    particleTimer += GetFrameTime();
    
    for (int i = 0; i < 20; i++) {
        float x = sinf(particleTimer * 0.5f + i * 0.3f) * GetScreenWidth() * 0.4f + GetScreenWidth() * 0.5f;
        float y = cosf(particleTimer * 0.2f + i * 0.7f) * 100.0f + GetScreenHeight() * 0.2f;
        float size = sinf(particleTimer + i) * 2.0f + 3.0f;
        
        DrawCircle(x, y, size, ColorAlpha(accentColor, 0.7f));
    }
}
 
 void updateMainMenu(void) {
     // Atualização da lógica do menu principal, se necessário
 }
 
 // Seleção de adversário
 void drawOpponentSelection(void) {
     // Desenhar fundo
     ClearBackground(DARKBLUE);
     
     // Desenhar título
     const char* title = "Selecione seu Adversário";
     Vector2 titleSize = MeasureTextEx(gameFont, title, 40, 2);
     DrawTextEx(gameFont, title, (Vector2){ GetScreenWidth()/2 - titleSize.x/2, 50 }, 40, 2, WHITE);
     
     // Desenhar botões de seleção
     int buttonWidth = 250;
     int buttonHeight = 80;
     int buttonSpacing = 40;
     int startY = GetScreenHeight()/2 - buttonHeight - buttonSpacing/2;
     
     if (drawButton((Rectangle){ GetScreenWidth()/2 - buttonWidth/2, startY, buttonWidth, buttonHeight }, "Computador (Bot)", DARKGREEN)) {
         PlaySound(selectSound);
         vsBot = true;
         currentScreen = MONSTER_SELECTION;
         teamSelectionCount = 0;
         
         // Liberar time anterior se existir
         if (playerTeam != NULL) {
             freeMonsterList(playerTeam);
         }
         
         // Criar novo time do jogador
         playerTeam = createMonsterList();
     }
     
     startY += buttonHeight + buttonSpacing;
     if (drawButton((Rectangle){ GetScreenWidth()/2 - buttonWidth/2, startY, buttonWidth, buttonHeight }, "Outro Jogador", MAROON)) {
         PlaySound(selectSound);
         vsBot = false;
         currentScreen = MONSTER_SELECTION;
         teamSelectionCount = 0;
         
         // Liberar times anteriores se existirem
         if (playerTeam != NULL) {
             freeMonsterList(playerTeam);
         }
         if (opponentTeam != NULL) {
             freeMonsterList(opponentTeam);
         }
         
         // Criar novos times
         playerTeam = createMonsterList();
         opponentTeam = createMonsterList();
     }
     
     // Botão de voltar
     if (drawButton((Rectangle){ 20, GetScreenHeight() - 70, 150, 50 }, "Voltar", GRAY)) {
         PlaySound(selectSound);
         currentScreen = MAIN_MENU;
     }
 }
 
 void updateOpponentSelection(void) {
     // Atualização da lógica de seleção de adversário, se necessário
 }
 
 // Seleção de monstros
 void drawMonsterSelection(void) {
     // Desenhar fundo
     ClearBackground(LIGHTGRAY);
     
     // Desenhar instruções
     const char* instruction;
     if (vsBot) {
         instruction = "Selecione 3 monstros para seu time";
     } else {
         if (teamSelectionCount == 0) {
             instruction = "Jogador 1: Selecione 3 monstros para seu time";
         } else {
             instruction = "Jogador 2: Selecione 3 monstros para seu time";
         }
     }
     
     Vector2 instrSize = MeasureTextEx(gameFont, instruction, 30, 2);
     DrawTextEx(gameFont, instruction, (Vector2){ GetScreenWidth()/2 - instrSize.x/2, 20 }, 30, 2, DARKGRAY);
     
     // Se estiver visualizando estatísticas de um monstro
     if (viewingStats && currentViewedMonster != NULL) {
         // Desenhar detalhes do monstro
         drawMonsterStats((Rectangle){ 50, 80, GetScreenWidth() - 100, GetScreenHeight() - 160 }, currentViewedMonster);
         
         // Botão de voltar
         if (drawButton((Rectangle){ GetScreenWidth()/2 - 75, GetScreenHeight() - 70, 150, 50 }, "Voltar", GRAY)) {
             PlaySound(selectSound);
             viewingStats = false;
         }
     } else {
         // Desenhar grade de monstros disponíveis
         int monsterCount = getMonsterCount();
         int columns = 3;
       
         
         int cardWidth = 230;
         int cardHeight = 160;
         int spacingX = 20;
         int spacingY = 20;
         int startX = (GetScreenWidth() - (cardWidth * columns + spacingX * (columns - 1))) / 2;
         int startY = 80;
         
         for (int i = 0; i < monsterCount; i++) {
             int row = i / columns;
             int col = i % columns;
             
             Rectangle bounds = {
                 startX + col * (cardWidth + spacingX),
                 startY + row * (cardHeight + spacingY),
                 cardWidth,
                 cardHeight
             };
             
             PokeMonster* monster = getMonsterByIndex(i);
             if (monster != NULL) {
                 // Desenhar card do monstro
                 drawMonsterCard(bounds, monster, i == selectedMonsterIndex);
                 
                 // Botões de ação
                 Rectangle selectBounds = {
                     bounds.x + 10,
                     bounds.y + bounds.height - 60,
                     bounds.width / 2 - 15,
                     40
                 };
                 
                 Rectangle statsBounds = {
                     bounds.x + bounds.width / 2 + 5,
                     bounds.y + bounds.height - 60,
                     bounds.width / 2 - 15,
                     40
                 };
                 
                 // Verificar se o monstro já está no time
                 bool alreadySelected = false;
                 if (teamSelectionCount == 0) {
                     // Verificar no time do jogador 1
                     PokeMonster* current = playerTeam->first;
                     while (current != NULL) {
                         if (strcmp(current->name, monster->name) == 0) {
                             alreadySelected = true;
                             break;
                         }
                         current = current->next;
                     }
                 } else {
                     // Verificar no time do jogador 2
                     PokeMonster* current = opponentTeam->first;
                     while (current != NULL) {
                         if (strcmp(current->name, monster->name) == 0) {
                             alreadySelected = true;
                             break;
                         }
                         current = current->next;
                     }
                 }
                 
                 // Desenhar botão de seleção (desabilitado se já selecionado)
                 Color selectColor = alreadySelected ? GRAY : GREEN;
                 if (!alreadySelected && drawButton(selectBounds, "Selecionar", selectColor)) {
                    PlaySound(selectSound);
                    
                    // Adicionar monstro ao time apropriado
                    PokeMonster* newMonster = createMonsterCopy(monster);
                    printf("Criado novo monstro: %s\n", newMonster->name); // Debug
                    
                    if (teamSelectionCount == 0) {
                        // Time do jogador 1
                        addMonster(playerTeam, newMonster);
                        printf("Monstros do jogador após adição: %d\n", playerTeam->count); // Debug
                        
                        // Se já selecionou 3 monstros
                        if (playerTeam->count >= 3) {
                            printf("3 monstros selecionados para o jogador!\n"); // Debug
                            
                            if (vsBot) {
                                // Gerar time do bot aleatoriamente
                                opponentTeam = generateRandomTeam(3);
                                printf("Time do bot gerado: %d monstros\n", 
                                        opponentTeam ? opponentTeam->count : 0); // Debug
                                
                                // Iniciar batalha
                                startNewBattle(playerTeam, opponentTeam);
                                currentScreen = BATTLE_SCREEN;
                                StopMusicStream(menuMusic);
                                PlayMusicStream(battleMusic);
                            } else {
                                // Passar para a seleção do jogador 2
                                teamSelectionCount = 1;
                            }
                        }
                     } else {
                         // Time do jogador 2
                         addMonster(opponentTeam, newMonster);
                         
                         // Se já selecionou 3 monstros
                         if (opponentTeam->count >= 3) {
                             // Iniciar batalha
                             startNewBattle(playerTeam, opponentTeam);
                             currentScreen = BATTLE_SCREEN;
                             StopMusicStream(menuMusic);
                             PlayMusicStream(battleMusic);
                         }
                     }
                 }
                 
                 // Desenhar botão de estatísticas
                 if (drawButton(statsBounds, "Detalhes", BLUE)) {
                     PlaySound(selectSound);
                     viewingStats = true;
                     currentViewedMonster = monster;
                 }
             }
         }
         
         // Botão de voltar
         if (drawButton((Rectangle){ 20, GetScreenHeight() - 70, 150, 50 }, "Voltar", GRAY)) {
             PlaySound(selectSound);
             
             if (teamSelectionCount == 1 && !vsBot) {
                 // Voltar para a seleção do jogador 1
                 teamSelectionCount = 0;
                 
                 // Limpar o time do jogador 2
                 if (opponentTeam != NULL) {
                     freeMonsterList(opponentTeam);
                     opponentTeam = createMonsterList();
                 }
             } else {
                 // Voltar para a seleção de adversário
                 currentScreen = OPPONENT_SELECTION;
                 
                 // Limpar os times
                 if (playerTeam != NULL) {
                     freeMonsterList(playerTeam);
                     playerTeam = NULL;
                 }
                 
                 if (opponentTeam != NULL) {
                     freeMonsterList(opponentTeam);
                     opponentTeam = NULL;
                 }
             }
         }
     }
 }
 
 void updateMonsterSelection(void) {
     // Atualização da lógica de seleção de monstros, se necessário
 }
 
 // Função atualizada para desenhar a tela de batalha
void drawBattleScreen(void) {
    // Atualizar música da batalha
    UpdateMusicStream(battleMusic);
    
    // Desenhar fundo (cor de céu azul claro típico de Pokémon)
    ClearBackground((Color){ 120, 200, 255, 255 });
    
    // Desenhar grama/campo de batalha
    DrawRectangle(0, GetScreenHeight()/2, GetScreenWidth(), GetScreenHeight()/2, (Color){ 120, 180, 100, 255 });
    
    // Plataformas de batalha (estilo Fire Red)
    Color platformColor = (Color){ 210, 180, 140, 255 }; // Cor terra/areia
    
    // Plataforma do oponente (mais alta)
    DrawEllipse(GetScreenWidth()/4, GetScreenHeight()/2 - 40, 120, 30, platformColor);
    
    // Plataforma do jogador (mais baixa)
    DrawEllipse(GetScreenWidth() - GetScreenWidth()/4, GetScreenHeight()/2 + 40, 120, 30, platformColor);
    
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
        
        // Caixa de mensagem estilo Pokémon (parte inferior)
        Rectangle messageBox = { 20, GetScreenHeight() - 180, GetScreenWidth() - 40, 50 };
        DrawRectangleRounded(messageBox, 0.2f, 10, (Color){ 240, 240, 240, 250 });
        DrawRectangleRoundedLines(messageBox, 0.2f, 10, BLACK);
        DrawText(getBattleDescription(), messageBox.x + 15, messageBox.y + 15, 20, BLACK);
        
        // Desenhar menu de batalha estilo Pokémon
        switch (battleSystem->battleState) {
            case BATTLE_SELECT_ACTION:
                // Mostrar opções de ação se for o turno do jogador
                if (battleSystem->playerTurn) {
                    if (isMonsterFainted(battleSystem->playerTeam->current)) {
                        // Forçar o estado de troca
                        battleSystem->battleState = BATTLE_FORCED_SWITCH;
                        return;
                    }
                    
                    // Caixa de menu principal (estilo Fire Red)
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
                    
                    DrawText("Aguardando ação", waitBox.x + 20, waitBox.y + 60, 24, BLACK);
                    DrawText("do oponente...", waitBox.x + 20, waitBox.y + 90, 24, BLACK);
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
                            
                            // Passar o turno para o bot escolher
                            battleSystem->playerTurn = false;
                            battleSystem->battleState = BATTLE_SELECT_ACTION;
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
                
            case BATTLE_ITEM_MENU:
                // Confirmar uso do item (estilo Pokémon)
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
                
            case BATTLE_EXECUTING_ACTIONS:
                // Mostrar mensagem de execução
                Rectangle executingArea = { 20, GetScreenHeight() - 180, GetScreenWidth() - 40, 160 };
                DrawRectangleRounded(executingArea, 0.2f, 10, (Color){ 240, 240, 240, 250 });
                DrawRectangleRoundedLines(executingArea, 0.2f, 10, BLACK);
                
                // Mensagem simples
                DrawText("Executando ações...", executingArea.x + 20, executingArea.y + 70, 24, DARKGRAY);
                break;
                
            case BATTLE_RESULT_MESSAGE:
                // Exibir mensagem do resultado da ação e aguardar confirmação
                Rectangle messageArea = { 20, GetScreenHeight() - 180, GetScreenWidth() - 40, 160 };
                DrawRectangleRounded(messageArea, 0.2f, 10, (Color){ 240, 240, 240, 250 });
                DrawRectangleRoundedLines(messageArea, 0.2f, 10, BLACK);
                
                // Mostrar a mensagem do resultado
                DrawText(getBattleDescription(), messageArea.x + 20, messageArea.y + 30, 22, BLACK);
                
                // Indicador para continuar (pisca para chamar atenção)
                static float blinkTimer = 0.0f;
                blinkTimer += GetFrameTime() * 3.0f;
                
                if (sinf(blinkTimer) > 0.0f) {
                    DrawText("Clique para continuar...", 
                            messageArea.x + messageArea.width - 250, 
                            messageArea.y + messageArea.height - 40, 
                            20, DARKGRAY);
                }
                
                // Verificar interação do usuário para continuar
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || 
                    IsKeyPressed(KEY_SPACE) || 
                    IsKeyPressed(KEY_ENTER)) {
                    
                    // Verificar se ainda há ações na fila
                    if (!isQueueEmpty(battleSystem->actionQueue)) {
                        // Ainda há ações para executar
                        battleSystem->battleState = BATTLE_EXECUTING_ACTIONS;
                    } else {
                        // Todas as ações foram executadas, verificar estado do jogo
                        if (isBattleOver()) {
                            battleSystem->battleState = BATTLE_OVER;
                        } else {
                            // Configurar para próximo turno - CRUCIAL para continuar o jogo
                            battleSystem->battleState = BATTLE_SELECT_ACTION;
                        }
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

    drawBattleEffects();
    
    // Indicador de IA
    drawAIIndicator();
}
 
 // Função para atualizar a lógica da batalha
void updateBattleScreen(void) {
    
    if (battleSystem != NULL) {
        // Atualizar música da batalha
        UpdateMusicStream(battleMusic);
        
        // Se for a vez do bot escolher
        if (battleSystem->battleState == BATTLE_SELECT_ACTION && !battleSystem->playerTurn) {
            // Dar um pequeno delay para parecer mais natural (efeito "pensando")
            static int botDelay = 0;
            botDelay++;
            
            if (botDelay > 45) { // ~0.75 segundo a 60 FPS
                // VERIFICAR se há ação do jogador na fila antes de continuar
                if (!isQueueEmpty(battleSystem->actionQueue)) {
                    // Bot escolhe sua ação
                    botChooseAction();
                    
                    // Ordenar ações por velocidade
                    determineAndExecuteTurnOrder();
                    
                    // Começar executando as ações
                    battleSystem->battleState = BATTLE_EXECUTING_ACTIONS;
                } else {
                    // Se não há ação do jogador enfileirada, algo deu errado
                    // Resetar para o estado inicial
                    battleSystem->playerTurn = true;
                    battleSystem->battleState = BATTLE_SELECT_ACTION;
                }
                
                botDelay = 0;
            }
        }
        
        // Atualizar a lógica da batalha
        updateBattle();
        
        // Verificar se deve tocar efeitos sonoros baseados na batalha
        static int lastHP[2] = { -1, -1 };
        
        // Verificar o HP do jogador para efeitos sonoros
        if (battleSystem->playerTeam && battleSystem->playerTeam->current) {
            int currentHP = battleSystem->playerTeam->current->hp;
            if (lastHP[0] != -1 && currentHP < lastHP[0]) {
                // Tomou dano
                PlaySound(hitSound);
            }
            lastHP[0] = currentHP;
            
            // Verificar se desmaiou
            if (currentHP <= 0 && lastHP[0] > 0) {
                // Desmaiou
                PlaySound(faintSound);
            }
            lastHP[0] = currentHP;
        }
        
        // Verificar o HP do oponente para efeitos sonoros
        if (battleSystem->opponentTeam && battleSystem->opponentTeam->current) {
            int currentHP = battleSystem->opponentTeam->current->hp;
            if (lastHP[1] != -1 && currentHP < lastHP[1]) {
                // Tomou dano
                PlaySound(hitSound);
            }
            
            // Verificar se desmaiou
            if (currentHP <= 0 && lastHP[1] > 0) {
                // Desmaiou
                PlaySound(faintSound);
            }
            lastHP[1] = currentHP;
        }
        
        // Verificar transições de estado para efeitos sonoros
        static int lastBattleState = -1;
        
        if (lastBattleState != battleSystem->battleState) {
            // Tocar sons para certas transições
            switch (battleSystem->battleState) {
                case BATTLE_SELECT_ATTACK:
                case BATTLE_SELECT_MONSTER:
                case BATTLE_ITEM_MENU:
                    PlaySound(selectSound);
                    break;
                case BATTLE_EXECUTING_ACTIONS:
                    // Som de início de execução
                    if (lastBattleState == BATTLE_SELECT_ACTION && !isQueueEmpty(battleSystem->actionQueue)) {
                        // Verificar o tipo de ação na fila
                        int action, parameter;
                        PokeMonster* monster;
                        
                        // Só espiamos a ação sem removê-la
                        if (battleSystem->actionQueue->count > 0) {
                            action = battleSystem->actionQueue->actions[battleSystem->actionQueue->front];
                            
                            if (action == 0) { // Ataque
                                PlaySound(attackSound);
                            }
                        }
                    }
                    break;
                case BATTLE_OVER:
                    // Som de vitória ou derrota
                    int winner = getBattleWinner();
                    if (winner == 1) {
                        // Jogador venceu - som de vitória
                        // (Aqui você pode adicionar um som específico para vitória)
                    } else if (winner == 2) {
                        // Jogador perdeu - som de derrota
                        // (Aqui você pode adicionar um som específico para derrota)
                    }
                    break;
            }
            
            lastBattleState = battleSystem->battleState;
        }
        
        // Efeitos de transição quando um monstro é trocado
        static PokeMonster* lastPlayerMonster = NULL;
        static PokeMonster* lastOpponentMonster = NULL;
        
        if (battleSystem->playerTeam && battleSystem->playerTeam->current) {
            if (lastPlayerMonster != battleSystem->playerTeam->current) {
                // Monstro do jogador foi trocado
                PlaySound(selectSound);
                lastPlayerMonster = battleSystem->playerTeam->current;
            }
        }
        
        if (battleSystem->opponentTeam && battleSystem->opponentTeam->current) {
            if (lastOpponentMonster != battleSystem->opponentTeam->current) {
                // Monstro do oponente foi trocado
                PlaySound(selectSound);
                lastOpponentMonster = battleSystem->opponentTeam->current;
            }
        }
        
        // Tratamento especial para o estado de execução de ações
        if (battleSystem->battleState == BATTLE_EXECUTING_ACTIONS) {
            static int executionDelay = 0;
            executionDelay++;
    
            if (executionDelay > 60) { // Aumentado de 30 para 60 (~1 segundo)
                updateBattle();
                executionDelay = 0;
                }
        }
        static int afterAttackDelay = 0;
        if (battleSystem->battleState == BATTLE_RESULT_MESSAGE) {
            afterAttackDelay++;
            if (afterAttackDelay < 60) { // Espera 1 segundo antes de permitir continuar
                return;
            }
} else {
    afterAttackDelay = 0;
}
    }
    updateBattleEffects();
}
 
 // Tabela de tipos
 void drawTypesTable(void) {
     // Desenhar fundo
     ClearBackground(RAYWHITE);
     
     // Desenhar título
     const char* title = "Tabela de Tipos";
     Vector2 titleSize = MeasureTextEx(gameFont, title, 40, 2);
     DrawTextEx(gameFont, title, (Vector2){ GetScreenWidth()/2 - titleSize.x/2, 20 }, 40, 2, BLACK);
     
     // Desenhar tabela de efetividade de tipos
     int cellSize = 50;
     int startX = (GetScreenWidth() - (cellSize * (TYPE_COUNT + 1))) / 2;
     int startY = 80;
     
     // Desenhar cabeçalhos de coluna (tipos atacantes)
     for (int i = 0; i < TYPE_COUNT; i++) {
         Rectangle cell = { startX + (i + 1) * cellSize, startY, cellSize, cellSize };
         DrawRectangleRec(cell, getTypeColor(i));
         DrawText(getTypeName(i), cell.x + 5, cell.y + 15, 10, WHITE);
     }
     
     // Desenhar cabeçalhos de linha (tipos defensores)
     for (int i = 0; i < TYPE_COUNT; i++) {
         Rectangle cell = { startX, startY + (i + 1) * cellSize, cellSize, cellSize };
         DrawRectangleRec(cell, getTypeColor(i));
         DrawText(getTypeName(i), cell.x + 5, cell.y + 15, 10, WHITE);
     }
     
     // Desenhar células de efetividade
     for (int i = 0; i < TYPE_COUNT; i++) {
         for (int j = 0; j < TYPE_COUNT; j++) {
             Rectangle cell = { 
                 startX + (j + 1) * cellSize, 
                 startY + (i + 1) * cellSize, 
                 cellSize, 
                 cellSize 
             };
             
             float effectiveness = typeEffectiveness[j][i];
             Color cellColor;
             
             if (effectiveness > 1.5f) {
                 cellColor = GREEN;
             } else if (effectiveness < 0.5f) {
                 cellColor = RED;
             } else if (effectiveness == 0.0f) {
                 cellColor = BLACK;
             } else {
                 cellColor = LIGHTGRAY;
             }
             
             DrawRectangleRec(cell, cellColor);
             
             char effText[10];
             if (effectiveness == 0.0f) {
                 strcpy(effText, "0");
             } else {
                 sprintf(effText, "%.1fx", effectiveness);
             }
             
             DrawText(effText, cell.x + cell.width/2 - MeasureText(effText, 20)/2, 
                     cell.y + cell.height/2 - 10, 20, WHITE);
         }
     }
     
     // Legenda
     DrawText("Efetividade:", startX, startY + (TYPE_COUNT + 1) * cellSize + 20, 20, BLACK);
     
     DrawRectangle(startX, startY + (TYPE_COUNT + 1) * cellSize + 50, 20, 20, GREEN);
     DrawText("Super efetivo (>1.5x)", startX + 30, startY + (TYPE_COUNT + 1) * cellSize + 50, 20, BLACK);
     
     DrawRectangle(startX, startY + (TYPE_COUNT + 1) * cellSize + 80, 20, 20, LIGHTGRAY);
     DrawText("Normal (0.5x - 1.5x)", startX + 30, startY + (TYPE_COUNT + 1) * cellSize + 80, 20, BLACK);
     
     DrawRectangle(startX, startY + (TYPE_COUNT + 1) * cellSize + 110, 20, 20, RED);
     DrawText("Pouco efetivo (<0.5x)", startX + 30, startY + (TYPE_COUNT + 1) * cellSize + 110, 20, BLACK);
     
     DrawRectangle(startX, startY + (TYPE_COUNT + 1) * cellSize + 140, 20, 20, BLACK);
     DrawText("Sem efeito (0x)", startX + 30, startY + (TYPE_COUNT + 1) * cellSize + 140, 20, BLACK);
     
     // Botão de voltar
     if (drawButton((Rectangle){ 20, GetScreenHeight() - 70, 150, 50 }, "Voltar", BLUE)) {
         PlaySound(selectSound);
         currentScreen = MAIN_MENU;
     }
 }
 
 void updateTypesTable(void) {
     // Atualização da lógica da tabela de tipos, se necessário
 }
 
 // Configurações
 void drawSettings(void) {
     // Desenhar fundo
     ClearBackground(DARKBLUE);
     
     // Desenhar título
     const char* title = "Configurações";
     Vector2 titleSize = MeasureTextEx(gameFont, title, 40, 2);
     DrawTextEx(gameFont, title, (Vector2){ GetScreenWidth()/2 - titleSize.x/2, 50 }, 40, 2, WHITE);
     
     // Área de configurações
     Rectangle settingsArea = { GetScreenWidth()/2 - 200, 120, 400, 300 };
     DrawRectangleRounded(settingsArea, 0.2f, 10, LIGHTGRAY);
     
     // Volume da música
     DrawText("Volume da Música", settingsArea.x + 20, settingsArea.y + 30, 20, BLACK);
     
     // Slider para volume da música
     Rectangle musicSlider = { settingsArea.x + 20, settingsArea.y + 60, 360, 30 };
     DrawRectangleRec(musicSlider, DARKGRAY);
     DrawRectangleRec((Rectangle){ musicSlider.x, musicSlider.y, musicSlider.width * musicVolume, musicSlider.height }, GREEN);
     
     // Verificar interação com o slider
     if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
         Vector2 mousePos = GetMousePosition();
         if (CheckCollisionPointRec(mousePos, musicSlider)) {
             musicVolume = (mousePos.x - musicSlider.x) / musicSlider.width;
             
             if (musicVolume < 0.0f) musicVolume = 0.0f;
             if (musicVolume > 1.0f) musicVolume = 1.0f;
             
             // Aplicar volume
             SetMusicVolume(menuMusic, musicVolume);
             SetMusicVolume(battleMusic, musicVolume);
         }
     }
     
     // Volume dos efeitos sonoros
     DrawText("Volume dos Efeitos", settingsArea.x + 20, settingsArea.y + 110, 20, BLACK);
     
     // Slider para volume dos efeitos
     Rectangle soundSlider = { settingsArea.x + 20, settingsArea.y + 140, 360, 30 };
     DrawRectangleRec(soundSlider, DARKGRAY);
     DrawRectangleRec((Rectangle){ soundSlider.x, soundSlider.y, soundSlider.width * soundVolume, soundSlider.height }, BLUE);
     
     // Verificar interação com o slider
     if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
         Vector2 mousePos = GetMousePosition();
         if (CheckCollisionPointRec(mousePos, soundSlider)) {
             soundVolume = (mousePos.x - soundSlider.x) / soundSlider.width;
             
             if (soundVolume < 0.0f) soundVolume = 0.0f;
             if (soundVolume > 1.0f) soundVolume = 1.0f;
             
             // Aplicar volume
             SetSoundVolume(selectSound, soundVolume);
             SetSoundVolume(attackSound, soundVolume);
             SetSoundVolume(hitSound, soundVolume);
             SetSoundVolume(faintSound, soundVolume);
         }
     }
     
     // Opção de tela cheia
     DrawText("Modo Tela Cheia", settingsArea.x + 20, settingsArea.y + 190, 20, BLACK);
     
     // Botão de alternar tela cheia
     Rectangle fullscreenButton = { settingsArea.x + 200, settingsArea.y + 190, 180, 30 };
     if (drawButton(fullscreenButton, fullscreen ? "Desativar" : "Ativar", fullscreen ? RED : GREEN)) {
         PlaySound(selectSound);
         fullscreen = !fullscreen;
         ToggleFullscreen();
     }
     
     // Botão de voltar
     if (drawButton((Rectangle){ settingsArea.x + 125, settingsArea.y + 240, 150, 40 }, "Salvar e Voltar", BLUE)) {
         PlaySound(selectSound);
         currentScreen = MAIN_MENU;
     }
 }
 
 void updateSettings(void) {
     // Atualização da lógica de configurações, se necessário
 }
 
 // Créditos
 void drawCredits(void) {
     // Desenhar fundo
     ClearBackground(BLACK);
     
     // Desenhar título
     const char* title = "Créditos";
     Vector2 titleSize = MeasureTextEx(gameFont, title, 40, 2);
     DrawTextEx(gameFont, title, (Vector2){ GetScreenWidth()/2 - titleSize.x/2, 50 }, 40, 2, WHITE);
     
     // Texto dos créditos
     const char* credits[] = {
         "PokeBattle",
         "Um jogo inspirado em Pokémon Stadium",
         "",
         "Desenvolvido por:",
         "Julia Torres, Fatima Beatriz, Maria Claudia, Matheus Martins, Vinicius Jose - 2025",
         "",
         "Projeto para a disciplina de",
         "Algoritmos e Estruturas de Dados",
         "",
         "Agradecimentos:",
         "A todos os professores e colegas",
         "que tornaram este projeto possível.",
         "",
         "Recursos utilizados:",
         "Raylib - Biblioteca gráfica",
         "libcurl - Integração com API",
         "Gemini AI - API de IA para comportamento do bot"
     };
     
     int creditCount = sizeof(credits) / sizeof(credits[0]);
     int startY = 120;
     
     for (int i = 0; i < creditCount; i++) {
         int fontSize = (i == 0) ? 30 : 20;
         Color textColor = (i == 0 || i == 3 || i == 9 || i == 13) ? GOLD : WHITE;
         
         Vector2 textSize = MeasureTextEx(gameFont, credits[i], fontSize, 1);
         DrawTextEx(gameFont, credits[i], (Vector2){ GetScreenWidth()/2 - textSize.x/2, startY }, fontSize, 1, textColor);
         
         startY += fontSize + 10;
     }
     
     // Botão de voltar
     if (drawButton((Rectangle){ GetScreenWidth()/2 - 75, GetScreenHeight() - 70, 150, 50 }, "Voltar", BLUE)) {
         PlaySound(selectSound);
         currentScreen = MAIN_MENU;
     }
 }
 
 void updateCredits(void) {
     // Atualização da lógica dos créditos, se necessário
 }
 
 // Funções auxiliares de desenho
 
 // Desenha um botão e retorna true se foi clicado
 bool drawButton(Rectangle bounds, const char* text, Color color) {
     bool clicked = false;
     Vector2 mousePoint = GetMousePosition();
     
     // Verificar hover
     if (CheckCollisionPointRec(mousePoint, bounds)) {
         // Clarear a cor quando hover
         Color hoverColor = (Color){ 
             (unsigned char)fmin(255, color.r + 40),
             (unsigned char)fmin(255, color.g + 40),
             (unsigned char)fmin(255, color.b + 40),
             color.a
         };
         
         DrawRectangleRounded(bounds, 0.2f, 10, hoverColor);
         
         // Verificar clique
         if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
             clicked = true;
         }
     } else {
         DrawRectangleRounded(bounds, 0.2f, 10, color);
     }
     
     // Desenhar borda
     DrawRectangleRoundedLines(bounds, 0.2f, 10, BLACK);
     
     // Desenhar texto
     Vector2 textSize = MeasureTextEx(gameFont, text, 20, 1);
     DrawTextEx(gameFont, text, (Vector2){ 
         bounds.x + bounds.width/2 - textSize.x/2,
         bounds.y + bounds.height/2 - textSize.y/2
     }, 20, 1, WHITE);
     
     return clicked;
 }
 
 // Desenha um card de monstro
 void drawMonsterCard(Rectangle bounds, PokeMonster* monster, bool selected) {
     // Desenhar fundo do card
     Color cardColor = selected ? SKYBLUE : LIGHTGRAY;
     DrawRectangleRounded(bounds, 0.2f, 10, cardColor);
     
     // Desenhar borda
     DrawRectangleRoundedLines(bounds, 0.2f, 10, selected ? BLUE : BLACK);
     
     // Desenhar nome
     DrawText(monster->name, bounds.x + 10, bounds.y + 10, 20, BLACK);
     
     // Desenhar tipos
     Rectangle type1Rect = { bounds.x + 10, bounds.y + 40, 60, 25 };
     DrawRectangleRec(type1Rect, getTypeColor(monster->type1));
     DrawText(getTypeName(monster->type1), type1Rect.x + 5, type1Rect.y + 5, 15, WHITE);
     
     if (monster->type2 != TYPE_NONE) {
         Rectangle type2Rect = { bounds.x + 80, bounds.y + 40, 60, 25 };
         DrawRectangleRec(type2Rect, getTypeColor(monster->type2));
         DrawText(getTypeName(monster->type2), type2Rect.x + 5, type2Rect.y + 5, 15, WHITE);
     }
     
     // Desenhar estatísticas básicas
     char stats[100];
     sprintf(stats, "HP: %d  ATK: %d", monster->maxHp, monster->attack);
     DrawText(stats, bounds.x + 10, bounds.y + 70, 15, BLACK);
     
     sprintf(stats, "DEF: %d  SPD: %d", monster->defense, monster->speed);
     DrawText(stats, bounds.x + 10, bounds.y + 90, 15, BLACK);
 }
 
 // Desenha ícone de tipo
 void drawTypeIcon(Rectangle bounds, MonsterType type) {
     DrawRectangleRec(bounds, getTypeColor(type));
     DrawText(getTypeName(type), bounds.x + 5, bounds.y + 5, 15, WHITE);
 }
 // Estrutura para armazenar o estado de animação da barra de vida
typedef struct {
    void* monsterId;          // ID único para o monstro (endereço de memória)
    float animatedFillRatio;  // Valor atual da animação
    int lastHP;               // Último valor de HP registrado
    float timer;              // Timer para animação
} HPBarAnimation;

// Array para armazenar animações de múltiplas barras de HP
#define MAX_BARS 10
static HPBarAnimation hpBars[MAX_BARS] = {0};

// Encontrar ou criar uma animação para um monstro específico
HPBarAnimation* getHPBarAnimation(PokeMonster* monster) {
    if (monster == NULL) return NULL;
    
    // Usar endereço do monstro como ID
    void* monsterId = (void*)monster;
    
    // Procurar animação existente
    for (int i = 0; i < MAX_BARS; i++) {
        if (hpBars[i].monsterId == monsterId) {
            return &hpBars[i];
        }
    }
    
    // Criar nova animação
    for (int i = 0; i < MAX_BARS; i++) {
        if (hpBars[i].monsterId == NULL) {
            hpBars[i].monsterId = monsterId;
            hpBars[i].animatedFillRatio = (float)monster->hp / monster->maxHp;
            hpBars[i].lastHP = monster->hp;
            hpBars[i].timer = 0;
            return &hpBars[i];
        }
    }
    
    return NULL; // Não deveria acontecer a menos que MAX_BARS seja muito pequeno
}

// Limpar animações antigas (chame isto quando iniciar uma nova batalha)
void clearHPBarAnimations(void) {
    for (int i = 0; i < MAX_BARS; i++) {
        hpBars[i].monsterId = NULL;
        hpBars[i].animatedFillRatio = 0;
        hpBars[i].lastHP = 0;
        hpBars[i].timer = 0;
    }
}

void drawHealthBar(Rectangle bounds, int currentHP, int maxHP, PokeMonster* monster) {
    static bool battleStarted = false;
    if (!battleStarted && battleSystem != NULL) {
        clearHPBarAnimations();
        battleStarted = true;
    }
    // Obter o estado de animação para este monstro específico
    HPBarAnimation* anim = getHPBarAnimation(monster);
    if (anim == NULL) {
        // Fallback caso não consiga obter a animação
        float fillRatio = (float)currentHP / maxHP;
        if (fillRatio < 0) fillRatio = 0;
        if (fillRatio > 1) fillRatio = 1;
        
        DrawRectangleRec(bounds, BLACK);
        Rectangle innerBounds = {
            bounds.x + 1,
            bounds.y + 1,
            bounds.width - 2,
            bounds.height - 2
        };
        DrawRectangleRec(innerBounds, WHITE);
        
        Color fillColor = fillRatio > 0.5f ? GREEN : (fillRatio > 0.2f ? YELLOW : RED);
        DrawRectangleRec((Rectangle){ 
            innerBounds.x, innerBounds.y, 
            innerBounds.width * fillRatio, innerBounds.height 
        }, fillColor);
        return;
    }
    
    // Calcular preenchimento baseado no HP atual
    float targetFillRatio = (float)currentHP / maxHP;
    if (targetFillRatio < 0) targetFillRatio = 0;
    if (targetFillRatio > 1) targetFillRatio = 1;
    
    // Detectar mudança de HP
    if (anim->lastHP != currentHP) {
        anim->lastHP = currentHP;
        anim->timer = 0;  // Resetar timer quando o HP muda
    }
    
    // Avançar o timer
    anim->timer += GetFrameTime();
    
    // Animação mais lenta e suave (0.5 unidades por segundo)
    float animationSpeed = 0.5f;
    
    // Animação suave da barra
    float delta = targetFillRatio - anim->animatedFillRatio;
    if (fabs(delta) > 0.001f) {
        anim->animatedFillRatio += delta * animationSpeed * GetFrameTime() * 4.0f;
        
        // Garantir que não ultrapasse o objetivo
        if (delta > 0 && anim->animatedFillRatio > targetFillRatio) {
            anim->animatedFillRatio = targetFillRatio;
        } else if (delta < 0 && anim->animatedFillRatio < targetFillRatio) {
            anim->animatedFillRatio = targetFillRatio;
        }
    }
    
    // Desenhar fundo da barra (borda preta)
    DrawRectangleRec(bounds, BLACK);
    
    // Borda interna (espaço para o preenchimento)
    Rectangle innerBounds = {
        bounds.x + 1,
        bounds.y + 1,
        bounds.width - 2,
        bounds.height - 2
    };
    DrawRectangleRec(innerBounds, WHITE);
    
    // Determinar cor baseada no HP - estilo Pokémon
    Color fillColor;
    if (anim->animatedFillRatio > 0.5f) {
        fillColor = (Color){ 0, 200, 80, 255 }; // Verde
    } else if (anim->animatedFillRatio > 0.2f) {
        fillColor = (Color){ 255, 180, 0, 255 }; // Amarelo
    } else {
        fillColor = (Color){ 200, 0, 0, 255 }; // Vermelho
    }
    
    // Desenhar preenchimento com animação
    DrawRectangleRec((Rectangle){ 
        innerBounds.x, innerBounds.y, 
        innerBounds.width * anim->animatedFillRatio, innerBounds.height 
    }, fillColor);
    
    // Adicionar efeito de gradiente sutil na barra de HP
    for (int i = 0; i < innerBounds.height; i += 2) {
        DrawLine(
            innerBounds.x, 
            innerBounds.y + i,
            innerBounds.x + innerBounds.width * anim->animatedFillRatio,
            innerBounds.y + i,
            (Color){ 
                (unsigned char)fmin(255, fillColor.r + 40),
                (unsigned char)fmin(255, fillColor.g + 40),
                (unsigned char)fmin(255, fillColor.b + 40),
                fillColor.a
            }
        );
    }
    
    // Adicionar efeito de "piscada" quando o HP estiver muito baixo
    if (anim->animatedFillRatio <= 0.2f) {
        float blinkRate = 3.0f;  // Frequência de piscada mais lenta
        
        if (sinf(anim->timer * blinkRate) > 0.0f) {
            DrawRectangleRec(
                (Rectangle){ 
                    innerBounds.x, innerBounds.y, 
                    innerBounds.width * anim->animatedFillRatio, innerBounds.height 
                }, 
                (Color){ 255, 255, 255, 100 }
            );
        }
    }
}
 
 // Desenha estatísticas detalhadas de um monstro
 void drawMonsterStats(Rectangle bounds, PokeMonster* monster) {
     // Desenhar fundo
     DrawRectangleRounded(bounds, 0.2f, 10, LIGHTGRAY);
     
     // Desenhar nome
     DrawText(monster->name, bounds.x + 20, bounds.y + 20, 30, BLACK);
     
     // Desenhar tipos
     Rectangle type1Rect = { bounds.x + 20, bounds.y + 60, 80, 30 };
     DrawRectangleRec(type1Rect, getTypeColor(monster->type1));
     DrawText(getTypeName(monster->type1), type1Rect.x + 10, type1Rect.y + 5, 20, WHITE);
     
     if (monster->type2 != TYPE_NONE) {
         Rectangle type2Rect = { bounds.x + 110, bounds.y + 60, 80, 30 };
         DrawRectangleRec(type2Rect, getTypeColor(monster->type2));
         DrawText(getTypeName(monster->type2), type2Rect.x + 10, type2Rect.y + 5, 20, WHITE);
     }
     
     // Desenhar estatísticas
     DrawText("Estatísticas:", bounds.x + 20, bounds.y + 100, 25, BLACK);
     
     char statText[50];
     sprintf(statText, "HP: %d", monster->maxHp);
     DrawText(statText, bounds.x + 30, bounds.y + 130, 20, BLACK);
     
     sprintf(statText, "Ataque: %d", monster->attack);
     DrawText(statText, bounds.x + 30, bounds.y + 160, 20, BLACK);
     
     sprintf(statText, "Defesa: %d", monster->defense);
     DrawText(statText, bounds.x + 30, bounds.y + 190, 20, BLACK);
     
     sprintf(statText, "Velocidade: %d", monster->speed);
     DrawText(statText, bounds.x + 30, bounds.y + 220, 20, BLACK);
     
     // Desenhar ataques
     DrawText("Ataques:", bounds.x + bounds.width/2, bounds.y + 100, 25, BLACK);
     
     for (int i = 0; i < 4; i++) {
         Rectangle attackRect = { 
             bounds.x + bounds.width/2, 
             bounds.y + 130 + i * 40, 
             bounds.width/2 - 30, 
             35 
         };
         
         DrawRectangleRec(attackRect, getTypeColor(monster->attacks[i].type));
         DrawText(monster->attacks[i].name, attackRect.x + 10, attackRect.y + 8, 20, WHITE);
         
         char attackInfo[50];
         if (monster->attacks[i].power > 0) {
             sprintf(attackInfo, "Poder: %d  Precisão: %d", 
                    monster->attacks[i].power, monster->attacks[i].accuracy);
         } else {
             sprintf(attackInfo, "Status  Precisão: %d", monster->attacks[i].accuracy);
         }
         
         DrawText(attackInfo, attackRect.x + 10, attackRect.y + 30, 15, BLACK);
     }
 }
 
 // Desenha lista de ataques
 void drawAttackList(Rectangle bounds, PokeMonster* monster, int selectedAttack) {
    int attackWidth = (bounds.width - 130) / 4;
    
    for (int i = 0; i < 4; i++) {
        Rectangle attackBounds = {
            bounds.x + 10 + i * (attackWidth + 10),
            bounds.y + 10,
            attackWidth,
            60
        };
        
        Color attackColor = getTypeColor(monster->attacks[i].type);
        
        if (monster->attacks[i].ppCurrent <= 0) {
            attackColor.r = attackColor.r / 2;
            attackColor.g = attackColor.g / 2;
            attackColor.b = attackColor.b / 2;
        }
        
        if (monster->attacks[i].ppCurrent > 0 && drawButton(attackBounds, monster->attacks[i].name, attackColor)) {
            PlaySound(selectSound);
            battleSystem->selectedAttack = i;
            
            // Enfileirar ação de ataque
            enqueue(battleSystem->actionQueue, 0, i, battleSystem->playerTeam->current);
            
            // Passar o turno para o bot escolher
            battleSystem->playerTurn = false;
            battleSystem->battleState = BATTLE_SELECT_ACTION;
        } else if (monster->attacks[i].ppCurrent <= 0) {
            DrawRectangleRounded(attackBounds, 0.2f, 10, attackColor);
            DrawRectangleRoundedLines(attackBounds, 0.2f, 10, BLACK);
            
            Vector2 textSize = MeasureTextEx(gameFont, monster->attacks[i].name, 20, 1);
            DrawTextEx(gameFont, monster->attacks[i].name, (Vector2){ 
                attackBounds.x + attackBounds.width/2 - textSize.x/2,
                attackBounds.y + 10
            }, 20, 1, WHITE);
            
            char ppText[20];
            sprintf(ppText, "PP: %d/%d", monster->attacks[i].ppCurrent, monster->attacks[i].ppMax);
            DrawText(ppText, attackBounds.x + 10, attackBounds.y + 35, 15, WHITE);
        } else {
            char ppText[20];
            sprintf(ppText, "PP: %d/%d", monster->attacks[i].ppCurrent, monster->attacks[i].ppMax);
            DrawText(ppText, attackBounds.x + 10, attackBounds.y + 35, 15, WHITE);
        }
    }
}
 
 // Desenha caixa de mensagem
 void drawMessageBox(Rectangle bounds, const char* message) {
     // Desenhar fundo
     DrawRectangleRounded(bounds, 0.2f, 10, WHITE);
     DrawRectangleRoundedLines(bounds, 0.2f, 10, BLACK);
     
     // Desenhar texto
     DrawText(message, bounds.x + 15, bounds.y + bounds.height/2 - 10, 20, BLACK);
 }
 
 // Desenha diálogo de confirmação
 void drawConfirmationDialog(const char* message, const char* yesText, const char* noText) {
     // Desenhar fundo semi-transparente
     DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){ 0, 0, 0, 150 });
     
     // Desenhar caixa de diálogo
     Rectangle dialogBox = {
         GetScreenWidth()/2 - 200,
         GetScreenHeight()/2 - 100,
         400,
         200
     };
     
     DrawRectangleRounded(dialogBox, 0.2f, 10, WHITE);
     DrawRectangleRoundedLines(dialogBox, 0.2f, 10, BLACK);
     
     // Desenhar mensagem
     Vector2 textSize = MeasureTextEx(gameFont, message, 20, 1);
     DrawTextEx(gameFont, message, (Vector2){ 
         dialogBox.x + dialogBox.width/2 - textSize.x/2,
         dialogBox.y + 40
     }, 20, 1, BLACK);
     
     // Desenhar botões
     Rectangle yesButton = {
         dialogBox.x + 50,
         dialogBox.y + 120,
         100,
         40
     };
     
     Rectangle noButton = {
         dialogBox.x + 250,
         dialogBox.y + 120,
         100,
         40
     };
     
     if (drawButton(yesButton, yesText, RED)) {
         PlaySound(selectSound);
         // Voltar ao menu principal
         StopMusicStream(battleMusic);
         PlayMusicStream(menuMusic);
         currentScreen = MAIN_MENU;
         resetBattle();
     }
     
     if (drawButton(noButton, noText, GREEN)) {
         PlaySound(selectSound);
         // Continuar a batalha
         battleSystem->battleState = BATTLE_SELECT_ACTION;
     }
 }

// Função auxiliar para desenhar o indicador de IA
void drawAIIndicator(void) {
    Rectangle indicator = { GetScreenWidth() - 80, 5, 75, 25 };
    Color indicatorColor;
    const char* text;
    
    // Verificar se a IA está disponível
    if (initialized && curl_handle != NULL) {
        indicatorColor = GREEN;
        text = "IA ON";
    } else {
        indicatorColor = YELLOW;
        text = "IA OFF";
    }
    
    DrawRectangleRounded(indicator, 0.3f, 8, indicatorColor);
    DrawRectangleRoundedLines(indicator, 0.3f, 8, BLACK);
    DrawText(text, indicator.x + 8, indicator.y + 4, 18, WHITE);
}

// Estrutura para efeitos visuais na batalha
typedef struct {
    bool active;
    float timer;
    float duration;
    int type;
    Rectangle bounds;
    Color color;
    Vector2 origin;
    Vector2 target;
} BattleEffect;

// Variáveis para efeitos
#define MAX_EFFECTS 10
static BattleEffect effects[MAX_EFFECTS] = {0};

// Tipos de efeito
enum {
    EFFECT_NONE = 0,
    EFFECT_FLASH,
    EFFECT_SHAKE,
    EFFECT_PARTICLES,
    EFFECT_SLASH,
    EFFECT_FIRE,
    EFFECT_WATER,
    EFFECT_ELECTRIC,
    EFFECT_NATURE
};

// Inicializar sistema de efeitos
void initBattleEffects(void) {
    for (int i = 0; i < MAX_EFFECTS; i++) {
        effects[i].active = false;
    }
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