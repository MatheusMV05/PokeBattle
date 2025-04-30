/**
 * PokeBattle - Telas do jogo
 * 
 * Este arquivo contém as declarações de funções para as diferentes telas do jogo.
 */

 #ifndef SCREENS_H
 #define SCREENS_H
 
 #include "raylib.h"
 
 // Protótipos das funções para cada tela
 
 // Menu principal
 void drawMainMenu(void);
 void updateMainMenu(void);
 
 // Seleção de adversário
 void drawOpponentSelection(void);
 void updateOpponentSelection(void);
 
 // Seleção de monstros
 void drawMonsterSelection(void);
 void updateMonsterSelection(void);
 
 // Tela de batalha
 void drawBattleScreen(void);
 void updateBattleScreen(void);
 
 // Tabela de tipos
 void drawTypesTable(void);
 void updateTypesTable(void);
 
 // Configurações
 void drawSettings(void);
 void updateSettings(void);
 
 // Créditos
 void drawCredits(void);
 void updateCredits(void);
 
 // Funções auxiliares para a interface
 
 // Carrega texturas, fontes e recursos visuais
 void loadTextures(void);
 void unloadTextures(void);
 
 // Carrega sons e música
 void loadSounds(void);
 void unloadSounds(void);
 
 // Funções para desenhar elementos de interface
 bool drawButton(Rectangle bounds, const char* text, Color color);
 void drawMonsterCard(Rectangle bounds, PokeMonster* monster, bool selected);
 void drawTypeIcon(Rectangle bounds, MonsterType type);
 void drawHealthBar(Rectangle bounds, int currentHP, int maxHP);
 void drawMonsterStats(Rectangle bounds, PokeMonster* monster);
 void drawAttackList(Rectangle bounds, PokeMonster* monster, int selectedAttack);
 void drawMessageBox(Rectangle bounds, const char* message);
 void drawBattleHUD(void);
 void drawConfirmationDialog(const char* message, const char* yesText, const char* noText);
 
 // Funções de transição entre telas
 void fadeToScreen(void (*drawFunc)(void), int frames);
 void slideToScreen(void (*drawFunc)(void), int frames, int direction);
 
 #endif // SCREENS_H