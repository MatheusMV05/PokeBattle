# PokeBattle

PokeBattle é um jogo de batalha por turnos inspirado em Pokémon Stadium, desenvolvido em C utilizando a biblioteca gráfica Raylib e integração com a API de IA Gemini.

## Requisitos

Para compilar e executar o jogo, você precisa das seguintes dependências:

- GCC (GNU Compiler Collection)
- Make
- Biblioteca Raylib (versão 4.0 ou superior)
- Biblioteca libcurl (para integração com a API de IA)
- Uma chave de API do Google AI Studio (Gemini)

## Instalação das Dependências

### Ubuntu/Debian

```bash
# Instalação do GCC e Make
sudo apt-get update
sudo apt-get install gcc make

# Instalação da libcurl
sudo apt-get install libcurl4-openssl-dev

# Instalação da Raylib
sudo apt-get install libasound2-dev libx11-dev libxrandr-dev libxi-dev libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev libxinerama-dev

# Clonar e compilar Raylib
git clone https://github.com/raysan5/raylib.git
cd raylib/src
make PLATFORM=PLATFORM_DESKTOP
sudo make install
```

### Windows com MinGW

1. Instale o [MinGW](https://osdn.net/projects/mingw/releases/)
2. Instale [CMake](https://cmake.org/download/)
3. Baixe e compile a Raylib:
   ```bash
   git clone https://github.com/raysan5/raylib.git
   cd raylib
   mkdir build && cd build
   cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..
   mingw32-make
   ```
4. Instale o [libcurl](https://curl.se/windows/)

## Configuração da API Gemini

1. Obtenha uma chave de API gratuita do Google AI Studio em [https://makersuite.google.com/app](https://makersuite.google.com/app)
2. Edite o arquivo `ai_integration.c` e substitua o valor da constante `API_KEY` pela sua chave:
   ```c
   static const char* API_KEY = "SUA_CHAVE_AQUI"; // Substitua pela sua chave real
   ```

## Compilação

Para compilar o jogo, execute o seguinte comando no diretório raiz do projeto:

```bash
make
```

Isso irá gerar o executável `pokebattle`.

## Execução

Para executar o jogo:

```bash
./pokebattle
```

Ou use o comando:

```bash
make run
```

## Estruturas de Dados Utilizadas

O jogo implementa as seguintes estruturas de dados:

- **Lista Duplamente Encadeada**: Para armazenar os times de monstros
- **Fila**: Para a sequência de ações durante a batalha
- **Pilha**: Para os efeitos de status aplicados durante a batalha

## Algoritmo de Ordenação

O jogo utiliza o algoritmo QuickSort para ordenar os monstros por velocidade, determinando a ordem das ações durante a batalha.

## Controles

- **Mouse**: Use o mouse para clicar nos botões e interagir com a interface
- **Teclado**: 
  - Teclas direcionais para navegar nos menus (em algumas telas)
  - Espaço/Enter para confirmar ações

## Mecânica de Jogo

1. Selecione seu adversário (Bot ou outro jogador)
2. Escolha 3 monstros para formar seu time
3. Durante a batalha, escolha entre:
   - Lutar: Use um dos 4 ataques do seu monstro
   - Monstros: Troque para outro monstro do seu time
   - Mochila: Use o item disponível para a batalha
   - Desistir: Encerre a batalha atual

A integração com a API de IA influencia o comportamento do bot adversário, gerando descrições criativas para os ataques e fornecendo dicas estratégicas ao jogador.

## Solução de Problemas

Se você encontrar erro relacionado à libcurl, verifique se a biblioteca está instalada corretamente e se os caminhos estão configurados no Makefile.

Se a API de IA não estiver funcionando, verifique:
1. Se sua chave de API está correta
2. Se você tem uma conexão ativa com a internet
3. Se o modelo especificado (`gemini-1.5-flash-latest`) está disponível

## Créditos

- Desenvolvido por Julia Torres, Maria Claudia, Matheus Martins e Vinicius Jose para a disciplina de Algoritmos e Estruturas de Dados
- Utilizando a biblioteca Raylib para gráficos
- Integração com a API Gemini do Google AI
