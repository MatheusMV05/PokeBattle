/**
 * PokeBattle - Implementação do gerenciamento de monstros
 *
 * Este arquivo contém as implementações das funções para gerenciar o banco de dados de monstros.
 */
#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #define NOGDI
    #define NOUSER
#endif

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include "raylib.h"
 #include "monsters.h"
 #include "battle.h"
#include "globals.h"

 static void createMonsterDatabase(void);

 // Banco de dados de monstros
 MonsterDatabase monsterDB;

// Matriz de efetividade de tipos conforme os jogos oficiais
// Valores: 0.0 = sem efeito, 0.5 = não muito efetivo, 1.0 = normal, 2.0 = super efetivo
float typeEffectiveness[TYPE_COUNT][TYPE_COUNT] = {
    // NORMAL   FIRE    WATER   GRASS   ELECTRIC  ICE     FIGHT   POISON  GROUND  FLYING  PSYCHIC  BUG     ROCK    GHOST   DRAGON  DARK    STEEL   FAIRY
    {  1.0f,    1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   0.5f,   0.0f,   1.0f,   1.0f,   0.5f,   1.0f  }, // NORMAL
    {  1.0f,    0.5f,   0.5f,   2.0f,   1.0f,   2.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   2.0f,   0.5f,   1.0f,   0.5f,   1.0f,   2.0f,   1.0f  }, // FIRE
    {  1.0f,    2.0f,   0.5f,   0.5f,   1.0f,   1.0f,   1.0f,   1.0f,   2.0f,   1.0f,   1.0f,   1.0f,   2.0f,   1.0f,   0.5f,   1.0f,   1.0f,   1.0f  }, // WATER
    {  1.0f,    0.5f,   2.0f,   0.5f,   1.0f,   1.0f,   1.0f,   0.5f,   2.0f,   0.5f,   1.0f,   0.5f,   2.0f,   1.0f,   0.5f,   1.0f,   0.5f,   1.0f  }, // GRASS
    {  1.0f,    1.0f,   2.0f,   0.5f,   0.5f,   1.0f,   1.0f,   1.0f,   0.0f,   2.0f,   1.0f,   1.0f,   1.0f,   1.0f,   0.5f,   1.0f,   1.0f,   1.0f  }, // ELECTRIC
    {  1.0f,    0.5f,   0.5f,   2.0f,   1.0f,   0.5f,   1.0f,   1.0f,   2.0f,   2.0f,   1.0f,   1.0f,   1.0f,   1.0f,   2.0f,   1.0f,   0.5f,   1.0f  }, // ICE
    {  2.0f,    1.0f,   1.0f,   1.0f,   1.0f,   2.0f,   1.0f,   0.5f,   1.0f,   0.5f,   0.5f,   0.5f,   2.0f,   0.0f,   1.0f,   2.0f,   2.0f,   0.5f  }, // FIGHTING
    {  1.0f,    1.0f,   1.0f,   2.0f,   1.0f,   1.0f,   1.0f,   0.5f,   0.5f,   1.0f,   1.0f,   1.0f,   0.5f,   0.5f,   1.0f,   1.0f,   0.0f,   2.0f  }, // POISON
    {  1.0f,    2.0f,   1.0f,   0.5f,   2.0f,   1.0f,   1.0f,   2.0f,   1.0f,   0.0f,   1.0f,   0.5f,   2.0f,   1.0f,   1.0f,   1.0f,   2.0f,   1.0f  }, // GROUND
    {  1.0f,    1.0f,   1.0f,   2.0f,   0.5f,   1.0f,   2.0f,   1.0f,   1.0f,   1.0f,   1.0f,   2.0f,   0.5f,   1.0f,   1.0f,   1.0f,   0.5f,   1.0f  }, // FLYING
    {  1.0f,    1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   2.0f,   2.0f,   1.0f,   1.0f,   0.5f,   1.0f,   1.0f,   1.0f,   1.0f,   0.0f,   0.5f,   1.0f  }, // PSYCHIC
    {  1.0f,    0.5f,   1.0f,   2.0f,   1.0f,   1.0f,   0.5f,   0.5f,   1.0f,   0.5f,   2.0f,   1.0f,   1.0f,   0.5f,   1.0f,   2.0f,   0.5f,   0.5f  }, // BUG
    {  1.0f,    2.0f,   1.0f,   1.0f,   1.0f,   2.0f,   0.5f,   1.0f,   0.5f,   2.0f,   1.0f,   2.0f,   1.0f,   1.0f,   1.0f,   1.0f,   0.5f,   1.0f  }, // ROCK
    {  0.0f,    1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   2.0f,   1.0f,   1.0f,   2.0f,   1.0f,   0.5f,   1.0f,   1.0f  }, // GHOST
    {  1.0f,    1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   2.0f,   1.0f,   0.5f,   0.0f  }, // DRAGON
    {  1.0f,    1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   0.5f,   1.0f,   1.0f,   1.0f,   2.0f,   1.0f,   1.0f,   2.0f,   1.0f,   0.5f,   1.0f,   0.5f  }, // DARK
    {  1.0f,    0.5f,   0.5f,   1.0f,   0.5f,   2.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   2.0f,   1.0f,   1.0f,   1.0f,   0.5f,   2.0f  }, // STEEL
    {  1.0f,    0.5f,   1.0f,   1.0f,   1.0f,   1.0f,   2.0f,   0.5f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   2.0f,   2.0f,   0.5f,   1.0f  }  // FAIRY
};

// Nomes dos tipos
static const char* typeNames[TYPE_COUNT] = {
    "Normal", "Fogo", "Água", "Grama", "Elétrico",
    "Gelo", "Lutador", "Veneno", "Terra", "Voador",
    "Psíquico", "Inseto", "Pedra", "Fantasma",
    "Dragão", "Sombrio", "Metal", "Fada"
};

// Cores dos tipos
static Color typeColors[TYPE_COUNT] = {
    (Color){ 168, 168, 120, 255 }, // Normal (bege)
    (Color){ 240, 80, 48, 255 },   // Fogo (vermelho)
    (Color){ 48, 128, 240, 255 },  // Água (azul)
    (Color){ 120, 200, 80, 255 },  // Grama (verde)
    (Color){ 248, 208, 48, 255 },  // Elétrico (amarelo)
    (Color){ 152, 216, 216, 255 }, // Gelo (azul claro)
    (Color){ 192, 48, 40, 255 },   // Lutador (vermelho escuro)
    (Color){ 160, 64, 160, 255 },  // Veneno (roxo)
    (Color){ 224, 192, 104, 255 }, // Terra (marrom)
    (Color){ 152, 168, 240, 255 }, // Voador (azul claro)
    (Color){ 248, 88, 136, 255 },  // Psíquico (rosa)
    (Color){ 168, 184, 32, 255 },  // Inseto (verde oliva)
    (Color){ 184, 160, 56, 255 },  // Pedra (marrom)
    (Color){ 112, 88, 152, 255 },  // Fantasma (roxo escuro)
    (Color){ 112, 56, 248, 255 },  // Dragão (roxo)
    (Color){ 112, 88, 72, 255 },   // Sombrio (marrom escuro)
    (Color){ 184, 184, 208, 255 }, // Metal (cinza)
    (Color){ 240, 182, 188, 255 }  // Fada (rosa claro)
};

 // Inicializa o banco de dados de monstros
 void initializeMonsterDatabase(void) {
    monsterDB.count = 0;
    monsterDB.monsters = NULL;

    // Criar monstros manualmente
    createMonsterDatabase();

    // Note: As texturas serão carregadas depois pela função loadMonsterTextures
    // Não carregamos texturas aqui porque o Raylib precisa estar inicializado primeiro
}

// Função de diagnóstico para identificar problemas com o mapeamento de tipos
void diagnoseTypeIssues(void) {
    printf("\n================ DIAGNÓSTICO DE TIPOS ================\n");

    // 1. Validar a ordem dos arrays typeNames e typeColors
    printf("VERIFICANDO ARRAYS DE TIPOS:\n");
    printf("Temos %d tipos definidos (TYPE_COUNT)\n", TYPE_COUNT);
    printf("Array typeNames tem capacidade para %lu tipos\n", sizeof(typeNames)/sizeof(typeNames[0]));
    printf("Array typeColors tem capacidade para %lu tipos\n", sizeof(typeColors)/sizeof(typeColors[0]));

    if (sizeof(typeNames)/sizeof(typeNames[0]) != TYPE_COUNT) {
        printf("ERRO: Array typeNames não tem o mesmo tamanho que TYPE_COUNT!\n");
    }

    if (sizeof(typeColors)/sizeof(typeColors[0]) != TYPE_COUNT) {
        printf("ERRO: Array typeColors não tem o mesmo tamanho que TYPE_COUNT!\n");
    }

    // 2. Listar o mapeamento entre enum e nomes para depuração
    printf("\nMAPEAMENTO ATUAL DE TIPOS:\n");
    for (int i = 0; i < TYPE_COUNT; i++) {
        printf("Índice %2d: Nome='%s'\n", i, typeNames[i]);
    }

    // 3. Diagnóstico da matriz de efetividade
    printf("\nVERIFICANDO MATRIZ DE EFETIVIDADE:\n");
    printf("Dimensão da matriz: %lu x %lu\n",
           sizeof(typeEffectiveness)/sizeof(typeEffectiveness[0]),
           sizeof(typeEffectiveness[0])/sizeof(typeEffectiveness[0][0]));

    if (sizeof(typeEffectiveness)/sizeof(typeEffectiveness[0]) != TYPE_COUNT) {
        printf("ERRO: Número de linhas da matriz não corresponde a TYPE_COUNT!\n");
    }

    if (sizeof(typeEffectiveness[0])/sizeof(typeEffectiveness[0][0]) != TYPE_COUNT) {
        printf("ERRO: Número de colunas da matriz não corresponde a TYPE_COUNT!\n");
    }

    // 4. Verificar monstros com tipos especificados incorretamente
    printf("\nMONSTROS COM TIPOS POTENCIALMENTE PROBLEMÁTICOS:\n");
    for (int i = 0; i < monsterDB.count; i++) {
        if (monsterDB.monsters[i].type1 < 0 || monsterDB.monsters[i].type1 >= TYPE_COUNT) {
            printf("ERRO: %s tem type1=%d (fora do intervalo válido 0-%d)\n",
                   monsterDB.monsters[i].name, monsterDB.monsters[i].type1, TYPE_COUNT-1);
        }

        if (monsterDB.monsters[i].type2 != TYPE_NONE &&
            (monsterDB.monsters[i].type2 < 0 || monsterDB.monsters[i].type2 >= TYPE_COUNT)) {
            printf("ERRO: %s tem type2=%d (fora do intervalo válido)\n",
                   monsterDB.monsters[i].name, monsterDB.monsters[i].type2);
        }
    }

    // 5. Verificar função getTypeName
    printf("\nVERIFICANDO FUNÇÃO getTypeName():\n");
    for (int i = 0; i < TYPE_COUNT; i++) {
        printf("getTypeName(%d) retorna '%s'\n", i, getTypeName(i));
    }

    // 6. Verificar definição de alguns monstros específicos
    printf("\nVERIFICANDO MONSTROS ESPECÍFICOS:\n");
    for (int i = 0; i < monsterDB.count; i++) {
        if (strcmp(monsterDB.monsters[i].name, "Charizard") == 0) {
            printf("Charizard: type1=%d (%s), type2=%d (%s)\n",
                   monsterDB.monsters[i].type1, getTypeName(monsterDB.monsters[i].type1),
                   monsterDB.monsters[i].type2, getTypeName(monsterDB.monsters[i].type2));
        }
        else if (strcmp(monsterDB.monsters[i].name, "Grillitron") == 0) {
            printf("Grillitron: type1=%d (%s), type2=%d (%s)\n",
                   monsterDB.monsters[i].type1, getTypeName(monsterDB.monsters[i].type1),
                   monsterDB.monsters[i].type2, getTypeName(monsterDB.monsters[i].type2));
        }
        else if (strcmp(monsterDB.monsters[i].name, "Tatarion") == 0) {
            printf("Tatarion: type1=%d (%s), type2=%d (%s)\n",
                   monsterDB.monsters[i].type1, getTypeName(monsterDB.monsters[i].type1),
                   monsterDB.monsters[i].type2, getTypeName(monsterDB.monsters[i].type2));
        }
    }

    printf("\n============= FIM DO DIAGNÓSTICO ==================\n\n");
}

 // Cria o banco de dados de monstros com monstros pré-definidos
void createMonsterDatabase(void) {
    // Alocar espaço para todos os monstros (9 Pokitos + 45 Pokémon clássicos)
    monsterDB.count = 54;
    monsterDB.monsters = (PokeMonster*)malloc(sizeof(PokeMonster) * monsterDB.count);

    if (monsterDB.monsters == NULL) {
        printf("Erro ao alocar memória para o banco de monstros!\n");
        return;
    }

    int index = 0;

    // === POKITOS ORIGINAIS ===

    // Pokitos 1: Air fryer - Grillitron (Metal/Fogo)
    strcpy(monsterDB.monsters[index].name, "Grillitron");
    monsterDB.monsters[index].type1 = TYPE_STEEL;
    monsterDB.monsters[index].type2 = TYPE_FIRE;
    monsterDB.monsters[index].maxHp = 85;
    monsterDB.monsters[index].hp = 85;
    monsterDB.monsters[index].attack = 90;
    monsterDB.monsters[index].defense = 100;
    monsterDB.monsters[index].speed = 60;
    monsterDB.monsters[index].statusCondition = STATUS_NONE;
    monsterDB.monsters[index].statusCounter = 0;
    monsterDB.monsters[index].statusTurns = 0;
    monsterDB.monsters[index].next = NULL;
    monsterDB.monsters[index].prev = NULL;
    addAttackToMonster(&monsterDB.monsters[index], 0, "Grill Blast", TYPE_FIRE, 80, 100, 15, STATUS_BURNING, 0, 20);
    addAttackToMonster(&monsterDB.monsters[index], 1, "Metal Slice", TYPE_STEEL, 75, 95, 20, 0, 0, 0);
    addAttackToMonster(&monsterDB.monsters[index], 2, "Heat Wave", TYPE_FIRE, 95, 90, 10, 0, 0, 0);
    addAttackToMonster(&monsterDB.monsters[index], 3, "Steel Defense", TYPE_STEEL, 0, 100, 15, 7, 30, 100);
    index++;

    // Pokitos 2: Boi tata - Tatárion (Dragão/Fogo)
    strcpy(monsterDB.monsters[index].name, "Tatarion");
    monsterDB.monsters[index].type1 = TYPE_DRAGON;
    monsterDB.monsters[index].type2 = TYPE_FIRE;
    monsterDB.monsters[index].maxHp = 95;
    monsterDB.monsters[index].hp = 95;
    monsterDB.monsters[index].attack = 110;
    monsterDB.monsters[index].defense = 80;
    monsterDB.monsters[index].speed = 90;
    monsterDB.monsters[index].statusCondition = STATUS_NONE;
    monsterDB.monsters[index].statusCounter = 0;
    monsterDB.monsters[index].statusTurns = 0;
    monsterDB.monsters[index].next = NULL;
    monsterDB.monsters[index].prev = NULL;
    addAttackToMonster(&monsterDB.monsters[index], 0, "Flame Charge", TYPE_FIRE, 85, 100, 15, 0, 0, 0);
    addAttackToMonster(&monsterDB.monsters[index], 1, "Dragon Rage", TYPE_DRAGON, 90, 95, 10, 0, 0, 0);
    addAttackToMonster(&monsterDB.monsters[index], 2, "Fire Blast", TYPE_FIRE, 110, 85, 5, STATUS_BURNING, 0, 30);
    addAttackToMonster(&monsterDB.monsters[index], 3, "Dragon Pulse", TYPE_DRAGON, 95, 90, 10, 0, 0, 0);
    index++;

    // Pokitos 3: Iara - Aquariah (Água/Fada)
    strcpy(monsterDB.monsters[index].name, "Aquariah");
    monsterDB.monsters[index].type1 = TYPE_WATER;
    monsterDB.monsters[index].type2 = TYPE_FAIRY;
    monsterDB.monsters[index].maxHp = 90;
    monsterDB.monsters[index].hp = 90;
    monsterDB.monsters[index].attack = 75;
    monsterDB.monsters[index].defense = 85;
    monsterDB.monsters[index].speed = 85;
    monsterDB.monsters[index].statusCondition = STATUS_NONE;
    monsterDB.monsters[index].statusCounter = 0;
    monsterDB.monsters[index].statusTurns = 0;
    monsterDB.monsters[index].next = NULL;
    monsterDB.monsters[index].prev = NULL;
    addAttackToMonster(&monsterDB.monsters[index], 0, "Aqua Jet", TYPE_WATER, 70, 100, 20, 0, 0, 0);
    addAttackToMonster(&monsterDB.monsters[index], 1, "Moonblast", TYPE_FAIRY, 95, 100, 15, 0, 0, 0);
    addAttackToMonster(&monsterDB.monsters[index], 2, "Enchanted Voice", TYPE_FAIRY, 80, 100, 15, STATUS_SLEEPING, 0, 20);
    addAttackToMonster(&monsterDB.monsters[index], 3, "Hydro Pump", TYPE_WATER, 120, 80, 5, 0, 0, 0);
    index++;

    // Pokitos 4: Mula sem cabeça - Pyromula (Fogo)
    strcpy(monsterDB.monsters[index].name, "Pyromula");
    monsterDB.monsters[index].type1 = TYPE_FIRE;
    monsterDB.monsters[index].type2 = TYPE_GHOST;
    monsterDB.monsters[index].maxHp = 80;
    monsterDB.monsters[index].hp = 80;
    monsterDB.monsters[index].attack = 100;
    monsterDB.monsters[index].defense = 70;
    monsterDB.monsters[index].speed = 115;
    monsterDB.monsters[index].statusCondition = STATUS_NONE;
    monsterDB.monsters[index].statusCounter = 0;
    monsterDB.monsters[index].statusTurns = 0;
    monsterDB.monsters[index].next = NULL;
    monsterDB.monsters[index].prev = NULL;
    addAttackToMonster(&monsterDB.monsters[index], 0, "Flame Kick", TYPE_FIRE, 85, 100, 15, 0, 0, 0);
    addAttackToMonster(&monsterDB.monsters[index], 1, "Fire Gallop", TYPE_FIRE, 90, 95, 10, 0, 0, 0);
    addAttackToMonster(&monsterDB.monsters[index], 2, "Headless Charge", TYPE_GHOST, 100, 90, 10, 0, 0, 20);
    addAttackToMonster(&monsterDB.monsters[index], 3, "Quick Attack", TYPE_NORMAL, 60, 100, 30, 0, 0, 0);
    index++;

    // Pokitos 5: Cabeça de cuia - Netomon (Fantasma)
    strcpy(monsterDB.monsters[index].name, "Netomon");
    monsterDB.monsters[index].type1 = TYPE_GHOST;
    monsterDB.monsters[index].type2 = TYPE_NONE;
    monsterDB.monsters[index].maxHp = 70;
    monsterDB.monsters[index].hp = 70;
    monsterDB.monsters[index].attack = 85;
    monsterDB.monsters[index].defense = 75;
    monsterDB.monsters[index].speed = 100;
    monsterDB.monsters[index].statusCondition = STATUS_NONE;
    monsterDB.monsters[index].statusCounter = 0;
    monsterDB.monsters[index].statusTurns = 0;
    monsterDB.monsters[index].next = NULL;
    monsterDB.monsters[index].prev = NULL;
    addAttackToMonster(&monsterDB.monsters[index], 0, "Shadow Ball", TYPE_GHOST, 80, 100, 15, 0, 0, 0);
    addAttackToMonster(&monsterDB.monsters[index], 1, "Nightmare", TYPE_GHOST, 60, 100, 15, STATUS_SLEEPING, 0, 20);
    addAttackToMonster(&monsterDB.monsters[index], 2, "Phantom Force", TYPE_GHOST, 90, 90, 10, 0, 0, 0);
    addAttackToMonster(&monsterDB.monsters[index], 3, "Shadow Sneak", TYPE_GHOST, 70, 100, 20, 0, 0, 0);
    index++;

    // Pokitos 6: Brigadeiro - Brigadeli (Fada)
    strcpy(monsterDB.monsters[index].name, "Brigadeli");
    monsterDB.monsters[index].type1 = TYPE_FAIRY;
    monsterDB.monsters[index].type2 = TYPE_NONE;
    monsterDB.monsters[index].maxHp = 85;
    monsterDB.monsters[index].hp = 85;
    monsterDB.monsters[index].attack = 75;
    monsterDB.monsters[index].defense = 85;
    monsterDB.monsters[index].speed = 80;
    monsterDB.monsters[index].statusCondition = STATUS_NONE;
    monsterDB.monsters[index].statusCounter = 0;
    monsterDB.monsters[index].statusTurns = 0;
    monsterDB.monsters[index].next = NULL;
    monsterDB.monsters[index].prev = NULL;
    addAttackToMonster(&monsterDB.monsters[index], 0, "Sweet Kiss", TYPE_FAIRY, 50, 100, 20, 0, 0, 0);
    addAttackToMonster(&monsterDB.monsters[index], 1, "Dazzling Gleam", TYPE_FAIRY, 80, 100, 15, 0, 0, 0);
    addAttackToMonster(&monsterDB.monsters[index], 2, "Play Rough", TYPE_FAIRY, 90, 90, 10, STATUS_ATK_DOWN, 20, 30);
    addAttackToMonster(&monsterDB.monsters[index], 3, "Charm", TYPE_FAIRY, 0, 100, 15, STATUS_ATK_DOWN, 30, 100);
    index++;

    // Pokitos 7: Ventilador - Ventaforte (Voador/Elétrico)
    strcpy(monsterDB.monsters[index].name, "Ventaforte");
    monsterDB.monsters[index].type1 = TYPE_FLYING;
    monsterDB.monsters[index].type2 = TYPE_ELECTRIC;
    monsterDB.monsters[index].maxHp = 75;
    monsterDB.monsters[index].hp = 75;
    monsterDB.monsters[index].attack = 85;
    monsterDB.monsters[index].defense = 70;
    monsterDB.monsters[index].speed = 110;
    monsterDB.monsters[index].statusCondition = STATUS_NONE;
    monsterDB.monsters[index].statusCounter = 0;
    monsterDB.monsters[index].statusTurns = 0;
    monsterDB.monsters[index].next = NULL;
    monsterDB.monsters[index].prev = NULL;
    addAttackToMonster(&monsterDB.monsters[index], 0, "Air Slash", TYPE_FLYING, 80, 95, 15, 0, 0, 0);
    addAttackToMonster(&monsterDB.monsters[index], 1, "Thunder Shock", TYPE_ELECTRIC, 75, 100, 20, STATUS_PARALYZED, 0, 30);
    addAttackToMonster(&monsterDB.monsters[index], 2, "Gust", TYPE_FLYING, 65, 100, 25, 0, 0, 0);
    addAttackToMonster(&monsterDB.monsters[index], 3, "Discharge", TYPE_ELECTRIC, 90, 90, 10, STATUS_PARALYZED, 0, 30);
    index++;

    // Pokitos 8: Extensão - Gambiarra (Elétrico)
    strcpy(monsterDB.monsters[index].name, "Gambiarra");
    monsterDB.monsters[index].type1 = TYPE_ELECTRIC;
    monsterDB.monsters[index].type2 = TYPE_NONE;
    monsterDB.monsters[index].maxHp = 70;
    monsterDB.monsters[index].hp = 70;
    monsterDB.monsters[index].attack = 90;
    monsterDB.monsters[index].defense = 65;
    monsterDB.monsters[index].speed = 105;
    monsterDB.monsters[index].statusCondition = STATUS_NONE;
    monsterDB.monsters[index].statusCounter = 0;
    monsterDB.monsters[index].statusTurns = 0;
    monsterDB.monsters[index].next = NULL;
    monsterDB.monsters[index].prev = NULL;
    addAttackToMonster(&monsterDB.monsters[index], 0, "Spark", TYPE_ELECTRIC, 75, 100, 20, 0, 0, 0);
    addAttackToMonster(&monsterDB.monsters[index], 1, "Power Surge", TYPE_ELECTRIC, 85, 90, 15, STATUS_PARALYZED, 0, 20);
    addAttackToMonster(&monsterDB.monsters[index], 2, "Thunderbolt", TYPE_ELECTRIC, 95, 95, 10, STATUS_PARALYZED, 0, 10);
    addAttackToMonster(&monsterDB.monsters[index], 3, "Charge", TYPE_ELECTRIC, 0, 100, 15, 0, 0, 0);
    index++;

    // Pokitos 9: Goiaba + Mandragora - Mandragoiaba (Grama/Fada)
    strcpy(monsterDB.monsters[index].name, "Mandragoiaba");
    monsterDB.monsters[index].type1 = TYPE_GRASS;
    monsterDB.monsters[index].type2 = TYPE_FAIRY;
    monsterDB.monsters[index].maxHp = 80;
    monsterDB.monsters[index].hp = 80;
    monsterDB.monsters[index].attack = 80;
    monsterDB.monsters[index].defense = 85;
    monsterDB.monsters[index].speed = 75;
    monsterDB.monsters[index].statusCondition = STATUS_NONE;
    monsterDB.monsters[index].statusCounter = 0;
    monsterDB.monsters[index].statusTurns = 0;
    monsterDB.monsters[index].next = NULL;
    monsterDB.monsters[index].prev = NULL;
    addAttackToMonster(&monsterDB.monsters[index], 0, "Magical Leaf", TYPE_GRASS, 80, 100, 15, 0, 0, 0);
    addAttackToMonster(&monsterDB.monsters[index], 1, "Petal Dance", TYPE_GRASS, 90, 90, 10, 0, 0, 0);
    addAttackToMonster(&monsterDB.monsters[index], 2, "Fairy Wind", TYPE_FAIRY, 75, 100, 15, 0, 0, 0);
    addAttackToMonster(&monsterDB.monsters[index], 3, "Screech", TYPE_NORMAL, 0, 90, 20, STATUS_DEF_DOWN, 30, 100);
    index++;

    // === POKÉMON CLÁSSICOS ===

    // Venusaur (Grama/Veneno)
    strcpy(monsterDB.monsters[index].name, "Venusaur");
    monsterDB.monsters[index].type1 = TYPE_GRASS;
    monsterDB.monsters[index].type2 = TYPE_POISON;
    monsterDB.monsters[index].maxHp = 80;
    monsterDB.monsters[index].hp = 80;
    monsterDB.monsters[index].attack = 82;
    monsterDB.monsters[index].defense = 83;
    monsterDB.monsters[index].speed = 80;
    monsterDB.monsters[index].statusCondition = STATUS_NONE;
    monsterDB.monsters[index].statusCounter = 0;
    monsterDB.monsters[index].statusTurns = 0;
    monsterDB.monsters[index].next = NULL;
    monsterDB.monsters[index].prev = NULL;
    addAttackToMonster(&monsterDB.monsters[index], 0, "Vine Whip", TYPE_GRASS, 70, 100, 20, 0, 0, 0);
    addAttackToMonster(&monsterDB.monsters[index], 1, "Solar Beam", TYPE_GRASS, 120, 100, 10, 0, 0, 0);
    addAttackToMonster(&monsterDB.monsters[index], 2, "Sludge Bomb", TYPE_POISON, 90, 95, 10, 0, 0, 0);
    addAttackToMonster(&monsterDB.monsters[index], 3, "Sleep Powder", TYPE_GRASS, 0, 50, 5, STATUS_SLEEPING, 0, 10);
    index++;

    // Charizard (Fogo/Voador)
    strcpy(monsterDB.monsters[index].name, "Charizard");
    monsterDB.monsters[index].type1 = TYPE_FIRE;
    monsterDB.monsters[index].type2 = TYPE_FLYING;
    monsterDB.monsters[index].maxHp = 78;
    monsterDB.monsters[index].hp = 78;
    monsterDB.monsters[index].attack = 84;
    monsterDB.monsters[index].defense = 78;
    monsterDB.monsters[index].speed = 100;
    monsterDB.monsters[index].statusCondition = STATUS_NONE;
    monsterDB.monsters[index].statusCounter = 0;
    monsterDB.monsters[index].statusTurns = 0;
    monsterDB.monsters[index].next = NULL;
    monsterDB.monsters[index].prev = NULL;
    addAttackToMonster(&monsterDB.monsters[index], 0, "Flamethrower", TYPE_FIRE, 95, 100, 15, STATUS_BURNING, 0, 10);
    addAttackToMonster(&monsterDB.monsters[index], 1, "Air Slash", TYPE_FLYING, 75, 95, 20, 0, 0, 0);
    addAttackToMonster(&monsterDB.monsters[index], 2, "Dragon Claw", TYPE_DRAGON, 80, 100, 15, 0, 0, 0);
    addAttackToMonster(&monsterDB.monsters[index], 3, "Fire Blast", TYPE_FIRE, 120, 85, 5, STATUS_BURNING, 0, 30);
    index++;

    // Blastoise (Água)
    strcpy(monsterDB.monsters[index].name, "Blastoise");
    monsterDB.monsters[index].type1 = TYPE_WATER;
    monsterDB.monsters[index].type2 = TYPE_NONE;
    monsterDB.monsters[index].maxHp = 79;
    monsterDB.monsters[index].hp = 79;
    monsterDB.monsters[index].attack = 83;
    monsterDB.monsters[index].defense = 100;
    monsterDB.monsters[index].speed = 78;
    monsterDB.monsters[index].statusCondition = STATUS_NONE;
    monsterDB.monsters[index].statusCounter = 0;
    monsterDB.monsters[index].statusTurns = 0;
    monsterDB.monsters[index].next = NULL;
    monsterDB.monsters[index].prev = NULL;
    addAttackToMonster(&monsterDB.monsters[index], 0, "Water Gun", TYPE_WATER, 65, 100, 25, 0, 0, 0);
    addAttackToMonster(&monsterDB.monsters[index], 1, "Hydro Pump", TYPE_WATER, 120, 80, 5, 0, 0, 0);
    addAttackToMonster(&monsterDB.monsters[index], 2, "Ice Beam", TYPE_ICE, 90, 100, 10, 0, 0, 0);
    addAttackToMonster(&monsterDB.monsters[index], 3, "Skull Bash", TYPE_NORMAL, 100, 100, 15, 0, 0, 0);
    index++;

    // Caterpie (Inseto)
    strcpy(monsterDB.monsters[index].name, "Caterpie");
    monsterDB.monsters[index].type1 = TYPE_BUG;
    monsterDB.monsters[index].type2 = TYPE_NONE;
    monsterDB.monsters[index].maxHp = 45;
    monsterDB.monsters[index].hp = 45;
    monsterDB.monsters[index].attack = 30;
    monsterDB.monsters[index].defense = 35;
    monsterDB.monsters[index].speed = 45;
    monsterDB.monsters[index].statusCondition = STATUS_NONE;
    monsterDB.monsters[index].statusCounter = 0;
    monsterDB.monsters[index].statusTurns = 0;
    monsterDB.monsters[index].next = NULL;
    monsterDB.monsters[index].prev = NULL;
    addAttackToMonster(&monsterDB.monsters[index], 0, "Tackle", TYPE_NORMAL, 50, 100, 25, 0, 0, 0);
    addAttackToMonster(&monsterDB.monsters[index], 1, "String Shot", TYPE_BUG, 0, 95, 20, STATUS_SPD_DOWN, 20, 100);
    addAttackToMonster(&monsterDB.monsters[index], 2, "Bug Bite", TYPE_BUG, 60, 100, 20, 0, 0, 0);
    addAttackToMonster(&monsterDB.monsters[index], 3, "Struggle", TYPE_NORMAL, 40, 100, 10, 0, 0, 0);
    index++;

// Butterfree (Inseto/Voador)
strcpy(monsterDB.monsters[index].name, "Butterfree");
monsterDB.monsters[index].type1 = TYPE_BUG;
monsterDB.monsters[index].type2 = TYPE_FLYING;
monsterDB.monsters[index].maxHp = 60;
monsterDB.monsters[index].hp = 60;
monsterDB.monsters[index].attack = 45;
monsterDB.monsters[index].defense = 50;
monsterDB.monsters[index].speed = 70;
monsterDB.monsters[index].statusCondition = STATUS_NONE;
monsterDB.monsters[index].statusCounter = 0;
monsterDB.monsters[index].statusTurns = 0;
monsterDB.monsters[index].next = NULL;
monsterDB.monsters[index].prev = NULL;
addAttackToMonster(&monsterDB.monsters[index], 0, "Confusion", TYPE_PSYCHIC, 50, 100, 25, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 1, "Sleep Powder", TYPE_GRASS, 0, 50, 5, STATUS_SLEEPING, 0, 100);
addAttackToMonster(&monsterDB.monsters[index], 2, "Gust", TYPE_FLYING, 40, 100, 35, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 3, "Psychic", TYPE_PSYCHIC, 90, 100, 10, 0, 0, 0);
index++;

// Pidgeot (Normal/Voador)
strcpy(monsterDB.monsters[index].name, "Pidgeot");
monsterDB.monsters[index].type1 = TYPE_NORMAL;
monsterDB.monsters[index].type2 = TYPE_FLYING;
monsterDB.monsters[index].maxHp = 83;
monsterDB.monsters[index].hp = 83;
monsterDB.monsters[index].attack = 80;
monsterDB.monsters[index].defense = 75;
monsterDB.monsters[index].speed = 101;
monsterDB.monsters[index].statusCondition = STATUS_NONE;
monsterDB.monsters[index].statusCounter = 0;
monsterDB.monsters[index].statusTurns = 0;
monsterDB.monsters[index].next = NULL;
monsterDB.monsters[index].prev = NULL;
addAttackToMonster(&monsterDB.monsters[index], 0, "Quick Attack", TYPE_NORMAL, 40, 100, 30, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 1, "Wing Attack", TYPE_FLYING, 60, 100, 35, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 2, "Hurricane", TYPE_FLYING, 110, 70, 10, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 3, "Aerial Ace", TYPE_FLYING, 60, 100, 20, 0, 0, 0);
index++;

// Fearow (Normal/Voador)
strcpy(monsterDB.monsters[index].name, "Fearow");
monsterDB.monsters[index].type1 = TYPE_NORMAL;
monsterDB.monsters[index].type2 = TYPE_FLYING;
monsterDB.monsters[index].maxHp = 65;
monsterDB.monsters[index].hp = 65;
monsterDB.monsters[index].attack = 90;
monsterDB.monsters[index].defense = 65;
monsterDB.monsters[index].speed = 100;
monsterDB.monsters[index].statusCondition = STATUS_NONE;
monsterDB.monsters[index].statusCounter = 0;
monsterDB.monsters[index].statusTurns = 0;
monsterDB.monsters[index].next = NULL;
monsterDB.monsters[index].prev = NULL;
addAttackToMonster(&monsterDB.monsters[index], 0, "Drill Peck", TYPE_FLYING, 80, 100, 20, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 1, "Fury Attack", TYPE_NORMAL, 15, 85, 20, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 2, "Aerial Ace", TYPE_FLYING, 60, 100, 20, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 3, "Mirror Move", TYPE_FLYING, 0, 100, 20, 0, 0, 0);
index++;

// Pikachu (Elétrico)
strcpy(monsterDB.monsters[index].name, "Pikachu");
monsterDB.monsters[index].type1 = TYPE_ELECTRIC;
monsterDB.monsters[index].type2 = TYPE_NONE;
monsterDB.monsters[index].maxHp = 35;
monsterDB.monsters[index].hp = 35;
monsterDB.monsters[index].attack = 55;
monsterDB.monsters[index].defense = 40;
monsterDB.monsters[index].speed = 90;
monsterDB.monsters[index].statusCondition = STATUS_NONE;
monsterDB.monsters[index].statusCounter = 0;
monsterDB.monsters[index].statusTurns = 0;
monsterDB.monsters[index].next = NULL;
monsterDB.monsters[index].prev = NULL;
addAttackToMonster(&monsterDB.monsters[index], 0, "Thunder Shock", TYPE_ELECTRIC, 40, 100, 30, STATUS_PARALYZED, 0, 10);
addAttackToMonster(&monsterDB.monsters[index], 1, "Quick Attack", TYPE_NORMAL, 40, 100, 30, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 2, "Thunderbolt", TYPE_ELECTRIC, 90, 100, 15, STATUS_PARALYZED, 0, 10);
addAttackToMonster(&monsterDB.monsters[index], 3, "Iron Tail", TYPE_STEEL, 100, 75, 15, 0, 0, 0);
index++;

// Raichu (Elétrico)
strcpy(monsterDB.monsters[index].name, "Raichu");
monsterDB.monsters[index].type1 = TYPE_ELECTRIC;
monsterDB.monsters[index].type2 = TYPE_NONE;
monsterDB.monsters[index].maxHp = 60;
monsterDB.monsters[index].hp = 60;
monsterDB.monsters[index].attack = 90;
monsterDB.monsters[index].defense = 55;
monsterDB.monsters[index].speed = 110;
monsterDB.monsters[index].statusCondition = STATUS_NONE;
monsterDB.monsters[index].statusCounter = 0;
monsterDB.monsters[index].statusTurns = 0;
monsterDB.monsters[index].next = NULL;
monsterDB.monsters[index].prev = NULL;
addAttackToMonster(&monsterDB.monsters[index], 0, "Thunder", TYPE_ELECTRIC, 110, 70, 10, STATUS_PARALYZED, 0, 30);
addAttackToMonster(&monsterDB.monsters[index], 1, "Thunderbolt", TYPE_ELECTRIC, 90, 100, 15, STATUS_PARALYZED, 0, 10);
addAttackToMonster(&monsterDB.monsters[index], 2, "Quick Attack", TYPE_NORMAL, 40, 100, 30, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 3, "Thunder Wave", TYPE_ELECTRIC, 0, 90, 20, STATUS_PARALYZED, 0, 100);
index++;

// Ninetales (Fogo)
strcpy(monsterDB.monsters[index].name, "Ninetales");
monsterDB.monsters[index].type1 = TYPE_FIRE;
monsterDB.monsters[index].type2 = TYPE_NONE;
monsterDB.monsters[index].maxHp = 73;
monsterDB.monsters[index].hp = 73;
monsterDB.monsters[index].attack = 76;
monsterDB.monsters[index].defense = 75;
monsterDB.monsters[index].speed = 100;
monsterDB.monsters[index].statusCondition = STATUS_NONE;
monsterDB.monsters[index].statusCounter = 0;
monsterDB.monsters[index].statusTurns = 0;
monsterDB.monsters[index].next = NULL;
monsterDB.monsters[index].prev = NULL;
addAttackToMonster(&monsterDB.monsters[index], 0, "Flamethrower", TYPE_FIRE, 90, 100, 15, STATUS_BURNING, 0, 10);
addAttackToMonster(&monsterDB.monsters[index], 1, "Quick Attack", TYPE_NORMAL, 40, 100, 30, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 2, "Fire Spin", TYPE_FIRE, 35, 85, 15, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 3, "Confuse Ray", TYPE_GHOST, 0, 100, 10, 0, 0, 0);
index++;

// Golbat (Veneno/Voador)
strcpy(monsterDB.monsters[index].name, "Golbat");
monsterDB.monsters[index].type1 = TYPE_POISON;
monsterDB.monsters[index].type2 = TYPE_FLYING;
monsterDB.monsters[index].maxHp = 75;
monsterDB.monsters[index].hp = 75;
monsterDB.monsters[index].attack = 80;
monsterDB.monsters[index].defense = 70;
monsterDB.monsters[index].speed = 90;
monsterDB.monsters[index].statusCondition = STATUS_NONE;
monsterDB.monsters[index].statusCounter = 0;
monsterDB.monsters[index].statusTurns = 0;
monsterDB.monsters[index].next = NULL;
monsterDB.monsters[index].prev = NULL;
addAttackToMonster(&monsterDB.monsters[index], 0, "Wing Attack", TYPE_FLYING, 60, 100, 35, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 1, "Poison Fang", TYPE_POISON, 50, 100, 15, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 2, "Leech Life", TYPE_BUG, 80, 100, 10, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 3, "Air Cutter", TYPE_FLYING, 60, 95, 25, 0, 0, 0);
index++;

// Parasect (Inseto/Grama)
strcpy(monsterDB.monsters[index].name, "Parasect");
monsterDB.monsters[index].type1 = TYPE_BUG;
monsterDB.monsters[index].type2 = TYPE_GRASS;
monsterDB.monsters[index].maxHp = 60;
monsterDB.monsters[index].hp = 60;
monsterDB.monsters[index].attack = 95;
monsterDB.monsters[index].defense = 80;
monsterDB.monsters[index].speed = 30;
monsterDB.monsters[index].statusCondition = STATUS_NONE;
monsterDB.monsters[index].statusCounter = 0;
monsterDB.monsters[index].statusTurns = 0;
monsterDB.monsters[index].next = NULL;
monsterDB.monsters[index].prev = NULL;
addAttackToMonster(&monsterDB.monsters[index], 0, "Slash", TYPE_NORMAL, 70, 100, 20, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 1, "Spore", TYPE_GRASS, 0, 100, 15, STATUS_SLEEPING, 0, 100);
addAttackToMonster(&monsterDB.monsters[index], 2, "Giga Drain", TYPE_GRASS, 75, 100, 10, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 3, "X-Scissor", TYPE_BUG, 80, 100, 15, 0, 0, 0);
index++;

// Arcanine (Fogo)
strcpy(monsterDB.monsters[index].name, "Arcanine");
monsterDB.monsters[index].type1 = TYPE_FIRE;
monsterDB.monsters[index].type2 = TYPE_NONE;
monsterDB.monsters[index].maxHp = 90;
monsterDB.monsters[index].hp = 90;
monsterDB.monsters[index].attack = 110;
monsterDB.monsters[index].defense = 80;
monsterDB.monsters[index].speed = 95;
monsterDB.monsters[index].statusCondition = STATUS_NONE;
monsterDB.monsters[index].statusCounter = 0;
monsterDB.monsters[index].statusTurns = 0;
monsterDB.monsters[index].next = NULL;
monsterDB.monsters[index].prev = NULL;
addAttackToMonster(&monsterDB.monsters[index], 0, "Flamethrower", TYPE_FIRE, 90, 100, 15, STATUS_BURNING, 0, 10);
addAttackToMonster(&monsterDB.monsters[index], 1, "Extreme Speed", TYPE_NORMAL, 80, 100, 5, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 2, "Crunch", TYPE_DARK, 80, 100, 15, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 3, "Fire Blast", TYPE_FIRE, 110, 85, 5, STATUS_BURNING, 0, 10);
index++;

// Alakazam (Psíquico)
strcpy(monsterDB.monsters[index].name, "Alakazam");
monsterDB.monsters[index].type1 = TYPE_PSYCHIC;
monsterDB.monsters[index].type2 = TYPE_NONE;
monsterDB.monsters[index].maxHp = 55;
monsterDB.monsters[index].hp = 55;
monsterDB.monsters[index].attack = 50;
monsterDB.monsters[index].defense = 45;
monsterDB.monsters[index].speed = 120;
monsterDB.monsters[index].statusCondition = STATUS_NONE;
monsterDB.monsters[index].statusCounter = 0;
monsterDB.monsters[index].statusTurns = 0;
monsterDB.monsters[index].next = NULL;
monsterDB.monsters[index].prev = NULL;
addAttackToMonster(&monsterDB.monsters[index], 0, "Psychic", TYPE_PSYCHIC, 90, 100, 10, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 1, "Psybeam", TYPE_PSYCHIC, 65, 100, 20, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 2, "Shadow Ball", TYPE_GHOST, 80, 100, 15, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 3, "Calm Mind", TYPE_PSYCHIC, 0, 100, 20, 0, 0, 0);
index++;

// Machamp (Lutador)
strcpy(monsterDB.monsters[index].name, "Machamp");
monsterDB.monsters[index].type1 = TYPE_FIGHTING;
monsterDB.monsters[index].type2 = TYPE_NONE;
monsterDB.monsters[index].maxHp = 90;
monsterDB.monsters[index].hp = 90;
monsterDB.monsters[index].attack = 130;
monsterDB.monsters[index].defense = 80;
monsterDB.monsters[index].speed = 55;
monsterDB.monsters[index].statusCondition = STATUS_NONE;
monsterDB.monsters[index].statusCounter = 0;
monsterDB.monsters[index].statusTurns = 0;
monsterDB.monsters[index].next = NULL;
monsterDB.monsters[index].prev = NULL;
addAttackToMonster(&monsterDB.monsters[index], 0, "Cross Chop", TYPE_FIGHTING, 100, 80, 5, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 1, "Dynamic Punch", TYPE_FIGHTING, 100, 50, 5, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 2, "Rock Slide", TYPE_ROCK, 75, 90, 10, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 3, "Earthquake", TYPE_GROUND, 100, 100, 10, 0, 0, 0);
index++;

// Victreebel (Grama/Veneno)
strcpy(monsterDB.monsters[index].name, "VictreeBel");
monsterDB.monsters[index].type1 = TYPE_GRASS;
monsterDB.monsters[index].type2 = TYPE_POISON;
monsterDB.monsters[index].maxHp = 80;
monsterDB.monsters[index].hp = 80;
monsterDB.monsters[index].attack = 105;
monsterDB.monsters[index].defense = 65;
monsterDB.monsters[index].speed = 70;
monsterDB.monsters[index].statusCondition = STATUS_NONE;
monsterDB.monsters[index].statusCounter = 0;
monsterDB.monsters[index].statusTurns = 0;
monsterDB.monsters[index].next = NULL;
monsterDB.monsters[index].prev = NULL;
addAttackToMonster(&monsterDB.monsters[index], 0, "Razor Leaf", TYPE_GRASS, 55, 95, 25, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 1, "Acid", TYPE_POISON, 40, 100, 30, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 2, "Leaf Blade", TYPE_GRASS, 90, 100, 15, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 3, "Sleep Powder", TYPE_GRASS, 0, 75, 15, STATUS_SLEEPING, 0, 100);
index++;

// Tentacruel (Água/Veneno)
strcpy(monsterDB.monsters[index].name, "Tentacruel");
monsterDB.monsters[index].type1 = TYPE_WATER;
monsterDB.monsters[index].type2 = TYPE_POISON;
monsterDB.monsters[index].maxHp = 80;
monsterDB.monsters[index].hp = 80;
monsterDB.monsters[index].attack = 70;
monsterDB.monsters[index].defense = 65;
monsterDB.monsters[index].speed = 100;
monsterDB.monsters[index].statusCondition = STATUS_NONE;
monsterDB.monsters[index].statusCounter = 0;
monsterDB.monsters[index].statusTurns = 0;
monsterDB.monsters[index].next = NULL;
monsterDB.monsters[index].prev = NULL;
addAttackToMonster(&monsterDB.monsters[index], 0, "Hydro Pump", TYPE_WATER, 110, 80, 5, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 1, "Toxic", TYPE_POISON, 0, 90, 10, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 2, "Surf", TYPE_WATER, 90, 100, 15, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 3, "Poison Jab", TYPE_POISON, 80, 100, 20, 0, 0, 0);
index++;

// Golem (Pedra/Terra)
strcpy(monsterDB.monsters[index].name, "Golem");
monsterDB.monsters[index].type1 = TYPE_ROCK;
monsterDB.monsters[index].type2 = TYPE_GROUND;
monsterDB.monsters[index].maxHp = 80;
monsterDB.monsters[index].hp = 80;
monsterDB.monsters[index].attack = 120;
monsterDB.monsters[index].defense = 130;
monsterDB.monsters[index].speed = 45;
monsterDB.monsters[index].statusCondition = STATUS_NONE;
monsterDB.monsters[index].statusCounter = 0;
monsterDB.monsters[index].statusTurns = 0;
monsterDB.monsters[index].next = NULL;
monsterDB.monsters[index].prev = NULL;
addAttackToMonster(&monsterDB.monsters[index], 0, "Earthquake", TYPE_GROUND, 100, 100, 10, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 1, "Rock Slide", TYPE_ROCK, 75, 90, 10, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 2, "Stone Edge", TYPE_ROCK, 100, 80, 5, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 3, "Explosion", TYPE_NORMAL, 250, 100, 5, 0, 0, 0);
index++;

// Rapidash (Fogo)
strcpy(monsterDB.monsters[index].name, "Rapidash");
monsterDB.monsters[index].type1 = TYPE_FIRE;
monsterDB.monsters[index].type2 = TYPE_NONE;
monsterDB.monsters[index].maxHp = 65;
monsterDB.monsters[index].hp = 65;
monsterDB.monsters[index].attack = 100;
monsterDB.monsters[index].defense = 70;
monsterDB.monsters[index].speed = 105;
monsterDB.monsters[index].statusCondition = STATUS_NONE;
monsterDB.monsters[index].statusCounter = 0;
monsterDB.monsters[index].statusTurns = 0;
monsterDB.monsters[index].next = NULL;
monsterDB.monsters[index].prev = NULL;
addAttackToMonster(&monsterDB.monsters[index], 0, "Fire Blast", TYPE_FIRE, 110, 85, 5, STATUS_BURNING, 0, 10);
addAttackToMonster(&monsterDB.monsters[index], 1, "Flare Blitz", TYPE_FIRE, 120, 100, 15, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 2, "Stomp", TYPE_NORMAL, 65, 100, 20, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 3, "Megahorn", TYPE_BUG, 120, 85, 10, 0, 0, 0);
index++;

// Slowbro (Água/Psíquico)
strcpy(monsterDB.monsters[index].name, "SlowBro");
monsterDB.monsters[index].type1 = TYPE_WATER;
monsterDB.monsters[index].type2 = TYPE_PSYCHIC;
monsterDB.monsters[index].maxHp = 95;
monsterDB.monsters[index].hp = 95;
monsterDB.monsters[index].attack = 75;
monsterDB.monsters[index].defense = 110;
monsterDB.monsters[index].speed = 30;
monsterDB.monsters[index].statusCondition = STATUS_NONE;
monsterDB.monsters[index].statusCounter = 0;
monsterDB.monsters[index].statusTurns = 0;
monsterDB.monsters[index].next = NULL;
monsterDB.monsters[index].prev = NULL;
addAttackToMonster(&monsterDB.monsters[index], 0, "Surf", TYPE_WATER, 90, 100, 15, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 1, "Psychic", TYPE_PSYCHIC, 90, 100, 10, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 2, "Ice Beam", TYPE_ICE, 90, 100, 10, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 3, "Calm Mind", TYPE_PSYCHIC, 0, 100, 20, 0, 0, 0);
index++;

// Magneton (Elétrico/Aço)
strcpy(monsterDB.monsters[index].name, "Magneton");
monsterDB.monsters[index].type1 = TYPE_ELECTRIC;
monsterDB.monsters[index].type2 = TYPE_STEEL;
monsterDB.monsters[index].maxHp = 50;
monsterDB.monsters[index].hp = 50;
monsterDB.monsters[index].attack = 60;
monsterDB.monsters[index].defense = 95;
monsterDB.monsters[index].speed = 70;
monsterDB.monsters[index].statusCondition = STATUS_NONE;
monsterDB.monsters[index].statusCounter = 0;
monsterDB.monsters[index].statusTurns = 0;
monsterDB.monsters[index].next = NULL;
monsterDB.monsters[index].prev = NULL;
addAttackToMonster(&monsterDB.monsters[index], 0, "Thunderbolt", TYPE_ELECTRIC, 90, 100, 15, STATUS_PARALYZED, 0, 10);
addAttackToMonster(&monsterDB.monsters[index], 1, "Flash Cannon", TYPE_STEEL, 80, 100, 10, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 2, "Thunder Wave", TYPE_ELECTRIC, 0, 90, 20, STATUS_PARALYZED, 0, 100);
addAttackToMonster(&monsterDB.monsters[index], 3, "Tri Attack", TYPE_NORMAL, 80, 100, 10, 0, 0, 0);
index++;

// Gengar (Fantasma/Veneno)
strcpy(monsterDB.monsters[index].name, "Gengar");
monsterDB.monsters[index].type1 = TYPE_GHOST;
monsterDB.monsters[index].type2 = TYPE_POISON;
monsterDB.monsters[index].maxHp = 60;
monsterDB.monsters[index].hp = 60;
monsterDB.monsters[index].attack = 65;
monsterDB.monsters[index].defense = 60;
monsterDB.monsters[index].speed = 110;
monsterDB.monsters[index].statusCondition = STATUS_NONE;
monsterDB.monsters[index].statusCounter = 0;
monsterDB.monsters[index].statusTurns = 0;
monsterDB.monsters[index].next = NULL;
monsterDB.monsters[index].prev = NULL;
addAttackToMonster(&monsterDB.monsters[index], 0, "Shadow Ball", TYPE_GHOST, 80, 100, 15, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 1, "Sludge Bomb", TYPE_POISON, 90, 100, 10, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 2, "Dream Eater", TYPE_PSYCHIC, 100, 100, 15, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 3, "Hypnosis", TYPE_PSYCHIC, 0, 60, 20, STATUS_SLEEPING, 0, 100);
index++;

// Onix (Pedra/Terra)
strcpy(monsterDB.monsters[index].name, "Onix");
monsterDB.monsters[index].type1 = TYPE_ROCK;
monsterDB.monsters[index].type2 = TYPE_GROUND;
monsterDB.monsters[index].maxHp = 35;
monsterDB.monsters[index].hp = 35;
monsterDB.monsters[index].attack = 45;
monsterDB.monsters[index].defense = 160;
monsterDB.monsters[index].speed = 70;
monsterDB.monsters[index].statusCondition = STATUS_NONE;
monsterDB.monsters[index].statusCounter = 0;
monsterDB.monsters[index].statusTurns = 0;
monsterDB.monsters[index].next = NULL;
monsterDB.monsters[index].prev = NULL;
addAttackToMonster(&monsterDB.monsters[index], 0, "Rock Slide", TYPE_ROCK, 75, 90, 10, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 1, "Earthquake", TYPE_GROUND, 100, 100, 10, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 2, "Sandstorm", TYPE_ROCK, 0, 100, 10, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 3, "Iron Tail", TYPE_STEEL, 100, 75, 15, 0, 0, 0);
index++;

// Exeggutor (Grama/Psíquico)
strcpy(monsterDB.monsters[index].name, "Exeggutor");
monsterDB.monsters[index].type1 = TYPE_GRASS;
monsterDB.monsters[index].type2 = TYPE_PSYCHIC;
monsterDB.monsters[index].maxHp = 95;
monsterDB.monsters[index].hp = 95;
monsterDB.monsters[index].attack = 95;
monsterDB.monsters[index].defense = 85;
monsterDB.monsters[index].speed = 55;
monsterDB.monsters[index].statusCondition = STATUS_NONE;
monsterDB.monsters[index].statusCounter = 0;
monsterDB.monsters[index].statusTurns = 0;
monsterDB.monsters[index].next = NULL;
monsterDB.monsters[index].prev = NULL;
addAttackToMonster(&monsterDB.monsters[index], 0, "Solar Beam", TYPE_GRASS, 120, 100, 10, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 1, "Psychic", TYPE_PSYCHIC, 90, 100, 10, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 2, "Sleep Powder", TYPE_GRASS, 0, 75, 15, STATUS_SLEEPING, 0, 100);
addAttackToMonster(&monsterDB.monsters[index], 3, "Egg Bomb", TYPE_NORMAL, 100, 75, 10, 0, 0, 0);
index++;

// Hitmonlee (Lutador)
strcpy(monsterDB.monsters[index].name, "Hitmonlee");
monsterDB.monsters[index].type1 = TYPE_FIGHTING;
monsterDB.monsters[index].type2 = TYPE_NONE;
monsterDB.monsters[index].maxHp = 50;
monsterDB.monsters[index].hp = 50;
monsterDB.monsters[index].attack = 120;
monsterDB.monsters[index].defense = 53;
monsterDB.monsters[index].speed = 87;
monsterDB.monsters[index].statusCondition = STATUS_NONE;
monsterDB.monsters[index].statusCounter = 0;
monsterDB.monsters[index].statusTurns = 0;
monsterDB.monsters[index].next = NULL;
monsterDB.monsters[index].prev = NULL;
addAttackToMonster(&monsterDB.monsters[index], 0, "High Jump Kick", TYPE_FIGHTING, 130, 90, 10, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 1, "Mega Kick", TYPE_NORMAL, 120, 75, 5, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 2, "Rock Slide", TYPE_ROCK, 75, 90, 10, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 3, "Brick Break", TYPE_FIGHTING, 75, 100, 15, 0, 0, 0);
index++;

// Hitmonchan (Lutador)
strcpy(monsterDB.monsters[index].name, "Hitmonchan");
monsterDB.monsters[index].type1 = TYPE_FIGHTING;
monsterDB.monsters[index].type2 = TYPE_NONE;
monsterDB.monsters[index].maxHp = 50;
monsterDB.monsters[index].hp = 50;
monsterDB.monsters[index].attack = 105;
monsterDB.monsters[index].defense = 79;
monsterDB.monsters[index].speed = 76;
monsterDB.monsters[index].statusCondition = STATUS_NONE;
monsterDB.monsters[index].statusCounter = 0;
monsterDB.monsters[index].statusTurns = 0;
monsterDB.monsters[index].next = NULL;
monsterDB.monsters[index].prev = NULL;
addAttackToMonster(&monsterDB.monsters[index], 0, "Mach Punch", TYPE_FIGHTING, 40, 100, 30, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 1, "Fire Punch", TYPE_FIRE, 75, 100, 15, STATUS_BURNING, 0, 10);
addAttackToMonster(&monsterDB.monsters[index], 2, "Ice Punch", TYPE_ICE, 75, 100, 15, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 3, "Thunder Punch", TYPE_ELECTRIC, 75, 100, 15, STATUS_PARALYZED, 0, 10);
index++;

// Weezing (Veneno)
strcpy(monsterDB.monsters[index].name, "Weezing");
monsterDB.monsters[index].type1 = TYPE_POISON;
monsterDB.monsters[index].type2 = TYPE_NONE;
monsterDB.monsters[index].maxHp = 65;
monsterDB.monsters[index].hp = 65;
monsterDB.monsters[index].attack = 90;
monsterDB.monsters[index].defense = 120;
monsterDB.monsters[index].speed = 60;
monsterDB.monsters[index].statusCondition = STATUS_NONE;
monsterDB.monsters[index].statusCounter = 0;
monsterDB.monsters[index].statusTurns = 0;
monsterDB.monsters[index].next = NULL;
monsterDB.monsters[index].prev = NULL;
addAttackToMonster(&monsterDB.monsters[index], 0, "Sludge Bomb", TYPE_POISON, 90, 100, 10, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 1, "Toxic", TYPE_POISON, 0, 90, 10, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 2, "Thunderbolt", TYPE_ELECTRIC, 90, 100, 15, STATUS_PARALYZED, 0, 10);
addAttackToMonster(&monsterDB.monsters[index], 3, "Self-Destruct", TYPE_NORMAL, 200, 100, 5, 0, 0, 0);
index++;

// Rhydon (Terra/Pedra)
strcpy(monsterDB.monsters[index].name, "Rhydon");
monsterDB.monsters[index].type1 = TYPE_GROUND;
monsterDB.monsters[index].type2 = TYPE_ROCK;
monsterDB.monsters[index].maxHp = 105;
monsterDB.monsters[index].hp = 105;
monsterDB.monsters[index].attack = 130;
monsterDB.monsters[index].defense = 120;
monsterDB.monsters[index].speed = 40;
monsterDB.monsters[index].statusCondition = STATUS_NONE;
monsterDB.monsters[index].statusCounter = 0;
monsterDB.monsters[index].statusTurns = 0;
monsterDB.monsters[index].next = NULL;
monsterDB.monsters[index].prev = NULL;
addAttackToMonster(&monsterDB.monsters[index], 0, "Earthquake", TYPE_GROUND, 100, 100, 10, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 1, "Rock Slide", TYPE_ROCK, 75, 90, 10, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 2, "Horn Drill", TYPE_NORMAL, 0, 30, 5, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 3, "Megahorn", TYPE_BUG, 120, 85, 10, 0, 0, 0);
index++;

// Chansey (Normal)
strcpy(monsterDB.monsters[index].name, "Chansey");
monsterDB.monsters[index].type1 = TYPE_NORMAL;
monsterDB.monsters[index].type2 = TYPE_NONE;
monsterDB.monsters[index].maxHp = 250;
monsterDB.monsters[index].hp = 250;
monsterDB.monsters[index].attack = 5;
monsterDB.monsters[index].defense = 5;
monsterDB.monsters[index].speed = 50;
monsterDB.monsters[index].statusCondition = STATUS_NONE;
monsterDB.monsters[index].statusCounter = 0;
monsterDB.monsters[index].statusTurns = 0;
monsterDB.monsters[index].next = NULL;
monsterDB.monsters[index].prev = NULL;
addAttackToMonster(&monsterDB.monsters[index], 0, "Soft-Boiled", TYPE_NORMAL, 0, 100, 10, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 1, "Ice Beam", TYPE_ICE, 90, 100, 10, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 2, "Thunderbolt", TYPE_ELECTRIC, 90, 100, 15, STATUS_PARALYZED, 0, 10);
addAttackToMonster(&monsterDB.monsters[index], 3, "Seismic Toss", TYPE_FIGHTING, 0, 100, 20, 0, 0, 0);
index++;


// Kangaskhan (Normal)
strcpy(monsterDB.monsters[index].name, "Kangaskhan");
monsterDB.monsters[index].type1 = TYPE_NORMAL;
monsterDB.monsters[index].type2 = TYPE_NONE;
monsterDB.monsters[index].maxHp = 105;
monsterDB.monsters[index].hp = 105;
monsterDB.monsters[index].attack = 95;
monsterDB.monsters[index].defense = 80;
monsterDB.monsters[index].speed = 90;
monsterDB.monsters[index].statusCondition = STATUS_NONE;
monsterDB.monsters[index].statusCounter = 0;
monsterDB.monsters[index].statusTurns = 0;
monsterDB.monsters[index].next = NULL;
monsterDB.monsters[index].prev = NULL;
addAttackToMonster(&monsterDB.monsters[index], 0, "Body Slam", TYPE_NORMAL, 85, 100, 15, STATUS_PARALYZED, 0, 30);
addAttackToMonster(&monsterDB.monsters[index], 1, "Earthquake", TYPE_GROUND, 100, 100, 10, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 2, "Crunch", TYPE_DARK, 80, 100, 15, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 3, "Outrage", TYPE_DRAGON, 120, 100, 10, 0, 0, 0);
index++;

// Scyther (Inseto/Voador)
strcpy(monsterDB.monsters[index].name, "Scyther");
monsterDB.monsters[index].type1 = TYPE_BUG;
monsterDB.monsters[index].type2 = TYPE_FLYING;
monsterDB.monsters[index].maxHp = 70;
monsterDB.monsters[index].hp = 70;
monsterDB.monsters[index].attack = 110;
monsterDB.monsters[index].defense = 80;
monsterDB.monsters[index].speed = 105;
monsterDB.monsters[index].statusCondition = STATUS_NONE;
monsterDB.monsters[index].statusCounter = 0;
monsterDB.monsters[index].statusTurns = 0;
monsterDB.monsters[index].next = NULL;
monsterDB.monsters[index].prev = NULL;
addAttackToMonster(&monsterDB.monsters[index], 0, "X-Scissor", TYPE_BUG, 80, 100, 15, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 1, "Aerial Ace", TYPE_FLYING, 60, 100, 20, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 2, "Slash", TYPE_NORMAL, 70, 100, 20, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 3, "Swords Dance", TYPE_NORMAL, 0, 100, 20, 0, 0, 0);
index++;


// Electabuzz (Elétrico)
strcpy(monsterDB.monsters[index].name, "Electabuzz");
monsterDB.monsters[index].type1 = TYPE_ELECTRIC;
monsterDB.monsters[index].type2 = TYPE_NONE;
monsterDB.monsters[index].maxHp = 65;
monsterDB.monsters[index].hp = 65;
monsterDB.monsters[index].attack = 83;
monsterDB.monsters[index].defense = 57;
monsterDB.monsters[index].speed = 105;
monsterDB.monsters[index].statusCondition = STATUS_NONE;
monsterDB.monsters[index].statusCounter = 0;
monsterDB.monsters[index].statusTurns = 0;
monsterDB.monsters[index].next = NULL;
monsterDB.monsters[index].prev = NULL;
addAttackToMonster(&monsterDB.monsters[index], 0, "Thunder Punch", TYPE_ELECTRIC, 75, 100, 15, STATUS_PARALYZED, 0, 10);
addAttackToMonster(&monsterDB.monsters[index], 1, "Thunderbolt", TYPE_ELECTRIC, 90, 100, 15, STATUS_PARALYZED, 0, 10);
addAttackToMonster(&monsterDB.monsters[index], 2, "Ice Punch", TYPE_ICE, 75, 100, 15, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 3, "Psychic", TYPE_PSYCHIC, 90, 100, 10, 0, 0, 0);
index++;


// Magmar (Fogo)
strcpy(monsterDB.monsters[index].name, "Magmar");
monsterDB.monsters[index].type1 = TYPE_FIRE;
monsterDB.monsters[index].type2 = TYPE_NONE;
monsterDB.monsters[index].maxHp = 65;
monsterDB.monsters[index].hp = 65;
monsterDB.monsters[index].attack = 95;
monsterDB.monsters[index].defense = 57;
monsterDB.monsters[index].speed = 93;
monsterDB.monsters[index].statusCondition = STATUS_NONE;
monsterDB.monsters[index].statusCounter = 0;
monsterDB.monsters[index].statusTurns = 0;
monsterDB.monsters[index].next = NULL;
monsterDB.monsters[index].prev = NULL;
addAttackToMonster(&monsterDB.monsters[index], 0, "Fire Blast", TYPE_FIRE, 110, 85, 5, STATUS_BURNING, 0, 10);
addAttackToMonster(&monsterDB.monsters[index], 1, "Fire Punch", TYPE_FIRE, 75, 100, 15, STATUS_BURNING, 0, 10);
addAttackToMonster(&monsterDB.monsters[index], 2, "Thunderbolt", TYPE_ELECTRIC, 90, 100, 15, STATUS_PARALYZED, 0, 10);
addAttackToMonster(&monsterDB.monsters[index], 3, "Psychic", TYPE_PSYCHIC, 90, 100, 10, 0, 0, 0);
index++;

// Gyarados (Água/Voador)
strcpy(monsterDB.monsters[index].name, "Gyarados");
monsterDB.monsters[index].type1 = TYPE_WATER;
monsterDB.monsters[index].type2 = TYPE_FLYING;
monsterDB.monsters[index].maxHp = 95;
monsterDB.monsters[index].hp = 95;
monsterDB.monsters[index].attack = 125;
monsterDB.monsters[index].defense = 79;
monsterDB.monsters[index].speed = 81;
monsterDB.monsters[index].statusCondition = STATUS_NONE;
monsterDB.monsters[index].statusCounter = 0;
monsterDB.monsters[index].statusTurns = 0;
monsterDB.monsters[index].next = NULL;
monsterDB.monsters[index].prev = NULL;
addAttackToMonster(&monsterDB.monsters[index], 0, "Hydro Pump", TYPE_WATER, 110, 80, 5, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 1, "Dragon Rage", TYPE_DRAGON, 0, 100, 10, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 2, "Ice Beam", TYPE_ICE, 90, 100, 10, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 3, "Hyper Beam", TYPE_NORMAL, 150, 90, 5, 0, 0, 0);
index++;

// Eevee (Normal)
strcpy(monsterDB.monsters[index].name, "Eevee");
monsterDB.monsters[index].type1 = TYPE_NORMAL;
monsterDB.monsters[index].type2 = TYPE_NONE;
monsterDB.monsters[index].maxHp = 55;
monsterDB.monsters[index].hp = 55;
monsterDB.monsters[index].attack = 55;
monsterDB.monsters[index].defense = 50;
monsterDB.monsters[index].speed = 55;
monsterDB.monsters[index].statusCondition = STATUS_NONE;
monsterDB.monsters[index].statusCounter = 0;
monsterDB.monsters[index].statusTurns = 0;
monsterDB.monsters[index].next = NULL;
monsterDB.monsters[index].prev = NULL;
addAttackToMonster(&monsterDB.monsters[index], 0, "Quick Attack", TYPE_NORMAL, 40, 100, 30, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 1, "Bite", TYPE_DARK, 60, 100, 25, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 2, "Take Down", TYPE_NORMAL, 90, 85, 20, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 3, "Sand Attack", TYPE_GROUND, 0, 100, 15, 0, 0, 0);
index++;

// Vaporeon (Água)
strcpy(monsterDB.monsters[index].name, "Vaporeon");
monsterDB.monsters[index].type1 = TYPE_WATER;
monsterDB.monsters[index].type2 = TYPE_NONE;
monsterDB.monsters[index].maxHp = 130;
monsterDB.monsters[index].hp = 130;
monsterDB.monsters[index].attack = 65;
monsterDB.monsters[index].defense = 60;
monsterDB.monsters[index].speed = 65;
monsterDB.monsters[index].statusCondition = STATUS_NONE;
monsterDB.monsters[index].statusCounter = 0;
monsterDB.monsters[index].statusTurns = 0;
monsterDB.monsters[index].next = NULL;
monsterDB.monsters[index].prev = NULL;
addAttackToMonster(&monsterDB.monsters[index], 0, "Hydro Pump", TYPE_WATER, 110, 80, 5, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 1, "Ice Beam", TYPE_ICE, 90, 100, 10, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 2, "Surf", TYPE_WATER, 90, 100, 15, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 3, "Aurora Beam", TYPE_ICE, 65, 100, 20, 0, 0, 0);
index++;

// Jolteon (Elétrico)
strcpy(monsterDB.monsters[index].name, "Jolteon");
monsterDB.monsters[index].type1 = TYPE_ELECTRIC;
monsterDB.monsters[index].type2 = TYPE_NONE;
monsterDB.monsters[index].maxHp = 65;
monsterDB.monsters[index].hp = 65;
monsterDB.monsters[index].attack = 65;
monsterDB.monsters[index].defense = 60;
monsterDB.monsters[index].speed = 130;
monsterDB.monsters[index].statusCondition = STATUS_NONE;
monsterDB.monsters[index].statusCounter = 0;
monsterDB.monsters[index].statusTurns = 0;
monsterDB.monsters[index].next = NULL;
monsterDB.monsters[index].prev = NULL;
addAttackToMonster(&monsterDB.monsters[index], 0, "Thunderbolt", TYPE_ELECTRIC, 90, 100, 15, STATUS_PARALYZED, 0, 10);
addAttackToMonster(&monsterDB.monsters[index], 1, "Thunder Wave", TYPE_ELECTRIC, 0, 90, 20, STATUS_PARALYZED, 0, 100);
addAttackToMonster(&monsterDB.monsters[index], 2, "Pin Missile", TYPE_BUG, 25, 95, 20, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 3, "Quick Attack", TYPE_NORMAL, 40, 100, 30, 0, 0, 0);
index++;

// Flareon (Fogo)
strcpy(monsterDB.monsters[index].name, "Flareon");
monsterDB.monsters[index].type1 = TYPE_FIRE;
monsterDB.monsters[index].type2 = TYPE_NONE;
monsterDB.monsters[index].maxHp = 65;
monsterDB.monsters[index].hp = 65;
monsterDB.monsters[index].attack = 130;
monsterDB.monsters[index].defense = 60;
monsterDB.monsters[index].speed = 65;
monsterDB.monsters[index].statusCondition = STATUS_NONE;
monsterDB.monsters[index].statusCounter = 0;
monsterDB.monsters[index].statusTurns = 0;
monsterDB.monsters[index].next = NULL;
monsterDB.monsters[index].prev = NULL;
addAttackToMonster(&monsterDB.monsters[index], 0, "Flamethrower", TYPE_FIRE, 90, 100, 15, STATUS_BURNING, 0, 10);
addAttackToMonster(&monsterDB.monsters[index], 1, "Fire Spin", TYPE_FIRE, 35, 85, 15, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 2, "Quick Attack", TYPE_NORMAL, 40, 100, 30, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 3, "Bite", TYPE_DARK, 60, 100, 25, 0, 0, 0);
index++;

// Porygon (Normal)
strcpy(monsterDB.monsters[index].name, "Porygon");
monsterDB.monsters[index].type1 = TYPE_NORMAL;
monsterDB.monsters[index].type2 = TYPE_NONE;
monsterDB.monsters[index].maxHp = 65;
monsterDB.monsters[index].hp = 65;
monsterDB.monsters[index].attack = 60;
monsterDB.monsters[index].defense = 70;
monsterDB.monsters[index].speed = 40;
monsterDB.monsters[index].statusCondition = STATUS_NONE;
monsterDB.monsters[index].statusCounter = 0;
monsterDB.monsters[index].statusTurns = 0;
monsterDB.monsters[index].next = NULL;
monsterDB.monsters[index].prev = NULL;
addAttackToMonster(&monsterDB.monsters[index], 0, "Tri Attack", TYPE_NORMAL, 80, 100, 10, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 1, "Psychic", TYPE_PSYCHIC, 90, 100, 10, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 2, "Thunderbolt", TYPE_ELECTRIC, 90, 100, 15, STATUS_PARALYZED, 0, 10);
addAttackToMonster(&monsterDB.monsters[index], 3, "Ice Beam", TYPE_ICE, 90, 100, 10, 0, 0, 0);
index++;

// Snorlax (Normal)
strcpy(monsterDB.monsters[index].name, "Snorlax");
monsterDB.monsters[index].type1 = TYPE_NORMAL;
monsterDB.monsters[index].type2 = TYPE_NONE;
monsterDB.monsters[index].maxHp = 160;
monsterDB.monsters[index].hp = 160;
monsterDB.monsters[index].attack = 110;
monsterDB.monsters[index].defense = 65;
monsterDB.monsters[index].speed = 30;
monsterDB.monsters[index].statusCondition = STATUS_NONE;
monsterDB.monsters[index].statusCounter = 0;
monsterDB.monsters[index].statusTurns = 0;
monsterDB.monsters[index].next = NULL;
monsterDB.monsters[index].prev = NULL;
addAttackToMonster(&monsterDB.monsters[index], 0, "Body Slam", TYPE_NORMAL, 85, 100, 15, STATUS_PARALYZED, 0, 30);
addAttackToMonster(&monsterDB.monsters[index], 1, "Earthquake", TYPE_GROUND, 100, 100, 10, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 2, "Hyper Beam", TYPE_NORMAL, 150, 90, 5, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 3, "Rest", TYPE_PSYCHIC, 0, 100, 10, 0, 0, 0);
index++;

// Articuno (Gelo/Voador)
strcpy(monsterDB.monsters[index].name, "Articuno");
monsterDB.monsters[index].type1 = TYPE_ICE;
monsterDB.monsters[index].type2 = TYPE_FLYING;
monsterDB.monsters[index].maxHp = 90;
monsterDB.monsters[index].hp = 90;
monsterDB.monsters[index].attack = 85;
monsterDB.monsters[index].defense = 100;
monsterDB.monsters[index].speed = 85;
monsterDB.monsters[index].statusCondition = STATUS_NONE;
monsterDB.monsters[index].statusCounter = 0;
monsterDB.monsters[index].statusTurns = 0;
monsterDB.monsters[index].next = NULL;
monsterDB.monsters[index].prev = NULL;
addAttackToMonster(&monsterDB.monsters[index], 0, "Ice Beam", TYPE_ICE, 90, 100, 10, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 1, "Blizzard", TYPE_ICE, 110, 70, 5, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 2, "Hurricane", TYPE_FLYING, 110, 70, 10, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 3, "Ancient Power", TYPE_ROCK, 60, 100, 5, 0, 0, 0);
index++;

// Zapdos (Elétrico/Voador)
strcpy(monsterDB.monsters[index].name, "Zapdos");
monsterDB.monsters[index].type1 = TYPE_ELECTRIC;
monsterDB.monsters[index].type2 = TYPE_FLYING;
monsterDB.monsters[index].maxHp = 90;
monsterDB.monsters[index].hp = 90;
monsterDB.monsters[index].attack = 90;
monsterDB.monsters[index].defense = 85;
monsterDB.monsters[index].speed = 100;
monsterDB.monsters[index].statusCondition = STATUS_NONE;
monsterDB.monsters[index].statusCounter = 0;
monsterDB.monsters[index].statusTurns = 0;
monsterDB.monsters[index].next = NULL;
monsterDB.monsters[index].prev = NULL;
addAttackToMonster(&monsterDB.monsters[index], 0, "Thunderbolt", TYPE_ELECTRIC, 90, 100, 15, STATUS_PARALYZED, 0, 10);
addAttackToMonster(&monsterDB.monsters[index], 1, "Thunder", TYPE_ELECTRIC, 110, 70, 10, STATUS_PARALYZED, 0, 30);
addAttackToMonster(&monsterDB.monsters[index], 2, "Drill Peck", TYPE_FLYING, 80, 100, 20, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 3, "Ancient Power", TYPE_ROCK, 60, 100, 5, 0, 0, 0);
index++;

// Moltres (Fogo/Voador)
strcpy(monsterDB.monsters[index].name, "Moltres");
monsterDB.monsters[index].type1 = TYPE_FIRE;
monsterDB.monsters[index].type2 = TYPE_FLYING;
monsterDB.monsters[index].maxHp = 90;
monsterDB.monsters[index].hp = 90;
monsterDB.monsters[index].attack = 100;
monsterDB.monsters[index].defense = 90;
monsterDB.monsters[index].speed = 90;
monsterDB.monsters[index].statusCondition = STATUS_NONE;
monsterDB.monsters[index].statusCounter = 0;
monsterDB.monsters[index].statusTurns = 0;
monsterDB.monsters[index].next = NULL;
monsterDB.monsters[index].prev = NULL;
addAttackToMonster(&monsterDB.monsters[index], 0, "Fire Blast", TYPE_FIRE, 110, 85, 5, STATUS_BURNING, 0, 10);
addAttackToMonster(&monsterDB.monsters[index], 1, "Flamethrower", TYPE_FIRE, 90, 100, 15, STATUS_BURNING, 0, 10);
addAttackToMonster(&monsterDB.monsters[index], 2, "Hurricane", TYPE_FLYING, 110, 70, 10, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 3, "Ancient Power", TYPE_ROCK, 60, 100, 5, 0, 0, 0);
index++;

// Mewtwo (Psíquico)
strcpy(monsterDB.monsters[index].name, "Mewtwo");
monsterDB.monsters[index].type1 = TYPE_PSYCHIC;
monsterDB.monsters[index].type2 = TYPE_NONE;
monsterDB.monsters[index].maxHp = 106;
monsterDB.monsters[index].hp = 106;
monsterDB.monsters[index].attack = 110;
monsterDB.monsters[index].defense = 90;
monsterDB.monsters[index].speed = 130;
monsterDB.monsters[index].statusCondition = STATUS_NONE;
monsterDB.monsters[index].statusCounter = 0;
monsterDB.monsters[index].statusTurns = 0;
monsterDB.monsters[index].next = NULL;
monsterDB.monsters[index].prev = NULL;
addAttackToMonster(&monsterDB.monsters[index], 0, "Psychic", TYPE_PSYCHIC, 90, 100, 10, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 1, "Shadow Ball", TYPE_GHOST, 80, 100, 15, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 2, "Aura Sphere", TYPE_FIGHTING, 80, 100, 20, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 3, "Ice Beam", TYPE_ICE, 90, 100, 10, 0, 0, 0);
index++;

// Mew (Psíquico)
strcpy(monsterDB.monsters[index].name, "Mew");
monsterDB.monsters[index].type1 = TYPE_PSYCHIC;
monsterDB.monsters[index].type2 = TYPE_NONE;
monsterDB.monsters[index].maxHp = 100;
monsterDB.monsters[index].hp = 100;
monsterDB.monsters[index].attack = 100;
monsterDB.monsters[index].defense = 100;
monsterDB.monsters[index].speed = 100;
monsterDB.monsters[index].statusCondition = STATUS_NONE;
monsterDB.monsters[index].statusCounter = 0;
monsterDB.monsters[index].statusTurns = 0;
monsterDB.monsters[index].next = NULL;
monsterDB.monsters[index].prev = NULL;
addAttackToMonster(&monsterDB.monsters[index], 0, "Psychic", TYPE_PSYCHIC, 90, 100, 10, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 1, "Ancient Power", TYPE_ROCK, 60, 100, 5, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 2, "Aura Sphere", TYPE_FIGHTING, 80, 100, 20, 0, 0, 0);
addAttackToMonster(&monsterDB.monsters[index], 3, "Transform", TYPE_NORMAL, 0, 100, 10, 0, 0, 0);
index++;

     diagnoseTypeIssues(); // Espero que funcione :(
}

 // Libera o banco de dados de monstros
 void freeMonsterDatabase(void) {
    // Note: As texturas já devem ter sido descarregadas pela função unloadMonsterTextures

    if (monsterDB.monsters != NULL) {
        free(monsterDB.monsters);
        monsterDB.monsters = NULL;
        monsterDB.count = 0;
    }
}

 // Obtém um monstro do banco de dados pelo índice
 PokeMonster* getMonsterByIndex(int index) {
     if (index < 0 || index >= monsterDB.count || monsterDB.monsters == NULL) {
         return NULL;
     }
     return &monsterDB.monsters[index];
 }

 // Obtém um monstro do banco de dados pelo nome
 PokeMonster* getMonsterByName(const char* name) {
     if (name == NULL || monsterDB.monsters == NULL) {
         return NULL;
     }

     for (int i = 0; i < monsterDB.count; i++) {
         if (strcmp(monsterDB.monsters[i].name, name) == 0) {
             return &monsterDB.monsters[i];
         }
     }

     return NULL;
 }

 // Retorna o número total de monstros no banco de dados
 int getMonsterCount(void) {
     return monsterDB.count;
 }

// Cria uma cópia de um monstro do banco de dados
PokeMonster* createMonsterCopy(PokeMonster* source) {
     if (source == NULL) {
         return NULL;
     }

     PokeMonster* copy = (PokeMonster*)malloc(sizeof(PokeMonster));
     if (copy == NULL) {
         return NULL;
     }

     // Copiar todos os dados
     memcpy(copy, source, sizeof(PokeMonster));

     // Resetar ponteiros da lista
     copy->next = NULL;
     copy->prev = NULL;

     // Inicializar os campos de status
     copy->statusCondition = STATUS_NONE;
     copy->statusCounter = 0;
     copy->statusTurns = 0;

     return copy;
 }

 // Libera um monstro alocado dinamicamente
 void freeMonster(PokeMonster* monster) {
     if (monster != NULL) {
         free(monster);
     }
 }

 // Adiciona um ataque a um monstro
 void addAttackToMonster(PokeMonster* monster, int slot, const char* name, MonsterType type,
                          int power, int accuracy, int pp, int statusEffect,
                          int statusPower, int statusChance) {
     if (monster == NULL || slot < 0 || slot >= 4) {
         return;
     }

     strcpy(monster->attacks[slot].name, name);
     monster->attacks[slot].type = type;
     monster->attacks[slot].power = power;
     monster->attacks[slot].accuracy = accuracy;
     monster->attacks[slot].ppMax = pp;
     monster->attacks[slot].ppCurrent = pp;
     monster->attacks[slot].statusEffect = statusEffect;
     monster->attacks[slot].statusPower = statusPower;
     monster->attacks[slot].statusChance = statusChance;
 }

 // Calcula o multiplicador de dano baseado nos tipos
float calculateTypeEffectiveness(MonsterType attackType, MonsterType defenderType1, MonsterType defenderType2) {
     if (attackType < 0 || attackType >= TYPE_COUNT || defenderType1 < 0 || defenderType1 >= TYPE_COUNT) {
         return 1.0f; // Sem modificador em caso de erro
         }

     float multiplier = typeEffectiveness[attackType][defenderType1];

     // Se o defensor tiver um segundo tipo, multiplicar pelo efeito nesse tipo também
     if (defenderType2 != TYPE_NONE && defenderType2 >= 0 && defenderType2 < TYPE_COUNT) {
         multiplier *= typeEffectiveness[attackType][defenderType2];
     }

     return multiplier;
 }

 // Gera um time aleatório de monstros para o bot
 MonsterList* generateRandomTeam(int teamSize) {
     if (teamSize <= 0 || teamSize > monsterDB.count) {
         teamSize = 3; // Default para 3 monstros se o número for inválido
     }

     MonsterList* team = createMonsterList();
     if (team == NULL) {
         return NULL;
     }

     // Array para controlar quais monstros já foram escolhidos
     bool* chosen = (bool*)calloc(monsterDB.count, sizeof(bool));
     if (chosen == NULL) {
         freeMonsterList(team);
         return NULL;
     }

     int count = 0;
     while (count < teamSize) {
         int index = rand() % monsterDB.count;

         // Se este monstro ainda não foi escolhido
         if (!chosen[index]) {
             PokeMonster* monster = createMonsterCopy(&monsterDB.monsters[index]);
             if (monster != NULL) {
                 addMonster(team, monster);
                 chosen[index] = true;
                 count++;
             }
         }
     }

     free(chosen);

     // Define o primeiro monstro como o atual
     if (team->count > 0) {
         team->current = team->first;
     }

     return team;
 }

 // Algoritmo Quick Sort para ordenar monstros por velocidade
 void quickSortMonstersBySpeed(PokeMonster** monsters, int left, int right) {
     if (left < right) {
         int pivotIndex = partitionMonsters(monsters, left, right);
         quickSortMonstersBySpeed(monsters, left, pivotIndex - 1);
         quickSortMonstersBySpeed(monsters, pivotIndex + 1, right);
     }
 }

 // Função de particionamento para o Quick Sort
 int partitionMonsters(PokeMonster** monsters, int left, int right) {
     // Usando o elemento mais à direita como pivô
     PokeMonster* pivot = monsters[right];
     int i = left - 1;

     for (int j = left; j < right; j++) {
         // Ordenação descendente por velocidade
         if (monsters[j]->speed > pivot->speed) {
             i++;
             // Troca monsters[i] e monsters[j]
             PokeMonster* temp = monsters[i];
             monsters[i] = monsters[j];
             monsters[j] = temp;
         }
     }

     // Troca monsters[i+1] e monsters[right] (pivô)
     PokeMonster* temp = monsters[i + 1];
     monsters[i + 1] = monsters[right];
     monsters[right] = temp;

     return i + 1;
 }

 // Retorna o nome do tipo como string
 const char* getTypeName(MonsterType type) {
     if (type == TYPE_NONE) {
         return "Nenhum";
     }

     if (type < 0 || type >= TYPE_COUNT) {
         return "Desconhecido";
     }

     return typeNames[type];
 }

 // Obtém a cor correspondente ao tipo do monstro
 Color getTypeColor(MonsterType type) {
     if (type == TYPE_NONE) {
         return GRAY;
     }

     if (type < 0 || type >= TYPE_COUNT) {
         return WHITE;
     }

     return typeColors[type];
 }

// Mapeamento entre nomes de monstros e seus números na Pokedex
// Para seus Pokitos originais, vamos usar números a partir de 1000
static const MonsterMapping monsterMapping[] = {
     // Pokitos originais (números 1000+)
     {"Grillitron", 1000},
     {"Tatarion", 1001},
     {"Aquariah", 1002},
     {"Pyromula", 1003},
     {"Netomon", 1004},
     {"Brigadeli", 1005},
     {"Ventaforte", 1006},
     {"Gambiarra", 1007},
     {"Mandragoiaba", 1008},

     // Pokémon clássicos (números oficiais da Pokedex)
     {"Venusaur", 3},
     {"Charizard", 6},
     {"Blastoise", 9},
     {"Caterpie", 10},
     {"Butterfree", 12},
     {"Pidgeot", 18},
     {"Fearow", 22},
     {"Pikachu", 25},
     {"Raichu", 26},
     {"Ninetales", 38},
     {"Golbat", 42},
     {"Parasect", 47},
     {"Arcanine", 59},
     {"Alakazam", 65},
     {"Machamp", 68},
     {"VictreeBel", 71},
     {"Tentacruel", 73},
     {"Golem", 76},
     {"Rapidash", 78},
     {"SlowBro", 80},
     {"Magneton", 82},
     {"Gengar", 94},
     {"Onix", 95},
     {"Exeggutor", 103},
     {"Hitmonlee", 106},
     {"Hitmonchan", 107},
     {"Weezing", 110},
     {"Rhydon", 112},
     {"Chansey", 113},
     {"Kangaskhan", 115},
     {"Scyther", 123},
     {"Electabuzz", 125},
     {"Magmar", 126},
     {"Gyarados", 130},
     {"Eevee", 133},
     {"Vaporeon", 134},
     {"Jolteon", 135},
     {"Flareon", 136},
     {"Porygon", 137},
     {"Snorlax", 143},
     {"Articuno", 144},
     {"Zapdos", 145},
     {"Moltres", 146},
     {"Mewtwo", 150},
     {"Mew", 151},

     // Marcador de fim da lista
     {NULL, 0}
};

// Função para obter o número da Pokedex a partir do nome do monstro
int getPokedexNumber(const char* monsterName) {
    for (int i = 0; monsterMapping[i].name != NULL; i++) {
        if (strcmp(monsterMapping[i].name, monsterName) == 0) {
            return monsterMapping[i].pokedexNum;
        }
    }
    // Caso não encontre, retorna -1
    return -1;
}

 /**
 * Carrega as texturas para os monstros usando o padrão de nomenclatura
 * [número](front/back).(gif)
 */
void loadMonsterTextures(void) {
    char frontPath[256];
    char backPath[256];
    char fallbackPath[256];
    int pokedexNum;

    printf("Iniciando carregamento de texturas dos monstros...\n");

    // Para cada monstro no banco de dados
    for (int i = 0; i < monsterDB.count; i++) {
        // Obter o número da Pokedex para este monstro
        pokedexNum = getPokedexNumber(monsterDB.monsters[i].name);

        if (pokedexNum < 0) {
            printf("Aviso: Monstro '%s' não tem número na Pokedex mapeado.\n",
                monsterDB.monsters[i].name);
            continue;
        }

        // Novos caminhos simplificados para sprites
        snprintf(frontPath, sizeof(frontPath), "resources/sprites/pokemon/%03df.gif", pokedexNum);
        snprintf(backPath, sizeof(backPath), "resources/sprites/pokemon/%03db.gif", pokedexNum);

        // Carregar imagens GIF com todos os frames
        monsterDB.monsters[i].frontFrames = 0;
        monsterDB.monsters[i].backFrames = 0;
        monsterDB.monsters[i].currentFrontFrame = 0;
        monsterDB.monsters[i].currentBackFrame = 0;

        // Tentar carregar o GIF frontal
        if (FileExists(frontPath)) {
            monsterDB.monsters[i].frontImage = LoadImageAnim(frontPath, &monsterDB.monsters[i].frontFrames);
            monsterDB.monsters[i].frontTexture = LoadTextureFromImage(monsterDB.monsters[i].frontImage);
            printf("GIF frontal carregado para %s: %d frames\n",
                   monsterDB.monsters[i].name, monsterDB.monsters[i].frontFrames);
        } else {
            // Carregar fallback estático
            snprintf(fallbackPath, sizeof(fallbackPath), "resources/fallback/%03d.png", pokedexNum % 10);
            Image frontImage = LoadImage(fallbackPath);

            if (!frontImage.data) {
                frontImage = GenImageColor(64, 64, getTypeColor(monsterDB.monsters[i].type1));
            }

            monsterDB.monsters[i].frontTexture = LoadTextureFromImage(frontImage);
            UnloadImage(frontImage);

            // Inicializar valores nulos para a imagem GIF
            monsterDB.monsters[i].frontImage.data = NULL;
            monsterDB.monsters[i].frontFrames = 0;
        }

        // Tentar carregar o GIF traseiro
        if (FileExists(backPath)) {
            monsterDB.monsters[i].backImage = LoadImageAnim(backPath, &monsterDB.monsters[i].backFrames);
            monsterDB.monsters[i].backTexture = LoadTextureFromImage(monsterDB.monsters[i].backImage);
            printf("GIF traseiro carregado para %s: %d frames\n",
                   monsterDB.monsters[i].name, monsterDB.monsters[i].backFrames);
        } else {
            // Se não existe, usar a mesma textura frontal se disponível
            if (monsterDB.monsters[i].frontTexture.id != 0) {
                monsterDB.monsters[i].backTexture = monsterDB.monsters[i].frontTexture;
            } else {
                Image backImage = GenImageColor(64, 64, getTypeColor(monsterDB.monsters[i].type1));
                monsterDB.monsters[i].backTexture = LoadTextureFromImage(backImage);
                UnloadImage(backImage);
            }

            // Inicializar valores nulos para a imagem GIF
            monsterDB.monsters[i].backImage.data = NULL;
            monsterDB.monsters[i].backFrames = 0;
        }
    }
}


/**
 * Descarrega as texturas dos monstros da memória
 */
void unloadMonsterTextures(void) {
    printf("Descarregando texturas dos monstros...\n");

    for (int i = 0; i < monsterDB.count; i++) {
        // Descarregar texturas
        if (monsterDB.monsters[i].frontTexture.id != 0) {
            UnloadTexture(monsterDB.monsters[i].frontTexture);
        }

        if (monsterDB.monsters[i].backTexture.id != 0 &&
            monsterDB.monsters[i].backTexture.id != monsterDB.monsters[i].frontTexture.id) {
            UnloadTexture(monsterDB.monsters[i].backTexture);
            }

        // Descarregar imagens GIF
        if (monsterDB.monsters[i].frontImage.data != NULL) {
            UnloadImage(monsterDB.monsters[i].frontImage);
        }

        if (monsterDB.monsters[i].backImage.data != NULL) {
            UnloadImage(monsterDB.monsters[i].backImage);
        }
    }

    printf("Texturas de monstros descarregadas com sucesso.\n");
}

void updateMonsterAnimations(void) {
    static int frameCounter = 0;
    static int frameDelay = 8; // Velocidade de animação

    frameCounter++;
    if (frameCounter >= frameDelay) {
        frameCounter = 0;

        if (battleSystem && battleSystem->playerTeam && battleSystem->opponentTeam) {
            // Atualizar monstro do jogador
            PokeMonster* playerMonster = battleSystem->playerTeam->current;
            if (playerMonster && playerMonster->backFrames > 0) {
                playerMonster->currentBackFrame = (playerMonster->currentBackFrame + 1) % playerMonster->backFrames;

                // Calcular offset do próximo frame na imagem
                unsigned int offset = playerMonster->backImage.width *
                                     playerMonster->backImage.height * 4 *
                                     playerMonster->currentBackFrame;

                // Em vez de atualizar a textura existente, vamos criar uma nova
                UnloadTexture(playerMonster->backTexture);

                // Criar uma imagem temporária com apenas o frame atual
                Image frameImage = {
                    .data = ((unsigned char *)playerMonster->backImage.data) + offset,
                    .width = playerMonster->backImage.width,
                    .height = playerMonster->backImage.height,
                    .mipmaps = 1,
                    .format = playerMonster->backImage.format
                };

                // Carregar nova textura a partir da imagem
                playerMonster->backTexture = LoadTextureFromImage(frameImage);

                // Não descarregamos frameImage.data pois é apenas um ponteiro para dados em backImage
            }

            // Atualizar monstro do oponente
            PokeMonster* enemyMonster = battleSystem->opponentTeam->current;
            if (enemyMonster && enemyMonster->frontFrames > 0) {
                enemyMonster->currentFrontFrame = (enemyMonster->currentFrontFrame + 1) % enemyMonster->frontFrames;

                // Calcular offset do próximo frame na imagem
                unsigned int offset = enemyMonster->frontImage.width *
                                     enemyMonster->frontImage.height * 4 *
                                     enemyMonster->currentFrontFrame;

                // Em vez de atualizar a textura existente, vamos criar uma nova
                UnloadTexture(enemyMonster->frontTexture);

                // Criar uma imagem temporária com apenas o frame atual
                Image frameImage = {
                    .data = ((unsigned char *)enemyMonster->frontImage.data) + offset,
                    .width = enemyMonster->frontImage.width,
                    .height = enemyMonster->frontImage.height,
                    .mipmaps = 1,
                    .format = enemyMonster->frontImage.format
                };

                // Carregar nova textura a partir da imagem
                enemyMonster->frontTexture = LoadTextureFromImage(frameImage);

                // Não descarregamos frameImage.data pois é apenas um ponteiro para dados em frontImage
            }
        }
    }
}

void verifyMonsterSprites(void) {
    char path[256];
    int totalGifs = 0;
    int totalMissing = 0;

    printf("\nVerificando GIFs disponíveis...\n");

    for (int i = 1; i <= 151; i++) {
        // Verificar GIF frontal
        snprintf(path, sizeof(path), "resources/sprites/pokemon/%03df.gif", i);
        if (FileExists(path)) {
            totalGifs++;
        } else {
            printf("GIF frontal não encontrado: %s\n", path);
            totalMissing++;
        }

        // Verificar GIF traseiro
        snprintf(path, sizeof(path), "resources/sprites/pokemon/%03db.gif", i);
        if (FileExists(path)) {
            totalGifs++;
        } else {
            printf("GIF traseiro não encontrado: %s\n", path);
            totalMissing++;
        }
    }

    printf("Total de GIFs encontrados: %d\n", totalGifs);
    printf("Total de GIFs faltando: %d\n\n", totalMissing);
}