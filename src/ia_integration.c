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
#include "battle.h"
#include "globals.h"
#include <ctype.h>

extern BattleSystem* battleSystem;

 
 // Chave da API (em um sistema real, seria obtida de uma variável de ambiente ou arquivo seguro)
 static const char* API_KEY = "AIzaSyCQDFjSqzONZN29n3TW-jMjzjr5Lm2YifM"; 
 
 // Modelo a ser usado
 static const char* MODEL = "gemini-1.5-flash-latest";

 
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

    printf("[interpretAIResponse] Analisando: %s\n", response);

    // Verificar palavras-chave explícitas primeiro
    const char* actionKeywords[][2] = {
        {"atacar", "0"},
        {"ataque", "0"},
        {"trocar", "1"},
        {"troca", "1"},
        {"item", "2"},
        {"poção", "2"},
        {"curar", "2"},
    };

    int numActionKeywords = sizeof(actionKeywords) / sizeof(actionKeywords[0]);

    for (int i = 0; i < numActionKeywords; i++) {
        if (strstr(response, actionKeywords[i][0]) != NULL) {
            printf("[interpretAIResponse] Palavra-chave '%s' encontrada -> %s\n",
                   actionKeywords[i][0], actionKeywords[i][1]);
            return atoi(actionKeywords[i][1]);
        }
    }

    // BUSCA 1: Procurar por dígitos isolados
    for (int i = 0; response[i] != '\0'; i++) {
        if (response[i] >= '0' && response[i] <= '9') {
            // Verificar se é um dígito isolado
            if ((i == 0 || !isalnum(response[i-1])) &&
                (response[i+1] == '\0' || !isalnum(response[i+1]))) {
                printf("[interpretAIResponse] Dígito isolado encontrado: %c\n", response[i]);
                return response[i] - '0';
            }
        }
    }

    // BUSCA 2: Procurar padrões mais específicos "Ação: X" ou "Opção X"
    const char* patterns[] = {
        "ação: 0", "ação: 1", "ação: 2", "ação: 3",
        "ação:0", "ação:1", "ação:2", "ação:3",
        "opção 0", "opção 1", "opção 2", "opção 3",
        "opção: 0", "opção: 1", "opção: 2", "opção: 3",
        "ataque 0", "ataque 1", "ataque 2", "ataque 3"
    };

    for (int i = 0; i < sizeof(patterns)/sizeof(patterns[0]); i++) {
        if (strstr(response, patterns[i]) != NULL) {
            printf("[interpretAIResponse] Padrão '%s' encontrado\n", patterns[i]);
            return patterns[i][strlen(patterns[i])-1] - '0';
        }
    }

    // BUSCA 3: Verificar por número entre aspas ou colchetes
    char quote_patterns[][3] = {
        {'\"', '0', '\"'}, {'\"', '1', '\"'}, {'\"', '2', '\"'}, {'\"', '3', '\"'},
        {'\'', '0', '\''}, {'\'', '1', '\''}, {'\'', '2', '\''}, {'\'', '3', '\''},
        {'[', '0', ']'}, {'[', '1', ']'}, {'[', '2', ']'}, {'[', '3', ']'}
    };

    for (int i = 0; i < 12; i++) {
        char pattern[4] = {quote_patterns[i][0], quote_patterns[i][1], quote_patterns[i][2], '\0'};
        if (strstr(response, pattern) != NULL) {
            printf("[interpretAIResponse] Padrão '%s' encontrado\n", pattern);
            return quote_patterns[i][1] - '0';
        }
    }

    // BUSCA 4: Última chance - encontrar qualquer número de 0 a 3
    for (int i = 0; response[i] != '\0'; i++) {
        if (response[i] >= '0' && response[i] <= '3') {
            printf("[interpretAIResponse] Número encontrado: %c\n", response[i]);
            return response[i] - '0';
        }
    }

    // Se chegou até aqui, nenhum número válido foi encontrado
    printf("[interpretAIResponse] Nenhum número válido encontrado. Retornando padrão: 0\n");
    return 0;
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

const char* getEffectDescription(int statusEffect, int statusChance) {
     if (statusEffect == STATUS_NONE || statusChance == 0)
         return "Nenhum";

     char* description = malloc(64);
     if (description == NULL) return "Erro";

     switch (statusEffect) {
         case STATUS_PARALYZED:
             sprintf(description, "Paralisia (%d%% chance)", statusChance);
             break;
         case STATUS_SLEEPING:
             sprintf(description, "Sono (%d%% chance)", statusChance);
             break;
         case STATUS_BURNING:
             sprintf(description, "Queimadura (%d%% chance)", statusChance);
             break;
         case STATUS_ATK_DOWN:
             sprintf(description, "↓Ataque (%d%% chance)", statusChance);
             break;
         case STATUS_DEF_DOWN:
             sprintf(description, "↓Defesa (%d%% chance)", statusChance);
             break;
         case STATUS_SPD_DOWN:
             sprintf(description, "↓Velocidade (%d%% chance)", statusChance);
             break;
         default:
             sprintf(description, "Efeito %d (%d%% chance)", statusEffect, statusChance);
     }

     return description;
 }


// Sugere a melhor ação para o bot com sistema de fallback
int getAISuggestedAction(PokeMonster* botMonster, PokeMonster* playerMonster) {
    if (botMonster == NULL || playerMonster == NULL) {
        return 0;
    }

    // Tentar usar a IA Gemini primeiro
    if (initialized && curl_handle != NULL) {
        // IMPORTANTE: Manter registro das últimas decisões para evitar repetição
        static int lastAction = -1;
        static int repeatCount = 0;

        char prompt[MAX_PROMPT_SIZE];
        snprintf(prompt, MAX_PROMPT_SIZE,
                 "Você é um especialista em jogos Pokémon, analisando a melhor ação em uma batalha.\n\n"
                 "SITUAÇÃO ATUAL:\n"
                 "- MEU POKÉMON: %s (tipo primário: %s, tipo secundário: %s)\n"
                 "- MEU HP: %d/%d (%.1f%%)\n"
                 "- MEU STATUS: %s\n\n"
                 "- POKÉMON INIMIGO: %s (tipo primário: %s, tipo secundário: %s)\n"
                 "- HP INIMIGO: %d/%d (%.1f%%)\n"
                 "- STATUS INIMIGO: %s\n\n"
                 "MEUS ATAQUES:\n"
                 "0: %s (tipo: %s, poder: %d, precisão: %d, PP: %d/%d, efeito: %s)\n"
                 "1: %s (tipo: %s, poder: %d, precisão: %d, PP: %d/%d, efeito: %s)\n"
                 "2: %s (tipo: %s, poder: %d, precisão: %d, PP: %d/%d, efeito: %s)\n"
                 "3: %s (tipo: %s, poder: %d, precisão: %d, PP: %d/%d, efeito: %s)\n\n"
                 "AÇÕES POSSÍVEIS:\n"
                 "0 - ATACAR: Use um dos meus ataques listados acima\n"
                 "1 - TROCAR: Troque para outro Pokémon\n"
                 "2 - USAR ITEM: Use um item para curar ou causar efeito\n\n"
                 "Sua análise deve considerar:\n"
                 "- Vantagens de tipo (2x dano) e desvantagens (0.5x dano)\n"
                 "- Situação de HP - se estou com HP baixo, talvez deva curar ou trocar\n"
                 "- Status atuais - status negativos podem influenciar a decisão\n"
                 "- Poder dos ataques e PP restante\n\n"
                 "Responda APENAS com o número da ação (0, 1 ou 2) que representa a melhor estratégia.",
                 botMonster->name,
                 getTypeName(botMonster->type1),
                 getTypeName(botMonster->type2 != TYPE_NONE ? botMonster->type2 : botMonster->type1),
                 botMonster->hp, botMonster->maxHp, (float)botMonster->hp/botMonster->maxHp*100.0f,
                 getStatusName(botMonster->statusCondition),

                 playerMonster->name,
                 getTypeName(playerMonster->type1),
                 getTypeName(playerMonster->type2 != TYPE_NONE ? playerMonster->type2 : playerMonster->type1),
                 playerMonster->hp, playerMonster->maxHp, (float)playerMonster->hp/playerMonster->maxHp*100.0f,
                 getStatusName(playerMonster->statusCondition),

                 botMonster->attacks[0].name, getTypeName(botMonster->attacks[0].type),
                 botMonster->attacks[0].power, botMonster->attacks[0].accuracy,
                 botMonster->attacks[0].ppCurrent, botMonster->attacks[0].ppMax,
                 getEffectDescription(botMonster->attacks[0].statusEffect, botMonster->attacks[0].statusChance),

                 botMonster->attacks[1].name, getTypeName(botMonster->attacks[1].type),
                 botMonster->attacks[1].power, botMonster->attacks[1].accuracy,
                 botMonster->attacks[1].ppCurrent, botMonster->attacks[1].ppMax,
                 getEffectDescription(botMonster->attacks[1].statusEffect, botMonster->attacks[1].statusChance),

                 botMonster->attacks[2].name, getTypeName(botMonster->attacks[2].type),
                 botMonster->attacks[2].power, botMonster->attacks[2].accuracy,
                 botMonster->attacks[2].ppCurrent, botMonster->attacks[2].ppMax,
                 getEffectDescription(botMonster->attacks[2].statusEffect, botMonster->attacks[2].statusChance),

                 botMonster->attacks[3].name, getTypeName(botMonster->attacks[3].type),
                 botMonster->attacks[3].power, botMonster->attacks[3].accuracy,
                 botMonster->attacks[3].ppCurrent, botMonster->attacks[3].ppMax,
                 getEffectDescription(botMonster->attacks[3].statusEffect, botMonster->attacks[3].statusChance));

        // Consultar a IA
        char* response = queryAI(prompt);

        // Processar a resposta
        if (response != NULL && strstr(response, "Erro:") != response) {
            printf("[IA Gemini] Decisão completa recebida: %s\n", response);
            int action = interpretAIResponse(response);

            // Verificar se está repetindo a mesma ação constantemente
            if (action == lastAction) {
                repeatCount++;

                // Se repetir a mesma ação muitas vezes seguidas, forçar variação
                if (repeatCount >= 3) {
                    printf("[IA Gemini] Detectada repetição excessiva! Forçando variação...\n");

                    // 50% de chance de usar o fallback para variar
                    if (rand() % 2 == 0) {
                        free(response);
                        repeatCount = 0; // Resetar contador

                        // Usar fallback mas excluir a ação repetitiva
                        int newAction = getAISuggestedActionSimple(botMonster, playerMonster);
                        while (newAction == action) {
                            newAction = rand() % 3; // Forçar uma ação diferente aleatória
                        }

                        lastAction = newAction;
                        return newAction;
                    }
                }
            } else {
                // Resetar contador se a ação for diferente
                repeatCount = 0;
            }

            lastAction = action;
            free(response);
            return action;
        }

        // Se falhou, liberar a resposta
        printf("[IA Gemini] Falhou. Usando sistema simples.\n");
        if (response) free(response);
    }

    // Fallback para IA simples
    printf("[Sistema Simples] Tomando decisão localmente...\n");
    int action = getAISuggestedActionSimple(botMonster, playerMonster);
    return action;
}




// Função auxiliar para obter nome do status
const char* getStatusName(int statusCondition) {
    switch (statusCondition) {
        case STATUS_NONE: return "Nenhum";
        case STATUS_ATK_DOWN: return "Ataque reduzido";
        case STATUS_DEF_DOWN: return "Defesa reduzida";
        case STATUS_SPD_DOWN: return "Velocidade reduzida";
        case STATUS_PARALYZED: return "Paralisado";
        case STATUS_SLEEPING: return "Dormindo";
        case STATUS_BURNING: return "Queimando";
        default: return "Desconhecido";
    }
}

// Sugere o melhor ataque para o bot com sistema de fallback
int getAISuggestedAttack(PokeMonster* botMonster, PokeMonster* playerMonster) {
    if (botMonster == NULL || playerMonster == NULL) {
        return 0; // Ataque padrão: primeiro ataque
    }
    
    // Tentar usar a IA Gemini primeiro
    if (initialized && curl_handle != NULL) {
        // Manter registro das últimas decisões para evitar repetição
        static int lastAttack = -1;
        static int repeatCount = 0;

        // Formatar o prompt para a IA de forma mais detalhada e estratégica
        char prompt[MAX_PROMPT_SIZE];
        snprintf(prompt, MAX_PROMPT_SIZE,
                 "Você é um mestre treinador Pokémon escolhendo o melhor ataque para essa situação.\n\n"
                 "SITUAÇÃO ATUAL:\n"
                 "- MEU POKÉMON: %s (tipo primário: %s, tipo secundário: %s)\n"
                 "- MEU HP: %d/%d (%.1f%%)\n"
                 "- MEU STATUS: %s\n\n"
                 "- POKÉMON INIMIGO: %s (tipo primário: %s, tipo secundário: %s)\n"
                 "- HP INIMIGO: %d/%d (%.1f%%)\n"
                 "- STATUS INIMIGO: %s\n\n"
                 "MEUS ATAQUES DISPONÍVEIS:\n"
                 "0: %s (tipo: %s, poder: %d, precisão: %d, PP: %d/%d, efeito: %s)\n"
                 "1: %s (tipo: %s, poder: %d, precisão: %d, PP: %d/%d, efeito: %s)\n"
                 "2: %s (tipo: %s, poder: %d, precisão: %d, PP: %d/%d, efeito: %s)\n"
                 "3: %s (tipo: %s, poder: %d, precisão: %d, PP: %d/%d, efeito: %s)\n\n"
                 "LEMBRE-SE DAS REGRAS DE TIPO:\n"
                 "- Ataques super efetivos (2x dano): ataques do tipo X contra Pokémon tipo Y...\n"
                 "- Ataques não muito efetivos (0.5x dano): ataques do tipo X contra Pokémon tipo Y...\n"
                 "- Ataques sem efeito (0x dano): ataques do tipo X contra Pokémon tipo Y...\n\n"
                 "Analise cuidadosamente e responda APENAS com o número do ataque (0, 1, 2 ou 3) que você escolheria nesta situação.",
                 botMonster->name,
                 getTypeName(botMonster->type1),
                 getTypeName(botMonster->type2 != TYPE_NONE ? botMonster->type2 : botMonster->type1),
                 botMonster->hp, botMonster->maxHp, (float)botMonster->hp/botMonster->maxHp*100.0f,
                 getStatusName(botMonster->statusCondition),

                 playerMonster->name,
                 getTypeName(playerMonster->type1),
                 getTypeName(playerMonster->type2 != TYPE_NONE ? playerMonster->type2 : playerMonster->type1),
                 playerMonster->hp, playerMonster->maxHp, (float)playerMonster->hp/playerMonster->maxHp*100.0f,
                 getStatusName(playerMonster->statusCondition),

                 botMonster->attacks[0].name, getTypeName(botMonster->attacks[0].type),
                 botMonster->attacks[0].power, botMonster->attacks[0].accuracy,
                 botMonster->attacks[0].ppCurrent, botMonster->attacks[0].ppMax,
                 getEffectDescription(botMonster->attacks[0].statusEffect, botMonster->attacks[0].statusChance),

                 botMonster->attacks[1].name, getTypeName(botMonster->attacks[1].type),
                 botMonster->attacks[1].power, botMonster->attacks[1].accuracy,
                 botMonster->attacks[1].ppCurrent, botMonster->attacks[1].ppMax,
                 getEffectDescription(botMonster->attacks[1].statusEffect, botMonster->attacks[1].statusChance),

                 botMonster->attacks[2].name, getTypeName(botMonster->attacks[2].type),
                 botMonster->attacks[2].power, botMonster->attacks[2].accuracy,
                 botMonster->attacks[2].ppCurrent, botMonster->attacks[2].ppMax,
                 getEffectDescription(botMonster->attacks[2].statusEffect, botMonster->attacks[2].statusChance),

                 botMonster->attacks[3].name, getTypeName(botMonster->attacks[3].type),
                 botMonster->attacks[3].power, botMonster->attacks[3].accuracy,
                 botMonster->attacks[3].ppCurrent, botMonster->attacks[3].ppMax,
                 getEffectDescription(botMonster->attacks[3].statusEffect, botMonster->attacks[3].statusChance));

        // Consultar a IA
        char* response = queryAI(prompt);

        // Processar a resposta
        if (response != NULL && strstr(response, "Erro:") != response) {
            printf("[IA Gemini] Ataque sugerido (completo): %s\n", response);
            int attackIndex = interpretAIResponse(response);

            // Verificar se o ataque tem PP e é válido
            if (attackIndex >= 0 && attackIndex <= 3 && botMonster->attacks[attackIndex].ppCurrent > 0) {
                // Verificar se está repetindo o mesmo ataque constantemente
                if (attackIndex == lastAttack) {
                    repeatCount++;

                    // Se repetir o mesmo ataque muitas vezes seguidas, forçar variação
                    if (repeatCount >= 3) {
                        printf("[IA Gemini] Repetição excessiva de ataques! Forçando variação...\n");
                        free(response);

                        // Encontrar alternativas válidas
                        int validAttacks[4] = {0};
                        int validCount = 0;

                        for (int i = 0; i < 4; i++) {
                            if (i != attackIndex && botMonster->attacks[i].ppCurrent > 0) {
                                validAttacks[validCount++] = i;
                            }
                        }

                        // Se houver alternativas, escolher uma aleatoriamente
                        if (validCount > 0) {
                            int newAttack = validAttacks[rand() % validCount];
                            lastAttack = newAttack;
                            repeatCount = 0;
                            return newAttack;
                        }
                    }
                } else {
                    // Resetar contador se o ataque for diferente
                    repeatCount = 0;
                }

                lastAttack = attackIndex;
                free(response);
                return attackIndex;
            } else {
                printf("[IA Gemini] Ataque inválido ou sem PP. Escolhendo outro...\n");
            }

            free(response);
        } else {
            printf("[IA Gemini] Falha na resposta. Usando sistema simples.\n");
            if (response) free(response);
        }
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
        char monsterList[512] = "";

        int count = 0;
        PokeMonster* current = botTeam->first;

        // Preparar detalhes de cada monstro
        while (current != NULL) {
            if (!isMonsterFainted(current) && current != botTeam->current) {
                char temp[256];
                snprintf(temp, sizeof(temp), "%d:%s (tipos:%s/%s, HP:%d/%d, ATK:%d, DEF:%d, SPD:%d), ",
                        count,
                        current->name,
                        getTypeName(current->type1),
                        getTypeName(current->type2 != TYPE_NONE ? current->type2 : current->type1),
                        current->hp, current->maxHp,
                        current->attack, current->defense, current->speed);
                strcat(monsterList, temp);
                count++;
            }
            current = current->next;
        }

        // Se tiver monstros disponíveis
        if (count > 0) {
            snprintf(prompt, MAX_PROMPT_SIZE,
                    "Como IA para um jogo de batalha Pokémon, qual o melhor monstro para trocar contra %s (tipos:%s/%s, HP:%d/%d)? Considere vantagens de tipo e status. Monstros disponíveis: %s. Responda APENAS com o número do monstro.",
                    playerMonster->name,
                    getTypeName(playerMonster->type1),
                    getTypeName(playerMonster->type2 != TYPE_NONE ? playerMonster->type2 : playerMonster->type1),
                    playerMonster->hp, playerMonster->maxHp,
                    monsterList);

            // Consultar a IA
            char* response = queryAI(prompt);

            // Processar a resposta
            if (response != NULL && strstr(response, "Erro:") != response) {
                int monsterIndex = interpretAIResponse(response);
                if (monsterIndex >= 0 && monsterIndex < count) {
                    free(response);
                    return monsterIndex;
                }
            }

            // Se falhou, liberar a resposta
            if (response) free(response);
        } else {
            printf("[IA Gemini] Nenhum monstro disponível para troca.\n");
            return 0;
        }
    }

    // Fallback para lógica simples: escolher monstro com mais vida percentual
    printf("Usando lógica simples para escolha de monstro.\n");

    PokeMonster* bestMonster = NULL;
    float bestHpRatio = 0.0f;
    int bestIndex = 0;
    int index = 0;

    PokeMonster* current = botTeam->first;
    while (current != NULL) {
        if (!isMonsterFainted(current) && current != botTeam->current) {
            float hpRatio = (float)current->hp / current->maxHp;

            // Verificar vantagem de tipo
            float typeAdvantage = calculateTypeEffectiveness(
                current->type1, playerMonster->type1, playerMonster->type2);

            // Priorizar monstros com vantagem de tipo
            if (typeAdvantage > 1.0f) {
                hpRatio *= 1.5f;  // Aumentar a prioridade para monstros com vantagem
            }

            if (hpRatio > bestHpRatio) {
                bestHpRatio = hpRatio;
                bestMonster = current;
                bestIndex = index;
            }
        }
        current = current->next;
        index++;
    }

    if (bestMonster != NULL) {
        return bestIndex;
    }

    return 0; // Se não encontrar melhor opção, retorna o primeiro índice
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
