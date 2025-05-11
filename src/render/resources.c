#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#endif

#include "resources.h"
#include "structures.h"
#include "monsters.h"
#include <stdio.h>



// Definições das variáveis globais de recursos
Texture2D battleBackgrounds[BATTLE_BACKGROUNDS_COUNT];
int currentBattleBackground = 0;
Font gameFont;
Texture2D backgroundTexture;
Texture2D menuBackground;
Texture2D battleBackground;
Texture2D monsterSelectBackground;
Texture2D typeIcons[TYPE_COUNT];
Sound selectSound;
Sound attackSound;
Sound hitSound;
Sound faintSound;
Music menuMusic;
Music battleMusic;

extern float musicVolume;
extern float soundVolume;

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
    // Carregar backgrounds de batalha
    printf("Carregando backgrounds de batalha...\n");

    for (int i = 0; i < BATTLE_BACKGROUNDS_COUNT; i++) {
        char filename[256];
        sprintf(filename, "resources/bg_%d.png", i);
        battleBackgrounds[i] = LoadTexture(filename);

        if (battleBackgrounds[i].id == 0) {
            printf("Aviso: Falha ao carregar background %s\n", filename);
        } else {
            printf("Background %d carregado com sucesso!\n", i);
        }
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

    // Descarregar backgrounds de batalha
    for (int i = 0; i < BATTLE_BACKGROUNDS_COUNT; i++) {
        if (battleBackgrounds[i].id != 0) {
            UnloadTexture(battleBackgrounds[i]);
        }
    }
}

// Carrega sons e música
void loadSounds(float musicVol, float soundVol) {
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