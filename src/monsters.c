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
 #include <time.h>
 #include "raylib.h"
 #include "monsters.h"
 #include "battle.h"
 
 static void createMonsterDatabase(void);

 // Banco de dados de monstros
 MonsterDatabase monsterDB;
 
 // Matriz de efetividade de tipos (multiplicadores de dano)
 // Linhas: tipo do ataque, Colunas: tipo do defensor
 float typeEffectiveness[TYPE_COUNT][TYPE_COUNT] = {
    // METAL     FIRE    WATER   ELECTRIC  GRASS   DRAGON   GHOST   FLYING   FAIRY
     {  0.5f,    0.5f,   0.5f,   0.5f,     1.0f,   1.0f,    1.0f,   2.0f,    2.0f  }, // METAL
     {  2.0f,    0.5f,   0.5f,   1.0f,     2.0f,   0.5f,    1.0f,   1.0f,    0.5f  }, // FIRE
     {  1.0f,    2.0f,   0.5f,   1.0f,     0.5f,   0.5f,    1.0f,   1.0f,    1.0f  }, // WATER
     {  1.0f,    1.0f,   2.0f,   0.5f,     0.5f,   0.5f,    1.0f,   2.0f,    1.0f  }, // ELECTRIC
     {  2.0f,    0.5f,   2.0f,   1.0f,     0.5f,   0.5f,    1.0f,   0.5f,    1.0f  }, // GRASS
     {  1.0f,    1.0f,   1.0f,   1.0f,     1.0f,   2.0f,    1.0f,   1.0f,    0.0f  }, // DRAGON
     {  1.0f,    1.0f,   1.0f,   1.0f,     1.0f,   1.0f,    2.0f,   1.0f,    1.0f  }, // GHOST
     {  0.5f,    1.0f,   1.0f,   0.5f,     2.0f,   1.0f,    1.0f,   1.0f,    1.0f  }, // FLYING
     {  1.0f,    0.5f,   1.0f,   1.0f,     1.0f,   2.0f,    1.0f,   1.0f,    1.0f  }  // FAIRY
 };
 
 // Nomes dos tipos
 static const char* typeNames[TYPE_COUNT] = {
     "Metal", "Fogo", "Água", "Elétrico", "Grama", "Dragão", "Fantasma", "Voador", "Fada"
 };
 
 // Cores dos tipos
 static Color typeColors[TYPE_COUNT] = {
     (Color){ 170, 170, 190, 255 },  // Metal (cinza metálico)
     (Color){ 240, 80, 48, 255 },    // Fogo (Vermelho)
     (Color){ 48, 128, 240, 255 },   // Água (Azul)
     (Color){ 248, 208, 48, 255 },   // Elétrico (Amarelo)
     (Color){ 120, 200, 80, 255 },   // Grama (Verde)
     (Color){ 112, 56, 248, 255 },   // Dragão (Roxo)
     (Color){ 112, 88, 152, 255 },   // Fantasma (Roxo escuro)
     (Color){ 152, 168, 240, 255 },  // Voador (Azul claro)
     (Color){ 240, 182, 188, 255 }   // Fada (Rosa)
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
 
 // Cria o banco de dados de monstros com monstros pré-definidos
void createMonsterDatabase(void) {
    // Alocar espaço para 9 monstros (um para cada tipo)
    monsterDB.count = 9;
    monsterDB.monsters = (PokeMonster*)malloc(sizeof(PokeMonster) * monsterDB.count);
    
    if (monsterDB.monsters == NULL) {
        printf("Erro ao alocar memória para o banco de monstros!\n");
        return;
    }
    
    // Monstro 1: Tipo Metal
    PokeMonster* monster = &monsterDB.monsters[0];
    strcpy(monster->name, "Monstro_Metal");
    monster->type1 = TYPE_METAL;
    monster->type2 = TYPE_NONE; // Mantido apenas para compatibilidade com o código existente
    monster->maxHp = 90;
    monster->hp = 90;
    monster->attack = 80;
    monster->defense = 120;
    monster->speed = 70;
    monster->statusCondition = STATUS_NONE;
    monster->statusCounter = 0;
    monster->statusTurns = 0; 
    monster->next = NULL;
    monster->prev = NULL;
    
    // Adicionar ataques (pelo menos um do tipo do monstro)
    addAttackToMonster(monster, 0, "Lança Metal", TYPE_METAL, 80, 95, 15, 0, 0, 0);
    addAttackToMonster(monster, 1, "Escudo de Aço", TYPE_METAL, 0, 100, 20, 7, 30, 100);
    addAttackToMonster(monster, 2, "Impacto Pesado", TYPE_METAL, 95, 85, 10, 0, 0, 0);
    addAttackToMonster(monster, 3, "Rocha Pontiaguda", TYPE_METAL, 65, 100, 20, 2, 10, 30);
    
    // Monstro 2: Tipo Fogo
    monster = &monsterDB.monsters[1];
    strcpy(monster->name, "Monstro_Fogo");
    monster->type1 = TYPE_FIRE;
    monster->type2 = TYPE_NONE; // Mantido apenas para compatibilidade com o código existente
    monster->maxHp = 78;
    monster->hp = 78;
    monster->attack = 95;
    monster->defense = 70;
    monster->speed = 100;
    monster->statusCondition = STATUS_NONE;
    monster->statusCounter = 0;
    monster->statusTurns = 0; 
    monster->next = NULL;
    monster->prev = NULL;
    
    // Adicionar ataques (pelo menos um do tipo do monstro)
    addAttackToMonster(monster, 0, "Lança-Chamas", TYPE_FIRE, 90, 100, 15, 0, 0, 0);
    addAttackToMonster(monster, 1, "Chama Ardente", TYPE_FIRE, 65, 90, 15, STATUS_BURNING, 0, 30);
    addAttackToMonster(monster, 2, "Garras Afiadas", TYPE_FLYING, 65, 95, 20, 0, 0, 0);
    addAttackToMonster(monster, 3, "Rajada de Calor", TYPE_FIRE, 110, 80, 10, 0, 0, 0);
    
    // Monstro 3: Tipo Água
    monster = &monsterDB.monsters[2];
    strcpy(monster->name, "Monstro_Agua");
    monster->type1 = TYPE_WATER;
    monster->type2 = TYPE_NONE; // Mantido apenas para compatibilidade com o código existente
    monster->maxHp = 85;
    monster->hp = 85;
    monster->attack = 80;
    monster->defense = 90;
    monster->speed = 80;
    monster->statusCondition = STATUS_NONE;
    monster->statusCounter = 0;
    monster->statusTurns = 0; 
    monster->next = NULL;
    monster->prev = NULL;
    
    // Adicionar ataques (pelo menos um do tipo do monstro)
    addAttackToMonster(monster, 0, "Jato d'Água", TYPE_WATER, 80, 100, 15, 0, 0, 0);
    addAttackToMonster(monster, 1, "Bolhas", TYPE_WATER, 65, 100, 20, 2, 10, 30);
    addAttackToMonster(monster, 2, "Cabeçada", TYPE_METAL, 70, 95, 20, 0, 0, 0);
    addAttackToMonster(monster, 3, "Hidro Bomba", TYPE_WATER, 120, 80, 5, 0, 0, 0);
    
    // Monstro 4: Tipo Elétrico
    monster = &monsterDB.monsters[3];
    strcpy(monster->name, "Monstro_Eletrico");
    monster->type1 = TYPE_ELECTRIC;
    monster->type2 = TYPE_NONE; // Mantido apenas para compatibilidade com o código existente
    monster->maxHp = 70;
    monster->hp = 70;
    monster->attack = 90;
    monster->defense = 65;
    monster->speed = 120;
    monster->statusCondition = STATUS_NONE;
    monster->statusCounter = 0;
    monster->statusTurns = 0; 
    monster->next = NULL;
    monster->prev = NULL;
    
    // Adicionar ataques (pelo menos um do tipo do monstro)
    addAttackToMonster(monster, 0, "Choque do Trovão", TYPE_ELECTRIC, 75, 100, 15, STATUS_PARALYZED, 0, 30);
    addAttackToMonster(monster, 1, "Onda de Choque", TYPE_ELECTRIC, 60, 90, 20, 3, 20, 50);
    addAttackToMonster(monster, 2, "Investida Rápida", TYPE_FLYING, 40, 100, 30, 0, 0, 0);
    addAttackToMonster(monster, 3, "Trovão", TYPE_ELECTRIC, 110, 70, 10, 4, 20, 30);
    
    // Monstro 5: Tipo Grama
    monster = &monsterDB.monsters[4];
    strcpy(monster->name, "Monstro_Grama");
    monster->type1 = TYPE_GRASS;
    monster->type2 = TYPE_NONE; // Mantido apenas para compatibilidade com o código existente
    monster->maxHp = 80;
    monster->hp = 80;
    monster->attack = 85;
    monster->defense = 85;
    monster->speed = 80;
    monster->statusCondition = STATUS_NONE;
    monster->statusCounter = 0;
    monster->statusTurns = 0; 
    monster->next = NULL;
    monster->prev = NULL;
    
    // Adicionar ataques (pelo menos um do tipo do monstro)
    addAttackToMonster(monster, 0, "Chicote de Vinha", TYPE_GRASS, 75, 100, 15, 0, 0, 0);
    addAttackToMonster(monster, 1, "Pólen Sonífero", TYPE_GRASS, 0, 75, 15, STATUS_SLEEPING, 0, 80);
    addAttackToMonster(monster, 2, "Semente Bomba", TYPE_GRASS, 85, 90, 10, 0, 0, 0);
    addAttackToMonster(monster, 3, "Lâmina de Folha", TYPE_GRASS, 95, 85, 10, 2, 15, 30);
    
    // Monstro 6: Tipo Dragão
    monster = &monsterDB.monsters[5];
    strcpy(monster->name, "Monstro_Dragao");
    monster->type1 = TYPE_DRAGON;
    monster->type2 = TYPE_NONE; // Mantido apenas para compatibilidade com o código existente
    monster->maxHp = 90;
    monster->hp = 90;
    monster->attack = 100;
    monster->defense = 80;
    monster->speed = 95;
    monster->statusCondition = STATUS_NONE;
    monster->statusCounter = 0;
    monster->statusTurns = 0; 
    monster->next = NULL;
    monster->prev = NULL;
    
    // Adicionar ataques (pelo menos um do tipo do monstro)
    addAttackToMonster(monster, 0, "Sopro do Dragão", TYPE_DRAGON, 85, 90, 15, 0, 0, 0);
    addAttackToMonster(monster, 1, "Garra de Dragão", TYPE_DRAGON, 75, 100, 20, 0, 0, 0);
    addAttackToMonster(monster, 2, "Asa de Aço", TYPE_FLYING, 70, 90, 20, 7, 10, 10);
    addAttackToMonster(monster, 3, "Fúria do Dragão", TYPE_DRAGON, 120, 75, 5, 0, 0, 0);
    
    // Monstro 7: Tipo Fantasma
    monster = &monsterDB.monsters[6];
    strcpy(monster->name, "Monstro_Fantasma");
    monster->type1 = TYPE_GHOST;
    monster->type2 = TYPE_NONE; // Mantido apenas para compatibilidade com o código existente
    monster->maxHp = 70;
    monster->hp = 70;
    monster->attack = 85;
    monster->defense = 75;
    monster->speed = 110;
    monster->statusCondition = STATUS_NONE;
    monster->statusCounter = 0;
    monster->statusTurns = 0; 
    monster->next = NULL;
    monster->prev = NULL;
    
    // Adicionar ataques (pelo menos um do tipo do monstro)
    addAttackToMonster(monster, 0, "Sombra Espectral", TYPE_GHOST, 80, 100, 15, 0, 0, 0);
    addAttackToMonster(monster, 1, "Punho Sombrio", TYPE_GHOST, 70, 100, 20, 1, 15, 30);
    addAttackToMonster(monster, 2, "Pesadelo", TYPE_GHOST, 60, 100, 15, 5, 1, 70);
    addAttackToMonster(monster, 3, "Bola Sombria", TYPE_GHOST, 90, 85, 10, 2, 20, 30);
    
    // Monstro 8: Tipo Voador
    monster = &monsterDB.monsters[7];
    strcpy(monster->name, "Monstro_Voador");
    monster->type1 = TYPE_FLYING;
    monster->type2 = TYPE_NONE; // Mantido apenas para compatibilidade com o código existente
    monster->maxHp = 75;
    monster->hp = 75;
    monster->attack = 85;
    monster->defense = 70;
    monster->speed = 115;
    monster->statusCondition = STATUS_NONE;
    monster->statusCounter = 0;
    monster->statusTurns = 0; 
    monster->next = NULL;
    monster->prev = NULL;
    
    // Adicionar ataques (pelo menos um do tipo do monstro)
    addAttackToMonster(monster, 0, "Furacão", TYPE_FLYING, 85, 90, 15, 0, 0, 0);
    addAttackToMonster(monster, 1, "Asas de Aço", TYPE_FLYING, 70, 100, 20, 7, 15, 20);
    addAttackToMonster(monster, 2, "Picada Aérea", TYPE_FLYING, 75, 95, 20, 3, 15, 30);
    addAttackToMonster(monster, 3, "Rajada de Ar", TYPE_FLYING, 100, 80, 10, 0, 0, 0);
    
    // Monstro 9: Tipo Fada
    monster = &monsterDB.monsters[8];
    strcpy(monster->name, "Monstro_Fada");
    monster->type1 = TYPE_FAIRY;
    monster->type2 = TYPE_NONE; // Mantido apenas para compatibilidade com o código existente
    monster->maxHp = 75;
    monster->hp = 75;
    monster->attack = 80;
    monster->defense = 85;
    monster->speed = 90;
    monster->statusCondition = STATUS_NONE;
    monster->statusCounter = 0;
    monster->statusTurns = 0; 
    monster->next = NULL;
    monster->prev = NULL;
    
    // Adicionar ataques (pelo menos um do tipo do monstro)
    addAttackToMonster(monster, 0, "Encanto Lunar", TYPE_FAIRY, 85, 95, 15, 0, 0, 0);
    addAttackToMonster(monster, 1, "Raio Encantado", TYPE_FAIRY, 70, 100, 20, 1, 15, 30);
    addAttackToMonster(monster, 2, "Beijo Mágico", TYPE_FAIRY, 60, 100, 20, 5, 1, 50);
    addAttackToMonster(monster, 3, "Luz Cegante", TYPE_ELECTRIC, 75, 90, 15, 3, 20, 30);
    
    // Carregar texturas para os monstros (em um jogo real, seria uma imagem para cada)
    // Por agora, vamos deixar sem imagem e depois podemos adicionar
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
    copy->statusCondition = STATUS_NONE; // Garante que o monstro começa sem status
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

 void loadMonsterTextures(void) {
    char filename[100];
    
    // Carregar texturas para cada monstro
    for (int i = 0; i < monsterDB.count; i++) {
        // Construir o caminho do arquivo baseado no tipo do monstro
        switch (monsterDB.monsters[i].type1) {
            case TYPE_METAL:
                strcpy(filename, "resources/sprites/monstro_metal.png");
                break;
            case TYPE_FIRE:
                strcpy(filename, "resources/sprites/monstro_fogo.png");
                break;
            case TYPE_WATER:
                strcpy(filename, "resources/sprites/monstro_agua.png");
                break;
            case TYPE_ELECTRIC:
                strcpy(filename, "resources/sprites/monstro_eletrico.png");
                break;
            case TYPE_GRASS:
                strcpy(filename, "resources/sprites/monstro_grama.png");
                break;
            case TYPE_DRAGON:
                strcpy(filename, "resources/sprites/monstro_dragao.png");
                break;
            case TYPE_GHOST:
                strcpy(filename, "resources/sprites/monstro_fantasma.png");
                break;
            case TYPE_FLYING:
                strcpy(filename, "resources/sprites/monstro_voador.png");
                break;
            case TYPE_FAIRY:
                strcpy(filename, "resources/sprites/monstro_fada.png");
                break;
            default:
                strcpy(filename, "resources/sprites/monstro_padrao.png");
                break;
        }
        
        // Carregar a textura
        monsterDB.monsters[i].texture = LoadTexture(filename);
        
        // Verificar se a textura foi carregada com sucesso
        if (monsterDB.monsters[i].texture.id == 0) {
            printf("Aviso: Falha ao carregar textura para o monstro %s. Usando textura padrão.\n", 
                   monsterDB.monsters[i].name);
            
            // Tentar carregar uma textura padrão como fallback
            monsterDB.monsters[i].texture = LoadTexture("resources/sprites/monstro_padrao.png");
        }
    }
    
    printf("Texturas de monstros carregadas com sucesso.\n");
}

void unloadMonsterTextures(void) {
    // Descarregar texturas para cada monstro
    for (int i = 0; i < monsterDB.count; i++) {
        if (monsterDB.monsters[i].texture.id != 0) {
            UnloadTexture(monsterDB.monsters[i].texture);
        }
    }
    
    printf("Texturas de monstros descarregadas com sucesso.\n");
}