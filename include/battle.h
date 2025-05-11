// battle.h
#ifndef BATTLE_H
#define BATTLE_H

#include <stdbool.h>

typedef struct PokeMonster PokeMonster;
typedef struct MonsterList MonsterList;
typedef struct Attack Attack;
typedef struct BattleSystem BattleSystem;
extern bool actionQueueReady;




// Estados de batalha
typedef enum {
    BATTLE_IDLE = 0,
    BATTLE_INTRO,
    BATTLE_SELECT_ACTION,
    BATTLE_SELECT_ATTACK,
    BATTLE_SELECT_MONSTER,
    BATTLE_ITEM_MENU,
    BATTLE_PREPARING_ACTIONS,
    BATTLE_EXECUTING_ACTIONS,
    BATTLE_ATTACK_ANIMATION,
    BATTLE_DAMAGE_ANIMATION,
    BATTLE_MESSAGE_DISPLAY,
    BATTLE_RESULT_MESSAGE,
    BATTLE_TURN_END,
    BATTLE_OVER,
    BATTLE_CONFIRM_QUIT,
    BATTLE_FORCED_SWITCH
} BattleState;

// Adicionar estruturas necessárias
typedef struct {
    char message[256];
    float displayTime;
    float elapsedTime;
    bool waitingForInput;
    bool autoAdvance;
} BattleMessage;

typedef struct {
    bool isAnimating;
    float animationTime;
    float elapsedTime;
    int animationType;
    PokeMonster* source;
    PokeMonster* target;
} BattleAnimation;

// Tipos de itens
typedef enum {
    ITEM_POTION = 0,
    ITEM_RED_CARD,
    ITEM_COIN
} ItemType;

// Tipos de status
typedef enum {
    STATUS_NONE = 0,
    STATUS_ATK_DOWN = 1,
    STATUS_DEF_DOWN = 2,
    STATUS_SPD_DOWN = 3,
    STATUS_PARALYZED = 4,
    STATUS_SLEEPING = 5,
    STATUS_BURNING = 6
} StatusType;

// Protótipos das funções existentes...
void initializeBattleSystem(void);
void freeBattleSystem(void);
void startNewBattle(MonsterList* playerTeam, MonsterList* opponentTeam);
void updateBattle(void);
void processBattleInput(void);
void executeAttack(PokeMonster* attacker, PokeMonster* defender, int attackIndex);
int calculateDamage(PokeMonster* attacker, PokeMonster* defender, Attack* attack);
void applyStatusEffect(PokeMonster* target, int statusEffect, int statusPower, int duration);
void processTurnEnd(void);
bool isBattleOver(void);
int getBattleWinner(void);
void determineTurnOrder(void);
ItemType rollRandomItem(void);
void useItem(ItemType itemType, PokeMonster* target);
bool isMonsterFainted(PokeMonster* monster);
void switchMonster(MonsterList* team, PokeMonster* newMonster);
void botChooseAction(void);
int botChooseAttack(PokeMonster* botMonster, PokeMonster* playerMonster);
PokeMonster* botChooseMonster(MonsterList* botTeam, PokeMonster* playerMonster);
const char* getBattleDescription(void);
void resetBattle(void);
int getAISuggestedActionSimple(PokeMonster* botMonster, PokeMonster* playerMonster);
bool canAttack(PokeMonster* monster);
void processStatusEffects(PokeMonster* monster);
void displayStatusMessage(const char* message);
void determineAndExecuteTurnOrder(void);
void messageDisplayComplete(void);
#endif // BATTLE_H