# Compilador
CC = gcc

# Flags de compilação
CFLAGS = -Wall -Wextra -g

# Flags para a biblioteca Raylib
RAYLIB_FLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

# Flags para a biblioteca cURL
CURL_FLAGS = -lcurl

# Nome do executável
TARGET = pokebattle

# Diretórios
SRC_DIR = src
INCLUDE_DIR = include
OBJ_DIR = obj
RENDER_DIR = src/render

# Arquivos fonte (todos os .c em src/)
SOURCES = $(wildcard $(SRC_DIR)/*.c) $(wildcard $(RENDER_DIR)/*.c)

# Arquivos objeto (correspondendo aos fontes)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Diretórios de inclusão
INCLUDES = -I$(INCLUDE_DIR) -I$(INCLUDE_DIR)/render -I/usr/local/include

# Diretórios de bibliotecas
LIBS = -L/usr/local/lib

# Criar diretório de objetos se não existir
$(shell mkdir -p $(OBJ_DIR))
$(shell mkdir -p $(OBJ_DIR)/render)

# Regra principal
all: $(TARGET)

# Regra para o executável
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LIBS) $(RAYLIB_FLAGS) $(CURL_FLAGS)

# Regra para os arquivos objeto
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(OBJ_DIR)/render/%.o: $(RENDER_DIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Limpar arquivos gerados
clean:
	rm -rf $(OBJ_DIR) $(TARGET)

# Regra para executar o jogo
run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run