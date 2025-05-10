/**
 * PokeBattle - Implementação das estruturas de dados
 * 
 * Este arquivo contém as implementações das funções para manipular as estruturas de dados.
 */

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #define NOGDI
    #define NOUSER
#endif

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include "structures.h"
 
 // Funções para a lista duplamente encadeada
 
 // Cria uma nova lista de monstros
 MonsterList* createMonsterList(void) {
     MonsterList* list = (MonsterList*)malloc(sizeof(MonsterList));
     if (list == NULL) {
         return NULL;
     }
     
     list->first = NULL;
     list->last = NULL;
     list->current = NULL;
     list->count = 0;
     
     return list;
 }
 
 // Libera uma lista de monstros
 void freeMonsterList(MonsterList* list) {
     if (list == NULL) {
         return;
     }
     
     // Liberar todos os monstros na lista
     PokeMonster* current = list->first;
     while (current != NULL) {
         PokeMonster* next = current->next;
         freeMonster(current);
         current = next;
     }
     
     // Liberar a própria lista
     free(list);
 }
 
 // Adiciona um monstro à lista
 void addMonster(MonsterList* list, PokeMonster* monster) {
    if (list == NULL || monster == NULL) {
        printf("Erro: lista ou monstro nulo em addMonster\n"); // Debug
        return;
    }
     
     // Configurar os ponteiros do novo monstro
     monster->next = NULL;
     monster->prev = list->last;
     
     // Primeiro monstro da lista
     if (list->first == NULL) {
         list->first = monster;
         list->last = monster;
         list->current = monster; // Se for o primeiro, também é o atual
     } else {
         // Caso contrário, anexar ao final
         list->last->next = monster;
         list->last = monster;
     }
     
     list->count++;
     printf("Monstro %s adicionado. Total na lista: %d\n", monster->name, list->count); // Debug
 }
 
 // Remove um monstro da lista
 void removeMonster(MonsterList* list, PokeMonster* monster) {
     if (list == NULL || monster == NULL) {
         return;
     }
     
     // Verificar se o monstro está na lista
     PokeMonster* current = list->first;
     bool found = false;
     
     while (current != NULL) {
         if (current == monster) {
             found = true;
             break;
         }
         current = current->next;
     }
     
     if (!found) {
         return; // Monstro não está na lista
     }
     
     // Ajustar o ponteiro current se necessário
     if (list->current == monster) {
         list->current = monster->next ? monster->next : monster->prev;
     }
     
     // Ajustar ponteiros prev/next
     if (monster->prev) {
         monster->prev->next = monster->next;
     } else {
         // É o primeiro da lista
         list->first = monster->next;
     }
     
     if (monster->next) {
         monster->next->prev = monster->prev;
     } else {
         // É o último da lista
         list->last = monster->prev;
     }
     
     // Liberar o monstro
     freeMonster(monster);
     list->count--;
 }
 
 // Encontra um monstro pelo nome
 PokeMonster* findMonster(MonsterList* list, const char* name) {
     if (list == NULL || name == NULL) {
         return NULL;
     }
     
     PokeMonster* current = list->first;
     while (current != NULL) {
         if (strcmp(current->name, name) == 0) {
             return current;
         }
         current = current->next;
     }
     
     return NULL; // Não encontrado
 }
 
 // Troca o monstro atual
 void switchCurrentMonster(MonsterList* list, PokeMonster* newCurrent) {
     if (list == NULL || newCurrent == NULL) {
         return;
     }
     
     // Verificar se o monstro está na lista
     PokeMonster* current = list->first;
     bool found = false;
     
     while (current != NULL) {
         if (current == newCurrent) {
             found = true;
             break;
         }
         current = current->next;
     }
     
     if (found) {
         list->current = newCurrent;
     }
 }
 
 // Funções para a fila de ações
 
 // Cria uma nova fila de ações
 ActionQueue* createActionQueue(int capacity) {
     if (capacity <= 0) {
         capacity = 10; // Capacidade padrão
     }
     
     ActionQueue* queue = (ActionQueue*)malloc(sizeof(ActionQueue));
     if (queue == NULL) {
         return NULL;
     }
     
     queue->actions = (int*)malloc(sizeof(int) * capacity);
     queue->parameters = (int*)malloc(sizeof(int) * capacity);
     queue->monsters = (PokeMonster**)malloc(sizeof(PokeMonster*) * capacity);
     
     if (queue->actions == NULL || queue->parameters == NULL || queue->monsters == NULL) {
         // Falha ao alocar memória, liberar tudo e retornar NULL
         if (queue->actions) free(queue->actions);
         if (queue->parameters) free(queue->parameters);
         if (queue->monsters) free(queue->monsters);
         free(queue);
         return NULL;
     }
     
     queue->front = 0;
     queue->rear = -1;
     queue->capacity = capacity;
     queue->count = 0;
     
     return queue;
 }
 
 // Libera uma fila de ações
 void freeActionQueue(ActionQueue* queue) {
     if (queue == NULL) {
         return;
     }
     
     if (queue->actions) free(queue->actions);
     if (queue->parameters) free(queue->parameters);
     if (queue->monsters) free(queue->monsters);
     free(queue);
 }
 
 // Verifica se a fila está vazia
 bool isQueueEmpty(ActionQueue* queue) {
     return (queue == NULL || queue->count == 0);
 }
 
 // Verifica se a fila está cheia
 bool isQueueFull(ActionQueue* queue) {
     return (queue == NULL || queue->count == queue->capacity);
 }
 
 // Adiciona uma ação à fila
 bool enqueue(ActionQueue* queue, int action, int parameter, PokeMonster* monster) {
     if (queue == NULL || isQueueFull(queue)) {
         return false;
     }
     
     queue->rear = (queue->rear + 1) % queue->capacity;
     queue->actions[queue->rear] = action;
     queue->parameters[queue->rear] = parameter;
     queue->monsters[queue->rear] = monster;
     queue->count++;
     
     return true;
 }
 
 // Remove uma ação da fila
 bool dequeue(ActionQueue* queue, int* action, int* parameter, PokeMonster** monster) {
     if (queue == NULL || isQueueEmpty(queue)) {
         return false;
     }
     
     if (action) *action = queue->actions[queue->front];
     if (parameter) *parameter = queue->parameters[queue->front];
     if (monster) *monster = queue->monsters[queue->front];
     
     queue->front = (queue->front + 1) % queue->capacity;
     queue->count--;
     
     return true;
 }
 
 // Limpa a fila
 void clearQueue(ActionQueue* queue) {
     if (queue == NULL) {
         return;
     }
     
     queue->front = 0;
     queue->rear = -1;
     queue->count = 0;
 }
 
 // Funções para a pilha de efeitos
 
 // Cria uma nova pilha de efeitos
 EffectStack* createEffectStack(int capacity) {
     if (capacity <= 0) {
         capacity = 10; // Capacidade padrão
     }
     
     EffectStack* stack = (EffectStack*)malloc(sizeof(EffectStack));
     if (stack == NULL) {
         return NULL;
     }
     
     stack->types = (int*)malloc(sizeof(int) * capacity);
     stack->durations = (int*)malloc(sizeof(int) * capacity);
     stack->values = (int*)malloc(sizeof(int) * capacity);
     stack->targets = (PokeMonster**)malloc(sizeof(PokeMonster*) * capacity);
     
     if (stack->types == NULL || stack->durations == NULL || 
         stack->values == NULL || stack->targets == NULL) {
         // Falha ao alocar memória, liberar tudo e retornar NULL
         if (stack->types) free(stack->types);
         if (stack->durations) free(stack->durations);
         if (stack->values) free(stack->values);
         if (stack->targets) free(stack->targets);
         free(stack);
         return NULL;
     }
     
     stack->top = -1;
     stack->capacity = capacity;
     
     return stack;
 }
 
 // Libera uma pilha de efeitos
 void freeEffectStack(EffectStack* stack) {
     if (stack == NULL) {
         return;
     }
     
     if (stack->types) free(stack->types);
     if (stack->durations) free(stack->durations);
     if (stack->values) free(stack->values);
     if (stack->targets) free(stack->targets);
     free(stack);
 }
 
 // Verifica se a pilha está vazia
 bool isStackEmpty(EffectStack* stack) {
     return (stack == NULL || stack->top == -1);
 }
 
 // Verifica se a pilha está cheia
 bool isStackFull(EffectStack* stack) {
     return (stack == NULL || stack->top == stack->capacity - 1);
 }
 
 // Adiciona um efeito à pilha
 bool push(EffectStack* stack, int type, int duration, int value, PokeMonster* target) {
     if (stack == NULL || isStackFull(stack)) {
         return false;
     }
     
     stack->top++;
     stack->types[stack->top] = type;
     stack->durations[stack->top] = duration;
     stack->values[stack->top] = value;
     stack->targets[stack->top] = target;
     
     return true;
 }
 
 // Remove um efeito da pilha
 bool pop(EffectStack* stack, int* type, int* duration, int* value, PokeMonster** target) {
     if (stack == NULL || isStackEmpty(stack)) {
         return false;
     }
     
     if (type) *type = stack->types[stack->top];
     if (duration) *duration = stack->durations[stack->top];
     if (value) *value = stack->values[stack->top];
     if (target) *target = stack->targets[stack->top];
     
     stack->top--;
     
     return true;
 }
 
 // Limpa a pilha
 void clearStack(EffectStack* stack) {
     if (stack == NULL) {
         return;
     }
     
     stack->top = -1;
 }