// structures.h
#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <stddef.h>  // Para size_t
#include "raylib.h"

// Forward declarations
typedef struct PokeMonster PokeMonster;

// Definições de estruturas

// Tipos de PokeMonstros
typedef enum {
    TYPE_NONE = -1,
    TYPE_METAL = 0,
    TYPE_FIRE,
    TYPE_WATER,
    TYPE_ELECTRIC,
    TYPE_GRASS,
    TYPE_DRAGON,
    TYPE_GHOST,
    TYPE_FLYING,
    TYPE_FAIRY,
    TYPE_COUNT
} MonsterType;

// Estrutura para os ataques
typedef struct Attack {
    char name[32];
    MonsterType type;
    int power;
    int accuracy;
    int ppMax;
    int ppCurrent;
    int statusEffect;
    int statusPower;
    int statusChance;
} Attack;

// Estrutura para um PokeMonstro
struct PokeMonster {
    char name[32];
    MonsterType type1;
    MonsterType type2;
    int hp;
    int maxHp;
    int attack;
    int defense;
    int speed;
    Attack attacks[4];
    int statusCondition;
    int statusCounter;
    int statusTurns;
    Texture2D texture;
    struct PokeMonster* next;
    struct PokeMonster* prev;
};

// Estrutura para lista duplamente encadeada de monstros
typedef struct MonsterList {
    PokeMonster* first;
    PokeMonster* last;
    PokeMonster* current;
    int count;
} MonsterList;

// Estrutura para armazenar a resposta da API
typedef struct {
    char* buffer;
    size_t size;
} MemoryStruct;

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

// Estrutura para resoluções
typedef struct {
    int width;
    int height;
    const char* description;
} Resolution;

// Estrutura para a fila de ações
typedef struct {
    int* actions;
    int* parameters;
    PokeMonster** monsters;
    int front;
    int rear;
    int capacity;
    int count;
} ActionQueue;

// Estrutura para a pilha de efeitos
typedef struct {
    int* types;
    int* durations;
    int* values;
    PokeMonster** targets;
    int top;
    int capacity;
} EffectStack;

// Estrutura para o banco de dados de monstros
typedef struct {
    PokeMonster* monsters;
    int count;
} MonsterDatabase;

// Estrutura para o sistema de batalha
typedef struct BattleSystem {
    MonsterList* playerTeam;
    MonsterList* opponentTeam;
    ActionQueue* actionQueue;
    EffectStack* effectStack;
    int turn;
    int battleState;
    bool playerTurn;
    int selectedAttack;
    int selectedAction;
    bool itemUsed;
    int itemType;
} BattleSystem;

// Protótipos de funções para manipulação das estruturas de dados

// Funções para a lista duplamente encadeada
MonsterList* createMonsterList(void);
void freeMonsterList(MonsterList* list);
void addMonster(MonsterList* list, PokeMonster* monster);
void removeMonster(MonsterList* list, PokeMonster* monster);
PokeMonster* findMonster(MonsterList* list, const char* name);
void switchCurrentMonster(MonsterList* list, PokeMonster* newCurrent);

// Funções para a fila de ações
ActionQueue* createActionQueue(int capacity);
void freeActionQueue(ActionQueue* queue);
bool isQueueEmpty(ActionQueue* queue);
bool isQueueFull(ActionQueue* queue);
bool enqueue(ActionQueue* queue, int action, int parameter, PokeMonster* monster);
bool dequeue(ActionQueue* queue, int* action, int* parameter, PokeMonster** monster);
void clearQueue(ActionQueue* queue);

// Funções para a pilha de efeitos
EffectStack* createEffectStack(int capacity);
void freeEffectStack(EffectStack* stack);
bool isStackEmpty(EffectStack* stack);
bool isStackFull(EffectStack* stack);
bool push(EffectStack* stack, int type, int duration, int value, PokeMonster* target);
bool pop(EffectStack* stack, int* type, int* duration, int* value, PokeMonster** target);
void clearStack(EffectStack* stack);

void freeMonster(PokeMonster* monster);

#endif // STRUCTURES_H