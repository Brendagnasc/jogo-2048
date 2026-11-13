#pragma once
/*
 * GameLogic.h — Modelo do jogo (MVC: o "M")
 * ──────────────────────────────────────────
 * CONCEITO: Por que GameLogic inteiro está no .h e não num .cpp?
 *
 *   GameLogic é uma CLASSE TEMPLATE (template<int N>).
 *   O compilador gera código concreto ao instanciar: GameLogic<4>.
 *   Para gerar esse código, ele precisa ver a implementação COMPLETA
 *   no momento da instanciação — não apenas a declaração.
 *
 *   Se movêssemos o corpo para GameLogic.cpp, o linker falharia com
 *   "undefined reference to GameLogic<4>::applyMove" porque o compilador
 *   não teria o código disponível quando compilasse main.cpp.
 *
 *   Solução: manter tudo no header (comum para templates).
 *   Alternativa avançada: "explicit instantiation" — declarar no .h e
 *   adicionar 'template class GameLogic<4>;' no .cpp. Isso funciona mas
 *   limita o template às instâncias declaradas explicitamente.
 *
 * CONCEITO: Encapsulamento
 *   Membros 'private:' só são acessíveis dentro da própria classe.
 *   'public:' define a API que o mundo externo pode usar.
 *   Isso protege o estado interno de modificações acidentais.
 *
 * CONCEITO: RAII (Resource Acquisition Is Initialization)
 *   Recursos são adquiridos no construtor e liberados no destrutor.
 *   std::stack<Snapshot<N>> gerencia sua própria memória — sem 'new'/'delete' manual.
 */

#include "Types.h"
#include "Observer.h"

#include <stack>     // std::stack — pilha LIFO para o sistema de undo
#include <vector>
#include <memory>    // shared_ptr, make_shared
#include <random>    // mt19937, random_device
#include <tuple>     // std::tuple, structured bindings
#include <algorithm>

template <int N>
class GameLogic {
public:
    // ── Construtor ────────────────────────────────────────────────────────
    // 'explicit': proíbe conversão implícita — GameLogic g = 4; seria erro.
    // Lista de inicialização: inicializa membros ANTES do corpo do construtor.
    // Preferível a atribuição no corpo: evita inicialização dupla.
    explicit GameLogic()
        : score_(0), best_(0), moves_(0), won_(false), over_(false)
    {
        resetGrid();
        spawnTile();
        spawnTile();
    }

    // ── Accessors (getters) const ─────────────────────────────────────────
    // 'const' após o método: compilador garante que NÃO modifica o objeto.
    // Retornar 'const Grid<N>&' = referência constante: sem cópia + sem modificação.
    const Grid<N>& grid()  const { return grid_;  }
    int  score()           const { return score_; }
    int  best()            const { return best_;  }
    int  moves()           const { return moves_; }
    bool hasWon()          const { return won_;   }
    bool isOver()          const { return over_;  }
    bool canUndo()         const { return !history_.empty(); }

    // ── Registrar Observer ────────────────────────────────────────────────
    // shared_ptr<IObserver>: ponteiro inteligente com contagem de referências.
    // O observer é destruído automaticamente quando ninguém mais o referencia.
    // 'std::move(obs)': transfere a propriedade do shared_ptr sem cópia.
    void addObserver(std::shared_ptr<IObserver> obs) {
        observers_.push_back(std::move(obs));
    }

    // ── Aplicar Movimento ─────────────────────────────────────────────────
    bool applyMove(Direction dir);

    // ── Desfazer ──────────────────────────────────────────────────────────
    bool undo();

    // ── Reiniciar ─────────────────────────────────────────────────────────
    void reset();

private:
    // ── Dados privados ────────────────────────────────────────────────────
    // Convenção de nomenclatura: sufixo '_' para membros privados.
    // Deixa claro no código quando estamos acessando estado interno.
    Grid<N>                                grid_;
    int                                    score_, best_, moves_;
    bool                                   won_, over_;
    std::stack<Snapshot<N>>                history_;    // Pilha de undo (LIFO)
    std::vector<std::shared_ptr<IObserver>> observers_; // Lista de observadores

    // ── Gerador de Números Aleatórios (Singleton estático) ────────────────
    // 'static' em função membro: existe UMA instância compartilhada entre
    // todas as chamadas e objetos. Inicializado apenas na primeira chamada.
    // mt19937: Mersenne Twister 32-bit — alta qualidade estatística.
    // random_device{}: semente baseada em hardware (verdadeiramente aleatória).
    static std::mt19937& rng() {
        static std::mt19937 gen(std::random_device{}());
        return gen;
    }

    // ── Helpers privados (implementados abaixo, no mesmo header) ─────────
    void emit(const GameEvent& e);
    void resetGrid();
    void spawnTile();
    bool checkWin()     const;
    bool checkGameOver() const;

    // Núcleo: retorna {nova grade, pontos ganhos}
    // std::pair<A,B>: container de dois valores de tipos possivelmente diferentes
    std::pair<Grid<N>, int> computeMove(const Grid<N>& src, Direction dir) const;

    // Desliza uma linha para a esquerda e funde pares iguais
    std::pair<std::array<int,N>, int> slideLeft(const std::array<int,N>& row) const;

    // Rotação 90° sentido horário: base para todos os movimentos
    Grid<N> rotateCW(const Grid<N>& g) const;

    // Quantas rotações CW equivalem à direção pedida
    static int rotFor(Direction d);
};

// ═══════════════════════════════════════════════════════════════════════════════
//  Implementações (no header por ser template — veja explicação acima)
// ═══════════════════════════════════════════════════════════════════════════════

template <int N>
void GameLogic<N>::emit(const GameEvent& e) {
    for (auto& obs : observers_) obs->onEvent(e);
}

template <int N>
void GameLogic<N>::resetGrid() {
    // range-for sobre std::array: auto& row é referência para cada linha
    for (auto& row : grid_) row.fill(0);
}

template <int N>
void GameLogic<N>::spawnTile() {
    // Coletar células vazias com emplace_back (constrói pair in-place, sem cópia)
    std::vector<std::pair<int,int>> empty;
    for (int r = 0; r < N; r++)
        for (int c = 0; c < N; c++)
            if (grid_[r][c] == 0) empty.emplace_back(r, c);
    if (empty.empty()) return;

    std::uniform_int_distribution<int> rpos(0, static_cast<int>(empty.size())-1);
    std::uniform_int_distribution<int> rval(0, 9);

    // C++17 Structured binding: decompõe pair<int,int> em (r, c) diretamente
    auto [r, c] = empty[rpos(rng())];
    int val = (rval(rng()) < 9) ? 2 : 4;   // Operador ternário: 90% chance de 2
    grid_[r][c] = val;
    emit({ GameEvent::Type::NewTile, val, score_ });
}

template <int N>
bool GameLogic<N>::applyMove(Direction dir) {
    if (over_ || dir == Direction::None) return false;

    // Salvar snapshot ANTES de alterar — Padrão Command (para undo)
    history_.push({ grid_, score_, moves_ });

    auto [newGrid, gained] = computeMove(grid_, dir);  // Structured binding

    // Verificar se o tabuleiro realmente mudou
    if (newGrid == grid_) {
        history_.pop();   // Nenhuma mudança: descarta snapshot para não poluir o histórico
        return false;
    }

    grid_   = newGrid;
    score_ += gained;
    moves_++;
    if (score_ > best_) best_ = score_;

    emit({ GameEvent::Type::Move, 0, score_ });
    if (gained > 0) emit({ GameEvent::Type::Merge, gained, score_ });

    spawnTile();

    if (!won_ && checkWin()) {
        won_ = true;
        emit({ GameEvent::Type::Win, WIN_TILE, score_ });
    }
    if (checkGameOver()) {
        over_ = true;
        emit({ GameEvent::Type::GameOver, 0, score_ });
    }
    return true;
}

template <int N>
bool GameLogic<N>::undo() {
    if (history_.empty()) return false;
    // top(): referência ao elemento do topo — sem remover
    // pop(): remove o topo (não retorna o valor — design intencional da STL)
    auto snap = history_.top();
    history_.pop();
    grid_  = snap.grid;
    score_ = snap.score;
    moves_ = snap.moves;
    over_  = false;
    emit({ GameEvent::Type::Undo, 0, score_ });
    return true;
}

template <int N>
void GameLogic<N>::reset() {
    while (!history_.empty()) history_.pop();
    score_ = moves_ = 0;
    won_ = over_ = false;
    resetGrid();
    spawnTile();
    spawnTile();
}

template <int N>
std::pair<Grid<N>, int> GameLogic<N>::computeMove(const Grid<N>& src, Direction dir) const {
    int rot = rotFor(dir);
    Grid<N> g = src;

    // Estratégia de rotação: em vez de 4 algoritmos, usamos 1 (slideLeft)
    // e rotacionamos o tabuleiro para sempre deslizar na mesma direção.
    for (int i = 0; i < rot; i++) g = rotateCW(g);

    int gained = 0;
    for (int r = 0; r < N; r++) {
        auto [row, pts] = slideLeft(g[r]);
        g[r]    = row;
        gained += pts;
    }

    // Desfazer a rotação: (4 - rot) % 4 rotações no mesmo sentido = volta ao original
    int unrot = (4 - rot) % 4;
    for (int i = 0; i < unrot; i++) g = rotateCW(g);

    return { g, gained };
}

template <int N>
std::pair<std::array<int,N>, int> GameLogic<N>::slideLeft(const std::array<int,N>& row) const {
    // Passo 1: compactar — remover zeros mantendo a ordem
    std::array<int,N> t{};  // {} inicializa todos os elementos com 0
    int cnt = 0;
    for (int v : row) if (v) t[cnt++] = v;

    // Passo 2: fundir pares adjacentes iguais (da esquerda para direita)
    int pts = 0;
    for (int i = 0; i < cnt - 1; i++) {
        if (t[i] == t[i+1]) {
            t[i] *= 2;     // Dobra o primeiro
            pts  += t[i];
            // Desloca o restante à esquerda para preencher o buraco
            for (int j = i+1; j < cnt-1; j++) t[j] = t[j+1];
            t[--cnt] = 0;  // Zera o último e reduz o contador
            // NÃO incrementa i: bloco fundido não pode fundir de novo no mesmo movimento
        }
    }

    // Passo 3: resultado já está compactado (zeros já estão no final por construção)
    std::array<int,N> result{};
    for (int i = 0; i < cnt; i++) result[i] = t[i];
    return { result, pts };
}

template <int N>
Grid<N> GameLogic<N>::rotateCW(const Grid<N>& g) const {
    // Rotação 90° horária: elemento [r][c] vai para [c][N-1-r]
    // Verificação mental: canto superior-esquerdo [0][0] vai para [0][N-1] (canto sup-dir) ✓
    Grid<N> out{};
    for (int r = 0; r < N; r++)
        for (int c = 0; c < N; c++)
            out[c][N-1-r] = g[r][c];
    return out;
}

template <int N>
int GameLogic<N>::rotFor(Direction d) {
    switch (d) {
        case Direction::Left:  return 0;  // Nenhuma rotação — slideLeft direto
        case Direction::Right: return 2;  // 180° → slideLeft → 180° (desfaz)
        case Direction::Up:    return 3;  // 270° CW = 90° CCW → slideLeft → 90° CW
        case Direction::Down:  return 1;  //  90° CW → slideLeft → 270° CW
        default:               return 0;
    }
}

template <int N>
bool GameLogic<N>::checkWin() const {
    for (const auto& row : grid_)
        for (int v : row)
            if (v >= WIN_TILE) return true;
    return false;
}

template <int N>
bool GameLogic<N>::checkGameOver() const {
    // Condição 1: existe célula vazia? → ainda pode jogar
    for (const auto& row : grid_)
        for (int v : row)
            if (!v) return false;

    // Condição 2: existe fusão horizontal possível?
    for (int r = 0; r < N; r++)
        for (int c = 0; c < N-1; c++)
            if (grid_[r][c] == grid_[r][c+1]) return false;

    // Condição 3: existe fusão vertical possível?
    for (int r = 0; r < N-1; r++)
        for (int c = 0; c < N; c++)
            if (grid_[r][c] == grid_[r+1][c]) return false;

    return true;  // Tabuleiro cheio e sem fusões → game over
}
