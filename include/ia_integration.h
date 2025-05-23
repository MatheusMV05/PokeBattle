#ifndef IA_INTEGRATION_H
#define IA_INTEGRATION_H

#include <stdbool.h>
#include "structures.h"
#include "battle.h"
#include <curl/curl.h>


// Tamanho máximo para um prompt de IA
#define MAX_PROMPT_SIZE 2048
// Tamanho máximo para uma resposta da IA
#define MAX_RESPONSE_SIZE 4096

extern bool initialized;
extern CURL* curl_handle;

// Protótipos das funções

// Inicializa a conexão com a API de IA
bool initializeAI(void);

// Encerra a conexão com a API de IA
void shutdownAI(void);

// Consulta a API de IA com um prompt e retorna a resposta
char* queryAI(const char* prompt);

// Funções específicas para utilização da IA no jogo

// Gera uma descrição criativa para um ataque
char* generateAttackDescription(PokeMonster* attacker, PokeMonster* defender, Attack* attack);

// Sugere a melhor ação para o bot
int getAISuggestedAction(PokeMonster* botMonster, PokeMonster* playerMonster);

// Sugere o melhor ataque para o bot
int getAISuggestedAttack(PokeMonster* botMonster, PokeMonster* playerMonster);

// Sugere o melhor monstro para troca
int getAISuggestedMonster(MonsterList* botTeam, PokeMonster* playerMonster);

// Fornece dicas estratégicas para o jogador
char* getStrategicHint(PokeMonster* playerMonster, PokeMonster* botMonster);

// Gera uma narração para a batalha atual
char* generateBattleNarration(PokeMonster* playerMonster, PokeMonster* botMonster, int lastAction);

// Gera uma frase de vitória ou derrota
char* generateBattleResult(bool playerWon);

// Gera uma frase ao usar um item
char* generateItemUseDescription(ItemType itemType, PokeMonster* user);

// Interpreta a resposta da API de IA
int interpretAIResponse(const char* response);

// Funções auxiliares para melhorar a integração com IA
const char* getStatusName(int statusCondition);
const char* getEffectDescription(int statusEffect, int statusChance);

// Sistema de fallback quando a IA falha
int getAISuggestedActionSimple(PokeMonster* botMonster, PokeMonster* playerMonster);

// Testa a conexão com a API
bool testAIConnection(void);

#endif // IA_INTEGRATION_H
