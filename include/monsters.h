/**
 * PokeBattle - Gerenciamento de monstros
 * 
 * Este arquivo contém as declarações de funções para gerenciar o banco de dados de monstros.
 */

#ifndef MONSTERS_H
#define MONSTERS_H

#include "structures.h"

// Protótipos das funções
extern float typeEffectiveness[TYPE_COUNT][TYPE_COUNT];

// Inicializa o banco de dados de monstros
void initializeMonsterDatabase(void);

// Libera o banco de dados de monstros
void freeMonsterDatabase(void);

// Carrega os dados dos monstros a partir de um arquivo
bool loadMonstersFromFile(const char* filename);

// Obtém um monstro do banco de dados pelo índice
PokeMonster* getMonsterByIndex(int index);

// Obtém um monstro do banco de dados pelo nome
PokeMonster* getMonsterByName(const char* name);

// Retorna o número total de monstros no banco de dados
int getMonsterCount(void);

// Cria uma cópia de um monstro do banco de dados
PokeMonster* createMonsterCopy(PokeMonster* source);

// Libera um monstro alocado dinamicamente
void freeMonster(PokeMonster* monster);

// Cria um novo monstro com os parâmetros especificados
PokeMonster* createMonster(const char* name, MonsterType type1, MonsterType type2,
                           int hp, int attack, int defense, int speed);

// Adiciona um ataque a um monstro
void addAttackToMonster(PokeMonster* monster, int slot, const char* name, MonsterType type,
                        int power, int accuracy, int pp, int statusEffect,
                        int statusPower, int statusChance);

// Calcula o multiplicador de dano baseado nos tipos do atacante e do defensor
float calculateTypeEffectiveness(MonsterType attackType, MonsterType defenderType1, MonsterType defenderType2);

// Gera um time aleatório de monstros para o bot
MonsterList* generateRandomTeam(int teamSize);

// Algoritmo Quick Sort para ordenar monstros por velocidade
void quickSortMonstersBySpeed(PokeMonster** monsters, int left, int right);

// Função de particionamento para o Quick Sort
int partitionMonsters(PokeMonster** monsters, int left, int right);

// Retorna o nome do tipo como string
const char* getTypeName(MonsterType type);

// Obtém a cor correspondente ao tipo do monstro
Color getTypeColor(MonsterType type);

void loadMonsterTextures(void);
void unloadMonsterTextures(void);
void updateMonsterAnimations(void);
void verifyMonsterSprites(void);
void UpdateAnimation(Animation* anim);
void removeMonsterByName(MonsterList *team, const char *name);

#endif // MONSTERS_H
