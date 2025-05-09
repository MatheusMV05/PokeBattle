/**
 * PokeBattle - Sistema de batalha
 * 
 * Este arquivo contém as declarações de funções para o sistema de batalha.
 */

 #ifndef BATTLE_H
 #define BATTLE_H
 
 #include "structures.h"
 
 // Estados de batalha
 typedef enum {
     BATTLE_IDLE = 0,
     BATTLE_SELECT_ACTION,
     BATTLE_SELECT_ATTACK,
     BATTLE_SELECT_MONSTER,
     BATTLE_ITEM_MENU,
     BATTLE_EXECUTING_ACTIONS,
     BATTLE_ANIMATION,
     BATTLE_RESULT_MESSAGE,
     BATTLE_OVER,
     BATTLE_CONFIRM_QUIT,
     BATTLE_FORCED_SWITCH 
 } BattleState;
 
 // Tipos de itens
 typedef enum {
     ITEM_POTION = 0,      // Restaura 20 de HP
     ITEM_RED_CARD,        // Força o oponente a trocar de monstro
     ITEM_COIN             // 50% de chance de curar todo HP, 50% de morte instantânea
 } ItemType;

 // Tipos de status
typedef enum {
    STATUS_NONE = 0,
    STATUS_ATK_DOWN = 1,    // Ataque reduzido
    STATUS_DEF_DOWN = 2,    // Defesa reduzida
    STATUS_SPD_DOWN = 3,    // Velocidade reduzida
    STATUS_PARALYZED = 4,   // Paralisado: reduz velocidade e chance de não atacar
    STATUS_SLEEPING = 5,    // Dormindo: não pode atacar
    STATUS_BURNING = 6      // Em chamas: perde HP a cada turno
} StatusType;
 // Protótipos das funções
 
 // Inicializa o sistema de batalha
 void initializeBattleSystem(void);
 
 // Libera o sistema de batalha
 void freeBattleSystem(void);
 
 // Inicia uma nova batalha
 void startNewBattle(MonsterList* playerTeam, MonsterList* opponentTeam);
 
 // Atualiza o estado da batalha
 void updateBattle(void);
 
 // Processa a entrada do jogador durante a batalha
 void processBattleInput(void);
 
 // Executa um ataque
 void executeAttack(PokeMonster* attacker, PokeMonster* defender, int attackIndex);
 
 // Calcula o dano de um ataque
 int calculateDamage(PokeMonster* attacker, PokeMonster* defender, Attack* attack);
 
 // Aplica efeitos de status
 void applyStatusEffect(PokeMonster* target, int statusEffect, int statusPower, int duration);
 
 // Processa o final do turno (efeitos de status, etc.)
 void processTurnEnd(void);
 
 // Verifica se a batalha acabou
 bool isBattleOver(void);
 
 // Determina o vencedor da batalha
 int getBattleWinner(void); // 0 = continue, 1 = jogador venceu, 2 = oponente venceu
 
 // Define o turno baseado na velocidade dos monstros
 void determineTurnOrder(void);
 
 // Sorteia um tipo de item aleatório para a batalha
 ItemType rollRandomItem(void);
 
 // Usa um item
 void useItem(ItemType itemType, PokeMonster* target);
 
 // Verifica se um monstro está incapacitado
 bool isMonsterFainted(PokeMonster* monster);
 
 // Troca o monstro atual por outro
 void switchMonster(MonsterList* team, PokeMonster* newMonster);
 
 // Faz o bot escolher uma ação
 void botChooseAction(void);
 
 // Faz o bot escolher um ataque
 int botChooseAttack(PokeMonster* botMonster, PokeMonster* playerMonster);
 
 // Faz o bot escolher um monstro para troca
 PokeMonster* botChooseMonster(MonsterList* botTeam, PokeMonster* playerMonster);
 
 // Retorna uma descrição do que aconteceu na batalha para exibição
 const char* getBattleDescription(void);
 
 // Reinicia a batalha
 void resetBattle(void);
 
 
 int getAISuggestedActionSimple(PokeMonster* botMonster, PokeMonster* playerMonster);
 
 bool canAttack(PokeMonster* monster);

 void processStatusEffects(PokeMonster* monster);

 void displayStatusMessage(const char* message);

 void determineAndExecuteTurnOrder(void);
 
 #endif // BATTLE_H