/**
 * PokeBattle - Estruturas de dados
 * 
 * Este arquivo contém as definições das estruturas de dados utilizadas no jogo.
 */

 #ifndef STRUCTURES_H
 #define STRUCTURES_H
 
#include <stddef.h>
#include "raylib.h"

 
 // Tipos de PokeMonstros
 typedef enum {
     TYPE_NONE = -1,
     TYPE_METAL = 0,   // Metal 
     TYPE_FIRE,       // Fogo
     TYPE_WATER,      // Água
     TYPE_ELECTRIC,   // Elétrico
     TYPE_GRASS,      // Grama
     TYPE_DRAGON,     // Dragão
     TYPE_GHOST,      // Fantasma
     TYPE_FLYING,     // Voador
     TYPE_FAIRY,      // Fada
     TYPE_COUNT       // Contador de tipos
 } MonsterType;
 
 // Estrutura para os ataques
 typedef struct {
     char name[32];          // Nome do ataque
     MonsterType type;       // Tipo do ataque
     int power;              // Poder do ataque (0 para ataques de status)
     int accuracy;           // Precisão do ataque (0-100)
     int ppMax;              // Pontos de poder máximos
     int ppCurrent;          // Pontos de poder atuais
     int statusEffect;       // Efeito de status (0 = nenhum, 1 = ataque-, 2 = defesa-, etc.)
     int statusPower;        // Poder do efeito de status
     int statusChance;       // Chance do efeito de status (0-100)
 } Attack;
 
 // Estrutura para um PokeMonstro
 typedef struct PokeMonster {
     char name[32];          // Nome do monstro
     MonsterType type1;      // Tipo primário
     MonsterType type2;      // Tipo secundário (TYPE_NONE se não tiver)
     int hp;                 // Pontos de vida atuais
     int maxHp;              // Pontos de vida máximos
     int attack;             // Ataque
     int defense;            // Defesa
     int speed;              // Velocidade
     Attack attacks[4];      // Lista de 4 ataques
     int statusCondition;    // Condição de status atual
     int statusCounter;      // Contador para efeitos de status temporários
     int statusTurns;         // Duração restante do status (em turnos)
     Texture2D texture;      // Textura do monstro
     struct PokeMonster* next; // Próximo monstro (para lista encadeada)
     struct PokeMonster* prev; // Monstro anterior (para lista duplamente encadeada)
 } PokeMonster;
 
 // Estrutura para lista duplamente encadeada de monstros
 typedef struct {
     PokeMonster* first;     // Primeiro monstro da lista
     PokeMonster* last;      // Último monstro da lista
     PokeMonster* current;   // Monstro atual em campo
     int count;              // Quantidade de monstros na lista
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

// Estrutura para resoluções
typedef struct {
    int width;
    int height;
    const char* description;
} Resolution;

 // Estrutura para a fila de ações
 typedef struct {
     int* actions;           // Array de ações (0 = ataque, 1 = troca, 2 = item)
     int* parameters;        // Parâmetros das ações (índice do ataque, do monstro, etc.)
     PokeMonster** monsters; // Monstros envolvidos nas ações
     int front;              // Índice frontal da fila
     int rear;               // Índice traseiro da fila
     int capacity;           // Capacidade da fila
     int count;              // Quantidade de elementos na fila
 } ActionQueue;
 
 // Estrutura para a pilha de efeitos
 typedef struct {
     int* types;             // Tipos de efeito
     int* durations;         // Durações
     int* values;            // Valores dos efeitos
     PokeMonster** targets;  // Alvos dos efeitos
     int top;                // Topo da pilha
     int capacity;           // Capacidade da pilha
 } EffectStack;
 
 // Estrutura para o banco de dados de monstros
 typedef struct {
     PokeMonster* monsters;  // Array de todos os monstros disponíveis
     int count;              // Quantidade de monstros no banco
 } MonsterDatabase;
 
 // Estrutura para o sistema de batalha
 typedef struct {
     MonsterList* playerTeam;     // Time do jogador
     MonsterList* opponentTeam;   // Time do oponente
     ActionQueue* actionQueue;    // Fila de ações
     EffectStack* effectStack;    // Pilha de efeitos
     int turn;                    // Contador de turnos
     int battleState;             // Estado da batalha
     bool playerTurn;             // Flag para controlar de quem é o turno
     int selectedAttack;          // Ataque selecionado
     int selectedAction;          // Ação selecionada
     bool itemUsed;               // Se o item já foi usado
     int itemType;                // Tipo do item disponível
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