


## 🎮 PokeBattle

Um jogo de batalhas inspirado em **Pokémon**, desenvolvido como projeto para a disciplina de **Algoritmos e Estruturas de Dados** no CESAR School.

---

## 📌 Sobre o Projeto

**PokeBattle** é um jogo de **batalha por turnos** inspirado nos clássicos **Pokémon Stadium** e **Black/White**. Os jogadores montam suas equipes de monstros e participam de combates estratégicos, baseados em **tipos**, **ataques** e **estatísticas**.

---

## ✨ Características Principais

* 🔁 Sistema de batalha por turnos com mecânicas baseadas em Pokémon
* 🤖 Integração com **IA (Gemini API)** para controle do oponente
* 🧠 Estruturas de dados implementadas:

  * Lista **duplamente encadeada** para os times
  * **Fila** para ordenação das ações
  * **Pilha** para efeitos de status
  * **Quick Sort** para ordenação por velocidade
* 🎨 Interface gráfica com **Raylib**, estilo visual inspirado em jogos Pokémon

---

## ⚙️ Instruções de Execução

### ✅ Pré-requisitos

* Compilador C (GCC ou MinGW)
* **CMake** (versão 3.20+)
* Biblioteca **Raylib**
* Biblioteca **libcurl**

### 🛠️ Compilação

Clone o repositório:

```bash
git clone https://github.com/MatheusMV05/PokeBattle
cd pokebattle
```

Configure o projeto com CMake:

```bash
mkdir build
cd build
cmake .. -DRAYLIB_DIR="caminho/para/raylib" -DCURL_DIR="caminho/para/curl"
```

Compile o projeto:

```bash
cmake --build .
```

### ▶️ Execução

Execute o jogo na pasta `build`:

```bash
./pokebattle
```

---

## 📁 Estrutura do Projeto

```
pokebattle/
├── include/              # Arquivos de cabeçalho (.h)
│   └── render/           # Cabeçalhos de renderização
├── src/                  # Código-fonte (.c)
│   └── render/           # Implementações gráficas
├── resources/            # Recursos do jogo (sprites, sons, etc.)
│   ├── sprites/          # Imagens dos monstros
│   ├── sounds/           # Efeitos sonoros
├── CMakeLists.txt        # Configuração CMake
└── README.md             # Este arquivo
```

---

## 🕹️ Como Jogar

1. Na tela inicial, clique em **JOGAR**
2. Escolha o modo: **BATALHA LIVRE** ou **MODO CARREIRA**
3. Selecione **3 monstros** para o seu time
4. Durante a batalha:

   * **LUTAR**: escolha um dos 4 ataques
   * **MOCHILA**: use itens disponíveis
   * **POKÉMON**: troque de monstro
   * **FUGIR**: encerra a batalha

---

## 🎒 Itens da Mochila

Durante a batalha, a mochila pode conter os seguintes itens:

* 🧪 **Poção** (sempre disponível):
  Restaura **20 pontos de vida** do Pokémon atual.

* 🪙 **Moeda** (item aleatório):
  Possui **50% de chance** de **curar totalmente** o Pokémon e **50% de chance** de **fazê-lo desmaiar**.

* 🟥 **Cartão Vermelho** (item aleatório):
  Força a **troca imediata** do Pokémon em campo.

> ⚠️ Apenas **um item aleatório** estará disponível por batalha, selecionado de forma randômica. A **poção** está sempre fixa na mochila.

---

## 🤖 Integração com IA

O jogo utiliza a **API Gemini (Google)** para:

* 🧠 Controlar o Pokémon oponente

> 💡 Um indicador **"IA ON/OFF"** aparece no canto superior direito. Se a conexão falhar, o jogo usa um modo offline alternativo.

---

## 🧩 Estruturas de Dados Implementadas

### 🔗 Lista Duplamente Encadeada

```c
typedef struct MonsterList {
    PokeMonster* first;
    PokeMonster* last;
    PokeMonster* current;
    int count;
} MonsterList;
```

### 📤 Fila de Ações

```c
typedef struct {
    int* actions;
    int* parameters;
    PokeMonster** monsters;
    int front;
    int rear;
    int capacity;
    int count;
} ActionQueue;
```

### 🧱 Pilha de Efeitos

```c
typedef struct {
    int* types;
    int* durations;
    int* values;
    PokeMonster** targets;
    int top;
    int capacity;
} EffectStack;
```

### ⚡ Quick Sort

```c
void quickSortMonstersBySpeed(PokeMonster** monsters, int left, int right) {
    if (left < right) {
        int pivotIndex = partitionMonsters(monsters, left, right);
        quickSortMonstersBySpeed(monsters, left, pivotIndex - 1);
        quickSortMonstersBySpeed(monsters, pivotIndex + 1, right);
    }
}

int partitionMonsters(PokeMonster** monsters, int left, int right) {
    PokeMonster* pivot = monsters[right];
    int i = left - 1;
    for (int j = left; j < right; j++) {
        if (monsters[j]->speed > pivot->speed) {
            i++;
            PokeMonster* temp = monsters[i];
            monsters[i] = monsters[j];
            monsters[j] = temp;
        }
    }
    PokeMonster* temp = monsters[i + 1];
    monsters[i + 1] = monsters[right];
    monsters[right] = temp;
    return i + 1;
}
```

---

## 👥 Contribuidores

* Julia Torres
* Fatima Beatriz
* Maria Claudia
* Matheus Martins
* Vinicius Jose

---

## 🙏 Agradecimentos

* Professora Natacha da disciplina de AED
* CESAR School
* Comunidade Raylib
* Nintendo e Game Freak pela inspiração

---

## 📄 Licença

> Este projeto foi desenvolvido **exclusivamente para fins educacionais**, como parte de uma atividade acadêmica.

---
