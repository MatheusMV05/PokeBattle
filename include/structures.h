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
    TYPE_NORMAL = 0,
    TYPE_FIRE,       // Fogo
    TYPE_WATER,      // Água
    TYPE_GRASS,      // Grama
    TYPE_ELECTRIC,   // Elétrico
    TYPE_ICE,        // Gelo
    TYPE_FIGHTING,   // Lutador
    TYPE_POISON,     // Veneno
    TYPE_GROUND,     // Terra
    TYPE_FLYING,     // Voador
    TYPE_PSYCHIC,    // Psíquico
    TYPE_BUG,        // Inseto
    TYPE_ROCK,       // Pedra
    TYPE_GHOST,      // Fantasma
    TYPE_DRAGON,     // Dragão
    TYPE_DARK,       // Sombrio
    TYPE_STEEL,      // Metal/Ferro
    TYPE_FAIRY,      // Fada
    TYPE_COUNT
} MonsterType;

typedef struct {
    Vector2 position;
    Vector2 velocity;
    float life;
    float maxLife;
    Color color;
    float size;
    bool active;
} PokeballParticle;

// Estrutura para animação de uma pokébola
typedef struct {
    Vector2 startPos;        // Posição inicial (fora da tela)
    Vector2 targetPos;       // Posição final (onde o pokémon fica)
    Vector2 currentPos;      // Posição atual
    Vector2 velocity;        // Velocidade atual
    float rotation;          // Rotação da pokébola
    float rotationSpeed;     // Velocidade de rotação
    float scale;             // Escala da pokébola
    float bounceHeight;      // Altura do bounce quando pousar
    float bounceTimer;       // Timer do bounce
    float openTimer;         // Timer para abertura
    bool isLanded;           // Se pousou no chão
    bool isOpening;          // Se está se abrindo
    bool isOpen;             // Se já abriu completamente
    Color tint;              // Cor da pokébola
} PokeballAnimation;

typedef struct {
    const char* name;
    int pokedexNum;
} MonsterMapping;

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

typedef struct {
    Texture2D *frames;     // Array de texturas
    int frameCount;        // Número total de frames
    int currentFrame;      // Frame atual
    float frameDelay;      // Tempo entre frames (em segundos)
    float elapsedTime;     // Tempo acumulado
} Animation;


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
    Animation frontAnimation;  // Substitui frontTexture
    Animation backAnimation;   // Substitui backTexture
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

typedef struct {
    char messages[5][256]; // Até 5 mensagens em sequência
    int messageCount;
    int currentMessage;
    float timePerMessage;
    bool waitingForInput;
} MessageSequence;

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
    bool playerItemUsed;  // Flag para item usado pelo jogador
    bool botItemUsed;     // Flag para item usado pelo bot
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