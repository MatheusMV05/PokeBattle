/**
 * PokeBattle - Estruturas de dados
 * 
 * Este arquivo contém as definições das estruturas de dados utilizadas no jogo.
 */

 #ifndef STRUCTURES_H
 #define STRUCTURES_H
 
#include "raylib.h"
#include "battle.h"
 
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
    char name[32];
    MonsterType type;
    int power;              // Poder do ataque (0 para ataques de status)
    int accuracy;           // Precisão (0-100)
    int ppMax;              // PP máximo
    int ppCurrent;          // PP atual
    bool isPhysical;        // Se é físico ou especial (para cálculo de dano)
    int priority;           // Prioridade do ataque (-1 a +1, padrão 0)
    int critRate;           // Taxa de crítico (0-100)
    int effectChance;       // Chance do efeito secundário (0-100)
    StatusType statusEffect;// Efeito de status que pode causar
    int statChange[4];      // Mudanças de stats [ATK, DEF, SPD, ACC]
    int selfStatChange[4];  // Mudanças de stats no usuário
    bool hitsAll;           // Se atinge todos os oponentes
    bool makesContact;      // Se faz contato físico
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