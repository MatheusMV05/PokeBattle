/**
 * PokeBattle - Integração com API de IA
 * 
 * Este arquivo contém as implementações das funções para integração com a API de IA.
 * Baseado no exemplo ex_gemini.c fornecido.
 */
#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #define NOGDI
    #define NOUSER
#endif

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <curl/curl.h>
 #include "ia_integration.h"
 #include "structures.h"
 #include "monsters.h"
 
 // Variáveis globais
 CURL* curl_handle = NULL;
 bool initialized = false;
 static char errorBuffer[256];
 
 // Chave da API (em um sistema real, seria obtida de uma variável de ambiente ou arquivo seguro)
 static const char* API_KEY = "AIzaSyCQDFjSqzONZN29n3TW-jMjzjr5Lm2YifM"; 
 
 // Modelo a ser usado
 static const char* MODEL = "gemini-1.5-flash-latest";
 
 // Estrutura para armazenar a resposta da API
 typedef struct {
     char* buffer;
     size_t size;
 } MemoryStruct;
 
 // Função de callback para escrever os dados recebidos na memória
 static size_t WriteMemoryCallback(void* contents, size_t size, size_t nmemb, void* userp) {
     size_t realsize = size * nmemb;
     MemoryStruct* mem = (MemoryStruct*)userp;
 
     // Realoca o buffer para acomodar os novos dados + terminador nulo
     char* ptr = realloc(mem->buffer, mem->size + realsize + 1);
     if (ptr == NULL) {
         fprintf(stderr, "Erro: falha ao alocar memória no callback!\n");
         return 0; // Retornar 0 sinaliza erro para libcurl
     }
 
     mem->buffer = ptr;
     memcpy(&(mem->buffer[mem->size]), contents, realsize);
     mem->size += realsize;
     mem->buffer[mem->size] = 0; // Adiciona terminador nulo
 
     return realsize;
 }
 
 // Inicializa a conexão com a API de IA
 bool initializeAI(void) {
     // Já inicializado
     if (initialized) {
         return true;
     }
     
     // Inicialização global da libcurl
     curl_global_init(CURL_GLOBAL_ALL);
     
     // Inicializa o handle easy
     curl_handle = curl_easy_init();
     if (curl_handle == NULL) {
         fprintf(stderr, "Erro ao inicializar o handle da libcurl.\n");
         return false;
     }
     
     // Configurações gerais que serão usadas em todas as requisições
     curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "pokebattle-ai-client/1.0");
     curl_easy_setopt(curl_handle, CURLOPT_ERRORBUFFER, errorBuffer);
     curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
     curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);
     curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0L);
     
     initialized = true;
     return true;
 }
 
 // Encerra a conexão com a API de IA
 void shutdownAI(void) {
     if (!initialized) {
         return;
     }
     
     // Liberar o handle
     if (curl_handle) {
         curl_easy_cleanup(curl_handle);
         curl_handle = NULL;
     }
     
     // Limpeza global
     curl_global_cleanup();
     
     initialized = false;
 }
 
 // Consulta a API de IA com um prompt e retorna a resposta
 char* queryAI(const char* prompt) {
     if (!initialized || curl_handle == NULL || prompt == NULL) {
         return strdup("Erro: API de IA não inicializada ou prompt inválido.");
     }
     
     // Criar a URL da API com a chave
     char api_url[512];
     snprintf(api_url, sizeof(api_url),
              "https://generativelanguage.googleapis.com/v1beta/models/%s:generateContent?key=%s",
              MODEL, API_KEY);
     
              
     // Montar o payload JSON para a requisição
     char json_payload[MAX_PROMPT_SIZE + 256]; // +256 para os campos do JSON
     snprintf(json_payload, sizeof(json_payload),
              "{\"contents\":[{\"parts\":[{\"text\":\"%s\"}]}]}",
              prompt);
     
     // Estrutura para armazenar a resposta
     MemoryStruct chunk;
     chunk.buffer = malloc(1);
     chunk.size = 0;
     
     if (chunk.buffer == NULL) {
         return strdup("Erro: Falha ao alocar memória para a resposta.");
     }
     
     // Configurar a requisição
     curl_easy_setopt(curl_handle, CURLOPT_URL, api_url);
     curl_easy_setopt(curl_handle, CURLOPT_POST, 1L);
     curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, json_payload);
     
     // Configurar cabeçalhos
     struct curl_slist* headers = NULL;
     headers = curl_slist_append(headers, "Content-Type: application/json; charset=UTF-8");
     curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);
     
     // Configurar callbacks para receber a resposta
     curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
     curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void*)&chunk);
     
     // Executar a requisição
     CURLcode res = curl_easy_perform(curl_handle);
     
     // Liberar os cabeçalhos
     curl_slist_free_all(headers);
     
     // Verificar resultado
     if (res != CURLE_OK) {
         free(chunk.buffer);
         char* error = (char*)malloc(strlen(errorBuffer) + 50);
         sprintf(error, "Erro na requisição: %s", errorBuffer);
         return error;
     }
     
     // Extrair o texto da resposta JSON
     char* text_start = strstr(chunk.buffer, "\"text\": \"");
     char* response_text = NULL;
     
     if (text_start) {
         text_start += strlen("\"text\": \""); // Pular a marcação
         char* text_end = strstr(text_start, "\"");
         
         if (text_end) {
             // Extrair o texto da resposta
             size_t text_length = text_end - text_start;
             response_text = (char*)malloc(text_length + 1);
             strncpy(response_text, text_start, text_length);
             response_text[text_length] = '\0';
             
             // Limpar alguns escapes de JSON simples
             char* write_pos = response_text;
             for (char* read_pos = response_text; *read_pos; read_pos++) {
                 if (*read_pos == '\\' && *(read_pos + 1) == 'n') {
                     *write_pos++ = '\n';
                     read_pos++;
                 } else if (*read_pos == '\\' && *(read_pos + 1) == 't') {
                     *write_pos++ = '\t';
                     read_pos++;
                 } else if (*read_pos == '\\' && *(read_pos + 1) == '\"') {
                     *write_pos++ = '\"';
                     read_pos++;
                 } else if (*read_pos == '\\' && *(read_pos + 1) == '\\') {
                     *write_pos++ = '\\';
                     read_pos++;
                 } else {
                     *write_pos++ = *read_pos;
                 }
             }
             *write_pos = '\0';
         }
     }
     
     // Liberar a resposta original
     free(chunk.buffer);
     
     // Se não conseguiu extrair o texto, retornar uma mensagem genérica
     if (response_text == NULL) {
         return strdup("A IA não conseguiu gerar uma resposta válida.");
     }
     
     return response_text;
 }
 
 // Gera uma descrição para um ataque
 char* generateAttackDescription(PokeMonster* attacker, PokeMonster* defender, Attack* attack) {
    if (attacker == NULL || defender == NULL || attack == NULL) {
        return strdup("Um ataque foi realizado!");
    }
    
    // Buffer para a mensagem
    char* description = (char*)malloc(256);
    if (description == NULL) {
        return strdup("Erro de memória!");
    }
    
    // Escolher um formato de mensagem aleatoriamente
    int format = rand() % 5;
    
    switch (format) {
        case 0:
            sprintf(description, "%s usou %s!", attacker->name, attack->name);
            break;
        case 1:
            sprintf(description, "%s atacou com %s!", attacker->name, attack->name);
            break;
        case 2:
            sprintf(description, "%s lançou %s contra %s!", attacker->name, attack->name, defender->name);
            break;
        case 3:
            sprintf(description, "Um poderoso %s foi usado por %s!", attack->name, attacker->name);
            break;
        case 4:
            sprintf(description, "%s executou %s com eficiência!", attacker->name, attack->name);
            break;
        default:
            sprintf(description, "%s usou %s!", attacker->name, attack->name);
            break;
    }
    
    return description;
}
 
 
 // Interpreta a resposta da API de IA para um número
 int interpretAIResponse(const char* response) {
     if (response == NULL) {
         return 0; // Valor padrão
     }
     
     // Procurar por um número na resposta
     for (int i = 0; response[i] != '\0'; i++) {
         if (response[i] >= '0' && response[i] <= '9') {
             return response[i] - '0';
         }
     }
     
     return 0; // Valor padrão se não encontrar número
 }
 
 // Fornece dicas estratégicas para o jogador
 char* getStrategicHint(PokeMonster* playerMonster, PokeMonster* botMonster) {
     if (playerMonster == NULL || botMonster == NULL) {
         return strdup("Escolha sua estratégia com sabedoria!");
     }
     
     // Formatar o prompt para a IA
     char prompt[MAX_PROMPT_SIZE];
     snprintf(prompt, MAX_PROMPT_SIZE,
              "Como IA para um jogo de batalha tipo Pokémon, dê uma dica estratégica CURTA para um jogador usando %s (%s/%s) contra um %s (%s/%s). Seja conciso (máximo 15 palavras).",
              playerMonster->name, 
              getTypeName(playerMonster->type1), 
              getTypeName(playerMonster->type2 != TYPE_NONE ? playerMonster->type2 : playerMonster->type1),
              botMonster->name,
              getTypeName(botMonster->type1), 
              getTypeName(botMonster->type2 != TYPE_NONE ? botMonster->type2 : botMonster->type1));
     
     // Consultar a IA
     char* hint = queryAI(prompt);
     
     // Se a consulta falhar, usar uma dica padrão
     if (hint == NULL || strstr(hint, "Erro:") == hint) {
         char* default_hint = strdup("Considere os tipos envolvidos para maximizar seu dano!");
         if (hint) free(hint);
         return default_hint;
     }
     
     return hint;
 }

// Sugere a melhor ação para o bot com sistema de fallback
int getAISuggestedAction(PokeMonster* botMonster, PokeMonster* playerMonster) {
    if (botMonster == NULL || playerMonster == NULL) {
        return 0;
    }
    
    // Tentar usar a IA Gemini primeiro
    if (initialized && curl_handle != NULL) {
        char prompt[MAX_PROMPT_SIZE];
        snprintf(prompt, MAX_PROMPT_SIZE,
                 "Como IA para um jogo de batalha tipo Pokémon, qual seria a melhor ação para um %s com %d/%d de HP contra um %s com %d/%d de HP? Responda APENAS com um número: 0 para atacar, 1 para trocar, 2 para usar item.",
                 botMonster->name, botMonster->hp, botMonster->maxHp,
                 playerMonster->name, playerMonster->hp, playerMonster->maxHp);
        
        // Consultar a IA
        char* response = queryAI(prompt);
        
        // Processar a resposta
        if (response != NULL && strstr(response, "Erro:") != response) {
            printf("[IA Gemini] Decisão recebida: %s\n", response);
            int action = interpretAIResponse(response);
            free(response);
            return action;
        }
        
        // Se falhou, liberar a resposta
        printf("[IA Gemini] Falhou. Usando sistema simples.\n");
        if (response) free(response);
    }
    
    // Fallback para IA simples
    printf("[Sistema Simples] Tomando decisão localmente...\n");
    return getAISuggestedActionSimple(botMonster, playerMonster);
}

// Sugere o melhor ataque para o bot com sistema de fallback
int getAISuggestedAttack(PokeMonster* botMonster, PokeMonster* playerMonster) {
    if (botMonster == NULL || playerMonster == NULL) {
        return 0; // Ataque padrão: primeiro ataque
    }
    
    // Tentar usar a IA Gemini primeiro
    if (initialized && curl_handle != NULL) {
        // Formatar o prompt para a IA
        char prompt[MAX_PROMPT_SIZE];
        snprintf(prompt, MAX_PROMPT_SIZE,
                 "Como IA para um jogo de batalha tipo Pokémon, qual seria o melhor ataque para um %s (%s) usar contra um %s (%s)? Os ataques disponíveis são: 0:%s (%s), 1:%s (%s), 2:%s (%s), 3:%s (%s). Responda APENAS com o número do ataque (0, 1, 2 ou 3).",
                 botMonster->name, 
                 getTypeName(botMonster->type1),
                 playerMonster->name,
                 getTypeName(playerMonster->type1),
                 botMonster->attacks[0].name, getTypeName(botMonster->attacks[0].type),
                 botMonster->attacks[1].name, getTypeName(botMonster->attacks[1].type),
                 botMonster->attacks[2].name, getTypeName(botMonster->attacks[2].type),
                 botMonster->attacks[3].name, getTypeName(botMonster->attacks[3].type));
        
        // Consultar a IA
        char* response = queryAI(prompt);
        
        // Processar a resposta
        if (response != NULL && strstr(response, "Erro:") != response) {
            int attackIndex = interpretAIResponse(response);
            
            // Validar o índice do ataque
            if (attackIndex >= 0 && attackIndex <= 3) {
                free(response);
                return attackIndex;
            }
        }
        
        // Se falhou, liberar a resposta
        if (response) free(response);
    }
    
    // Fallback para lógica simples
    printf("Usando IA simples para escolha de ataque.\n");
    return botChooseAttack(botMonster, playerMonster);
}

// Sugere o melhor monstro para troca com sistema de fallback
int getAISuggestedMonster(MonsterList* botTeam, PokeMonster* playerMonster) {
    if (botTeam == NULL || playerMonster == NULL) {
        return 0;
    }
    
    // Tentar usar a IA Gemini primeiro
    if (initialized && curl_handle != NULL) {
        // Construir lista de monstros disponíveis para o prompt
        char prompt[MAX_PROMPT_SIZE];
        char monsterList[256] = "";
        
        int count = 0;
        PokeMonster* current = botTeam->first;
        
        while (current != NULL && count < 3) {
            if (!isMonsterFainted(current) && current != botTeam->current) {
                char temp[64];
                snprintf(temp, sizeof(temp), "%d:%s (HP:%d/%d) ", 
                        count, current->name, current->hp, current->maxHp);
                strcat(monsterList, temp);
            }
            current = current->next;
            count++;
        }
        
        snprintf(prompt, MAX_PROMPT_SIZE,
                 "Como IA para um jogo de batalha tipo Pokémon, qual seria o melhor monstro para trocar contra um %s com %d/%d de HP? Monstros disponíveis: %s. Responda APENAS com o número do monstro.",
                 playerMonster->name, playerMonster->hp, playerMonster->maxHp, monsterList);
        
        // Consultar a IA
        char* response = queryAI(prompt);
        
        // Processar a resposta
        if (response != NULL && strstr(response, "Erro:") != response) {
            int monsterIndex = interpretAIResponse(response);
            free(response);
            return monsterIndex;
        }
        
        // Se falhou, liberar a resposta
        if (response) free(response);
    }
    
    // Fallback para lógica simples
    printf("Usando IA simples para escolha de monstro.\n");
    PokeMonster* chosenMonster = botChooseMonster(botTeam, playerMonster);
    
    // Encontrar o índice do monstro escolhido
    int index = 0;
    PokeMonster* current = botTeam->first;
    while (current != NULL && current != chosenMonster) {
        index++;
        current = current->next;
    }
    
    return index;
}

bool testAIConnection(void) {
    if (!initialized || curl_handle == NULL) {
        printf("AVISO: Sistema de IA não está inicializado!\n");
        return false;
    }
    
    // Testar com um prompt simples
    char* response = queryAI("Teste de conexão: Responda apenas 'OK'");
    
    if (response != NULL && strstr(response, "Erro:") != response) {
        printf("✓ Sistema de IA conectado com sucesso!\n");
        free(response);
        return true;
    } else {
        printf("✗ Falha ao conectar com a IA. Usando sistema simples de fallback.\n");
        if (response) free(response);
        return false;
    }
}
